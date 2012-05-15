/*
 * ITC and timer routines for CX2450x/ARM11 SoC
 *
 * (C) Copyright 2008
 * Coolstream International Limited
 *
 * Timer code derived from:
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
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

/* the Nevis SoC has 16 General Purpose Timers */

static const u32 timer_index = 0;

/*******************************************************************************/

/* some globals */
int timer_load_val = 0;
static ulong timestamp;
static ulong lastinc;

/*******************************************************************************/

/* macro to read the 16 bit timer */
static inline ulong READ_TIMER(void)
{
    volatile u32 *tvalue = (volatile u32*) (TIMER_BASE_REG + (timer_index << 4) + 0);
    return (ulong) *tvalue;
}

/*******************************************************************************/

int interrupt_init(void)
{
    u32 timer = timer_index << 4;

    /* the registers to access the first of the 16 general purpose timers. */
    volatile u32 *tvalue  = (volatile u32*) (TIMER_BASE_REG + timer + 0);
    volatile u32 *tlimit  = (volatile u32*) (TIMER_BASE_REG + timer + 4);
    volatile u32 *tmode   = (volatile u32*) (TIMER_BASE_REG + timer + 8);
    volatile u32 *ttimeb  = (volatile u32*) (TIMER_BASE_REG + timer +12);
    /* set the right lockbit according to the timer index */
    u32 lock_mask = 1 << (timer_index + 16); 

    /* step one: the system timer */

    UnlockRegs(lock_mask);
    /* setup the timer */
    *tvalue = 0;                /* start value is 0 */
    *tlimit = 0xFFFFFFFF;       /* the limit at which the timer rolls over to 0 */
    *ttimeb = 54;               /* the input clock for the timer is hardwired to 54 MHz. So 54 clock periods equals 1us stepping */
    *tmode  = 0x01;             /* set bit 0 to enable the timer */
    LockRegs(lock_mask);

    reset_timer_masked();

    /* step two: the interrupt controller 
       we do nothing here, because the only things to do with the ITC is to enabled
       the interrupt source, which depends on the board */

    return (0);
}

/*******************************************************************************/

void reset_timer(void)
{
    reset_timer_masked();
}

/*******************************************************************************/

void reset_timer_masked(void)
{
    lastinc = READ_TIMER();
    timestamp = 0;
}

/*******************************************************************************/

ulong get_timer(ulong base)
{
    return get_timer_masked() - base;
}

/*******************************************************************************/

ulong get_timer_masked(void)
{
    u32 now = READ_TIMER();

    if (now >= lastinc)                 /* normal mode (non roll) */
        timestamp += (now - lastinc);   /* move stamp fordward with absoulte diff ticks */
    else                                /* we have rollover of incrementer */
        timestamp += (0xFFFFFFFF - lastinc) + now;
        lastinc = now;
    return timestamp;
}

/*******************************************************************************/

void set_timer(ulong t)
{
    timestamp = t;
}

/*******************************************************************************/
/* delay x useconds AND perserve advance timstamp value                        */

void udelay(unsigned long usec)
{
    /* our timer runs at a resolution of 1 us, so we need no unsave calculations */
    u32 now = READ_TIMER();                     /* the current timer value */
    u32 end = (now + usec) & 0xFFFFFFFF;        /* the end value the timer must reach, including the timer rollover to 0 */

    if (end > now)
    {
        /* normal case without rollover */
        while (READ_TIMER() < end);
    }
    else
    {
        /* rollover case */
        while (READ_TIMER() > end);
        while (READ_TIMER() < end);
    }
}
									    
/*******************************************************************************/

/* waits specified delay value and resets timestamp */

void udelay_masked(unsigned long usec)
{
    ulong tmo;
    ulong endtime;
    signed long diff;

    if (usec >= 1000)			/* if "big" number, spread normalization to seconds */
    {
        tmo  = usec / 1000;		/* start to normalize for usec to ticks per sec */
        tmo *= CFG_HZ;			/* find number of "ticks" to wait to achieve target */
        tmo /= 1000;			/* finish normalize. */
    }
    else                                /* else small number, don't kill it prior to HZ multiply */
    {
        tmo  = usec * CFG_HZ;
        tmo /= (1000*1000);
    }

    endtime = get_timer_masked () + tmo;

    do
    {
        ulong now = get_timer_masked ();
        diff = endtime - now;
    } while (diff >= 0);
}
													    
/*******************************************************************************/
/* This function is derived from PowerPC code (read timebase as long long).
   On ARM it just returns the timer value.				       */

unsigned long long get_ticks(void)
{
   return get_timer(0);
}

/*******************************************************************************/
/* This function is derived from PowerPC code (timebase clock frequency).
   On ARM it returns the number of timer ticks per second.		       */

ulong get_tbclk(void)
{
    return 54000000;
}

/*******************************************************************************/
/* reset the cpu by writing something into the reset register		       */

void reset_cpu(ulong ignored)
{
    /* every write access - independend of the written value - 
       will cause a hard reset */

    volatile u32 *reg = (volatile u32*) SOFTRESET_REG;
    *reg = 0x0000000;
}

/*******************************************************************************/
/* handle incoming interrupts                                                  */

void do_irq(struct pt_regs *pt_regs)
{
    /* call the boards interupt handler */

    if (board_do_interrupt(pt_regs))
    {
	printf ("Warning, unhandled IRQ\n");
	show_regs(pt_regs);
    }
}

#endif /* CONFIG_NEVIS */
