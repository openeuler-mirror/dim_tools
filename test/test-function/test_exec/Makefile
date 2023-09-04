# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

all: test_exec

test_exec: test_exec.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
clean:
	rm -f  *.o test_exec *.hash *.code
