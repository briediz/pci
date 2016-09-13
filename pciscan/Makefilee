

MODULES := pci

#guest architecture
ARCH := powerpc

CROSS_COMPILE := $(CROSS_COMPILE)
$(MODULES)-objs := pciscan.o

obj-m += $(MODULES).o

#path of the powerpc compiled kernel
#ROOTDIR := /home/freescale/EvalBoard/P1020/Kernel/git
ROOTDIR := /bin/opengrok/src/stereo/modules-stereo-fsl-1.9/../kernel-stereo-fsl-1.4/git

MAKEARCH := $(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

all: 
	$(MAKEARCH) -C $(ROOTDIR) M=$(shell pwd) modules
	rm -rf *.o *.mod.c *.symvers *.order
	chmod 777 *.ko

clean:
	$(MAKEARCH) -C $(ROOTDIR) M=$(shell pwd) clean
	rm -rf *.o *.mod.c *.symvers *.order
