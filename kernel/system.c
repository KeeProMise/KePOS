#include "system.h"
#include "printk.h"
#include "global.h"
#include "device.h"
#include "window.h"
#include "task.h"
#include "lib.h"

static int  state = 0;
static int mouseState = 0;
struct SystemCallTabel systemCallTabel = {{0}};
static int shift_l,shift_r,ctrl_l,ctrl_r,alt_l,alt_r;

//鼠标相关调用
unsigned long mouseBufferIsEmpty(){
    return isEmpty(&deviceTable.devices[MOUSEID]->inBuffer);
}
void doMousePack(struct MousePack * mousepack){

    long x = mousepack->x;
    long y = mousepack->y;
    switch (state) {
        case 0:
            if(mousepack->btn == 0x01) state = 1;
            else if(mousepack->btn == 0x02) state = 4;
            else if(mousepack->byte[0] == 0x08 && (x!=0 || y!=0)) state = 6;
            else state = 0;
            break;
        case 1:
            if(mousepack->byte[0] == 0x08) state = 2;
            else if(mousepack->btn == 0x01 && (x!=0 || y!=0)) state = 3;
            else state = 0;
            break;
        case 3:
             if(mousepack->byte[0] == 0x08) state = 0;
             else if(mousepack->btn == 0x01 && (x!=0 || y!=0)) state = 3;
             else state = 0;
            break;
        case 4:
             if(mousepack->byte[0] == 0x08) state = 5;
             else state = 0;
            break;
        case 5:
            break;
        case 6:
            if(mousepack->byte[0] = 0x08 && (x!=0 || y!=0)) state = 6;
            else if(mousepack->btn == 0x01) state = 1;
            else if(mousepack->btn == 0x02) state = 4;
            else if(mousepack->byte[0] == 0x08 && (x!=0 || y!=0)) state = 6;
            else state = 0;
            break;
    }
  //  color_printk(RED,BLUE,"state:%#018lx\n",state);
    switch (state) {
        case 0:break;
        //如果选中的是窗口，则交换顶层
        case 1:
            cli();
            changeWindow();
            sti();
            break;
        //交换结束
        case 2:state = 0;break;
        //移动顶层窗口，和鼠标
        case 3:
            cli();
            movMouse(x,y);
            movWindowAndMouse(windows.windows->prev,x,y);
            sti();
            break;
        //点击地方变色
        case 4:
           // cli();
            initAtomUserTask();
            //sti();
            break;
        //触发点击相应的动作
        case 5:state = 0;break;
        //移动鼠标
        case 6:
            cli();
            movMouse(x,y);
            sti();
            //state = 0;
            break;
    }
}
void analysisMouse(){
    unsigned char c = deviceTable.devices[MOUSEID]->read(MOUSEID,1,0);
    switch(mouseState)
    {
        case 0:
            if(c == 0xfa) mouseState++;
            break;
        case 1:
            if((c&0xc8) == 0x08){
                mousePack.byte[0] = c;
                mouseState++;
            }
            break;

        case 2:
            mousePack.byte[1] = c;
            mouseState++;
            break;

        case 3:
            mousePack.byte[2] = c;
            mouseState = 1;
            mousePack.btn = mousePack.byte[0]&0x07;
            mousePack.x = (long)mousePack.byte[1];
            mousePack.y = (long)mousePack.byte[2];
            if((mousePack.byte[0]&0x10) != 0) mousePack.x |= 0xffffffffffffff00;
            if((mousePack.byte[0]&0x20) != 0) mousePack.y |= 0xffffffffffffff00;
            doMousePack(&mousePack);
            break;

        default:
            break;
    }
}

//键盘相关调用
unsigned long keyBoardBufferIsEmpty(){
    return isEmpty(&deviceTable.devices[KEYID]->inBuffer);
}
unsigned long keyBufferIsEmpty(){
    return isEmpty(&deviceTable.devices[KEYID]->outBuffer);
}
unsigned char get_scancode(){
    unsigned char d = deviceTable.devices[KEYID]->read(0,1,0);
    return d;
}
//分析键盘输入
void analysis_keycode()
{
    unsigned char x = 0;
    int i;
    int key = 0;
    int make = 0;

    x = get_scancode();

    if(x == 0xE1)	//pause break;
    {
        key = PAUSEBREAK;
        for(i = 1;i<6;i++)
            if(get_scancode() != pausebreak_scode[i])
            {
                key = 0;
                break;
            }
    }
    else if(x == 0xE0) //print screen
    {
        x = get_scancode();

        switch(x)
        {
            case 0x2A: // press printscreen

                if(get_scancode() == 0xE0)
                    if(get_scancode() == 0x37)
                    {
                        key = PRINTSCREEN;
                        make = 1;
                    }
                break;

            case 0xB7: // UNpress printscreen

                if(get_scancode() == 0xE0)
                    if(get_scancode() == 0xAA)
                    {
                        key = PRINTSCREEN;
                        make = 0;
                    }
                break;

            case 0x1d: // press right ctrl

                ctrl_r = 1;
                key = OTHERKEY;
                break;

            case 0x9d: // UNpress right ctrl

                ctrl_r = 0;
                key = OTHERKEY;
                break;

            case 0x38: // press right alt

                alt_r = 1;
                key = OTHERKEY;
                break;

            case 0xb8: // UNpress right alt

                alt_r = 0;
                key = OTHERKEY;
                break;

            default:
                key = OTHERKEY;
                break;
        }

    }

    if(key == 0)
    {
        unsigned int * keyrow = NULL;
        int column = 0;

        make = (x & FLAG_BREAK ? 0:1);

        keyrow = &keycode_map_normal[(x & 0x7F) * MAP_COLS];

        if(shift_l || shift_r)
            column = 1;

        key = keyrow[column];

        switch(x & 0x7F)
        {
            case 0x2a:	//SHIFT_L:
                shift_l = make;
                key = 0;
                break;

            case 0x36:	//SHIFT_R:
                shift_r = make;
                key = 0;
                break;

            case 0x1d:	//CTRL_L:
                ctrl_l = make;
                key = 0;
                break;

            case 0x38:	//ALT_L:
                alt_l = make;
                key = 0;
                break;

            default:
                if(!make)
                    key = 0;
                break;
        }

        if(key){
            while (isFull(&deviceTable.devices[0]->outBuffer)){
                deviceTable.devices[0]->read(0,1,1);
            }
            deviceTable.devices[0]->write(0,&key,1,1);
        }
    }
}

//系统调用
unsigned long sys_no(unsigned long vir1,unsigned long vir2,unsigned long vir3,unsigned long vir4,unsigned long vir5){
    color_printk(RED,WHITE,"no syscall \n");
    return 0;
}
//输出 1
unsigned long sys_print(unsigned int FRcolor,unsigned int BKcolor,char * string,unsigned long vir1,unsigned long vir2){
    color_printk(FRcolor,BKcolor,string,vir1,vir2);
    return 0;
}
//任务运行结束，任务退出1
unsigned long sys_taskExit(){
    //将当前进程插入到等待队列，等待删除
    reetrantlock();
    struct Task * nowRunningTask = taskManage.running->next;
    //关中断，原子操作
  //  color_printk(RED,WHITE,"cli \n");

    deleteTargetTask(nowRunningTask);
   // color_printk(RED,WHITE,"wit:%#018lx \n",nowRunningTask,taskManage.waits->prev);
    insertTaskBehand(taskManage.waits->prev,nowRunningTask);
    //开中断，原子操作完成
    reetrantUnLock();
 //   color_printk(RED,WHITE,"sti \n");
    //等待时钟中断切换到下一个任务
    while(True);
}

//判断键盘是否有键位按下,没有则立即返回，2
unsigned long sys_kerBoradIsPressAndReturn(){
    reetrantlock();
    struct Task * task = taskManage.running->prev;
    struct Window * topWindow = windows.windows->prev;
    reetrantUnLock();
    //只有当前任务是顶部窗口时，才会获取键盘输入。
    if(task->window != topWindow) return NO;
    if(!keyBufferIsEmpty()){
        unsigned char c = deviceTable.devices[0]->read(0,1,1);
        return c;
    }
    return NO;
}
//scanf系统调用,3
unsigned long sys_scanf(){
    while(keyBufferIsEmpty()){
        ;
    }
    unsigned char c = deviceTable.devices[0]->read(0,1,1);
    return c;
}

//添加一个窗口,4
unsigned long sys_AddAndShowMainWindow(long x,long y,long xlength,long ylength,char * name){
    reetrantlock();
    struct Task * nowtask = taskManage.running->prev;
    unsigned int i;
    addTaskWindow(nowtask,x,y,xlength,ylength,name);
//    for(i=0;i<10000000;i++);
//    color_printk(RED,WHITE,"flush in\n");
    flushWindows(nowtask->window);
//    for(i=0;i<10000000;i++);
//    color_printk(RED,WHITE,"flush out\n");
    reetrantUnLock();
    return 0;
}


//在窗口画一个方块,5
unsigned long sys_showBlock(long x,long y,long xlength,long ylength,int color){
    //color_printk(BLUE,WHITE,"%#018lx, %#018lx,%#018lx,%#018lx\n",x,y,xlength,ylength);
    reetrantlock();
    struct Window * mainWindow = taskManage.running->prev->window;
    //color_printk(BLUE,WHITE,"1in%#018lx task%#018lx\n",mainWindow,taskManage.running->prev);
    if(x+xlength>mainWindow->xlength) xlength = mainWindow->xlength - x;
    if(y+ylength+20>mainWindow->ylength) ylength = mainWindow->ylength - y - 20;
    x = mainWindow->x+x;
    y = mainWindow->y+y+20;
//    unsigned int i;
//    for(i=0;i<10000000;i++);
//    color_printk(RED,WHITE,"showOneBlock\n");
    showOneBlock(x,y,xlength,ylength,mainWindow->high,color,mainWindow);
   // color_printk(BLUE,WHITE,"2in%#018lx task%#018lx\n",mainWindow,taskManage.running->prev);
    reetrantUnLock();
    return 0;

}
//添加系统调用函数
void addSysCall(unsigned long sysfun,unsigned long num){
    systemCallTabel.fun[num] = sysfun;
}

//系统调用处理函数
unsigned long systemCall(unsigned long vector,unsigned long vir1,unsigned long vir2,unsigned long vir3,unsigned long vir4,unsigned long vir5){
    return systemCallTabel.fun[vector](vir1,vir2,vir3,vir4,vir5);
}



void initSystemCall(){
    unsigned long STAR = 0x0013000800000000;
    unsigned long LSTAR = (unsigned long )systemIn;
    unsigned long i ;
    wrmsr(0xc0000081,STAR);
    wrmsr(0xc0000082,LSTAR);
    wrmsr(0xc0000084,0x0);

    for(i=0;i<256;i++){
        addSysCall(sys_no,i);
    }
}

void systemMain(){
    color_printk(GREEN,BLACK,"system \n");
    initSystemCall();

    //添加系统调用
    addSysCall(sys_print,0);
    addSysCall(sys_taskExit,1);
    addSysCall(sys_kerBoradIsPressAndReturn,2);
    addSysCall(sys_scanf,3);
    addSysCall(sys_AddAndShowMainWindow,4);
    addSysCall(sys_showBlock,5);

    //..........
}