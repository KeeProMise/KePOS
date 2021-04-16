#ifndef MEMORY
#define MEMORY


#define KernalPhyAddressToVirAddress(phy) (phy+0xffff800000000000)
#define KernelVirAddressToPhyAddress(vir) (vir-0xffff800000000000)
//Zone相关
#define RAM 1
#define ROM 2
#define ACPI 3
#define Other 4

//Page相关
#define PAGE_2M_SHIFT 21
#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PageItemMask    ((1UL << 9) -1)
#define PML4E_offset(virAddress)  ((virAddress >> 39) & PageItemMask)
#define PDPTE_offset(virAddress)  ((virAddress >> 30) & PageItemMask)
#define PDT_offset(virAddress)    ((virAddress >> 21) & PageItemMask)
//2m下边界对齐
#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define KERNEL 1
#define USER 2
#define SLAB 3


//Slab相关
#define BlockSize_32B 0
#define BlockSize_64B 1
#define BlockSize_128B 2
#define BlockSize_256B 3
#define BlockSize_512B 4
#define BlockSize_1KB 5
#define BlockSize_4KB 6

#import "global.h"
#include "lib.h"
#include "printk.h"
unsigned long E820Address = 0xffff800000007e00;
unsigned long totalMemory = 0;
//定义该全局结构体


struct E820{
    unsigned long address;
    unsigned long length;
    unsigned int	type;
}__attribute__((packed));

#define DefaultZone {NULL,NULL,0}
//保存到内核.bss段
struct Zone{
    //该区域的页位图映射
    unsigned long * bitmap;
    //区域管理的开始页位置
    struct Page * pages;
    //该区域管理的页总数
    unsigned long length;
};

//内核运行时，动态初始化
struct Page
{
    //区域属性
    unsigned long zoneAttribute;
    //页的开始物理地址
    unsigned long   physicsAddress;
    //页的属性(0:未使用，1:内核使用，2:用户使用，3:Slab使用)
    unsigned long  attribute;
    //保留
    unsigned long reserve[2];
};

//动态初始化
struct Slab{
    //链子（双向循环链表）
    struct Slab * prev;
    struct Slab * next;
    //管理的页
    unsigned long pageID;
    //使用计数
    unsigned long usingCount;
    //空闲计数
    unsigned long freeCount;
    //对应的虚拟地址
    unsigned long virtualAddress;
    //区域的位图映射
    unsigned long * bitMap;
    //位图长度
    unsigned long bitMapLength;
};
//代表一块特定大小(blockSize)的内存池对象
struct SlabPool{
    unsigned long blockSize;
    struct Slab * head;
    struct Slab * now;
    struct Slab * reserve;
    unsigned long useCount;
    unsigned long freeCount;
};

//保存在内核.bss段
struct Memory{
    //管理的E820结构体
    struct E820 e820[32];
    unsigned int e820Length;
    //管理的区域
    struct Zone zone[5];
    //SlabPooL
    struct SlabPool slabPool[7];
};

void initKernelPage(unsigned long CR3PhyAddress);
void set(unsigned long virAddress,char target,unsigned long count){
    unsigned long i;
    for(i=0;i<count;i++,virAddress++){
        (* (char *)virAddress) = target;
    }
}
void copy(unsigned long from,unsigned long to,unsigned long count){
    unsigned long i;
    for(i = 0;i<count;i++,from++,to++){
        (* (char *)to) = (* (char *)from);
    }
}
//初始化内存结构体
void memoryInit();

void memoryMain();

//Page相关------------------------------------------
void backOnePage(unsigned long pageVirAddress);
//分配count页,并修改对应的位
struct Page * allocatePage(int type,unsigned int count);
unsigned long allocateOnePage(unsigned long attrubute);
//设置页的属性
unsigned long setPageAttribute(struct Page * page,unsigned long count,unsigned long attr);
struct Page * getPageByPageID(unsigned long pageID);
unsigned long virPageTOPhyPage(unsigned long virAddress,unsigned long phyAddress,unsigned long CR3PhyAddress,unsigned long type);
//Bitmap相关-------------------------------------------
//修改Bitmap某一位(0>1,1>0),bitID即要修改的位(0~length)
unsigned long modifyBitmap(unsigned long * bitmap,unsigned long length,unsigned long bitID);
//修改由bitID开始的count个位，同上
unsigned long modifyBitmapSome(unsigned long * bitmap,unsigned long length,unsigned long bitID,unsigned long count);
//从biemap中找到count个空闲块，可用的块总数是length,count最大为63,返回开始块号(从0开始)。
unsigned long findFreeBlockInBitmap(unsigned long * bitmap,unsigned long length,unsigned long count);

//Slab相关方法--------------------------------------------
//向内核申请一个block
unsigned long  getMemoryBlock(unsigned long blockSize);
//生成一个slab
struct Slab * getOneSlab(unsigned long blockSize);
//向内核归还一个block
unsigned long backMemoryBlock(unsigned long virtualAddress);
void backOneBlock(struct SlabPool * slabPool,struct Slab * slab,unsigned long blockId);

void backCR3page(unsigned long CR3PhyAddress);


void initOneSlabNode(struct Slab * Traget,unsigned long PageID,unsigned long blockSize){
    unsigned long endSlab = 0+ sizeof(struct Slab);
    Traget->prev = Traget;
    Traget->next = Traget;
    Traget->pageID = PageID;
    Traget->bitMap = (unsigned long *)((unsigned long)Traget + sizeof(struct Slab));
    Traget->bitMapLength = PAGE_2M_SIZE/blockSize;
    endSlab += ((((Traget->bitMapLength)>>6)+1) << 3);
    unsigned long bitCount = ((endSlab+blockSize-1)&(~(blockSize-1)))/blockSize;
    modifyBitmapSome(Traget->bitMap,Traget->bitMapLength,0,bitCount);
    Traget->usingCount = 0;
    Traget->freeCount = Traget->bitMapLength-bitCount;
}

//获取第一块由空闲的Slab
struct Slab * getFristFreeSlab(struct Slab * head){
    if(head==NULL) return (struct Slab * )NO;
    if(head->freeCount>0) return head;
    struct Slab * now = head->next;
    for(;now!=head;now = now->next){
        if(now->freeCount>0) return now;
    }
    return (struct Slab * )NO;
}
//分配一个块,返回虚拟地址
unsigned long allocateOneBlockInSlap(struct SlabPool * slabPool,struct Slab * slab){
    unsigned freeBlockID = findFreeBlockInBitmap(slab->bitMap,slab->bitMapLength,1);
    modifyBitmap(slab->bitMap,slab->bitMapLength,freeBlockID);
    slab->usingCount++;
    slab->freeCount--;
    slabPool->useCount++;
    slabPool->freeCount--;
    return (((long)slab)+(slabPool->blockSize*freeBlockID));
}
void insertBefore(struct Slab * Object,struct Slab * Target){
    Target->next = Object;
    Target->prev = Object->prev;
    Object->prev = Target;
    Target->prev->next = Target;
}

void insertBehand(struct Slab * Object,struct Slab * Target){
    Target->next = Object->next;
    Target->prev = Object;
    Object->next = Target;
    Target->next->prev = Target;
}

void deleteTargetNode(struct Slab * Target){
    Target->prev->next = Target->next;
    Target->next->prev = Target->prev;
}



void modifyCR3(unsigned long CR3phyAddress){
    __asm__ __volatile__	(
    "movq	%%rax,	%%cr3	\n\t"
    :
    :"a"(CR3phyAddress)
    :"memory"
    );
}

unsigned long * getCR3() {
    unsigned long * tmp;
    __asm__ __volatile__	(
    "movq	%%cr3,	%0	\n\t"
    :"=r"(tmp)
    :
    :"memory"
    );
    return tmp;
}


#endif