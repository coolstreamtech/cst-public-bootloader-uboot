#ifndef __FP_UART_H__
#define __FP_UART_H__

#define FP_DEFAULT_UART				(1)

/* UART's 
   (16550 compatible except: FIFOs allways on, no DMA mode select, 
   no support for 5 and 6 bit data frames, no scratch register) */
#define UART_FIFO_BRDL_REG(x)		(0xE0410000 + ((x) * 0x1000))	/* FIFO (BDS=0) or lower baud rate divisor (BDS=1) register (x = 0 ... 3) */
#define UART_IRQE_BRDU_REG(x)		(0xE0410004 + ((x) * 0x1000))	/* Interrupt enable (BDS=0) or upper baud rate divisor (BDS=1) register (x = 0 ... 3) */
#define UART_FIFC_REG(x)		(0xE0410008 + ((x) * 0x1000))	/* FIFO control register (x = 0 ... 3) */
#define UART_FRMC_REG(x)		(0xE041000C + ((x) * 0x1000))	/* Frame control register (x = 0 ... 3) */
#define UART_STAT_REG(x)		(0xE0410014 + ((x) * 0x1000))	/* Status register (x = 0 ... 3) */
#define UART_IRLVL_REG(x)		(0xE0410018 + ((x) * 0x1000))	/* Interrupt level register (x = 0 ... 3) */
#define UART_IRDC_REG(x)		(0xE0410020 + ((x) * 0x1000))	/* IrDA control register (x = 0 ... 3) */
#define UART_TXSTA_REG(x)		(0xE0410028 + ((x) * 0x1000))	/* Transmit FIFO status register (x = 0 ... 3) */
#define UART_RXSTA_REG(x)		(0xE041002C + ((x) * 0x1000))	/* Receive FIFO status register (x = 0 ... 3) */
#define UART_EXP_REG(x)			(0xE0410030 + ((x) * 0x1000))	/* Expansion register (x = 0 ... 3) */

static inline void fp_uart_start_tx(void)
{
	volatile u8 *irqe = (volatile u8 *) UART_IRQE_BRDU_REG(FP_DEFAULT_UART);
	*irqe |= 0x40;		/* TID */
}

static inline void fp_uart_stop_tx(void)
{
	volatile u8 *irqe = (volatile u8 *) UART_IRQE_BRDU_REG(FP_DEFAULT_UART);
	*irqe &= ~0x60;
}

static inline void fp_uart_start_rx(void)
{
	volatile u8 *irqe = (volatile u8 *) UART_IRQE_BRDU_REG(FP_DEFAULT_UART);
	*irqe |= 0x01;		/* TID */
}

static inline void fp_uart_stop_rx(void)
{
	volatile u8 *irqe = (volatile u8 *) UART_IRQE_BRDU_REG(FP_DEFAULT_UART);
	*irqe &= ~0x01;
}

static inline u8 fp_uart_get_status(void)
{
	volatile u8 *sta = (volatile u8 *) UART_STAT_REG(FP_DEFAULT_UART);
	return *sta;
}

static inline u8 fp_uart_get_ie(void)
{
	volatile u8 *irqe = (volatile u8 *) UART_IRQE_BRDU_REG(FP_DEFAULT_UART);

	return *irqe;
}

static inline u8 fp_uart_get_rx(void)
{
	volatile u8 *fifo = (volatile u8 *) UART_FIFO_BRDL_REG(FP_DEFAULT_UART);
	return *fifo;
}

static inline void fp_uart_set_tx(u8 d)
{
	volatile u8 *fifo = (volatile u8 *) UART_FIFO_BRDL_REG(FP_DEFAULT_UART);
	*fifo = d;
}

static inline u8 fp_uart_get_rx_count(void)
{
	volatile u8 *rxsta = (volatile u8 *) UART_RXSTA_REG(FP_DEFAULT_UART);
	return *rxsta;
}

static inline u8 fp_uart_get_tx_count(void)
{
	volatile u8 *txsta = (volatile u8 *) UART_TXSTA_REG(FP_DEFAULT_UART);
	return *txsta;
}

#endif /* __FP_UART_H__ */
