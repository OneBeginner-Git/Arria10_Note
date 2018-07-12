# Linux驱动

## SPI 驱动

>参考博客园博客总结写成：https://www.cnblogs.com/jason-lu/articles/3165327.html

| 层级   |     描述            | 
|--|------------------|
|内核层|内核层驱动与平台无关，抽象出了SPI控制器层的相同部分然后提供了统一的API给SPI设备层来使用
|设备层|一个SPI控制器以platform_device的形式注册进内核,并且调用spi_register_board_info函数注册了spi_board_info结构
Linux设备模型常识告诉我们,当系统中注册了一个名为"spi_davinci"的==platform_device==时,同时又注册了一个名为"spi_davinci"的==platform_driver==.那么就会执行这里的probe回调.这里我们来看davinci_spi_probe函数.

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
2. `dm365_init_spi0`在arch/arm/mach-davinci/dm365.c中

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

	spi_register_board_info(info, len);

	platform_device_register(&dm365_spi0_device);
}

//
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
		list_add_tail(&bi->list, &board_list);
		list_for_each_entry(master, &spi_master_list, list)
			spi_match_master_to_boardinfo(master, &bi->board_info);
		mutex_unlock(&board_lock);
	}

	return 0;
}
```

```java
//platform_driver 在 spi-davinci.c
static struct platform_driver davinci_spi_driver = {
	.driver = {
		.name = "spi_davinci",
		.of_match_table = of_match_ptr(davinci_spi_of_match),
	},
	.probe = davinci_spi_probe,
	.remove = davinci_spi_remove,
};

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
};
```


