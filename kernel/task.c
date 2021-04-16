#include "task.h"
#include "printk.h"
#include "global.h"
#include "memory.h"
#include "trap.h"
#include "lib.h"

struct TaskManage taskManage = {&NullTaskReady,&NullTaskRunning,&NullTaskWait,1};
volatile struct  ReeTrantLock reeTrantLock = {0,NULL,&NullReeTrantLock};
//static int volatile * itest  = 0;
//
//void test11(){
//    *itest = 0;
//    *itest ++;
//    int i  = *itest;
//    i++;
//}


//加锁
void reetrantlock(){
    //原子操作
    cli();

    //如果获得锁的任务不是当前任务，则判断锁是否可以被当前任务获取
    struct Task * noTask = taskManage.running->prev;
    int i;
    if(reeTrantLock.nowTask != noTask){
        //当前程序获取到锁
        if(reeTrantLock.count == 0){
            reeTrantLock.count ++;
            reeTrantLock.nowTask = noTask;
        }else{
            //当前线程获取锁失败，自旋。
            while(reeTrantLock.count != 0){
                sti();
                for(i = 0;i<1000;i++);
                cli();
            }
            //获得到锁
            reeTrantLock.count ++;
            reeTrantLock.nowTask = noTask;
        }
    }else{
        reeTrantLock.count ++;
    }
    sti();
}

//解锁
void reetrantUnLock(){
    //原子操作
    cli();
    //给当前锁的引用计数减去1
    reeTrantLock.count --;
    //如果当前锁的计数为0，则唤醒一个等待的任务
    if(reeTrantLock.count == 0){
        reeTrantLock.nowTask = NULL;
    }
    sti();
}
void initPagesBuffer(struct PageBuffer * buffer,unsigned long bufferSize){
    buffer->bufferLength = bufferSize;
    buffer->buf = (unsigned long *)getMemoryBlock(bufferSize);
    buffer->head = 0;
    buffer->tail = 0;
}
void backPagesBuffer(struct PageBuffer * buffer){
    backMemoryBlock(buffer->buf);
}
void initTaskNode(struct Task * task,unsigned long maxUsePages){
    task->next = task;
    task->prev = task;
    set(&task->registers,0x00,sizeof(struct Register));
    task->window = NULL;
    task->state = NEW;
    initPagesBuffer(&task->pages,maxUsePages);
}

//传入任务的第一个代码的线性地址
void initUserTask(void * userTask){

    //初始化一个task节点
    struct Task * newTask = (struct Task *)getMemoryBlock(sizeof(struct Task));
    initTaskNode(newTask,64);
    //分配新的页基地址
    unsigned long newCR3 = (struct Task *)getMemoryBlock(4096);
    newTask->cr3 = KernelVirAddressToPhyAddress(newCR3);

    //分配2页，并获取其物理地址
    unsigned long newPageID1 =  findFreeBlockInBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, 1);
    modifyBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, newPageID1);
    unsigned long phyAddress1 = getPageByPageID(newPageID1)->physicsAddress;
    set((unsigned int *)KernalPhyAddressToVirAddress(phyAddress1),(char)0x00,PAGE_2M_SIZE);
    setPageAttribute(getPageByPageID(newPageID1),1,USER);

    unsigned long newPageID2 =  findFreeBlockInBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, 1);
    modifyBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, newPageID2);
    unsigned long phyAddress2 = getPageByPageID(newPageID2)->physicsAddress;
    set((unsigned int *)KernalPhyAddressToVirAddress(phyAddress2),(char)0x00,PAGE_2M_SIZE);
    setPageAttribute(getPageByPageID(newPageID2),1,USER);

    //将新的页插入buffer。
    insertPagesBuffer(&newTask->pages,newPageID1);
    newTask->structPage = newPageID2;
    //insertPagesBuffer(&newTask->pages,newPageID2);

    //设置页映射
    unsigned long rspVirAdd = 0x00007fffffffffff;
    unsigned long ripVirAdd = 0x00007fffffe00000;
    virPageTOPhyPage(ripVirAdd,phyAddress2,newTask->cr3,7);
    virPageTOPhyPage(0x0000000000000000,phyAddress1,newTask->cr3,7);

    //copy程序到对应的用户页面
   // copy((unsigned long)userTaskStart,KernalPhyAddressToVirAddress(phyAddress2),200000);
    copy((unsigned long)userTask,KernalPhyAddressToVirAddress(phyAddress1),200000);
    //设置任务的寄存器
    newTask->registers.cs = 0x23;
    newTask->registers.rip = 0x0000000000000000;
    newTask->registers.ss = 0x1b;
    newTask->registers.ds = 0x1b;
    newTask->registers.es = 0x1b;
    newTask->registers.rsp = rspVirAdd;
    newTask->path = NULL;
    newTask->registers.rbp = newTask->registers.rsp;
    newTask->registers.rflagc |= (1UL << 9);

    newTask->id = taskManage.count;
    taskManage.count += 1;
    insertTaskBehand(taskManage.readys->prev,newTask);
 //   color_printk(BLUE,WHITE,"task:%#018lx\n",taskManage.readys->prev);
    newTask->state = READY;
    newTask->window = NULL;
    //初始化该任务的内核态页表
    initKernelPage(newTask->cr3);

}

//初始化一个内核任务 rsp为内核任务的栈基址
void initKernelTask(void * taskMainFunvirAddress,unsigned long rsp){

    //初始化一个task节点
    struct Task * newTask = (struct Task *)getMemoryBlock(sizeof(struct Task));
    initTaskNode(newTask,64);
    //分配新的页基地址
    newTask->cr3 = 0x100000;

    unsigned long phyAddress = KernelVirAddressToPhyAddress(taskMainFunvirAddress);
    unsigned long PageID = (phyAddress - 0x200000)/PAGE_2M_SIZE;
    //将新的页插入buffer。
    insertPagesBuffer(&newTask->pages,PageID);
    //初始化任务的寄存器
    newTask->registers.cs = 0x08;
    newTask->registers.rip = (unsigned long)taskMainFunvirAddress;
    newTask->registers.ss = 0x10;
    newTask->registers.ds = 0x10;
    newTask->registers.es = 0x10;
    newTask->registers.rsp = rsp;
    newTask->path = (unsigned long)taskMainFunvirAddress;
    newTask->registers.rbp = newTask->registers.rsp;
    newTask->registers.rflagc |= (1UL << 9);
    //初始化任务id
    newTask->id = taskManage.count;
    taskManage.count += 1;
    //初始化该任务的页表

//    virPageTOPhyPage((unsigned long)taskMainFunvirAddress,phyAddress,newTask->cr3,3);

    //将Task1内核任务，加入就绪队列
    insertTaskBehand(taskManage.readys->prev,newTask);
    newTask->state = READY;

}

//任务切换，输入当前栈指针
void changeTask(unsigned long rsp){

    //如果没有就绪的任务，直接返回
    if((taskManage.readys->prev) == &NullTaskReady){
        return;
    }
    //如果有就绪，但是没有运行中的任务，则直接切换。
    if((taskManage.running->prev) == &NullTaskRunning){
        struct Task * nowTask = taskManage.readys->next;
        deleteTargetTask(nowTask);
        insertTaskBehand(taskManage.running->prev,nowTask);
        nowTask->state = RUNNING;
        backTaskRegisterTogStick(nowTask,rsp);
        modifyCR3(nowTask->cr3);
    }else{
        //两个队列都不为空,把就绪队列的第一个加入running队列，把running的第一个移到就绪队列。
        struct Task * nowReadTask = taskManage.readys->next;
        struct Task * nowRunningTask = taskManage.running->next;
        deleteTargetTask(nowReadTask);
        deleteTargetTask(nowRunningTask);
        insertTaskBehand(taskManage.readys->prev,nowRunningTask);
        insertTaskBehand(taskManage.running->prev,nowReadTask);
        nowRunningTask->state = READY;
        nowReadTask->state = RUNNING;
        saveTaskRegisterTogStick(nowRunningTask,rsp);
        backTaskRegisterTogStick(nowReadTask,rsp);
        modifyCR3(nowReadTask->cr3);
    }

}

void exitNowtask(struct Task * nowRunningTask){
    reetrantlock();
    deleteTargetTask(nowRunningTask);
    //归还当前task获得的所有页面
    unsigned long pageId = nowRunningTask->structPage;
    modifyBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, pageId);
    setPageAttribute(getPageByPageID(pageId),1,0);
    while(!isPagesEmpty(&nowRunningTask->pages)){
        pageId = deleteAndreturnPage(&nowRunningTask->pages);
        modifyBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, pageId);
        setPageAttribute(getPageByPageID(pageId),1,0);
    }
    //归还CR3占用的空间
    backCR3page(nowRunningTask->cr3);
    //归还pagsbuffer
    backPagesBuffer(&nowRunningTask->pages);
    //归还该task结构体占用空间
    backMemoryBlock(nowRunningTask);
    //归还窗口占用的空间
    if(nowRunningTask->window->windata != NULL) deleteTaskWindow(nowRunningTask->window);
    reetrantUnLock();
}
//加任务的寄存器值加载到当前栈中
void backTaskRegisterTogStick(struct Task * task,unsigned long rsp){
    * (unsigned long *)(rsp+R15) = task->registers.r15;
    * (unsigned long *)(rsp+R14) = task->registers.r14;
    * (unsigned long *)(rsp+R13) = task->registers.r13;
    * (unsigned long *)(rsp+R12) = task->registers.r12;
    * (unsigned long *)(rsp+R11) = task->registers.r11;
    * (unsigned long *)(rsp+R10) = task->registers.r10;
    * (unsigned long *)(rsp+R9) = task->registers.r9;
    * (unsigned long *)(rsp+R8) = task->registers.r8;
    * (unsigned long *)(rsp+RAX) = task->registers.rax;
    * (unsigned long *)(rsp+RBP) = task->registers.rbp;
    * (unsigned long *)(rsp+RBX) = task->registers.rbx;
    * (unsigned long *)(rsp+RCX) = task->registers.rcx;
    * (unsigned long *)(rsp+RDI) = task->registers.rdi;
    * (unsigned long *)(rsp+RSI) = task->registers.rsi;
    * (unsigned long *)(rsp+RDX) = task->registers.rdx;
    * (unsigned long *)(rsp+RFLAGS) = task->registers.rflagc;
    * (unsigned long *)(rsp+RIP) = task->registers.rip;
    * (unsigned long *)(rsp+DS) = task->registers.ds;
    * (unsigned long *)(rsp+ES) = task->registers.es;
    * (unsigned long *)(rsp+CS) = task->registers.cs;
    * (unsigned long *)(rsp+SS) = task->registers.ss;
    * (unsigned long *)(rsp+RSP) = task->registers.rsp;
}
//将栈中内容保存到任务中
void saveTaskRegisterTogStick(struct Task * task,unsigned long rsp){
    task->registers.r15 = * (unsigned long *)(rsp+R15);
    task->registers.r14 = * (unsigned long *)(rsp+R14) ;
    task->registers.r13 = * (unsigned long *)(rsp+R13);
    task->registers.r12 = * (unsigned long *)(rsp+R12) ;
    task->registers.r11 = * (unsigned long *)(rsp+R11) ;
    task->registers.r10 = * (unsigned long *)(rsp+R10);
    task->registers.r9 = * (unsigned long *)(rsp+R9);
    task->registers.r8 = * (unsigned long *)(rsp+R8);
    task->registers.rax = * (unsigned long *)(rsp+RAX);
    task->registers.rbp = * (unsigned long *)(rsp+RBP);
    task->registers.rbx = * (unsigned long *)(rsp+RBX);
    task->registers.rcx = * (unsigned long *)(rsp+RCX);
    task->registers.rdi = * (unsigned long *)(rsp+RDI);
    task->registers.rsi = * (unsigned long *)(rsp+RSI);
    task->registers.rdx = * (unsigned long *)(rsp+RDX);
    task->registers.rflagc = * (unsigned long *)(rsp+RFLAGS);
    task->registers.rip = * (unsigned long *)(rsp+RIP);
    task->registers.ds = * (unsigned long *)(rsp+DS);
    task->registers.es = * (unsigned long *)(rsp+ES);
    task->registers.cs = * (unsigned long *)(rsp+CS);
    task->registers.ss = * (unsigned long *)(rsp+SS);
    task->registers.rsp = * (unsigned long *)(rsp+RSP);
}

void mouseTaskMain(){
    unsigned long i = 0;
    unsigned long now = 0;
    while(True){
        //处理鼠标移动
	    if(!mouseBufferIsEmpty()){

            analysisMouse();

	    }
    }
}
void keyboardTaskMain(){
    unsigned long i = 0;
    unsigned long now = 0;
    while(True){
        //处理键盘输入
        if (!keyBoardBufferIsEmpty()){
            analysis_keycode();
        }
    }
}
void task2Main(){
    unsigned long i = 0;
    unsigned long now = 0;
    while (True){
//        if (!keyBufferIsEmpty()){
//            unsigned char key = deviceTable.devices[KEYID]->read(0,1,1);
//            color_printk(RED,YELLOW,"(K:%c)\t",key);
//        }
    }
}

void initAtomUserTask(){
    reetrantlock();
    initUserTask(0xffff8000d0000000);
    reetrantUnLock();
}

//测试窗口
void task3Main(){
    unsigned long i = 0;
    unsigned long now = 0;

    struct Task * nowtask = taskManage.running->prev;
    char name[10] = {'m','t','e','s','t','A','P','P',' ',' '};
//    addTaskWindow(nowtask,300,300,400,300,&name);
//    flushWindows(nowtask->window->high,nowtask->window->high);

    while (True){
//        for(i=0;i<1000000000;i++);
//        initAtomUserTask();
//        initAtomUserTask();
//        initAtomUserTask();
//        color_printk(WHITE,ORANGE,"task3 running:%#018lx \n",now);
        now++;

    }
}
void task4Main(){
    unsigned long i = 0;
    unsigned long now = 0;

    struct Task * nowtask = taskManage.running->prev;
    char name[10] = {'m','t','e','s','t','A','P','P','1',' '};
//    addTaskWindow(nowtask,100,100,400,300,&name);
//    flushWindows(nowtask->window->high,nowtask->window->high);
//    copyWindowToSVGA(nowtask->window);
    //copy(nowtask->window->windata,SVGA_Virtual_Address,PAGE_2M_SIZE);

    //copy(windows.windows[1].windata,SVGA_Virtual_Address,PAGE_2M_SIZE);
    while (True){
//        for(i=0;i<1000000000;i++);
//        initAtomUserTask();
//        initAtomUserTask();
//        initAtomUserTask();
//        color_printk(WHITE,ORANGE,"task3 running:%#018lx \n",now);
        now++;

    }
}
//该任务负责清除wait队列中任务
void cleanTask(){
    unsigned long i = 0;
    unsigned long now = 0;

    while (True){
        while(taskManage.waits->prev != &NullTaskWait){
            exitNowtask(taskManage.waits->prev);
        }
    }
}


void taskMain(){
    NullTaskReady.next = &NullTaskReady;
    NullTaskRunning.next = &NullTaskRunning;
    NullTaskWait.next = &NullTaskWait;
    NullTaskWait.prev = &NullTaskWait;
    NullTaskRunning.prev = &NullTaskRunning;
    NullTaskReady.prev = &NullTaskReady;
    NullReeTrantLock.prev = &NullReeTrantLock;
    NullReeTrantLock.next = &NullReeTrantLock;

    //初始化内核进程
    initKernelTask(mouseTaskMain,0x90000 + 0xffff800000000000);
    initKernelTask(task2Main,0x8f000 + 0xffff800000000000);
    initKernelTask(task3Main,0x8e000 + 0xffff800000000000);
    initKernelTask(task4Main,0x8b000 + 0xffff800000000000);
    initKernelTask(cleanTask,0x8d000 + 0xffff800000000000);
    initKernelTask(keyboardTaskMain,0x8c000 + 0xffff800000000000);

    int i;
 //   initUserTask(0xffff8000d0000000);
  //    initUserTask(0xffff8000d0000000);

   // color_printk(RED,BLACK,"task \n");

}