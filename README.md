# KePOS
![tag](https://img.shields.io/badge/version-v0.1-brightgreen "tag") ![license](https://img.shields.io/badge/License-Apache%202.0-blue "license") ![centos](https://img.shields.io/badge/centos-6-orange "centos") ![gcc](https://img.shields.io/badge/gcc-4.4.7-green "gcc")  ![AMD](https://img.shields.io/badge/AMD-RYZEN%205-yellowgreen,"AMD")

## 1.介绍

KePOS是一个64位的操作系统。做这个系统是为了将所学的操作系统理论知识和实践相结合，加深对于操作系统的理解。kePOS适合想了解基本的现代操作系统实现方式的人，也适合想了解用户态程序是如何(编译->链接->运行在操作系统上)，如何进行进程切换，系统调用是怎么进入内核并返回的。

KePOS实现了现代操作系统涉及的：`内存管理`，`设备管理(键盘，鼠标)`，`中断和异常处理`，`多进程`，`GUI`，`系统调用`；由于时间关系，KePOS并没有实现`文件系统`，后续可能会补充实现。

如果想要运行KePOS，或者阅读其代码，需要有基本的C语言，汇编语言知识。

###### 参考：

> 《操作系统概念》第七版

> 《一个64位操作系统的实现》

> 《AMD64 Architecture  Programmer’s Manual:  Volumes 1-5》

## 2.环境

下面是项目的开发和部署环境：

- 开发所用的操作系统：centos 6
- gcc 4.4.7
- CPU : AMD RYZEN 5
- 运行KePOS虚拟机：Oracle VM VirtualBox

## 3.文件目录介绍

###### bin目录:

- ​	*kernel.bin*：内核编译后的二进制文件
- ​	*loader.bin*：bootloader的二进制文件
- ​	*mymain.bin*： 用户程序的二进制文件

###### bootloader目录：

​	保存boot和loader的汇编源代码，和makefile文件

###### kernel目录：

​	保存内核源码

###### user目录：

​	保存用户程序和库文件。

- ​	*main.c*为用户程序源代码。
- ​    *makefile*：用于编译和链接用户程序和库.c代码

*KePOS_v0.1.img*：已经制作好的软盘文件(包含：F12文件系统，kernel.bin，mymain.bin,boot.bin，loader.bin)

## 4.系统架构

![绘图4](E:\github image\github image\kePOS\KePOS架构图.gif)