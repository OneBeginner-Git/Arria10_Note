# Linux驱动

## SPI 驱动

Linux设备模型常识告诉我们,当系统中注册了一个名为"spi_davinci"的==platform_device==时,同时又注册了一个名为"spi_davinci"的==platform_driver==.那么就会执行这里的probe回调.这里我们来看davinci_spi_probe函数.


```cpp
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
