# Linux驱动

## SPI 驱动
Linux设备模型常识告诉我们,当系统中注册了一个名为"spi_davinci"的platform_device时,同时又住了一个名为"spi_davinci"的platform_driver.那么就会执行这里的probe回调.这里我们来看davinci_spi_probe函数.