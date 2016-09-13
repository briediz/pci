#ifndef __MODTEST_H
#define __MODTEST_H

struct irq_chan {
	struct device *dev;
	struct resource *res;
	void __iomem* regs;
	struct list_head list;
	char irq_name[16];
	int irq;
	u8 irqnumber;
	wait_queue_head_t wq_poll;
};


struct _mIrqsignal{
	dev_t dev_nr;
	spinlock_t splock;
	struct list_head ch_list;

	struct cdev* drv_obj;
	struct platform_device *pdev;
	struct device* dev;
	struct class* m_class;
	int irq_active;

	/* Hardware device constant */
	unsigned long* map_StartAddr;

	/* Drivers statistics */
};

#endif  //__MODTEST_H
