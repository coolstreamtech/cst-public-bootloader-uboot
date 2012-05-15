/*
 * Configuration for the HD1 board (Satellite HDTV capable Set-Top-Box)
 *
 * (C) Copyright 2008
 * Coolstream International Limited
 *
 * Configuration settings for Coolstream's HD1 STB board.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/arch/nevis.h>				/* get chip and board defs */

#define LITTLEENDIAN		1			/* we have an LE architecture */

/* High Level Configuration Options */
#define CONFIG_ARM1176		1			/* We have an ARM1176 CPU core */
#define CONFIG_NEVIS		1			/* in an CX2450x (Nevis) SoC */

#define PRODUCT_NAME		"Coolstream HDx"	/* up to 24 characters */
#define VENDOR_VERSION		"0.9"			/* vendor specific version string, apended to u-boots version string */

#define CONFIG_USE_IRQ                 			/* no support for IRQs at this time */
#define CONFIG_MISC_INIT_P				/* we have some additional inits to do */

#define CONFIG_CMDLINE_TAG		0		/* enable passing kernel commandline tags */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_DECARM_TAG		1
#define CONFIG_MAC_TAG			1
#define CONFIG_SERIAL_TAG		1
#define CONFIG_REVISION_TAG		1

#define CONFIG_FIXED_MEM_START		0x00000000
#define CONFIG_FIXED_MEM_SIZE		0x18000000

#define CONFIG_ARM9_RESET_ADDR			0x10000
#define CONFIG_ARM9_SETUP_LEGACY_MODE		0
#define CONFIG_ARM9_OFFSET			0x1000
#define CONFIG_ARM9_OFFSET_LEGACY		0x0040
#define CONFIG_ARM9_SHARED_CODE_OFFSET		(CONFIG_ARM9_OFFSET + 0x0000)
#define CONFIG_ARM9_SHARED_RAM_OFFSET		(CONFIG_ARM9_OFFSET + 0x0004)
#define CONFIG_ARM9_SHARED_CODE_OFFSET_LEGACY	(CONFIG_ARM9_OFFSET_LEGACY + 0x0000)
#define CONFIG_ARM9_SHARED_RAM_OFFSET_LEGACY	(CONFIG_ARM9_OFFSET_LEAGCY + 0x0004)
#define CONFIG_LOAD_INITIAL_DECARM_CODE	1		/* the ARM9 co-processor must be initialized with default code */

/* Size of malloc() pool */
#define CONFIG_ENV_SIZE			SZ_128K		/* Total Size of Environment Sector */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + SZ_4M)
#define CONFIG_SYS_GBL_DATA_SIZE	128		/* size in bytes reserved for initial data */

/* Hardware drivers */

/* serial VFD module */
#define HAVE_COOLSTREAM_VFD_CONTROLLER	1

#ifdef HAVE_COOLSTREAM_VFD_CONTROLLER
#define DISPLAY_PIO_CLOCK		8		/* PIO for the clock */
#define DISPLAY_PIO_RESET		9		/* PIO to reset the controller (1 = reset, 0 = normal operation) */
#define DISPLAY_PIO_DATA		68		/* PIO for the serial data */
#define DISPLAY_PIO_STATUS		189		/* PIO to read the status from the controller */
#endif /* HAVE_COOLSTREAM_VFD_CONTROLLER */

/* select serial console configuration */
#define CONFIG_SERIAL3					/* we use the 3. serial port (UART) for console communication */
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}
#define CONFIG_SYS_CONSOLE_INFO_QUIET

/* Command line commands configuration. */
#define CONFIG_CMD_AUTOSCRIPT				/* Autoscript Support                                          	*/
#define CONFIG_CMD_BDI					/* bdinfo                                                      	*/
#define CONFIG_CMD_BOOTD				/* bootd                                                       	*/
#define CONFIG_CMD_CONSOLE				/* coninfo                                                     	*/
#define CONFIG_CMD_DIAG					/* Diagnostics                                                 	*/
#define CONFIG_CMD_ENV					/* saveenv                                                     	*/
#define CONFIG_CMD_FLASH				/* flinfo, erase, protect                                      	*/
#define CONFIG_CMD_IMI					/* iminfo                                                     	*/
#define CONFIG_CMD_IMLS					/* List all found images                                      	*/
#define CONFIG_CMD_ITEST				/* Integer (and string) test (required for CMD_ENV, CMD_FLASH)	*/
#define CONFIG_CMD_MEMORY				/* md mm nm mw cp cmp crc base loop mtest                     	*/
#define CONFIG_CMD_NET					/* bootp, tftpboot, rarpboot                                  	*/
#define CONFIG_CMD_PING					/* ping support                                               	*/
#define CONFIG_CMD_USB					/* we wanna have USB-support					*/
#define CONFIG_CMD_SAVEENV				/* saveenv							*/
#define CONFIG_CMD_EXT2					/* EXT2 support							*/
#define CONFIG_CMD_FAT					/* FAT/VFAT support						*/
#define CONFIG_CMD_JFFS2				/* JFFS2 support						*/

/* default Network configuration */
#define CONFIG_NETMASK			255.255.255.0
#define CONFIG_IPADDR			192.168.1.100
#define CONFIG_SERVERIP			192.168.1.16
#define CONFIG_BOOTFILE			"uImage"

/* default boot configiguration */
#define CONFIG_BOOTDELAY        	1
#define CONFIG_BOOTARGS			"console=ttyRI0 mtdparts=cx2450xflash:384k(U-Boot),128k(Splash),4096k(kernel),28160k(systemFS) root=mtd3 rootfstype=jffs2 rw mem=384M"
#define CONFIG_BOOTCOMMAND		"bootm 0xF0080000"	/* load and execute the Kernel from Flash at address 0xF0080000 */
//#define CONFIG_BOOTCOMMAND		"bootp; bootm"		/* load from network (bootp = dhcp + tftp) */

#define CONFIG_NETMASK          255.255.255.0
#define CONFIG_IPADDR           192.168.1.100
#define CONFIG_SERVERIP         192.168.1.16
#define CONFIG_BOOTFILE         "uImage"

/* Miscellaneous configurable options */
#define CONFIG_SYS_PROMPT 		"HDx> "
#define CONFIG_SYS_CBSIZE		256				/* Console I/O Buffer Size */

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16				/* max number of command args */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE		/* Boot Argument Buffer Size */

#define CONFIG_SYS_MEMTEST_START	0x02000000			/* memtest works on */
#define CONFIG_SYS_MEMTEST_END		0x02F00000			/* dummy to provide a area for the test. We have more RAM than tested */

#define CONFIG_SYS_LOAD_ADDR		0x0E000000			/* the DEFAULT load address */
#define CONFIG_SYS_MAXARGS		16				/* maximum number of arguments for the 'go' command */

#define CONFIG_SYS_HZ			1000000				/* system timer is clocked with 54 MHz and divides the clock by 54 which results in 1 MHz or in other words 1us per clock */
#define CFG_HZ				CONFIG_SYS_HZ

/* Stack sizesn (are set up in start.S using the settings below) */
#define CONFIG_STACKSIZE		SZ_1M				/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ		SZ_16K				/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ		SZ_16K				/* FIQ stack */
#endif

/* Physical Memory Map */
#define CONFIG_NR_DRAM_BANKS    	2				/* Nevis can handle 2 DDR2-SDRAM banks */
#define PHYS_FLASH			0xF0000000			/* Flash Bank #1 fixed start address */

/* FLASH and environment organization */
#define	CONFIG_ENV_IS_IN_FLASH		1				/* we can store a user config into the flash */

#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH
#define CONFIG_SYS_FLASH_OTP		1				/* we use flash with an additional small OTP sector */
#define CONFIG_SYS_MAX_FLASH_SECT	(2048)				/* define more than avail to have secure auto detect */
#define CONFIG_SYS_MAX_FLASH_BANKS	2				/* we limit to 2 banks at CS0 and CS1. CS2 and above are for I/O */
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_FLASH_BASE		/* u-boot is located at the bottom in the flash */
#define CONFIG_ENV_ADDR			(CONFIG_SYS_FLASH_BASE + SZ_256K)

#define CFG_EXT_LEGACY_FLASH		1				/* assume we have a little bit more modern flashes than an 29400 ;-) */
#define CONFIG_SYS_FLASH_PROTECTION	1				/* Use hardware sector protection */

/* USB support */
#define CONFIG_USB_EHCI			1				/* we have an EHCI USB-Host Controller */
#define CONFIG_USB_EHCI_CX2450X		1				/* tell the EHCI core to use our CX2450X host controller */
#define CONFIG_USB_STORAGE		1				/* we wanna have storage support */
#define CONFIG_EHCI_IS_TDI		1
#define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS 2

/* Filesystems */
#define CONFIG_DOS_PARTITION		1

/* misc stuff */
#define BEAUTIFY_CONSOLE		1				/* make the console output nicer by using some ANSI graphics */
#define CONFIG_DISPLAY_CPUINFO		1

/* New stuff */
#ifdef CONFIG_CMD_JFFS2
#undef CONFIG_CMD_MTDPARTS
#define CONFIG_SYS_FLASH0_BASE		CONFIG_SYS_FLASH_BASE
#define CONFIG_JFFS2_DEV		"nor0"
#define CONFIG_JFFS2_PART_SIZE		(0x01B80000)
#define CONFIG_JFFS2_PART_OFFSET	(0x00480000)
#define CONFIG_SYS_JFFS2_NUM_BANKS	CONFIG_SYS_MAX_FLASH_BANKS
#define CONFIG_SYS_JFFS2_SORT_FRAGMENTS
#endif

#define CONFIG_VIDEO_LOGO
#define CONFIG_SPLASH_SCREEN		
#define CONFIG_CFB_CONSOLE		
#define VIDEO_KBD_INIT_FCT		-1
#define VIDEO_TSTC_FCT		serial_tstc
#define VIDEO_GETC_FCT		serial_getc
#define CONFIG_VIDEO_BMP_GZIP		
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(1 << 21)

#define CONFIG_EXTRA_ENV_SETTINGS			\
		"splashimage=f0060000\0"		\
		""

#endif /* __CONFIG_H */
