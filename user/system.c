#include "system.h"

void print(unsigned int FColor,unsigned int BColor,char * string,unsigned long vir1){
    systemIn(0,FColor,BColor,string,vir1,0);
}
void exit(){
    systemIn(1,0,0,0,0,0);
}
unsigned long kerboardIsPressAndReturn(){
    return systemIn(2,0,0,0,0,0);
}
void applyMainWindow(long x,long y,long xlength,long ylength,char * name){
    systemIn(4,x,y,xlength,ylength,(unsigned long)name);
}
void drawOneBlock(long x,long y,long xlength,long ylength,int color){
    systemIn(5,x,y,xlength,ylength,color);
}
//所有用户态任务的入口函数
void starttask(){
    __asm__ __volatile__	(
    "leaq main(%%rip) , %%rax \n\t"
    "callq *%%rax \n\t"
    :
    :
    :"memory"
    );
    //执行结束退出
    exit();
}
