# Linux驱动

## device tree 设备树

### 


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




