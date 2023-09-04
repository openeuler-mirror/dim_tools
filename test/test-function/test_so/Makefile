# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

CFLAGS=-fPIC -shared

all: test_so.so

test_so.so: test_so.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
clean:
	rm -f  *.o *.so *.code *.hash
