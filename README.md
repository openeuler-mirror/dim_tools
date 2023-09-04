# dim_tools

## 1 概述

DIM（Dynamic Integrity Measurement）动态完整性度特性能够检测到运行态的篡改和注入等攻击引起的内存代码段变化，通过将内存代码段动态值同基线值对比，确定运行态代码是否被篡改，从而发现攻击行为，并采取应对措施。

dim_tools是DIM特性的静态基线生成工具，提供dim_gen_baseline命令通过解析ELF文件生成指定格式的代码段度量基线。

## 2 安装dim_tools

### 2.1 使用openEuler源进行安装

以openEuler 23.09版本为例：

```
yum install -y dim_tools
```

### 2.2 使用源码进行编译安装

**(1) 安装依赖软件包**

编译dim_tools需要的软件有：

* make
* gcc
* elfutils-devel
* openssl-devel
* kmod-devel
* kmod-libs

运行dim_tools需要的软件有：

* elfutils
* openssl
* kmod-libs

**(2) 下载源码**

```
git clone https://gitee.com/openeuler/dim_tools.git
```

**(3) 编译安装**

```
cd dim_tools/src/
make
make install
```

**(4) 执行测试用例**

安装执行测试用例所需要的软件包：

```
yum install -y kernel-devel
```

运行测试用例：

```
cd dim_tools/test/test-function
sh test.sh
```

## 3 快速使用指南

为bash进程生成动态度量基线：

```
mkdir -p /etc/dim/digest_list
dim_gen_baseline /usr/bin/bash -o /etc/dim/digest_list/test.hash
```

## 4 文档资料

命令行手册：[dim_tools命令行手册](doc/cmd.md)

## 5 如何贡献

我们非常欢迎新贡献者加入到项目中来，也非常高兴能为新加入贡献者提供指导和帮助。在您贡献代码前，需要先签署[CLA](https://openeuler.org/en/cla.html)。
