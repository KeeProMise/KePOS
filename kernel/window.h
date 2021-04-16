#ifndef WINDOW
#define WINDOW

#include "task.h"
#include "global.h"


#define NameWinColor 0x00ADD8E6
#define TitleWinColor 0x00E0FFFF
#define TitleColor 0x00F0FFFF
#define BackWinColor 0x0000BFFF
#define BackColor  0x00D3D3D3
#define WordColor 0x001C1C1C
#define MOUSECOLOR 0x006600cc

struct Mouse{
    long x;
    long y;
    unsigned int color;
    unsigned int high;
};

struct Window{
    long x;
    long y;
    long xlength;
    long ylength;
    unsigned int high;
    char name[10] ;
    struct Task * task;
    unsigned long windata;
    struct Window * prev;
    struct Window * next;
};

struct Windows{
    struct Window * windows;
    struct Mouse mouse;
    //当前最大的high是多少
    unsigned int top;
    unsigned long highData;
};

void insertWindowBefore(struct Window * Object,struct Window * Target){
    Target->next = Object;
    Target->prev = Object->prev;
    Object->prev = Target;
    Target->prev->next = Target;
}

void insertWindowBehand(struct Window * Object,struct Window * Target){
    Target->next = Object->next;
    Target->prev = Object;
    Object->next = Target;
    Target->next->prev = Target;
}

void deleteTargetWindow(struct Window * Target){
    Target->prev->next = Target->next;
    Target->next->prev = Target->prev;
    Target->prev = Target;
    Target->next = Target;
}

void flushWindows(struct Window * window);
void deleteTaskWindow(struct Window * window);
void changeWindow();
struct Window * getTopWindow();
struct Window * witchWindowIsPause(long x,long y);
void modifyHighMata(long oldx,long oldy,long xlength,long ylength);
void copyWindowToSVGA(struct Window * window);
void movWindowAndMouse(struct Window *window,long x,long y);
void loadShowWinData(struct Window * window,long x,long y,long xlength,long ylength);
void flushBlock(long x,long y,long xlength,long ylength,struct Window * window);
void movMouse(long x,long y);
void fluahAllWindow();
void showTaskWindow(struct Window * window);
void addTaskWindow(struct Task * task,long x,long y,long xlength,long ylength,char * name);
void showMouse(struct Mouse * mouse);
void showOneChar(char c,long x,long y,unsigned long high,unsigned long Pcolor,unsigned long Bcolor,struct Window *window);
void showOnePoint(long x,long y,unsigned long high,unsigned int color,struct Window *window);
void showOneBlock(long x,long y,long xlength,long ylength,unsigned long high,unsigned int color,struct Window *window);
void showMainWindow(struct Window * window);
void showName(long x,long y,char * name,unsigned int length,unsigned long high,unsigned int wordColor,unsigned int backColor,struct Window *window);
void initWindows();
void windowMain();
#endif