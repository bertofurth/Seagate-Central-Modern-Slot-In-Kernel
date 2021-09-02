
/*
 * Module to handle the gpio buttons on the CNs3XXX boards.
 *
 * Author: Sreeju A Selvaraj <sselvaraj@mvista.com>
 *
 * 2011 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/platform_data/cns3xxx.h>
#include <linux/platform_data/gpio-cns3xxx.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>

#define __DEBUG__	1

/* Define Maximum number of buttons*/
#define BUTTONS_MAX	1

/* Pin Description */
#define FACTORY_DEFAULT		GPIOB(2)

/* In ms */
#define FACTORY_DEFAULT_BUTTON_HOLD_DURATION	10000

/* In jiffies (HZ=>100, 10jiffies=>100ms)*/
// #define SAMPLING_INTERVAL           (HZ / 10)
#define SAMPLING_INTERVAL               HZ

/* button events */
#define EV_FACTORY_DEFAULT		4

/* ioctl commands */
#define BIOC_GET_EVENT			1

struct button_control {
	int timer_count;
	int hold_timer_count;
	bool processed;
};

struct button_data {
	struct button_control control[BUTTONS_MAX];
	struct timer_list timer;
	struct work_struct work;
};

DEFINE_MUTEX(b_mutex);
static DECLARE_WAIT_QUEUE_HEAD(b_wait);	/* Wait Queue */

static struct gpio button_gpios[] = {
	{FACTORY_DEFAULT, GPIOF_IN, "Factory default button"},
};

/* button hold duration */
static int button_hold_time[BUTTONS_MAX] = {
	FACTORY_DEFAULT_BUTTON_HOLD_DURATION,
};

/* button events */
char b_events[BUTTONS_MAX][2] = {
	{0, EV_FACTORY_DEFAULT},
};

/* button data */
struct button_data *bdata;
static int button_event;

/*
 * Open button device
 */
int button_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 * Close button device
 */
int button_release(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 * ioctls to get button events
 */
long int button_ioctl(struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;

	switch (cmd) {

	case BIOC_GET_EVENT:
		wait_event_interruptible(b_wait, button_event != 0);
		mutex_lock(&b_mutex);
		if (button_event) {
		    ret = copy_to_user(argp, &button_event, sizeof(button_event));
		    button_event = 0;
		}
		mutex_unlock(&b_mutex);
		break;

	default:
		ret = -EIO;
		break;
	}

	return ret;
}

static const struct file_operations button_fops = {
	.owner = THIS_MODULE,
	.open = button_open,
	.unlocked_ioctl = button_ioctl,
	.release = button_release,
};

static struct miscdevice my_button_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "button",
	.fops = &button_fops
};

/*
 * Report the button event.
 */
static void report_gpio_button_event(int button, int status)
{
	mutex_lock(&b_mutex);
	if (bdata->control[button].timer_count >=
	    bdata->control[button].hold_timer_count) {

		/* pressing for long duration */
		bdata->control[button].processed = 1;

		/* Set button event */	
		button_event = b_events[button][1];
#if __DEBUG__
		printk(KERN_INFO "%s: Button press and hold\n",
		       button_gpios[button].label);
#endif
		wake_up_interruptible(&b_wait);	

	} else if ((bdata->control[button].timer_count >= 1) && (!status)) {


		/* pressing for short duration */
		bdata->control[button].processed = 1;
#if __DEBUG__
		printk(KERN_INFO "%s: Button press and release\n",
		       button_gpios[button].label);
#endif
		// For Futer use.
		//button_event = b_events[button][1];
		//	wake_up_interruptible(&b_wait);
	}
	mutex_unlock(&b_mutex);
}

static void gpio_button_work_func(struct work_struct *work)
{
	int ret;
	int irq, status, pressed = 0;

	status = (gpio_get_value(button_gpios[0].gpio) ? 1 : 0);
	if (status)
		bdata->control[0].timer_count++;

	if (!(bdata->control[0].processed))
		report_gpio_button_event(0, status);

	pressed |= status;
	status = 0;

	/* If any button, still remain pressed, continue the polling */
	if (pressed) {
	    	printk(KERN_INFO " Factory Default Button is pressed\n");
		ret = mod_timer(&bdata->timer, jiffies + SAMPLING_INTERVAL);
		if (ret)
			printk(KERN_ERR "button_work: mod timer error\n");
		/* re-enable irqs */
	} else {
		bdata->control[0].timer_count = 0;
		bdata->control[0].processed = 0;
		irq = gpio_to_irq(button_gpios[0].gpio);
		printk(KERN_INFO " Factory Default Button is NOT pressed\n");
		enable_irq(irq);
	}
}

void button_timer(struct timer_list *timer)
{
	/* schedule work */
	schedule_work(&bdata->work);
}

static irqreturn_t button_gpio_intr_handler(int i, void *d)
{
	int irq;
	int ret = 1;

	ret = mod_timer(&bdata->timer, jiffies + SAMPLING_INTERVAL);
	if (ret) {
		printk(KERN_ERR "button_gpio_intr_handler: mod timer error\n");
	} else {
		/* disable the irqs */
		irq = gpio_to_irq(button_gpios[0].gpio);
		disable_irq_nosync(irq);
	}

	return IRQ_HANDLED;
}

static int button_gpio_init(void)
{
//	int button;
        int ret;
	
	/* Enable GPIOA GPIOB 2 */
	MISC_GPIOB_PIN_ENABLE_REG_VALUE &= ~(0x1 << 2);

	bdata->control[0].timer_count = 0;
	bdata->control[0].hold_timer_count =
	    button_hold_time[0] /
	    jiffies_to_msecs(SAMPLING_INTERVAL);
	bdata->control[0].processed = 0;

	/* Request GPIO */
	if (gpio_request
	    (button_gpios[0].gpio, button_gpios[0].label)) {
		printk(KERN_ERR "gpio_request for %s failed\n",
		       button_gpios[0].label);
		return -1;
	}
	gpio_direction_input(button_gpios[0].gpio);
	timer_setup(&bdata->timer, button_timer, 0);
	INIT_WORK(&bdata->work, gpio_button_work_func);
	ret = request_irq(gpio_to_irq(FACTORY_DEFAULT), button_gpio_intr_handler,
			  IRQF_SHARED | IRQF_TRIGGER_RISING, "factory default",
			  my_button_dev.this_device);
	return 0;
}

static int __init button_init(void)
{
	int result;

	/* Registering device */
	result = misc_register(&my_button_dev);

	if (result < 0) {
		printk(KERN_ERR "button: cannot register as misc\n");
		return result;
	}

	/* allocate button  data */
	bdata = kzalloc(sizeof(struct button_data), GFP_KERNEL);

	if (!bdata) {
		printk(KERN_ERR "bdata allocation failed...\r\n");
		goto fail;
	}

	if (button_gpio_init()) {
		kfree(bdata);
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static void __exit button_exit(void)
{
	del_timer_sync(&bdata->timer);
	cancel_work_sync(&bdata->work);
	kfree(bdata);

	free_irq(gpio_to_irq(button_gpios[0].gpio), button_gpios);
	gpio_free(button_gpios[0].gpio);
	misc_deregister(&my_button_dev);
}

late_initcall(button_init);
module_exit(button_exit);

MODULE_AUTHOR("Sreeju Selvaraj <sselvaraj@mvista.com>");
MODULE_DESCRIPTION("GPIO-Button driver");
MODULE_LICENSE("GPL");
