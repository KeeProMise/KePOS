#include "device.h"
#include "printk.h"
#include "global.h"
#include "memory.h"
#include "lib.h"


struct DeviceTable deviceTable = {{0},0};


unsigned long is8042InBufReady(){
    return !(io_in8(CMDPort)&in8042buf);
}
unsigned long is8042OutBufReady(){
    return !(io_in8(CMDPort)&out8042buf);
}
//向系统注册驱动程序,返回驱动程序的id号
unsigned long registerDevice(void * deviceLoad,unsigned long bufferSize){
    struct Device * device = (struct Device *) getMemoryBlock(sizeof(struct Device));
    device->devID = deviceTable.count;
    deviceTable.devices[deviceTable.count] = device;
    void * loadfun = (void *)getMemoryBlock(4096);
    copy((unsigned long) deviceLoad,(unsigned long) loadfun,4096);
    device->load =  loadfun;
    if(device->load(device,bufferSize)){
        if(loadDeviceFunTOKernel(device)){
            device->init(device->devID);
        }
    }
    deviceTable.count++;
    return device->devID;
}
//将驱动程序代码加载到内核空间
unsigned long loadDeviceFunTOKernel(struct Device * device){
    void * fun = (void *)getMemoryBlock(4096);
    copy((unsigned long) device->init,(unsigned long) fun,4096);
    device->init = fun;
    fun = (void *)getMemoryBlock(4096);
    copy((unsigned long) device->read,(unsigned long) fun,4096);
    device->read = fun;
    fun = (void *)getMemoryBlock(4096);
    copy((unsigned long) device->write,(unsigned long) fun,4096);
    device->write = fun;
    fun = (void *)getMemoryBlock(4096);
    copy((unsigned long) device->open,(unsigned long) fun,4096);
    device->open =fun;
    fun = (void *)getMemoryBlock(4096);
    copy((unsigned long) device->close,(unsigned long) fun,4096);
    device->close =fun;
    fun = (void *)getMemoryBlock(4096);
    copy((unsigned long) device->exit,(unsigned long) fun,4096);
    device->exit = fun;
    return True;
}
//初始化Bffer
void initBuffer(struct Buffer * buffer,unsigned long bufferSize){
    buffer->bufferLength = bufferSize;
    buffer->buf = (char *)getMemoryBlock(bufferSize);
    buffer->head = 0;
    buffer->tail = 0;
}

//键盘驱动
unsigned long keyboardInit(unsigned long devID){
    color_printk(ORANGE,BLACK,"init:%#02lx \n",devID);
    unsigned long i;
    while(!is8042InBufReady());
    io_out8(CMDPort,0x60);
    while(!is8042InBufReady());
    io_out8(DataPort,initMode);
    for(i=0;i<1000;i++){
        for(i=0;i<1000;i++);
    }
    return True;
}
unsigned long keyboardOpen(unsigned long devID){
    return True;
}
unsigned char keyboardRead(unsigned long devID,unsigned long length,unsigned long InOrOutbuffer){
    unsigned long i;
    struct Buffer * buffer;
    if(InOrOutbuffer == 0)  buffer = &deviceTable.devices[devID]->inBuffer;
    else buffer = &deviceTable.devices[devID]->outBuffer;
    return deleteAndreturn(buffer);
}
unsigned long kerboardWrite(unsigned long devID,char * buf,unsigned long length,unsigned long InOrOutbuffer){
    unsigned long i;
    struct Buffer * buffer;
    if(InOrOutbuffer == 0)  buffer = &deviceTable.devices[devID]->inBuffer;
    else buffer = &deviceTable.devices[devID]->outBuffer;
    for(i=0;i<length;i++){
        char c = *(buf + i);
        insert(buffer,c);
    }
    return True;
}

unsigned long kerboardExit(unsigned long devID){
    return True;
}
unsigned long kerboardClose(unsigned long devID){
    color_printk(ORANGE,BLACK,"Keyboard exit \n");
    return True;
}

unsigned long kerboardLoad(struct Device * device,unsigned long bufferSize){
    device->init = keyboardInit;
    device->open = keyboardOpen;
    device->read = keyboardRead;
    device->write = kerboardWrite;
    device->exit = kerboardExit;
    device->close = kerboardClose;
    initBuffer(&device->inBuffer,bufferSize);
    initBuffer(&device->outBuffer,bufferSize);
    return True;
}

//鼠标驱动
unsigned long mouseInit(unsigned long devID){
    color_printk(ORANGE,BLACK,"init:%#02lx \n",devID);
    unsigned long i;
    while(!is8042InBufReady());
    io_out8(CMDPort,0xa8);
    for(i=0;i<1000;i++){
        for(i=0;i<1000;i++);
    }
    while(!is8042InBufReady());
    io_out8(CMDPort,0xd4);
    while(!is8042InBufReady());
    io_out8(DataPort,0xf4);
    for(i=0;i<1000;i++){
        for(i=0;i<1000;i++);
    }
    while(!is8042InBufReady());
    io_out8(CMDPort,0x60);
    while(!is8042InBufReady());
    io_out8(DataPort,0x47);
    for(i=0;i<1000;i++){
        for(i=0;i<1000;i++);
    }
    return True;
}

unsigned long mouseOpen(unsigned long devID){
    return True;
}

unsigned char mouseRead(unsigned long devID,unsigned long length,unsigned long InOrOutbuffer){
    unsigned long i;
    struct Buffer * buffer;
    if(InOrOutbuffer == 0)  buffer = &deviceTable.devices[devID]->inBuffer;
    else buffer = &deviceTable.devices[devID]->outBuffer;
    return deleteAndreturn(buffer);
}
unsigned long mouseWrite(unsigned long devID,char * buf,unsigned long length,unsigned long InOrOutbuffer){
    unsigned long i;
    struct Buffer * buffer;
    if(InOrOutbuffer == 0)  buffer = &deviceTable.devices[devID]->inBuffer;
    else buffer = &deviceTable.devices[devID]->outBuffer;
    for(i=0;i<length;i++){
        char c = *(buf + i);
        insert(buffer,c);
    }

    return True;
}

unsigned long mouseExit(unsigned long devID){
    return True;
}
unsigned long mouseClose(unsigned long devID){
    return True;
}

unsigned long mouseLoad(struct Device * device,unsigned long bufferSize){
    device->init = mouseInit;
    device->open = mouseOpen;
    device->read = mouseRead;
    device->write = mouseWrite;
    device->exit = mouseExit;
    device->close = mouseClose;
    initBuffer(&device->inBuffer,bufferSize);
    initBuffer(&device->outBuffer,bufferSize);
    return True;
}
//磁盘驱动











void deviceMain(){
    color_printk(RED,BLACK,"device \n");
    registerDevice(kerboardLoad,32);
    registerDevice(mouseLoad,128);

    //初始化时钟频率为100hz
    io_out8(TIMER_MODE, RATE_GENERATOR);
    io_out8(TIMER0, (unsigned char)TIMER_FREQ/HZ);
    io_out8(TIMER0, ((TIMER_FREQ/HZ)>>8));
}