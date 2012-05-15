/*
 * (C) Copyright 2008
 * Coolstream International Limited
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR /PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
  */
#ifndef _NEVIS_SYS_PROTO_H_
#define _NEVIS_SYS_PROTO_H_

#define PIO_HIGH	PIO_DRIVE_HIGH_REG	/* drive PIO HIGH */
#define PIO_LOW		PIO_DRIVE_LOW_REG	/* drive PIO LOW */
#define PIO_OFF		PIO_DRIVE_OFF_REG	/* switch PIO into input mode */

#define	SYSFLAG_FULLERASE	0x80000000	/* enabled the flash routines to erase/write the boot sector itself */

/* board/<board>/interrupt.c */
#ifdef CONFIG_USE_IRQ
void board_init_itc(void);
int board_do_interrupt(struct pt_regs *pt_regs);
void board_disable_all_interrupts(void);
#endif

/* board/<board>/gpio.c */
void board_gpio_drive(u32 pio, u32 state);
u32 board_gpio_read(u32 pio);

/* board/<board>/network.c */
void board_eth_init(void);		/* our low level init, only called once */
int eth_init(bd_t * bis);		/* called from u-boot's network subsys multiple times */
int eth_handle_interrupt(void);

/* board/<board>/display.c */
typedef enum {
	VFD_LED_1_ON		= 0x81,
	VFD_LED_2_ON		= 0x82,
	VFD_LED_3_ON		= 0x83,
	VFD_LED_1_OFF		= 0x01,
	VFD_LED_2_OFF		= 0x02,
	VFD_LED_3_OFF		= 0x03,
} vfd_led_ctrl_t;

void board_display_init(void);
void display_set_text(char *text);
void display_show_progress(u32 first, u32 last, u32 current);
void board_display_clear(void);
void display_show_icon(u32 icon, u32 clear);
void display_led_control(vfd_led_ctrl_t ctrl);

/* board/<board>/flash.c */
void flash_set_flag(void);
#ifdef CONFIG_SYS_FLASH_OTP
int flash_dump_otp(flash_info_t *info);
int flash_write_otp(flash_info_t *info, uchar *data, ulong cnt);
#endif /* CONFIG_SYS_FLASH_OTP */

/* board/<board>/splash.c */
u32 board_get_width(void);
u32 board_get_height(void);

/* cpu/arm1176/nevis/misc.c */
void LockRegs(u32 mask);
void UnlockRegs(u32 mask);
void CleanupBeforeLinux(void);

#endif
