

#include "lib.h"
#include "printk.h"

#include "trap.h"
#include "memory.h"
#include "global.h"
#include "window.h"
#include "task.h"
#include "device.h"
#include "system.h"

unsigned long SVGA_Virtual_Address = 0;
void Start_Kernel(void)
{
    SVGA_Virtual_Address = 0Xffff800000a00000;
    unsigned long SVGA = SVGA_Virtual_Address;
    int *addr = (int *)SVGA;
	int i;

	Pos.testX = 0;
	Pos.textY = 0;
	Pos.XResolution = WINDOW_WIDTH;
	Pos.YResolution = WINDOW_HIGH;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	Pos.FB_addr = (int *)SVGA;
	Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4);

	for(i = 0 ;i<WINDOW_WIDTH*20;i++)
	{
		*((char *)addr+0)=(char)0x00;
		*((char *)addr+1)=(char)0x00;
		*((char *)addr+2)=(char)0xff;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<WINDOW_WIDTH*20;i++)
	{
		*((char *)addr+0)=(char)0x00;
		*((char *)addr+1)=(char)0xff;
		*((char *)addr+2)=(char)0x00;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<WINDOW_WIDTH*20;i++)
	{
		*((char *)addr+0)=(char)0xff;
		*((char *)addr+1)=(char)0x00;
		*((char *)addr+2)=(char)0x00;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<WINDOW_WIDTH*20;i++)
	{
		*((char *)addr+0)=(char)0xff;
		*((char *)addr+1)=(char)0xff;
		*((char *)addr+2)=(char)0xff;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}

	memoryMain();

    deviceMain();

    trapMain();

    systemMain();


    windowMain();
    //task必须在中断之前
    taskMain();

    interruptMain();




	while(True){
	    //color_printk(RED,WHITE,"MAIN running\n");
	    //处理鼠标移动
//	    if(!mouseBufferIsEmpty()){
//            analysisMouse();
//	    }
        ;
	}


}
