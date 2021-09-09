/* drivers/leds/leds-cns3xxx.c
 * CNS3XXX - LEDs GPIO driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <asm/mach/irq.h>
#include <asm/uaccess.h>

#include <linux/platform_data/gpio-cns3xxx.h>
#include <linux/platform_data/cns3xxx.h>


/*
 * Note that the colors of the LEDs may not match
 * the actual color. For example, on a Seagate Central
 * the "yellow" LED is actually red.
 */
#define GPIO_PROC_NAME          "leds"
#define GREEN_LED               0
#define YELLOW_LED              1
#define BLINK_PERIOD_MS         500

enum {
    LS_OFF,
    LS_SOLID_GREEN,
    LS_SOLID_YELLOW,
    LS_SOLID_GREEN_AND_YELLOW,
    LS_BLINKING_GREEN,
    LS_BLINKING_YELLOW,
    LS_BLINKING_GREEN_AND_YELLOW,
    /* N.B. States below are not in the original firmware */
    LS_BLINKING_ALTERNATE,
    LS_BLINKING_GREEN_SLOW,
    LS_BLINKING_YELLOW_SLOW,
    LS_BLINKING_GREEN_AND_YELLOW_SLOW,
    LS_BLINKING_ALTERNATE_SLOW,
    LS_NO
};

static struct proc_dir_entry *proc_cns3xxx_leds;
static struct timer_list cns3xx_leds_timer;
static char cns3xxx_leds_state_buffer[10];
static int cns3xxx_leds_state = LS_OFF;
static char cns3xxx_blink_flag = 1;
static DEFINE_SPINLOCK(cns3xx_leds_lock);

static long cns3xx_panic_blink(int state)
{
    long delay = 0;
    static char led;

    led = (state) ? 1 : 0;

    led ^= 1;
    cns3xxx_gpio_port_set(0, GREEN_LED, led);
    cns3xxx_gpio_port_set(0, YELLOW_LED, led);

    return delay;
}

static inline void set_led(unsigned int led)
{
    cns3xxx_gpio_port_set(0, led, 1);
}

static inline void clear_led(unsigned int led)
{
    cns3xxx_gpio_port_set(0, led, 0);
}

static inline void start_leds_timer(unsigned int period_ms)
{
    //no need to check the returned value
    mod_timer(&cns3xx_leds_timer, jiffies + msecs_to_jiffies(period_ms));
}

static inline void stop_leds_timer(void)
{
    //no need to check the returned value
    del_timer(&cns3xx_leds_timer);
}

static void cns3xx_leds_timer_callback(struct timer_list *t)
{
    spin_lock(&cns3xx_leds_lock);

    switch(cns3xxx_leds_state) {
    case LS_BLINKING_GREEN:
        cns3xxx_blink_flag ^= 1;
        cns3xxx_gpio_port_set(0, GREEN_LED, cns3xxx_blink_flag);
        start_leds_timer(BLINK_PERIOD_MS);
	break;
    case LS_BLINKING_YELLOW:
	cns3xxx_blink_flag ^= 1;
	cns3xxx_gpio_port_set(0, YELLOW_LED, cns3xxx_blink_flag);
	start_leds_timer(BLINK_PERIOD_MS);
	break;
    case LS_BLINKING_GREEN_AND_YELLOW:
	cns3xxx_blink_flag ^= 1;
	cns3xxx_gpio_port_set(0, GREEN_LED, cns3xxx_blink_flag);
	cns3xxx_gpio_port_set(0, YELLOW_LED, cns3xxx_blink_flag);
	start_leds_timer(BLINK_PERIOD_MS);
	break;
    case LS_BLINKING_ALTERNATE:
	cns3xxx_blink_flag ^= 1;
	cns3xxx_gpio_port_set(0, GREEN_LED, cns3xxx_blink_flag);
	cns3xxx_gpio_port_set(0, YELLOW_LED, !cns3xxx_blink_flag);
	start_leds_timer(BLINK_PERIOD_MS);
	break;
    case LS_BLINKING_GREEN_SLOW:
        cns3xxx_blink_flag ^= 1;
        cns3xxx_gpio_port_set(0, GREEN_LED, cns3xxx_blink_flag);
        start_leds_timer(BLINK_PERIOD_MS * 2);
	break;
    case LS_BLINKING_YELLOW_SLOW:
	cns3xxx_blink_flag ^= 1;
	cns3xxx_gpio_port_set(0, YELLOW_LED, cns3xxx_blink_flag);
	start_leds_timer(BLINK_PERIOD_MS * 2);
	break;
    case LS_BLINKING_GREEN_AND_YELLOW_SLOW:
	cns3xxx_blink_flag ^= 1;
	cns3xxx_gpio_port_set(0, GREEN_LED, cns3xxx_blink_flag);
	cns3xxx_gpio_port_set(0, YELLOW_LED, cns3xxx_blink_flag);
	start_leds_timer(BLINK_PERIOD_MS * 2);
	break;
    case LS_BLINKING_ALTERNATE_SLOW:
	cns3xxx_blink_flag ^= 1;
	cns3xxx_gpio_port_set(0, GREEN_LED, cns3xxx_blink_flag);
	cns3xxx_gpio_port_set(0, YELLOW_LED, !cns3xxx_blink_flag);
	start_leds_timer(BLINK_PERIOD_MS * 2);
	break;
    }
    

    spin_unlock(&cns3xx_leds_lock);
}

static void leds_state_manager(unsigned int new_state)
{
    spin_lock(&cns3xx_leds_lock);

    if (cns3xxx_leds_state == new_state)
    {
        spin_unlock(&cns3xx_leds_lock);
        return;
    }
    cns3xxx_leds_state = new_state;

    //printk(KERN_INFO"current leds state is: %d\n", cns3xxx_leds_state);

    switch(cns3xxx_leds_state)
    {
        case LS_OFF:
            stop_leds_timer( );
            clear_led(GREEN_LED);
            clear_led(YELLOW_LED);
            break;

        case LS_SOLID_GREEN:
            stop_leds_timer( );
            set_led(GREEN_LED);
            clear_led(YELLOW_LED);
            break;

        case LS_SOLID_YELLOW:
            stop_leds_timer( );
            clear_led(GREEN_LED);
            set_led(YELLOW_LED);
            break;

        case LS_SOLID_GREEN_AND_YELLOW:
            stop_leds_timer( );
            set_led(GREEN_LED);
            set_led(YELLOW_LED);
            break;

        case LS_BLINKING_GREEN:
            cns3xxx_blink_flag = 1;
            set_led(GREEN_LED);
            clear_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS);
            break;

        case LS_BLINKING_YELLOW:
            cns3xxx_blink_flag = 1;
            clear_led(GREEN_LED);
            set_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS);
            break;

        case LS_BLINKING_GREEN_AND_YELLOW:
            cns3xxx_blink_flag = 1;
            set_led(GREEN_LED);
            set_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS);
            break;

        case LS_BLINKING_ALTERNATE:
            cns3xxx_blink_flag = 1;
            set_led(GREEN_LED);
            clear_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS);
            break;

        case LS_BLINKING_GREEN_SLOW:
            cns3xxx_blink_flag = 1;
            set_led(GREEN_LED);
            clear_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS * 2);
            break;

        case LS_BLINKING_YELLOW_SLOW:
            cns3xxx_blink_flag = 1;
            clear_led(GREEN_LED);
            set_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS * 2);
            break;

        case LS_BLINKING_GREEN_AND_YELLOW_SLOW:
            cns3xxx_blink_flag = 1;
            set_led(GREEN_LED);
            set_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS * 2);
            break;

        case LS_BLINKING_ALTERNATE_SLOW:
            cns3xxx_blink_flag = 1;
            set_led(GREEN_LED);
            clear_led(YELLOW_LED);
            start_leds_timer(BLINK_PERIOD_MS * 2);
            break;
    }

    spin_unlock(&cns3xx_leds_lock);
}

static int cns3xxx_leds_read_proc(struct seq_file *s, void *unused)
{
    spin_lock(&cns3xx_leds_lock);
    
    
    seq_printf(s, "current state: %d\n", cns3xxx_leds_state);
    seq_printf(s, "0 - off\n");
    seq_printf(s, "1 - solid green\n");
    seq_printf(s, "2 - solid yellow (red)\n");
    seq_printf(s, "3 - solid green and yellow (amber)\n");
    seq_printf(s, "4 - blink green\n");
    seq_printf(s, "5 - blink yellow\n");
    seq_printf(s, "6 - blink green and yellow\n");
    /* N.B. States below are not in the original firmware */
    seq_printf(s, "7 - blink green and yellow alternately\n");
    seq_printf(s, "8 - slow blink green\n");
    seq_printf(s, "9 - slow blink yellow\n");
    seq_printf(s, "10 - slow blink green and yellow\n");
    seq_printf(s, "11 - slow blink green and yellow alternately\n");

    spin_unlock(&cns3xx_leds_lock);

    return 0;
}

int cns3xxx_leds_write_proc(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{
    unsigned int tmp;
    int ret;

    if (count > (sizeof(cns3xxx_leds_state_buffer) - 1)) {
        printk(KERN_ERR"failed to set leds state: invalid count: %u - only values between \"%u\" - \"%u\" are accepted\n", count, LS_OFF, LS_NO - 1);
        return -EINVAL;
    }

    if (copy_from_user(cns3xxx_leds_state_buffer, buffer, count)) {
        printk(KERN_ERR"failed to set leds state: copy_from_user failed\n");
        return -EFAULT;
    }

    cns3xxx_leds_state_buffer[count] = 0x0; /* Ensure null terminator...not very elegant */

    ret = kstrtouint(cns3xxx_leds_state_buffer, 0, &tmp);
    if (ret != 0) {
	    printk(KERN_ERR"failed to convert leds value %s to number\n", cns3xxx_leds_state_buffer);
	    return -EFAULT;
    }
    
    if (tmp < LS_OFF || tmp >= LS_NO)
    {
        printk(KERN_ERR"failed to set leds state: accepted values: \"%u\" - \"%u\" but \"%u\" was received\n", LS_OFF, LS_NO - 1, tmp);
        return -EINVAL;
    }

    leds_state_manager(tmp);

    return count;
}

static int cns3xxx_leds_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, cns3xxx_leds_read_proc, &inode->i_private);
}

static const struct proc_ops cns3xxx_leds_fops = {
    .proc_open           = cns3xxx_leds_open_proc,
    .proc_read           = seq_read,
    .proc_lseek          = seq_lseek,
    .proc_release        = single_release,
    .proc_write          = cns3xxx_leds_write_proc,
};

static int cns3xxx_leds_probe(struct platform_device *pdev)
{
    if (cns3xxx_proc_dir) {
        proc_cns3xxx_leds = proc_create(GPIO_PROC_NAME, S_IFREG | S_IRUGO,
					cns3xxx_proc_dir, &cns3xxx_leds_fops);
    }

    return 0;
}

static const struct of_device_id cns3xxx_leds_match[] = {
    { .compatible = "cavium,cns3xxx_leds", },
    {},
};
MODULE_DEVICE_TABLE(of, cns3xxx_leds_match);

static struct platform_driver cns3xxx_leds_driver = {
    .probe      = cns3xxx_leds_probe,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = "cns3xxx_leds",
    },
};

static char banner[] __initdata =
    KERN_INFO "CNS3XXX LEDs GPIO driver\n";

int __init cns3xxx_leds_init(void)
{
    printk(banner);
    cns3xxx_gpio_port_direction_out(0, 0, 0);
    cns3xxx_gpio_port_direction_out(0, 1, 0);

    panic_blink = cns3xx_panic_blink;

    timer_setup(&cns3xx_leds_timer, cns3xx_leds_timer_callback, 0);

    leds_state_manager(LS_BLINKING_GREEN);

    return platform_driver_register(&cns3xxx_leds_driver);
}

void __exit cns3xxx_leds_exit(void)
{
    stop_leds_timer();

    if (proc_cns3xxx_leds)
        remove_proc_entry(GPIO_PROC_NAME, cns3xxx_proc_dir);
    platform_driver_unregister(&cns3xxx_leds_driver);

    panic_blink = NULL;
}

module_init(cns3xxx_leds_init);
module_exit(cns3xxx_leds_exit);

MODULE_DESCRIPTION("CNS3XXX LED driver");
MODULE_LICENSE("GPL");
