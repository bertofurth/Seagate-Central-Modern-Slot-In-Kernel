/*
 * Copyright 2008 Cavium Networks
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/platform_data/cns3xxx.h>
#include <mach/pm.h>

static int cns3xxx_ehci_init(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval;
	printk("\n %s %s \n", __FILE__,  __FUNCTION__);
	/*
	 * EHCI and OHCI share the same clock and power,
	 * resetting twice would cause the 1st controller been reset.
	 * Therefore only do power up  at the first up device, and
	 * power down at the last down device.
	 *
	 * Set USB AHB INCR length to 16
	 */
/* scottpatch */
//	if (atomic_inc_return(&usb_pwr_ref) == 1) {
//		cns3xxx_pwr_power_up(1 << PM_PLL_HM_PD_CTRL_REG_OFFSET_PLL_USB);
//		cns3xxx_pwr_clk_en(1 << PM_CLK_GATE_REG_OFFSET_USB_HOST);
//		cns3xxx_pwr_soft_rst(1 << PM_SOFT_RST_REG_OFFST_USB_HOST);
//		__raw_writel((__raw_readl(MISC_CHIP_CONFIG_REG) | (0X2 << 24)),
//			MISC_CHIP_CONFIG_REG);
//	}

/*
 *     Power up ,Enable Clock and de-assert reset of USB2.0 port
 */
	cns3xxx_pwr_power_up(1<<PM_PLL_HM_PD_CTRL_REG_OFFSET_PLL_USB);//USB and OTG PLL  on
	cns3xxx_pwr_clk_en(1<<PM_CLK_GATE_REG_OFFSET_USB_HOST);
	cns3xxx_pwr_soft_rst(1<<PM_SOFT_RST_REG_OFFST_USB_HOST);

	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs
	    + HC_LENGTH(ehci, ehci_readl(ehci, &ehci->caps->hc_capbase));
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	hcd->has_tt = 0;
	ehci_reset(ehci);

	retval = ehci_init(hcd);
	if (retval)
		return retval;

#ifdef CONFIG_ARM_64KB_MMU_PAGE_SIZE_SUPPORT
//	ehci_writel(ehci, 0x00040004, hcd->regs + 0x94); //KH: modify from 80 to 03
	ehci_writel(ehci, 0x00800060, hcd->regs + 0x94); //Jacky-2011-0121: Fix Seagate issue for up-nas-64k
        printk("%s,***Threshold OUT=0x80,IN=0x60 ***\n",__func__);
#else
	ehci_writel(ehci, 0x00600060, hcd->regs + 0x94); //Jacky-20100915: fix Seagate FreeeAgent go 320G issue
        printk("%s,***Threshold OUT=0x60,IN=0x60 ***\n",__func__);
#endif




        // write USB AHB INCR length from 4 to 16
	// This is MISC_CHIP_CONFIG_REG. Already done..
        writel( (readl(((u32 *)CNS3XXX_MISC_BASE_VIRT+0x04))| (0X2<<24)), (u32 *)(CNS3XXX_MISC_BASE_VIRT+0x04));//Jacky-20100921: INCR4-->INCR16
//	ehci_port_power(ehci, 0);
	
	return retval;
}

static const struct hc_driver cns3xxx_ehci_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "CNS3XXX EHCI Host Controller",
	.hcd_priv_size		= sizeof(struct ehci_hcd),
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,
	.reset			= cns3xxx_ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,
	.get_frame_number	= ehci_get_frame,
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
#ifdef CONFIG_PM
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
#endif
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

static int cns3xxx_ehci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct usb_hcd *hcd;
	const struct hc_driver *driver = &cns3xxx_ehci_hc_driver;
	struct resource *res;
	int irq;
	int retval;
	printk("\n %s %s \n", __FILE__,  __FUNCTION__);
	if (usb_disabled())
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "Found HC with no IRQ.\n");
		return -ENODEV;
	}
	irq = res->start;

	hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Found HC with no register addr.\n");
		retval = -ENODEV;
		goto err1;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = res->end - res->start + 1;

#ifdef CNS3XXX_USB_BASE_VIRT
	hcd->regs = (void __iomem *) CNS3XXX_USB_BASE_VIRT;
#else
	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len,
				driver->description)) {
		dev_dbg(dev, "controller already in use\n");
		retval = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (hcd->regs == NULL) {
		dev_dbg(dev, "error mapping memory\n");
		retval = -EFAULT;
		goto err2;
	}
#endif
	retval = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (retval == 0)
		return retval;

	
#ifndef CNS3XXX_USB_BASE_VIRT
	cns3xxx_iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
#endif
err1:
	usb_put_hcd(hcd);

	return retval;
}

static int cns3xxx_ehci_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
#ifndef CNS3XXX_USB_BASE_VIRT
	cns3xxx_iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
#endif

	/*
	 * EHCI and OHCI share the same clock and power,
	 * resetting twice would cause the 1st controller been reset.
	 * Therefore only do power up  at the first up device, and
	 * power down at the last down device.
	 */
/* scottpatch */
//	if (atomic_dec_return(&usb_pwr_ref) == 0);
//		cns3xxx_pwr_clk_dis(1 << PM_CLK_GATE_REG_OFFSET_USB_HOST);

	usb_put_hcd(hcd);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

MODULE_ALIAS("platform:cns3xxx-ehci");

static const struct of_device_id ehci_cns3xxx_of_match[] = {
    {.compatible = "cavium,cns3xxx-ehci",},
    {},
};
MODULE_DEVICE_TABLE(of, ehci_hcd_cns3xxx_of_match);

static struct platform_driver cns3xxx_ehci_driver = {
	.probe = cns3xxx_ehci_probe,
	.remove = cns3xxx_ehci_remove,
	.shutdown = usb_hcd_platform_shutdown,
	.driver = {
		.name = "cns3xxx-ehci",
		.owner = THIS_MODULE,
		.of_match_table = ehci_cns3xxx_of_match,
		.bus = &platform_bus_type,
	},
};
