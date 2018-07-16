#ifndef __SOCFPGA_H
#define __SOCFPGA_H

#include <linux/clk.h>
#include <linux/videodev2.h>
#include <linux/davinci_emac.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/platform_data/davinci_asp.h>
#include <linux/platform_data/edma.h>
#include <linux/platform_data/keyscan-davinci.h>
#include <mach/hardware.h>


void __init a10_init_spi0(unsigned chipselect_mask,
		const struct spi_board_info *info, unsigned len);

#endif