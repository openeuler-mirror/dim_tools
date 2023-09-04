# dim_tools命令行手册

[TOC]

## 1 概述

本文档介绍dim_tools提供的静态基线生成工具dim_gen_baseline的命令行使用。

dim_gen_baseline在DIM特性中的作用是为度量目标进程对应的二进制文件生成静态基线数据，该数据会被DIM内核模块解析并且作为度量基准值。

### 1.1 静态基线格式说明

静态基线数据以文本文件方式存储，以UNIX换行符进行分隔，每行代表一条静态基线，当前支持以下几种配置格式：

1. 用户态进程基线：

   ```
   dim USER sha256:6ae347be2d1ba03bf71d33c888a5c1b95262597fbc8d00ae484040408a605d2b <度量目标进程可执行文件或动态库对应二进制文件的绝对路径>
   ```

2. 内核模块基线：

   ```
   dim KERNEL sha256:a18bb578ff0b6043ec5c2b9b4f1c5fa6a70d05f8310a663ba40bb6e898007ac5 <内核release号>/<内核模块名>
   ```

3. 内核基线：

   ```
   dim KERNEL sha256:2ce2bc5d65e112ba691c6ab46d622fac1b7dbe45b77106631120dcc5441a3b9a <内核release号>
   ```

**参考示例：**

```
dim USER sha256:6ae347be2d1ba03bf71d33c888a5c1b95262597fbc8d00ae484040408a605d2b /usr/bin/bash
dim USER sha256:bc937f83dee4018f56cc823f5dafd0dfedc7b9872aa4568dc6fbe404594dc4d0 /usr/lib64/libc.so.6
dim KERNEL sha256:a18bb578ff0b6043ec5c2b9b4f1c5fa6a70d05f8310a663ba40bb6e898007ac5 6.4.0-1.0.1.4.oe2309.x86_64/dim_monitor
dim KERNEL sha256:2ce2bc5d65e112ba691c6ab46d622fac1b7dbe45b77106631120dcc5441a3b9a 6.4.0-1.0.1.4.oe2309.x86_64
```

### 1.2 静态基线计算说明

当前支持为如下几种文件生成静态基线数据：

| 文件类型      | 计算方法                                                     |
| ------------- | ------------------------------------------------------------ |
| 可执行ELF文件 | 获取ELF文件中类型为LOAD，权限为RE的段数据，并计算哈希值作为静态基线值。如果文件存在多个符合要求的段数据，则合并计算。将文件绝对路径作为索引名称。 |
| 内核模块文件  | 直接计算文件哈希值作为静态基线值。并提取内核的vermagic字段中的内核版本号信息，与内核模块名组合作为索引名称。 |
| 内核          | 直接计算文件哈希值作为静态基线值，用户需要指定内核版本号作为索引名称。 |

**注意：**

- 当前对于内核模块/内核，难以直接基于二进制文件预测实际加载运行后的内存数据，因此静态基线值仅作为一个固定标识，并不实际参与度量结果对比；
- 内核模块名需要以.ko为后缀，暂不支持直接为压缩内核模块生成静态基线；
- 当前不校验内核文件名和文件类型。

## 2 命令行格式说明

dim_gen_baseline的命令行格式为：

```
dim_gen_baseline [options] PATH...
```

其中[options]代表可选参数，PATH...代表需要生成静态基线的文件路径信息，可跟随一个或多个路径。

## 3 命令行参数说明

### 3.1 --help, -h

**功能描述：**

打印dim_gen_baseline命令行帮助信息。

**命令格式：**

```
dim_gen_baseline --help
```

**使用示例：**

```
# dim_gen_baseline --help
Usage: dim_gen_baseline [options] PATH...
options:
  -a, --hashalgo <ALGO>     sha256, sm3 (default is sha256)
  -R, --root <PATH>         relative root path
  -o, --output <PATH>       output file (default is stdout)
  -r, --recursive           recurse into directories
  -k, --kernel <RELEASE>    calculate kernel file hash, specify kernel release, conflict with -r
  -p, --pagesize <SIZE>     page size align for calculating, now support 0, 4K (default is 4K)
  -v, --verbose             increase verbosity level
  -h, --help                display this help and exit
```

### 3.2 --verbose, -V

**功能描述：**

增加打印等级，显示更详细信息。

**命令格式：**

```
dim_gen_baseline --verbose PATH...
```

**使用示例：**

```
# dim_gen_baseline --verbose /usr/bin/bash
Finish calculating baseline of /usr/bin/bash
dim USER sha256:3efee880a67508ba8526aa78fa3ee912d551faaeaa1643fd4afa135f65c76ab3 /usr/bin/bash
```

### 3.3 --hashalgo, -a

**功能描述：**

指定算法，可选sha256，sm3，默认sha256。

**命令格式：**

```
dim_gen_baseline --hashalgo <ALGO> PATH...
```

**使用示例：**

```
# dim_gen_baseline --hashalgo sm3 /usr/bin/bash
dim USER sm3:16ac528e1a30c6537f124a7669cdbd7357f3d2d6ffaaa58c938932c607c60a8e /usr/bin/bash
```

### 3.4 --root, -R

**功能描述：**

指定相对根目录路径，指定后静态基线数据中的路径会截取相对根目录路径前缀。

**命令格式：**

```
dim_gen_baseline --root <PATH> PATH...
```

**使用示例：**

```
# dim_gen_baseline --root /usr/bin /usr/bin/bash
dim USER sha256:3efee880a67508ba8526aa78fa3ee912d551faaeaa1643fd4afa135f65c76ab3 /bash
```

### 3.5 --output, -o

**功能描述：**

将生成的静态基线数据写入指定文件中。

**命令格式：**

```
dim_gen_baseline --output <PATH> PATH...
```

**使用示例：**

```
# dim_gen_baseline /usr/bin/bash -o test.hash
# cat test.hash
dim USER sha256:3efee880a67508ba8526aa78fa3ee912d551faaeaa1643fd4afa135f65c76ab3 /usr/bin/bash
```

### 3.6 --recursive, -r

**功能描述：**

递归目录，为符合条件的文件生成静态基线（当前支持可执行ELF文件和后缀为.ko内核模块文件）。

**命令格式：**

```
dim_gen_baseline --recursive PATH...
```

注意：此时PATH应为有效目录。

**使用示例：**

```
dim_gen_baseline --recursive /usr/bin
dim USER sha256:0c2552990f89a04020199c66da3f5626e00563a612e70d9d0e6b7d76f2bae053 /usr/bin/efikeygen
dim USER sha256:44a7b49be8161180df50b8d9df5ca8269ef4dd4209fde96c18159a9d51db0c92 /usr/bin/localedef
dim USER sha256:26432e339be76b588a7670729603e91592c2c35d4ea34c0486d4d07a1d8dd2a5 /usr/bin/sha1hmac
......
```

### 3.7 --kernel, -k

**功能描述：**

生成内核静态基线，并制定内核版本号，此参数与--recursive参数冲突。

**命令格式：**

```
dim_gen_baseline --kernel <RELEASE> PATH
```

**使用示例：**

```
# dim_gen_baseline -k $(uname -r) /boot/vmlinuz-5.10.0-60.18.0.50.oe2203.x86_64
dim KERNEL sha256:577085ca72886d3c827690950adf6cd455fab22e6fb08167741f1d4989858351 5.10.0-60.18.0.50.oe2203.x86_64
```

### 3.8 --pagesize, -p

**功能描述：**

计算动态基线时，制定页对齐大小，可选0，4K，0代表按照代码段实际长度计算，4K代表按照4K页对齐计算。此参数仅对可执行ELF文件有效。

**命令格式：**

```
dim_gen_baseline --pagesize <SIZE> PATH..
```

**使用示例：**

```
# dim_gen_baseline --pagesize 4K /usr/bin/bash
dim USER sha256:3efee880a67508ba8526aa78fa3ee912d551faaeaa1643fd4afa135f65c76ab3 /usr/bin/bash
```

## 4 LICENSE

dim_tools文档使用的协议为[BY-SA 4.0](LICENSE)。

### 
