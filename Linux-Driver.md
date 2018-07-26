# Linux驱动

## device tree 设备树

### Device Tree基本概念和作用

在内核源码中，存在大量对板级细节信息描述的代码。这些代码充斥在/arch/arm/plat-xxx和/arch/arm/mach-xxx目录，对内核而言这些platform设备、resource、i2c_board_info、spi_board_info以及各种硬件的platform_data绝大多数纯属垃圾冗余代码。为了解决这一问题，ARM内核版本3.x之后引入了原先在Power PC等其他体系架构已经使用的Flattened Device Tree。

“A data structure by which bootloaders pass hardware layout to Linux in a device-independent manner, simplifying hardware probing.”开源文档中对设备树的描述是，一种描述硬件资源的数据结构，它通过bootloader将硬件资源传给内核，使得内核和硬件资源描述相对独立(也就是说*.dtb文件由Bootloader读入内存，之后由内核来解析)。

Device Tree可以描述的信息包括CPU的数量和类别、内存基地址和大小、总线和桥、外设连接、中断控制器和中断使用情况、GPIO控制器和GPIO使用情况、Clock控制器和Clock使用情况。

另外，设备树对于可热插拔的热备不进行具体描述，它只描述用于控制该热插拔设备的控制器。

设备树的主要优势：对于同一SOC的不同主板，只需更换设备树文件.dtb即可实现不同主板的无差异支持，而无需更换内核文件。

注：要使得3.x之后的内核支持使用设备树，除了内核编译时需要打开相对应的选项外，bootloader也需要支持将设备树的数据结构传给内核。

### dts、dtsi文件的基本语法

DTS文件的内容包括一系列节点以及描述节点的属性。

“/”为root节点。在一个.dts文件中，有且仅有一个root节点；在root节点下有“node1”，“node2”子节点，称root为“node1”和“node2”的parent节点，除了root节点外，每个节点有且仅有一个parent；其中子节点node1下还存在子节点“child-nodel1”和“child-node2”。

注：如果看过内核/arch/arm/boot/dts目录的读者看到这可能有一个疑问。在每个.dsti和.dts中都会存在一个“/”根节点，那么如果在一个设备树文件中include一个.dtsi文件，那么岂不是存在多个“/”根节点了么。其实不然，编译器DTC在对.dts进行编译生成dtb时，会对node进行合并操作，最终生成的dtb只有一个root node。Dtc会进行合并操作这一点从属性上也可以得到验证。这个稍后做讲解。

在节点的{}里面是描述该节点的属性（property），即设备的特性。它的值是多样化的：

1.它可以是字符串string，如`compatible = "spidev";`；也可能是字符串数组string-list，如`compatible = "altr,a10sycon-gpio";`

2.它也可以是32 bit unsigned integers，如cell，整形用<>表示`reg = <1>;`

3.它也可以是binary data，十六进制用[]表示`a-byte-data-property = [0x01 0x23 0x03];`

4.它也可能是空，如`gpio-controller;`

## 板级文件--board.c

### 宏定义-MACHINE_START
```cpp
MACHINE_START(DAVINCI_DM365_EVM, "DaVinci DM365 EVM")
	.atag_offset	= 0x100,
	.map_io		= dm365_evm_map_io,
	.init_irq	= davinci_irq_init,
	.init_time	= davinci_timer_init,
	.init_machine	= dm365_evm_init,
	.init_late	= davinci_init_late,
	.dma_zone_size	= SZ_128M,
	.restart	= davinci_restart,
MACHINE_END
```

### resource类型

平台设备主要是提供设备资源和平台数据给平台驱动，resource为设备资源数组，类型有IORESOURCE_IO、IORESOURCE_MEM、IORESOURCE_IRQ、IORESOURCE_DMA、IORESOURCE_DMA。下面是一个网卡芯片DM9000的外设资源：

```cpp
static struct resource dm9000_resources[] = {
    [0] = {
        .start        = S3C64XX_PA_DM9000,
        .end        = S3C64XX_PA_DM9000 + 3,
        .flags        = IORESOURCE_MEM,
    },
    [1] = {
        .start        = S3C64XX_PA_DM9000 + 4,
        .end        = S3C64XX_PA_DM9000 + S3C64XX_SZ_DM9000 - 1,
        .flags        = IORESOURCE_MEM,
    },
    [2] = {
        .start        = IRQ_EINT(7),
        .end        = IRQ_EINT(7),
        .flags        = IORESOURCE_IRQ | IRQF_TRIGGER_HIGH,
    },
};
```

dm9000_resources里面有三个设备资源，第一个为IORESOURCE_MEM类型，指明了第一个资源内存的起始地址为S3C64XX_PA_DM9000结束地址为S3C64XX_PA_DM9000 + 3，第二个同样为IORESOURCE_MEM类型，指明了第二个资源内存的起始地址为S3C64XX_PA_DM9000 + 4结束地址为S3C64XX_PA_DM9000 + S3C64XX_SZ_DM9000 - 1，第三个为IORESOURCE_IRQ类型，指明了中断号为IRQ_EINT(7)。


## SPI 驱动

>参考博客园博客总结写成：https://www.cnblogs.com/jason-lu/articles/3165327.html

| 层级   |     描述            | 
|--|------------------|
|内核层|内核层驱动与平台无关，抽象出了SPI控制器层的相同部分然后提供了统一的API给SPI设备层来使用
|设备层|一个SPI控制器以platform_device的形式注册进内核,并且调用spi_register_board_info函数注册了spi_board_info结构

### 调用过程

1. 平台应用文件`arch/arm/mach-davinci/board-dm365-evm.c`中，起手函数是`dm365_evm_init(void)`，对dm365平台进行初始化。在初始化函数中，进行各个外设的初始化操作，其中对SPI设备的初始化由`dm365_init_spi0`实现，函数的一个重要参数就是`dm365_evm_spi_info`，它的数据类型是 **struct spi_board_info**：

```cpp
static __init void dm365_evm_init(void)
   {
       ……
       dm365_init_spi0(BIT(0), dm365_evm_spi_info,
               ARRAY_SIZE(dm365_evm_spi_info));
   }

```
```cpp
static struct spi_board_info mx51_3ds_spi_nor_device[] = {
            {
             .modalias = "m25p80",
             .max_speed_hz = 25000000,      /* max spi clock (SCK) speed in HZ */
             .bus_num = 1,
             .chip_select = 1,
             .mode = SPI_MODE_0,
             .platform_data = NULL,},
}
```
  
一个struct spi_board_info对象对应一个spi设备。上面代码中：

`.modalias = "m25p80"`, 
spi设备名字，设备驱动探测时会用到该项；

`.max_speed_hz = 25000000`, 
记录了这个spi设备支持的最大速度；

`.bus_num = 1`, 
记录了该spi设备是连接在哪个spi总线上的，所在总线的编号；

`.chip_select = 1`, 
该spi设备的片选编号；

`.mode = SPI_MODE_0`, 
和这个spi设备支持spi总线的工作模式。

2. `dm365_init_spi0`在arch/arm/mach-davinci/dm365.c中，设备移植时的重要工作就是向spi_board_info结构体中填充内容，先看看spi初始化函数的实现。

```java
void __init dm365_init_spi0(unsigned chipselect_mask,
		const struct spi_board_info *info, unsigned len)
{
	davinci_cfg_reg(DM365_SPI0_SCLK);
	davinci_cfg_reg(DM365_SPI0_SDI);
	davinci_cfg_reg(DM365_SPI0_SDO);

	/* not all slaves will be wired up */
	if (chipselect_mask & BIT(0))
		davinci_cfg_reg(DM365_SPI0_SDENA0);
	if (chipselect_mask & BIT(1))
		davinci_cfg_reg(DM365_SPI0_SDENA1);

	spi_register_board_info(info, len);//将spi从设备的信息添加到板级信息链表中去

	platform_device_register(&dm365_spi0_device);
}
```

- 可以看到info参数最终是传递给了`spi_register_board_info()`使用，这个函数可以注册所有需要的spi从设备。函数里还有一个局部结构体指针变量--`struct boardinfo`，定义在drivers/spi/spi.c中。这个结构是一个板级相关信息链表,就是说它是一些描述spi_device的信息的集合.结构体boardinfo管理多个结构体spi_board_info,结构体spi_board_info中挂在SPI总线上的设备的平台信息.一个结构体spi_board_info对应着一个SPI设备spi_device.
同时我们也看到了,函数中出现的board_list和spi_master_list都是全局的链表,它们分别记录了系统中所有的boardinfo和所有的spi_master.

```java
/*向特定板卡注册SPI设备
*
*参数：@info: spi 设备描述符
*	   @n:设备数量
*
*/

int spi_register_board_info(struct spi_board_info const *info, unsigned n)
{
	struct boardinfo *bi;
	int i;

	if (!n)
		return -EINVAL;

	bi = kzalloc(n * sizeof(*bi), GFP_KERNEL);
	if (!bi)
		return -ENOMEM;

	for (i = 0; i < n; i++, bi++, info++) {
		struct spi_master *master;

		memcpy(&bi->board_info, info, sizeof(*info));
		mutex_lock(&board_lock);
		list_add_tail(&bi->list, &board_list); /*添加到板级描述符链表*/
		list_for_each_entry(master, &spi_master_list, list)/*将SPI主机控制类链表所有的节点匹配板级信息的设备初始化*/
		spi_match_master_to_boardinfo(master, &bi->board_info);
		mutex_unlock(&board_lock);
	}

	return 0;
}

	/* SPI devices should normally not be created by SPI device drivers; that
	* would make them board-specific.  Similarly with SPI master drivers.
	* Device registration normally goes into like arch/.../mach.../board-YYY.c
	* with other readonly (flashable) information about mainboard devices.
	*/
struct boardinfo {
	struct list_head	list;
	struct spi_board_info	board_info;
};

```
接下来`platform_device_register()`用来注册平台设备，参数就是platform_device类型的结构体变量

```java
/**
 * platform_device_register - add a platform-level device
 * @pdev: platform device we're adding
 */
int platform_device_register(struct platform_device *pdev)
{
	device_initialize(&pdev->dev);
	arch_setup_pdev_archdata(pdev);
	return platform_device_add(pdev);
}
EXPORT_SYMBOL_GPL(platform_device_register);
```

Linux设备模型常识告诉我们,当系统中注册了一个名为"spi_davinci"的==platform_device==时,同时又注册了一个名为"spi_davinci"的==platform_driver==.那么就会执行这里的probe回调.

```java
//platform_device 在 dm365.c
static struct platform_device dm365_spi0_device = {
	.name = "spi_davinci",
	.id = 0,
	.dev = {
		.dma_mask = &dm365_spi0_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &dm365_spi0_pdata,
	},
	.num_resources = ARRAY_SIZE(dm365_spi0_resources),

	.resource = dm365_spi0_resources,
};

//platform_driver 在 spi-davinci.c
static struct platform_driver davinci_spi_driver = {
	.driver = {
		.name = "spi_davinci",
		.of_match_table = of_match_ptr(davinci_spi_of_match),
	},
	.probe = davinci_spi_probe,
	.remove = davinci_spi_remove,
};

```

**相对应的，对于altera平台的设备：**

```spi_platform_driver``` 位于spi-altera.c中

```cpp

static struct platform_driver altera_spi_driver = {
	.probe = altera_spi_probe,
	.remove = altera_spi_remove,
	.driver = {
		.name = DRV_NAME,
		.pm = NULL,
		.of_match_table = of_match_ptr(altera_spi_match),
	},
}

static struct platform_driver altera_spi_driver = {
	.probe = altera_spi_probe,
	.remove = altera_spi_remove,
	.driver = {
		.name = DRV_NAME,
		.pm = NULL,
		.of_match_table = of_match_ptr(altera_spi_match),
	},
}
	
```


这里我们来看davinci_spi_probe函数.




