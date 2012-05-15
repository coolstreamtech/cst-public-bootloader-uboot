/*
 * Board specific very low level init
 *
 * (C) Copyright 2008
 * Coolstream International Limited
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <version.h>
#include <asm/arch/nevis.h>

#define PIN_CONFIG0_REG_DEFAULT		0xBFFFEFFE	/* 1111 1111 1111 1111 1110 1111 1111 1110			*/ /* 23apr2010 SMC1 on */
#define PIN_ALT_FUNC_REG_DEFAULT	0x00000010	/* 0000 0000 0000 0000 0000 0000 0001 0000			*/ /* 23apr2010 UART1 on */

#define GPIO_PIN_MUX0_REG_DEFAULT	0x0100D000	/* 0000 0001 0000 0000 1101 0000 0000 0000	PIO 031 ... 000 */ /* 22jun2009: PIO24 to PWM */
#define GPIO_PIN_MUX1_REG_DEFAULT	0x00000080	/* 0000 0000 0000 0000 0000 0000 1000 0000	PIO 063 ... 032 */
#define GPIO_PIN_MUX2_REG_DEFAULT	0x000021C0	/* 0000 0000 0000 0000 0010 0001 1100 0000	PIO 095 ... 064 */
#define GPIO_PIN_MUX3_REG_DEFAULT	0xFFBFC380	/* 1111 1111 1011 1111 1100 0011 1000 0000	PIO 127 ... 096 */
#define GPIO_PIN_MUX4_REG_DEFAULT	0xFFFC0006	/* 1111 1111 1111 1100 0000 0000 0000 0110	PIO 159 ... 128 */
#define GPIO_PIN_MUX5_REG_DEFAULT	0x0006F209	/* 0000 0000 0000 0110 1111 0010 0000 1001	PIO 191 ... 160 */
#define GPIO_PIN_MUX6_REG_DEFAULT	0x00000002	/* 0000 0000 0000 0000 0000 0000 0000 0010	PIO 223 ... 192 */

#define GPIO_PIN_SMUX0_REG_DEFAULT	0x00000000	/* 0000 0000 0000 0000 0000 0000 0000 0000	PIO 031 ... 000 */
#define GPIO_PIN_SMUX1_REG_DEFAULT	0x00003000	/* 0000 0000 0000 0000 0011 0000 0000 0000	PIO 063 ... 032 */
#define GPIO_PIN_SMUX2_REG_DEFAULT	0x3F7C0000	/* 0011 1111 0111 1100 0000 0000 0000 0000	PIO 095 ... 064 */
#define GPIO_PIN_SMUX3_REG_DEFAULT	0x20000000	/* 0010 0000 0000 0000 0000 0000 0000 0000	PIO 127 ... 096 */
#define GPIO_PIN_SMUX4_REG_DEFAULT	0x00000000	/* 0000 0000 0000 0000 0000 0000 0000 0000	PIO 159 ... 128 */
#define GPIO_PIN_SMUX5_REG_DEFAULT	0x00000000	/* 0000 0000 0000 0000 0000 0000 0000 0000	PIO 191 ... 160 */
#define GPIO_PIN_SMUX6_REG_DEFAULT	0x00000000	/* 0000 0000 0000 0000 0000 0000 0000 0000	PIO 223 ... 192 */

#define ITC_INTENABLE1_REG_DEFAULT	0x00000000
#define ITC_INTENABLE2_REG_DEFAULT	0x00000000

#if (CONFIG_ARM9_SETUP_LEGACY_MODE == 1)
#define ARM9_VECTOR_OFFSET		CONFIG_ARM9_OFFSET_LEGACY
#else
#define ARM9_VECTOR_OFFSET		CONFIG_ARM9_OFFSET
#endif

_TEXT_BASE:
	.word	TEXT_BASE	/* sdram load addr from config.mk */

PLL_NEVIS:
	.word 0xE0440050			, 0x00120508 /* DIV_MUX_CTRL0 */ /* ASX -> PLL0(432MHZ) / 8 == 54Mhz */
	.word 0xE0440054			, 0x06010002 /* DIV_MUX_CTRL1 */
	.word 0xE0440058			, 0x07020003 /* DIV_MUX_CTRL2 */
	.word 0xE044005C			, 0x000B000B /* DIV_MUX_CTRL3 */
	.word 0xE0440060			, 0x07020004 /* DIV_MUX_CTRL4 */
	.word 0xE0440064			, 0x00000702 /* DIV_MUX_CTRL5 */
	.word 0xE0440068			, 0x00630058 /* DIV_MUX_CTRL6 */
	.word 0xE044006C			, 0x00030003 /* DIV_MUX_CTRL7 */
	.word 0xE0440070			, 0x00000803 /* DIV_MUX_CTRL8 */
	.word 0xE0440074			, 0x00000002 /* DIV_MUX_CTRL9 */
	.word 0xE0440078			, 0x00000003 /* DIV_MUX_CTRL10 */
	.word 0xE044007C			, 0x05080506 /* DIV_MUX_CTRL11 */
	.word 0xE0440080			, 0x00080512 /* DIV_MUX_CTRL12 */
	.word 0xE0440084			, 0x08030003 /* DIV_MUX_CTRL13 */
	.word 0xE0440088			, 0x08120812 /* DIV_MUX_CTRL14 */
	.word 0xE044008C			, 0x00030003 /* DIV_MUX_CTRL15 */

	.word 0xE0440000			, 0x13CCCCCD /* MPG0_INTFRAC	*/
	.word 0xE0440010			, 0x13C7BC7C /* HD_INTFRAC	*/
	.word 0xE0440018			, 0x10624DD3 /* AUD_INTFRAC	*/
	.word 0xE0440020			, 0x0E666666 /* PLL0_INTFRAC	*/
	/* 0x13CCCCCD = 594 Mhz, 0x0F000000 = 450 Mhz */
	.word 0xE0440028			, 0x0F000000 /* PLL1_INTFRAC	*/
	.word 0xE0440030			, 0x11C71C70 /* PLL2_INTFRAC	*/
	.word 0xE0440004			, 0x02214242 /* MPG0_CTRL	*/
	.word 0xE0440014			, 0x02214242 /* HD_CTRL		*/
	.word 0xE044001C			, 0x02514242 /* AUD_CTRL	*/
	.word 0xE0440024			, 0x02117242 /* PLL0_CTRL	*/
	.word 0xE044002C			, 0x02117242 /* PLL1_CTRL	*/
	.word 0xE0440034			, 0x02117242 /* PLL2_CTRL	*/
	.word 0xE0440048			, 0xCF0004A3 /* FENRUS_PLL_CTRL */
	.word 0xE0440070			, 0x00001803 /* DIV_MUX_CTRL8	*/

	.word 0xE04400AC			, 0x249F0064 /* SMART0_PLL_CTRL */
	.word 0xE04400B0			, 0x249F0064 /* SMART1_PLL_CTRL */

	.word 0xE04400E4			, 0x00000064 /* PIO_CLK_DIV0 */
	.word 0xE04400E8			, 0x00000064 /* PIO_CLK_DIV1 */
	.word 0xE04400EC			, 0x00000064 /* PIO_CLK_DIV2 */
	.word 0xE04400E0			, 0x00000888 /* PIO_CLK_MUX */

	.word 0xE0440228			, 0x07777770 /* HS_CLK_MUX_CTRL0 */
	.word 0xE044022C			, 0x00005500 /* HS_CLK_MUX_CTRL1 */
	.word 0xE0440204			, 0x03E8000A /* HS1_CLK_DIV_CTRL */
	.word 0xE0440208			, 0x03E8000A /* HS2_CLK_DIV_CTRL */
	.word 0xE044020C			, 0x03E8000A /* HS3_CLK_DIV_CTRL */
	.word 0xE0440210			, 0x03E8000A /* HS4_CLK_DIV_CTRL */
	.word 0xE0440214			, 0x03E8000A /* HS5_CLK_DIV_CTRL */
	.word 0xE0440218			, 0x03E8000A /* HS6_CLK_DIV_CTRL */
	.word 0xDEADBEEF			, 0xDEADBEEF

MC_COMMON:
	.word MC_CTL_REG			, 0x00000000
	.word MC_PRIORITY_PCI_REG		, 0x02010002
	.word MC_PRIORITY_SATA1_REG		, 0x03010006
	.word MC_PRIORITY_DMA0_REG		, 0x02010004
	.word MC_PRIORITY_CRYPT_REG		, 0x03010002
	.word MC_PRIORITY_JTAG_REG		, 0x00010002
	.word MC_PRIORITY_JPEG_REG		, 0x05010001
	.word MC_PRIORITY_SBM_REG		, 0x02010004
	.word MC_PRIORITY_SSP_REG		, 0x03010000
	.word MC_PRIORITY_SATA0_REG		, 0x03010006
	.word MC_PRIORITY_TSP0_REG		, 0x02010007
	.word MC_PRIORITY_TSP1_REG		, 0x02010007
	.word MC_PRIORITY_TSR0_REG		, 0x02010007
	.word MC_PRIORITY_TSR1_REG		, 0x02010007
	.word MC_PRIORITY_BSP0_REG		, 0x02010004
	.word MC_PRIORITY_BSP1_REG		, 0x02010004
	.word MC_PRIORITY_AMP_REG		, 0x00010005
	.word MC_PRIORITY_APP_REG		, 0x01010007
	.word MC_PRIORITY_IMP0_REG		, 0x02010005
	.word MC_PRIORITY_IMP1_REG		, 0x02010005
	.word MC_PRIORITY_DPROC_INST_REG	, 0x02010007
	.word MC_PRIORITY_DPROC_DATA_REG	, 0x02010007
	.word MC_PRIORITY_CPM_REG		, 0x01010007
	.word MC_PRIORITY_DMA1_REG		, 0x02010004
	.word MC_PRIORITY_DNT_REG		, 0x00020006
	.word MC_PRIORITY_DRM00_REG		, 0x03040006
	.word MC_PRIORITY_DRM01_REG		, 0x03040006
	.word MC_PRIORITY_DRM02_REG		, 0x03060007
	.word MC_PRIORITY_DRM03_REG		, 0x03000006
	.word MC_PRIORITY_DRM10_REG		, 0x03040006
	.word MC_PRIORITY_DRM11_REG		, 0x03040006
	.word MC_PRIORITY_DRM12_REG		, 0x03060007
	.word MC_PRIORITY_DRM13_REG		, 0x03000006
	.word MC_PRIORITY_POSTP0_REG		, 0x01020004
	.word MC_PRIORITY_POSTP1_REG		, 0x00020004
	.word MC_PRIORITY_GXA_REG		, 0x09030000
	.word MC_PRIORITY_NU12_REG		, 0x00010000
	.word MC_PRIORITY_MSDF0_REG		, 0x04030005
	.word MC_PRIORITY_MSDP0_REG		, 0x04030005
	.word MC_PRIORITY_MSDF1_REG		, 0x04070005
	.word MC_PRIORITY_MSDP1_REG		, 0x04070005
	.word MC_PRIORITY_MBF0_REG		, 0x00010000
	.word MC_PRIORITY_MBP0_REG		, 0x00010000
	.word MC_PRIORITY_MBF1_REG		, 0x00010000
	.word MC_PRIORITY_MBP1_REG		, 0x00010000
	.word MC_PRIORITY_ARM_INST		, 0x00000007
	.word MC_PRIORITY_ARM_DATA		, 0x00000007
	.word MC_PRIORITY_USB0			, 0x00000004
	.word MC_PRIORITY_USB1			, 0x00000004
	.word MC_PRIORITY_USB2			, 0x00000004
	.word MC_PRIORITY_EMAC0			, 0x00000007
	.word MC_PRIORITY_EMAC1			, 0x00000007
	.word MC_PRIORITY_MCARD0		, 0x00000007
	.word MC_PRIORITY_BUS4_WR_REG		, 0x00000007
	.word MC_ATTR0_START_REG		, 0x00000000
	.word MC_ATTR1_START_REG		, 0x00000000
	.word MC_ATTR2_START_REG		, 0x00000000
	.word MC_ATTR3_START_REG		, 0x00000000
	.word MC_ATTR4_START_REG		, 0x00000000
	.word MC_ATTR5_START_REG		, 0x00000000
	.word MC_ATTR6_START_REG		, 0x00000000
	.word MC_ATTR7_START_REG		, 0x00000000
	.word MC_ATTR8_START_REG		, 0x00000000
	.word MC_ATTR9_START_REG		, 0x00000000
	.word MC_ATTR10_START_REG		, 0x00000000
	.word MC_ATTR11_START_REG		, 0x00000000
	.word MC_ATTR12_START_REG		, 0x00000000
	.word MC_ATTR13_START_REG		, 0x00000000
	.word MC_ATTR14_START_REG		, 0x00000000
	.word MC_ATTR15_START_REG		, 0x00000000
	.word MC_ATTR0_END_REG			, 0x00000000
	.word MC_ATTR1_END_REG			, 0x00000000
	.word MC_ATTR2_END_REG			, 0x00000000
	.word MC_ATTR3_END_REG			, 0x00000000
	.word MC_ATTR4_END_REG			, 0x00000000
	.word MC_ATTR5_END_REG			, 0x00000000
	.word MC_ATTR6_END_REG			, 0x00000000
	.word MC_ATTR7_END_REG			, 0x00000000
	.word MC_ATTR8_END_REG			, 0x00000000
	.word MC_ATTR9_END_REG			, 0x00000000
	.word MC_ATTR10_END_REG			, 0x00000000
	.word MC_ATTR11_END_REG			, 0x00000000
	.word MC_ATTR12_END_REG			, 0x00000000
	.word MC_ATTR13_END_REG			, 0x00000000
	.word MC_ATTR14_END_REG			, 0x00000000
	.word MC_ATTR15_END_REG			, 0x00000000
	.word MC_ATTR_WP_CTL_REG		, 0x00000000
	.word MC_ATTR_INTR_EN_REG		, 0x00000000
	.word MC_ATTR_WP_LOG0_REG		, 0x00000000
	.word MC_ATTR_WP_LOG1_REG		, 0x00000000
	.word MC_ATTR_CONTENT_RD_LOG0_REG	, 0x00000000
	.word MC_ATTR_CONTENT_RD_LOG1_REG	, 0x00000000
	.word MC_TILE0_START_REG		, 0x00000000
	.word MC_TILE1_START_REG		, 0x00000000
	.word MC_TILE2_START_REG		, 0x00000000
	.word MC_TILE3_START_REG		, 0x00000000
	.word MC_TILE4_START_REG		, 0x00000000
	.word MC_TILE5_START_REG		, 0x00000000
	.word MC_TILE6_START_REG		, 0x00000000
	.word MC_TILE7_START_REG		, 0x00000000
	.word MC_TILE0_END_REG			, 0x00000000
	.word MC_TILE1_END_REG			, 0x00000000
	.word MC_TILE2_END_REG			, 0x00000000
	.word MC_TILE3_END_REG			, 0x00000000
	.word MC_TILE4_END_REG			, 0x00000000
	.word MC_TILE5_END_REG			, 0x00000000
	.word MC_TILE6_END_REG			, 0x00000000
	.word MC_TILE7_END_REG			, 0x00000000
	.word MC_TILE0_CTL_REG			, 0x00000000
	.word MC_TILE1_CTL_REG			, 0x00000000
	.word MC_TILE2_CTL_REG			, 0x00000000
	.word MC_TILE3_CTL_REG			, 0x00000000
	.word MC_TILE4_CTL_REG			, 0x00000000
	.word MC_TILE5_CTL_REG			, 0x00000000
	.word MC_TILE6_CTL_REG			, 0x00000000
	.word MC_TILE7_CTL_REG			, 0x00000000
	.word MC_RING_CTL_REG			, 0x00000000
	.word MC_RING_FREQUENCY_REG		, 0x00000000
	.word MC_EVENT_SEL0_REG			, 0x00000000
	.word MC_EVENT_SEL1_REG			, 0x00000000
	.word MC_EVENT_CTL_REG			, 0x00000000
	.word MC_EVENT_TIM_REG			, 0x00000000
	.word MC_DBG_CTL_REG			, 0x00000000
	.word 0xDEADBEEF			, 0xDEADBEEF

MC_NEVIS_C:
	/* Memory controller #0 settings */
	.word MC_CFG0_REG			, 0x1AF40111
	.word MC_CFG1_REG			, 0x01010308
	.word MC_CFG2_REG			, 0x0A070807
	.word MC_PARAM0_REG			, 0x120D0404
	.word MC_PARAM1_REG			, 0x030D0704
	.word MC_PARAM2_REG			, 0x062704B9
	.word MC_PARAM3_REG			, 0x05020101
	.word MC_PARAM4_REG			, 0x057C00C7
	.word MC_MRS_REG			, 0x00000853
	.word MC_EMRS1_REG			, 0x00000006
	.word MC_EMRS2_REG			, 0x00000000
	.word MC_EMRS3_REG			, 0x00000000
	.word MC_DLL_CTL_CLOCK_REG		, 0x15151515
	.word MC_DLL_CTL_SKEW_REG		, 0x99999999
	.word MC_DLL_CTL_OVERRIDE0_REG		, 0x00000000
	.word MC_DLL_CTL_OVERRIDE1_REG		, 0x00000000
	.word MC_INTR_EN_REG			, 0x00000000
	.word MC_INTR_STAT_REG			, 0xFFFFFFFF
	.word MC_THRESH0_REG			, 0x18203040
	.word MC_THRESH1_REG			, 0x00040810
	.word MC_CFG3_REG			, 0x00F00704
	.word MC_PARAM5_REG			, 0x0B00F231
	.word MC_PARAM6_REG			, 0x00506210
	.word MC_DRIVE0_REG			, 0x00000006
	.word MC_DRIVE1_REG			, 0x00000000
	.word MC_DRIVE2_REG			, 0x00000006
	.word MC_DRIVE3_REG			, 0x00000000
	.word MC_DRIVE4_REG			, 0x00000000
	.word MC_O_DELAY_REG			, 0x003FFFFF
	.word MC_RING_CTL_REG			, 0x00000000
	.word MC_RING_FREQUENCY_REG		, 0x00000000

	/* Memory controller #1 settings */
	.word MC_CFG0_M1_REG			, 0x1AF40111
	.word MC_CFG1_M1_REG			, 0x01010308
	.word MC_PARAM0_M1_REG			, 0x120D0404
	.word MC_PARAM1_M1_REG			, 0x030D0704
	.word MC_PARAM2_M1_REG			, 0x062704B9
	.word MC_PARAM3_M1_REG			, 0x05020101
	.word MC_PARAM4_M1_REG			, 0x057C00C7
	.word MC_MRS_M1_REG			, 0x00000853
	.word MC_EMRS1_M1_REG			, 0x00000006
	.word MC_EMRS2_M1_REG			, 0x00000000
	.word MC_EMRS3_M1_REG			, 0x00000000
	.word MC_PARAM5_M1_REG			, 0x0B00F231
	.word MC_PARAM6_M1_REG			, 0x00506210
	.word MC_O_DELAY_M1_REG			, 0x003FFFFF
	/* Memory PLL settings (310MHz) */
	.word MEM_PLL_CTRL0_REG			, 0x00414032
	.word MEM_PLL_CTRL1_REG			, 0x0000943A	/* .word 0xE0500804, 0x0000143A */
	.word MEM_PLL_CTRL2_REG			, 0x0000003E	/* .word 0xE0500808, 0x86120043 */
	.word MEM_PLL_CTRL3_REG			, 0x00000000
	.word MEM_PLL_CTRL1_REG			, (0x0000943A & 0xFFFD) /* Reset Memory controller */
	.word MEM_PLL_CTRL1_REG			, 0x0000943A	/* Release again */
	.word 0xDEADBEEF			, 0xDEADBEEF

MC_NEVIS_D:
	/* Memory controller #0 settings */
	.word MC_CFG0_REG			, 0x1AF40111
	.word MC_CFG1_REG			, 0x21010308
	.word MC_CFG2_REG			, 0x0A070807
	.word MC_PARAM0_REG			, 0x120D0404
	.word MC_PARAM1_REG			, 0x030D0704
	.word MC_PARAM2_REG			, 0x062704B9
	.word MC_PARAM3_REG			, 0x05020101
	.word MC_PARAM4_REG			, 0x057C00C7
	.word MC_MRS_REG			, 0x00000853
	.word MC_EMRS1_REG			, 0x00000006
	.word MC_EMRS2_REG			, 0x00000000
	.word MC_EMRS3_REG			, 0x00000000
	.word MC_DLL_CTL_CLOCK_REG		, 0x16151515
	.word MC_DLL_CTL_SKEW_REG		, 0x99999999
	.word MC_DLL_CTL_OVERRIDE0_REG		, 0x00000000
	.word MC_DLL_CTL_OVERRIDE1_REG		, 0x00000000
	.word MC_INTR_EN_REG			, 0x00000000
	.word MC_INTR_STAT_REG			, 0xFFFFFFFF
	.word MC_THRESH0_REG			, 0x18203040
	.word MC_THRESH1_REG			, 0x00040810
	.word MC_CFG3_REG			, 0x00F00704
	.word MC_PARAM5_REG			, 0x0B00F231
	.word MC_PARAM6_REG			, 0x00606210
	.word MC_DRIVE0_REG			, 0x00000006
	.word MC_DRIVE1_REG			, 0x00000000
	.word MC_DRIVE2_REG			, 0x00000006
	.word MC_DRIVE3_REG			, 0x00000000
	.word MC_DRIVE4_REG			, 0x00000000
	.word MC_O_DELAY_REG			, 0x0016FFFF
	.word MC_RING_CTL_REG			, 0x00000000
	.word MC_RING_FREQUENCY_REG		, 0x00000000

	/* Memory controller #1 settings */
	.word MC_CFG0_M1_REG			, 0x1AF40111
	.word MC_CFG1_M1_REG			, 0x21010308
	.word MC_PARAM0_M1_REG			, 0x120D0404
	.word MC_PARAM1_M1_REG			, 0x030D0704
	.word MC_PARAM2_M1_REG			, 0x062704B9
	.word MC_PARAM3_M1_REG			, 0x05020101
	.word MC_PARAM4_M1_REG			, 0x057C00C7
	.word MC_MRS_M1_REG			, 0x00000853
	.word MC_EMRS1_M1_REG			, 0x00000006
	.word MC_EMRS2_M1_REG			, 0x00000000
	.word MC_EMRS3_M1_REG			, 0x00000000
	.word MC_PARAM5_M1_REG			, 0x0B00F231
	.word MC_PARAM6_M1_REG			, 0x00606210
	.word MC_O_DELAY_M1_REG			, 0x0016FFFF
	/* Memory PLL settings (310MHz) */
	.word MEM_PLL_CTRL0_REG			, 0x00414032
	.word MEM_PLL_CTRL1_REG			, 0x0000943A
	.word MEM_PLL_CTRL2_REG			, 0x0000003E
	.word MEM_PLL_CTRL3_REG			, 0x00000000
	.word MEM_PLL_CTRL1_REG			, (0x0000943A & 0xFFFD) /* Reset Memory controller */
	.word MEM_PLL_CTRL1_REG			, 0x0000943A	/* Release again */
	.word 0xDEADBEEF			, 0xDEADBEEF

/*******************************************************************************/
/* small helper to unlock the locked special function registers */
UnlockRegs:
	mov	r10, lr	/* save callers address */

	ldr	r2, =LOCKCMD_REG
	mov	r3, #0x00
	str	r3, [r2]
	mov	r3, #0xF8
	str	r3, [r2]
	mov	r3, #0x2B
	str	r3, [r2]
	ldr	r2, =LOCKSTAT_REG
	ldr	r3, [r2]
	bic	r3, r3,r0
	str	r3, [r2]

	ldr	r2, =LOCKCMD_REG
	mov	r3, #0x00
	str	r3, [r2]

	mov pc, r10	/* return to caller */
	.ltorg

/*******************************************************************************/
LockRegs:
	mov r10, lr	/* save callers address */

	ldr	r2, =LOCKCMD_REG
	mov	r3, #0x00
	str	r3, [r2]
	mov	r3, #0xF8
	str	r3, [r2]
	mov	r3, #0x2B
	str	r3, [r2]
	ldr	r2, =LOCKSTAT_REG 
	ldr	r3, [r2]
	orr	r3, r3, r0
	str	r3, [r2]

	ldr	r2, =LOCKCMD_REG
	mov	r3, #0x00
	str	r3, [r2]

	mov	pc, r10	/* return to caller */
	.ltorg

/*******************************************************************************/

.globl ReadChipIDs
ReadChipIDs:
	mov r10, lr	/* save callers address */

	ldr	r1, =PCI_CFG_ADDR_REG
	ldr	r2, =PCI_CFG_DATA_REG
	ldr	r3, =SCRATCH_4_REG
	ldr	r4, =SCRATCH_5_REG
	mov	r0, #0x00		@ PCI-Config register 0x00 contains the chip ID (bit 31...16) and the chip vendor (bit 15...0)
	str	r0, [r1]
	ldr	r0, [r2]
	str	r0, [r3]
	ldr	r0, =0x08		@ PCI-Config register 0x08 contains the major (bit 7..4) and minor (bit 3...0) chip revision
	str	r0, [r1]
	ldr	r0, [r2]
	ldr	r1, =0x000000FF		@ remove upper 24 bit (contains the PCI class code)
	and	r0, r0, r1
	str	r0, [r4]

	mov pc, r10	/* return to caller */
	.ltorg

/*******************************************************************************/
/* helper function to load a bunch of registers */

.globl LoadRegArray
LoadRegArray:
	mov	r10, lr	/* save callers address */

LoadRegArrayLoop:
	ldr	r1, [r0]
	add	r0, r0, #4
	ldr	r2, [r0]
	add	r0, r0, #4
	cmp	r1, r2
	strne	r2, [r1]
	bne	LoadRegArrayLoop

	mov pc, r10	/* return to caller */

InitClockSpeed:
	mov	r10, lr	/* save callers address */

	/*
	 * CPU clock setting.
	 *
	 * Check which revision we have and update the CPU-clock to the max 
	 * supported value (Rev. A/B/C -> 450/550 MHz, Rev. D -> 594 MHz).
	 * Notes:	SCRATCH_4_REG contains the vendor ID and 
	 *		SCRATCH_5_REG contains the chip revision code.
	 */

	ldr	r0, =SCRATCH_5_REG	/* r0 = register address */
	ldr	r1, [r0]		/* r1 = register value */
	and	r1, #0xFF
	cmp	r1, #0x2F		/* Rev. A = 0x0x, B = 0x1x, C = 0x2x */
	ble	CPU_REV_ABC
	ldr	r1, =0x13CCCCCD		/* PL1_INTFRAC = 594 */
	b	CPU_CLK_SET
CPU_REV_ABC:
	ldr	r0, =0xE044014C		/* undocumented hardware erata register */
	ldr	r1, [r0]
	ands	r1, #0x00800000		/* mask bit 23 */
	bne	CPU_CLOCK_DONE
	ldr	r1, =0x12555555		/* PL1_INTFRAC = 550 */
CPU_CLK_SET:
	ldr	r0, =0xE0440028		/* PL1_INTFRAC_REG */
	str	r1, [r0]
CPU_CLOCK_DONE:
	mov	pc, r10 /* return to caller */
	.ltorg

/*******************************************************************************/
/* main function for really low level init (before any C enviroment) called
   from cpu/<CPU>/Start.S							   */

.globl lowlevel_init
lowlevel_init:
	mov	r13, lr

	ldr	r0, =HSX_ASX_BRIDGE_CTRL_REG
	ldr	r1, [r0]
	orr	r1,r1, #0x00000001
	str	r1, [r0]

	/* Setup the primary GPIO MUX configuration */
	ldr	r0, =PIN_CONFIG0_REG
	ldr	r1, =PIN_CONFIG0_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =PIN_ALT_FUNC_REG
	ldr	r1, =PIN_ALT_FUNC_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_MUX0_REG
	ldr	r1, =GPIO_PIN_MUX0_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_MUX1_REG
	ldr	r1, =GPIO_PIN_MUX1_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_MUX2_REG
	ldr	r1, =GPIO_PIN_MUX2_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_MUX3_REG
	ldr	r1, =GPIO_PIN_MUX3_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_MUX4_REG
	ldr	r1, =GPIO_PIN_MUX4_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_MUX5_REG
	ldr	r1, =GPIO_PIN_MUX5_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_MUX6_REG
	ldr	r1, =GPIO_PIN_MUX6_REG_DEFAULT
	str	r1, [r0]

	/* Setup the secondary MUX registers */
	ldr	r0, =GPIO_PIN_SMUX0_REG
	ldr	r1, =GPIO_PIN_SMUX0_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_SMUX1_REG
	ldr	r1, =GPIO_PIN_SMUX1_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_SMUX2_REG
	ldr	r1, =GPIO_PIN_SMUX2_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_SMUX3_REG
	ldr	r1, =GPIO_PIN_SMUX3_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_SMUX4_REG
	ldr	r1, =GPIO_PIN_SMUX4_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_SMUX5_REG
	ldr	r1, =GPIO_PIN_SMUX5_REG_DEFAULT
	str	r1, [r0]

	ldr	r0, =GPIO_PIN_SMUX6_REG
	ldr	r1, =GPIO_PIN_SMUX6_REG_DEFAULT
	str	r1, [r0]

	/* Remove FLASH shadow */
	ldr	r0, =REMAP_REG
	ldr	r1, =0x00000000
	str	r1, [r0]

	/* read the chip vendor, ID,revision */
	bl	ReadChipIDs

	/* Setup PLLs */
	mov	r0, #0x300
	bl	UnlockRegs

	adrl	r0, PLL_NEVIS
	bl	LoadRegArray

	bl	InitClockSpeed

	ldr	r0, =SREG_PLL_BYPASS_REG
	ldr	r1, =0x000005FF
	str	r1, [r0]

	/* MPG0 */
	ldr	r0, =AUDIO_CLK_SEL_REG
	ldr	r1, [r0]
	ldr	r2, =0x00FFFFFF 
	bic	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =AUDIO_DIVIDE0_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	ldr	r2, =0x30570000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x00000200
	orr	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =AUDIO_DIVIDE1_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	ldr	r2, =0x30570000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x00000200
	orr	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =AUDIO_DIVIDE2_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	ldr	r2, =0x30570000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x00000200
	orr	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =AUDIO_DIVIDE3_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	ldr	r2, =0x30570000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x00000200
	orr	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =AUDIO_DIVIDE4_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	ldr	r2, =0x30570000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x00000200
	orr	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =AUDIO_DIVIDE5_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	ldr	r2, =0x30570000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x00000200
	orr	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =MPG2_DIVIDE_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	ldr	r2, =0x2B030000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x000003E8
	orr	r1, r1, r2
	str	r1, [r0]

	ldr	r0, =MPG3_DIVIDE_REG
	ldr	r1, [r0]
	ldr	r2, =0xFFFF0000
	bic	r1, r1, r2
	mov	r2, #0x08000000
	orr	r1, r1, r2
	str	r1, [r0]
	ldr	r2, =0x0000FFFF
	bic	r1, r1, r2
	mov	r2, #0x000000E1
	orr	r1, r1, r2
	str	r1, [r0]

	/* wait for PLLs to lock */
	mov	r1, #0x000000FF
WFL:
	ldr	r0, =0xE0400018
	ldr	r0, [r0]
	and	r0, r0, r1
	cmp	r0, r1
	bne	WFL

	mov	r0, #0x300
	bl	LockRegs

	/* Exception vector table at 0x00000000 */
	mrc	p15, 0, r0, c1, c0
	and	r0, r0, #0xFFFFDFFF
	mcr	p15, 0, r0, c1, c0

	ldr	r2, =MC_CTL_REG /* Set the "START" bit to 0 to stop the memory controller */
	mov	r3, #0
	str	r3, [r2]

WaitForMCStop:
	ldr	r1, [r2]
	and	r1, r1, #0x000F0000
	cmp	r1, #0x000A0000
	bne	WaitForMCStop

	/* Setup DDR-Controller */
	adrl	r0, MC_COMMON
	bl	LoadRegArray

	ldr	r0, =SCRATCH_5_REG	/* r0 = register address */
	ldr	r1, [r0]		/* r1 = register value */
	and	r1, #0xF0
	cmp	r1, #0x20		/* Rev. A = 0x0x, B = 0x1x, C = 0x2x */
	beq	MEM_REVC
	cmp	r1, #0x30
	beq	MEM_REVD
	b	MEM_REVC
MEM_REVC:
	adrl	r0, MC_NEVIS_C
	bl	LoadRegArray
	b	PLL_LOCK
MEM_REVD:
	adrl	r0, MC_NEVIS_D
	bl	LoadRegArray
PLL_LOCK:
	/* wait for MC PLLs to lock */
	ldr	r2, =0x000001FF
WFMCL:
	ldr	r0, =0xE0400018
	ldr	r1, [r0]
	cmp	r1, r2
	bne	WFMCL

	ldr	r0, =HSX_0_ARBMODE_REG
	ldr	r1, =0x00000000
	str	r1, [R0]

	ldr	r0, =HSX_0_ARBMODE1_REG
	ldr	r1, =0x00000000
	str	r1, [r0]

	ldr	r0, =HSX_1_ARBMODE_REG
	ldr	r1, =0x00000000
	str	r1, [r0]

	ldr	r0, =HSX_1_ARBMODE1_REG
	ldr	r1, =0x00000000
	str	r1, [r0]

	ldr	r0, =HSX_2_ARBMODE_REG
	ldr	r1, =0x00000000
	str	r1, [r0]

	ldr	r0, =HSX_2_ARBMODE1_REG
	ldr	r1, =0x00000000
	str	r1, [r0]

	ldr	r0, =MEMORY_TIMER_REG
	ldr	r1, =0x0000000
	str	r1, [r0]

	ldr	r0, =MC_TILE_CFG_REG
	ldr	r1, =0x00001038 /* 0x00001036 0x00001038 */
	str	r1, [r0]

	ldr	r2, =MC_CTL_REG /* Set the "START" bit to 1 to load and use configured values */
	mov	r3, #0
	str	r3, [r2]
	mov	r3, #1
	str	r3, [r2]

	ldr	r2, =MC_CTL_REG /* we see PDN_STATE0 and PDN_STATE1 fields go to zero when MC is up */
WaitForMC:
	ldr	R1, [r2]
	ands	r3, R1, #0x000F0000
	bne	WaitForMC
	/* Debug settings */
	ldr	r1, =HSX_PIT_GENERAL_REG
	ldr	r0, =0x00000001
	str	r0, [r1]
	ldr	r1, =HSX_PIT_DATA_REG
	ldr	r0, =0x00000000
	str	r0, [r1]
	ldr	r1, =HSX_PIT_INSTR_REG
	ldr	r0, =0x00800000
	str	r0, [r1]

	/* RAM check */
	/* first clear addresses which we will check */
	ldr	r0, =0x00400000		@ first address to test for bank 0
	ldr	r1, =0x00000000
	ldr	r4, =0x20000000		@ limit the clear range to 512 MB 

	/* write zeros at all addresses which we will test */
init_loop:
	str	r1, [r0]
	add	r0, r0, #0x00400000
	cmp	r0, r4
	bne	init_loop

	ldr	r0, =0x00400000		@ first address to test
	ldr	r4, =0x10000000		@ limit the test to 256 MB 
	ldr	r5, =0x00000000

	/* hardware bank #0 */
test_loop_b0:
	ldr	r1, =0x55AA55AA;	@ test pattern
	str	r1, [r0]		@ write test pattern
	ldr	r2, [r0]		@ read back
	cmp	r1, r2			@ compare written and read value
	bne	not_pass_b0

	ldr	r1, =0xAA55AAB0;	@ test pattern
	str	r1, [r0]		@ write test pattern
	ldr	r2, [r0]		@ read back
	cmp	r1, r2			@ compare written and read value
	bne	not_pass_b0

	mov	r5, r0			@ copy current address to r5
ntest_loop_b0:
	add	r5, r5, #0x00400000
	ldr	r2, [r5]
	cmp	r1, r2
	beq	not_pass_b0
	cmp	r5, r4
	bne	ntest_loop_b0

	add	r0, r0, #0x00400000	@ increment the address to test by 4 MB
	cmp	r0, r4			@ compare current address with max address
	blt	test_loop_b0
not_pass_b0:
	ldr	r0, =SCRATCH_2_REG	@ save the size of bank 0
	str	r5, [r0]

	/* hardware bank #1 */
	ldr	r0, =0x10000000		@ first address to test for bank 1
	ldr	r4, =0x20000000		@ limit the test to 512 MB
	ldr	r5, =0x10000000

test_loop_b1:
	ldr	r1, =0xAA55AA55		@ test pattern
	str	r1, [r0]		@ write test pattern
	ldr	r2, [r0]		@ read back
	cmp	r1, r2			@ compare written and read value
	bne	not_pass_b1

	ldr	r1, =0x55AA55B1		@ test pattern
	str	r1, [r0]		@ write test pattern
	ldr	r2, [r0]		@ read back
	cmp	r1, r2			@ compare written and read value
	bne	not_pass_b1

	mov	r5, r0			@ copy current address to r5
ntest_loop_b1:
	add	r5, r5, #0x00400000
	ldr	r2, [r5]
	cmp	r1, r2
	beq	not_pass_b1
	cmp	r5, r4
	bne	ntest_loop_b1

	add	r0, r0, #0x00400000	@ increment the address to test by 4 MB
	cmp	r0, r4			@ compare current address with max address
	blt	test_loop_b1
not_pass_b1:
	sub	r5, r5, #0x10000000
	ldr	r0, =SCRATCH_3_REG	@ save the size of bank 1
	str	r5, [r0]

	/* Disable all Interrupts */
	ldr	r1, =ITC_INTENABLE1_REG_DEFAULT
	ldr	r0, =ITC_INTENABLE1_REG
	str	r1, [r0]

	ldr	r1, =ITC_INTENABLE2_REG_DEFAULT
	ldr	r0, =ITC_INTENABLE2_REG
	str	r1, [r0]

	/* enable the peripherial port */
	ldr	r0, =0xE8000009
	mcr	p15, 0, r0, c15, c2, 4

	/* back to arch calling code */
	mov	pc, r13
	/* the literal pools origin */
	.ltorg

/*
 * Some code for ARM 926. We assemble it here so we can use it in our
 * C code later.. ;)
 */
.globl arm926_spin_begin
arm926_spin_begin:
	ldr	r0, [pc, #(ARM9_VECTOR_OFFSET - 0x08)] /* consider the prefetch! */
	ldr	r1, [pc, #(ARM9_VECTOR_OFFSET - 0x08)] /* consider the prefetch! */
	cmp	r0, #0
	subeq	pc, pc, #0x14
	cmp	r1, #0
	subeq	pc, pc, #0x1C
	bx	r0
.globl arm926_spin_end
arm926_spin_end:
	/* the literal pools origin */
	.ltorg
