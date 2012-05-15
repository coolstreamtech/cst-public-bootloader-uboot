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
#include <version.h>
#include <asm/arch/nevis.h>
#include <asm/arch/fp_uart.h>
#include <asm/arch/sys_proto.h>

//DECLARE_GLOBAL_DATA_PTR;

/* driver for an Samsun HCR-13SS22 serial VFD-display connected via an
   Coolstream VFD-Controller.
   We do not use the VFD-stuff provided by u-boot, because this VFD has only
   12 digits with free programmable 5x7 matrix and some fixed display elements.
   So it makes no sense to map it into a framebuffer. */

#ifdef HAVE_COOLSTREAM_VFD_CONTROLLER

#ifndef DISPLAY_PIO_CLOCK
#error DISPLAY_PIO_CLOCK not defined
#endif

#ifndef DISPLAY_PIO_DATA
#error DISPLAY_PIO_DATA not defined
#endif

#ifndef DISPLAY_PIO_RESET
#error DISPLAY_PIO_STROBE not defined
#endif

#ifndef DISPLAY_PIO_STATUS
#error DISPLAY_PIO_BLANK not defined
#endif

/*******************************************************************************/

#define STX			0x02
#define ETX			0x03
#define ACK			0x06
#define BEL			0x07
#define NAK			0x15

#define VFDCTRL_CMD_WRITE_CHAR	0x00	/* wirte the ASCCI charater to the given segment address (len = 1) */
#define VFDCTRL_CMD_WRITE_RAW	0x10	/* write pixel data to the given segment address (len = 5) */
#define VFDCTRL_CMD_CLEAR	0x20	/* clear the entire display (len = 0) */
#define VFDCTRL_CMD_SET_BRIGHT	0x30	/* set the displays brightness (len = 0, address field = brightness 0 to 15) */
#define VFDCTRL_CMD_SET_ICON    0x40
#define VFDCTRL_CMD_CLR_ICON    0x50
#define VFDCTRL_CMD_SET_PIO     0x60
#define VFDCTRL_CMD_CLR_PIO     0x70
#define VFDCTRL_CMD_VERSION	0xF0	/* let the controller print it's own version onto the display */
#define VFDCTRL_CMD_WRITE_STR	0xF1	/* let the controller print a simple text */
#define VFDCTRL_CMD_LED_CTRL	0xF2	/* let the controller set a led */

#define VFD_DEFAULT_BAUDRATE   115200


typedef enum
{
    VFD_ICON_BAR8       = 0x00000004,
    VFD_ICON_BAR7       = 0x00000008,
    VFD_ICON_BAR6       = 0x00000010,
    VFD_ICON_BAR5       = 0x00000020,
    VFD_ICON_BAR4       = 0x00000040,
    VFD_ICON_BAR3       = 0x00000080,
    VFD_ICON_BAR2       = 0x00000100,
    VFD_ICON_BAR1       = 0x00000200,
    VFD_ICON_FRAME      = 0x00000400,
    VFD_ICON_HDD        = 0x00000800,
    VFD_ICON_MUTE       = 0x00001000,
    VFD_ICON_DOLBY      = 0x00002000,
    VFD_ICON_POWER      = 0x00004000,
    VFD_ICON_TIMESHIFT  = 0x00008000,
    VFD_ICON_SIGNAL     = 0x00010000,
    VFD_ICON_TV         = 0x00020000,
    VFD_ICON_RADIO      = 0x00040000,
    VFD_ICON_HD         = 0x01000001,
    VFD_ICON_1080P      = 0x02000001,
    VFD_ICON_1080I      = 0x03000001,
    VFD_ICON_720P       = 0x04000001,
    VFD_ICON_480P       = 0x05000001,
    VFD_ICON_480I       = 0x06000001,
    VFD_ICON_USB        = 0x07000001,
    VFD_ICON_MP3        = 0x08000001,
    VFD_ICON_PLAY       = 0x09000001,
    VFD_ICON_COL1       = 0x09000002,
    VFD_ICON_PAUSE      = 0x0A000001,
    VFD_ICON_CAM1       = 0x0B000001,
    VFD_ICON_COL2       = 0x0B000002,
    VFD_ICON_CAM2       = 0x0C000001
} vfd_icon;

#if 0
typedef enum {
	VFD_LED_1_ON		= 0x81,
	VFD_LED_2_ON		= 0x82,
	VFD_LED_3_ON		= 0x83,
	VFD_LED_1_OFF		= 0x01,
	VFD_LED_2_OFF		= 0x02,
	VFD_LED_3_OFF		= 0x03,
} vfd_led_ctrl_t;
#endif
static u32 have_controller;
static vfd_icon last_val = 0xFF;

typedef struct {
	int  (*init           )(void);
	int  (*detect         )(void);
	int  (*led_control    )(vfd_led_ctrl_t ctrl);
	void (*tx             )(u8 cmd, u8 addr, u8 len, char *data);
	void (*set_text       )(char *text, int len);
} vfd_ops;

static const vfd_ops *ops;

/*******************************************************************************/

static int fp_uart_rx(u8 *data, int len)
{
	int i = 0, j = len;
	int timeout = 8000; /* 800 ms */

	while (j--) {
		while ((fp_uart_get_rx_count() == 0) && timeout) {
			if ((fp_uart_get_status() & 1) && !fp_uart_get_rx_count())
				fp_uart_get_rx();
			udelay(100);
			timeout--;
		}
		if (!timeout) {
			printf("*** Warning: RX timed-out\n");
			break;
		}

		data[i++] = fp_uart_get_rx();
	}

	return len - j;
}

/*******************************************************************************/

static int fp_uart_tx(u8 *data, int len)
{
	int i = 0, j = len;
	int timeout = 8000; /* 800 ms */

	while (j--) {
		while ((fp_uart_get_tx_count() != 0) && timeout) {
			udelay(100);
			timeout--;
		}
		if(!timeout) {
			printf("*** Warning: TX timed-out\n");
			break;
		}

		fp_uart_set_tx(data[i]);
	}

	return len - j;
}

/*******************************************************************************/

static int _old_vfd_init(void)
{
	board_gpio_drive( 69, PIO_OFF);	/* hack */
	board_gpio_drive(188, PIO_OFF);	/* hack */

	board_gpio_drive(DISPLAY_PIO_RESET, PIO_HIGH);	/* assert reset signal */
	board_gpio_drive(DISPLAY_PIO_STATUS, PIO_OFF);	/* configure as input */
	board_gpio_drive(DISPLAY_PIO_CLOCK, PIO_HIGH);	/* set clock to high level for proper initial value */
	udelay(2000);					/* wait a little bit */
	board_gpio_drive(DISPLAY_PIO_RESET, PIO_LOW);	/* release reset signal */

	return 0;
}

/*******************************************************************************/

static int _old_vfd_detect(void)
{
	u32 have_ctrl = 0;
	int cnt = 0;

	for (cnt = 0; cnt < 16; cnt++) {
		udelay(100);
		/* read the status line, it goes low, if the controller is ready */
		if (board_gpio_read(DISPLAY_PIO_STATUS) == 0) {
			/* with a 18.432 MHz Xtal this takes around 5 loops */
			cnt = 16;
			have_ctrl = 1;
		}
	}

	return have_ctrl == 1;
}

/*******************************************************************************/

static void _old_vfd_set_text(char *text, int len)
{
	int cnt;

	if (!ops)
		return;

	for (cnt = 0; cnt < 12; cnt++) {
		if (cnt < len)
			ops->tx(VFDCTRL_CMD_WRITE_CHAR, cnt + 1, 1, text + cnt);
		else
			ops->tx(VFDCTRL_CMD_WRITE_CHAR, cnt + 1, 1, " ");
	}
}

/*******************************************************************************/

static void _old_vfd_tx(u8 cmd, u8 addr, u8 len, char *data)
{
	char outbuffer[6];
	u8 cnt = 0;
	u8 err = 1;
	s8 byte, bit;

	if (((len > 0) & (!data)) || (!have_controller))
		return;

	board_gpio_drive(DISPLAY_PIO_CLOCK, PIO_LOW);	/* set clock to low level to indicate that we want xfer new data */

	outbuffer[0] = cmd | addr;
	if (len > 5)
		len = 5;

	for (byte = 0; byte < len; byte++)
		outbuffer[byte + 1] = data[byte];

	while (cnt < 16) {
		cnt++;
		udelay(1);
		if (board_gpio_read(DISPLAY_PIO_STATUS) == 1) {
			/* OK, the controller is ready to receive data */
			err = 0;
			cnt = 16;
		}
	}

	if (err) {
		printf("*** Warning: VFD-Controller did not respond.\n");
		return;
	}

	/* send the data byte by byte, MSB first */
	for (byte = 0; byte < 6; byte++) {
		for (bit = 7; bit >= 0; bit--) {
			board_gpio_drive(DISPLAY_PIO_CLOCK, PIO_LOW);
			udelay(2);
			board_gpio_drive(DISPLAY_PIO_DATA, (outbuffer[byte] & (1 << bit)) ? PIO_HIGH : PIO_LOW);
			udelay(2);
			board_gpio_drive(DISPLAY_PIO_CLOCK, PIO_HIGH);
			udelay(10);	/* min. 9 clock cycles (0.5 us per clock cycle at 12 MHz) */
		}
	}
	/* wait, until the controller set it's status line back to low level */
	cnt = 0;
	err = 1;
	while (cnt < 255) {
		cnt++;
		udelay(10);
		if (board_gpio_read(DISPLAY_PIO_STATUS) == 0) {
			/* OK, the controller has finished receiving our data */
			cnt = 255;
			err = 0;
		}
	}

	if (err)
		printf("*** Warning: VFD-Controller did not accepted our data.\n");

}

/*******************************************************************************/

static const vfd_ops old_vfd_ops = {
	.init		= _old_vfd_init,
	.detect		= _old_vfd_detect,
	.tx		= _old_vfd_tx,
	.set_text	= _old_vfd_set_text,
};

/*******************************************************************************/

static int _new_vfd_init(void)
{
	volatile u8 *brdl = (volatile u8 *) UART_FIFO_BRDL_REG(FP_DEFAULT_UART);
	volatile u8 *brdu = (volatile u8 *) UART_IRQE_BRDU_REG(FP_DEFAULT_UART);
	volatile u8 *exp = (volatile u8 *) UART_EXP_REG(FP_DEFAULT_UART);
	volatile u8 *frmc = (volatile u8 *) UART_FRMC_REG(FP_DEFAULT_UART);
	volatile u8 *fifc = (volatile u8 *) UART_FIFC_REG(FP_DEFAULT_UART);
	u32 baudrate;

	baudrate	 = (54000000 / (16 * VFD_DEFAULT_BAUDRATE)) - 1;
	*brdu		 = 0x00;		/* TIDE, TSRE, RSRE */
	*frmc		|= 0x81;		/* BDS = 1 *//* 25000bps, 8, N, 1 */
	*brdl		 =  baudrate       & 0xFF;
	*brdu		 = (baudrate >> 8) & 0xFF;
	*exp		 = 0x00;
	*frmc		&= ~0x80;		/* BDS = 0 */
	*fifc		 = 0x07 | (3 << 4);	/* FIFO:128b, TXINT: empty */
	*brdu		 = 0x00;		/* TIDE, TSRE, RSRE */

	board_gpio_drive(DISPLAY_PIO_RESET, PIO_HIGH);	/* assert reset signal */
	udelay(2000);		/* wait a little bit */
	board_gpio_drive(DISPLAY_PIO_RESET, PIO_LOW);	/* release reset signal */

	return 0;
}

/*******************************************************************************/

static int _new_vfd_detect(void)
{
	u8 data;

	fp_uart_rx(&data, 1);
	if (data != BEL)
		return 0;

	/* Enable button irq here! */
	fp_uart_start_rx();

	return 1;
}

/*******************************************************************************/

static void _new_vfd_tx(u8 cmd, u8 addr, u8 len, char *data)
{
	unsigned char cmd_buf[256 + 4];
	u8 cnt = 0, i;

	if (((len > 0) && (!data)))
		return;

	cmd_buf[cnt++] = STX;
	switch (cmd) {
	case VFDCTRL_CMD_WRITE_STR:
		cmd_buf[cnt++] = 'A';
		memcpy(cmd_buf + cnt, data, len);
		cnt += len;
		break;
	case VFDCTRL_CMD_SET_BRIGHT:
		cmd_buf[cnt++] = 'B';
		cmd_buf[cnt++] = addr;
		break;
	case VFDCTRL_CMD_CLEAR:
		cmd_buf[cnt++] = 'A';
		break;
	case VFDCTRL_CMD_WRITE_RAW:
		cmd_buf[cnt++] = 'P';
		cmd_buf[cnt++] = addr;			/* Segment */
		memcpy(cmd_buf + cnt, data, len);	/* Pixel data */
		cnt += len;
		break;
	case VFDCTRL_CMD_LED_CTRL:
		cmd_buf[cnt++] = 'L';
		cmd_buf[cnt++] = addr;			/* LED */
		break;
	case VFDCTRL_CMD_SET_ICON:
	case VFDCTRL_CMD_CLR_ICON:
	{
		cmd_buf[cnt++] = 'P';
		cmd_buf[cnt++]  = addr | 0x80 | ((cmd == VFDCTRL_CMD_CLR_ICON) ? 0x40 : 0x00);
		memset(&cmd_buf[cnt], 0, 2);
		cnt += 2;
		memcpy(&cmd_buf[cnt], data, 3);	/* Pixel data (max 3 bytes) */
		cnt += 3;
		break;
	}
	case VFDCTRL_CMD_SET_PIO:
	case VFDCTRL_CMD_CLR_PIO:
	case VFDCTRL_CMD_VERSION:
	case VFDCTRL_CMD_WRITE_CHAR:
	default:
		printf("*** Warning: FrontPanel: unknown command\n");
		return;
	}

	cmd_buf[cnt++] = ETX;

	fp_uart_stop_rx();

	for (i = 0; i < cnt; i++) {
		u8 d;
		fp_uart_tx(&cmd_buf[i], 1);
		fp_uart_rx(&d, 1);
		if (d != ACK) {
			printf("*** Warning: FrontPanel: no ACK\n");
			//break;
		}
	}

	fp_uart_start_rx();
}

/*******************************************************************************/

static int _new_vfd_led_control(vfd_led_ctrl_t ctrl)
{
	if (!ops || !ops->tx)
		return 0;

	ops->tx(VFDCTRL_CMD_LED_CTRL, ctrl, 0, NULL);

	return 0;
}

/*******************************************************************************/

static void _new_vfd_set_text(char *text, int len)
{
	if(!ops || !ops->tx)
		return;

	ops->tx(VFDCTRL_CMD_WRITE_STR, 0, len, text);
}

/*******************************************************************************/

static const vfd_ops new_vfd_ops = {
	.init		= _new_vfd_init,
	.detect		= _new_vfd_detect,
	.tx		= _new_vfd_tx,
	.led_control	= _new_vfd_led_control,
	.set_text	= _new_vfd_set_text,
};

/*******************************************************************************/

void board_display_init(void)
{
	u32 system_rev;
	extern u32 get_board_rev(void);

	system_rev = get_board_rev();

	//system_rev = 8;// HACK
	if (system_rev >= 8)
		ops = &new_vfd_ops;
	else
		ops = &old_vfd_ops;

	if(ops->init)
		ops->init();

	if (ops->detect) {
		if (!(have_controller = ops->detect()))
			printf("*** Warning: no VFD-Controller present\n");
	}
}

/*******************************************************************************/

void board_display_clear(void)
{
	if (!have_controller)
		return;

	if (!ops || ops->tx)
		return;

	ops->tx(VFDCTRL_CMD_CLEAR, 0, 0, NULL);
}

/*******************************************************************************/

void display_set_text(char *text)
{
	u32 tlen;

	if (!have_controller)
		return;

	if (!ops || !ops->set_text)
		return;

	if (!text)
		text = "U-Boot";

	tlen = strlen(text);
	ops->set_text(text, tlen);
}

/*******************************************************************************/

static const vfd_icon ict[9] = {0, VFD_ICON_BAR1, VFD_ICON_BAR2, VFD_ICON_BAR3, VFD_ICON_BAR4, VFD_ICON_BAR5, VFD_ICON_BAR6, VFD_ICON_BAR7, VFD_ICON_BAR8};

/*******************************************************************************/

void display_show_progress(u32 first, u32 last, u32 current)
{
	char ic_data[3];
	u32 tmp, val = 0;
	vfd_icon vin = 0;

	if (!have_controller)
		return;

	if(!ops || !ops->tx)
		return;

	if ((first == 0) && (last == 0) && (current == 0)) {
		/* switch off */
		vin |= VFD_ICON_BAR8 | VFD_ICON_BAR7 | VFD_ICON_BAR6 | VFD_ICON_BAR5 | VFD_ICON_BAR4 | VFD_ICON_BAR3 | VFD_ICON_BAR2 | VFD_ICON_BAR1 | VFD_ICON_FRAME;
		ic_data[0] = (vin >> 16) & 0xFF;
		ic_data[1] = (vin >>  8) & 0xFF;
		ic_data[2] =  vin	 & 0xFF;
		ops->tx(VFDCTRL_CMD_CLR_ICON, 0, sizeof(ic_data), ic_data);
	} else {
		if (first > last) {
			/* revert */
			tmp = first;
			first = last;
			last = tmp;
		}

		if (current > last)
			current = last;

		/* adjust range so that it start at 0 */
		last -= first;
		current -= first;

		/* break down the values to get a range of 0 to 8 */
		if (last == 0)
			val = 8;
		else {
			val = (current * 8) / last;
			if (((current * 8) % last) > (last / 2))	/* round up */
				val++;

			if (val > 8)	/* should never happen */
				val = 8;
		}

		if (val != last_val) {
			for (tmp = val + 1; tmp < 9; tmp++)
				vin |= ict[tmp];

			ic_data[0] = (vin >> 16) & 0xFF;
			ic_data[1] = (vin >>  8) & 0xFF;
			ic_data[2] =  vin	 & 0xFF;
			ops->tx(VFDCTRL_CMD_CLR_ICON, 0, sizeof(ic_data), ic_data);

			vin = VFD_ICON_FRAME;
			for (tmp = 0; tmp <= val; tmp++)
				vin |= ict[tmp];

			ic_data[0] = (vin >> 16) & 0xFF;
			ic_data[1] = (vin >>  8) & 0xFF;
			ic_data[2] =  vin	 & 0xFF;
			ops->tx(VFDCTRL_CMD_SET_ICON, 0, sizeof(ic_data), ic_data);
			last_val = val;
		}
	}
}

/*******************************************************************************/

void display_show_icon(u32 icon, u32 clear)
{
	char ic_data[3];
	//u8 cmd;

	if (!have_controller)
		return;

	if(!ops || !ops->tx)
		return;

	ic_data[0] = (icon >> 16) & 0xFF;
	ic_data[1] = (icon >>  8) & 0xFF;
	ic_data[2] =  icon	  & 0xFF;
	//cmd	   = clear ? VFDCTRL_CMD_CLR_ICON : VFDCTRL_CMD_SET_ICON;

	ops->tx(clear ? VFDCTRL_CMD_CLR_ICON : VFDCTRL_CMD_SET_ICON, (icon >> 24) & 0xFF, sizeof(ic_data), ic_data);
}

void display_led_control(vfd_led_ctrl_t ctrl)
{
	if (!have_controller)
		return;

	if(!ops || !ops->led_control)
		return;

	ops->led_control(ctrl);
}

#endif /* HAVE_COOLSTREAM_VFD_CONTROLLER */
