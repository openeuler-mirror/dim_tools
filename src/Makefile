# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

BINDIR ?= $(DESTDIR)/usr/bin
TARGETS = dim_gen_baseline

CFLAGS ?= -O2 -fstack-protector-all -fPIE -fPIC -D_FORTIFY_SOURCE=2 -O2
LDFLAGS ?= -Wl,-z,relro,-z,now,-z,noexecstack -pie -lcrypto -lelf -lkmod

ifdef DIM_TOOLS_GCOV
	CFLAGS += -ftest-coverage -fprofile-arcs
endif

all: $(TARGETS)
dim_gen_baseline: dim_gen_baseline.o calculate.o format.o baseline.o utils.o
	$(CC) $(CFLAGS)  -o $@ $^ $(LDFLAGS)
	strip dim_gen_baseline
%.o: %.c 
	$(CC) $(CFLAGS) -o $@ -c $<

install:
	mkdir -p $(BINDIR)
	install -m 0755 dim_gen_baseline $(BINDIR)

clean:
	rm -f *.o *.gcda *.gcno *.info $(TARGETS)
