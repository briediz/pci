/* for dev_dbg */
/*#define DEBUG*/
#undef DEBUG
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/pci.h>

#include "pciscan.h"

/* Define debugging for use during driver bringup */
#undef PDEBUG
#ifdef DEBUG
#define PDEBUG(fmt, args...) printk(KERN_DEBUG fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif
#define V3S_PCIE_MAX_BARS 4
struct v3s_bar_info {
  volatile unsigned int *base;
  unsigned int size;
  unsigned int physical;
};

/* PCI DEVICE Parameters */
struct StrV3sPcie {
struct pci_dev* pcidev;
  int index;

  /* memory resources */
  unsigned int nbars;
  u32 msi_enabled;
  struct v3s_bar_info bar [V3S_PCIE_MAX_BARS];
};

#define DRIVER_NAME     "pciscan"
static const char *driver_name = DRIVER_NAME;

static DEFINE_PCI_DEVICE_TABLE(v3s_pci_tbl) = {
  { 0x1ad4, 0x0001, PCI_ANY_ID, PCI_ANY_ID, .driver_data = 123 },
  { 0, }     /* terminate list */
};

static s8 v3s_pci_init_resource(struct pci_dev *pdev, struct StrV3sPcie* myPcie)
{
	int bar_index;
	pr_err("->%s\n", __func__);
	if((! pdev) || (! myPcie)){
		printk (KERN_ERR "%s: pdev or myPcie do not exist\n",
				__func__);
		return -1;
	}

	for (bar_index = 0; bar_index < V3S_PCIE_MAX_BARS; ++bar_index) {
		unsigned int res_start = 0,
								 res_length = 0,
								 res_flags = 0;
		const char *type = "xx";

		res_start = pci_resource_start (pdev, bar_index);
		//res_end = pci_resource_end (pdev, bar_index);
		res_length = pci_resource_len (pdev, bar_index);
		res_flags = pci_resource_flags (pdev, bar_index);

		if (res_flags & IORESOURCE_IO){
			type = "IO";
		}else if (res_flags & IORESOURCE_MEM){
			type = "MEM";
		}else{
				printk (KERN_ERR "%s: Incorrect BAR configuration %d\n", driver_name, bar_index);
				return -2;
		}

		printk (KERN_INFO "%s: BAR #%d, start=%08x length=%08x flags=%08x (%s)\n",
				driver_name,
				bar_index,
				res_start,
				res_length,
				res_flags,
				type);

		if ((res_flags & IORESOURCE_MEM) && (res_start != 0) && (res_length != 0)) {
			//myPcie->bar [bar_index] .base = ioremap_wc (res_start, res_length);
			myPcie->bar [bar_index] .base = pci_iomap(pdev, bar_index, res_length);

			if (myPcie->bar [bar_index] .base == NULL) {
				printk (KERN_ERR "%s: pci_iomap(BAR #%d) failed\n", driver_name, bar_index);
				return -3;
			}
			myPcie->bar [bar_index] .size = res_length;
			myPcie->bar [bar_index] .physical = res_start;

			printk (KERN_INFO "%s: BAR #%d, start=%08x,  physical=%08x \n",
					driver_name,
					bar_index,
					myPcie->bar[bar_index].base,
					myPcie->bar[bar_index].physical
					);
			myPcie->nbars += 1;
		}
	}

	if (myPcie->nbars > 0) {
		printk (KERN_DEBUG "%s: BAR0..3 (%08x %08x %08x %08x)\n",
				driver_name,
				le32_to_cpu(myPcie->bar [0] .base [0]),
				le32_to_cpu(myPcie->bar [1] .base [0]),
				le32_to_cpu(myPcie->bar [2] .base [0]),
				le32_to_cpu(myPcie->bar [3] .base [0]));
	}



	return 0;
}
static int V3S_pci_probe (struct pci_dev *pdev,
                              const struct pci_device_id *ent)
{
  int retval;
  struct StrV3sPcie* myPcie;
  pr_err("->%s\n", __func__);
  u8 myirq = 0;

#ifdef NOT_USED
  printk (KERN_INFO "%s: probe: driver_data=%d\n", driver_name, board_type);
#endif

	/* Allocate memory for our structure */
	myPcie = kzalloc((size_t)sizeof(struct StrV3sPcie), GFP_KERNEL);
	if(myPcie == NULL){
		printk(KERN_ERR "%s, Unable to allocate v3s pcie device structure\n", __func__);
		return -ENOMEM;
	}

	myPcie->pcidev = pdev;
	myPcie->nbars = 0;

	/* for accessing the pci device (I/O regions or interrupt)
	* enable the device
	*/
	retval = pci_enable_device (pdev);
	if (retval) {
					printk (KERN_ERR "%s: pci_enable_device() failed\n", driver_name);
					goto err_free_dev;
	}

	retval = pci_read_config_byte (pdev, PCI_INTERRUPT_LINE, &myirq);
	if (retval == 0) {
			printk (KERN_ERR "%s: pci_read_config_byte(), myirq=%d\n", driver_name, myirq);
    }

  retval = pci_request_regions (pdev, driver_name);
  if (retval < 0) {
    printk (KERN_INFO "%s: pci_request_regions() failed\n", driver_name);

		retval=-20;
    goto err_enable_device;
  }

  pci_set_master(pdev);

  if(v3s_pci_init_resource(pdev, myPcie) != 0){
    printk (KERN_INFO "%s: Error: resources failed\n", driver_name);
	 	retval=-30;
		goto err_release_region;
	}

	/* Append driver data pointer to the pdev structure */
  pci_set_drvdata (pdev, myPcie);


    retval = pci_enable_msi (pdev);
    printk (KERN_INFO "%s: pci_enable_msi() -> %d\n", driver_name, retval);

    if (retval == 0) {
      myPcie->msi_enabled = 1;
    }
  
    retval = pci_read_config_byte (pdev, PCI_INTERRUPT_LINE, &myirq);
    if (retval == 0) {
      printk (KERN_INFO "%s: pci_read_config_byte(), myirq=%d\n", driver_name, myirq);
    }

	
  return retval;

err_release_region:
  pci_release_regions (pdev);

err_enable_device:
  pci_disable_device (pdev);

err_free_dev:
	kzfree(myPcie);

  return retval;
} /* V3S_pci_probe */

static void V3S_pci_remove (struct pci_dev *pdev)
{
  struct StrV3sPcie *myPcie = pci_get_drvdata (pdev);
  pr_err("->%s\n", __func__);

  if (myPcie->msi_enabled) {
    pci_disable_msi (pdev);
    myPcie->msi_enabled = 0;
  }

  if (myPcie) {
    int bar_index;
    printk (KERN_INFO "%s.%d: Release Card\n", driver_name, myPcie->index);

    for (bar_index = 0; bar_index < myPcie->nbars; ++bar_index) {
      if (myPcie->bar [bar_index] .base) {
        pci_iounmap (pdev, (void*)myPcie->bar [bar_index] .base);
      }

      myPcie->bar [bar_index] .base = NULL;
      myPcie->bar [bar_index] .size = 0;
      myPcie->bar [bar_index] .physical = 0;
    }

    myPcie->nbars = 0;
  }

  pci_release_regions (pdev);

  pci_disable_device (pdev);

  kzfree(myPcie);
} 

static struct pci_driver v3s_pci_driver = {
         .name     = DRIVER_NAME,
         .id_table = v3s_pci_tbl,
         .probe    = V3S_pci_probe,
         .remove   = V3S_pci_remove,
};


static __init int mod_init(void)
{
  int ret = 0;

  printk ("mod_init\n");
  
  ret = pci_register_driver (&v3s_pci_driver);
  
  return 0;
}

static void __exit mod_exit(void)
{
  printk ("mod_exit\n");
  pci_unregister_driver (&v3s_pci_driver);
}

module_init( mod_init );
module_exit( mod_exit );

//Meta information
MODULE_AUTHOR("aabbcc");
MODULE_LICENSE("GPL");
