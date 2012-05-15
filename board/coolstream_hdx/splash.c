/*
 * (C) Copyright 2010
 * Coolstream Internation Limited
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <version.h>
#include <asm/arch/nevis.h>
#include <asm/arch/sys_proto.h>
#include <video_fb.h>

#define VIDEO_CLK_REG			(0xE0400000 + 0x40000 + 0xA8)
#define DRM_UNIT_OFFSET			(0x10000)
#define DRM_UNIT(unit)			(0xe0560000 + ((unit) * DRM_UNIT_OFFSET))
#define AFE_BASE			(0xE0540000 + 0x1000)
#define PHYSICAL_ADDRESS(a)		(a)
#define ALIGN_16(a) 			((unsigned long)((((unsigned long)(a)) + 15) & ~(16-1)))

extern u32 get_board_rev(void);

typedef struct {
	volatile u32 *reg;
	u32 val;
} reg_pair_t;

static const u32 cx2450x_osd_coeff[] = {
	0x00033215, 0x00022050, 0x00033214, 0x00023050,
	0x00033204, 0x00023060, 0x00033204, 0x00023060,
	0x000331f4, 0x00024060, 0x000331f3, 0x00024070,
	0x000331e3, 0x00025070, 0x000331e3, 0x00025070,
	0x000331d2, 0x00026080, 0x000331d2, 0x00026080,
	0x000331c2, 0x00026090, 0x000321c2, 0x00027090,
	0x000321b2, 0x00028090, 0x000321b1, 0x000280a0,
	0x000321a2, 0x000280a0, 0x00032191, 0x0002a0a0,
	0x00032191, 0x000290b0, 0x00032191, 0x000290b0,
	0x00031181, 0x0002a0c0, 0x00031181, 0x0002a0c0,
	0x00031170, 0x0002b0d0, 0x00031170, 0x0002b0d0,
	0x00031160, 0x0002b0e0, 0x00030160, 0x0002c0e0,
	0x00030150, 0x0002c0f0, 0x00030150, 0x0002c0f0,
	0x00030140, 0x0002d0f0, 0x0002f140, 0x0002d100,
	0x0002f130, 0x0002e100, 0x0002f120, 0x0002e110,
	0x0002f120, 0x0002e110, 0x0002e120, 0x0002e120,
	0x0002e110, 0x0002f120, 0x0002e110, 0x0002f120,
	0x0002e100, 0x0002f130, 0x0002d100, 0x0002f140,
	0x0002d0f0, 0x00030140, 0x0002c0f0, 0x00030150,
	0x0002c0f0, 0x00030150, 0x0002c0e0, 0x00030160,
	0x0002b0e0, 0x00031160, 0x0002b0d0, 0x00031170,
	0x0002b0d0, 0x00031170, 0x0002a0c0, 0x00031181,
	0x0002a0c0, 0x00031181, 0x000290b0, 0x00032191,
	0x000290b0, 0x00032191, 0x000290a0, 0x00033191,
	0x000280a0, 0x000321a2, 0x000280a0, 0x000321b1,
	0x00028090, 0x000321b2, 0x00027090, 0x000321c2,
	0x00026090, 0x000331c2, 0x00026080, 0x000331d2,
	0x00026080, 0x000331d2, 0x00025070, 0x000331e3,
	0x00025070, 0x000331e3, 0x00024070, 0x000331f3,
	0x00024060, 0x000331f4, 0x00023060, 0x00033204,
	0x00023060, 0x00033204, 0x00022050, 0x00034214,
	0x00022050, 0x00033215, 0x00022050, 0x00032225,

	0x000401f0, 0x00021000, 0x000401e0, 0x00022000,
	0x000401e0, 0x00022000, 0x000401d0, 0x00023000,
	0x000401c0, 0x00024000, 0x000401b0, 0x00025000,
	0x000401b0, 0x00025000, 0x0003f1a0, 0x00026010,
	0x0003f190, 0x00027010, 0x0003f180, 0x00028010,
	0x0003f170, 0x00029010, 0x0003f170, 0x00029010,
	0x0003e160, 0x0002a020, 0x0003e150, 0x0002b020,
	0x0003e140, 0x0002c020, 0x0003e140, 0x0002c020,
	0x0003d130, 0x0002d030, 0x0003d120, 0x0002e030,
	0x0003d120, 0x0002e030, 0x0003c110, 0x0002f040,
	0x0003c100, 0x00030040, 0x0003b100, 0x00030050,
	0x0003b0f0, 0x00031050, 0x0003b0e0, 0x00032050,
	0x0003a0e0, 0x00032060, 0x0003a0d0, 0x00033060,
	0x000390c0, 0x00034070, 0x000390c0, 0x00034070,
	0x000380b0, 0x00035080, 0x000380b0, 0x00035080,
	0x000370a0, 0x00036090, 0x00037090, 0x00037090,
	0x00036090, 0x000370a0, 0x00035080, 0x000380b0,
	0x00035080, 0x000380b0, 0x00034070, 0x000390c0,
	0x00034070, 0x000390c0, 0x00033060, 0x0003a0d0,
	0x00032060, 0x0003a0e0, 0x00032050, 0x0003b0e0,
	0x00031050, 0x0003b0f0, 0x00030050, 0x0003b100,
	0x00030040, 0x0003c100, 0x0002f040, 0x0003c110,
	0x0002e030, 0x0003d120, 0x0002e030, 0x0003d120,
	0x0002d030, 0x0003d130, 0x0002c020, 0x0003e140,
	0x0002c020, 0x0003e140, 0x0002b020, 0x0003e150,
	0x0002a020, 0x0003e160, 0x00029010, 0x0003f170,
	0x00029010, 0x0003f170, 0x00028010, 0x0003f180,
	0x00027010, 0x0003f190, 0x00026010, 0x0003f1a0,
	0x00025000, 0x000401b0, 0x00025000, 0x000401b0,
	0x00024000, 0x000401c0, 0x00023000, 0x000401d0,
	0x00022000, 0x000401e0, 0x00022000, 0x000401e0,
	0x00021000, 0x000401f0, 0x00020000, 0x00040200,

	0x0004c190, 0x0001b000, 0x0004c180, 0x0001c000,
	0x0004c170, 0x0001d000, 0x0004b160, 0x0001f000,
	0x0004b150, 0x00020000, 0x0004b140, 0x00021000,
	0x0004b130, 0x00022000, 0x0004b120, 0x00023000,
	0x0004b110, 0x00024000, 0x0004b100, 0x00025000,
	0x0004a0f0, 0x00027000, 0x0004a0e0, 0x00028000,
	0x000490e0, 0x00029000, 0x000490d0, 0x0002a000,
	0x000490c0, 0x0002b000, 0x000490b0, 0x0002c000,
	0x000480a0, 0x0002e000, 0x000470a0, 0x0002f000,
	0x00047090, 0x00030000, 0x00047080, 0x00031000,
	0x00046080, 0x00032000, 0x00046070, 0x00033000,
	0x00045060, 0x00035000, 0x00044060, 0x00036000,
	0x00044050, 0x00037000, 0x00043050, 0x00038000,
	0x00042040, 0x00039010, 0x00041040, 0x0003a010,
	0x00041030, 0x0003b010, 0x00040030, 0x0003c010,
	0x0003f020, 0x0003d020, 0x0003e020, 0x0003e020,
	0x0003d020, 0x0003f020, 0x0003c010, 0x00040030,
	0x0003b010, 0x00041030, 0x0003a010, 0x00041040,
	0x00039010, 0x00042040, 0x00038000, 0x00043050,
	0x00037000, 0x00044050, 0x00036000, 0x00044060,
	0x00035000, 0x00045060, 0x00033000, 0x00046070,
	0x00032000, 0x00046080, 0x00031000, 0x00047080,
	0x00030000, 0x00047090, 0x0002f000, 0x000470a0,
	0x0002d000, 0x000490a0, 0x0002c000, 0x000490b0,
	0x0002b000, 0x000490c0, 0x0002a000, 0x000490d0,
	0x00029000, 0x000490e0, 0x00028000, 0x0004a0e0,
	0x00027000, 0x0004a0f0, 0x00025000, 0x0004b100,
	0x00024000, 0x0004b110, 0x00023000, 0x0004b120,
	0x00022000, 0x0004b130, 0x00021000, 0x0004b140,
	0x00020000, 0x0004b150, 0x0001e000, 0x0004c160,
	0x0001d000, 0x0004c170, 0x0001c000, 0x0004c180,
	0x0001b000, 0x0004c190, 0x0001a000, 0x0004c1a0,

	0x0005b110, 0x00014000, 0x0005b100, 0x00015000,
	0x0005a0f0, 0x00017000, 0x0005a0e0, 0x00018000,
	0x0005a0d0, 0x00019000, 0x000590c0, 0x0001b000,
	0x000590b0, 0x0001c000, 0x000590a0, 0x0001d000,
	0x00058090, 0x0001f000, 0x00058080, 0x00020000,
	0x00057070, 0x00022000, 0x00056060, 0x00024000,
	0x00056050, 0x00025000, 0x00055050, 0x00026000,
	0x00054040, 0x00028000, 0x00053040, 0x00029000,
	0x00052030, 0x0002b000, 0x00051020, 0x0002d000,
	0x00050020, 0x0002e000, 0x0004f020, 0x0002f000,
	0x0004e010, 0x00031000, 0x0004d010, 0x00032000,
	0x0004c000, 0x00034000, 0x0004b000, 0x00035000,
	0x00049000, 0x00037000, 0x00048000, 0x00038000,
	0x00047000, 0x00039000, 0x00045000, 0x0003b000,
	0x00044000, 0x0003c000, 0x00043000, 0x0003d000,
	0x00041000, 0x0003f000, 0x00040000, 0x00040000,
	0x0003f000, 0x00041000, 0x0003d000, 0x00043000,
	0x0003c000, 0x00044000, 0x0003b000, 0x00045000,
	0x00039000, 0x00047000, 0x00038000, 0x00048000,
	0x00037000, 0x00049000, 0x00035000, 0x0004b000,
	0x00034000, 0x0004c000, 0x00032000, 0x0004d010,
	0x00031000, 0x0004e010, 0x0002f000, 0x0004f020,
	0x0002e000, 0x00050020, 0x0002d000, 0x00051020,
	0x0002b000, 0x00052030, 0x00029000, 0x00053040,
	0x00028000, 0x00054040, 0x00026000, 0x00055050,
	0x00025000, 0x00056050, 0x00024000, 0x00056060,
	0x00022000, 0x00057070, 0x00020000, 0x00058080,
	0x0001f000, 0x00058090, 0x0001d000, 0x000590a0,
	0x0001c000, 0x000590b0, 0x0001b000, 0x000590c0,
	0x00019000, 0x0005a0d0, 0x00018000, 0x0005a0e0,
	0x00017000, 0x0005a0f0, 0x00015000, 0x0005b100,
	0x00014000, 0x0005b110, 0x00013000, 0x0005a130,

	0x0006b090, 0x0000c000, 0x0006b080, 0x0000d000,
	0x0006b070, 0x0000e000, 0x0006a060, 0x00010000,
	0x0006a050, 0x00011000, 0x00069040, 0x00013000,
	0x00069030, 0x00014000, 0x00067030, 0x00016000,
	0x00067020, 0x00017000, 0x00065020, 0x00019000,
	0x00064010, 0x0001b000, 0x00063010, 0x0001c000,
	0x00062000, 0x0001e000, 0x00060000, 0x00020000,
	0x0005e000, 0x00022000, 0x0005d000, 0x00023000,
	0x0005b000, 0x00025000, 0x00059000, 0x00027000,
	0x00057000, 0x00029000, 0x00056000, 0x0002a000,
	0x00054000, 0x0002c000, 0x00052000, 0x0002e000,
	0x00050000, 0x00030000, 0x0004f000, 0x00031000,
	0x0004d000, 0x00033000, 0x0004b000, 0x00035000,
	0x00049000, 0x00037000, 0x00047000, 0x00039000,
	0x00045000, 0x0003b000, 0x00044000, 0x0003c000,
	0x00042000, 0x0003e000, 0x00040000, 0x00040000,
	0x0003e000, 0x00042000, 0x0003c000, 0x00044000,
	0x0003b000, 0x00045000, 0x00039000, 0x00047000,
	0x00037000, 0x00049000, 0x00035000, 0x0004b000,
	0x00033000, 0x0004d000, 0x00031000, 0x0004f000,
	0x00030000, 0x00050000, 0x0002e000, 0x00052000,
	0x0002c000, 0x00054000, 0x0002a000, 0x00056000,
	0x00029000, 0x00057000, 0x00027000, 0x00059000,
	0x00025000, 0x0005b000, 0x00023000, 0x0005d000,
	0x00022000, 0x0005e000, 0x00020000, 0x00060000,
	0x0001e000, 0x00062000, 0x0001c000, 0x00063010,
	0x0001b000, 0x00064010, 0x00019000, 0x00065020,
	0x00018000, 0x00066020, 0x00016000, 0x00067030,
	0x00014000, 0x00069030, 0x00013000, 0x00069040,
	0x00011000, 0x0006a050, 0x00010000, 0x0006a060,
	0x0000e000, 0x0006b070, 0x0000d000, 0x0006b080,
	0x0000c000, 0x0006b090, 0x0000a000, 0x0006c0a0,

	0x0007a020, 0x00004000, 0x0007a010, 0x00005000,
	0x00079010, 0x00006000, 0x00078010, 0x00007000,
	0x00077000, 0x00009000, 0x00076000, 0x0000a000,
	0x00075000, 0x0000b000, 0x00073000, 0x0000d000,
	0x00072000, 0x0000e000, 0x00070000, 0x00010000,
	0x0006e000, 0x00012000, 0x0006c000, 0x00014000,
	0x0006b000, 0x00015000, 0x00069000, 0x00017000,
	0x00067000, 0x00019000, 0x00065000, 0x0001b000,
	0x00063000, 0x0001d000, 0x00061000, 0x0001f000,
	0x0005e000, 0x00022000, 0x0005c000, 0x00024000,
	0x0005a000, 0x00026000, 0x00058000, 0x00028000,
	0x00055000, 0x0002b000, 0x00053000, 0x0002d000,
	0x00051000, 0x0002f000, 0x0004e000, 0x00032000,
	0x0004c000, 0x00034000, 0x0004a000, 0x00036000,
	0x00047000, 0x00039000, 0x00045000, 0x0003b000,
	0x00042000, 0x0003e000, 0x00040000, 0x00040000,
	0x0003e000, 0x00042000, 0x0003b000, 0x00045000,
	0x00039000, 0x00047000, 0x00036000, 0x0004a000,
	0x00034000, 0x0004c000, 0x00032000, 0x0004e000,
	0x0002f000, 0x00051000, 0x0002d000, 0x00053000,
	0x0002b000, 0x00055000, 0x00028000, 0x00058000,
	0x00026000, 0x0005a000, 0x00024000, 0x0005c000,
	0x00022000, 0x0005e000, 0x0001f000, 0x00061000,
	0x0001d000, 0x00063000, 0x0001b000, 0x00065000,
	0x00019000, 0x00067000, 0x00017000, 0x00069000,
	0x00015000, 0x0006b000, 0x00014000, 0x0006c000,
	0x00012000, 0x0006e000, 0x00010000, 0x00070000,
	0x0000e000, 0x00072000, 0x0000d000, 0x00073000,
	0x0000b000, 0x00075000, 0x0000a000, 0x00076000,
	0x00009000, 0x00077000, 0x00007000, 0x00078010,
	0x00006000, 0x00079010, 0x00005000, 0x0007a010,
	0x00004000, 0x0007a020, 0x00003000, 0x0007a030,
};

static const reg_pair_t video_clock_pal[] = {
	{ (volatile u32 *)VIDEO_CLK_REG, 0x00000001 }, /* SD Interlaced(27MHz), Chan0, Chan1: MPG0 PLL */ 
	{ (volatile u32 *)0xffffffff   , 0xffffffff },
};

static const reg_pair_t denc_pal_settings[] = {
	/* PAL settings */
	{ (volatile u32 *)0xe05d0114, 0x00001108 },
	{ (volatile u32 *)0xe05d0118, 0x007f9551 },
	{ (volatile u32 *)0xe05d011c, 0x00808080 },
	{ (volatile u32 *)0xe05d0120, 0x00906680 },
	{ (volatile u32 *)0xe05d0124, 0x0085f056 },
	{ (volatile u32 *)0xe05d0128, 0x00a48ec8 },
	{ (volatile u32 *)0xe05d012c, 0x00a4f000 },
	{ (volatile u32 *)0xe05d0130, 0x2a098acb },
	{ (volatile u32 *)0xe05d0134, 0x00000000 },
	{ (volatile u32 *)0xe05d0138, 0x05a3049f },
	{ (volatile u32 *)0xe05d013c, 0x05a3049f },
	{ (volatile u32 *)0xe05d0140, 0x00000000 },
	{ (volatile u32 *)0xe05d0144, 0x7f00007f },
	{ (volatile u32 *)0xffffffff, 0xffffffff },
};

static const reg_pair_t denc_regs[] = {
	{ (volatile u32 *)0xe05d0000, 0x00000001 },
	{ (volatile u32 *)0xe05d0004, 0x00100000 },
	{ (volatile u32 *)0xe05d0008, 0x00000100 }, // DRM0-> SD Enc
	{ (volatile u32 *)0xe05d000c, 0x0000027f },
	{ (volatile u32 *)0xe05d001c, 0x00000003 },

	/* MAIN Raster registers */
	{ (volatile u32 *)0xe05d0100, 0x00000001 },
	{ (volatile u32 *)0xe05d0104, 0x80000161 | (0 << 9) },
	{ (volatile u32 *)0xe05d0108, 0x000006c0 },
	{ (volatile u32 *)0xe05d010c, 0x05a00116 },
	{ (volatile u32 *)0xe05d0110, 0x01200016 },

	/* AUX Raster registers */
	{ (volatile u32 *)0xe05d0504, 0x80000161 | (0 << 9) },
	{ (volatile u32 *)0xe05d0508, 0x000006c0 },
	{ (volatile u32 *)0xe05d050c, 0x05a00116 },
	{ (volatile u32 *)0xe05d0510, 0x01200016 },

//	{ (volatile u32 *)0xe05d0100, 0x00000001 },
	{ (volatile u32 *)0xe05d0200, 0x00000000 },
#if 0
	{ (volatile u32 *)0xe05d0148, 0x098c014a },
	{ (volatile u32 *)0xe05d014c, 0x00008080 },
	{ (volatile u32 *)0xe05d0150, 0x00008080 },
	{ (volatile u32 *)0xe05d0154, 0x00440000 },
	{ (volatile u32 *)0xe05d0158, 0x00000000 },
	{ (volatile u32 *)0xe05d015c, 0x00070139 },
	{ (volatile u32 *)0xe05d0160, 0x00000000 },
	{ (volatile u32 *)0xe05d0164, 0x00000000 },
	{ (volatile u32 *)0xe05d0168, 0x80000000 },
	{ (volatile u32 *)0xe05d016c, 0x00000110 },
	{ (volatile u32 *)0xe05d0170, 0x0000011e },
#endif
#if 0
	{ (volatile u32 *)0xe05d0174, 0x00000000 },
	{ (volatile u32 *)0xe05d0178, 0x00000000 },
	{ (volatile u32 *)0xe05d017c, 0x00000000 },
	{ (volatile u32 *)0xe05d0180, 0x029d3001 },
	{ (volatile u32 *)0xe05d0184, 0x0000020d },
#endif
	{ (volatile u32 *)0xffffffff, 0xffffffff },
};

static const reg_pair_t drm0_regs[] = {
	{ (volatile u32 *)0xe0560000, 0x00100000 },
	{ (volatile u32 *)0xe056000c, 0x00808000 },

	{ (volatile u32 *)0xe056002c, 0x00000000 },
	{ (volatile u32 *)0xe0560030, 0x00000000 },
	{ (volatile u32 *)0xe0560034, 0x00000000 },
	{ (volatile u32 *)0xe0560038, 0x03030000 },
	{ (volatile u32 *)0xe056003c, 0x00000000 },
	{ (volatile u32 *)0xe0560040, 0x00000000 },
	{ (volatile u32 *)0xe0560044, 0x00000000 },
	{ (volatile u32 *)0xe0560048, 0x00000000 },
	{ (volatile u32 *)0xe056004c, 0x00000000 },
	{ (volatile u32 *)0xe0560050, 0x0000ffff },
	{ (volatile u32 *)0xe0560054, 0x07203010 }, //0x07203010 
	{ (volatile u32 *)0xe0560058, 0x00000000 },
	{ (volatile u32 *)0xe056005c, 0x00000000 },
	{ (volatile u32 *)0xe0560060, 0x00000000 },
	{ (volatile u32 *)0xe0560064, 0x00000000 },
	{ (volatile u32 *)0xe0560068, 0x23000000 },
	{ (volatile u32 *)0xe056006c, 0x00000000 },
	{ (volatile u32 *)0xe0560070, 0x00000000 },
	{ (volatile u32 *)0xe0560074, 0x00000000 },
	{ (volatile u32 *)0xe0560078, 0x00000000 },
	{ (volatile u32 *)0xe056007c, 0x00000000 },
	{ (volatile u32 *)0xe0560084, 0x43430000 },
	{ (volatile u32 *)0xe05600a0, 0x00000000 },
	{ (volatile u32 *)0xe05600a4, 0x00000000 },
	{ (volatile u32 *)0xe05600a8, 0x83410000 },
	{ (volatile u32 *)0xe05600ac, 0x00000000 },
	{ (volatile u32 *)0xe05600b0, 0x00000000 },
	{ (volatile u32 *)0xe05600f0, 0x00000000 },
	{ (volatile u32 *)0xe05600f4, 0x007f01bd },
	{ (volatile u32 *)0xe05600f8, 0x00000000 },
	{ (volatile u32 *)0xffffffff, 0xffffffff },
};


static const reg_pair_t hdmi_regs[] = {
	{ (volatile u32 *)0xe0541018, 0x00000055 },
	{ (volatile u32 *)0xe0541040, 0x000001bf },
	{ (volatile u32 *)0xe0541044, 0x0000001f },
	{ (volatile u32 *)0xe0541048, 0x0000001f },
	{ (volatile u32 *)0xe054104c, 0x0000001f },
	{ (volatile u32 *)0xe0541050, 0x0000001f },
	{ (volatile u32 *)0xe0541054, 0x00000000 },
	{ (volatile u32 *)0xe0541058, 0x00000000 },
	{ (volatile u32 *)0xe054105c, 0x57000004 },
	{ (volatile u32 *)0xe0541060, 0x00000003 },
	{ (volatile u32 *)0xe0541064, 0x00000000 },
	{ (volatile u32 *)0xffffffff, 0xffffffff },
};

struct cx2450x_fb_osd_header {
	unsigned int next_osd_ptr;
	unsigned int x_display_start_width;
	unsigned int image_height_width;
	unsigned int display_data_addr;
	unsigned int line_stride_bits;
	unsigned int y_pos_rgn_alpha;
	unsigned int scale_factors;
	unsigned int palette_addr;
	unsigned int chroma_min_val;
	unsigned int chroma_max_val;
};

struct cx2450x_fb_color_system_descr {
	const char *name;
	int resX, resY;
	unsigned short startX, startY, activeX;
	unsigned short offsetX, offsetY;
	int interlaced;
	int nr_drm;

	const reg_pair_t *video_clock;
	const reg_pair_t *denc_system;
	const reg_pair_t *denc;
	const reg_pair_t *drm0;
	const reg_pair_t *drm1;
	const reg_pair_t *hdmi;
};

static struct cx2450x_fb_data {
	volatile struct cx2450x_fb_osd_header *osd_header;
	volatile unsigned int *palette;
	volatile unsigned int *fb;
	int system_idx;
} fb_data;

static const struct cx2450x_fb_color_system_descr cx2450x_output_system_descr[] = {
	{
		.name		= "PAL-BG 576i@50Hz",
		.resX		= 720,
		.resY		= 576,
		.startX		= 0x116, /* startX * (1/54000000) */
		.startY		= 0x2b,
		.activeX	= 703,
		.offsetX	= 40,
		.offsetY	= 20,
		.video_clock	= video_clock_pal,
		.denc_system	= denc_pal_settings,
		.denc		= denc_regs,
		.drm0		= drm0_regs,
		.hdmi		= hdmi_regs,

		.nr_drm		= 1,
		.interlaced	= 1,
	},
};

#define CX2450X_VIDEO_PAL_INDEX		0

static GraphicDevice cx2450x_fb;

static void cx2450x_fb_set_register(volatile u32 *reg, u32 mask, u32 val)
{
	*reg = (*reg & ~mask) | (val & mask);
}

#if 0
static u32 cx2450x_fb_get_register(volatile u32 *reg, u32 mask)
{
	return *reg & ~mask;
}

static void cx2450x_fb_set_registers(const reg_set_t *regs, int size)
{
	int i;

	for (i = 0; i < size; i++)
		cx2450x_fb_set_register(regs[i].reg, regs[i].mask, regs[i].val);
}
#endif

static void cx2450x_fb_set_register_pair(const reg_pair_t *regs)
{
	int i = 0;

	while((regs[i].reg != (volatile u32 *)0xFFFFFFFF) && (regs[i].val != 0xFFFFFFFF)) {
		cx2450x_fb_set_register(regs[i].reg, 0xFFFFFFFF, regs[i].val);
		i++;
	}
}

void draw_rect_32 (u32 *fb, u32 line_length, int x1, int y1, int x2, int y2, u32 pixel)
{
	int count;

	while( y1 < y2) {
		for(count = x1 ; count < x2 ; count++ ) {
			*((u32 *) fb + line_length/4 * y1 + count ) = (u_int32_t)pixel;
		}
		y1++;
	}
}

static inline unsigned short cx2450x_fb_get_scale_x(unsigned short srcX, unsigned short dstX)
{
	int scale_x = -1;

	if (srcX <= (dstX << 1))
		scale_x = ((srcX - 1) << 14) / (dstX - 1); /* Axis starts at 0 */

	return scale_x;
}

static inline unsigned short cx2450x_fb_get_scale_y(unsigned short srcY, unsigned short dstY)
{
	int scale_y = -1;

	if (dstY >= srcY)
		scale_y = ((srcY - 1) << 14) / (dstY - 1); /* Axis starts at 0 */

	return scale_y;
}

static inline int cx2450x_fb_get_scale_index_coeff(unsigned short scale_x)
{
	int index;

	index = (((90 * 0x4000) / scale_x) - 35) / 10; /* 0x4000 == 2^14 */

	if(index < 0)
		index = 0;
	if(index > 5)
		index = 5;

	return index;
}

static void cx2450x_fb_setup_osd_coeff(int unit, int index)
{
	volatile u32 *reg_base = (volatile u32 *)(DRM_UNIT(unit) + 0x1000);
	int i, j, k;

	if(unit > 1)
		return;
	if((index < 0) || (index > 5))
		return;

	j = (index * 128);
	k = 0;

	/* Setup 64 phases */
	for (i = 0; i < 64; i++) {
		cx2450x_fb_set_register(reg_base + k++, 0xFFFFFFFF, cx2450x_osd_coeff[j++]);
		cx2450x_fb_set_register(reg_base + k++, 0xFFFFFFFF, cx2450x_osd_coeff[j++]);
	}
	cx2450x_fb_set_register((volatile u32 *)(DRM_UNIT(unit) + 0x88), 0x0000007F, 0x30);
}

static void cx2450x_fb_setup_active_screen(int unit, const struct cx2450x_fb_color_system_descr *sys_descr)
{
	volatile u32 *act_scr_x = (volatile u32 *)(DRM_UNIT(unit) + 0x4);
	volatile u32 *act_scr_y = (volatile u32 *)(DRM_UNIT(unit) + 0x8);
	int x_active_start, x_active_end;
	int y_active_start, y_active_end;

	/* Pixel CLK Start / End */
	x_active_start	= sys_descr->startX;
	x_active_end	= x_active_start + ((sys_descr->activeX << 1) - 1);

	/* Line Start / End */
	y_active_start	= sys_descr->startY;
	y_active_end	= y_active_start + (sys_descr->resY - 1);

	/* Update the settings */
	*act_scr_x	= (x_active_end << 16) | x_active_start;
	*act_scr_y	= (y_active_end << 16) | y_active_start;
}

static void cx2450x_fb_setup_encoder(const struct cx2450x_fb_color_system_descr *sys_descr)
{
	volatile u32 *dac0 = (volatile u32 *)0xe05d0010;
	volatile u32 *dac1 = (volatile u32 *)0xe05d0014;
	volatile u32 *dac2 = (volatile u32 *)0xe05d0018;
	u32 rev = get_board_rev();

	if (rev >= 7) {
		volatile u32 *bypass = (volatile u32 *)(AFE_BASE + 0x04);

		/* Disable RFmod mode */
		*bypass = (*bypass & ~0x300000) | 0x100000;
		*dac0	= 0x090708; /* Ypr, Y, Ypb */
		*dac1	= 0x0C0B0A; /* R, G, B */
		*dac2	= 0x1f1f0D;
	} else {
		*dac0	= 0x080907;
		*dac1	= 0x1f1f0D;
	}

	/* Initialize all needed registers */
	cx2450x_fb_set_register_pair(sys_descr->video_clock);
	cx2450x_fb_set_register_pair(sys_descr->denc_system);
	cx2450x_fb_set_register_pair(sys_descr->denc);
	cx2450x_fb_set_register_pair(sys_descr->hdmi);

	/* Disable MV! */
	cx2450x_fb_set_register((volatile u32 *)0xE05D0194, 0xFF000000, 0);
	cx2450x_fb_set_register((volatile u32 *)0xE05D0294, 0xFF000000, 0);
}

static int cx2450x_fb_setup_osd(int unit, const struct cx2450x_fb_color_system_descr *sys_descr, int bpp, int srcW, int srcH)
{
	volatile u32 *osd_hdr = (volatile u32 *)(DRM_UNIT(unit) + 0x80);
	volatile struct cx2450x_fb_osd_header *header = fb_data.osd_header;
	int scaleX, scaleY;
	int coeff_index;
	int dstW, dstH;
	int startX, startY;
	int offsetX, offsetY;
	int endX, endY;
	int stride = 4;
	u8 pixmode;

	if (!fb_data.osd_header)
		return -1;
	if (!fb_data.palette)
		return -1;
	if (!fb_data.fb)
		return -1;
	if (unit > 1)
		return -1;

	switch(bpp) {
	case GDF_32BIT_X888RGB:
		pixmode	= 0x13;
		stride	= 4;
		break;
		/* TODO: add more pixel modes? */
	default:
		printf("Invalid pixel mode\n");
		return -1;
	}

	offsetX = (sys_descr->offsetX + 7) & ~7; /* align it to dword boundary */
	if (offsetX < 0)
		offsetX = 0;

	offsetY = sys_descr->offsetY;
	if (offsetY < 0)
		offsetY = 0;

	dstW	= sys_descr->resX - offsetX;
	dstH	= sys_descr->resY - offsetY;

	if ((scaleX = cx2450x_fb_get_scale_x(srcW, dstW)) < 0)
		return -1;
	if ((scaleY = cx2450x_fb_get_scale_y(srcH, dstH)) < 0)
		return -1;
	/*
	 * Calculate the coordinates for Line and pixel counter
	 * Note: we assume we always draw at offset 0, horizontally
	 *	 and vertically
	 */

	startX	= sys_descr->startX + (offsetX << 1);
	endX	= dstW; /* just the display width in pixels */
	startY	= sys_descr->startY + offsetY;
	endY	= startY + (dstH - 1);

	/* Clear the osd header. this should disable the plane */
	cx2450x_fb_set_register(osd_hdr, 0xFFFFFFFF, (u32) 0);
	/* Setup the active screen in DRM */
	cx2450x_fb_setup_active_screen(unit, sys_descr);
	/* Init the DRM module */
	cx2450x_fb_set_register_pair(unit == 0 ? sys_descr->drm0 : sys_descr->drm1);
	/* Calculate the scaling coefficient index for Horizontal scaling */
	coeff_index = cx2450x_fb_get_scale_index_coeff(scaleX);
	/* Set the coefficients */
	cx2450x_fb_setup_osd_coeff(unit, coeff_index);


	header->next_osd_ptr		= 0; /* NO next OSD window */
	header->x_display_start_width	= (2 << 30) | (endX << 16) | (2 << 14) | (startX << 0); /* Alpha blender1 input plane 0 = OSD, Alpha blender1 input plane 1 = OSD, 720 width, start 0 */
	header->image_height_width	= (2 << 29) | (1 << 28) | (srcH << 16) | (1 << 15) | (0 << 14) | (0 << 13) | (0 << 11) | (srcW << 0);
	header->display_data_addr	= (u32)PHYSICAL_ADDRESS((void *)fb_data.fb);
	header->line_stride_bits	= (1 << 29) | (2 << 25) | (1 << 23) | (pixmode << 16) | ((srcW * stride) << 0); /* Use region alpha to support 32 bit pixels without alpha value */
	header->y_pos_rgn_alpha		= (0xFF << 24) | (endY << 12) | (startY << 0);
	header->scale_factors		= (scaleY << 16) | scaleX; /* YScale, XScale*/
	header->palette_addr		= (u32)PHYSICAL_ADDRESS((void *)fb_data.palette);
	header->chroma_min_val		= 0;
	header->chroma_max_val		= 0;

	/* set the osd header pointer */
	cx2450x_fb_set_register(osd_hdr, 0xFFFFFFFF, (u32) PHYSICAL_ADDRESS((void *)header));

	return 0;
}

void *video_hw_init(void)
{
	GraphicDevice *pGd = &cx2450x_fb;
	int i;
	unsigned int *osd_palette, *image;
	volatile struct cx2450x_fb_osd_header *osd_header;
	int image_size;
	int system_idx;

	pGd->winSizeX	= board_get_width();
	pGd->winSizeY	= board_get_height();
	pGd->gdfIndex	= GDF_32BIT_X888RGB;
	pGd->gdfBytesPP = 4;

	image_size = 	pGd->winSizeX *
			pGd->winSizeY *
			pGd->gdfBytesPP;

	/*
	 * Start allocating framebuffer, osd header and palette,
	 * We do this at 1MB offset from PHYS_RAM_START. We dont use
	 * the MMU so we're dealing with physical memory addresses.
	 * If we want this picture to remain during linux kernel boot, we
	 * need to reserve this area as 'bootmem'.
	 */
	osd_header	= (volatile struct cx2450x_fb_osd_header *)0x100000; /* 1MB */
	image		= (unsigned int *) ALIGN_16(0x100000 + sizeof(struct cx2450x_fb_osd_header));
	osd_palette	= (unsigned int *) ALIGN_16(0x100000 + sizeof(struct cx2450x_fb_osd_header) + image_size);
	pGd->frameAdrs	= (unsigned long) image;

	/* Generate LUT */
	for (i = 0; i < 256; i++)
		osd_palette[i] = (i << 24) | (i << 16) | (i << 8) | i;

	/* Clear the image */
	memset(image, 0, image_size);

	system_idx		= CX2450X_VIDEO_PAL_INDEX;
	fb_data.osd_header	= osd_header;
	fb_data.palette		= osd_palette;
	fb_data.fb		= image;
	fb_data.system_idx	= system_idx;

	/* Setup the video encoder */
	cx2450x_fb_setup_encoder(&cx2450x_output_system_descr[system_idx]);
	/* TVEnc is setup to use DRM0 for SD and HD */
	if (cx2450x_fb_setup_osd(0, &cx2450x_output_system_descr[system_idx],
				pGd->gdfIndex,
				pGd->winSizeX,
				pGd->winSizeY) < 0)
		goto error;

	return pGd;
error:
	return NULL;
}

void video_set_lut (
	unsigned int index,           /* color number */
	unsigned char r,              /* red */
	unsigned char g,              /* green */
	unsigned char b               /* blue */
	)
{
	/* nothing in here */
}
