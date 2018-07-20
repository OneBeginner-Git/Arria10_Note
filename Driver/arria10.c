#include <linux/init.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/spi/spi.h>
#include <linux/platform_data/spi-davinci.h>

#include "socfpga-all.h"

static struct resource a10_spi0_resources[] = {
	/*{
		.start = 0x01c66000,
		.end   = 0x01c667ff,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_DM365_SPIINT0_0,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = 17,
		.flags = IORESOURCE_DMA,
	},
	{
		.start = 16,
		.flags = IORESOURCE_DMA,
	},*/
};

static struct platform_device a10_spi0_device = {
	.name = "spi_altera",
	.id = 0,
	.dev = {
		.dma_mask = &a10_spi0_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a10_spi0_pdata,
	},
	.num_resources = ARRAY_SIZE(a10_spi0_resources),
	.resource = a10_spi0_resources,
};

void __init a10_init_spi0(unsigned chipselect_mask,
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

	platform_device_register(&a10_spi0_device);
}