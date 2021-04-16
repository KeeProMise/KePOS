#include "system.h"


struct Block{
    long x;
    long y;
}block;


void showBlock(long x,long y){
    drawOneBlock(x,y,20,20,BLUE);
}

void hideBlock(long x,long y){
    drawOneBlock(x,y,20,20,BLACK);
}
void main(){
    unsigned long i = 0;
    unsigned long now = 0;
    unsigned char state = 'd';
    block.x = 0;
    block.y = 0;
    char name[10] = {'G','l','u','t','t','S','n','a','k','e',};

    applyMainWindow(100,100,400,300,&name);
 //   print(RED,WHITE,"1\n",1);
    drawOneBlock(0,0,300,300,BLACK);
 //   print(RED,WHITE,"2\n",1);
    showBlock(block.x,block.y);

    while(1) {
        for(i = 0;i<100000000/2;i++);
        unsigned long c = kerboardIsPressAndReturn();
        if(c!=0xffffffffffffffff){
            c = (unsigned char)c;
            if(c == 'w' || c == 'a' || c == 'd'|| c == 's') state = c;
        }
        switch (state) {
            case 'd':
                if((block.x + 20)<300){
                    hideBlock(block.x,block.y);
                    block.x += 20;
                    showBlock(block.x,block.y);
                }else goto end;
                break;
            case 'a':
                if((block.x - 20) >= 0){
                    hideBlock(block.x,block.y);
                    block.x -= 20;
                    showBlock(block.x,block.y);
                }else goto end;
                break;
            case 'w':
                if((block.y - 20) >= 0){
                    hideBlock(block.x,block.y);
                    block.y -= 20;
                    showBlock(block.x,block.y);
                }else goto end;
                break;
            case 's':
                if((block.y + 20) < 280){
                    hideBlock(block.x,block.y);
                    block.y += 20;
                    showBlock(block.x,block.y);
                }else goto end;
                break;
        }
    }

    end:
    {
      while(1);
    };
}