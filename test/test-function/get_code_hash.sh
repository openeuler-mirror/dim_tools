#!/bin/bash
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

line=$(readelf -Wl $1 | grep "R E")
if [ $? -ne 0 ]; then
	echo "Fail to get RE segment"
	exit 1
fi

offset=$(echo "$line" | awk '{print $2}')
length=$(echo "$line" | awk '{print $5}')

offset=${offset#*0x}
length=${length#*0x}

let length_4096=$((16#$length))/4096*4096
if [ $length_4096 -eq $((16#$length)) ]; then
	let length_4096=$((16#$length))
else
	let length_4096=($((16#$length))/4096+1)*4096
fi

dd if=$1 of=$1.code bs=1 skip=$((16#$offset)) count=$length_4096 &> /dev/null
if [ $? -ne 0 ]; then
	echo "Fail to dump segment"
	exit 1
fi

sha256sum $1.code | awk '{print $1}' > $1.code.hash
