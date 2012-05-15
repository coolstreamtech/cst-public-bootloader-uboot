/*
 * (C) Copyright 2008
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
#include <asm/arch/nevis.h>
#include <asm/arch/sys_proto.h>

/* some helpers for the GPIO controller */

/*******************************************************************************/
/* switch the pio number to the given state (PIO_HIGH, PIO_LOW, PIO_OFF)       */

void board_gpio_drive(u32 pio, u32 state)
{
    /* decode the requested PIO to the assigned bank/bit */

    volatile u32 *reg;
    u32 bank = pio / 32;
    u32 bit  = pio % 32;

    if (bank < 7)
    {
	reg = (volatile u32*)(state + (bank * 0x40));
	*reg = (1 << bit);
    }
}

/*******************************************************************************/
/* read the state of one GPIO pin (if configured as input before)              */

u32 board_gpio_read(u32 pio)
{
    volatile u32 *reg;
    u32 bank = pio / 32;
    u32 bit  = pio % 32;
    u32 ret  = 0;

    if (bank < 7)
    {
	reg = (volatile u32*)(PIO_READ_REG + (bank * 0x40));
	if ((*reg) & (1 << bit))
	    ret = 1;
    }

    return ret;
}
