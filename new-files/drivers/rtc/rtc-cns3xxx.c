/*******************************************************************************
 *
 *  drivers/rtc/rtc-cns3xxx.c
 *
 *  Real Time Clock driver for the CNS3XXX SOCs
 *  
 *  Author: Scott Shu
 *
 *  Copyright (c) 2008 Cavium Networks 
 * 
 *  This file is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License, Version 2, as 
 *  published by the Free Software Foundation. 
 *:
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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h> 
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/platform_data/gpio-cns3xxx.h>
#include <mach/pm.h>
#include "rtc-cns3xxx-nvedit.h"



/* select system clock as the RTC reference clock */
#undef RTC_TEST

#define RTC_IS_OPEN			0x01	/* /dev/rtc is in use */
#define RTC_TIMER_ON			0x02

#define RTC_INTR_ALARM			0x20
#define MINUS_30PPM			0x0
#define MINUS_15PPM			0x1
#define MINUS_10PPM			0x2
#define MINUS_0PPM			0x3
#define PLUS_10PPM			0x4
#define PLUS_15PPM			0x5
#define PLUS_30PPM			0x6
#define DEFAULT_PPM			MINUS_0PPM

#define RTC_ENABLE			(1 << 0)
#define RTC_AUTO_ALARM_SEC_EN		(1 << 1)
#define RTC_AUTO_ALARM_MIN_EN		(1 << 2)
#define RTC_AUTO_ALARM_HOUR_EN		(1 << 3)
#define RTC_AUTO_ALARM_DAY_EN		(1 << 4)
#define RTC_MATCH_ALARM_INTC_EN		(1 << 5)
#define RTC_SYSTEM_CLK			(1 << 6)
#define RTC_ACCESS_CMD			(1 << 7)
#define RTC_DEFUALT_DIGI_TRIM		(DEFAULT_PPM << 21)
#define RTC_SOFT_RESET			(1 << 24)
#define RTC_INTR_STATUS_SEC		(1 << 0)
#define RTC_INTR_STATUS_MIN		(1 << 1)
#define RTC_INTR_STATUS_HOUR		(1 << 2)
#define RTC_INTR_STATUS_DAY		(1 << 3)
#define RTC_INTR_STATUS_ALARM		(1 << 4)
#ifdef CONFIG_RTC_DEBUG 
#define __pr_debug(fmt,args...)		printk( KERN_ERR fmt,##args )
#else			
#define __pr_debug(fmt,args...)         pr_debug( KERN_INFO fmt, ##args )
#endif					
#define READ_FILE				0
#define WRITE_FILE				1
//powe saving sate
#define NO_MODE					0
#define STANDBY_MODE				1
#define MEM_MODE				2
#define DISK_MODE				3
#define ON_MODE					4

#define GIC_REG_VALUE(offset) (*((volatile unsigned int *)(CNS3XXX_TC11MP_GIC_DIST_BASE_VIRT+offset)))

static struct resource *cns3xxx_rtc_mem;
static void __iomem *cns3xxx_rtc_base;
static int cns3xxx_rtc_alarmno = NO_IRQ;
static spinlock_t rtc_lock;

static unsigned long rtc_status = 0;
static struct rtc_time set_alarm_tm_offset;
static struct rtc_time set_alarm_tm;
static struct proc_dir_entry * rtc_proc_entry;
static int pm_state;
static struct device *device;	 

static int cns3xxx_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm);
static int cns3xxx_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm);

static void go_to_halt_mode(void);
static void go_to_doze_mode(void);

static void go_to_doze_mode(void)
{
	int i=0;

printk("Enter Doze Mode...\n");

	spin_lock(&rtc_lock);
	__pr_debug("enter go_to_doze_mode\n");
	
	/* Set in HALT mode */
	cns3xxx_pwr_mode(CNS3XXX_PWR_CPU_MODE_DOZE);

	/* 2. disable all functional block */
	i = PM_CLK_GATE_REG_VALUE;
	PM_CLK_GATE_REG_VALUE &= ~0xFFFFDE0F;
	PM_CLK_GATE_REG_VALUE |=1 << 5;
#if 1
	/* set wake up interrupt source, use ext_intr1 to wake up*/
	PM_WU_CTRL0_REG_VALUE = 0x00000008; PM_WU_CTRL1_REG_VALUE = 0x00000000;
#endif
	
	/* 5. Let CPU enter into WFI state */
	GIC_REG_VALUE(0x104) = 0x1; /* enable clock scaling interrupt */
	__pr_debug("enter WFI\n");
	cns3xxx_wfi();
	PM_CLK_GATE_REG_VALUE = i;
	__pr_debug("leave WFI\n");
	cns3xxx_pwr_mode(CNS3XXX_PWR_CPU_MODE_DFS);
	spin_unlock(&rtc_lock);

printk("Leave Doze Mode...\n");
}

static void go_to_halt_mode(void)
{

	spin_lock(&rtc_lock);
	__pr_debug("enter go_to_halt_mode\n");
	/* Set in HALT mode */
	cns3xxx_pwr_mode(CNS3XXX_PWR_CPU_MODE_HALT);
#if 1
	/* set wake up interrupt source, use ext_intr1 to wake up*/
	PM_WU_CTRL0_REG_VALUE = 0x00000008; PM_WU_CTRL1_REG_VALUE = 0x00000000;
#endif
	/* 5. Let CPU enter into WFI state */
	GIC_REG_VALUE(0x104) = 0x1; /* enable clock scaling interrupt */
	__pr_debug("enter WFI\n");
	cns3xxx_wfi();
	__pr_debug("leave WFI\n");
	cns3xxx_pwr_mode(CNS3XXX_PWR_CPU_MODE_DFS);
	spin_unlock(&rtc_lock);
}

static int cns3xxx_rtc_proc(struct device *dev, struct seq_file *seq);

static int cns3xxx_rtc_read_proc(struct seq_file *s, void *unused)
{
	cns3xxx_rtc_proc(NULL, s);
	return 0;
}

static int cns3xxx_rtc_write_proc(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{
		
	struct rtc_time current_rtc_time;
	__pr_debug("enter rtc proc write, char:%s, count:%u\n", buffer, count);		
	
	if(count)
	{
		do
		{
			if ( strncmp(buffer, "standby",count-1) == 0 ){
				
				__pr_debug("go to standby mode\n");
				pm_state = STANDBY_MODE;
				go_to_halt_mode();
			}else if ( strncmp(buffer, "mem", count-1) == 0 ){
				pm_state = MEM_MODE;	
				go_to_doze_mode();
			}else if ( strncmp(buffer, "disk", count-1) == 0 ){
				pm_state = DISK_MODE;
			}else if( strncmp(buffer, "no", count-1 ) == 0){
				pm_state = NO_MODE;
			}
			if (pm_state != STANDBY_MODE && pm_state != MEM_MODE) {
			    /* nothing needs to be done */
			    goto rtc_write_ret;
			}
			cns3xxx_rtc_gettime( device, &current_rtc_time );
		}while(current_rtc_time.tm_mday != set_alarm_tm.tm_mday);
	}

rtc_write_ret:
	/*
	 * At this point should the pm mode be reset back to normal?
	 * pm_state = ON_MODE;
	 */
	return count;	
}


static int cns3xxx_rtc_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, cns3xxx_rtc_read_proc, &inode->i_private);
}

static const struct proc_ops cns3xxx_rtc_fops = {
    .proc_open           = cns3xxx_rtc_open_proc,
    .proc_read           = seq_read,
    .proc_lseek          = seq_lseek,
    .proc_release        = single_release,
    .proc_write          = cns3xxx_rtc_write_proc,
};

static int __init cns3xxx_rtc_proc_init(void)
{
    rtc_proc_entry = proc_create("rtc", S_IFREG | S_IRUGO,
				 cns3xxx_proc_dir, &cns3xxx_rtc_fops);
    return 1;
}


static irqreturn_t cns3xxx_rtc_alarmirq(int irq, void *id)
{
	struct rtc_device *dev = id;
	struct rtc_time current_rtc_time;

	__pr_debug("alarm interrupt !\n");
	cns3xxx_rtc_gettime( device, &current_rtc_time );

	__pr_debug("pm_state=%d\n", pm_state);
	writeb(RTC_INTR_STATUS_ALARM, cns3xxx_rtc_base + RTC_INTR_STS_OFFSET);
	
	if( current_rtc_time.tm_mday == set_alarm_tm.tm_mday)
	{
		__pr_debug("tmday == alamday\n");
		rtc_update_irq(dev, 1, RTC_AF | RTC_IRQF);
	}
	return IRQ_HANDLED;
}

static int cns3xxx_rtc_open(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);
	int ret;

	if (rtc_status & RTC_IS_OPEN)
		goto out_busy;

	rtc_status |= RTC_IS_OPEN;

	ret =
	    request_irq(cns3xxx_rtc_alarmno, cns3xxx_rtc_alarmirq,
			0, "cns3xxx-rtc alarm", rtc_dev);

	if (ret) {
		dev_err(dev, "IRQ%d error %d\n", cns3xxx_rtc_alarmno, ret);
		return ret;
	}
	device = dev;	
	return 0;
out_busy:
	return -EBUSY;
}

static void cns3xxx_rtc_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);

	free_irq(cns3xxx_rtc_alarmno, rtc_dev);

	rtc_status &= ~RTC_IS_OPEN;
}

static int cns3xxx_rtc_ioctl(struct device *dev, unsigned int cmd,
			     unsigned long arg)
{
	unsigned long ctrl;

	ctrl = readl(cns3xxx_rtc_base + RTC_CTRL_OFFSET);

	switch (cmd) {
	case RTC_AIE_OFF:
		__pr_debug("cns3xxx_rtc_ioctl: disable alarm\n");
		ctrl &= ~RTC_MATCH_ALARM_INTC_EN;
		writel(ctrl, cns3xxx_rtc_base + RTC_CTRL_OFFSET);
		return 0;
	case RTC_AIE_ON:
		__pr_debug("cns3xxx_rtc_ioctl: enable alarm\n");
		ctrl |= RTC_MATCH_ALARM_INTC_EN;
		writel(ctrl, cns3xxx_rtc_base + RTC_CTRL_OFFSET);
		return 0;
#if 0
	case RTC_UIE_OFF:
		__pr_debug("cns3xxx_rtc_ioctl: disable UIE off\n");
		//return 0;
		goto ret;
	case RTC_UIE_ON:	
		__pr_debug("cns3xxx_rtc_ioctl: enable UIE on\n");
		//return 0;
		goto ret;
	case RTC_RD_TIME:
	case RTC_SET_TIME:
		__pr_debug("cns3xxx_rtc_ioctl:set/read time\n");
		goto ret;
	case RTC_ALM_SET:
	case RTC_ALM_READ:
		__pr_debug("cns3xxx_rtc_ioctl:set/read alarm\n");
		goto ret;
	case RTC_IRQP_SET:
	case RTC_IRQP_READ:
		__pr_debug("cns3xxx_rtc_ioctl:set/read irq\n");
		goto ret;
#endif
	default:
		__pr_debug("un support ioctl:%ux\n", cmd);
		//return 0;
	}
	return -ENOIOCTLCMD;
}

static int cns3xxx_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int second, minute, hour, day;
	unsigned long rtc_record;
	unsigned long total_second;

	spin_lock(&rtc_lock);

	second = readw(cns3xxx_rtc_base + RTC_SEC_OFFSET);
	minute = readw(cns3xxx_rtc_base + RTC_MIN_OFFSET);
	hour = readw(cns3xxx_rtc_base + RTC_HOUR_OFFSET);
	day = readw(cns3xxx_rtc_base + RTC_DAY_OFFSET);
	rtc_record = readl(cns3xxx_rtc_base + RTC_REC_OFFSET);

	spin_unlock(&rtc_lock);

	total_second =
	    day * 24 * 60 * 60 + hour * 60 * 60 + minute * 60 + second +
	    rtc_record;

	rtc_time64_to_tm(total_second, rtc_tm);

	__pr_debug("read time %02x.%02x.%02x %02x/%02x/%02x\n",
		 rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
		 rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return 0;
}

static int cns3xxx_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	unsigned int second, minute, hour, day;
	unsigned long long rtc_record;
	unsigned char write_value[20];

	__pr_debug("set time %02d.%02d.%02d %02d/%02d/%02d\n",
		 tm->tm_year, tm->tm_mon, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);

	rtc_record = rtc_tm_to_time64(tm);

	spin_lock(&rtc_lock);

	second = readw(cns3xxx_rtc_base + RTC_SEC_OFFSET);
	minute = readw(cns3xxx_rtc_base + RTC_MIN_OFFSET);
	hour = readw(cns3xxx_rtc_base + RTC_HOUR_OFFSET);
	day = readw(cns3xxx_rtc_base + RTC_DAY_OFFSET);

	rtc_record -=
	    day * 24 * 60 * 60 + hour * 60 * 60 + minute * 60 + second;
	writel((u32)rtc_record, cns3xxx_rtc_base + RTC_REC_OFFSET);
	sprintf(write_value, "%8x",(unsigned int)rtc_record);	

#ifdef CONFIG_RTC_DRV_CNS3XXX_LOG_IN_FLASH
	if (rtcnvet_do_setenv(RTC_LOG_NAME, write_value))
	{
		__pr_debug("write rtc_rec to flash failed!");; 		
	}
#endif
	spin_unlock(&rtc_lock);
	return 0;
}

static int cns3xxx_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct rtc_time *alm_tm = &alarm->time;
	unsigned long long total_alarm_second;
	unsigned char status;
	unsigned long ctrl;

	spin_lock(&rtc_lock);
	
	alm_tm->tm_sec = readw(cns3xxx_rtc_base + RTC_SEC_ALM_OFFSET);
	alm_tm->tm_min = readw(cns3xxx_rtc_base + RTC_MIN_ALM_OFFSET);
	alm_tm->tm_hour = readw(cns3xxx_rtc_base + RTC_HOUR_ALM_OFFSET);
/*
	__pr_debug("%s,%x,mday:%x, hour:%x,minute:%x, sec:%x\n", __FUNCTION__, __LINE__,	\
		 alm_tm->tm_mday, alm_tm->tm_hour, alm_tm->tm_min,  alm_tm->tm_sec);	
*/
	
	if( alm_tm->tm_sec != 0 || alm_tm->tm_min != 0 || alm_tm->tm_hour != 0 )
	{
	        total_alarm_second = rtc_tm_to_time64(&set_alarm_tm_offset);
		total_alarm_second += readl(cns3xxx_rtc_base + RTC_REC_OFFSET);
		__pr_debug("get alarm total sec:%llux\n", total_alarm_second);
		rtc_time64_to_tm(total_alarm_second, alm_tm);
	}

	status = readb(cns3xxx_rtc_base + RTC_INTR_STS_OFFSET);
	alarm->pending = (status & RTC_INTR_STATUS_ALARM) ? 1 : 0;

	ctrl = readl(cns3xxx_rtc_base + RTC_CTRL_OFFSET);
	alarm->enabled = (ctrl & RTC_MATCH_ALARM_INTC_EN) ? 1 : 0;

	spin_unlock(&rtc_lock);

	__pr_debug("cns3xxx_rtc_getalarm: (%d), %02x/%02x/%02x-%02x.%02x.%02x\n",
		 alarm->enabled,
		 alm_tm->tm_year, alm_tm->tm_mon, alm_tm->tm_mday,
		 alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);

	return 0;
}

static int cns3xxx_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct rtc_time *alm_tm = &alarm->time;
	unsigned long ctrl;
	unsigned long long total_alarm_second;

	__pr_debug("cns3xxx_rtc_setalarm: (%d), %02x/%02x/%02x %02x.%02x.%02x\n",
		 alarm->enabled,
		 alm_tm->tm_year & 0xff, alm_tm->tm_mon & 0xff,
		 alm_tm->tm_mday & 0xff, alm_tm->tm_hour & 0xff,
		 alm_tm->tm_min & 0xff, alm_tm->tm_sec);
	
	spin_lock(&rtc_lock);
	memcpy( &set_alarm_tm, alm_tm, sizeof(struct rtc_time) );
	total_alarm_second = rtc_tm_to_time64(alm_tm);

	total_alarm_second -= readl(cns3xxx_rtc_base + RTC_REC_OFFSET);
#ifdef CONFIG_RTC_DEBUG
	__pr_debug("alarm time (second): %lux\n", total_alarm_second);
#endif
	rtc_time64_to_tm(total_alarm_second, &set_alarm_tm_offset);
#ifdef CONFIG_RTC_DEBUG
	__pr_debug("alarm time ():%02x,%02x/%02x/%02x\n", set_alarm_tm_offset.tm_mday, set_alarm_tm_offset.tm_hour,
		 set_alarm_tm_offset.tm_min, set_alarm_tm_offset.tm_sec);
#endif
	writeb(set_alarm_tm_offset.tm_sec, cns3xxx_rtc_base + RTC_SEC_ALM_OFFSET);
	writeb(set_alarm_tm_offset.tm_min, cns3xxx_rtc_base + RTC_MIN_ALM_OFFSET);
	writeb(set_alarm_tm_offset.tm_hour, cns3xxx_rtc_base + RTC_HOUR_ALM_OFFSET);

	__pr_debug("cns3xxx_rtc_setalarm 2: %02x.%02x.%02x\n",
			set_alarm_tm_offset.tm_hour, set_alarm_tm_offset.tm_min, set_alarm_tm_offset.tm_sec);

	writeb(RTC_INTR_STATUS_ALARM, cns3xxx_rtc_base + RTC_INTR_STS_OFFSET);
	alarm->pending = 0;

	ctrl = readl(cns3xxx_rtc_base + RTC_CTRL_OFFSET);
	writel(ctrl | RTC_MATCH_ALARM_INTC_EN,
	       cns3xxx_rtc_base + RTC_CTRL_OFFSET);
	alarm->enabled = 1;
	pm_state = ON_MODE;
	spin_unlock(&rtc_lock);

#if 1
	if (alarm->enabled)
		enable_irq_wake(cns3xxx_rtc_alarmno);
	else
		disable_irq_wake(cns3xxx_rtc_alarmno);
#endif
	return 0;
}

static int cns3xxx_rtc_proc(struct device *dev, struct seq_file *seq)
{
	seq_printf(seq, "======== CNS3XXX RTC REGISTER DUMP ========\n");
	seq_printf(seq, "0x00: 0x%08x : RTC_SEC_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_SEC_OFFSET));
	seq_printf(seq, "0x04: 0x%08x : RTC_MIN_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_MIN_OFFSET));
	seq_printf(seq, "0x08: 0x%08x : RTC_HOUR_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_HOUR_OFFSET));
	seq_printf(seq, "0x0C: 0x%08x : RTC_DAY_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_DAY_OFFSET));
	seq_printf(seq, "0x10: 0x%08x : RTC_SEC_ALM_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_SEC_ALM_OFFSET));
	seq_printf(seq, "0x14: 0x%08x : RTC_MIN_ALM_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_MIN_ALM_OFFSET));
	seq_printf(seq, "0x18: 0x%08x : RTC_HOUR_ALM_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_HOUR_ALM_OFFSET));
	seq_printf(seq, "0x1C: 0x%08x : RTC_REC_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_REC_OFFSET));
	seq_printf(seq, "0x20: 0x%08x : RTC_CTRL_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_CTRL_OFFSET));
	seq_printf(seq, "0x34: 0x%08x : RTC_INTR_STS_OFFSET\n",
		   readl(cns3xxx_rtc_base + RTC_INTR_STS_OFFSET));
	seq_printf(seq, "\n\nPower Management State = %d\n", pm_state);
	return 0;
}

#ifdef FIXME_RTC_CNS3XXX_IRQ_NOT_IMPLEMENTED
static int cns3xxx_periodic_irq_set_state(struct device *dev, int enabled)
{
	__pr_debug("%s,%s,%d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static int cns3xxx_periodic_irq_set_freq(struct device *dev, int freq)
{
	__pr_debug("%s,%s,%d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}
#endif /* FIXME_RTC_CNS3XXX_IRQ_NOT_IMPLEMENTED */

static const struct rtc_class_ops cns3xxx_rtcops = {
	.ioctl		= cns3xxx_rtc_ioctl,
	.read_time	= cns3xxx_rtc_gettime,
	.set_time	= cns3xxx_rtc_settime,
	.read_alarm	= cns3xxx_rtc_getalarm,
	.set_alarm	= cns3xxx_rtc_setalarm,
	.proc		= cns3xxx_rtc_proc,
//	.irq_set_state 	= cns3xxx_periodic_irq_set_state,
//	.irq_set_freq	= cns3xxx_periodic_irq_set_freq,
};

static int cns3xxx_rtc_probe(struct platform_device *dev)
{
	struct rtc_device *rtc;
	struct resource *res;
	unsigned long ctrl;
#ifdef CONFIG_RTC_DRV_CNS3XXX_LOG_IN_FLASH	
	unsigned long rtc_rec;
	char* buffer = NULL;
#endif	
	int ret = 0;

	__pr_debug("%s: probe=%p\n", __func__, dev);

	/* We only accept one device, and it must have an id of -1 */
	if (dev->id != -1)
		return -ENODEV;
	cns3xxx_rtc_alarmno = platform_get_irq(dev, 0);
	if (cns3xxx_rtc_alarmno < 0) {
		dev_err(&dev->dev, "no irq for alarm\n");
		return -ENOENT;
	}
	printk( KERN_ERR "rtc irq:%x\n",cns3xxx_rtc_alarmno );
	__pr_debug("cns3xxx_rtc: alarm irq %d\n", cns3xxx_rtc_alarmno);

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&dev->dev, "failed to get memory region resource\n");
		return -ENODEV;
	}
	cns3xxx_rtc_mem =
	    request_mem_region(res->start, res->end - res->start + 1,
			       dev->name);
	if (cns3xxx_rtc_mem == NULL) {
		dev_err(&dev->dev, "failed to reserve memory region\n");
		ret = -ENOENT;
		goto err_nortc;
	}

	cns3xxx_rtc_base = ioremap(res->start, res->end - res->start + 1);
	if (cns3xxx_rtc_base == NULL) {
		dev_err(&dev->dev, "failed ioremap()\n");
		ret = -ENOMEM;
		goto err_free;
	}


	cns3xxx_rtc_open(&dev->dev);

	/* first, disable RTC and initial RTC alarm registers */
	
	writel(0, cns3xxx_rtc_base + RTC_CTRL_OFFSET);
	writel(0, cns3xxx_rtc_base + RTC_SEC_ALM_OFFSET);
	writel(0, cns3xxx_rtc_base + RTC_MIN_ALM_OFFSET);
	writel(0, cns3xxx_rtc_base + RTC_HOUR_ALM_OFFSET);
	/* enable RTC */
	ctrl = readl(cns3xxx_rtc_base + RTC_CTRL_OFFSET);
#ifdef RTC_TEST			/* select system clock as the RTC reference clock */
	writel(ctrl | RTC_ENABLE | RTC_SYSTEM_CLK | RTC_DEFUALT_DIGI_TRIM,
	       cns3xxx_rtc_base + RTC_CTRL_OFFSET);
#else
	writel(ctrl | RTC_ENABLE | RTC_ACCESS_CMD | RTC_DEFUALT_DIGI_TRIM, cns3xxx_rtc_base + RTC_CTRL_OFFSET);
#endif
	/* register RTC */	
	rtc = devm_rtc_allocate_device(&dev->dev);
	if (IS_ERR(rtc)) {
		dev_err(&dev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		goto err_nores;
	}
	rtc->ops = &cns3xxx_rtcops;

	ret = devm_rtc_register_device(rtc);
	if (ret) {
		dev_err(&dev->dev, "cannot create dev");
		goto err_nores;
	}

#ifdef CONFIG_RTC_DRV_CNS3XXX_LOG_IN_FLASH
	//since RTC can't save the hardware time. we need to retrieve the offset rec from flash.
	rtcnvet_init();
	rtcnvet_get_env(RTC_LOG_NAME, &buffer);
	if (buffer)
	{
		rtc_rec = simple_strtoul(buffer, NULL, 16);
		writel(rtc_rec, cns3xxx_rtc_base + RTC_REC_OFFSET);
		__pr_debug("read from flash value:%s, write to register value:%lux\n", buffer, rtc_rec);
#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
		kfree(buffer);
		buffer = NULL;        
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

	}
#endif
	cns3xxx_rtc_proc_init();
	device_init_wakeup(&dev->dev, 1);
	platform_set_drvdata(dev, rtc);

	return 0;

err_nores:
	iounmap(cns3xxx_rtc_base);

err_free:
	release_resource(cns3xxx_rtc_mem);
	kfree(cns3xxx_rtc_mem);
err_nortc:
	__pr_debug("probe func has something wrong!");
	return ret;
}

static int cns3xxx_rtc_remove(struct platform_device *dev)
{
	__pr_debug("%s: remove=%p\n", __func__, dev);

	platform_set_drvdata(dev, NULL);

	iounmap(cns3xxx_rtc_base);
	release_resource(cns3xxx_rtc_mem);
	kfree(cns3xxx_rtc_mem);

#ifdef CONFIG_RTC_DRV_CNS3XXX_LOG_IN_FLASH
	rtcnvet_exit();
#endif

	cns3xxx_rtc_release(&dev->dev);
	return 0;
}

#ifdef CONFIG_PM

static int cns3xxx_rtc_suspend(struct platform_device *dev, pm_message_t state)
{
	__pr_debug("%s,%s,%d\n", __FILE__, __FUNCTION__, __LINE__);

	return 0;
}

static int cns3xxx_rtc_resume(struct platform_device *dev)
{
	__pr_debug("%s,%s,%d\n", __FILE__, __FUNCTION__, __LINE__);

	return 0;
}

#else
#define cns3xxx_rtc_suspend	NULL
#define cns3xxx_rtc_resume	NULL
#endif /* CONFIG_PM */

static struct platform_driver cns3xxx_rtcdrv = {
	.probe		= cns3xxx_rtc_probe,
	.remove		= cns3xxx_rtc_remove,
	.suspend	= cns3xxx_rtc_suspend,
	.resume		= cns3xxx_rtc_resume,
	.driver		= {
	.name 		= "cns3xxx-rtc",
	.owner 		= THIS_MODULE,
	},
};

static char banner[] __initdata =
    KERN_INFO "CNS3XXX Real Time Clock, (c) 2009 Cavium Networks\n";

static int __init cns3xxx_rtc_init(void)
{
	printk(banner);

	spin_lock_init(&rtc_lock);

	return platform_driver_register(&cns3xxx_rtcdrv);
}

static void __exit cns3xxx_rtc_exit(void)
{
	platform_driver_unregister(&cns3xxx_rtcdrv);
}

module_init(cns3xxx_rtc_init);
module_exit(cns3xxx_rtc_exit);

MODULE_AUTHOR("Scott Shu");
MODULE_DESCRIPTION("Cavium Networks CNS3XXX RTC Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cns3xxx-rtc");
