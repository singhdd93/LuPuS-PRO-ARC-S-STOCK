/*
 * Copyright (C) 2009 Valentin Longchamp, EPFL Mobots group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/imx-uart.h>
#include <mach/iomux-mx3.h>
#include <mach/mmc.h>

#include "devices.h"

static unsigned int marxbot_pins[] = {
	/* SDHC2 */
	MX31_PIN_PC_PWRON__SD2_DATA3, MX31_PIN_PC_VS1__SD2_DATA2,
	MX31_PIN_PC_READY__SD2_DATA1, MX31_PIN_PC_WAIT_B__SD2_DATA0,
	MX31_PIN_PC_CD2_B__SD2_CLK, MX31_PIN_PC_CD1_B__SD2_CMD,
	MX31_PIN_ATA_DIOR__GPIO3_28, MX31_PIN_ATA_DIOW__GPIO3_29,
	/* CSI */
	MX31_PIN_CSI_D4__CSI_D4, MX31_PIN_CSI_D5__CSI_D5,
	MX31_PIN_CSI_D6__CSI_D6, MX31_PIN_CSI_D7__CSI_D7,
	MX31_PIN_CSI_D8__CSI_D8, MX31_PIN_CSI_D9__CSI_D9,
	MX31_PIN_CSI_D10__CSI_D10, MX31_PIN_CSI_D11__CSI_D11,
	MX31_PIN_CSI_D12__CSI_D12, MX31_PIN_CSI_D13__CSI_D13,
	MX31_PIN_CSI_D14__CSI_D14, MX31_PIN_CSI_D15__CSI_D15,
	MX31_PIN_CSI_HSYNC__CSI_HSYNC, MX31_PIN_CSI_MCLK__CSI_MCLK,
	MX31_PIN_CSI_PIXCLK__CSI_PIXCLK, MX31_PIN_CSI_VSYNC__CSI_VSYNC,
	MX31_PIN_GPIO3_0__GPIO3_0, MX31_PIN_GPIO3_1__GPIO3_1,
	MX31_PIN_TXD2__GPIO1_28,
	/* dsPIC resets */
	MX31_PIN_STXD5__GPIO1_21, MX31_PIN_SRXD5__GPIO1_22,
};

#define SDHC2_CD IOMUX_TO_GPIO(MX31_PIN_ATA_DIOR)
#define SDHC2_WP IOMUX_TO_GPIO(MX31_PIN_ATA_DIOW)

static int marxbot_sdhc2_get_ro(struct device *dev)
{
	return !gpio_get_value(SDHC2_WP);
}

static int marxbot_sdhc2_init(struct device *dev, irq_handler_t detect_irq,
		void *data)
{
	int ret;

	ret = gpio_request(SDHC2_CD, "sdhc-detect");
	if (ret)
		return ret;

	gpio_direction_input(SDHC2_CD);

	ret = gpio_request(SDHC2_WP, "sdhc-wp");
	if (ret)
		goto err_gpio_free;
	gpio_direction_input(SDHC2_WP);

	ret = request_irq(gpio_to_irq(SDHC2_CD), detect_irq,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"sdhc2-card-detect", data);
	if (ret)
		goto err_gpio_free_2;

	return 0;

err_gpio_free_2:
	gpio_free(SDHC2_WP);
err_gpio_free:
	gpio_free(SDHC2_CD);

	return ret;
}

static void marxbot_sdhc2_exit(struct device *dev, void *data)
{
	free_irq(gpio_to_irq(SDHC2_CD), data);
	gpio_free(SDHC2_WP);
	gpio_free(SDHC2_CD);
}

static struct imxmmc_platform_data sdhc2_pdata = {
	.get_ro	= marxbot_sdhc2_get_ro,
	.init	= marxbot_sdhc2_init,
	.exit	= marxbot_sdhc2_exit,
};

#define TRSLAT_RST_B	IOMUX_TO_GPIO(MX31_PIN_STXD5)
#define DSPICS_RST_B	IOMUX_TO_GPIO(MX31_PIN_SRXD5)

static void dspics_resets_init(void)
{
	if (!gpio_request(TRSLAT_RST_B, "translator-rst")) {
		gpio_direction_output(TRSLAT_RST_B, 1);
		gpio_export(TRSLAT_RST_B, false);
	}

	if (!gpio_request(DSPICS_RST_B, "dspics-rst")) {
		gpio_direction_output(DSPICS_RST_B, 1);
		gpio_export(DSPICS_RST_B, false);
	}
}

/*
 * system init for baseboard usage. Will be called by mx31moboard init.
 */
void __init mx31moboard_marxbot_init(void)
{
	printk(KERN_INFO "Initializing mx31marxbot peripherals\n");

	mxc_iomux_setup_multiple_pins(marxbot_pins, ARRAY_SIZE(marxbot_pins),
		"marxbot");

	dspics_resets_init();

	mxc_register_device(&mxcsdhc_device1, &sdhc2_pdata);
}
