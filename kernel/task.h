#ifndef TASK
#define TASK


#define NEW 0
#define READY 1
#define RUNNING 2
#define WAIT 3

#include "window.h"
#include "device.h"
#include "global.h"

struct Register{
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rbx;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;
    unsigned long rdi;
    unsigned long rbp;
    unsigned long ds;
    unsigned long es;
    unsigned long rax;

    unsigned long rip;
    unsigned long cs;
    unsigned long rflagc;
    unsigned long rsp;
    unsigned long ss;
};

struct PageBuffer{
    unsigned long * buf;
    unsigned long bufferLength;
    unsigned long head;
    unsigned long tail;
};

struct Task{
    unsigned long id;
    unsigned long cr3;
    struct PageBuffer pages;
    unsigned long structPage;
    //unsigned long kernelRsp;
    unsigned long state;
    unsigned long path;
    struct Window * window;
    struct Register registers;
    struct Task * next;
    struct Task * prev;
} NullTaskReady,NullTaskRunning,NullTaskWait,NullReeTrantLock;

struct TaskManage{
    struct Task * readys;
    struct Task * running;
    struct Task * waits;
    unsigned long count;
};

struct ReeTrantLock{
    volatile unsigned long count;
    volatile struct Task * nowTask;
    volatile struct Task * waitTask;
};


//void test();
//lock 相关
void reetrantlock();
void reetrantUnLock();
//Buffer相关
void initPagesBuffer(struct PageBuffer * buffer,unsigned long bufferSize);
void backPagesBuffer(struct PageBuffer * buffer);
unsigned long isPagesEmpty(struct PageBuffer * buffer){
    if(buffer->head == buffer->tail) return True;
    return False;
}

unsigned long isPagesFull(struct PageBuffer * buffer){
    if(((buffer->tail+1)%buffer->bufferLength) == buffer->head) return True;
    return False;
}

void insertPagesBuffer(struct PageBuffer * buffer,unsigned long c){
    *(buffer->buf + buffer->tail) = c;
    buffer->tail = (buffer->tail+1)%buffer->bufferLength;
}

unsigned long  deleteAndreturnPage(struct PageBuffer * buffer){
    unsigned long  c = *(buffer->buf + buffer->head);
    buffer->head = (buffer->head+1)%buffer->bufferLength;
    return c;
}

void initTaskNode(struct Task * task,unsigned long maxUsePages);

void insertTaskBefore(struct Task * Object,struct Task * Target){
    Target->next = Object;
    Target->prev = Object->prev;
    Object->prev = Target;
    Target->prev->next = Target;
}

void insertTaskBehand(struct Task * Object,struct Task * Target){
    Target->next = Object->next;
    Target->prev = Object;
    Object->next = Target;
    Target->next->prev = Target;
}

void deleteTargetTask(struct Task * Target){
    Target->prev->next = Target->next;
    Target->next->prev = Target->prev;
    Target->prev = Target;
    Target->next = Target;
}
void closeInterrupt(){
    __asm__ __volatile__ ("cli	\n\t");
}
void openInterrupt(){
    __asm__ __volatile__ ("sti	\n\t");
}

void initAtomUserTask();
void exitNowtask();
void initUserTask(void * userTask);
void initKernelTask(void * taskMainFunvirAddress,unsigned long rsp);
void changeTask(unsigned long rsp);
void backTaskRegisterTogStick(struct Task * task,unsigned long rsp);
void saveTaskRegisterTogStick(struct Task * task,unsigned long rsp);
void taskMain();
#endif