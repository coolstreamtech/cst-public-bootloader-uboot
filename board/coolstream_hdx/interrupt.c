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
#include <asm/arch/fp_uart.h>
#include <asm/arch/sys_proto.h>

#ifdef CONFIG_USE_IRQ

static u32 last_btn = 0;
static u32 last_pos = 0;
static u32 found_ent= 0;

static u8 uart_rx_pos;
static u8 uart_rx_data[256];

/* HD1: Menu - OK - CH+ - OK - VOL- - OK - Stanby - Standby */
static const u32 btn_seq[8] = { 0x0C, 0x04, 0x06, 0x04, 0x03, 0x04, 0x08, 0x08 };

/*******************************************************************************/
/* basic initialization of the ITC                                             */
int cs_fp_key = 0;

void board_init_itc(void)
{
    u32 bank;

    /* set defaults */
    last_btn = 0;
    last_pos = 0;

    /* reset all pending interrupts on the GPIO-lines */
    for (bank = 0; bank < 7; bank++)
    {
	volatile u32 *pio_reg = (volatile u32*)(PIO_IRQSTAT_REG + (bank * 0x40));
        *pio_reg = 0xFFFFFFFF;
    }

    /* setup the Interrupt controller here */
    volatile u32 *G1 = (volatile u32*) 0xE0450000;	/* INTDEST1_REG */
    *G1 = 0xFFFFFFFF;   /* sources generates IRQ and no FIQ */

    G1 = (volatile u32*) 0xE0450004;			/* ITC_ENABLE1_REG */
    *G1 = 0x01126202;   /* 0000 0001 0001 0010 0110 0010 0000 0010 */ /* 23apr2010 enable UART2 IRQ */

    G1 = (volatile u32*) 0xE0450020;			/* INTDEST2_REG */
    *G1 = 0xFFFFFFFF;   /* sources generates IRQ and no FIQ */

    G1 = (volatile u32*) 0xE0450024;			/* ITC_ENABLE2_REG */
    *G1 = 0x00000080;   /* 0000 0000 0000 0000 0000 0000 1000 0000 */

    G1 = (volatile u32*) 0xE0450040;			/* INTDEST3_REG */
    *G1 = 0xFFFFFFFF;   /* sources generates IRQ and no FIQ */

    G1 = (volatile u32*) 0xE0450044;			/* ITC_ENABLE3_REG */
    *G1 = 0x00000000;   /* 0000 0001 0000 0000 0000 0000 0000 0000 */

    /* enable PIO 048, 049 (CI-slots) */
    G1 = (volatile u32*) 0xE0470054;			/* IRQ_ENABLE0_2_REG */
    *G1 = 0x00030000;   /* 0000 0000 0000 0011 0000 0000 0000 0000 PIO 032 */

    G1 = (volatile u32*) 0xE0470058;    		/* POS_EDGE_2_REG */
    *G1 = 0x00030000;   /* 0000 0000 0000 0011 0000 0000 0000 0000 PIO 032 */

    G1 = (volatile u32*) 0xE047005C;    		/* NEG_EDGE_2_REG */
    *G1 = 0x00030000;   /* 0000 0000 0000 0011 0000 0000 0000 0000 PIO 032 */


    /* enable interrupts for PIO 182, 183, 185, 186 (front panel button matrix) */
    G1 = (volatile u32*) 0xE0470154;			/* IRQ_ENABLE0_6_REG */
    *G1 = 0x06C00000;   /* 0000 0110 1100 0000 0000 0000 0000 0000 PIO 160 */

    G1 = (volatile u32*) 0xE0470158;    		/* POS_EDGE_6_REG */
    *G1 = 0x06C00000;   /* 0000 0110 1100 0000 0000 0000 0000 0000 PIO 160 */

    G1 = (volatile u32*) 0xE047015C;    		/* NEG_EDGE_6_REG */
    *G1 = 0x06C00000;   /* 0000 0110 1100 0000 0000 0000 0000 0000 PIO 160 */
}

/*******************************************************************************/
/* handle the received interrupts (read and reset the bits set in the ITC)     */
int board_do_interrupt(struct pt_regs *pt_regs)
{
    int ret = 0;
    u32 data, group, num;
    u32 reset_irq = 1;
    volatile u32 *itc_reg;

    for (group = 0; group < 3; group++)
    {
	itc_reg  = (volatile u32*)(ITC_INTRIRQ1_REG + (group * 0x20));
	data = *itc_reg;

	if (data)
	{
	    for (num = 0; num < 32; num++)
	    {
		if (data & (1 << num))
		{
		    s32 bank;
		    u32 irq = num + (group * 32);
		    switch (irq)
		    {
			case 1:				/* UART 2 */
			{
				u8 d;

				if (fp_uart_get_rx_count() == 0) { /* No data to read, just a timeout */
					d = fp_uart_get_rx();
				} else if (fp_uart_get_rx_count() > 0) {
					u32 btn_val;

					while(fp_uart_get_rx_count()) {
						d = fp_uart_get_rx();
						if (d == 0x02) { // STX
							uart_rx_pos = 0;
						} else if ((d == 0x03) && (uart_rx_data[1] == 'B')) { //ETX
							if (uart_rx_data[2] == 0x30) /* Button release */
								break;
							btn_val = uart_rx_data[2] & 0x0F;
							/* printf("Received button code: %02x\n", btn_val); */
							if (btn_val == 0x8)
								cs_fp_key = 1;
							uart_rx_data[1] = 0; /* Clear 'B' */
							/* check, if the user tries to enter the hidden stuff */
							if (btn_val == btn_seq[last_pos]) {
								last_pos++;
								if (last_pos == 8) {
									/* Tilt - the user found the hidden entrace */
									found_ent	= 1;
									last_pos	= 0;
//									
								}
							} else
								last_pos = 0;
						}
						uart_rx_data[uart_rx_pos++ & 255] = d;
					}
				}
				reset_irq = 1;
				break;
			}
			case 13:			/* USB 0 */
			case 17:			/* USB 1 */
			case 20:			/* USB 2 */
			    printf("Interrupt from USB (IRQ-num = %d)\n", irq);
			    break;
			case 14:			/* IRQ 14 = GPIO0 controller */
//			    printf("IRQ %.2d: GPIO0 ", irq);
			    for (bank = 6; bank >= 0; bank--)
			    {
				volatile u32 *pio_reg = (volatile u32*)(PIO_IRQSTAT_REG + (bank * 0x40));
				u32 pio_data = *pio_reg;

				if (bank == 5)	/* handle our front panel buttons */
				{
				    u32 btn_irq = ((pio_data >> 22) & 0x03) | ((pio_data >> 23) & 0x0C);
				    if (btn_irq)	/* one of the front buttons are pressed */
				    {
					u32 btn_val = (board_gpio_read(182) << 3) | 
						      (board_gpio_read(186) << 2) |
						      (board_gpio_read(185) << 1) |
						      (board_gpio_read(183) << 0);
					btn_val ^= 0x0F;
					/* printf("Received button code: %02x\n", btn_val); */

					if(btn_val == 0x8)
						cs_fp_key = 1;
					if (btn_val != last_btn)	/* debounce */
					{
					    last_btn = btn_val;
					    if (btn_val > 0)
					    {
						udelay(25000);		/* necessary here for debounce, belive me */
						/* check, if the user tries to enter the hidden stuff */
						if (btn_val == btn_seq[last_pos])
						{
						    last_pos++;
						    if (last_pos == 8)
						    {
							/* Tilt - the user found the hidden entrace */
							last_pos	= 0;
							found_ent	= 1;
						    }
						}
						else
						    last_pos = 0;
					    }
					}
				    }
				}

				/* reset */
				if (pio_data)
				    *pio_reg = pio_data;
			    }
			    break;
			case 24:			/* IRQ 24 = EMAC0 Ethernet controller */
			    reset_irq = eth_handle_interrupt();
			    break;
			default:
			    printf("IRQ %.2d: unknown source\n", irq);
			    break;
		    }
		}
	    }
	}
	/* reset */
	if (reset_irq)
	{
	    itc_reg = (volatile u32*)(ITC_INTRCLR1_REG + (group * 0x20));
	    *itc_reg = data;
	}

	if (found_ent)
	{
		printf("\nCongrats, you found the hidden entrace ;-)\n");
		flash_set_flag();
		display_show_icon(0x0B000001, 0);
		found_ent = 0;
	}
    }

    return ret;
}

/*******************************************************************************/

void board_disable_all_interrupts(void)
{
	volatile u32 *ie;

        /* Disable all Interrupts */
	ie = (volatile u32*)ITC_INTENABLE1_REG;
	*ie = (u32) 0;

	ie = (volatile u32*)ITC_INTENABLE2_REG;
	*ie =(u32) 0;
}



#endif
