#ifndef SYSTEM
#define SYSTEM

//用户态系统调用库=============================================================================
#define WHITE 	0x00ffffff		//白
#define BLACK 	0x00000000		//黑
#define RED	0x00ff0000		//红
#define ORANGE	0x00ff8000		//橙
#define YELLOW	0x00ffff00		//黄
#define GREEN	0x0000ff00		//绿
#define BLUE	0x000000ff		//蓝
#define INDIGO	0x0000ffff		//靛
#define PURPLE	0x008000ff		//紫

unsigned long kerboardIsPressAndReturn();
void starttask();
void print(unsigned int FColor,unsigned int BColor ,char * string,unsigned long vir1);
void exit();
void applyMainWindow(long x,long y,long xlength,long ylength,char * name);
void drawOneBlock(long x,long y,long xlength,long ylength,int color);
//入口函数
extern unsigned long systemIn(unsigned long vector,unsigned long vir1,unsigned long vir2,unsigned long vir3,unsigned long vir4,unsigned long vir5);
__asm__ (
"systemIn:	\n\t"
    "pushq %r10 \n\t"
    "movq %rcx , %r10 \n\t"
    "syscall \n\t"
    "popq %r10 \n\t"
    "retq \n\t"
);





#endif