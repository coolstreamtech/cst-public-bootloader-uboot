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

DECLARE_GLOBAL_DATA_PTR;

flash_info_t flash_info[CONFIG_SYS_MAX_FLASH_BANKS];

static u32 flash_flags = 0;

#ifndef CFG_EXT_LEGACY_FLASH
#error CFG_EXT_LEGACY_FLASH must be set to use this flash driver
#endif

/* AMD/Spansion commands */
#define CMD_UNLOCK1		0x00AA
#define CMD_UNLOCK2		0x0055
#define CMD_WRITE_BUFF_SETUP	0x0025
#define CMD_WRITE_BUFF_CONFIRM	0x0029
#define CMD_ERASE_CONFIRM	0x0030
#define CMD_ERASE_SETUP		0x0080
#define CMD_SSP_ENTER		0x0088	/* enter secured silicon sector entry access */
#define CMD_AUTOSELECT		0x0090	/* enter auto selection command mode */
#define CMD_WRITE_WORD_SETUP	0x00A0
#define	CMD_PPB_ENTER		0x00C0	/* enter persistent protection bit programming mode */
#define CMD_READ_ARRAY		0x00F0	/* also know as reset */

/* flags */
#define FLASH_ERASE_ALL		0x0001

/*******************************************************************************/

static void exec_flash_cmd(ushort bank, ulong base_addr, ulong sect_addr, ushort cmd)
{
    volatile u32 *romdesc = (volatile u32*)(ROM_DESC0_REG + (bank * 4));
    *romdesc |= 0x00800000;	/* bit 23: enable write access (16 bit translation) */

    *((volatile ushort*)(base_addr + (0x0555 << 1))) = CMD_UNLOCK1;
    *((volatile ushort*)(base_addr + (0x02AA << 1))) = CMD_UNLOCK2;

    if ((cmd == CMD_ERASE_CONFIRM) || (cmd == CMD_WRITE_BUFF_SETUP))
	*((volatile ushort*) sect_addr) = cmd;
    else
	*((volatile ushort*)(base_addr + (0x0555 << 1))) = cmd;

    *romdesc &= 0xFF7FFFFF;	/* bit 23: disable write access */
}

/*******************************************************************************/

static void leave_flash_pgm(ushort bank, ulong addr, int leave_ssse)
{
    volatile u32 *romdesc = (volatile u32*)(ROM_DESC0_REG + (bank * 4));
    *romdesc |= 0x00800000;	/* bit 23: enable write access (16 bit translation) */
    if (leave_ssse)
    {
	*((volatile ushort*)(addr + (0x0555 << 1))) = CMD_UNLOCK1;
        *((volatile ushort*)(addr + (0x02AA << 1))) = CMD_UNLOCK2;
	*((volatile ushort*)(addr + (0x0555 << 1))) = 0x090;
    }
    else
	*((volatile ushort*) addr) = 0x0090;
    *((volatile ushort*) addr) = 0x0000;
    *romdesc &= 0xFF7FFFFF;	/* bit 23: disable write access */
}

/*******************************************************************************/

static int wait_for_write_exec_done(ulong sect_addr)
{
    /* not as shown in the datasheet, because we can not read back data while 
       in program mode */
    u16 d1, d2;

    while (1)
    {
        /* read status byte twice */
        d1 = *((volatile ushort*)(sect_addr));
	d2 = *((volatile ushort*)(sect_addr));

	if ((d1 & 0x40) == (d2 & 0x40))
	{
	    /* no more toggle, so the flash is ready */
	    return ERR_OK;
	}
    }
}

/*******************************************************************************/

static int wait_for_erase_exec_done(ulong sect_addr)
{
    /* as shown in datasheet page 32, Figure 7.4 */
    u16 d1, d2, d3;

    while (1)
    {
	/* read 1, check DQ7 */
	d1 = *((volatile ushort*)(sect_addr));

	if (d1 & 0x80)
	{
	    /* data is valid, read status twice */
	    d2 = *((volatile ushort*)(sect_addr));
	    d3 = *((volatile ushort*)(sect_addr));

	    if ((d2 & 0x40) == (d3 & 0x40))
	    {
		/* no mor toggling, check DQ2 */
		if ((d2 & 0x04) == (d3 & 0x04))
		    return ERR_OK;
		else
		    return ERR_NOT_ERASED;	/* device in Erase/Suspend Mode */
	    }
	    else
		return ERR_NOT_ERASED;	/* device error */
	}
	else
	{
	    /* data not (yet) valid, check DQ5 */
	    if (d1 & 0x20)
	    {
		d2 = *((volatile ushort*)(sect_addr));
		d3 = *((volatile ushort*)(sect_addr));

		/* check DQ6 */
		if ((d2 & 0x40) != (d3 & 0x40))
		    return ERR_TIMOUT;		/* operation timed out */
		/* else device busy, repoll */
	    }
	    /* else device busy, repoll */
	}
    }
}

/*******************************************************************************/

static int wait_for_ppb_exec_done(ulong sa0, ulong sa, ushort erase)
{
    /* as shown in datasheet page 39, Figure 8.2 */

    u16 d1, d2;

    while (1)
    {
	/* read status byte twice */
        d1 = *((volatile ushort*)(sa0));
        d2 = *((volatile ushort*)(sa0));

	if ((d1 & 0x40) == (d2 & 0x40))
	{
	    /* no more toggle of DQ6, so the flash is ready */
	    udelay(500);			/* wait 500 us */
	    d1 = *((volatile ushort*)(sa + 4));
	    if ((d1 & 1) == (erase & 1))	/* DQ0 equals the erase/program state ? */
		return ERR_OK;
	    else
	    {
		printf("%s(0x%.8lX, 0x%.8lX, %d); DQ0 verify failed\n", __FUNCTION__, sa0, sa, erase);
		return ERR_TIMOUT;
	    }
	}
	else
	{
	    /* check DQ5 */
	    if (d1 & 0x20) 
	    {
		/* check for togglind DQ6 once more */
    		d1 = *((volatile ushort*)(sa0));
    		d2 = *((volatile ushort*)(sa0));

		if ((d1 & 0x40) == (d2 & 0x40))
		{
		    udelay(500);			/* wait 500 us */
		    d1 = *((volatile ushort*)(sa + 4));
		    if ((d1 & 1) == (erase & 1))	/* DQ0 equals the erase/program state ? */
			return ERR_OK;
		    else
		    {
			printf("%s(0x%.8lX, 0x%.8lX, %d); DQ0 verify failed (repeat)\n", __FUNCTION__, sa0, sa, erase);
			return ERR_TIMOUT;
		    }
		}
	    }
	}
    }

    /* send a reset command - normally, but leave_flash_pgm() must be called, which includes an device reset */
    
}

/*******************************************************************************/
/* Detect the installed flash chips and initalize the controller in that way
   that we have an continous address range starting at 0xF0000000. This
   routine requires, that u-boot was relocated into RAM.                       */

ulong flash_init(void)
{
    ulong  full_size = 0;
    ushort bank;
    ulong  curr_addr = 0xF0000000;
    ulong  data;

    flash_flags = 0;

    for (bank = 0; bank < CONFIG_SYS_MAX_FLASH_BANKS; bank++)
    {
	/* setup the ROM descriptor and enable write access */
	volatile u32 *romdesc = (volatile u32*)(ROM_DESC0_REG + (bank * 4));
	*romdesc &= 0x7FFFF000;	/* bit 31: ROM descritptor, bit 10 ... 8: clear CS, bit 4: 16 bit ROM */
	*romdesc |= 0x00000020;	/* bit 5: 16 bit ROM */
	*romdesc |= bank << 8;	/* bit 10 ... 8: set CS */

	/* enable ROM_MAP_REG to have access to the next possible available chip */
	volatile u32 *rommap = (volatile u32*)(ROM_MAP_REG + (bank * 4));
	*rommap = (curr_addr & 0x0FFF0000) | 0x00000FFF;

	/* disable XOE for this CS */
	volatile u32 *romxoe = (volatile u32*) ROM_XOE_MASK_REG;
	*romxoe |= (1 << bank);

	/* timing stuff this CS */
	volatile u32 *romext = (volatile u32*)(0xE0010080 + (bank * 4));
	*romext = 0x00020021;

	/* enter auto select command state */
	exec_flash_cmd(bank, curr_addr, curr_addr, CMD_AUTOSELECT);

	/* read the manufacture ID / basic product ID */
	volatile u32 *read_addr = (volatile u32*) curr_addr;
	data = *read_addr;

	if (data != 0x227E0001)		/* Spansion S29GLxxxP (Mirrorbit TM) series */
	{
	    exec_flash_cmd(bank, curr_addr, curr_addr, CMD_READ_ARRAY);
	    *romdesc |= 0x80000000;	/* bit 31: ROM descritptor - no supported ROM present */
	    continue;
	}

	flash_info[bank].flash_id        = (data << 16) | (data >> 16);
	flash_info[bank].manufacturer_id = data & 0xFFFF;
	flash_info[bank].device_id       = data >> 16;

	/* read extended chip IDs*/
	read_addr = (volatile u32*)(curr_addr + 0x1C);
	data = *read_addr;

	flash_info[bank].device_id2 = data & 0xFFFF;
	flash_info[bank].device_id3 = data >> 16;

	flash_info[bank].bank_number = bank;

	/* fill in the rest of flash_info */
	switch (flash_info[bank].flash_id)
	{
	    case 0x0001227E:
	    {
		flash_info[bank].sector_size = SZ_128K;
		switch (flash_info[bank].device_id2)
		{
		    case 0x2228:	/* 128 MB */
		        flash_info[bank].size         = 0x08000000;
		        flash_info[bank].sector_count = 1024;
		        flash_info[bank].name         = "Spansion S29GL01GP (1024 Mbit / 64M x 16)";
		        break;
		    case 0x2223:	/* 64 MB */
		        flash_info[bank].size         = 0x04000000;
		        flash_info[bank].sector_count = 512;
		        flash_info[bank].name         = "Spansion S29GL512P (512 Mbit / 32M x 16)";
		        break;
		    case 0x2222:	/* 32 MB */
		        flash_info[bank].size         = 0x02000000;
		        flash_info[bank].sector_count = 256;
		        flash_info[bank].name         = "Spansion S29GL256P (256 Mbit / 16M x 16)";
		        break;
		    case 0x2221:	/* 16 MB */
		        flash_info[bank].size         = 0x01000000;
		        flash_info[bank].sector_count = 128;
		        flash_info[bank].name         = "Spansion S29GL128P (128 Mbit / 8M x 16)";
		        break;
		    default:
		        flash_info[bank].size         = 0x00000000;
		        flash_info[bank].sector_count = 0;
		        flash_info[bank].name         = "Unknown Spansion S29GLxxxP";
		        break;
		}
		break;
	    }
	    default:
		flash_info[bank].size         = 0x00000000;
		flash_info[bank].sector_count = 0;
		flash_info[bank].name         = "Unknown device";
		break;
	}

	if (flash_info[bank].size > 0)
	{
	    /* setup the size in the SoC, to have proper address translation */
	    rommap = (volatile u32*)(ROM_MAP_REG + (bank * 4));
	    u32 size_mask = 0;

	    if (flash_info[bank].size == SZ_64K)
		size_mask = 0x0FFF;
	    else if (flash_info[bank].size == SZ_128K)
		size_mask = 0x0FFE;
	    else if (flash_info[bank].size == SZ_256K)
		size_mask = 0x0FFC;
	    else if (flash_info[bank].size == SZ_512K)
		size_mask = 0x0FF8;
	    else if (flash_info[bank].size == SZ_1M)
		size_mask = 0x0FF0;
	    else if (flash_info[bank].size == SZ_2M)
		size_mask = 0x0FE0;
	    else if (flash_info[bank].size == SZ_4M)
		size_mask = 0x0FC0;
	    else if (flash_info[bank].size == SZ_8M)
		size_mask = 0x0F80;
	    else if (flash_info[bank].size == SZ_16M)
		size_mask = 0x0F00;
	    else if (flash_info[bank].size == SZ_32M)
		size_mask = 0x0E00;
	    else if (flash_info[bank].size == SZ_64M)
		size_mask = 0x0C00;
	    else if (flash_info[bank].size == SZ_128M)
		size_mask = 0x0800;
	    else if (flash_info[bank].size == SZ_256M)
		size_mask = 0x0000;

	    *rommap = ((curr_addr & 0x0FFF0000) | size_mask);	/* offset from 0xF0000000 to the start address of the flash */

	    ulong sect;
	    for (sect = 0; sect < flash_info[bank].sector_count; sect++)
	    {
		flash_info[bank].start[sect] = curr_addr + (sect * flash_info[bank].sector_size);
		/* we are still in auto select command mode, so read the protection status of each sector */
		read_addr = (volatile u32*)(flash_info[bank].start[sect] + 4);
		data = *read_addr;
		flash_info[bank].protect[sect] = data & 0xFFFF;
	    }
	    /* leave auto select command state */
	    exec_flash_cmd(bank, curr_addr, curr_addr, CMD_READ_ARRAY);

	    /* read ESN - this is special for our Flash usage */
	    exec_flash_cmd(bank, curr_addr, curr_addr, CMD_SSP_ENTER);
	    read_addr = (volatile u32*) (curr_addr + 0x30);
	    data = *read_addr;
	    flash_info[bank].serial[7] = (data >> 24) & 0xFF;
	    flash_info[bank].serial[6] = (data >> 16) & 0xFF;
	    flash_info[bank].serial[5] = (data >>  8) & 0xFF;
	    flash_info[bank].serial[4] =  data        & 0xFF;

	    read_addr = (volatile u32*) (curr_addr + 0x34);
	    data = *read_addr;
	    flash_info[bank].serial[3] = (data >> 24) & 0xFF;
	    flash_info[bank].serial[2] = (data >> 16) & 0xFF;
	    flash_info[bank].serial[1] = (data >>  8) & 0xFF;
	    flash_info[bank].serial[0] =  data        & 0xFF;
	    leave_flash_pgm(bank, curr_addr, 1);

	    /* increase address and size */
	    curr_addr +=  flash_info[bank].size;
	    full_size +=  flash_info[bank].size;

	    #if defined (BEAUTIFY_CONSOLE) && (CFG_EXT_LEGACY_FLASH)
	    /* because the flash_info is not exported, this output is done here */
	    printf("\xBA %.8lX \xB3 %.8lX \xB3 FLASH \xB3 %-46s \xBA\n", flash_info[bank].start[0], flash_info[bank].start[0] + flash_info[bank].size - 1, flash_info[bank].name);
	    #endif
	}
	else
	    bank = CONFIG_SYS_MAX_FLASH_BANKS;	/* cancel auto detect */

    }

    /* protect our self */
    flash_protect(FLAG_PROTECT_SET, CONFIG_SYS_FLASH_BASE, CONFIG_SYS_FLASH_BASE + monitor_flash_len - 1, &flash_info[0]);
    #ifdef CONFIG_ENV_IS_IN_FLASH
    flash_protect(FLAG_PROTECT_SET, CONFIG_ENV_ADDR,       CONFIG_ENV_ADDR     + CONFIG_ENV_SIZE     - 1, &flash_info[0]);
    #endif

    /* print out the serial info */
    #if defined (BEAUTIFY_CONSOLE) && (CFG_EXT_LEGACY_FLASH)
    printf("\xCC\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD" \
	   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB9\n");
    printf("\xBA Serial number: %.3d-%.8d (%.2X-%.2X%.2X%.2X)                                      \xBA\n", 
	    flash_info[0].serial[4], (flash_info[0].serial[5] << 16) | (flash_info[0].serial[6] << 8) | flash_info[0].serial[7], 
	    flash_info[0].serial[4], flash_info[0].serial[5], flash_info[0].serial[6], flash_info[0].serial[7]);
    #endif
	
    return full_size;
}

/*******************************************************************************/
/* print out some informations about the installed Flash(es) (cmd "flinfo")    */

void flash_print_info(flash_info_t *info)
{
    u32 sect;

    if (info->size > 0)
    {
        /* the bank has an flash chip installed */
        printf("    %s\n              %ld MB in %d uniform sectors\n", info->name, info->size >> 20, info->sector_count);

	printf("              \xDA\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xBF\n");
	printf("              \xB3 %-59s \xB3\n", "Sector status");
	printf("              \xC3\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC2\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xB4\n");
	for (sect = 0; sect < info->sector_count; sect++)
	{
	    if ((sect % 48) == 0)
	    {
		if (sect > 0)
		    printf(" \xB3\n");
		printf("              \xB3 %.8lX \xB3 ", info->start[sect]);
	    }
	    /* check, if the sector is empty */
	    ulong not_empty = 0;
	    ulong addr_count = 0;
	    volatile u32 *read_addr = (volatile u32*)info->start[sect];

	    while (addr_count < info->sector_size)
	    {
		if (*read_addr != 0xFFFFFFFF)
		{
		    not_empty = 1;
		    addr_count = info->sector_size;
		}
		read_addr++;	/* 32-bit pointer, so it incrememts by 32 bit! */
		addr_count += 4;
	    }
	    if (not_empty)
	        printf("%c", info->protect[sect] ? '#' : '*');
	    else
		printf("%c", info->protect[sect] ? '_' : '.');
	}
	if (sect % 48)
	{
    	    while (sect % 48)
	    {
		printf(" ");
		sect++;
	    }
	    printf(" ");
	}
	printf("\xB3\n");
	printf("              \xC3\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xB4\n");
	printf("              \xB3 . empty, _ empty & protected, * used, # used & protected    \xB3\n");
	printf("              \xC0\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xD9\n");
    }
    else
	printf("NO FLASH INSTALLED\n");
}

/*******************************************************************************/
/* erase flash sectors 							       */

int flash_erase(flash_info_t *info, int s_first, int s_last)
{
    int ret = ERR_OK;
    u32 sect;

    /* reject all types of flashes which we do not know to work with this routines */
    if (info->flash_id != 0x0001227E)
	return ERR_UNKNOWN_FLASH_TYPE;

    if ((s_first < 0) || (s_first > s_last))
	return ERR_INVAL;

    for (sect = s_first; sect <= s_last; sect++)
    {
	if (info->protect[sect])
	    return ERR_PROTECTED;
    }

    display_set_text("Erase Flash");

    #ifdef CONFIG_USE_IRQ
    int irq_status = disable_interrupts();
    #endif

    /* Start erase on unprotected sectors */
    for (sect = s_first; sect <= s_last && !ctrlc(); sect++)
    {
	if ((info->start[sect] >= CONFIG_SYS_MONITOR_BASE) && (info->start[sect] < CONFIG_SYS_MONITOR_BASE + monitor_flash_len))
	{
	    /* the user try to erase ourself. So ech, if he has enabled the hidden stuff. */
	    if ((flash_flags & FLASH_ERASE_ALL) == 0)
	    {
		printf("Sector at 0x%.8lX not erased. You are not allowed to erase the bootloader!\n", info->start[sect]);
		continue;
	    }
	    else
		printf("Warning, you erased the bootloader. Your box will be dead, after the next reboot\nif no new bootloader is written. !\n");
	}

	display_show_progress(s_first, s_last, sect);
	/* for speed up, first test, if the requested sector is empty or not */
	u32 not_empty = 0;
	u32 addr_count = 0;
	volatile u32 *read_addr = (volatile u32*)(info->start[sect] + addr_count);

	while(addr_count < info->sector_size)
	{
	    if (*read_addr != 0xFFFFFFFF)
	    {
	        not_empty = 1;
	        addr_count = info->sector_size;
	    }
	    read_addr++;
	    addr_count += 4;
	}

	if (not_empty)
	{
    	    exec_flash_cmd(info->bank_number, info->start[0], info->start[0], CMD_ERASE_SETUP);
    	    exec_flash_cmd(info->bank_number, info->start[0], info->start[sect], CMD_ERASE_CONFIRM);

	    /* datasheet page 26, figure 7.3 - poll DQ3 until it becomes 1 */
	    volatile u16 *stat_addr = (volatile u16 *)(info->start[sect]);
	    while ((*stat_addr & 0x08) == 0x08)
	    {
		;
	    }

	    /* execute data polling algo */
	    ret = wait_for_erase_exec_done(info->start[sect]);

    	    exec_flash_cmd(info->bank_number, info->start[0], info->start[0], CMD_READ_ARRAY);
	}
	putc('.');
    }

    #ifdef CONFIG_USE_IRQ
    if (irq_status)
	enable_interrupts();
    #endif

    printf("\n");
    display_show_progress(0, 0, 0);
    display_set_text(NULL);

    return ret;
}

/*******************************************************************************/
/* write a block of data from given src and with given length into flash.
   The Spansion Chips support page write, so we can accellerate the flash 
   programming process by writing the data in pages of 32 words (64 byte).     */

int write_buff(flash_info_t *info, uchar *src, ulong addr, ulong cnt)
{
    int ret = ERR_OK;

    /* reject all types of flashes which we do not know to work with this routines */
    if (info->flash_id != 0x0001227E)
	return ERR_UNKNOWN_FLASH_TYPE;

    #ifdef CONFIG_USE_IRQ
    int irq_status = disable_interrupts();
    #endif

    u32 pages  = cnt / 64;
    u32 rbytes = cnt % 64;
    u32 current_page;
    u16 *sdata = (u16*) src;
    u16 vdata;
    volatile u16 *dstaddr = (volatile u16*) addr;
    u32 sector = (addr - info->start[0]) / info->sector_size;
    volatile u32 *romdesc = (volatile u32*)(ROM_DESC0_REG + (info->bank_number * 4));

    display_set_text("Write Flash");

    /* first write the pages */
    for (current_page = 0; current_page < pages; current_page++)
    {
	display_show_progress(0, pages - 1, current_page);

	exec_flash_cmd(info->bank_number, info->start[0], info->start[sector], CMD_WRITE_BUFF_SETUP);

        /* enter uncashed 16-bit mode */
        *romdesc |= 0x00800000;

	u16 wordcount = 0;
	*((volatile ushort*)(info->start[sector])) = 31;	/* words minus one */

	while (wordcount < 32)
	{
	    /* write a page of 32 words (64 bytes) into the flash's volatile buffer */
	    *dstaddr = *sdata;
	    dstaddr++;
	    vdata = *sdata;
	    sdata++;
	    wordcount++;
	}
	
	*((volatile ushort*)(info->start[sector])) = CMD_WRITE_BUFF_CONFIRM;

	ret = wait_for_write_exec_done(info->start[sector]);

	if (ret != ERR_OK)
	    printf("error while flash write (%d)\n", ret);

	u32 old_sector = sector;
	sector = ((u32)dstaddr - info->start[0]) / info->sector_size;
	if (sector != old_sector)
	    putc('.');

	*romdesc &= 0xFF7FFFFF;	/* bit 23: disable write access */
    }

    /* write the outstanding bytes */
    if (rbytes % 2)	/* round up, if odd byte count */
	rbytes++;
    u32 rwords = rbytes >> 1;	/* divide by two to have word count */

    if (rwords)
    {
	u16 wordcount = 0;
	while (wordcount < rwords)
	{
	    exec_flash_cmd(info->bank_number, info->start[0], info->start[sector], CMD_WRITE_WORD_SETUP);
    	    *romdesc |= 0x00800000;
	    *dstaddr = *sdata;
	    vdata = *sdata;
	    dstaddr++;
	    sdata++;
	    wordcount++;
	    ret = wait_for_write_exec_done(info->start[sector]);

	    *romdesc &= 0xFF7FFFFF;
	}
    }

    if (ret != ERR_OK)
	exec_flash_cmd(info->bank_number, info->start[0], info->start[0], CMD_READ_ARRAY);

    #ifdef CONFIG_USE_IRQ
    if (irq_status)
	enable_interrupts();
    #endif

    putc('\n');
    display_show_progress(0, 0, 0);
    display_set_text(NULL);

    return ret;
}

/*******************************************************************************/
/* set the protection bit for the given sector				       */

int flash_real_protect(flash_info_t *info, long sector, int prot)
{
    int ret = ERR_OK;

    /* reject all types of flashes which we do not know to work with this routines */
    if (info->flash_id != 0x0001227E)
	return ERR_UNKNOWN_FLASH_TYPE;

    #ifdef CONFIG_USE_IRQ
    int irq_status = disable_interrupts();
    #endif

    if (info->protect[sector] != prot)
    {
	/* only do anything, if it's necessary */
	if (prot)
	{
	    /* protecting is easy */
	    /* enter auto select command state */
	    exec_flash_cmd(info->bank_number, info->start[0], info->start[0], CMD_PPB_ENTER);

	    /* enter uncashed 16-bit mode */
    	    volatile u32 *romdesc = (volatile u32*)(ROM_DESC0_REG + (info->bank_number * 4));
	    *romdesc |= 0x00800000;

	    /* protect by clearing the bit */
	    *((volatile ushort*)(info->start[0]))      = 0x00A0;
	    *((volatile ushort*)(info->start[sector])) = 0x0000;

	    ret = wait_for_ppb_exec_done(info->start[0], info->start[sector], 0);

	    if (ret == ERR_OK)
		info->protect[sector] = 1;

	    leave_flash_pgm(info->bank_number, info->start[sector], 0);
	}
	else
	{
	    /* this is a little bit more work, because protection can only be disabled globally */
	    exec_flash_cmd(info->bank_number, info->start[0], info->start[0], CMD_PPB_ENTER);

	    /* enter uncashed 16-bit mode */
    	    volatile u32 *romdesc = (volatile u32*)(ROM_DESC0_REG + (info->bank_number * 4));
	    *romdesc |= 0x00800000;

	    /* unprotect all */
	    *((volatile ushort*)(info->start[0])) = 0x0080;
	    *((volatile ushort*)(info->start[0])) = 0x0030;

	    ret = wait_for_ppb_exec_done(info->start[0], info->start[sector], 1);

	    leave_flash_pgm(info->bank_number, info->start[0], 0);

	    /* unset protection status for this sector */
	    if (ret == ERR_OK)
		info->protect[sector] = 0;

	    /* loop through all sectors and reprotect if necessary */
	    u32 sect;
	    for (sect = 0; sect < info->sector_count; sect++)
	    {
		if (info->protect[sect])
		{
		    info->protect[sect] = 0;		/* indicate, this sector is not protected */
		    flash_real_protect(info, sect, 1);	/* call ourself to reprotect */
		}
	    }
	}
    }

    #ifdef CONFIG_USE_IRQ
    if (irq_status)
	enable_interrupts();
    #endif

    return ret;
}

/*******************************************************************************/

void flash_set_flag(void)
{
    flash_flags |= FLASH_ERASE_ALL;
}

/*******************************************************************************/

#ifdef CONFIG_SYS_FLASH_OTP

int flash_dump_otp(flash_info_t *info)
{
    #ifdef CONFIG_USE_IRQ
    int irq_status = disable_interrupts();
    #endif

    volatile u32 *read_addr;
    u32 data;
    unsigned int ofs;

    /* enter SSP mode */
    exec_flash_cmd(0, info->start[0], info->start[0], CMD_SSP_ENTER);

    /* flash's SSP size is 128 word (256 byte) */
    printf("otpdata: ");
    for (ofs = 0; ofs < 256; ofs += 4)
    {
	read_addr = (volatile u32*) (info->start[0] + ofs);
	data = *read_addr;
	printf("%.2X%.2X%.2X%.2X", data & 0xFF, (data >> 8) & 0xFF, (data >> 16) & 0xFF, (data >> 24) & 0xFF);
    }
    printf("\n\n");

    /* leave SSP mode */
    leave_flash_pgm(0, info->start[0], 1);

    #ifdef CONFIG_USE_IRQ
    if (irq_status)
	enable_interrupts();
    #endif

    return ERR_OK;
}

/*******************************************************************************/

int flash_write_otp(flash_info_t *info, uchar *data, ulong cnt)
{
    #ifdef CONFIG_USE_IRQ
    int irq_status = disable_interrupts();
    #endif

    int ret = ERR_OK;
    int ofs;
    int rbytes = cnt;
    u16 *wdata = (u16*) data;
    volatile u16 *dstaddr = (volatile u16*) info->start[0];
    volatile u32 *romdesc = (volatile u32*)(ROM_DESC0_REG + (info->bank_number * 4));

    u8 *sdata = (u8*) data;
    printf("writing the following %ld byte of data into flash:\n", cnt);
    for (ofs = 0; ofs < 256; ofs += 4)
    {
	if ((ofs % 16) == 0)
	    printf("\n0x%.4X: ", ofs);

	printf("%.2X %.2X %.2X %.2X ", sdata[ofs], sdata[ofs + 1], sdata[ofs + 2], sdata[ofs + 3]);
    }
    printf("\n");


    exec_flash_cmd(0, info->start[0], info->start[0], CMD_SSP_ENTER);


    display_set_text("Write OTP");

    /* write the outstanding bytes */
    if (rbytes % 2)	/* round up, if odd byte count */
	rbytes++;
    u32 rwords = rbytes >> 1;	/* divide by two to have word count */

    if (rwords)
    {
	u16 wordcount = 0;
	while (wordcount < rwords)
	{
	    exec_flash_cmd(info->bank_number, info->start[0], info->start[0], CMD_WRITE_WORD_SETUP);
    	    *romdesc |= 0x00800000;
	    *dstaddr = *wdata;
	    dstaddr++;
	    wdata++;
	    wordcount++;
	    ret = wait_for_write_exec_done(info->start[0]);

	    *romdesc &= 0xFF7FFFFF;
	}
    }

    /* leave SSP mode */
    leave_flash_pgm(0, info->start[0], 1);

    #ifdef CONFIG_USE_IRQ
    if (irq_status)
	enable_interrupts();
    #endif

    putc('\n');
    display_set_text(NULL);

    return ret;
}

#endif /* CONFIG_SYS_FLASH_OTP */
