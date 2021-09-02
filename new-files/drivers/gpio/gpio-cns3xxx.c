/*
 * Copyright 2012 Gateworks Corporation
 *     Chris Lang <clang@gateworks.com>
 *     Tim Harvey <tharvey@gateworks.com>
 *
 * Copyright (c) 2011 Cavium
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/platform_data/cns3xxx.h>
#include <linux/platform_data/gpio-cns3xxx.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/mach/irq.h>


/*
 * Registers
 */
#define GPIO_INPUT                          0x04
#define GPIO_DIR                            0x08
#define GPIO_SET                            0x10
#define GPIO_CLEAR                          0x14
#define GPIO_INTERRUPT_ENABLE               0x20
#define GPIO_INTERRUPT_RAW_STATUS           0x24
#define GPIO_INTERRUPT_MASKED_STATUS        0x28
#define GPIO_INTERRUPT_MASK                 0x2C
#define GPIO_INTERRUPT_CLEAR                0x30
#define GPIO_INTERRUPT_TRIGGER_METHOD       0x34
#define GPIO_INTERRUPT_TRIGGER_BOTH_EDGES   0x38
#define GPIO_INTERRUPT_TRIGGER_TYPE         0x3C

#define GPIO_INTERRUPT_TRIGGER_METHOD_EDGE  0
#define GPIO_INTERRUPT_TRIGGER_METHOD_LEVEL 1
#define GPIO_INTERRUPT_TRIGGER_EDGE_SINGLE  0
#define GPIO_INTERRUPT_TRIGGER_EDGE_BOTH    1
#define GPIO_INTERRUPT_TRIGGER_TYPE_RISING  0
#define GPIO_INTERRUPT_TRIGGER_TYPE_FALLING 1
#define GPIO_INTERRUPT_TRIGGER_TYPE_HIGH    0
#define GPIO_INTERRUPT_TRIGGER_TYPE_LOW     1

struct cns3xxx_gpio_chip {
	struct gpio_chip    chip;
	struct irq_domain   *domain;
	spinlock_t          lock;
	void __iomem        *base;
        void __iomem        *reg_sharepin_en;
        void __iomem        *reg_pud;
        struct device       *dev;
};

static struct cns3xxx_gpio_chip cns3xxx_gpio_chips[2];
static int cns3xxx_gpio_chip_count;

struct cns3xxx_regs {
    const char *name;
    volatile unsigned int *addr;
    u32 offset;
};

struct cns3xxx_regs gpio_regs[] =  {
    {"Data Out",                    0, GPIO_OUTPUT_OFFSET},
    {"Data In",                     0, GPIO_INPUT_OFFSET},
    {"Direction",                   0, GPIO_DIR_OFFSET},
    {"Interrupt Enable",            0, GPIO_INTR_ENABLE_OFFSET},
    {"Interrupt Raw Status",        0, GPIO_INTR_RAW_STATUS_OFFSET},
    {"Interrupt Masked Status",     0, GPIO_INTR_MASKED_STATUS_OFFSET},
    {"Interrupt Level Trigger",     0, GPIO_INTR_TRIGGER_METHOD_OFFSET},
    {"Interrupt Both Edge",         0, GPIO_INTR_TRIGGER_BOTH_EDGES_OFFSET},
    {"Interrupt Falling Edge",      0, GPIO_INTR_TRIGGER_TYPE_OFFSET},
    {"Interrupt MASKED",            0, GPIO_INTR_MASK_OFFSET},
    {"GPIO Bounce Enable",          0, GPIO_BOUNCE_ENABLE_OFFSET},
    {"GPIO Bounce Prescale",        0, GPIO_BOUNCE_PRESCALE_OFFSET},
};

struct cns3xxx_regs misc_regs[] =  {
    {"Drive Strength Register A",   &MISC_IO_PAD_DRIVE_STRENGTH_CTRL_A},
    {"Drive Strength Register B",   &MISC_IO_PAD_DRIVE_STRENGTH_CTRL_B},
    {"Pull Up/Down Ctrl A[15:0]",   &MISC_GPIOA_15_0_PULL_CTRL_REG},
    {"Pull Up/Down Ctrl A[31:16]",  &MISC_GPIOA_16_31_PULL_CTRL_REG},
    {"Pull Up/Down Ctrl B[15:0]",   &MISC_GPIOB_15_0_PULL_CTRL_REG},
    {"Pull Up/Down Ctrl B[31:16]",  &MISC_GPIOB_16_31_PULL_CTRL_REG},
};

/* The CNS3XXX GPIO pins are shard with special functions which is described in
 * the following table. "none" in this table represent the corresponding pins
 * are dedicate GPIO.
 */
const char *sharepin_desc[] = {
    /* GPIOA group */
    /*  0 */ "none",        "none",         "SD_PWR_ON",    "OTG_DRV_VBUS",
    /*  4 */ "Don't use",   "none",         "none",         "none",
    /*  8 */ "CIM_nOE",     "LCD_Power",    "SMI_nCS3",     "SMI_nCS2",
    /* 12 */ "SMI_Clk",     "SMI_nADV",     "SMI_CRE",      "SMI_Addr[26]",
    /* 16 */ "SD_nCD",      "SD_nWP",       "SD_CLK",       "SD_CMD",
    /* 20 */ "SD_DT[7]",    "SD_DT[6]",     "SD_DT[5]",     "SD_DT[4]",
    /* 24 */ "SD_DT[3]",    "SD_DT[2]",     "SD_DT[1]",     "SD_DT[0]",
    /* 28 */ "SD_LED",      "UR_RXD1",      "UR_TXD1",      "UR_RTS2",
    /* GPIOB group */
    /*  0 */ "UR_CTS2",     "UR_RXD2",      "UR_TXD2",      "PCMCLK",
    /*  4 */ "PCMFS",       "PCMDT",        "PCMDR",        "SPInCS1",
    /*  8 */ "SPInCS0",     "SPICLK",       "SPIDT",        "SPIDR",
    /* 12 */ "SCL",         "SDA",          "GMII2_CRS",    "GMII2_COL",
    /* 16 */ "RGMII1_CRS",  "RGMII1_COL",   "RGMII0_CRS",   "RGMII0_COL",
    /* 20 */ "MDC",         "MDIO",         "I2SCLK",       "I2SFS",
    /* 24 */ "I2SDT",       "I2SDR",        "ClkOut",       "Ext_Intr2",
    /* 28 */ "Ext_Intr1",   "Ext_Intr0",    "SATA_LED1",    "SATA_LED0",
};

static struct proc_dir_entry    *proc_cns3xxx_gpio;

static int cns3xxx_gpio_read_proc(struct seq_file *s, void *unused)
{

    int i, nr_regs;

    nr_regs = ARRAY_SIZE(gpio_regs);
    seq_printf(s,   "Register Base               0x%px     0x%px\n",
	       cns3xxx_gpio_chips[0].base, cns3xxx_gpio_chips[1].base);
    seq_printf(s,
	                               "Register Description        GPIOA     GPIOB\n"
	       "====================        =====     =====\n");
    seq_printf(s,   "%-26.26s: %08x  %08x\n", "GPIO Disable",
	       readl(cns3xxx_gpio_chips[0].reg_sharepin_en),
	       readl(cns3xxx_gpio_chips[1].reg_sharepin_en));
    for (i = 0; i < nr_regs; i++) {
	seq_printf(s, "%-26.26s: %08x  %08x\n",
		   gpio_regs[i].name,
		   readl(cns3xxx_gpio_chips[0].base + gpio_regs[i].offset),
		   readl(cns3xxx_gpio_chips[1].base + gpio_regs[i].offset));
    }

    seq_printf(s, "\n"
	                               "Register Description        Value\n"
	       "====================        =====\n");
    nr_regs = ARRAY_SIZE(misc_regs);
    for (i = 0; i < cns3xxx_gpio_chip_count; i++) {
	seq_printf(s, "%-26.26s: %08x\n",
		   misc_regs[i].name,
		   *misc_regs[i].addr);
    }
    return 0;
}

#ifdef CONFIG_DEBUG_FS

#include <linux/debugfs.h>

const char *pull_state[] = {
    "--",
    "PD",
    "PU",
            ""
};

static int get_bits(void __iomem *addr, int shift)
{
    u32 val;

    if (!addr)
	return 0;

    if (shift < 16)
	val = readl(addr);
    else {
	shift -= 16;
	val = readl(addr + 4);
    }

    return (val >> (shift * 2)) & 0x03;
}

#define header0 "           Label                 Mode      Dir Val\n"
#define header1 " ====================================================\n"
#define format0 " %s_%-3d [%-20.20s] %-10.10s%s %s  %s\n"

static int cns3xxx_dbg_gpio_show_all(struct seq_file *s, void *unused)
{
    int i, j, is_out, disabled;
    unsigned gpio, reg;
    const char *gpio_label;
    struct gpio_chip *chip;
    int bits;

    for (j = 0; j < cns3xxx_gpio_chip_count; j++) {
	chip = &cns3xxx_gpio_chips[j].chip;
	reg = readl(cns3xxx_gpio_chips[j].reg_sharepin_en);
	seq_printf(s, "GPIO %d Sharepin Register at 0x%px  Value 0x%08x\n",
		   j, cns3xxx_gpio_chips[j].reg_sharepin_en, reg);
	seq_printf(s, header0);
	seq_printf(s, header1);
	for (i = 0; i < chip->ngpio; i++) {
	    gpio = chip->base + i;
	    disabled = test_bit(i, cns3xxx_gpio_chips[j].reg_sharepin_en);
	    gpio_label = gpiochip_is_requested(chip, i);
	    if (!gpio_label) {
		if (disabled)
		    gpio_label = sharepin_desc[gpio];
		else
		    gpio_label = "";
	    }
	    is_out = test_bit(i, cns3xxx_gpio_chips[j].base + GPIO_DIR_OFFSET);
	    bits = get_bits(cns3xxx_gpio_chips[j].reg_pud, i);
	    seq_printf(s, format0, chip->label, i, gpio_label,
		       disabled ? "Function" : "GPIO",
		       is_out ? "out" : "in ",
		       chip->get(chip, i) ? "hi" : "lo",
		       pull_state[bits]);
	}
	seq_printf(s, "\n");
    }

    return 0;
}


static int dbg_gpio_open(struct inode *inode, struct file *file)
{
    return single_open(file, cns3xxx_dbg_gpio_show_all, &inode->i_private);
}

static const struct file_operations debug_fops = {
    .open           = dbg_gpio_open,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int __init cns3xxx_gpio_debuginit(void)
{
    debugfs_create_file("gpio-cns3xxx", S_IRUGO, NULL, NULL, &debug_fops);
    return 0;
}
late_initcall(cns3xxx_gpio_debuginit);

#endif /* CONFIG_DEBUG_FS */




/*
 * Turn off corresponding share pin function.
 */
void cns3xxx_sharepin_free(unsigned gpio)
{
    struct gpio_chip *chip;
    int i, reg, offset = gpio;

    gpio_free(gpio);

    for (i = 0; i < cns3xxx_gpio_chip_count; i++) {
	chip = &cns3xxx_gpio_chips[i].chip;
	if (offset >= chip->ngpio) {
	    offset -= chip->ngpio;
	    continue;
	}
	spin_lock(&cns3xxx_gpio_chips[i].lock);
	reg = readl(cns3xxx_gpio_chips[i].reg_sharepin_en);
	reg &= ~(1 << offset);
	writel(reg, cns3xxx_gpio_chips[i].reg_sharepin_en);
	spin_unlock(&cns3xxx_gpio_chips[i].lock);
	pr_debug(KERN_INFO "%s[%d] share pin function (%s) disabled!\n",
		 chip->label, offset, sharepin_desc[gpio]);
	break;
    }
}
EXPORT_SYMBOL(cns3xxx_sharepin_free);

/**
 * cns3xxx_sharepin_free_array - release multiple GPIOs in a single call
 * @array:      array of the 'struct gpio'
 * @num:        how many GPIOs in the array
 */
void cns3xxx_sharepin_free_array(struct gpio *array, size_t num)
{
    while (num--)
	cns3xxx_sharepin_free((array++)->gpio);
}

EXPORT_SYMBOL_GPL(cns3xxx_sharepin_free_array);



static inline void
__set_direction(struct cns3xxx_gpio_chip *cchip, unsigned pin, int input)
{
	u32 reg;

	reg = __raw_readl(cchip->base + GPIO_DIR);
	if (input)
		reg &= ~(1 << pin);
	else
		reg |= (1 << pin);
	__raw_writel(reg, cchip->base + GPIO_DIR);
}

/*
 * GENERIC_GPIO primatives
 */
static int cns3xxx_gpio_direction_input(struct gpio_chip *chip, unsigned pin)
{
	struct cns3xxx_gpio_chip *cchip =
		container_of(chip, struct cns3xxx_gpio_chip, chip);
	unsigned long flags;

	spin_lock_irqsave(&cchip->lock, flags);
	__set_direction(cchip, pin, 1);
	spin_unlock_irqrestore(&cchip->lock, flags);

	return 0;
}

static int cns3xxx_gpio_get(struct gpio_chip *chip, unsigned pin)
{
	struct cns3xxx_gpio_chip *cchip =
		container_of(chip, struct cns3xxx_gpio_chip, chip);
	int val;

	val = ((__raw_readl(cchip->base + GPIO_INPUT) >> pin) & 0x1);

	return val;
}

static int cns3xxx_gpio_direction_output(struct gpio_chip *chip, unsigned pin, int level)
{
	struct cns3xxx_gpio_chip *cchip =
		container_of(chip, struct cns3xxx_gpio_chip, chip);
	unsigned long flags;

	spin_lock_irqsave(&cchip->lock, flags);
	if (level)
		__raw_writel(1 << pin, cchip->base + GPIO_SET);
	else
		__raw_writel(1 << pin, cchip->base + GPIO_CLEAR);
	__set_direction(cchip, pin, 0);
	spin_unlock_irqrestore(&cchip->lock, flags);

	return 0;
}

void cns3xxx_gpio_set(struct gpio_chip *chip, unsigned pin,
	int level)
{
	struct cns3xxx_gpio_chip *cchip =
		container_of(chip, struct cns3xxx_gpio_chip, chip);

	if (level)
		__raw_writel(1 << pin, cchip->base + GPIO_SET);
	else
		__raw_writel(1 << pin, cchip->base + GPIO_CLEAR);
}

/**
 * Same functionality as cns3xxx_gpio_set.
 * It is meant to be called from other modules.
 * port: 0 - GPIOA or 1 - GPIOB.
 * bit: between 0-31
 * value: 0 or 1
 */

#define VALIDATE_PORT_AND_BIT(port, bit)                \
    if (port > ARRAY_SIZE(cns3xxx_gpio_chips)) {	\
    printk(KERN_INFO "Invalid port: %u\n", port);   \
    return -EINVAL;                                 \
    }                                                   \
                                                            \
    if (bit > 31) {                                     \
    printk(KERN_INFO "Invalid bit: %u\n", bit);     \
    return -EINVAL;                                 \
    }

int cns3xxx_gpio_port_set(unsigned port, unsigned bit, int value)
{
    VALIDATE_PORT_AND_BIT(port, bit);

    cns3xxx_gpio_set(&cns3xxx_gpio_chips[port].chip, bit, value);

    return 0;
}

EXPORT_SYMBOL(cns3xxx_gpio_set);

/**
 * Same functionality as cns3xxx_gpio_direction_out.
 * It is meant to be called from other modules.
 * port: 0 - GPIOA or 1 - GPIOB.
 * bit: between 0-31
 * value: 0 or 1
 */
int cns3xxx_gpio_port_direction_out(unsigned port, unsigned bit, int value)
{
    VALIDATE_PORT_AND_BIT(port, bit);

    return cns3xxx_gpio_direction_output(&cns3xxx_gpio_chips[port].chip, bit, value);
}

EXPORT_SYMBOL(cns3xxx_gpio_port_direction_out);

static int cns3xxx_gpio_to_irq(struct gpio_chip *chip, unsigned pin)
{
	struct cns3xxx_gpio_chip *cchip =
		container_of(chip, struct cns3xxx_gpio_chip, chip);

	return irq_find_mapping(cchip->domain, pin);
}


/*
 * Turn on corresponding shared pin function.
 * Turn on shared pin function will also disable GPIO function. Related GPIO
 * control registers are still accessable but not reflect to pin.
 */
int cns3xxx_sharepin_request(unsigned gpio, const char *label)
{
    struct gpio_chip *chip;
    int i, reg, ret, offset = gpio;

    if (!label)
	label = sharepin_desc[gpio];

    ret = gpio_request(gpio, label);
    if (ret) {
	printk(KERN_INFO "gpio-%d already in use! err=%d\n", gpio, ret);
	return ret;
    }

    for (i = 0; i < cns3xxx_gpio_chip_count; i++) {
	chip = &cns3xxx_gpio_chips[i].chip;
	if (offset >= chip->ngpio) {
	    offset -= chip->ngpio;
	    continue;
	}
	spin_lock(&cns3xxx_gpio_chips[i].lock);
	reg = readl(cns3xxx_gpio_chips[i].reg_sharepin_en);
	reg |= (1 << offset);
	writel(reg, cns3xxx_gpio_chips[i].reg_sharepin_en);
	spin_unlock(&cns3xxx_gpio_chips[i].lock);
	pr_debug("%s[%d] is occupied by %s function!\n",
		 chip->label, offset, label);
	break;
    }

    return 0;
}

/**
 * cns3xxx_sharepin_request_array - request multiple GPIOs in a single call
 * @array:      array of the 'struct gpio'
 * @num:        how many GPIOs in the array
 */
int cns3xxx_sharepin_request_array(struct gpio *array, size_t num)
{
    int i, err;

    for (i = 0; i < num; i++, array++) {
	err = cns3xxx_sharepin_request(array->gpio, array->label);
	if (err)
	    goto err_free;
	if (array->flags & GPIOF_DIR_IN)
	    err = gpio_direction_input(array->gpio);
	else
	    err = gpio_direction_output(array->gpio,
					(array->flags & GPIOF_INIT_HIGH) ? 1 : 0);
	if (err)
	    goto err_free;
    }
    return 0;

err_free:
    while (i--)
	cns3xxx_sharepin_free((--array)->gpio);
    return err;
}
EXPORT_SYMBOL_GPL(cns3xxx_sharepin_request_array);



/*
 * IRQ support
 */

/* one interrupt per GPIO controller (GPIOA/GPIOB)
 * this is called in task context, with IRQs enabled
 */
static void cns3xxx_gpio_irq_handler(struct irq_desc *desc)
{
	struct cns3xxx_gpio_chip *cchip = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	u16 i;
	u32 reg;

	chained_irq_enter(chip, desc); /* mask and ack the base interrupt */

	/* see which pin(s) triggered the interrupt */
	reg = __raw_readl(cchip->base + GPIO_INTERRUPT_RAW_STATUS);
	for (i = 0; i < 32; i++) {
		if (reg & (1 << i)) {
			/* let the generic IRQ layer handle an interrupt */
			generic_handle_irq(irq_find_mapping(cchip->domain, i));
		}
	}

	chained_irq_exit(chip, desc); /* unmask the base interrupt */
}

static int cns3xxx_gpio_irq_set_type(struct irq_data *d, u32 irqtype)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct cns3xxx_gpio_chip *cchip = gc->private;
	u32 gpio = d->hwirq;
	unsigned long flags;
	u32 method, edges, type;

	spin_lock_irqsave(&cchip->lock, flags);
	method = __raw_readl(cchip->base + GPIO_INTERRUPT_TRIGGER_METHOD);
	edges  = __raw_readl(cchip->base + GPIO_INTERRUPT_TRIGGER_BOTH_EDGES);
	type   = __raw_readl(cchip->base + GPIO_INTERRUPT_TRIGGER_TYPE);
	method &= ~(1 << gpio);
	edges  &= ~(1 << gpio);
	type   &= ~(1 << gpio);

	switch(irqtype) {
	case IRQ_TYPE_EDGE_RISING:
		method |= (GPIO_INTERRUPT_TRIGGER_METHOD_EDGE << gpio);
		edges  |= (GPIO_INTERRUPT_TRIGGER_EDGE_SINGLE << gpio);
		type   |= (GPIO_INTERRUPT_TRIGGER_TYPE_RISING << gpio);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		method |= (GPIO_INTERRUPT_TRIGGER_METHOD_EDGE << gpio);
		edges  |= (GPIO_INTERRUPT_TRIGGER_EDGE_SINGLE << gpio);
		type   |= (GPIO_INTERRUPT_TRIGGER_TYPE_FALLING << gpio);
		break;
	case IRQ_TYPE_EDGE_BOTH:
		method |= (GPIO_INTERRUPT_TRIGGER_METHOD_EDGE << gpio);
		edges  |= (GPIO_INTERRUPT_TRIGGER_EDGE_BOTH << gpio);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		method |= (GPIO_INTERRUPT_TRIGGER_METHOD_LEVEL << gpio);
		type   |= (GPIO_INTERRUPT_TRIGGER_TYPE_LOW << gpio);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		method |= (GPIO_INTERRUPT_TRIGGER_METHOD_LEVEL << gpio);
		type   |= (GPIO_INTERRUPT_TRIGGER_TYPE_HIGH << gpio);
		break;
	default:
		printk(KERN_WARNING "No irq type\n");
		spin_unlock_irqrestore(&cchip->lock, flags);
		return -EINVAL;
	}

	__raw_writel(method, cchip->base + GPIO_INTERRUPT_TRIGGER_METHOD);
	__raw_writel(edges,  cchip->base + GPIO_INTERRUPT_TRIGGER_BOTH_EDGES);
	__raw_writel(type,   cchip->base + GPIO_INTERRUPT_TRIGGER_TYPE);
	spin_unlock_irqrestore(&cchip->lock, flags);

	
	if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH))
		irq_set_handler_locked(d, handle_level_irq);
	else if (type & (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING))
		irq_set_handler_locked(d, handle_edge_irq);

	return 0;
}

void __init cns3xxx_gpio_init(int gpio_base, int ngpio, u32 base,
			      int irq, int secondary_irq_base, struct device *dev)
{
	struct cns3xxx_gpio_chip *cchip;
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;
	char gc_label[16];
	int irq_base, ret;
	
	if (cns3xxx_gpio_chip_count == ARRAY_SIZE(cns3xxx_gpio_chips))
		return;

	snprintf(gc_label, sizeof(gc_label), "cns3xxx_gpio%d",
		cns3xxx_gpio_chip_count);

	cchip = cns3xxx_gpio_chips + cns3xxx_gpio_chip_count;
	cchip->dev = dev;
	cchip->chip.label = kstrdup(gc_label, GFP_KERNEL);
	cchip->chip.direction_input = cns3xxx_gpio_direction_input;
	cchip->chip.get = cns3xxx_gpio_get;
	cchip->chip.direction_output = cns3xxx_gpio_direction_output;
	cchip->chip.set = cns3xxx_gpio_set;
	cchip->chip.to_irq = cns3xxx_gpio_to_irq;
	cchip->chip.base = gpio_base;
	cchip->chip.ngpio = ngpio;
	cchip->chip.can_sleep = 0;
	spin_lock_init(&cchip->lock);
	cchip->base = (void __iomem *)base;
	cchip->reg_sharepin_en = (void __iomem *)CNS3XXX_MISC_BASE_VIRT +
	    (cns3xxx_gpio_chip_count * 4) + MISC_GPIOA_PIN_DISABLE_OFFSET;
	cchip->reg_pud = (void __iomem *)CNS3XXX_MISC_BASE_VIRT +
	    (cns3xxx_gpio_chip_count * 8) + MISC_GPIOA_15_0_PULL_CTRL_OFFSET;
	
	BUG_ON(gpiochip_add(&cchip->chip) < 0);
	cns3xxx_gpio_chip_count++;

	/* clear GPIO interrupts */
	__raw_writel(0xffff, cchip->base + GPIO_INTERRUPT_CLEAR);

	irq_base = irq_alloc_descs(-1, secondary_irq_base, ngpio,
		numa_node_id());

	if (irq_base < 0)
		goto out_irqdesc_free;

	cchip->domain = irq_domain_add_legacy(dev->of_node, ngpio, irq_base, 0,
		&irq_domain_simple_ops, NULL);
	if (!cchip->domain)
		goto out_irqdesc_free;

	/*
	 * IRQ chip init
	 */
	gc = devm_irq_alloc_generic_chip(cchip->dev, KBUILD_MODNAME, 1,
					 irq_base,
					 cchip->base, handle_edge_irq); 
	gc->private = cchip;

	ct = gc->chip_types;
	ct->type = IRQ_TYPE_EDGE_FALLING; 
 	ct->regs.ack = GPIO_INTERRUPT_CLEAR;
	ct->chip.irq_ack = irq_gc_ack_set_bit;
	ct->regs.mask = GPIO_INTERRUPT_ENABLE;
	ct->chip.irq_enable = irq_gc_mask_set_bit;
	ct->chip.irq_disable = irq_gc_mask_clr_bit;
	ct->chip.irq_set_type = cns3xxx_gpio_irq_set_type;
	ct->handler = handle_edge_irq;

	ret = devm_irq_setup_generic_chip(cchip->dev, gc,
					  IRQ_MSK(ngpio), IRQ_GC_INIT_MASK_CACHE,
					  IRQ_NOREQUEST | IRQ_NOPROBE, 0);
	if (ret) {
	    printk (KERN_INFO "Unable to establish gpio interrupts for %s. Return Code %u\n",
		    gc_label, ret);
	    return;
	}

	irq_set_chained_handler(irq, cns3xxx_gpio_irq_handler);
	irq_set_handler_data(irq, cchip);

	return;

out_irqdesc_free:
	irq_free_descs(irq_base, ngpio);
}



static int cns3xxx_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, cns3xxx_gpio_read_proc, &inode->i_private);
}

static const struct proc_ops cns3xxx_gpio_fops = {
    .proc_open           = cns3xxx_proc_open,
    .proc_read           = seq_read,
    .proc_lseek          = seq_lseek,
    .proc_release        = single_release,
};

static int __init _gpio_probe(struct platform_device *pdev)
{
      struct device *dev = &pdev->dev;
//    struct resource *res;
//    void __iomem *misc_reg;
//    int i, j, err, nr_gpios = 0, irq = 0;

    cns3xxx_pwr_clk_en(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);
    cns3xxx_pwr_soft_rst(0x1 << PM_CLK_GATE_REG_OFFSET_GPIO);

    
    cns3xxx_gpio_init(0, 32, CNS3XXX_GPIOA_BASE_VIRT, IRQ_CNS3XXX_GPIOA,
		      NR_IRQS_CNS3XXX, dev);
    cns3xxx_gpio_init(32, 32, CNS3XXX_GPIOB_BASE_VIRT, IRQ_CNS3XXX_GPIOB,
		      NR_IRQS_CNS3XXX + 32, dev);
    if (cns3xxx_proc_dir) {
	proc_cns3xxx_gpio = proc_create("gpio", S_IFREG | S_IRUGO,
					cns3xxx_proc_dir, &cns3xxx_gpio_fops);
    }

    return 0;
}

struct platform_driver cns3xxx_gpio_driver __refdata = {
    .probe          = _gpio_probe,
    .driver         = {
	.owner  = THIS_MODULE,
	.name   = "cns3xxx-gpio",
    },
};

EXPORT_SYMBOL(cns3xxx_gpio_driver);
module_platform_driver(cns3xxx_gpio_driver);


MODULE_AUTHOR("Cavium Networks");
MODULE_DESCRIPTION("CNS3XXX GPIO Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cns3xxx-gpio");
