/*******************************************************************************
 *
 *  CNS3XXX SPI controller driver (master mode only)
 *
 *  Copyright (c) 2008 Cavium Networks 
 * 
 *  This file is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License, Version 2, as 
 *  published by the Free Software Foundation. 
 *
 *  This file is distributed in the hope that it will be useful, 
 *  but AS-IS and WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, TITLE, or 
 *  NONINFRINGEMENT.  See the GNU General Public License for more details. 
 *
 *  You should have received a copy of the GNU General Public License 
 *  along with this file; if not, write to the Free Software 
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA or 
 *  visit http://www.gnu.org/licenses/. 
 *
 *  This file may also be available under a different license from Cavium. 
 *  Contact Cavium Networks for more information
 *
 ******************************************************************************/

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spi-nor.h>
#include <linux/dma-mapping.h>

#include <asm/io.h>
#include <asm/memory.h>
#include <asm/dma.h>
#include <asm/delay.h>
#include <linux/platform_data/cns3xxx.h>
#include <linux/module.h>
#include <linux/platform_data/gpio-cns3xxx.h>

#define LE8221_SPI_CS		1
#define SI3226_SPI_CS		1

#define CNS3XXX_SPI_INTERRUPT
#undef CNS3XXX_SPI_INTERRUPT	/* Interrupt is not supported for D2 and SEN */

/*
 * define access macros
 */
#define SPI_MEM_MAP_VALUE(reg_offset)		(*((u32 volatile *)(CNS3XXX_SSP_BASE_VIRT + reg_offset)))

#define SPI_CONFIGURATION_REG			SPI_MEM_MAP_VALUE(0x40)
#define SPI_SERVICE_STATUS_REG			SPI_MEM_MAP_VALUE(0x44)
#define SPI_BIT_RATE_CONTROL_REG		SPI_MEM_MAP_VALUE(0x48)
#define SPI_TRANSMIT_CONTROL_REG		SPI_MEM_MAP_VALUE(0x4C)
#define SPI_TRANSMIT_BUFFER_REG			SPI_MEM_MAP_VALUE(0x50)
#define SPI_RECEIVE_CONTROL_REG			SPI_MEM_MAP_VALUE(0x54)
#define SPI_RECEIVE_BUFFER_REG			SPI_MEM_MAP_VALUE(0x58)
#define SPI_FIFO_TRANSMIT_CONFIG_REG		SPI_MEM_MAP_VALUE(0x5C)
#define SPI_FIFO_TRANSMIT_CONTROL_REG		SPI_MEM_MAP_VALUE(0x60)
#define SPI_FIFO_RECEIVE_CONFIG_REG		SPI_MEM_MAP_VALUE(0x64)
#define SPI_INTERRUPT_STATUS_REG		SPI_MEM_MAP_VALUE(0x68)
#define SPI_INTERRUPT_ENABLE_REG		SPI_MEM_MAP_VALUE(0x6C)

#define SPI_TRANSMIT_BUFFER_REG_ADDR		(CNS3XXX_SSP_BASE +0x50)
#define SPI_RECEIVE_BUFFER_REG_ADDR		(CNS3XXX_SSP_BASE +0x58)

/* Structure for SPI controller of CNS3XXX SOCs */
struct cns3xxx_spi {
	/* bitbang has to be first */
	struct spi_bitbang bitbang;
	struct completion done;
	wait_queue_head_t wait;

	int len;
	int count;
/*	int last_in_message_list; */

	/* data buffers */
	const unsigned char *tx;
	unsigned char *rx;

	struct spi_master *master;
	struct platform_device *pdev;
	struct device *dev;
};

static inline u8 cns3xxx_spi_bus_idle(void)
{
	return ((SPI_SERVICE_STATUS_REG & 0x1) ? 0 : 1);
}

static inline u8 cns3xxx_spi_tx_buffer_empty(void)
{
	return ((SPI_INTERRUPT_STATUS_REG & (0x1 << 3)) ? 1 : 0);
}

static inline u8 cns3xxx_spi_rx_buffer_full(void)
{
	return ((SPI_INTERRUPT_STATUS_REG & (0x1 << 2)) ? 1 : 0);
}

u8 cns3xxx_spi_tx_rx(u8 tx_channel, u8 tx_eof, u32 tx_data,
			    u32 * rx_data)
{
	u8 rx_channel;
	u8 rx_eof;

	while (!cns3xxx_spi_bus_idle()) ;	// do nothing

	while (!cns3xxx_spi_tx_buffer_empty()) ;	// do nothing

	SPI_TRANSMIT_CONTROL_REG &= ~(0x7);
	SPI_TRANSMIT_CONTROL_REG |= (tx_channel & 0x3) | ((tx_eof & 0x1) << 2);

	SPI_TRANSMIT_BUFFER_REG = tx_data;

	while (!cns3xxx_spi_rx_buffer_full()) ;	// do nothing

	rx_channel = SPI_RECEIVE_CONTROL_REG & 0x3;
	rx_eof = (SPI_RECEIVE_CONTROL_REG & (0x1 << 2)) ? 1 : 0;

	*rx_data = SPI_RECEIVE_BUFFER_REG;

	if ((tx_channel != rx_channel) || (tx_eof != rx_eof)) {
		return 0;
	} else {
		return 1;
	}
}

u8 cns3xxx_spi_tx(u8 tx_channel, u8 tx_eof, u32 tx_data)
{

        while (!cns3xxx_spi_bus_idle()) ;       // do nothing

        while (!cns3xxx_spi_tx_buffer_empty()) ;        // do nothing

        SPI_TRANSMIT_CONTROL_REG &= ~(0x7);
        SPI_TRANSMIT_CONTROL_REG |= (tx_channel & 0x3) | ((tx_eof & 0x1) << 2);

        SPI_TRANSMIT_BUFFER_REG = tx_data;

        return 1;
}



#ifdef CONFIG_SPI_CNS3XXX_DEBUG
static void spi_slave_probe(void)
{
	int i;
	u32 rx_data1, rx_data2, rx_data3;

	cns3xxx_spi_tx_rx(0, 0, 0x9f, &rx_data1);
	cns3xxx_spi_tx_rx(0, 0, 0xff, &rx_data1);
	cns3xxx_spi_tx_rx(0, 0, 0xff, &rx_data2);
	cns3xxx_spi_tx_rx(0, 1, 0xff, &rx_data3);
	printk("[SPI_CNS3XXX_DEBUG] manufacturer: %x\n", rx_data1);
	printk("[SPI_CNS3XXX_DEBUG] device:       %x\n",
	       ((rx_data2 & 0xff) << 8) | (u16) (rx_data3 & 0xff));

	cns3xxx_spi_tx_rx(0, 0, 0x03, &rx_data1);
	cns3xxx_spi_tx_rx(0, 0, 0x00, &rx_data1);
	cns3xxx_spi_tx_rx(0, 0, 0x00, &rx_data1);
	cns3xxx_spi_tx_rx(0, 0, 0x00, &rx_data1);
	for (i = 0; i < 15; i++) {
		cns3xxx_spi_tx_rx(0, 0, 0xff, &rx_data1);
		printk("[SPI_CNS3XXX_DEBUG] flash[%02d]:0x%02x\n", i,
		       rx_data1 & 0xff);
	}
	cns3xxx_spi_tx_rx(0, 1, 0xff, &rx_data1);
	printk("[SPI_CNS3XXX_DEBUG] flash[%02d]:0x%02x\n", i, rx_data1 & 0xff);
}
#else
#define spi_slave_probe()	do{}while(0)
#endif

static inline struct cns3xxx_spi *to_hw(struct spi_device *sdev)
{
	return spi_master_get_devdata(sdev->master);
}

static int cns3xxx_spi_setup_transfer(struct spi_device *spi,
				      struct spi_transfer *t)
{
	return 0;
}

static void cns3xxx_spi_chipselect(struct spi_device *spi, int value)
{
	unsigned int spi_config;

	switch (value) {
	case BITBANG_CS_INACTIVE:
		break;

	case BITBANG_CS_ACTIVE:
		spi_config = SPI_CONFIGURATION_REG;

		if (spi->mode & SPI_CPHA)
			spi_config |= (0x1 << 13);
		else
			spi_config &= ~(0x1 << 13);

		if (spi->mode & SPI_CPOL)
			spi_config |= (0x1 << 14);
		else
			spi_config &= ~(0x1 << 14);

		/* write new configration */
		SPI_CONFIGURATION_REG = spi_config;

		SPI_TRANSMIT_CONTROL_REG &= ~(0x7);
		SPI_TRANSMIT_CONTROL_REG |= (spi->chip_select & 0x3);

#if defined(CONFIG_LE8221_CONTROL)
		if (spi->chip_select == LE8221_SPI_CS) {
			SPI_CONFIGURATION_REG |= (0x1 << 9);
		}
#elif defined (CONFIG_SI3226_CONTROL_API)
		if (spi->chip_select == SI3226_SPI_CS) {
			SPI_CONFIGURATION_REG &= ~(0x1 << 9);
		}
#endif
		break;
	}
}

static int cns3xxx_spi_setup(struct spi_device *spi)
{
	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	return 0;
}

#ifdef CONFIG_SPI_CNS3XXX_USEDMA

int cns3xxx_spi_dma_irq_handler(void *pdata)
{

	struct cns3xxx_spi *hw = pdata;
	complete(&hw->done);

	return 0;
}

static int cns3xxx_spi_dma_initialize(int *rxchan, int *txchan, int *rxevtno,
				      int *txevtno, void *handlerargs)
{
	*rxchan = dmac_get_channel(cns3xxx_spi_dma_irq_handler, handlerargs);
	if ((*rxchan) == -1)
		goto fail1;
	*txchan = dmac_get_channel(NULL, NULL);
	if ((*txchan) == -1)
		goto fail2;
	*rxevtno = 9;
	if (dmac_get_event(*rxchan, *rxevtno) == -1)
		goto fail3;
	*txevtno = 10;
	if (dmac_get_event(*txchan, *txevtno) == -1)
		goto fail4;
	return 0;

fail4:
	dmac_release_event(*rxchan, *rxevtno);
fail3:
	dmac_release_channel(*txchan);
fail2:
	dmac_release_channel(*rxchan);
fail1:
	return -1;
}

static int cns3xxx_spi_start_dma(int rch, int tch, int rev, int tev,
				 struct spi_transfer *t, struct cns3xxx_spi *hw)
{
	static void *dummy;
	static dma_addr_t dummy_dma;
	dma_addr_t rdma, tdma;
	int rx_inc, tx_inc;
	int lc0, totlps, lc1, rump;
	u32 rx_data;

	if (!dummy) {
		dummy = dma_alloc_coherent(NULL, 16, &dummy_dma, GFP_KERNEL);
#ifdef CONFIG_SPI_CNS3XXX_DEBUG_DMA
		printk("Allocated Memory for dummy buffer va:%p,pa:%x\n", dummy,
		       dummy_dma);
#endif
	}
	if (!dummy) {
		return -1;
	}
	*((uint32_t *) dummy) = 0xffffffff;

	(t->tx_buf) ? (tdma = t->tx_dma, tx_inc = 1) :
	    (tdma = dummy_dma, tx_inc = 0);
	(t->rx_buf) ? (rdma = t->rx_dma, rx_inc = 1) :
	    (rdma = dummy_dma, rx_inc = 0);

#ifdef CONFIG_SPI_CNS3XXX_DEBUG_DMA
	printk("Here with tdma %x, rdma %x\n", tdma, rdma);
#endif

        if(t->len < 3) {
		if(t->len == 2){		
                	cns3xxx_spi_tx_rx(0,0,(t->tx_buf) ? hw->tx[0] : 0xff ,&rx_data);
		if(!(t->tx_buf))
			hw->rx[0] = rx_data & 0xff;
		}	
                cns3xxx_spi_dma_irq_handler(hw);
                return 0;
        }


	totlps = t->len - 1 -1;
	if (totlps > 0x100) {
		lc0 = 0x100;
		lc1 = totlps / lc0;
		rump = totlps % lc0;
	} else {
		lc0 = totlps;
		lc1 = 0;
		rump = 0;
	}

	if(t->tx_buf) {
                cns3xxx_spi_tx(0,0,*((uint32_t *) t->tx_buf));
                tdma+=1;
        }
        else {
                cns3xxx_spi_tx(0,0,0xff);
        }

	//SPI_RECEIVE_BUFFER_REG;
	{
		DMAC_DMAMOV(tch, SAR, tdma);
		DMAC_DMAMOV(tch, DAR, SPI_TRANSMIT_BUFFER_REG_ADDR);
		DMAC_DMAMOV(tch, CCR,
			    dmac_create_ctrlval(tx_inc, 1, 1, 0, 1, 1, 0));
		//DMAC_WFE(tch, rev);
          	if (lc1)
                        DMAC_DMALP(tch, 1, lc1);
                DMAC_DMALP(tch, 0, lc0);
                DMAC_WFE(tch, rev);
                DMAC_DMALDS(tch);
                DMAC_DMASTS(tch);
                DMAC_DMAWMB(tch);
                DMAC_DMASEV(tch, tev);
                DMAC_DMALPEND(tch, 0,
                              DMAWFE_INSTR_SIZE + DMASEV_INSTR_SIZE +
                              DMAWMB_INSTR_SIZE + DMAST_INSTR_SIZE +
                              DMALD_INSTR_SIZE, 1);
                if (lc1)
                        DMAC_DMALPEND(tch, 1,
                                      DMALP_INSTR_SIZE + DMALPEND_INSTR_SIZE +
                                      DMAWFE_INSTR_SIZE + DMASEV_INSTR_SIZE +
                                      DMAWMB_INSTR_SIZE + DMAST_INSTR_SIZE +
                                      DMALD_INSTR_SIZE, 1);

          	if (rump) {
                        DMAC_DMALP(tch, 0, rump);
                        DMAC_WFE(tch, rev);
                        DMAC_DMALDS(tch);
                        DMAC_DMASTS(tch);
                        DMAC_DMAWMB(tch);
                        DMAC_DMASEV(tch, tev);
                        DMAC_DMALPEND(tch, 0,
                                      DMAWFE_INSTR_SIZE + DMASEV_INSTR_SIZE +
                                      DMAWMB_INSTR_SIZE + DMAST_INSTR_SIZE +
                                      DMALD_INSTR_SIZE, 1);
                }


		DMAC_DMAEND(tch);
		DMAC_DMAGO(tch);
	}
	{
		DMAC_DMAMOV(rch, SAR, SPI_RECEIVE_BUFFER_REG_ADDR);
		DMAC_DMAMOV(rch, DAR, rdma);
		DMAC_DMAMOV(rch, CCR,
			    dmac_create_ctrlval(0, 1, 1, rx_inc, 1, 1, 0));

                if (lc1)
                        DMAC_DMALP(rch, 1, lc1);
                DMAC_DMALP(rch, 0, lc0);
                DMAC_DMAWFP(rch, DMAC_SPI_PERIPH_ID, PERIPHERAL);
                DMAC_DMALDP(rch, DMAC_SPI_PERIPH_ID, 0);
                DMAC_DMASTS(rch);
                DMAC_DMAWMB(rch);
                DMAC_DMASEV(rch, rev);
                DMAC_WFE(rch, tev);
                DMAC_DMALPEND(rch, 0,
                              DMAWFE_INSTR_SIZE + DMASEV_INSTR_SIZE +
                              DMAWMB_INSTR_SIZE + DMAST_INSTR_SIZE +
                              DMALDP_INSTR_SIZE + DMAWFP_INSTR_SIZE, 1);
                if (lc1)
                        DMAC_DMALPEND(rch, 1,
                                      DMAWFE_INSTR_SIZE +
                                      DMASEV_INSTR_SIZE + DMAWMB_INSTR_SIZE +
                                      DMAST_INSTR_SIZE + DMALDP_INSTR_SIZE +
                                      DMAWFP_INSTR_SIZE + DMALP_INSTR_SIZE +
                                      DMALPEND_INSTR_SIZE, 1);


                if (rump) {
                        DMAC_DMALP(rch, 0, rump);
                        DMAC_DMAWFP(rch, DMAC_SPI_PERIPH_ID, PERIPHERAL);
                        DMAC_DMALDP(rch, DMAC_SPI_PERIPH_ID, 0);
                        DMAC_DMASTS(rch);
                        DMAC_DMAWMB(rch);
                        DMAC_DMASEV(rch, rev);
                        DMAC_WFE(rch, tev);
                        DMAC_DMALPEND(rch, 0,
                                      DMAWFE_INSTR_SIZE +
                                      DMASEV_INSTR_SIZE + DMAWMB_INSTR_SIZE +
                                      DMAST_INSTR_SIZE + DMALDP_INSTR_SIZE +
                                      DMAWFP_INSTR_SIZE, 1);
                }
                // extra RX
		DMAC_DMAWFP(rch, DMAC_SPI_PERIPH_ID, PERIPHERAL);
                DMAC_DMALDP(rch, DMAC_SPI_PERIPH_ID, 0);
                DMAC_DMASTS(rch);
                DMAC_DMAWMB(rch);

		DMAC_DMAFLUSHP(rch, DMAC_SPI_PERIPH_ID);
                DMAC_DMASEV(rch, rch);  // This will generate an interrupt
		DMAC_DMAEND(rch);
		DMAC_DMAGO(rch);
	}
	return 0;
}

static void cns3xxx_spi_dma_uninitialize(int rch, int tch, int revt, int tevt)
{
	dmac_release_event(rch, revt);
	dmac_release_event(tch, tevt);
	dmac_release_channel(rch);
	dmac_release_channel(tch);
	return;
}

#endif /* CONFIG_SPI_CNS3XXX_USEDMA */

static int cns3xxx_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct cns3xxx_spi *hw = to_hw(spi);

	/* These should ideally be be per spi flash device */
	static uint8_t curr_cmd = 0x0;  
	static uint16_t curr_cmd_bytes = 0;

#ifdef CONFIG_SPI_CNS3XXX_USEDMA
	int spi_rxchan, spi_txchan, spi_rxevt, spi_txevt;
	int rx_data;
#endif
	dev_dbg(&spi->dev, "%s txrx: tx %p, rx %p, len %d\n", __FUNCTION__, t->tx_buf, t->rx_buf,
		t->len);

	hw->tx = t->tx_buf;
	hw->rx = t->rx_buf;
	hw->len = t->len;
	hw->count = 0;

	/*
	 * In original driver this variable is set via a custom
	 * addition in the generic spi.h file but it seems
	 * that we can use other criterion to judge whether this
	 * is the last message.
	 *
	 * hw->last_in_message_list = t->last_in_message_list;
	 */

#ifdef CONFIG_SPI_CNS3XXX_USEDMA
	init_completion(&hw->done);

	if (cns3xxx_spi_dma_initialize
	    (&spi_rxchan, &spi_txchan, &spi_rxevt, &spi_txevt, hw)) {
		dev_dbg(&spi->dev, "%s:%d Could not initialize DMA. \n",
			__FUNCTION__, __LINE__);
		return 0;
	}

	if (t->tx_buf)
		t->tx_dma =
		    dma_map_single(NULL, t->tx_buf, t->len, DMA_TO_DEVICE);
	if (t->rx_buf)
		t->rx_dma =
		    dma_map_single(NULL, t->rx_buf, t->len, DMA_FROM_DEVICE);

	if (cns3xxx_spi_start_dma
	    (spi_rxchan, spi_txchan, spi_rxevt, spi_txevt, t, hw)) {
		dev_dbg(&spi->dev, "Could not start DMA. \n");
		if (t->tx_buf)
			dma_unmap_single(NULL, t->tx_dma, t->len,
					 DMA_TO_DEVICE);
		t->tx_dma = 0;
		if (t->rx_buf)
			dma_unmap_single(NULL, t->rx_dma, t->len,
					 DMA_FROM_DEVICE);
		t->rx_dma = 0;
		cns3xxx_spi_dma_uninitialize(spi_rxchan, spi_txchan, spi_rxevt,
					     spi_txevt);
		return 0;
	}

	wait_for_completion(&hw->done);

	dev_dbg(&spi->dev, "DMA reported completion of transfer of %d bytes\n",
		t->len - 1);

	if (t->tx_buf)
		dma_unmap_single(NULL, t->tx_dma, t->len, DMA_TO_DEVICE);
	t->tx_dma = 0;
	if (t->rx_buf)
		dma_unmap_single(NULL, t->rx_dma, t->len, DMA_FROM_DEVICE);
	t->rx_dma = 0;
	cns3xxx_spi_dma_uninitialize(spi_rxchan, spi_txchan, spi_rxevt,
				     spi_txevt);

	if (hw->last_in_message_list)
		cns3xxx_spi_tx_rx(spi->chip_select, 1,
				  (hw->tx) ? hw->tx[hw->len - 1] : 0xff,
				  &rx_data);
	else
		cns3xxx_spi_tx_rx(spi->chip_select, 0,
				  (hw->tx) ? hw->tx[hw->len - 1] : 0xff,
				  &rx_data);

	if (hw->rx)
		hw->rx[hw->len - 1] = rx_data & 0xff;

	return hw->len;

#else /* !CONFIG_SPI_CNS3XXX_USEDMA */

#ifdef CNS3XXX_SPI_INTERRUPT

	init_completion(&hw->done);

	/* Effectively, we are enabling only the Receive Buffer Interrupt Enable */
	/* TX Buf Underrun and RX Buf Overrun are not to happen */
	SPI_INTERRUPT_ENABLE_REG = (0x1 << 2);
//	    (0x0) | (0x1 << 2) | (0x0 << 3) | (0x1 << 6) | (0x1 << 7);

	/* Write data and wait for completion */
	SPI_TRANSMIT_CONTROL_REG &= ~(0x7);
	SPI_TRANSMIT_CONTROL_REG |= (spi->chip_select & 0x3) |
	    ((((hw->last_in_message_list) && (hw->len == 1)) ? 0x1 : 0x0) << 2);

	SPI_TRANSMIT_BUFFER_REG = (hw->tx) ? hw->tx[hw->count] : 0xff;

	wait_for_completion(&hw->done);

	SPI_INTERRUPT_ENABLE_REG = 0;

	return hw->count;

#else /* !CNS3XXX_SPI_INTERRUPT */

	init_completion(&hw->done);

	if (hw->tx) {
		int i;
		u32 rx_data;
		for (i = 0; i < (hw->len - 1); i++) {
		    dev_dbg(&spi->dev,
			    "[SPI_CNS3XXX_DEBUG] hw->tx[%02d]: 0x%02x\n", i,
			    hw->tx[i]);

			cns3xxx_spi_tx_rx(spi->chip_select, 0, hw->tx[i],
					  &rx_data);
			if (hw->rx) {
				hw->rx[i] = rx_data;
				dev_dbg(&spi->dev,
					"[SPI_CNS3XXX_DEBUG] atx hw->rx[%02d]: 0x%02x\n",
					i, hw->rx[i]);
			}
		}

		/* 
		 * This is where in the original Kernel v2.6 implementation 
		 * "t->last_in_message_in_list" was queried to see if
		 * this message was the last part of a transaction. That 
		 * parameter is not available anymore so a new criterion 
		 * has to be used.
		 */
		
		if (curr_cmd == 0) {
		    curr_cmd = hw->tx[0];
		    curr_cmd_bytes = hw->len - 1;
		} else {
		    curr_cmd_bytes += hw->len;
		}

                /* 
		 * These "transmit only" SPI NOR flash operations have been 
		 * observed in use on a CNS3xxx based Seagate Central running 
		 * stock firmware. 
		 * If other such flash commands that require a "last message in
		 * list" style operation then they will need to be added here
		 * to function properly.
		 */
		if (curr_cmd == SPINOR_OP_WREN ||
		    curr_cmd == SPINOR_OP_WRDI ||
		    (curr_cmd == SPINOR_OP_BE_4K && curr_cmd_bytes >= 3) ||
		    (curr_cmd == SPINOR_OP_PP && curr_cmd_bytes >= 4)) {
			/*
			 * It is necessary to set the "tx_eof" parameter
			 * of this function to 1 when a flash transaction 
			 * is complete.
			 */ 
		        cns3xxx_spi_tx_rx(spi->chip_select, 1, hw->tx[i],
					  &rx_data);
			if (hw->rx) {
				hw->rx[i] = rx_data;
				dev_dbg(&spi->dev,
					"[SPI_CNS3XXX_DEBUG] tx - hw->rx[%02d]: 0x%02x\n",
					i, hw->rx[i]);
			}
			curr_cmd = 0x0;
			curr_cmd_bytes = 0;
		} else {
			cns3xxx_spi_tx_rx(spi->chip_select, 0, hw->tx[i],
					  &rx_data);
		}
		goto done;
	}

	if (hw->rx) {
		int i;
		u32 rx_data;
		for (i = 0; i < (hw->len - 1); i++) {
			cns3xxx_spi_tx_rx(spi->chip_select, 0, 0xff, &rx_data);
			hw->rx[i] = rx_data;
			dev_dbg(&spi->dev,
				"[SPI_CNS3XXX_DEBUG] hw->rx[%02d]: 0x%02x\n", i,
				hw->rx[i]);
		}

		cns3xxx_spi_tx_rx(spi->chip_select, 1, 0xff, &rx_data);
		hw->rx[i] = rx_data;
		dev_dbg(&spi->dev, "[SPI_CNS3XXX_DEBUG] LAST hw->rx[%02d]: 0x%02x\n",
			i, hw->rx[i]);
		curr_cmd = 0x0;
		curr_cmd_bytes = 0x0;
	}
done:
	return hw->len;

#endif /* CNS3XXX_SPI_INTERRUPT */

#endif /* CONFIG_SPI_CNS3XXX_USEDMA */
}

#ifdef CNS3XXX_SPI_INTERRUPT
/* Driver supports single master only. 
 * We have disabled fifo, so we wait for the receive buff full interrupt. 
 * Receive Buff overrun, transmit buff underrun are not to happen
 */
static irqreturn_t cns3xxx_spi_irq(int irq, void *dev)
{
	struct cns3xxx_spi *hw = dev;
	uint32_t int_status;
	uint8_t data;
	unsigned int count = hw->count;

	/* Read the interrupt status and clear interrupt */
	int_status = SPI_INTERRUPT_STATUS_REG;

	if (!(int_status & (0x1 << 2))) {
		printk("DEBUG THIS ! Unexpected interrupt (status = 0x%x)", int_status);
		/* Clearing spurious interrupts */
		SPI_INTERRUPT_STATUS_REG = (0xF << 4);
		goto irq_done;
	}

	/* Read to clear */
	data = SPI_RECEIVE_BUFFER_REG & 0xff;

	if (hw->rx)
		hw->rx[hw->count] = data;

	hw->count++;
	hw->len--;

	if (hw->len) {
		SPI_TRANSMIT_CONTROL_REG |=
		    ((((hw->last_in_message_list) && (hw->len == 1)) ? 0x1 : 0x0) << 2);
		SPI_TRANSMIT_BUFFER_REG = (hw->tx) ? hw->tx[hw->count] : 0xff;
	} else {
		complete(&hw->done);
	}

irq_done:
	return IRQ_HANDLED;
}
#endif

static struct gpio spi_gpios[] = {
	/* Pin no.   flags      label */
	{ GPIOB(7),  GPIOF_IN, "SPInCS1"	},
	{ GPIOB(8),  GPIOF_IN, "SPInCS0"	},
	{ GPIOB(9),  GPIOF_IN, "SPICLK"		},
	{ GPIOB(10), GPIOF_IN, "SPIDT"		},
	{ GPIOB(11), GPIOF_IN, "SPIDR"		},
};

static void __init cns3xxx_spi_initial(void)
{
	/* Share pin config. */
	cns3xxx_sharepin_request_array(spi_gpios, ARRAY_SIZE(spi_gpios));
	cns3xxx_pwr_clk_en(CNS3XXX_PWR_CLK_EN(SPI_PCM_I2C));
	cns3xxx_pwr_soft_rst(CNS3XXX_PWR_SOFTWARE_RST(SPI_PCM_I2C));
	//SPI Pin Drive Strength
	//(0x30: 21mA) 
	//(0x20: 15.7mA) 
	//(0x10: 10.5mA) 
	//(0x00: 5.2mA)
	MISC_IO_PAD_DRIVE_STRENGTH_CTRL_B &= ~0x30;
	MISC_IO_PAD_DRIVE_STRENGTH_CTRL_B |= 0x30; //21mA...
	SPI_CONFIGURATION_REG = (((0x0 & 0x3) << 0) |	/* 8bits shift length */
				 (0x0 << 9) |	/* SPI mode */
				 (0x0 << 10) |	/* disable FIFO */
				 (0x1 << 11) |	/* SPI master mode */
				 (0x0 << 12) |	/* disable SPI loopback mode */
				 (0x1 << 13) |	/* clock phase */
				 (0x1 << 14) |	/* clock polarity */
				 (0x0 << 24) |	/* disable - SPI data swap */
#ifdef CONFIG_SPI_CNS3XXX_2IOREAD
				 (0x1 << 29) |	/* enable - 2IO Read mode */
#else
				 (0x0 << 29) |	/* disablea - 2IO Read mode */
#endif
				 (0x0 << 30) |	/* disable - SPI high speed read for system boot up */
				 (0x0 << 31));	/* disable - SPI */

	/* Set SPI bit rate */
	{
		int val, div, set_div;
		int cpu_clk = cns3xxx_cpu_clock(); // Mhz
		int src_clk;
		
		/* Hope the clock rate is under 2Mhz */
		src_clk = cpu_clk/6;
		set_div = src_clk/2;
		
		val = 0;
		while (val <= 7) {
			if (val == 0) {
				if (set_div < 2) break;
			}else if (val == 1) {
				if (set_div < 4) break;
			}else if (val == 2) {
				if (set_div < 6) break;
			}else {
				if (set_div < (1<<val)) break;
			}
			val++;
		}
		
		SPI_BIT_RATE_CONTROL_REG = val;
		
		
		val = SPI_BIT_RATE_CONTROL_REG;
		switch (val) {
			case 0:
				div = 2;
				break;																												
			case 1:
				div = 4;
				break;																												
			case 2:
				div = 6;
				break;																												
			case 3:
				div = 8;
				break;																												
			case 4:
				div = 16;
				break;																												
			case 5:
				div = 32;
				break;																												
			case 6:
				div = 64;
				break;																												
			case 7:
				div = 128;				
				break;																												
			default:
				div = 1;
				printk("cns3xxx_spi_initial: unknow div!\r\n");
				break;																												
		}
		printk("SPI bit rate = %d/%d Mhz\r\n", src_clk, div);
	}
	/* Set SPI Tx channel 0 */
	SPI_TRANSMIT_CONTROL_REG = 0x0;

	/* Set Tx FIFO Threshold, Tx FIFO has 2 words */
	SPI_FIFO_TRANSMIT_CONFIG_REG &= ~(0x03 << 4);
	SPI_FIFO_TRANSMIT_CONFIG_REG |= ((0x0 & 0x03) << 4);

	/* Set Rx FIFO Threshold, Rx FIFO has 2 words */
	SPI_FIFO_RECEIVE_CONFIG_REG &= ~(0x03 << 4);
	SPI_FIFO_RECEIVE_CONFIG_REG |= ((0x0 & 0x03) << 4);

	/* Disable all interrupt */
	SPI_INTERRUPT_ENABLE_REG = 0x0;

	/* Clear spurious interrupt sources */
	SPI_INTERRUPT_STATUS_REG = (0x0F << 4);

	/* Enable SPI */
	SPI_CONFIGURATION_REG |= (0x1 << 31);

	return;
}

static int cns3xxx_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct cns3xxx_spi *hw;
	int err = 0;

	printk("%s: setup CNS3XXX SPI Controller", __FUNCTION__);
#ifdef CONFIG_SPI_CNS3XXX_USEDMA
	printk(" w/ DMA \n");
#else
#ifdef CNS3XXX_SPI_INTERRUPT
	printk(" in Interrupt mode, w/o DMA \n");
#else
	printk(" in polling mode, w/o DMA \n");
#endif
#endif

	/* Allocate master with space for cns3xxx_spi */
	master = spi_alloc_master(&pdev->dev, sizeof(struct cns3xxx_spi));
	if (master == NULL) {
		dev_err(&pdev->dev, "No memory for spi_master\n");
		err = -ENOMEM;
		goto err_nomem;
	}

	hw = spi_master_get_devdata(master);
	memset(hw, 0, sizeof(struct cns3xxx_spi));

	hw->master = spi_master_get(master);
	hw->dev = &pdev->dev;

	platform_set_drvdata(pdev, hw);
	init_completion(&hw->done);

	/* setup the master state. */

	master->num_chipselect = 4;
	master->bus_num = 1;

	/* setup the state for the bitbang driver */

	hw->bitbang.master = hw->master;
	hw->bitbang.setup_transfer = cns3xxx_spi_setup_transfer;
	hw->bitbang.chipselect = cns3xxx_spi_chipselect;
	hw->bitbang.txrx_bufs = cns3xxx_spi_txrx;
	hw->bitbang.master->setup = cns3xxx_spi_setup;

	dev_dbg(hw->dev, "bitbang at %p\n", &hw->bitbang);

#ifdef CNS3XXX_SPI_INTERRUPT
	err = request_irq(IRQ_CNS3XXX_SPI, cns3xxx_spi_irq, IRQF_SHARED, "cns3xxx_spi", hw);
	if (err) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		goto err_no_irq;
	}
#endif

	/* SPI controller initializations */
	cns3xxx_spi_initial();

	/* register SPI controller */

	err = spi_bitbang_start(&hw->bitbang);
	if (err) {
		dev_err(&pdev->dev, "Failed to register SPI master\n");
		goto err_register;
	}

	spi_slave_probe();

	return 0;

err_register:
#ifdef CNS3XXX_SPI_INTERRUPT
err_no_irq:
#endif
	spi_master_put(hw->master);;

err_nomem:
	return err;
}

static int cns3xxx_spi_remove(struct platform_device *dev)
{
	struct cns3xxx_spi *hw = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	spi_unregister_master(hw->master);

	//cns3xxx_spi_clk_disable();
	spi_master_put(hw->master);

	/* Disable SPI */
	SPI_CONFIGURATION_REG &= ~(0x1 << 31);	
#ifdef CONFIG_PM
	cns3xxx_pwr_clk_dis(CNS3XXX_PWR_CLK_EN(SPI_PCM_I2C));
#endif
	cns3xxx_sharepin_free_array(spi_gpios, ARRAY_SIZE(spi_gpios));

	return 0;
}

#ifdef CONFIG_PM

static int cns3xxx_spi_suspend(struct platform_device *pdev, pm_message_t msg)
{
	/* clock disable */
	cns3xxx_pwr_clk_dis(0x1 << PM_CLK_GATE_REG_OFFSET_SPI_PCM_I2C);

	return 0;
}

static int cns3xxx_spi_resume(struct platform_device *pdev)
{
	/* clock enable */
	cns3xxx_pwr_clk_en(0x1 << PM_CLK_GATE_REG_OFFSET_SPI_PCM_I2C);
	
	return 0;
}

#else
#define cns3xxx_spi_suspend	NULL
#define cns3xxx_spi_resume	NULL
#endif


static const struct of_device_id cns3xxx_spi_match[] = {
    { .compatible = "cavium,cns3xxx_spi", },
    {},
};
MODULE_DEVICE_TABLE(of, cns3xxx_spi_match);


struct platform_driver cns3xxx_spi_driver = {
	.probe		= cns3xxx_spi_probe,
	.remove		= cns3xxx_spi_remove,
	.suspend	= cns3xxx_spi_suspend,
	.resume		= cns3xxx_spi_resume,
	.driver		= {
		.name = "cns3xxx_spi",
		.owner = THIS_MODULE,
	},
};
EXPORT_SYMBOL(cns3xxx_spi_driver);


module_platform_driver(cns3xxx_spi_driver);

MODULE_AUTHOR("Cavium Networks");
MODULE_DESCRIPTION("CNS3XXX SPI Controller Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cns3xxx_spi");

EXPORT_SYMBOL_GPL(cns3xxx_spi_tx_rx);
