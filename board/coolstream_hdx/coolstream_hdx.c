/*
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/arch/nevis.h>
#include <asm/arch/sys_proto.h>
//#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;


#define DISPLAY_WIDTH			720
#define DISPLAY_HEIGHT			576

extern void arm926_spin_begin(void);
extern void arm926_spin_end(void);

#if (CONFIG_ARM9_SETUP_LEGACY_MODE == 1)
#define ARM9_VECTOR_OFFSET		CONFIG_ARM9_OFFSET_LEGACY
#define ARM9_SHARED_CODE_START		(CONFIG_ARM9_SHARED_CODE_OFFSET_LEGACY + CONFIG_ARM9_RESET_ADDR)
#define ARM9_SHARED_RAM_START		(CONFIG_ARM9_SHARED_RAM_OFFSET_LEGACY + CONFIG_ARM9_RESET_ADDR)
#else
#define ARM9_VECTOR_OFFSET		CONFIG_ARM9_OFFSET
#define ARM9_SHARED_CODE_START		(CONFIG_ARM9_SHARED_CODE_OFFSET + CONFIG_ARM9_RESET_ADDR)
#define ARM9_SHARED_RAM_START		(CONFIG_ARM9_SHARED_RAM_OFFSET + CONFIG_ARM9_RESET_ADDR)
#endif

extern flash_info_t flash_info[];       /* info for FLASH chips */

static void pwm_init(void)
{
	volatile u32 *CNTLO	= (volatile u32 *)0xE0480004;
	volatile u32 *PLSEO	= (volatile u32 *)0xE0480008;

	*CNTLO	= 0x05;
	*PLSEO	= (0x265 << 16) | (0x265 << 0);
}

static void led_init(void)
{
	display_led_control(VFD_LED_1_ON);
	display_led_control(VFD_LED_2_ON);
}

#ifdef CONFIG_LOAD_INITIAL_DECARM_CODE
static void arm9_init(void)
{
    const volatile u32 *reset_code = (volatile u32 *) arm926_spin_begin;
    volatile u32 *decarm_addr      = (volatile u32 *) RST_DECARM_CTL_REG;
    volatile u32 *reset_addr       = (volatile u32 *) CONFIG_ARM9_RESET_ADDR;
    volatile u32 *shr_code         = (volatile u32 *) ARM9_SHARED_CODE_START;
    volatile u32 *shr_ram          = (volatile u32 *) ARM9_SHARED_RAM_START;
    volatile u32 *ver_indicator    = (volatile u32 *) (CONFIG_ARM9_SHARED_CODE_OFFSET_LEGACY + CONFIG_ARM9_RESET_ADDR);
    volatile u32 *dest;


    dest = reset_addr;
    while(reset_code < (volatile u32 *)arm926_spin_end) {
        *dest++ = *reset_code++;
    }

    while(dest < (volatile u32 *) (CONFIG_ARM9_RESET_ADDR + ARM9_VECTOR_OFFSET)) {
        *dest++ = 0x00000000;
    }

    *ver_indicator = 0xEEEEEEEE;
    *shr_code     = 0x00000000;
    *shr_ram      = 0x00000000;
    *decarm_addr |= 1;  /* Remap to 0x10000 */
    *decarm_addr &= ~2; /* Release reset    */
}
#endif


/*******************************************************************************/
/* do early hardware initializations                                           */

int board_init(void)
{
    gd->bd->bi_arch_number = 0x04C8;	 /* Linux board ID -> Conexant Nevis */

    /* define where the boot parameters are placed for the kernel.
     * From <kernel>/Documentation/arm/Booting:
     *
     *	The tagged list must be placed in a region of memory where neither
     *	the kernel decompressor nor initrd 'bootp' program will overwrite
     *	it.  The recommended placement is in the first 16KiB of RAM.
     *
     *	For Conexant kernel we want it at physical address 0x17000000 (before kernel 
     * 	entry point address at 1 megabyte boundary.
     */

    /* coolstream kernel */
    gd->bd->bi_boot_params = 0x00000100;

    /* Initialize the Interrupt Controller for board specific usage */
    board_init_itc();

    return 0;
}

/*******************************************************************************/
/* miscellaneous platform dependent initialisations before devices_init(),     */
/* but after the enviroment is available.                                      */

int misc_init_p(void)
{
    /* Init our small VFD for which we do not need the framebuffer stuff. */
    board_display_init();
    display_set_text(NULL);

    /* some misc GPIO's to setup */
    board_gpio_drive(165, PIO_LOW);	/* SCART Pin 8 power on */
    board_gpio_drive(176, PIO_LOW);	/* SCART Pin 8 = 12V */
    board_gpio_drive(179, PIO_LOW);	/* SCART A/V out on */
    board_gpio_drive(190, PIO_LOW);	/* SCART Fastblank on (enable RGB) */

    /* low-level network config (PHY setup, Interrupt setup, ...) */
    board_eth_init();

    /* Set the FAN to on! */
    board_gpio_drive(187, PIO_HIGH);

    pwm_init();

#ifdef CONFIG_LOAD_INITIAL_DECARM_CODE
    arm9_init();
#endif

    led_init();

    return 0;
}



/*******************************************************************************/
/* set u-boot's idea of SDRAM size                                             */

int dram_init(void)
{
    /* In low level init we stored the size of the installed RAM
       into untouched RAM. Now copy that content into gd->bi */
    volatile u32 *size1 = (volatile u32*) SCRATCH_2_REG;
    volatile u32 *size2 = (volatile u32*) SCRATCH_3_REG;

    gd->bd->bi_dram[0].start = 0;
    gd->bd->bi_dram[0].size = *size1;

    gd->bd->bi_dram[1].start = 0x10000000;
    gd->bd->bi_dram[1].size = *size2;

    return (0);
}

/*******************************************************************************/

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	serialnr->high = (flash_info[0].serial[0] << 24) | (flash_info[0].serial[1] << 16) | (flash_info[0].serial[2] << 8) | flash_info[0].serial[3];
	serialnr->low  = (flash_info[0].serial[5] << 16) | (flash_info[0].serial[6] << 8) | flash_info[0].serial[7];
}
#endif

#ifdef CONFIG_REVISION_TAG
u32 get_board_rev(void)
{
	return flash_info[0].serial[4];
}
#endif

#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
    volatile u32 *reg;
    volatile u32 *idven = (volatile u32*) SCRATCH_4_REG;
    volatile u32 *chrev = (volatile u32*) SCRATCH_5_REG;
    volatile u32 *fuses = (volatile u32*) 0xE0440148;
    volatile u32 *hwopt = (volatile u32*) 0xE044014C;
    u32 CIV = *idven;
    u16 ChipID, ChipVendor;
    u8  Version;
    u32 HaveChip = 0;
    char  Rstr[10];
    char *ukn   = "unknown";
    char *Vstr  = ukn;
    char *IDstr = ukn;
    u32 pre, pos, cnt2;
    u64 i, f, fin, fp;
    u64 out;
    u32 outx, outr;
#ifdef CPU_CFG_DEBUG
    volatile u32 *ccfg0 = (volatile u32*) 0xE0440100;
    u32 ChipConfig;
    u8 AddressBits;
#endif

    memset(Rstr, 0, 10);
    strcpy(Rstr, ukn);

    ChipVendor =  CIV & 0xFFFF;
    ChipID     = (CIV >> 16) & 0xFFFF;
    Version    =  *chrev & 0xFF;

    if (ChipVendor == 0x14F1)
    {
	Vstr = "Conexant";
	if (ChipID == 0x2427)
	{
	    IDstr = "CX2427x (Pecos)";
	    HaveChip = 1;
	    if (Version == 0x10)
	    {
		/* Nevis Rev. B0 comes up as Pecos Rev. B0, but has an unused fuse but set */
		if (*fuses & 0x01000000)
		{
		    IDstr = "CX2450x (Nevis)";
		    ChipID = 0x2450;
		    *idven = 0x245014F1;
		}
	    }
	}
	else if (ChipID == 0x2450)
	{
	    IDstr = "CX2450x (Nevis)";
	    HaveChip = 1;
	    /* Rev C0 is coming up with wrong ID */
	    if (Version == 0x10)
	    {
		Version = 0x20;
		*chrev  = 0x20;
	    }
	}
	else if (ChipID == 0x4170)
	{
	    IDstr = "Pecos or Nevis";
	    HaveChip = 1;
	}

	if (HaveChip)
	{
	    /* Decode the chip revision */
	    u8 major = (Version >> 4) & 0x0F;
	    u8 minor =  Version       & 0x0F;
	    Rstr[0] = major + 0x41;
	    Rstr[1] = minor + 0x30;
	    Rstr[2] = 0x00;
	}
    }
    else
	Vstr = ukn;

    printf("\xBA Chipset information                                                          \xBA\n");
    printf("\xBA Vendor: %-10sType: %-17sRevision: %-9sFuses: %.8X  \xBA\n", Vstr, IDstr, Rstr, *fuses);
    printf("\xBA Option: %.8X  max. clock: %.3d MHz    Core voltage: %-22s\xBA\n", *hwopt, 
	    ((Version >> 4) == 2) ? (((*hwopt) & 0x800000) ? 450 : 550) : 600,
	    ((*hwopt) & 0x400000) ? "low" : "high");
    printf("\xCC\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
    	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB9\n");

    /* additionally read out the PLL's and display their clock frequency */
    u32 cnt, val;
    char *pllname[9] = {"MPG0", "MPG1", "HD  ", "AUD ", "PLL0", "PLL1", "PLL2", "FENRUS", "unknown"};
    u32   pllfreq[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    u32   cpumap[16] = {0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8};

    printf("\xBA Clock information                                                            \xBA\n");
    printf("\xBA ");

    for (cnt = 0; cnt < 7; cnt++)	/* there are 7 (6) verve PLLs for our interrest (MPG1 is not avail in Nevis) */
    {

	reg = (volatile u32*)(0xE0440000 + (cnt * 8));
	val = *reg;
	i   = (val >> 25) & 0x3F;	/* Integer part      bit 30:25 */
	f   =  val        & 0x01FFFFFF;	/* Fractional part   bit 24:00 */

	reg = (volatile u32*)(0xE0440000 + (cnt * 8) + 4);
	val = *reg;
	pre = (val >> 16) & 0x0F;	/* Pre divider       bit 19:16 */
	pos = (val >> 20) & 0x0F;	/* Post divider      bit 23:20 (not functional for Nevis Revision A to C) */

	if (pre > 0)
	{
	    fin = 60000000 / pre;		/* PLL input frequency = 60 MHz xtal frequency / pre divider */
	    fp  = (i << 32) + ((f << 32) / 0x02000000);
	    out = ((fin * fp) >> 32);
	}
	else
	    out = 0;

	/* round down to kHz */
	outx = (u32) out;	/* there is no code for u64 div, so cast here. It's save, believe me */
	for (cnt2 = 0; cnt2 < 3; cnt2++)
	{
	    outr = outx / 10;
	    if ((outx % 10) > 4)
		outr++;
	    outx /= 10;
	}

	pllfreq[cnt] = outr;

	printf("%s: %3d.%.3d MHz  ", pllname[cnt], outr / 1000, outr % 1000);
	if (cnt == 3)
	    printf(" \xBA\n\xBA ");
    }
    printf("                    \xBA\n");

    reg   = (volatile u32*)(0xE0440054);
    val   = *reg;
    outr  = pllfreq[cpumap[(val >> 24) & 0x0F]];
    outr /= ((val >> 16) & 0xFF);		/* apply the CPU clock post divider */

    printf("\xBA CPU : %3d.%.3d MHz from %-54s\xBA\n", outr / 1000, outr % 1000, pllname[cpumap[(val >> 24) & 0x0F]]);

    printf("\xCC\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
    	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB9\n");

#ifdef CPU_CFG_DEBUG
    printf("\xBA CPU configuration information                                                \xBA\n");

    ChipConfig  = *ccfg0;
    AddressBits = (ChipConfig >> 11) & 7;
    char *addrbits[6] = {"21 bits", "22 bits", "23 bits", "24 bits", "25 bits", "26 bits"};

    printf("\xBA CFG0 : %.8X                                                              \xBA\n", ChipConfig);
    printf("\xBA Flash addressing :  %.8X (%-10s)    Reset time: %.3dms               \xBA\n", AddressBits,
            (AddressBits > 5) ? "invalid" : addrbits[AddressBits],
            (ChipConfig & 1) ? 1 : 200);

    printf("\xCC\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
    	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB9\n");
#endif

    return (0);
}
#endif

u32 board_get_width(void)
{
	return DISPLAY_WIDTH;
}

u32 board_get_height(void)
{
	return DISPLAY_HEIGHT;
}

