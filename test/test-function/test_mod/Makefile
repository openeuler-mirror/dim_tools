# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

obj-m += test_mod.o

KERNEL := $(DESTDIR)/lib/modules/*/build
# KBUILD_EXTRA_SYMBOLS := $(KERNEL)/Module.symvers
CONFIG_MODULE_SIG=n

PWD :=$(shell pwd)

modules :
	$(MAKE) -C $(KERNEL) M=$(PWD) modules

.PHONEY:clean

clean :
	$(MAKE) -C $(KERNEL) SUBDIRS=$(PWD) clean
