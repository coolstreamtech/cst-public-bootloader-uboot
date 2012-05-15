/*
 * (C) Copyright 2008, Coolstream International Limited
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <pci.h>
#include <usb.h>
#include <asm/arch/nevis.h>

#include "usb_ehci.h"
#include "usb_ehci_core.h"

#define BASE_ADDR	0xE8000000

#define USB_DEBUG 1

/*******************************************************************************/

int ehci_hcd_init(void)
{
    volatile u32 *usb_enable = (volatile u32*) SREG_USB_ENABLE_REG;

    #ifdef USB_DEBUG
    printf("%s();\n", __FUNCTION__);
    #endif

    *usb_enable |= 0x00000703;		/* bit 8-6: bus pwr polarity: 0 = active high, bit 5-3: fault polarity: 0 = active low, bit 1-0: power on/off: 0 = off, 1 = on */
    udelay(250000);


    /* setup controllers register addresses */
    hccr = (struct ehci_hccr *)((uint32_t) (BASE_ADDR + 0x0100));
    hcor = (struct ehci_hcor *)((uint32_t) hccr + HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

    printf("CX2450x init hccr %x and hcor %x hc_length %d\n",
           (uint32_t)hccr, (uint32_t)hcor,
           (uint32_t)HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

    return 0;
}

/*******************************************************************************/

int ehci_hcd_stop(void)
{
    volatile u32 *usb_enable = (volatile u32*) SREG_USB_ENABLE_REG;

    #ifdef USB_DEBUG
    printf("%s();\n", __FUNCTION__);
    #endif

    *usb_enable &= ~3;

    return 0;
}
