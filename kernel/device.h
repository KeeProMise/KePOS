#ifndef DEVICE
#define DEVICE

#include "global.h"
#include "memory.h"
#include "lib.h"
#define DeviceClass 255


//时钟相关

#define TIMER0          0x40	/* I/O port for timer channel 0 */
#define TIMER_MODE      0x43	/* I/O port for timer mode control */
#define RATE_GENERATOR	0x34	/* 00-11-010-0 : Counter0 - LSB then MSB - rate generator - binary */
#define TIMER_FREQ	1193182L// 时钟周期的频率
#define HZ		100	// 时钟中断的频率


//8042控制器相关
#define DataPort 0x60
#define CMDPort 0x64
#define initMode 0x47
#define in8042buf 0x02
#define out8042buf 0x01


//键盘相关


unsigned long is8042InBufReady();
unsigned long is8042OutBufReady();
//队列
//head出队
//tail入队
struct Buffer{
    char * buf;
    unsigned long bufferLength;
    unsigned long head;
    unsigned long tail;
};

struct Device{
    unsigned long devID;
    //输入到cpu的buffer ----->CPU
    struct Buffer inBuffer;
    //输出到设备的buffer ----->设备
    struct Buffer outBuffer;
    //返回设备号
    unsigned long (* load)(struct Device *,unsigned long );
    unsigned long (* init)(unsigned long);
    unsigned long (* open)(unsigned long);
    //2:输入的字符，3:字符长度 4:(0)inbuffer (1)outbuffer
    unsigned long (* write)(unsigned long,char * ,unsigned long,unsigned long);
    //2:字符长度 3:(0)inbuffer (1)outbuffer
    char (* read)(unsigned long,unsigned long,unsigned long);
    //返回设备号
    unsigned long (* exit)(unsigned long);
    unsigned long (* close)(unsigned long);
};

struct DeviceTable{
    struct Device * devices[DeviceClass];
    unsigned long count;
};


//Buffer相关
void initBuffer(struct Buffer * buffer,unsigned long bufferSize);
unsigned long isEmpty(struct Buffer * buffer){
    if(buffer->head == buffer->tail) return True;
    return False;
}

unsigned long isFull(struct Buffer * buffer){
    if(((buffer->tail+1)%buffer->bufferLength) == buffer->head) return True;
    return False;
}

void insert(struct Buffer * buffer,char c){
    *(buffer->buf + buffer->tail) = c;
    buffer->tail = (buffer->tail+1)%buffer->bufferLength;
}

char deleteAndreturn(struct Buffer * buffer){
    char c = *(buffer->buf + buffer->head);
    buffer->head = (buffer->head+1)%buffer->bufferLength;
    return c;
}
unsigned long loadDeviceFunTOKernel(struct Device * device);
unsigned long registerDevice(void * deviceLoad,unsigned long bufferSize);
void deviceMain();
#endif