obj-m := adxl345_drv.o
SRC := $(shell pwd)

KERNEL_SRC ?= /scratch/lgasecki/socfpga.de0_nano_soc/sw/build/tmp/work-shared/de0-nano-soc/kernel-build-artifacts

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean
