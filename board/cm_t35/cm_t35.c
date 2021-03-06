/*
 * (C) Copyright 2011 CompuLab, Ltd. <www.compulab.co.il>
 *
 * Authors: Mike Rapoport <mike@compulab.co.il>
 *	    Igor Grinberg <grinberg@compulab.co.il>
 *
 * Derived from omap3evm and Beagle Board by
 *	Manikandan Pillai <mani.pillai@ti.com>
 *	Richard Woodruff <r-woodruff2@ti.com>
 *	Syed Mohammed Khasim <x0khasim@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.
 */

#include <common.h>
#include <status_led.h>
#include <netdev.h>
#include <net.h>
#include <i2c.h>
#include <mmc.h>
#include <twl4030.h>
#include <env_multi.h>
#include <nand.h>
#include <linux/compiler.h>

#include <asm/io.h>
#include <asm/arch/mem.h>
#include <asm/arch/mux.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>

#include "eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

/*
 **************************************************
 * DUGen2 specific 
 **************************************************
 */

static void du_set_muxconf(void)
{
	/* note, this table overrides some of the common functions 
	 * we limit modifications to the common functions to ease merging
	 * with new versions
	 */

	MUX_VAL(CP(UART1_RX),		(IEN  | PTD | DIS | M0)); /*UART1_RX*/
	MUX_VAL(CP(UART1_TX),		(IEN  | PTD | DIS | M0)); /*UART1_TX*/
	MUX_VAL(CP(UART1_CTS),		(IEN  | PTD | DIS | M0)); /*UART1_CTS*/
	MUX_VAL(CP(UART1_CTS),		(IEN  | PTD | DIS | M0)); /*UART1_CTS*/
	MUX_VAL(CP(UART1_RTS),		(IEN  | PTD | DIS | M0)); /*UART1_RTS*/

	MUX_VAL(CP(DSS_DATA4),		(IEN  | PTD | DIS | M4)); /*LCD_SHTDN_N*/
	MUX_VAL(CP(SYS_CLKOUT1),	(IEN  | PTD | EN  | M4)); /*DVI_PDn*/

	MUX_VAL(CP(GPMC_A4),		(IEN  | PTD | EN  | M4)); /*USB_MFI_RESET*/
	MUX_VAL(CP(GPMC_A6),		(IEN  | PTU | EN  | M4)); /*USB_HUB_RESET*/
	MUX_VAL(CP(HDQ_SIO),            (IEN  | PTD | DIS | M4)); /*OMAP_DUG_7*/
	MUX_VAL(CP(CAM_D10),            (IEN  | PTD | EN  | M4)); /*AUDIO_SDn*/

	MUX_VAL(CP(GPMC_NCS3),          (IEN  | PTU | EN  | M0)); /*GPMC_CS3 (UART)*/
	MUX_VAL(CP(GPMC_NCS4),		(IEN  | PTU | EN  | M0)); /*GPMC_CS4 (UART)*/
	MUX_VAL(CP(GPMC_NCS7),		(IEN  | PTU | EN  | M0)); /*GPMC_CS7 (UART)*/

	MUX_VAL(CP(MCBSP1_DR),          (IEN  | PTD | EN  | M4)); /*GPMC_OE (buffer enable, not bus
								    strobe*/
	MUX_VAL(CP(GPMC_CLK),           (IEN  | PTU | EN  | M4)); /*8T245_OEn*/

	MUX_VAL(CP(MCBSP1_CLKX),        (IEN  | PTD | EN  | M4)); /*DU_GPIO_UART_A_IRQ*/
	MUX_VAL(CP(MCBSP1_FSX),         (IEN  | PTD | EN  | M4)); /*DU_GPIO_UART_B_IRQ*/
	MUX_VAL(CP(MCBSP1_CLKR),        (IEN  | PTD | EN  | M4)); /*DU_GPIO_UART_C_IRQ*/
	MUX_VAL(CP(JTAG_EMU1),          (IEN  | PTD | EN  | M4)); /*DU_GPIO_UART_D_IRQ*/
}

#define DU_GPIO_MFI_RESET	37
#define DU_GPIO_USB_RESET_N	39
#define DU_GPIO_I2C3_SDA	185
#define DU_GPIO_I2C3_SCL	184
#define DU_GPIO_OMAP_DUG_7	170
#define DU_GPIO_GPMC_OE         159
#define DU_GPIO_8T245_OE_N	59
#define DU_GPIO_UART_A_IRQ	162
#define DU_GPIO_UART_B_IRQ	161
#define DU_GPIO_UART_C_IRQ	156
#define DU_GPIO_UART_D_IRQ	31

static void du_reset_usb_hub(void)
{
	printf("DU: Reset USB Hub\n");

	// set I2C3 signals low, toggle reset, and the
	// set I2C3 signals back to I2C
	MUX_VAL(CP(I2C3_SCL),		(IEN  | PTU | EN  | M4)); /*I2C3_SCL*/
	MUX_VAL(CP(I2C3_SDA),		(IEN  | PTU | EN  | M4)); /*I2C3_SDA*/

	gpio_direction_output(DU_GPIO_USB_RESET_N, 1);
	gpio_direction_output(DU_GPIO_I2C3_SCL, 0);
	gpio_direction_output(DU_GPIO_I2C3_SDA, 0);

	udelay(100);
	gpio_set_value(DU_GPIO_USB_RESET_N, 0);
	udelay(100);
	gpio_set_value(DU_GPIO_USB_RESET_N, 1);
	udelay(100);
	gpio_set_value(DU_GPIO_I2C3_SCL, 1);
	gpio_set_value(DU_GPIO_I2C3_SDA, 1);
	
	MUX_VAL(CP(I2C3_SCL),		(IEN  | PTU | EN  | M0)); /*I2C3_SCL*/
	MUX_VAL(CP(I2C3_SDA),		(IEN  | PTU | EN  | M0)); /*I2C3_SDA*/

	gpio_direction_output(DU_GPIO_OMAP_DUG_7, 1);
}

#define UART_GPMC_CONFIG1                      0x00011000
#define UART_GPMC_CONFIG2                      0x001F1F01
#define UART_GPMC_CONFIG3                      0x00080803
#define UART_GPMC_CONFIG4                      0x1D091D09
#define UART_GPMC_CONFIG5                      0x041D1F1F
#define UART_GPMC_CONFIG6                      0x1D0904C4

#define UART1_ADDR		0x1000000
#define UART2_ADDR		0x2000000
#define UART3_ADDR		0x3000000

static u32 gpmc_uart_config[] = {
	UART_GPMC_CONFIG1,
	UART_GPMC_CONFIG2,
	UART_GPMC_CONFIG3,
	UART_GPMC_CONFIG4,
	UART_GPMC_CONFIG5,
	UART_GPMC_CONFIG6,
};

static void du_init()
{
	printf("DU: setting up UART chip selects\n");

	// enable UART bus buffers

	gpio_direction_output(DU_GPIO_GPMC_OE, 1);
	gpio_direction_output(DU_GPIO_8T245_OE_N, 0);

	enable_gpmc_cs_config(gpmc_uart_config, &gpmc_cfg->cs[3],
			      UART1_ADDR, GPMC_SIZE_16M);
	enable_gpmc_cs_config(gpmc_uart_config, &gpmc_cfg->cs[4],
			      UART2_ADDR, GPMC_SIZE_16M);
	enable_gpmc_cs_config(gpmc_uart_config, &gpmc_cfg->cs[7],
			      UART3_ADDR, GPMC_SIZE_16M);
}


/*
 **************************************************
 * End DUGen2 specific 
 **************************************************
 */

const omap3_sysinfo sysinfo = {
	DDR_DISCRETE,
	"CM-T3x board",
	"NAND",
};

static u32 gpmc_net_config[GPMC_MAX_REG] = {
	NET_GPMC_CONFIG1,
	NET_GPMC_CONFIG2,
	NET_GPMC_CONFIG3,
	NET_GPMC_CONFIG4,
	NET_GPMC_CONFIG5,
	NET_GPMC_CONFIG6,
	0
};

static u32 gpmc_nand_config[GPMC_MAX_REG] = {
	SMNAND_GPMC_CONFIG1,
	SMNAND_GPMC_CONFIG2,
	SMNAND_GPMC_CONFIG3,
	SMNAND_GPMC_CONFIG4,
	SMNAND_GPMC_CONFIG5,
	SMNAND_GPMC_CONFIG6,
	0,
};

#ifdef CONFIG_BOARD_EARLY_INIT_F
/*
 * Routine: board_init
 * Description: hardware init.
 */
int board_early_init_f(void)
{
#if 0
	enum env_multi_dev env_name = ENV_NAND;

	if (get_boot_type() & 0x20)
		env_name = ENV_MMC;

	env_multi_set_current(env_name, 0);
#endif

	return 0;
}
#endif

/*
 * Routine: board_init
 * Description: hardware init.
 */
int board_init(void)
{
	gpmc_init(); /* in SRAM or SDRAM, finish GPMC */
	du_init();

#if 0
	enable_gpmc_cs_config(gpmc_nand_config, &gpmc_cfg->cs[0],
			      CONFIG_SYS_NAND_BASE, GPMC_SIZE_16M);
#endif

	/* board id for Linux */
	if (get_cpu_family() == CPU_OMAP34XX)
		gd->bd->bi_arch_number = MACH_TYPE_CM_T35;
	else
		gd->bd->bi_arch_number = MACH_TYPE_CM_T3730;

	/* boot param addr */
	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100);

#if defined(CONFIG_STATUS_LED) && defined(STATUS_LED_BOOT)
	status_led_set(STATUS_LED_BOOT, STATUS_LED_ON);
#endif

	du_reset_usb_hub();

	return 0;
}

static u32 cm_t3x_rev;

/*
 * Routine: get_board_rev
 * Description: read system revision
 */
u32 get_board_rev(void)
{
	if (!cm_t3x_rev)
		cm_t3x_rev = cm_t3x_eeprom_get_board_rev();

	return cm_t3x_rev;
};

/*
 * Routine: misc_init_r
 * Description: display die ID
 */
int misc_init_r(void)
{
	u32 board_rev = get_board_rev();
	u32 rev_major = board_rev / 100;
	u32 rev_minor = board_rev - (rev_major * 100);

	if ((rev_minor / 10) * 10 == rev_minor)
		rev_minor = rev_minor / 10;

	printf("PCB:   %u.%u\n", rev_major, rev_minor);
	dieid_num_r();

	return 0;
}


/*
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers specific to the
 *		hardware. Many pins need to be moved from protect to primary
 *		mode.
 */
static void cm_t3x_set_common_muxconf(void)
{
	/* SDRC */
	MUX_VAL(CP(SDRC_D0),		(IEN  | PTD | DIS | M0)); /*SDRC_D0*/
	MUX_VAL(CP(SDRC_D1),		(IEN  | PTD | DIS | M0)); /*SDRC_D1*/
	MUX_VAL(CP(SDRC_D2),		(IEN  | PTD | DIS | M0)); /*SDRC_D2*/
	MUX_VAL(CP(SDRC_D3),		(IEN  | PTD | DIS | M0)); /*SDRC_D3*/
	MUX_VAL(CP(SDRC_D4),		(IEN  | PTD | DIS | M0)); /*SDRC_D4*/
	MUX_VAL(CP(SDRC_D5),		(IEN  | PTD | DIS | M0)); /*SDRC_D5*/
	MUX_VAL(CP(SDRC_D6),		(IEN  | PTD | DIS | M0)); /*SDRC_D6*/
	MUX_VAL(CP(SDRC_D7),		(IEN  | PTD | DIS | M0)); /*SDRC_D7*/
	MUX_VAL(CP(SDRC_D8),		(IEN  | PTD | DIS | M0)); /*SDRC_D8*/
	MUX_VAL(CP(SDRC_D9),		(IEN  | PTD | DIS | M0)); /*SDRC_D9*/
	MUX_VAL(CP(SDRC_D10),		(IEN  | PTD | DIS | M0)); /*SDRC_D10*/
	MUX_VAL(CP(SDRC_D11),		(IEN  | PTD | DIS | M0)); /*SDRC_D11*/
	MUX_VAL(CP(SDRC_D12),		(IEN  | PTD | DIS | M0)); /*SDRC_D12*/
	MUX_VAL(CP(SDRC_D13),		(IEN  | PTD | DIS | M0)); /*SDRC_D13*/
	MUX_VAL(CP(SDRC_D14),		(IEN  | PTD | DIS | M0)); /*SDRC_D14*/
	MUX_VAL(CP(SDRC_D15),		(IEN  | PTD | DIS | M0)); /*SDRC_D15*/
	MUX_VAL(CP(SDRC_D16),		(IEN  | PTD | DIS | M0)); /*SDRC_D16*/
	MUX_VAL(CP(SDRC_D17),		(IEN  | PTD | DIS | M0)); /*SDRC_D17*/
	MUX_VAL(CP(SDRC_D18),		(IEN  | PTD | DIS | M0)); /*SDRC_D18*/
	MUX_VAL(CP(SDRC_D19),		(IEN  | PTD | DIS | M0)); /*SDRC_D19*/
	MUX_VAL(CP(SDRC_D20),		(IEN  | PTD | DIS | M0)); /*SDRC_D20*/
	MUX_VAL(CP(SDRC_D21),		(IEN  | PTD | DIS | M0)); /*SDRC_D21*/
	MUX_VAL(CP(SDRC_D22),		(IEN  | PTD | DIS | M0)); /*SDRC_D22*/
	MUX_VAL(CP(SDRC_D23),		(IEN  | PTD | DIS | M0)); /*SDRC_D23*/
	MUX_VAL(CP(SDRC_D24),		(IEN  | PTD | DIS | M0)); /*SDRC_D24*/
	MUX_VAL(CP(SDRC_D25),		(IEN  | PTD | DIS | M0)); /*SDRC_D25*/
	MUX_VAL(CP(SDRC_D26),		(IEN  | PTD | DIS | M0)); /*SDRC_D26*/
	MUX_VAL(CP(SDRC_D27),		(IEN  | PTD | DIS | M0)); /*SDRC_D27*/
	MUX_VAL(CP(SDRC_D28),		(IEN  | PTD | DIS | M0)); /*SDRC_D28*/
	MUX_VAL(CP(SDRC_D29),		(IEN  | PTD | DIS | M0)); /*SDRC_D29*/
	MUX_VAL(CP(SDRC_D30),		(IEN  | PTD | DIS | M0)); /*SDRC_D30*/
	MUX_VAL(CP(SDRC_D31),		(IEN  | PTD | DIS | M0)); /*SDRC_D31*/
	MUX_VAL(CP(SDRC_CLK),		(IEN  | PTD | DIS | M0)); /*SDRC_CLK*/
	MUX_VAL(CP(SDRC_DQS0),		(IEN  | PTD | DIS | M0)); /*SDRC_DQS0*/
	MUX_VAL(CP(SDRC_DQS1),		(IEN  | PTD | DIS | M0)); /*SDRC_DQS1*/
	MUX_VAL(CP(SDRC_DQS2),		(IEN  | PTD | DIS | M0)); /*SDRC_DQS2*/
	MUX_VAL(CP(SDRC_DQS3),		(IEN  | PTD | DIS | M0)); /*SDRC_DQS3*/
	MUX_VAL(CP(SDRC_CKE0),		(IDIS | PTU | EN  | M0)); /*SDRC_CKE0*/
	MUX_VAL(CP(SDRC_CKE1),		(IDIS | PTD | DIS | M7)); /*SDRC_CKE1*/

	/* GPMC */
	MUX_VAL(CP(GPMC_A1),		(IDIS | PTU | EN  | M0)); /*GPMC_A1*/
	MUX_VAL(CP(GPMC_A2),		(IDIS | PTU | EN  | M0)); /*GPMC_A2*/
	MUX_VAL(CP(GPMC_A3),		(IDIS | PTU | EN  | M0)); /*GPMC_A3*/
	MUX_VAL(CP(GPMC_A4),		(IDIS | PTU | EN  | M0)); /*GPMC_A4*/
	MUX_VAL(CP(GPMC_A5),		(IDIS | PTU | EN  | M0)); /*GPMC_A5*/
	MUX_VAL(CP(GPMC_A6),		(IDIS | PTU | EN  | M0)); /*GPMC_A6*/
	MUX_VAL(CP(GPMC_A7),		(IDIS | PTU | EN  | M0)); /*GPMC_A7*/
	MUX_VAL(CP(GPMC_A8),		(IDIS | PTU | EN  | M0)); /*GPMC_A8*/
	MUX_VAL(CP(GPMC_A9),		(IDIS | PTU | EN  | M0)); /*GPMC_A9*/
	MUX_VAL(CP(GPMC_A10),		(IDIS | PTU | EN  | M0)); /*GPMC_A10*/
	MUX_VAL(CP(GPMC_D0),		(IEN  | PTU | EN  | M0)); /*GPMC_D0*/
	MUX_VAL(CP(GPMC_D1),		(IEN  | PTU | EN  | M0)); /*GPMC_D1*/
	MUX_VAL(CP(GPMC_D2),		(IEN  | PTU | EN  | M0)); /*GPMC_D2*/
	MUX_VAL(CP(GPMC_D3),		(IEN  | PTU | EN  | M0)); /*GPMC_D3*/
	MUX_VAL(CP(GPMC_D4),		(IEN  | PTU | EN  | M0)); /*GPMC_D4*/
	MUX_VAL(CP(GPMC_D5),		(IEN  | PTU | EN  | M0)); /*GPMC_D5*/
	MUX_VAL(CP(GPMC_D6),		(IEN  | PTU | EN  | M0)); /*GPMC_D6*/
	MUX_VAL(CP(GPMC_D7),		(IEN  | PTU | EN  | M0)); /*GPMC_D7*/
	MUX_VAL(CP(GPMC_D8),		(IEN  | PTU | EN  | M0)); /*GPMC_D8*/
	MUX_VAL(CP(GPMC_D9),		(IEN  | PTU | EN  | M0)); /*GPMC_D9*/
	MUX_VAL(CP(GPMC_D10),		(IEN  | PTU | EN  | M0)); /*GPMC_D10*/
	MUX_VAL(CP(GPMC_D11),		(IEN  | PTU | EN  | M0)); /*GPMC_D11*/
	MUX_VAL(CP(GPMC_D12),		(IEN  | PTU | EN  | M0)); /*GPMC_D12*/
	MUX_VAL(CP(GPMC_D13),		(IEN  | PTU | EN  | M0)); /*GPMC_D13*/
	MUX_VAL(CP(GPMC_D14),		(IEN  | PTU | EN  | M0)); /*GPMC_D14*/
	MUX_VAL(CP(GPMC_D15),		(IEN  | PTU | EN  | M0)); /*GPMC_D15*/
	MUX_VAL(CP(GPMC_NCS0),		(IDIS | PTU | EN  | M0)); /*GPMC_nCS0*/

	/* SB-T35 Ethernet */
	MUX_VAL(CP(GPMC_NCS4),		(IEN  | PTU | EN  | M0)); /*GPMC_nCS4*/

	/* CM-T3x Ethernet */
	MUX_VAL(CP(GPMC_NCS5),		(IDIS | PTU | DIS | M0)); /*GPMC_nCS5*/
	MUX_VAL(CP(GPMC_CLK),		(IEN  | PTD | DIS | M4)); /*GPIO_59*/
	MUX_VAL(CP(GPMC_NADV_ALE),	(IDIS | PTD | DIS | M0)); /*nADV_ALE*/
	MUX_VAL(CP(GPMC_NOE),		(IDIS | PTD | DIS | M0)); /*nOE*/
	MUX_VAL(CP(GPMC_NWE),		(IDIS | PTD | DIS | M0)); /*nWE*/
	MUX_VAL(CP(GPMC_NBE0_CLE),	(IDIS | PTU | EN  | M0)); /*nBE0_CLE*/
	MUX_VAL(CP(GPMC_NBE1),		(IDIS | PTD | DIS | M4)); /*GPIO_61*/
	MUX_VAL(CP(GPMC_NWP),		(IEN  | PTD | DIS | M0)); /*nWP*/
	MUX_VAL(CP(GPMC_WAIT0),		(IEN  | PTU | EN  | M0)); /*WAIT0*/

	/* DSS */
	MUX_VAL(CP(DSS_PCLK),		(IDIS | PTD | DIS | M0)); /*DSS_PCLK*/
	MUX_VAL(CP(DSS_HSYNC),		(IDIS | PTD | DIS | M0)); /*DSS_HSYNC*/
	MUX_VAL(CP(DSS_VSYNC),		(IDIS | PTD | DIS | M0)); /*DSS_VSYNC*/
	MUX_VAL(CP(DSS_ACBIAS),		(IDIS | PTD | DIS | M0)); /*DSS_ACBIAS*/
	MUX_VAL(CP(DSS_DATA6),		(IDIS | PTD | DIS | M0)); /*DSS_DATA6*/
	MUX_VAL(CP(DSS_DATA7),		(IDIS | PTD | DIS | M0)); /*DSS_DATA7*/
	MUX_VAL(CP(DSS_DATA8),		(IDIS | PTD | DIS | M0)); /*DSS_DATA8*/
	MUX_VAL(CP(DSS_DATA9),		(IDIS | PTD | DIS | M0)); /*DSS_DATA9*/
	MUX_VAL(CP(DSS_DATA10),		(IDIS | PTD | DIS | M0)); /*DSS_DATA10*/
	MUX_VAL(CP(DSS_DATA11),		(IDIS | PTD | DIS | M0)); /*DSS_DATA11*/
	MUX_VAL(CP(DSS_DATA12),		(IDIS | PTD | DIS | M0)); /*DSS_DATA12*/
	MUX_VAL(CP(DSS_DATA13),		(IDIS | PTD | DIS | M0)); /*DSS_DATA13*/
	MUX_VAL(CP(DSS_DATA14),		(IDIS | PTD | DIS | M0)); /*DSS_DATA14*/
	MUX_VAL(CP(DSS_DATA15),		(IDIS | PTD | DIS | M0)); /*DSS_DATA15*/
	MUX_VAL(CP(DSS_DATA16),		(IDIS | PTD | DIS | M0)); /*DSS_DATA16*/
	MUX_VAL(CP(DSS_DATA17),		(IDIS | PTD | DIS | M0)); /*DSS_DATA17*/

	/* serial interface */
	MUX_VAL(CP(UART3_RX_IRRX),	(IEN  | PTD | DIS | M0)); /*UART3_RX*/
	MUX_VAL(CP(UART3_TX_IRTX),	(IDIS | PTD | DIS | M0)); /*UART3_TX*/

	/* mUSB */
	MUX_VAL(CP(HSUSB0_CLK),		(IEN  | PTD | DIS | M0)); /*HSUSB0_CLK*/
	MUX_VAL(CP(HSUSB0_STP),		(IDIS | PTU | EN  | M0)); /*HSUSB0_STP*/
	MUX_VAL(CP(HSUSB0_DIR),		(IEN  | PTD | DIS | M0)); /*HSUSB0_DIR*/
	MUX_VAL(CP(HSUSB0_NXT),		(IEN  | PTD | DIS | M0)); /*HSUSB0_NXT*/
	MUX_VAL(CP(HSUSB0_DATA0),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA0*/
	MUX_VAL(CP(HSUSB0_DATA1),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA1*/
	MUX_VAL(CP(HSUSB0_DATA2),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA2*/
	MUX_VAL(CP(HSUSB0_DATA3),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA3*/
	MUX_VAL(CP(HSUSB0_DATA4),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA4*/
	MUX_VAL(CP(HSUSB0_DATA5),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA5*/
	MUX_VAL(CP(HSUSB0_DATA6),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA6*/
	MUX_VAL(CP(HSUSB0_DATA7),	(IEN  | PTD | DIS | M0)); /*HSUSB0_DATA7*/

	/* I2C1 */
	MUX_VAL(CP(I2C1_SCL),		(IEN  | PTU | EN  | M0)); /*I2C1_SCL*/
	MUX_VAL(CP(I2C1_SDA),		(IEN  | PTU | EN  | M0)); /*I2C1_SDA*/
	/* I2C2 */
	MUX_VAL(CP(I2C2_SCL),		(IEN  | PTU | EN  | M0)); /*I2C2_SCL*/
	MUX_VAL(CP(I2C2_SDA),		(IEN  | PTU | EN  | M0)); /*I2C2_SDA*/
	/* I2C3 */
	MUX_VAL(CP(I2C3_SCL),		(IEN  | PTU | EN  | M0)); /*I2C3_SCL*/
	MUX_VAL(CP(I2C3_SDA),		(IEN  | PTU | EN  | M0)); /*I2C3_SDA*/

	/* control and debug */
	MUX_VAL(CP(SYS_32K),		(IEN  | PTD | DIS | M0)); /*SYS_32K*/
	MUX_VAL(CP(SYS_CLKREQ),		(IEN  | PTD | DIS | M0)); /*SYS_CLKREQ*/
	MUX_VAL(CP(SYS_NIRQ),		(IEN  | PTU | EN  | M0)); /*SYS_nIRQ*/
	MUX_VAL(CP(SYS_OFF_MODE),	(IEN  | PTD | DIS | M0)); /*OFF_MODE*/
	MUX_VAL(CP(SYS_CLKOUT1),	(IEN  | PTD | DIS | M0)); /*CLKOUT1*/
	MUX_VAL(CP(SYS_CLKOUT2),	(IDIS | PTU | DIS | M4)); /*green LED*/
	MUX_VAL(CP(JTAG_nTRST),		(IEN  | PTD | DIS | M0)); /*JTAG_nTRST*/
	MUX_VAL(CP(JTAG_TCK),		(IEN  | PTD | DIS | M0)); /*JTAG_TCK*/
	MUX_VAL(CP(JTAG_TMS),		(IEN  | PTD | DIS | M0)); /*JTAG_TMS*/
	MUX_VAL(CP(JTAG_TDI),		(IEN  | PTD | DIS | M0)); /*JTAG_TDI*/

	/* MMC1 */
	MUX_VAL(CP(MMC1_CLK),		(IDIS | PTU | EN  | M0)); /*MMC1_CLK*/
	MUX_VAL(CP(MMC1_CMD),		(IEN  | PTU | EN  | M0)); /*MMC1_CMD*/
	MUX_VAL(CP(MMC1_DAT0),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT0*/
	MUX_VAL(CP(MMC1_DAT1),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT1*/
	MUX_VAL(CP(MMC1_DAT2),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT2*/
	MUX_VAL(CP(MMC1_DAT3),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT3*/
}

static void cm_t35_set_muxconf(void)
{
	/* DSS */
	MUX_VAL(CP(DSS_DATA0),		(IDIS | PTD | DIS | M0)); /*DSS_DATA0*/
	MUX_VAL(CP(DSS_DATA1),		(IDIS | PTD | DIS | M0)); /*DSS_DATA1*/
	MUX_VAL(CP(DSS_DATA2),		(IDIS | PTD | DIS | M0)); /*DSS_DATA2*/
	MUX_VAL(CP(DSS_DATA3),		(IDIS | PTD | DIS | M0)); /*DSS_DATA3*/
	MUX_VAL(CP(DSS_DATA4),		(IDIS | PTD | DIS | M0)); /*DSS_DATA4*/
	MUX_VAL(CP(DSS_DATA5),		(IDIS | PTD | DIS | M0)); /*DSS_DATA5*/

	MUX_VAL(CP(DSS_DATA18),         (IDIS | PTD | DIS | M0)); /*DSS_DATA18*/
	MUX_VAL(CP(DSS_DATA19),         (IDIS | PTD | DIS | M0)); /*DSS_DATA19*/
	MUX_VAL(CP(DSS_DATA20),         (IDIS | PTD | DIS | M0)); /*DSS_DATA20*/
	MUX_VAL(CP(DSS_DATA21),         (IDIS | PTD | DIS | M0)); /*DSS_DATA21*/
	MUX_VAL(CP(DSS_DATA22),         (IDIS | PTD | DIS | M0)); /*DSS_DATA22*/
	MUX_VAL(CP(DSS_DATA23),         (IDIS | PTD | DIS | M0)); /*DSS_DATA23*/

	/* MMC1 */
	MUX_VAL(CP(MMC1_DAT4),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT4*/
	MUX_VAL(CP(MMC1_DAT5),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT5*/
	MUX_VAL(CP(MMC1_DAT6),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT6*/
	MUX_VAL(CP(MMC1_DAT7),		(IEN  | PTU | EN  | M0)); /*MMC1_DAT7*/
}

static void cm_t3730_set_muxconf(void)
{
	/* DSS */
	MUX_VAL(CP(DSS_DATA18),		(IDIS | PTD | DIS | M3)); /*DSS_DATA0*/
	MUX_VAL(CP(DSS_DATA19),		(IDIS | PTD | DIS | M3)); /*DSS_DATA1*/
	MUX_VAL(CP(DSS_DATA20),		(IDIS | PTD | DIS | M3)); /*DSS_DATA2*/
	MUX_VAL(CP(DSS_DATA21),		(IDIS | PTD | DIS | M3)); /*DSS_DATA3*/
	MUX_VAL(CP(DSS_DATA22),		(IDIS | PTD | DIS | M3)); /*DSS_DATA4*/
	MUX_VAL(CP(DSS_DATA23),		(IDIS | PTD | DIS | M3)); /*DSS_DATA5*/

	MUX_VAL(CP(SYS_BOOT0),		(IDIS | PTD | DIS | M3)); /*DSS_DATA18*/
	MUX_VAL(CP(SYS_BOOT1),		(IDIS | PTD | DIS | M3)); /*DSS_DATA19*/
	MUX_VAL(CP(SYS_BOOT3),		(IDIS | PTD | DIS | M3)); /*DSS_DATA20*/
	MUX_VAL(CP(SYS_BOOT4),		(IDIS | PTD | DIS | M3)); /*DSS_DATA21*/
	MUX_VAL(CP(SYS_BOOT5),		(IDIS | PTD | DIS | M3)); /*DSS_DATA22*/
	MUX_VAL(CP(SYS_BOOT6),		(IDIS | PTD | DIS | M3)); /*DSS_DATA23*/
}

void set_muxconf_regs(void)
{
	cm_t3x_set_common_muxconf();


	if (get_cpu_family() == CPU_OMAP34XX)
		cm_t35_set_muxconf();
	else
		cm_t3730_set_muxconf();

	du_set_muxconf();
}

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
	return omap_mmc_init(0, 0, 0);
}

/* environment size is 16K, we fit between the MBR and first partition */
#define MMC_ENV_OFFSET	0x400

int mmc_get_env_addr(struct mmc *mmc, u32 *env_addr)
{
	*env_addr = MMC_ENV_OFFSET;
	return 0;
}
#endif

/*
 * Routine: setup_net_chip_gmpc
 * Description: Setting up the configuration GPMC registers specific to the
 *		Ethernet hardware.
 */
static void setup_net_chip_gmpc(void)
{
	struct ctrl *ctrl_base = (struct ctrl *)OMAP34XX_CTRL_BASE;

	enable_gpmc_cs_config(gpmc_net_config, &gpmc_cfg->cs[5],
			      CM_T3X_SMC911X_BASE, GPMC_SIZE_16M);
	enable_gpmc_cs_config(gpmc_net_config, &gpmc_cfg->cs[4],
			      SB_T35_SMC911X_BASE, GPMC_SIZE_16M);

	/* Enable off mode for NWE in PADCONF_GPMC_NWE register */
	writew(readw(&ctrl_base->gpmc_nwe) | 0x0E00, &ctrl_base->gpmc_nwe);

	/* Enable off mode for NOE in PADCONF_GPMC_NADV_ALE register */
	writew(readw(&ctrl_base->gpmc_noe) | 0x0E00, &ctrl_base->gpmc_noe);

	/* Enable off mode for ALE in PADCONF_GPMC_NADV_ALE register */
	writew(readw(&ctrl_base->gpmc_nadv_ale) | 0x0E00,
		&ctrl_base->gpmc_nadv_ale);
}

#ifdef CONFIG_DRIVER_OMAP34XX_I2C
/*
 * Routine: reset_net_chip
 * Description: reset the Ethernet controller via TPS65930 GPIO
 */
static void reset_net_chip(void)
{
	/* Set GPIO1 of TPS65930 as output */
	twl4030_i2c_write_u8(TWL4030_CHIP_GPIO, 0x02,
				TWL4030_BASEADD_GPIO + 0x03);
	/* Send a pulse on the GPIO pin */
	twl4030_i2c_write_u8(TWL4030_CHIP_GPIO, 0x02,
				TWL4030_BASEADD_GPIO + 0x0C);
	udelay(1);
	twl4030_i2c_write_u8(TWL4030_CHIP_GPIO, 0x02,
				TWL4030_BASEADD_GPIO + 0x09);
	mdelay(40);
	twl4030_i2c_write_u8(TWL4030_CHIP_GPIO, 0x02,
				TWL4030_BASEADD_GPIO + 0x0C);
	mdelay(1);
}
#else
static inline void reset_net_chip(void) {}
#endif

#ifdef CONFIG_SMC911X
/*
 * Routine: handle_mac_address
 * Description: prepare MAC address for on-board Ethernet.
 */
static int handle_mac_address(void)
{
	unsigned char enetaddr[6];
	int rc;

	rc = eth_getenv_enetaddr("ethaddr", enetaddr);
	if (rc)
		return 0;

	rc = cm_t3x_eeprom_read_mac_addr(enetaddr);
	if (rc)
		return rc;

	if (!is_valid_ether_addr(enetaddr))
		return -1;

	return eth_setenv_enetaddr("ethaddr", enetaddr);
}


/*
 * Routine: board_eth_init
 * Description: initialize module and base-board Ethernet chips
 */
int board_eth_init(bd_t *bis)
{
	return 0;

	int rc = 0, rc1 = 0;

	setup_net_chip_gmpc();
	reset_net_chip();

	rc1 = handle_mac_address();
	if (rc1)
		printf("No MAC address found! ");

	rc1 = smc911x_initialize(0, CM_T3X_SMC911X_BASE);
	if (rc1 > 0)
		rc++;

	rc1 = smc911x_initialize(1, SB_T35_SMC911X_BASE);
	if (rc1 > 0)
		rc++;

	return rc;
}
#endif

void __weak get_board_serial(struct tag_serialnr *serialnr)
{
	/*
	 * This corresponds to what happens when we can communicate with the
	 * eeprom but don't get a valid board serial value.
	 */
	serialnr->low = 0;
	serialnr->high = 0;
};
