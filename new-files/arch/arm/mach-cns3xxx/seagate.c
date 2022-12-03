/*
 * Cavium Networks CNS3420 Validation Board
 *
 * Copyright 2000 Deep Blue Solutions Ltd
 * Copyright 2008 ARM Limited
 * Copyright 2008 Cavium Networks
 *		  Scott Shu
 * Copyright 2010 MontaVista Software, LLC.
 *		  Anton Vorontsov <avorontsov@mvista.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/serial_core.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/memblock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/partitions.h>
#include <linux/usb/ehci_pdriver.h>
#include <linux/usb/ohci_pdriver.h>
#include <linux/spi/spi.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <linux/platform_data/cns3xxx.h>
#include <linux/platform_data/gpio-cns3xxx.h>
#include <mach/pm.h>
#include "core.h"
#include "devices.h"

#ifdef CONFIG_DEBUG_FS 
#include <linux/debugfs.h>
#endif

extern struct smp_operations cns3xxx_smp_ops;

/*
 * UART
 */
static void __init cns3420_early_serial_setup(void)
{
#ifdef CONFIG_SERIAL_8250_CONSOLE
	static struct uart_port cns3420_serial_port = {
		.membase        = (void __iomem *)CNS3XXX_UART0_BASE_VIRT,
		.mapbase        = CNS3XXX_UART0_BASE,
		.irq            = IRQ_CNS3XXX_UART0,
		.iotype         = UPIO_MEM,
		.flags          = UPF_BOOT_AUTOCONF | UPF_FIXED_TYPE,
		.regshift       = 2,
		.uartclk        = 24000000,
		.line           = 0,
		.type           = PORT_16550A,
		.fifosize       = 16,
	};

	early_serial_setup(&cns3420_serial_port);
#endif
}


/*
 * USB
 */
static struct resource cns3xxx_usb_ehci_resources[] = {
	[0] = {
		.start = CNS3XXX_USB_BASE,
		.end   = CNS3XXX_USB_BASE + SZ_16M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_CNS3XXX_USB_EHCI,
		.end = IRQ_CNS3XXX_USB_EHCI,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 cns3xxx_usb_ehci_dma_mask = DMA_BIT_MASK(32);

#ifdef CNS3XXX_DEBUG_USB_REG_DUMP
static void cns3xxx_dump_regs(void *reg_base, uint max_offset) {

    u32 offset;

    printk (" %s Dump regs \n", __FUNCTION__);
    printk (" Base : 0x%px \n", reg_base);
    for (offset = 0x0; offset < max_offset; offset += 16) {

	printk (" %04x : %08x   %08x  %08x  %08x \n",
		offset,
		readl(reg_base + offset),
		readl(reg_base + offset + 4),
		readl(reg_base + offset + 8),
		readl(reg_base + offset + 12));
    }

} /* cns3xxx_dump_regs() */
#endif

static int csn3xxx_usb_power_on(struct platform_device *pdev)
{
	/*
	 * EHCI and OHCI share the same clock and power,
	 * resetting twice would cause the 1st controller been reset.
	 * Therefore only do power up  at the first up device, and
	 * power down at the last down device.
	 *
	 * Set USB AHB INCR length to 16
	 */
	if (atomic_inc_return(&usb_pwr_ref) == 1) {
		cns3xxx_pwr_power_up(1 << PM_PLL_HM_PD_CTRL_REG_OFFSET_PLL_USB);
		cns3xxx_pwr_clk_en(1 << PM_CLK_GATE_REG_OFFSET_USB_HOST);
		cns3xxx_pwr_soft_rst(1 << PM_SOFT_RST_REG_OFFST_USB_HOST);
		
		// This was in generic but it's not in seagate. It makes no difference
		// Leaving here just to cover up register setting differences. Remove if it
		// doesn't help.
		writel( (readl(((void *)(CNS3XXX_MISC_BASE_VIRT+0x04)))| (0X2<<24)), (void *)(CNS3XXX_MISC_BASE_VIRT+0x04));//Jacky-20100921: INCR4-->INCR16
		// The statement below is also used and has the same effect
                //		__raw_writel((__raw_readl(MISC_CHIP_CONFIG_REG) | (0X2 << 24)),
		//	MISC_CHIP_CONFIG_REG);

	}

#ifdef CNS3XXX_DEBUG_USB_REG_DUMP
	printk (" %s Dumping Power Registers \n", __FUNCTION__);
	cns3xxx_dump_regs(PM_CLK_GATE_REG, 0x40);
	printk (" %s Dumping MISC USB control Registers \n", __FUNCTION__);
	cns3xxx_dump_regs((void *)MISC_MEM_MAP(0x800), 0x20);
	printk (" %s Dumping GIC BASE Registers \n", __FUNCTION__);
	cns3xxx_dump_regs((void *)CNS3XXX_TC11MP_GIC_CPU_BASE_VIRT, 0x100);
	printk (" %s Dumping GIC Dist Registers \n", __FUNCTION__);
	cns3xxx_dump_regs((void *)CNS3XXX_TC11MP_GIC_DIST_BASE_VIRT, 0x200);
#endif /* CNS3XXX_DEBUG_USB_REG_DUMP */
	return 0;
}

static void csn3xxx_usb_power_off(struct platform_device *pdev)
{
	/*
	 * EHCI and OHCI share the same clock and power,
	 * resetting twice would cause the 1st controller been reset.
	 * Therefore only do power up  at the first up device, and
	 * power down at the last down device.
	 */
    if (atomic_dec_return(&usb_pwr_ref) == 0) {
		cns3xxx_pwr_clk_dis(1 << PM_CLK_GATE_REG_OFFSET_USB_HOST);
    }
}

static struct usb_ehci_pdata cns3xxx_usb_ehci_pdata = {
	.power_on	= csn3xxx_usb_power_on,
	.power_off	= csn3xxx_usb_power_off,
};

static struct platform_device cns3xxx_usb_ehci_device = {
//	.name          = "cns3xxx-ehci",
       	.name          = "ehci-platform",
	.num_resources = ARRAY_SIZE(cns3xxx_usb_ehci_resources),
	.resource      = cns3xxx_usb_ehci_resources,
	.dev           = {
		.dma_mask          = &cns3xxx_usb_ehci_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data     = &cns3xxx_usb_ehci_pdata,
	},
};

static struct resource cns3xxx_usb_ohci_resources[] = {
	[0] = {
		.start = CNS3XXX_USB_OHCI_BASE,
		.end   = CNS3XXX_USB_OHCI_BASE + SZ_16M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_CNS3XXX_USB_OHCI,
		.end = IRQ_CNS3XXX_USB_OHCI,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 cns3xxx_usb_ohci_dma_mask = DMA_BIT_MASK(32);

static struct usb_ohci_pdata cns3xxx_usb_ohci_pdata = {
	.num_ports	= 1,
	.power_on	= csn3xxx_usb_power_on,
	.power_off	= csn3xxx_usb_power_off,
};

static struct platform_device cns3xxx_usb_ohci_device = {
	.name          = "ohci-platform",
	.num_resources = ARRAY_SIZE(cns3xxx_usb_ohci_resources),
	.resource      = cns3xxx_usb_ohci_resources,
	.dev           = {
		.dma_mask          = &cns3xxx_usb_ohci_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data	   = &cns3xxx_usb_ohci_pdata,
	},
};

/*
 * Ethernet
 */

/*
 * Note : The first hwaddr entry below sets the default
 * Ethernet MAC address of the Seagate Central 
 * Single Bay to 02:5e:9a:7e:ce:00 in the case that
 * a MAC address can't be retrieved from flash.
 *
 * I can't find any other constant but unique per unit 
 * values to derive a MAC address from so I decided that 
 * having a predictable MAC address was better than a 
 * randomly generated one. It's a little bit dangerous 
 * because if there's more than one Seagate Central on
 * a LAN using this "default" then there will be major 
 * network problems.
 */
static struct cns3xxx_plat_info seagate_net_data = {  
	.ports = 0x01,
	.hwaddr = {{0x02, 0x5e, 0x9a, 0x7e, 0xce, 0x00},
		   {0x02, 0x5e, 0x9a, 0x7e, 0xce, 0x01},
		   {0x02, 0x5e, 0x9a, 0x7e, 0xce, 0x02},
		   {0x02, 0x5e, 0x9a, 0x7e, 0xce, 0x03}
	},
	.phy = {
		0,
		1,
		2,
	},
};

static struct resource seagate_net_resource[] = {
	{
		.name = "eth0_mem",
		.start = CNS3XXX_SWITCH_BASE,
		.end = CNS3XXX_SWITCH_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM
	}, {
		.name = "eth_rx",
		.start = IRQ_CNS3XXX_SW_R0RXC,
		.end = IRQ_CNS3XXX_SW_R0RXC,
		.flags = IORESOURCE_IRQ
	}, {
		.name = "eth_stat",
		.start = IRQ_CNS3XXX_SW_STATUS,
		.end = IRQ_CNS3XXX_SW_STATUS,
		.flags = IORESOURCE_IRQ
	}
};

static u64 seagate_net_dmamask = DMA_BIT_MASK(32);
static struct platform_device seagate_net_device = {
	.name = "cns3xxx_eth",
	.num_resources = ARRAY_SIZE(seagate_net_resource),
	.resource = seagate_net_resource,
	.dev = {
		.dma_mask = &seagate_net_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &seagate_net_data,
	}
};



/* RTC */
static struct resource cns3xxx_rtc_resources[] = {
    [0] = {
	.start = CNS3XXX_RTC_BASE,
	.end   = CNS3XXX_RTC_BASE + PAGE_SIZE - 1,
	.flags = IORESOURCE_MEM,
    },
    [1] = {
	.start = IRQ_CNS3XXX_RTC,
	.end   = IRQ_CNS3XXX_RTC,
	.flags = IORESOURCE_IRQ,
    }
};

static struct platform_device cns3xxx_rtc_device = {
    .name      = "cns3xxx-rtc",
    .id          = -1,
    .num_resources  = ARRAY_SIZE(cns3xxx_rtc_resources),
    .resource       = cns3xxx_rtc_resources,
};


/* GPIO */
static struct resource cns3xxx_gpio_resources[] = {
    [0] = {
	.start = CNS3XXX_GPIOA_BASE,
	.end   = CNS3XXX_GPIOA_BASE + GPIO_BOUNCE_PRESCALE_OFFSET + 3,
	.flags = IORESOURCE_MEM,
	.name  = "GPIOA",
    },
    [1] = {
	.start = CNS3XXX_GPIOB_BASE,
	.end   = CNS3XXX_GPIOB_BASE + GPIO_BOUNCE_PRESCALE_OFFSET + 3,
	.flags = IORESOURCE_MEM,
	.name  = "GPIOB",
    },
    [2] = {
	.start = IRQ_CNS3XXX_GPIOA,
	.end = IRQ_CNS3XXX_GPIOA,
	.flags = IORESOURCE_IRQ,
	.name  = "GPIOA",
    },
    [3] = {
	.start = IRQ_CNS3XXX_GPIOB,
	.end = IRQ_CNS3XXX_GPIOB,
	.flags = IORESOURCE_IRQ,
	.name  = "GPIOB",
    },
    [4] = {
	.start = CNS3XXX_MISC_BASE,
	.end   = CNS3XXX_MISC_BASE + 0x799,
	.flags = IORESOURCE_MEM,
	.name  = "MISC"
    },

};

static struct platform_device cns3xxx_gpio_device = {
    .name           = "cns3xxx-gpio",
    .num_resources  = ARRAY_SIZE(cns3xxx_gpio_resources),
    .resource       = cns3xxx_gpio_resources,
};


static struct platform_device cns3xxx_spi_controller_device = {
    .name           = "cns3xxx_spi",
};


struct spi_flash_platform_data {
    char            *name;
    struct mtd_partition *parts;
    unsigned int    nr_parts;

    char            *type;

    /* we'll likely add more ... use JEDEC IDs, etc */
};



/* SPI */
static struct mtd_partition cns3xxx_spi_partitions[] = {
    {
	.name           = "SPI-UBoot",
	.offset         = 0,
	.size           = 0x30000,
	.mask_flags     = 0, //MTD_WRITEABLE, /* force read-only */
    },
#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
    {
	.name           = "SPI-UBootEnv1",
	.offset         = 0x30000,
	.size           = 0x08000,
	.mask_flags     = 0,
    },
    {
	.name           = "SPI-UBootEnv2",
	.offset         = 0x38000,
	.size           = 0x08000,
	.mask_flags     = 0,
    },
#else
    {
	.name           = "SPI-UBootEnv",
	.offset         = 0x30000,
	.size           = 0x10000,
	.mask_flags     = 0,
    },
#endif
    {
	.name           = "SPI-FileSystem",
	.offset         = 0x40000,
	.size           = MTDPART_SIZ_FULL,
	.mask_flags     = 0,
    },
};

static struct spi_flash_platform_data cns3xxx_spi_flash_data = {
    .parts          = cns3xxx_spi_partitions,
    .nr_parts       = ARRAY_SIZE(cns3xxx_spi_partitions),
};


static struct spi_board_info __initdata cns3xxx_spi_devices[] = {
    [0] = {
	.modalias               = "m25p80",
	.bus_num                = 1,
	.chip_select            = 0,
	.max_speed_hz           = 25 * 1000 * 1000,
	.platform_data          =  &cns3xxx_spi_flash_data,
    }, {
#if defined(CONFIG_LE8221_CONTROL)
	.modalias               = "le88221",
#elif defined(CONFIG_SI3226_CONTROL_API)
	.modalias               = "si3226",
	#endif
	.bus_num                = 1,
	.chip_select            = 1,
	.max_speed_hz           = 25 * 1000 * 1000,
    },
};


/*
 * LEDS
 */
static struct platform_device cns3xxx_leds_device = {
    .name       = "cns3xxx_leds",
    .id     = 1,
    .dev        = {
	.platform_data  = NULL,
    },
};

#ifdef CONFIG_DEBUG_FS

struct dentry *cns3xxx_debugfs_dir;

/*
 * Debug file
 */

static void cns3xxx_debugfs_dump_regs(struct seq_file *s, void *reg_base, uint max_offset) {

    u32 offset;

    seq_printf (s, " Base : 0x%px \n", reg_base);
    for (offset = 0x0; offset < max_offset; offset += 16) {

	seq_printf (s, " %04x :  %08x    %08x    %08x    %08x \n",
		offset,
		readl(reg_base + offset),
		readl(reg_base + offset + 4),
		readl(reg_base + offset + 8),
		readl(reg_base + offset + 12));
    }

} /* cns3xxx_debugfs_dump_regs() */

static int cns3xxx_debugfs_show_all(struct seq_file *s, void *unused)
{

    seq_printf(s, "cns3xxx debug info: \n");

    seq_printf(s, "\nCNS3XXX_TC11MP_SCU_BASE_VIRT : \n");
    cns3xxx_debugfs_dump_regs(s, (void *)CNS3XXX_TC11MP_SCU_BASE_VIRT, 0x100);
    seq_printf(s, "\nCNS3XXX_TC11MP_GIC_CPU_BASE_VIRT : \n");
    cns3xxx_debugfs_dump_regs(s, (void *)CNS3XXX_TC11MP_GIC_CPU_BASE_VIRT, 0x100);
    seq_printf(s, "\nCNS3XXX_TC11MP_GIC_DIST_BASE_VIRT : \n");
    cns3xxx_debugfs_dump_regs(s, (void *)CNS3XXX_TC11MP_GIC_DIST_BASE_VIRT, 0x1000);
    seq_printf(s, "\nCNS3XXX_USB_BASE_VIRT \n");
    cns3xxx_debugfs_dump_regs(s, (void *)CNS3XXX_USB_BASE_VIRT, 0x104);
    seq_printf(s, "\nPower Registers \n");
    cns3xxx_debugfs_dump_regs(s, (void *)PM_CLK_GATE_REG, 0x40);
    seq_printf(s, "\nMISC USB control Registers \n");
    cns3xxx_debugfs_dump_regs(s, (void *)MISC_MEM_MAP(0x800), 0x20);
    seq_printf(s, "\nCNS3XXX_MISC_BASE_VIRT Misc Registers \n");
    cns3xxx_debugfs_dump_regs(s, (void *)CNS3XXX_MISC_BASE_VIRT, 0xb00);
    return 0;
}


static int debugfs_open(struct inode *inode, struct file *file)
{
    return single_open(file, cns3xxx_debugfs_show_all, &inode->i_private);
}

static const struct file_operations debug_fops = {
    .open           = debugfs_open,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int __init cns3xxx_debugfs_init(void)
{
    cns3xxx_debugfs_dir = debugfs_create_dir("cns3xxx", NULL);
    debugfs_create_file("cns3xxx-dump", S_IRUGO, cns3xxx_debugfs_dir, NULL, &debug_fops);
    return 0;
}

#endif /* CONFIG_DEBUG_FS */

#ifdef CONFIG_CNS3XXX_FAKE_SGNOTIFY_NODE
/*
 * Value below set as per original seagate firmware
 */
#define CNS3XXX_FAKE_SGNOTIFY_MAJOR  60

// #define DEBUG_FAKE_SGNOTIFY
//
// Note : We'll probably get rid of these debugs
// assuming nothing unexpected happens in the next
// few years of running this feature

static loff_t cns3xxx_fake_sgnotify_lseek(struct file *file,
					  loff_t offset,
					  int orig)
{
#ifdef DEBUG_FAKE_SGNOTIFY
	static int debug_count = 0;

	if (debug_count++ < 5) {
		printk ("%s :\n", __FUNCTION__);
	}
	if (debug_count > 10000) {
		debug_count = 0;
	}
#endif
	return 0;
}

static ssize_t cns3xxx_fake_sgnotify_read(struct file *file,
					  char __user *buf,
					  size_t count,
					  loff_t *ppos)
{
#ifdef DEBUG_FAKE_SGNOTIFY
	static int debug_count = 0;

	if (debug_count++ < 5) {
		printk ("%s :\n", __FUNCTION__);
	}
	if (debug_count > 1000) {
		debug_count = 0;
	}
#endif
	return 0;
}

static ssize_t cns3xxx_fake_sgnotify_write(struct file *file,
					   const char __user *buf,
					   size_t count,
					   loff_t *ppos)
{
#ifdef DEBUG_FAKE_SGNOTIFY
	static int debug_count = 0;

	if (debug_count++ < 20) {
		printk ("%s count %d string %*s \n", __FUNCTION__, count, count, buf);
	}
	if (debug_count > 10000) {
		debug_count = 0;
	}
#endif
	return count;
}
int cns3xxx_fake_sgnotify_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG_FAKE_SGNOTIFY
	static int debug_count = 0;

	if (debug_count++ < 40) {
		printk ("%s  \n", __FUNCTION__);
	}
	if (debug_count > 1000) {
		debug_count = 0;
	}
#endif
	return 0;
}

static __poll_t
cns3xxx_fake_sgnotify_poll(struct file *file, poll_table *wait)
{
#ifdef DEBUG_FAKE_SGNOTIFY
	static int debug_count = 0;
		
	if (debug_count < 20) {
		printk ("%s : Start \n", __FUNCTION__);
	}

	if (debug_count > 100) {
		debug_count = 0;
	}
#endif
	return (POLLOUT | POLLWRNORM);  // Ready for writing
}

/*
 * Numbers taken from /media_server/runScannerDaemon.py
 */
#define SG_FS_NOTIFY_GET_INFO        0x5301
#define SG_FS_NOTIFY_ACK             0x5302
#define SG_FS_NOTIFY_SET_WATCH_DIR   0x5303
#define SG_FS_NOTIFY_START_WATCH     0x5306
#define SG_FS_NOTIFY_ADD_IGNORE_DIR  0x5308


/*
 * This set of data is meant to indicate that the sgnotify
 * module has not detected any changes to the file systems.
 * In reality files may have indeed changed but this fake
 * module ignores them all.
 */
int cns3xxx_fake_sgnotify_info[16] = {0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0};


/*
 * cns3xxx_fake_sgnotify_ioctl()
 * This ioctl handling function hopefully convinces the
 * calling process that no changes have occured and so
 * the Seagate Media Server doesn't need to do anything.
 */
long cns3xxx_fake_sgnotify_ioctl(struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int res;
#ifdef DEBUG_FAKE_SGNOTIFY
	unsigned char *cargp = argp;
	static int debug_count = 0;

	if (debug_count++ < 6) {
		printk ("%s : count = %d cmd = 0x%x arg = %lu \n", __FUNCTION__, debug_count, cmd, arg);
		printk ("%s : Data 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
			__FUNCTION__,
			cargp[0], cargp[1], cargp[2], cargp[3],
			cargp[4], cargp[5], cargp[6], cargp[7]); 
	}
	if (debug_count > 10000) {
		debug_count = 0;
	}
#endif
	switch (cmd) {
	case SG_FS_NOTIFY_GET_INFO:
		res = copy_to_user(argp, &cns3xxx_fake_sgnotify_info,
				   sizeof(cns3xxx_fake_sgnotify_info));
		if (res) {
			printk ("%s : Fault copying cns3xxx_fake_sgnotify_info \n",
				__FUNCTION__);
			return -EFAULT;
		}
		return 0;

	case SG_FS_NOTIFY_ACK:
		return 0;

	case SG_FS_NOTIFY_START_WATCH:
		/*
		 * Normally invoked once as the server starts.
		 */
		return 0;
		
	default:
#ifdef DEBUG_FAKE_SGNOTIFY
		printk ("%s : UNEXPECTED IOCTL cmd = 0x%x arg = %lu \n", __FUNCTION__, cmd, arg);
		printk ("%s : Data 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
			__FUNCTION__,
			cargp[0], cargp[1], cargp[2], cargp[3],
			cargp[4], cargp[5], cargp[6], cargp[7]);
#endif
		return 0;
	}
	return 0;
}
int cns3xxx_fake_sgnotify_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations cns3xxx_fake_sgnotify_fops = {
	.owner = THIS_MODULE,
	.llseek = cns3xxx_fake_sgnotify_lseek,
	.read = cns3xxx_fake_sgnotify_read,
	.write = cns3xxx_fake_sgnotify_write,
	.open = cns3xxx_fake_sgnotify_open,
	.poll = cns3xxx_fake_sgnotify_poll,
	.unlocked_ioctl = cns3xxx_fake_sgnotify_ioctl,
	.compat_ioctl = cns3xxx_fake_sgnotify_ioctl,
	.release = cns3xxx_fake_sgnotify_release,
};

static int __init cns3xxx_fake_sgnotify_init(void)
{
        int result;
	result = register_chrdev(CNS3XXX_FAKE_SGNOTIFY_MAJOR, "sgfsnotify",
				 &cns3xxx_fake_sgnotify_fops);
	if (result < 0) {
		printk(KERN_ERR "cns3xxx_fake_sgnotify: cannot register.\n");
		return result;
	}
	return 0;
} /* cns3xxx_fake_sgnotify_init() */

#endif /* CONFIG_CNS3XXX_FAKE_SGNOTIFY_NODE */

/*
 * Initialization
 */
static struct platform_device *cns3420_pdevs[] __initdata = {
        &cns3xxx_rtc_device,        
        &cns3xxx_gpio_device,
	&cns3xxx_leds_device,
        &cns3xxx_spi_controller_device,
//	&cns3420_nor_pdev,
	&cns3xxx_usb_ehci_device,
	&cns3xxx_usb_ohci_device,
	&seagate_net_device,
};

struct proc_dir_entry *cns3xxx_proc_dir;
EXPORT_SYMBOL_GPL(cns3xxx_proc_dir);

static void __init cns3420_init(void)
{
	cns3xxx_l2x0_init();

	printk(KERN_INFO "CNS3420 Kernel specific features enabled: "
#ifdef CONFIG_ARM_64KB_MMU_PAGE_SIZE_SUPPORT
	       "64KB page size, "
#endif
#ifdef CONFIG_SMP
	       "SMP, "
#endif
	       "\n");
	printk(KERN_INFO "Seagate Central: "
#ifdef CONFIG_GPIO_CNS3XXX
	       "GPIO, "
#endif

#ifdef CONFIG_LEDS_CNS3XXX
	       "LEDs, "
#endif

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
	       "Dual uboot env, "
#endif

#ifdef CONFIG_CNS3XXX_ETH
	       "Ethernet, "
#ifdef CONFIG_CNS3XXX_ETHADDR_IN_FLASH
	       "MAC addr flash, "
#endif
#endif

#ifdef CONFIG_RTC_DRV_CNS3XXX
	       "RTC, "
#ifdef CONFIG_RTC_DRV_CNS3XXX_LOG_IN_FLASH
	       "RTC log flash, "
#endif
#endif	       

#ifdef CONFIG_SPI_CNS3XXX
	       "SPI, "
#endif

#ifdef CONFIG_CNS3XXX_FAKE_SGNOTIFY_NODE
	       "Fake sgnotify, "
#endif
	       "\n");
	
	platform_add_devices(cns3420_pdevs, ARRAY_SIZE(cns3420_pdevs));

	cns3xxx_ahci_init();
#ifndef CONFIG_MACH_SEAGATE_CENTRAL
	/*
	 * Enabling sdhci seems to stop USB working on 
	 * Segate central due to modification of the gpio
 	 * shared pins. Disable for now as Seagate Central
	 * doesn't seem to use sdhci functionality
	 */
	printk (KERN_WARN "%s  NOTE : ENABLING SDHCI. THIS WILL DISABLE USB ON SEAGATE CENTRAL!!! \n", __FUNCTION__);
	cns3xxx_sdhci_init();
#endif
        spi_register_board_info(cns3xxx_spi_devices,
				ARRAY_SIZE(cns3xxx_spi_devices));

	pm_power_off = cns3xxx_power_off;

	cns3xxx_proc_dir = proc_mkdir("cns3xxx", NULL);
#ifdef CONFIG_DEBUG_FS 
	cns3xxx_debugfs_init();
#endif /* CONFIG_DEBUG_FS */

#ifdef CONFIG_CNS3XXX_FAKE_SGNOTIFY_NODE
	cns3xxx_fake_sgnotify_init();
#endif
	
}

static struct map_desc cns3420_io_desc[] __initdata = {
	{
		.virtual	= CNS3XXX_UART0_BASE_VIRT,
		.pfn		= __phys_to_pfn(CNS3XXX_UART0_BASE),
		.length         = PAGE_SIZE,
		.type		= MT_DEVICE,
	},
};


static void __init cns3420_map_io(void)
{
	cns3xxx_map_io();
	iotable_init(cns3420_io_desc, ARRAY_SIZE(cns3420_io_desc));

	cns3420_early_serial_setup();
}

MACHINE_START(CNS3420VB, "Seagate CNS3420 NAS")
	.smp		= smp_ops(cns3xxx_smp_ops),
	.atag_offset	= 0x100,
	.map_io		= cns3420_map_io,
	.init_irq	= cns3xxx_init_irq,
	.init_time	= cns3xxx_timer_init,
	.init_machine	= cns3420_init,
	.init_late      = cns3xxx_pcie_init_late,
	.restart	= cns3xxx_restart,
MACHINE_END
