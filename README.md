# KePOS
![tag](https://img.shields.io/badge/version-v0.1-brightgreen "tag") ![license](https://img.shields.io/badge/License-Apache%202.0-blue "license") ![centos](https://img.shields.io/badge/centos-6-orange "centos") ![gcc](https://img.shields.io/badge/gcc-4.4.7-green "gcc")  ![AMD](https://img.shields.io/badge/AMD-RYZEN%205-yellowgreen,"AMD")

## 1.Introduction

KePOS is a 64-bit operating system. The purpose of this system is to combine the theoretical knowledge and practice of the operating system, and to deepen the understanding of the operating system. kePOS is suitable for people who want to understand the basic implementation of modern operating systems. It is also suitable for people who want to understand how user mode programs (compile -> link -> run on the operating system), how to switch processes, and how system calls enter the kernel and return.

KePOS implements what modern operating systems involve: **memory management**, **device management** (keyboard, mouse), **interrupt and exception handling**, **multi-process**, **GUI**, **system calls**; due to time constraints, KePOS does not implement a file system, and may be implemented later.

If you want to run KePOS or read its code, you need basic knowledge of **C language** and **assembly language**. 

##### presention：

[![cHMRBV.png](https://z3.ax1x.com/2021/04/20/cHMRBV.png)](https://imgtu.com/i/cHMRBV)

## 2.Environment 

The following is the development and deployment environment of the project:

- Operating system used for development: **centos 6**
- **gcc 4.4.7**
- CPU: **AMD** RYZEN 5, **Inter** cpu is also available (compatible instructions are used)
- Run KePOS virtual machine: Oracle VM **VirtualBox** 

## 3.Directory introduction 

###### bin directory:

- ​	*kernel.bin*：Binary file after kernel compilation 
- ​	*loader.bin*：Binary file after bootloadercompilation 
- ​	*mymain.bin*： Binary file of user program 

###### bootloader directory：

​	Save the assembly source code of boot and loader, and makefile 

###### kernel directory：

​	Save the kernel source code 

###### user directory：

​	Save user programs and library files. 

- ​	*main.c*   source code of user program. 
- ​    *makefile*：Used to compile and link user programs and libraries code .

*KePOS_v0.1.img*：The prepared floppy disk file (including: F12 file system, kernel.bin, mymain.bin, boot.bin, loader.bin) .

## 4.System structure 

[![cHWWNV.gif](https://z3.ax1x.com/2021/04/20/cHWWNV.gif)](https://imgtu.com/i/cHWWNV)

## 5.Installation

#### 1.Quick start 

In the Oracle VM VirtualBox , create a new **64-bit** virtual machine (memory: 4G, including floppy disk). Download *KePOS_v0.1.img* and add it to the  virtual machine to run the KePOS system.

Open the kePOS system and click the right mouse button to open a user process. The process can enter (w, a, s, d) on the keyboard to control the movement direction of the small square. 

*The window closing has not yet been realized.* 

#### 2.Compile

In centos 6, make sure the gcc version is 4, then execute the following commands:

Compile to generate *boot.bin* and *loader.bin,* go to the bootloader directory, and execute:

```shell
make
```

Compile to generate *kernel.bin*, go to the kernel directory, and execute:

```shell
make
```

Compile and generate *mymain.bin*, go to the user directory, and execute:

```shell
make
```

#### 3.Floppy disk image

In the directory where boot.bin is located, execute the following command, the *KePOS_v0.1.img* will be generated in the current directory. 

```shell
dd if=boot.bin of=KePOS_v0.1.img bs=512 count=1 conv=notrunc
dd if=/dev/zero of=KePOS_v0.1.img bs=512 count=2880
```

#### 4.Copy binary file to floppy disk image 

You can copy the prepared *KePOS_v0.1.img*, *loader.bin* and *kernel.bin*, *mymain.bin* to the **windows** operating system, and then use the software **WinImage** to open *KePOS_v0.1.img*, drag *loader.bin*, *kernel.bin*, *mymain.bin* to *KePOS_v0.1.img*. 

#### 5.Virtual machine settings 

- Create any 64-bit virtual machine in Oracle VM VirtualBox. 
- The memory size of the virtual machine>4G 
- Storage contains  floppy disk 
- Display controller select vboxSVGA, or vmSVGA 

#### 6.Run

- Add *KePOS_v0.1.img* to the virtual machine
- Click to start the virtual machine to run the system.

## 6.Improve 

The file system function has yet to be realized.

The GUI function needs to be improved. 

There are few types of user programs and need to be added. 

## 7.reference

> 《操作系统概念》第七版
>
> 《一个64位操作系统的实现》田宇
>
> 《Linux 内核完全注释》赵烔
>
> 《AMD64 Architecture  Programmer’s Manual:  Volumes 1-5》