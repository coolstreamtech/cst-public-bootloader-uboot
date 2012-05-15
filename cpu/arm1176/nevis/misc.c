/*
 * misc. routines for CX2450x/ARM11 SoC
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#ifdef CONFIG_NEVIS
#include <asm/arch/nevis.h>
#include <asm/arch/sys_proto.h>

/* helper routines to lock / unlock the protected registers */
void LockRegs(u32 mask)
{
	/* access to the timer registers are globally locked */
	volatile u32 *lcmdreg = (volatile u32*) LOCKCMD_REG;
	volatile u32 *lstareg = (volatile u32*) LOCKSTAT_REG;

	/* unlock sequence for access to the LOCKSTAT_REG */
	*lcmdreg = 0x00;
	*lcmdreg = 0xF8;
	*lcmdreg = 0x2B;
	/* re-set the lock bits */
	*lstareg |= mask;
	*lcmdreg = 0x00;
}

void UnlockRegs(u32 mask)
{
	/* access to the timer registers are globally locked */
	volatile u32 *lcmdreg = (volatile u32*) LOCKCMD_REG;
	volatile u32 *lstareg = (volatile u32*) LOCKSTAT_REG;

	/* unlock sequence for access to the LOCKSTAT_REG */
	*lcmdreg = 0x00;
	*lcmdreg = 0xF8;
	*lcmdreg = 0x2B;
	/* clear the bits */
	*lstareg &= ~mask;      /* disable one of bit 31 ... 16 to unlock timer register */
	*lcmdreg = 0x00;
}

/* Conexant compiled kernel needs a few registers in the default
 * state. Make sure they are.
 */
void CleanupBeforeLinux(void)
{
	u32 i;

        /* unlock HACK for Conexant Linux Kernel */
	UnlockRegs(0xFFFF0000);
#if 1
        /* reset the timers to default values, again conexant is BAD! This must 
	   be done at the last point at all, because this also disables u-boots
	   system timer */
        for(i = 0; i < 16; i++) {
                u32 timer_index = i << 4;
                volatile u32 *tvalue  = (volatile u32*) (TIMER_BASE_REG + timer_index + 0);
                volatile u32 *tlimit  = (volatile u32*) (TIMER_BASE_REG + timer_index + 4);
                volatile u32 *tmode   = (volatile u32*) (TIMER_BASE_REG + timer_index + 8);
                volatile u32 *ttimeb  = (volatile u32*) (TIMER_BASE_REG + timer_index +12);

                /* setup the timer */
                *tvalue = 0;
                *tlimit = 0;
                *ttimeb = 0;
                *tmode  = 0;
        }
#endif

}

#endif
