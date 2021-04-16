#include "printk.h"
#include "window.h"
#include "global.h"
#include "task.h"
#include "font.h"

struct Windows windows = {NULL,{0,0},0,0};



//删除task的窗口，归还内存
void deleteTaskWindow(struct Window * window){
    reetrantlock();
    //归还winData内存
    unsigned long windata = window->windata;
    backOnePage(windata);
    //在windows结构体中删除该window;
    deleteTargetWindow(window);
    //归还wid结构体占用的空间
    unsigned long wid = window;
    backMemoryBlock(wid);
    reetrantUnLock();
}
//交换最上面的窗口
void changeWindow(){
   // reetrantlock();
    struct Mouse * mouse = &windows.mouse;

    struct Window * topWidow = getTopWindow();
    struct Window * PressWidow = witchWindowIsPause(mouse->x,mouse->y);

    if(PressWidow !=NO && PressWidow != windows.windows){
//        deleteTargetWindow(topWidow);
//        insertWindowBehand(PressWidow,topWidow);
        deleteTargetWindow(PressWidow);
        insertWindowBehand(windows.windows->prev,PressWidow);
   //     topWidow->high = PressWidow->high;
        windows.top++;
        PressWidow->high = windows.top;
       // modifyHighMata(topWidow->x,topWidow->y,topWidow->xlength,topWidow->ylength);
       // flushBlock(topWidow->x,topWidow->y,topWidow->xlength,topWidow->ylength,topWidow);
       // copyWindowToSVGA(topWidow);
       // modifyHighMata(PressWidow->x,PressWidow->y,PressWidow->xlength,PressWidow->ylength);
        flushBlock(PressWidow->x,PressWidow->y,PressWidow->xlength,PressWidow->ylength,PressWidow);
        copyWindowToSVGA(PressWidow);
    }
}
//返回最顶部的窗口
struct Window * getTopWindow(){
    reetrantlock();
    struct Window * reWin = windows.windows->prev;
    reetrantUnLock();
    return reWin;
}
//判断哪个窗口被鼠标点击了
struct Window * witchWindowIsPause(long x,long y){
    //reetrantlock();
    unsigned int * highData = (unsigned int *)windows.highData + x + y*WINDOW_WIDTH;
    //color_printk(BLUE,RED,"high%#018lx \n",*highData);
    unsigned int nowHighData = (*highData);
    if(nowHighData == 1){
        //reetrantUnLock();
        return windows.windows;
    }
    struct Window * now = windows.windows->next;
    for(;now != windows.windows; now = now->next){
        if(now->high == nowHighData){
           // reetrantUnLock();
            return now;
        }
    }
   // reetrantUnLock();
    return NO;
}
//movMouse
void movMouse(long x,long y){

    struct Mouse * mouse = &windows.mouse;
    long oldx = mouse->x;
    long oldy = mouse->y;
    mouse->x = mouse->x + x;
    mouse->y = mouse->y - y;
    if(mouse->x > WINDOW_WIDTH-8)  mouse->x = WINDOW_WIDTH-8;
    if(mouse->x < 0) mouse->x = 0;
    if(mouse->y > WINDOW_HIGH-16) mouse->y = WINDOW_HIGH-16;
    if(mouse->y < 0) mouse->y = 0;

    modifyHighMata(oldx,oldy,8,16);
    showMouse(mouse);
    flushBlock(oldx,oldy,8,16,NO);

}

void movWindowAndMouse(struct Window *window,long x,long y){
    if(window == witchWindowIsPause(windows.mouse.x,windows.mouse.y)) {
        long oldx = window->x;
        long oldy = window->y;
        window->x = window->x + x;
        window->y = window->y - y;
        if(window->x < 0) window->x = 0;
        if(window->x > WINDOW_WIDTH - window->xlength) window->x = WINDOW_WIDTH - window->xlength;
        if(window->y < 0) window->y = 0;
        if(window->y > WINDOW_HIGH - window->ylength) window->y = WINDOW_HIGH - window->ylength;

        long movx = window->x - oldx;
        long movy = window->y - oldy;
        if(movx < 0) movx = 0-movx;
        if(movy < 0) movy = 0-movy;
        modifyHighMata(oldx,oldy,window->xlength,window->ylength);
        copyWindowToSVGA(window);
        flushBlock(oldx,oldy,window->xlength,window->ylength,window);
    }
}
//在屏幕上显示一个块，会改变highData中的高度值
void showOneBlock(long x,long y,long xlength,long ylength,unsigned long high,unsigned int color,struct Window *window){
    //color_printk(BLUE,WHITE,"%#018lx, %#018lx,%#018lx,%#018lx,high%#018lx,window:%018lx\n",x,y,xlength,ylength,high,window);
    long i;
    long j;
    for(i=x;i<x+xlength;i++){
        for(j=y;j<y+ylength;j++){
            showOnePoint(i,j,high,color,window);
        }
    }
}

//显示一个点，会改变highData中的高度值,并且将color写入当前窗口winData中
void showOnePoint(long x,long y,unsigned long high,unsigned int color,struct Window *window){
    if(x<0) x = 0;
    if(y<0) y = 0;
    if(x>WINDOW_WIDTH) x = WINDOW_WIDTH;
    if(y>WINDOW_HIGH) y = WINDOW_HIGH;
    unsigned int * highData = (unsigned int *)windows.highData;
    unsigned int * nowShow = (unsigned int *)SVGA_Virtual_Address;
    highData = highData + x + y*WINDOW_WIDTH;
    nowShow = nowShow + x + y*WINDOW_WIDTH;
    //只有高度大于已经显示的高度，才显示
    if((*highData) <= high ){
        (* nowShow) = color;
        (* highData) = high;
    }
    if(window != NO){
        unsigned int * nowWindata = (unsigned int *)window->windata;
        nowWindata = nowWindata + (x-window->x) + (y-window->y)*WINDOW_WIDTH;
        (*nowWindata) = color;
    }
}

//显示一个字符
void showOneChar(char c,long x,long y,unsigned long high,unsigned long Pcolor,unsigned long Bcolor,struct Window *window){
    unsigned int i;
    unsigned int j;
    unsigned char d1;
    unsigned char d2;
    for(i=0;i<16;i++){
        d1 = font_ascii[c][i];
        for(j=0;j<8;j++){
            d2 = (0x80) >> j;
            if(d2&d1) showOnePoint(j+x,i+y,high,Pcolor,window);
            else showOnePoint(j+x,i+y,high,Bcolor,window);
        }
    }
}

//显示鼠标
void showMouse(struct Mouse * mouse){
    long x = mouse->x;
    long y = mouse->y;
    unsigned int i;
    unsigned int j;
    unsigned char d1;
    unsigned char d2;
    unsigned char mouseBitmap[16] = {0x00,0x80,0xC0,0xC0,0xE0,0xE0,0xF0,0xF0,0xF8,0xFC,0xFC,0xFE,0xFE,0xFF,0xE0,0x80,};
    for(i=0;i<16;i++){
        d1 = mouseBitmap[i];
        for(j=0;j<8;j++){
            d2 = (0x80) >> j;
            if(d2&d1) showOnePoint(j+x,i+y,mouse->high,mouse->color,(struct Window *)NO);
        }
    }
}
//显示name
void showName(long x,long y,char * name,unsigned int length,unsigned long high,unsigned int wordColor,unsigned int backColor,struct Window * window){
    if(length > 10) length = 10;
    unsigned int i;
    char c;
    for(i=x;i<length*8 + x;i=i+8,name++){
        c = *name;
        showOneChar(c,i,y,high,wordColor,backColor,window);
    }
}

void showMainWindow(struct Window * window){
    showOneBlock(0,0,800,600,1,BackWinColor,window);
    showOneBlock(0,0,800,20,1,TitleWinColor,window);
    showOneBlock(0,1,80,18,1,NameWinColor,window);
    showOneBlock(720,1,80,18,1,NameWinColor,window);
    showName(2,2,&window->name,10,1,WordColor,NameWinColor,window);
}

//初始化主窗口结构体
void initMainWindow(){
    struct Window * window = getMemoryBlock(sizeof (struct Window));
    window->x = 0;
    window->y = 0;
    window->high = 1;
    window->task = (unsigned long *)NO;
    window->name[0] = 'k';
    window->name[1] = 'e';
    window->name[2] = 'e';
    window->name[3] = 'P';
    window->name[4] = 'r';
    window->name[5] = 'o';
    window->name[6] = 'm';
    window->name[7] = 'i';
    window->name[8] = 's';
    window->name[9] = 'e';
    window->windata = allocateOnePage(KERNEL);
    window->xlength = WINDOW_WIDTH;
    window->ylength = WINDOW_HIGH;
    window->next = window;
    window->prev = window;
    windows.windows =  window;
    windows.top = 1;

}

//初始化并添加用户窗口
void addTaskWindow(struct Task * task,long x,long y,long xlength,long ylength,char * name){
    unsigned long i;
    reetrantlock();
    struct Window * window = getMemoryBlock(sizeof(struct Window));
    windows.top++;
    unsigned long high = windows.top;
    window->task = task;
    window->high = high;
    for(;i<10;i++,name++){
        window->name[i] = *name;
    }
    window->windata = allocateOnePage(KERNEL);
  //  color_printk(RED,WHITE,"windata%#018lx \n",window->windata);
    window->x = x;
    window->y = y;
    window->xlength = xlength;
    window->ylength = ylength;
    window->task->window = window;
    insertWindowBehand(windows.windows->prev,window);
    reetrantUnLock();
}

//初始化windows结构体
void initWindows(){
    //获取一页保存highData
    windows.highData = allocateOnePage(KERNEL);
    windows.mouse.x = 310;
    windows.mouse.y = 310;
    windows.mouse.color = INDIGO;
    windows.mouse.high = 0xffffffff;
    initMainWindow();
    showMouse(&windows.mouse);
}

//显示TaskWindow
void showTaskWindow(struct Window * window){
    showOneBlock(window->x,window->y,window->xlength,window->ylength,window->high,BackColor,window);
    showOneBlock(window->x,window->y,window->xlength,20,window->high,TitleColor,window);
    showName(window->x+2,window->y+2,&window->name,10,window->high,WordColor,TitleColor,window);
    showOneChar('X',window->x+window->xlength-10,window->y+2,window->high,BLACK,TitleColor,window);
}

void loadShowWinData(struct Window * window,long x,long y,long xlength,long ylength){
    unsigned int * nowWindata = 0;

    long i;
    long j;
    for(i=x;i<x+xlength;i++){
        for(j=y;j<y+ylength;j++){
            //只加载刷新在范围内的
           // color_printk(RED,BLACK,"[%#03lx,%#03lx,%#03lx,%#03lx:%#03lx,%#03lx] \n",window->x,window->y,window->xlength,window->ylength,i,j);
            if(i>=window->x && i<(window->x+window->xlength) && j>=window->y && j<(window->y+window->ylength)){
                nowWindata = (unsigned int *)window->windata + (i-window->x) + (j-window->y)*WINDOW_WIDTH;
                showOnePoint(i,j,window->high,(*nowWindata),NO);
            }
        }
    }
}

//刷新除了主窗口的所有窗口
void flushWindows(struct Window * window){
    reetrantlock();
//    struct Window * now = windows.windows->next;
//    for(;now != windows.windows;now = now->next){
//        showTaskWindow(now);
//        copyWindowToSVGA(now);
//    }
    showTaskWindow(window);
    reetrantUnLock();
}

//刷新一个块,直到遇到window
void flushBlock(long x,long y,long xlength,long ylength,struct Window * window){
   // color_printk(RED,BLACK,"[top%#04lx,moH%04lx]",windows.top,windows.mouse.high);
    //reetrantlock();
    struct Window * now = windows.windows;
    if(window == NO){
        loadShowWinData(now,x,y,xlength,ylength);
        now = now->next;
        for(;now != windows.windows;now = now->next){
            loadShowWinData(now,x,y,xlength,ylength);
        }
    }else{
        for(;now != window;now = now->next){
            loadShowWinData(now,x,y,xlength,ylength);
        }
    }
    //reetrantUnLock();
}
void copyWindowToSVGA(struct Window * window){
    //reetrantlock();
    unsigned int * nowWindata = window->windata;
    long i;
    long j;
    for(i=window->x;i<window->xlength+window->x;i++){
        for(j=window->y;j<window->ylength+window->y;j++){
            nowWindata = (unsigned int *)window->windata + (i-window->x) + (j-window->y)*WINDOW_WIDTH;
            showOnePoint(i,j,window->high,(*nowWindata),NO);
        }
    }
   // reetrantUnLock();
}
//刷新所有的窗口
void fluahAllWindow(){
    //reetrantlock();
    //modifyHighMata(0,0,WINDOW_WIDTH,WINDOW_HIGH);
    struct Window * now = windows.windows;
    copyWindowToSVGA(now);
    now = now->next;
    for(;now != windows.windows;now = now->next){
        color_printk(RED,BLUE,"window:%#018lx\n",now);
        copyWindowToSVGA(now);
    }
   // reetrantUnLock();
}
//修改当前块的高度为0
void modifyHighMata(long oldx,long oldy,long xlength,long ylength){
  //  reetrantlock();
    unsigned int * hignData = 0;
    long i;
    long j;
    for(i=0;i<xlength;i++){
        for(j = 0;j<ylength;j++){
            hignData = (unsigned int *) windows.highData + oldx+i + (j+oldy)*WINDOW_WIDTH;
            (* hignData) = 0;
        }
    }
   // reetrantUnLock();
}
void windowMain(){
    color_printk(WHITE,BLACK,"window \n");
    initWindows();
    showMainWindow(windows.windows);
}