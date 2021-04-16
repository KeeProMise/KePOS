#include "printk.h"
#include "trap.h"
#include "global.h"
#include "lib.h"
#include "device.h"
#include "task.h"

unsigned long time = 0;
unsigned long now = 0;

//键盘的中断处理函数，将键盘扫描码读到键盘buffer中
void keyboardHandle(unsigned long rsp){
    struct Buffer * buffer = &deviceTable.devices[0]->inBuffer;
//    while (isFull(buffer)){
//        unsigned char d = deviceTable.devices[0]->read(0,1,0);
//        color_printk(RED,WHITE,"K[delete=:%#04lx]",d);
//    }
    unsigned char x = io_in8(0x60);
    deviceTable.devices[0]->write(0,&x,1,0);
    //color_printk(RED,WHITE,"K[x=:%#04lx]",x);
}

//鼠标中断处理函数
void mouseHandle(unsigned long rsp){
     struct Buffer * buffer = &deviceTable.devices[1]->inBuffer;
     unsigned char x = io_in8(0x60);
     deviceTable.devices[1]->write(1,&x,1,0);
}

//时钟中断处理函数
void timeHandle(unsigned long rsp){
    unsigned long hour,minute,second;
    hour = time/3600;
    minute = (time/60)%60;
    second = time%60;
    //一个时间片10ms，进行一次任务切换
    changeTask(rsp);
    //计时一秒
    if(now == 100){
        //color_printk(RED,BLACK,"T:[%#02d]:[%#02d]:[%02d]\n",hour,minute,second);
        now = 0;
        time += 1;
    }
    now++;
}
void doInterrupt(unsigned long rsp,unsigned long vector){
      if(vector == 0x20) timeHandle(rsp);
      else if(vector == 0x21) keyboardHandle(rsp);
      else if(vector == 0x2c) mouseHandle(rsp);
      else{
          unsigned long * rip = NULL;
          rip = (unsigned long *)(rsp + RIP);
          color_printk(RED,BLACK,"do_IRQ:%#02lx\t",vector);
          color_printk(RED,BLACK,"<RIP:%#018lx\tRSP:%#018lx>\n",*rip,rsp);
      }

      io_out8(0x20,0x20);
      io_out8(0xa0,0x20);
}



void init8259A(){
   // color_printk(RED,BLACK,"8259A init \n");
    //8259A-master	ICW1-4
    io_out8(0x20,0x11);
    io_out8(0x21,0x20);
    io_out8(0x21,0x04);
    io_out8(0x21,0x01);

    //8259A-slave	ICW1-4
    io_out8(0xa0,0x11);
    io_out8(0xa1,0x28);
    io_out8(0xa1,0x02);
    io_out8(0xa1,0x01);

    //8259A-M/S	OCW1
    //开启键盘和鼠标中断
    io_out8(0x21,0xf8);
    io_out8(0xa1,0xef);
    //开启系统中断
    sti();
}




void interruptMain(){
    //初始化中断处理程序
    init8259A();
}