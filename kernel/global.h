#ifndef GLOBAL
#define GLOBAL
//保存全局变量

#define NULL 0

#define True 1
#define False 0

#define NO 0xffffffffffffffff

#define WINDOW_WIDTH 800
#define WINDOW_HIGH 600

#include "memory.h"
#include "device.h"
#include "window.h"
#include "system.h"

//SVGA线性地址
extern unsigned long SVGA_Virtual_Address;
//E820保存的线性地址
extern unsigned long E820Address;
//全局内存结构体
extern struct Memory memory;
//保存总内存大小
extern unsigned long totalMemory;

//内核结构体占用内存结束位置
extern unsigned long StartStruct;

//GDT,TSS,IDT
extern unsigned long GDTbase;
extern unsigned long IDTbase;
extern unsigned long TSS64base;

//设备
extern struct DeviceTable deviceTable;

//window
extern struct Windows windows;

//time
extern unsigned long time;
extern unsigned long now;

//task
extern struct TaskManage taskManage;

//system
extern struct SystemCallTabel systemCallTabel;

//lock
extern volatile struct ReeTrantLock reeTrantLock;
#endif