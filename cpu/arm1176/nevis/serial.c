/*
 * low level UART routines for CX2450x/ARM1176 SoC (Nevis)
 *
 * (c) 2008 Coolstream International Limited
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
 *
 */

#include <common.h>
#include <asm/arch/nevis.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_NEVIS

#ifdef CONFIG_SERIAL1
#define UART_NR	0

#elif defined(CONFIG_SERIAL2)
#define UART_NR	1

#elif defined(CONFIG_SERIAL3)
#define UART_NR	2

#elif defined(CONFIG_SERIAL4)
#define UART_NR	3

#else
#error "Bad: you didn't configure serial ..."
#endif

/*******************************************************************************/

/* register definitions. The UART works nearly similar to 15550 standard UART,
   except all the Flow-Control stuff. */

enum 
{
    UART_FIFO_BRDL	= 0x00,		/* u8 - FRMC:BDS = 0: Write: Top of TX-FIFO, Read: Bottom of RX-FIFO. */
    UART_IRQE_BRDU	= 0x04,		/* u8 - FRMC:BDS = 0: Interrupt Enable Register. */
    UART_FIFC		= 0x08,		/* u8 - FIFO Control Register */
    UART_FRMC		= 0x0C,		/* u8 - Frame Control Register */
    UART_STAT		= 0x14,		/* u8 - Status Register */
    UART_IRVL		= 0x18,		/* u8 - Interrup Level Register */
    UART_IRDC		= 0x20,		/* u8 - IrDA Control Register */
    UART_TXSTA		= 0x28,		/* u8 - Transmit FIFO Status Register */
    UART_RXSTA		= 0x2C,		/* u8 - Receive FIFO Status Register */
    UART_EXP		= 0x30,		/* u8 - Expansion Register */
    UART_MDMC		= 0x34		/* u8 - Modem Control Register */
};

/*******************************************************************************/

/* Setup the baudrate */

void serial_setbrg(void)
{
    volatile u8 *FIFO_BRDL = (volatile u8*)(UART_BASE_REG + UART_FIFO_BRDL + (UART_NR * 0x1000));
    volatile u8 *IRQE_BRDU = (volatile u8*)(UART_BASE_REG + UART_IRQE_BRDU + (UART_NR * 0x1000));
    volatile u8 *FRMC      = (volatile u8*)(UART_BASE_REG + UART_FRMC      + (UART_NR * 0x1000));
    volatile u8 *EXP       = (volatile u8*)(UART_BASE_REG + UART_EXP       + (UART_NR * 0x1000));

    u32 brdiv = (54000000 / (16 * gd->baudrate)) - 1;

    *FRMC     |= 0x80;		/* set BDS to access the baudrate registers */
    *FIFO_BRDL = brdiv & 0xFF;
    *IRQE_BRDU = (brdiv >> 8) & 0xFF;

    /* fractional part of brdiv - from the formula above, rounded to 0 (0x00), 1/4 (0x01), 1/2 (0x02) or 3/4 (0x03) */
    switch (gd->baudrate)
    {
	case 9600:
	case 57600:
	    *EXP = 0x02;
	    break;
	case 19200:
	case 38400:
	    *EXP = 0x03;
	    break;
	case 115200:
	    *EXP = 0x01;
	    break;
	default:
	    *EXP = 0;
    }

    *FRMC     &= 0x7F;		/* unset BDS to have FIFO access */
    *IRQE_BRDU = 0x00;		/* disable interrupts */
}

/*******************************************************************************/

/* Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits. */

int serial_init(void)
{
    volatile u8 *FRMC = (volatile u8*)(UART_BASE_REG + UART_FRMC + (UART_NR * 0x1000));
    volatile u8 *IRDC = (volatile u8*)(UART_BASE_REG + UART_IRDC + (UART_NR * 0x1000));
    volatile u8 *FIFC = (volatile u8*)(UART_BASE_REG + UART_FIFC + (UART_NR * 0x1000));
    volatile u8 *MDMC = (volatile u8*)(UART_BASE_REG + UART_MDMC + (UART_NR * 0x1000));

    /* setup baudrate */
    serial_setbrg();

    *FRMC = 0x01;	/* default setup: 8 databits, 1 stop bit, no parity */ 
    *IRDC = 0x00;	/* Disable IrDA */
    *FIFC = 0x03;
    *MDMC = 0x00;	/* set RTS and DTR */

#ifdef BEAUTIFY_CONSOLE
    /* some VTxxx control character to initalize the console */
    const char *cls = {"\x1B[2h"	/* enable ANSI compatibility */
		       "\x1B[61\x22p"	/* switch to VT100 mode - most PC-Terminal-Emulators do not know this command */
		       "\x1B[m"		/* turn all attributes off */
		       "\x1B[44m"
		       "\x1B[37m"
		       "\x1B[1m"
		       "\x1B[2J"	/* erase entire screen */
		       "\x1B[1;1H"	/* move cursor to upper left corner */
		       "\x07"		/* BEL */
		       "\x1B[(B"	/* US character set as G0 */
		       "\x1B[)B"	/* US character set as G1 */
		       "\x1B[?3l"	/* 80 column mode */
		       "\x1B[?9h"};	/* 40 line mode */

    const char *topline = {"\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
			   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
			   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
			   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
			   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
			   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
			   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
			   "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\n"};
    printf("%s", cls);
    printf("%s", topline);
#endif /* BEAUTIFY_CONSOLE */

    return (0);
}

/*******************************************************************************/

/* Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c. */

int serial_getc(void)
{
    s32 ret;
    s32 error = 0;
    volatile u8 *FIFO_BRDL = (volatile u8*)(UART_BASE_REG + UART_FIFO_BRDL + (UART_NR * 0x1000));
    volatile u8 *STAT      = (volatile u8*)(UART_BASE_REG + UART_STAT      + (UART_NR * 0x1000));
    volatile u8 *RXSTA     = (volatile u8*)(UART_BASE_REG + UART_RXSTA     + (UART_NR * 0x1000));

    /* wait for character to arrive */
    while (!(*RXSTA & 0x1F));

    /* the datasheet request to read the Status register everytime before getting
       the next char from the FIFO */
    if ((*STAT & 0x1C))
	error = 1;
    
    /* even if there was an data error, we have to read the FIFO */
    ret = *FIFO_BRDL & 0xFF;

    return (error != 0) ? 0x00 : ret;
}

/*******************************************************************************/

/* Output a single byte to the serial port. */

void serial_putc(const char c)
{
    volatile u8 *FIFO_BRDL = (volatile u8*)(UART_BASE_REG + UART_FIFO_BRDL + (UART_NR * 0x1000));
    volatile u8 *TXSTA     = (volatile u8*)(UART_BASE_REG + UART_TXSTA     + (UART_NR * 0x1000));

    /* wait for room in the tx FIFO */
    while ((*TXSTA & 0x1F));

    *FIFO_BRDL = c;

    /* If \n, also do \r */
    if (c == '\n')
	serial_putc('\r');
}

/*******************************************************************************/

/* Test whether a character is in the RX buffer */
int serial_tstc(void)
{
    volatile u8 *RXSTA = (volatile u8*)(UART_BASE_REG + UART_RXSTA + (UART_NR * 0x1000));

    return *RXSTA & 0x1F;
}

/*******************************************************************************/

void serial_puts(const char *s)
{
    while (*s)
    {
	serial_putc (*s++);
    }
}

#endif /* CONFIG_NEVIS */
