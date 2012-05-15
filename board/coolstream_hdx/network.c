/*
 * Low level driver to access the network via the embedded VMAC 
 * controller and an attached Realtek RTL8201 PHY.
 *
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
#include <malloc.h>
#include <net.h>

#define NUM_RX_BDT	32
#define NUM_TX_BDT	1


/* TX Control Information Written By the EMAC Bit Masks */
#define ENET_CTL_INFO_TX_LEN		0x000007FF	/* Tx Length in this buffer to be Xmitted */
#define ENET_CTL_INFO_FIRST		0x00010000	/* First TX buffer in the packet */
#define ENET_CTL_INFO_LAST		0x00020000	/* Last TX buffer in the packet */
#define ENET_CTL_INFO_ADDCRC		0x00040000	/* Add the CRC to the pkt that is transmitted */
#define ENET_CTL_INFO_CARR_LOSS		0x00200000	/* Carrier Lost during xmission */
#define ENET_CTL_INFO_DEFER		0x00400000	/* xmission deferred due to traffic */
#define ENET_CTL_INFO_DROPPED		0x00800000	/* pkt dropped after 16 retries */
#define ENET_CTL_INFO_RETRY		0x0F000000	/* Retry count for TX */
#define ENET_CTL_INFO_RETRY_SHIFT	24  
#define ENET_CTL_INFO_LATE_COLL		0x10000000	/* Late Collision */
#define ENET_CTL_INFO_UFLO		0x20000000	/* Data not available on time */
#define ENET_CTL_INFO_BUFF		0x40000000	/* Buffer error - bad FIRST and LAST */
#define ENET_CTL_INFO_OWN		0x80000000	/* CPU/EMAC Ownership of buffer */

#define MDIO_WRITE		0x50020000	/* 0101 xxxx xxxx xx10 xxxx xxxx xxxx xxxx */
#define MDIO_READ		0x60020000	/* 0110 xxxx xxxx xx10 xxxx xxxx xxxx xxxx */

#define RTL8201_BMC_REG		0x00000000	/* Basic Mode Control Register   */
#define RTL8201_BMS_REG		0x00040000	/* Basic Mode Status Register    */
#define RTL8201_PHYID1_REG	0x00080000	/* PHY Identification Register 1 */
#define RTL8201_PHYID2_REG	0x000C0000	/* PHY Identification Register 2 */


typedef struct _xbdt
{
    volatile u32	 info;
    volatile u8		*data;
} __attribute__((packed)) xbdt;

typedef struct _rbdt
{
    xbdt bdt[NUM_RX_BDT];
} __attribute__((packed)) rbdt;

typedef struct _tbdt
{
    xbdt bdt[NUM_TX_BDT];
} __attribute__((packed)) tbdt;

static int have_vmac = 0;
static int phy_addr  = 0x20;
static volatile rbdt *dma_rx_buf = NULL;
static volatile tbdt *dma_tx_buf = NULL;
static volatile u8 *tx_buf[NUM_TX_BDT];
static volatile u8 *rx_buf[NUM_RX_BDT];
static u32 current_tx_packet;
static u32 current_rx_packet;
static u8 *rx_chain;
static u32 rx_chain_pos;

extern flash_info_t flash_info[];       /* info for FLASH chips */

DECLARE_GLOBAL_DATA_PTR;

static u16 _mdio_xfer(u32 dir, u32 addr, u32 reg, u16 data);
void debug_dump_hex (unsigned int len, unsigned char *data);

/*******************************************************************************/
/* Initialize the NIC - low-level part, only executed once                     */

void board_eth_init(void)
{
    char *nhwf = "*** Warning - no supported hardware found ***";
    volatile u32 *reg;
    volatile u32 *idreg   = (volatile u32*) EMAC_ID_REG;
    u32 id = *idreg;
    u8 ID  = id & 0xFF;
    u8 nID = (id >> 8) & 0xFF;
    u32 phy_cnt;

    have_vmac = 0;

    #ifdef BEAUTIFY_CONSOLE
    printf("\xBA %-76s \xBA\n", "Network configuration");
    #endif

    /* ID integrity check, ID-Register bit 15...8 are the complements to bit 7...0 */
    if ((ID ^ nID) != 0xFF)
    {
	#ifdef BEAUTIFY_CONSOLE
	printf("\xBA MAC: %-64s (%.2X/%.2X) \xBA\n", nhwf, ID, nID);
	#else
	printf("NET: Error device ID mismatch (%.2X/%.2X)\n", ID, nID);
	#endif
	return /*have_vmac*/;
    }

    /* check configured MAC-address and set default, if not configured */
    if (memcmp(gd->bd->bi_enetaddr, "\0\0\0\0\0\0", 6) == 0)
    {
        gd->bd->bi_enetaddr[0] = flash_info[0].serial[1];
        gd->bd->bi_enetaddr[1] = flash_info[0].serial[2];
        gd->bd->bi_enetaddr[2] = flash_info[0].serial[3];
        gd->bd->bi_enetaddr[3] = flash_info[0].serial[5];
        gd->bd->bi_enetaddr[4] = flash_info[0].serial[6];
        gd->bd->bi_enetaddr[5] = flash_info[0].serial[7];
    }

    #ifdef BEAUTIFY_CONSOLE
    printf("\xBA MAC: Conexant VMAC rev. %.2X       address: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X%-18s\xBA\n", 
	    (id >> 16) & 0xFF, 
	    gd->bd->bi_enetaddr[0], gd->bd->bi_enetaddr[1], gd->bd->bi_enetaddr[2], 
	    gd->bd->bi_enetaddr[3], gd->bd->bi_enetaddr[4], gd->bd->bi_enetaddr[5],
	    " ");
    #else
    printf("NET: Conexant VMAC rev. 0x%.2X detected\n", (id >> 16) & 0xFF);
    #endif

    reg   = (volatile u32*) EMAC_CTRL_REG;
    *reg &= 0xFFFFFFE6;		/* clear bit 0 to stop all network traffic */

    /* load the MAC-address into the device */
    reg  = (volatile u32*) EMAC_ADDRL_REG;
    *reg = (gd->bd->bi_enetaddr[3] << 24) | (gd->bd->bi_enetaddr[2] << 16) | (gd->bd->bi_enetaddr[1] << 8) | gd->bd->bi_enetaddr[0];

    reg  = (volatile u32*) EMAC_ADDRH_REG;
    *reg = (gd->bd->bi_enetaddr[5] << 8) | gd->bd->bi_enetaddr[4];

    /* clear filter */
    reg  = (volatile u32*) EMAC_LAFL_REG;
    *reg = 0;
    reg  = (volatile u32*) EMAC_LAFH_REG;
    *reg = 0;

    /* check the PHY - reset is not required here */
    for (phy_cnt = 0; phy_cnt < 32; phy_cnt++)
    {
	/* try to read ID2-register from every possible address */
	u16 data = _mdio_xfer(MDIO_READ, phy_cnt, RTL8201_PHYID2_REG, 0);

	switch(data) {
	    case 0x8201: // Realtek 8201CL
	        /* ok, seems to be a RTL8201CL, but for security, also check ID1 register */
	        data = _mdio_xfer(MDIO_READ, phy_cnt, RTL8201_PHYID1_REG, 0);
	        if (data == 0)
	        {
    		    /* tilt! This is a RTL8201CL, so store the address and skip the loop */
		    phy_addr = phy_cnt;
                    #ifdef BEAUTIFY_CONSOLE
                    printf("\xBA PHY: Realtek RTL8201CL           address: %.2X %32s\xBA\n", phy_addr, " ");
		    #else
		    printf("NET: Realtek RTL8201CL PHY at address 0x%.2X detected\n", phy_addr);
		    #endif
		    phy_cnt = 32;
	        }
		break;
	    case 0x0C54:
	        /* ok, seems to be a RTL8201CL, but for security, also check ID1 register */
	        data = _mdio_xfer(MDIO_READ, phy_cnt, RTL8201_PHYID1_REG, 0);
	        if (data == 0x0243)
	        {
    		    /* tilt! This is a IC Plus IP0101 so store the address and skip the loop */
		    phy_addr = phy_cnt;
                    #ifdef BEAUTIFY_CONSOLE
                    printf("\xBA PHY: IC Plus IP101               address: %.2X %32s\xBA\n", phy_addr, " ");
		    #else
		    printf("NET: IC Plus IP101 PHY at address 0x%.2X detected\n", phy_addr);
		    #endif
		    phy_cnt = 32;
	        }
		break;
	}
    }

    if (phy_addr < 0x20)
    {
	/* only continue, if we have an PHY */

	/* setup VMAC's DMA controller (alloc RAM for the BDT's and map to the DMA controller) */
	dma_tx_buf = malloc(NUM_TX_BDT * sizeof(tbdt));
	dma_rx_buf = malloc(NUM_RX_BDT * sizeof(rbdt));

	if (dma_tx_buf && dma_rx_buf)
	{
	    reg   = (volatile u32*) EMAC_CTRL_REG;
	    *reg  = (NUM_RX_BDT << 24) | (NUM_TX_BDT << 16);	/* delete all bits and set the TX and RX ringbuffer to only handle 1 BDT */

	    reg  = (volatile u32*) EMAC_TXRINGPTR_REG;
	    *reg = (u32) dma_tx_buf;
	    reg  = (volatile u32*) EMAC_RXRINGPTR_REG;
	    *reg = (u32) dma_rx_buf;

	    reg  = (volatile u32*) EMAC_XCTRL_REG;
	    *reg = 0x004F0040;

	    reg  = (volatile u32*) EMAC_POLLR_REG;
	    *reg = 1;

	    /* reset all interrupt status bits */
	    reg  = (volatile u32*) EMAC_STAT_REG;
	    *reg = 0x0000171F;

	    reg   = (volatile u32*) EMAC_ENABLE_REG;
	    *reg = 0x0000171F;	/* 1000 0000 0000 0000 0001 0111 0001 1111 */

	    u32 cnt;

	    for (cnt = 0; cnt < NUM_TX_BDT; cnt++)
	    {
		tx_buf[cnt] = malloc(0x0800);
		dma_tx_buf->bdt[cnt].data = tx_buf[cnt];
		dma_tx_buf->bdt[cnt].info = ENET_CTL_INFO_FIRST | ENET_CTL_INFO_LAST | ENET_CTL_INFO_ADDCRC;
	    }

	    for (cnt = 0; cnt < NUM_RX_BDT; cnt++)
	    {
		rx_buf[cnt] = malloc(0x0800);
		dma_rx_buf->bdt[cnt].data = rx_buf[cnt];
		dma_rx_buf->bdt[cnt].info = 0x07FF | ENET_CTL_INFO_OWN;
	    }

	    current_tx_packet = 0;
	    current_rx_packet = 0;
	    rx_chain = malloc(NUM_RX_BDT * 0x0800);
	    rx_chain_pos = 0;

	    have_vmac = 1;
	}
	else
	    #ifdef BEAUTIFY_CONSOLE
	    printf("\xBA *** Error - no memory for DMA available %-36s \xBA\n", "***");
	    #else
	    printf("NET: Error no memory for DMA available!\n");
	    #endif
    }
    else
    {
	#ifdef BEAUTIFY_CONSOLE
	printf("\xBA PHY: %-72s\xBA\n", nhwf);
	#else
	printf("NET: Error no PHY found\n");
	#endif

    }
}

/*******************************************************************************/
/* Initialize the NIC - u-boot call it every time before a network operation   */

int eth_init(bd_t *bis)
{
    u16 data;
    volatile u32 *reg;
    int ret = -1;	/* error condition */

    if (have_vmac)
    {
	/* read the link status from the PHY and update the link status in the MAC */
        data = _mdio_xfer(MDIO_READ, phy_addr, RTL8201_BMS_REG, 0);
	if ((data & 0x0024) == 0x0024)
	{
	    u16 duplex;
	    /* physical link established. Now read the spead and duplex mode */
	    data   = _mdio_xfer(MDIO_READ, phy_addr, RTL8201_BMC_REG, 0);
	    duplex = (data >>  8) & 1;	/* 0: half duplex, 1: full duplex */

	    /* write duplex mode into VMAC's cotrol register */
	    reg = (volatile u32*) EMAC_CTRL_REG;
	    if (duplex)		/* set bit 10 depending on the duplex mode read from the PHY */
		*reg |= 0x00000400;
	    else
		*reg &= 0xFFFFFBFF;

	    /* enable traffic */
	    reg   = (volatile u32*) EMAC_CTRL_REG;
	    *reg |= 0x00000019;	/* set bit 0, bit 3 (TX run), bit 4 (RX run) */

	    ret = 0;
	}
	else
	    printf("VMAC: network connection not established. Please check cable!\n");
    }

    return ret;
}

/*******************************************************************************/

#define INT_STATUS_TX	0x00000001	/* TX data send successfully */
#define INT_STATUS_RX	0x00000002	/* RX data available */
#define INT_STATUS_ERR	0x00000004	/* ERROR (one or more of the others set) */
#define INT_STATUS_TXCH	0x00000008	/* TX Chaining error (bad combination FIRST vs. LAST) */
#define INT_STATUS_MSER	0x00000010	/* Missed packed counter rooled over */
#define INT_STATUS_RXCR	0x00000100	/* RX CRC error counter rolled over */
#define INT_STATUS_RXFR	0x00000200	/* RX frame error counter rolled over */
#define INT_STATUS_RXFL	0x00000400	/* RX overflow counter rolled over */
#define INT_STATUS_MDIO	0x00001000	/* MDIO xfer to/from PHY complete */

int eth_handle_interrupt(void)
{
    volatile u32 *statusreg = (volatile u32*) EMAC_STAT_REG;
    u32 status = *statusreg;
    u32 cnt, do_rx = 0;

    if (status & INT_STATUS_ERR)
    {
	u32 ack = INT_STATUS_ERR;

	if (status & INT_STATUS_TXCH)
	    ack |= INT_STATUS_TXCH;

	if (status & INT_STATUS_MSER)
	    ack |= INT_STATUS_MSER;

	if (status & INT_STATUS_RXCR)
	    ack |= INT_STATUS_RXCR;

	if (status & INT_STATUS_RXFR)
	    ack |= INT_STATUS_RXFR;

	if (status & INT_STATUS_RXFL)
	    ack |= INT_STATUS_RXFL;

	*statusreg |= ack;
	status = *statusreg;
    }

    if (status & INT_STATUS_TX)	/* TX Interrupt */
    {
	for (cnt = 0; cnt < NUM_TX_BDT; cnt++)
	{
	    if ((dma_tx_buf->bdt[current_tx_packet].info & ENET_CTL_INFO_OWN) == 0)
	    {
//		printf("TX info: %.8X\n", dma_tx_buf->bdt[cnt].info);
//		debug_dump_hex(dma_tx_buf->bdt[cnt].info & 0x07FF, (u8*) dma_tx_buf->bdt[cnt].data);
	    }
	}

	*statusreg |= INT_STATUS_TX;
	status = *statusreg;
	do_rx = 1;
    }

    if ((status & INT_STATUS_RX) || (do_rx))	/* RX Interrupt */
    {
	while ((dma_rx_buf->bdt[current_rx_packet].info & ENET_CTL_INFO_OWN) == 0)
	{
//	    printf("RX info: %.8X\n", dma_rx_buf->bdt[current_rx_packet].info);

	    if (dma_rx_buf->bdt[current_rx_packet].info & ENET_CTL_INFO_FIRST)
		rx_chain_pos = 0;

	    u32 len = dma_rx_buf->bdt[current_rx_packet].info & 0x07FF;
	    if (len)
	    {
		memcpy(rx_chain + rx_chain_pos, (u8*) dma_rx_buf->bdt[current_rx_packet].data, len);
		rx_chain_pos += len;
	    }
	    if (dma_rx_buf->bdt[current_rx_packet].info & ENET_CTL_INFO_LAST)
	    {
//		debug_dump_hex(rx_chain_pos, rx_chain);
		NetReceive(rx_chain, rx_chain_pos);
	    }

	    dma_rx_buf->bdt[current_rx_packet].info = ENET_CTL_INFO_OWN | 0x07FF;
	    current_rx_packet++;
	    if (current_rx_packet == NUM_RX_BDT)
		current_rx_packet = 0;
	}

	if (status & INT_STATUS_RX)
	{
	    *statusreg |= INT_STATUS_RX;
	    status = *statusreg;
	}
    }

    if (status & INT_STATUS_MDIO)	/* MIDO xfer ready - can happen even if it's handled in _mdio_xfer() */
    {
	*statusreg |= INT_STATUS_MDIO;
	status = *statusreg;
    }

    return 1;
}

/*******************************************************************************/

int eth_send(volatile void *ptr, int len)
{
    volatile u32 *reg = (volatile u32*) EMAC_STAT_REG;

    memcpy((u8*) tx_buf[current_tx_packet], (u8*) ptr, len & 0x07FF);
    dma_tx_buf->bdt[current_tx_packet].info = (len & ENET_CTL_INFO_TX_LEN) | 
					       ENET_CTL_INFO_FIRST  | ENET_CTL_INFO_LAST | 
					       ENET_CTL_INFO_ADDCRC | ENET_CTL_INFO_OWN;

    *reg |= 0x80000000;	/* force poll */
    return 1;
}

/*******************************************************************************/

int eth_rx(void)
{
    /* we run in interrupt mode, so just return to the poller after some delay */
    udelay(250000);
    return 1;
}

/*******************************************************************************/

void eth_halt(void)
{
    /* stop all pending network traffic */
//    volatile u32 *reg = (volatile u32*) EMAC_CTRL_REG;
//    *reg &= 0xFFFFFFE6;		/* clear bit 0 */
}

/*******************************************************************************/

static u16 _mdio_xfer(u32 dir, u32 addr, u32 reg, u16 data)
{
    volatile u32 *mdioreg = (volatile u32*) EMAC_MDIO_DATA_REG;
    volatile u32 *statreg = (volatile u32*) EMAC_STAT_REG;
    u32 x = 0;
    u16 ret = 0xFFFF;

    *mdioreg = dir | (addr << 23) | reg;

    /* loop while the MDIO xfer ready bit is set. */
    while (((*statreg) & 0x00001000) == 0)
    {
	udelay(100);
	x++;
	if (x > 20)		/* xfer timed out */
	    return ret;
    }

    ret = *mdioreg & 0xFFFF;
    *statreg |= 0x00001000;	/* ACK the status bit */

    return ret;
}


void debug_dump_hex (unsigned int len, unsigned char *data)
{
    unsigned short y = 0;
    unsigned int   x = 0;
    int z;

    if ((!data) || (!len))
	return;

    printf("    ");

    for (x = 0; x < len; x++)
    {
        printf ("%.2X ", data[x]);
        y++;
        if (y == 16)
        {
            printf ("  ");
            for (z = 0; z < 16; z++)
            {
                if ((data[((x/16) * 16) + z] > 0x1F) &&
                    (data[((x/16) * 16) + z] < 0x7F))
                {
                    printf ("%c", data[((x/16) * 16) + z]);
                }
                else
                {
                    printf (".");
                }
            }
            y = 0;
            if (x < (len - 1))
            printf ("\n    ");
        }
    }
    if (y > 0)
    {
        for (z = y; z < 16; z++)
            printf ("   ");
        printf ("  ");
        for (z = 0; z < y; z++)
        {
            if ((data[(len - (len % 16)) + z] > 0x1F) &&
                (data[(len - (len % 16)) + z] < 0x7F))
            {
                printf ("%c", data[(len - (len % 16)) + z]);
            }
            else
            {
                printf (".");
            }
        }
    }
    printf ("\n");
}

