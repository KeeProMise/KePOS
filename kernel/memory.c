//内存管理

#include "memory.h"
#include "global.h"
#include "printk.h"

struct Memory memory = {{0},0,{0},{0}};

//初始化内核页表(线性地址<----->物理地址),即直接映射
void initKernelPage(unsigned long CR3PhyAddress){
    unsigned long nowPhyAddress;
    unsigned long CR3;
    for(nowPhyAddress=0;nowPhyAddress<(memory.e820[memory.e820Length].address+memory.e820[memory.e820Length].length);nowPhyAddress+=PAGE_2M_SIZE){
       CR3 = virPageTOPhyPage(KernalPhyAddressToVirAddress(nowPhyAddress),nowPhyAddress,CR3PhyAddress,3);
    }
    if(CR3!=NO){
        //color_printk(GREEN,BLACK,"before CR3:%#018lx\n",getCR3());
        modifyCR3(CR3);
        SVGA_Virtual_Address = 0xe0000000 + 0xffff800000000000;
        Pos.FB_addr = SVGA_Virtual_Address;
       // color_printk(GREEN,BLACK,"restart CR3:%#018lx\n",getCR3());
    }
}

void deleteOnePageTable(unsigned long baseAddress){
    if((baseAddress & 0x80)) return;
    baseAddress = ((baseAddress >> 12) << 12);
    unsigned long i ;
    unsigned long * now = (unsigned long *)baseAddress;
    unsigned long nowVirAddress;
    for(i=0;i<512;i++){
        nowVirAddress = *now;
        if((nowVirAddress & 0x1) == 1){
            nowVirAddress = KernalPhyAddressToVirAddress(nowVirAddress);
            deleteOnePageTable(nowVirAddress);
        }
        now++;
    }
  //  color_printk(BLUE,WHITE,"delete Address%#018lx \n",baseAddress);
    backMemoryBlock(baseAddress);
}
void backCR3page(unsigned long CR3PhyAddress){

    unsigned long CR3VirAddress = KernalPhyAddressToVirAddress(CR3PhyAddress);
  //  color_printk(BLUE,WHITE,"CR3VirAddress%#018lx \n",CR3VirAddress);

    deleteOnePageTable(CR3VirAddress);
   // color_printk(BLUE,RED,"finsh \n");
}
//根据CR3的物理地址，修改页表映射virAddress-->phyAddress,页属性位type(0~3位的值)
unsigned long virPageTOPhyPage(unsigned long virAddress,unsigned long phyAddress,unsigned long CR3PhyAddress,unsigned long type){
    virAddress = ((virAddress >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT);
    phyAddress = ((phyAddress >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT);
    unsigned long CR3VirAddress = KernalPhyAddressToVirAddress(CR3PhyAddress);
    unsigned long PML4EStart = (PML4E_offset(virAddress) << 3);
    unsigned long PDPTEstart = (PDPTE_offset(virAddress) << 3);
    unsigned long PDTEstart = (PDT_offset(virAddress) << 3);


    unsigned long * PML4E8BitPoint =  (unsigned long *)(CR3VirAddress+PML4EStart);
    if(!((* PML4E8BitPoint) & 1)){
        //赋值type
        (* PML4E8BitPoint) = type;
        //获取一块4KB的内存，并将其清0
        unsigned long block4kVirAdd = getMemoryBlock(4096);
        set(block4kVirAdd,(char)0x00,4096);
        (* PML4E8BitPoint) += KernelVirAddressToPhyAddress(block4kVirAdd);
    }

    unsigned long * PDPTE8BitPoint = (unsigned long *)((((* PML4E8BitPoint) >> 12) << 12)+PDPTEstart);
    PDPTE8BitPoint = (unsigned long)PDPTE8BitPoint+0xffff800000000000;
    if(!((* PDPTE8BitPoint) & 1)){
        //赋值type
        (* PDPTE8BitPoint) = type;
        //获取一块4KB的内存，并将其清0
        unsigned long block4kVirAdd = getMemoryBlock(4096);
        set(block4kVirAdd,(char)0x00,4096);
        (* PDPTE8BitPoint) += KernelVirAddressToPhyAddress(block4kVirAdd);
    }

    unsigned long * PDT8BitPoint = (unsigned long *)((((* PDPTE8BitPoint) >> 12) << 12)+PDTEstart);
    PDT8BitPoint = (unsigned long)PDT8BitPoint+0xffff800000000000;
    (* PDT8BitPoint) = (1UL << 7);
    (* PDT8BitPoint) += type;
    (* PDT8BitPoint) += phyAddress;
    if((* PDT8BitPoint) & 1) {
        return CR3PhyAddress;
    }

    else return NO;
}

void backOneBlock(struct SlabPool * slabPool,struct Slab * slab,unsigned long blockId){
    slabPool->useCount--;
    slabPool->freeCount++;
    slab->freeCount++;
    slab->usingCount--;
    if((slab->usingCount==0) && (((double)slabPool->freeCount/(double)slab->freeCount)>=1.5)){
        deleteTargetNode(slab);
        setPageAttribute(getPageByPageID(slab->pageID),1,0);
        modifyBitmap(memory.zone[RAM].bitmap,memory.zone[RAM].length,slab->pageID);
        slabPool->freeCount -= slab->freeCount;
        set((unsigned long)slab,0x00,PAGE_2M_SIZE);
        return;
    }
    modifyBitmap(slab->bitMap,slab->bitMapLength,blockId);
    set(((unsigned long)slab+blockId*(slabPool->blockSize)),(char)0x00,slabPool->blockSize);
}

unsigned long backMemoryBlock(unsigned long virtualAddress){
    int i ;
    struct SlabPool  * slabPool;
    struct Slab * now;
    unsigned long startAddress = (virtualAddress >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
    for(i=0;i<7;i++){
        slabPool = &memory.slabPool[i];
        if(slabPool->head==NULL) continue;
        now = slabPool->head;
        if((unsigned long)now==startAddress) goto Slabfind;
        else{
            for(;now!=slabPool->head;now=now->next){
                if(now == startAddress){
                    goto Slabfind;
                }
            }
        }
    }
    return False;
    //找到了
    Slabfind:
    {
        unsigned long blockSize = slabPool->blockSize;
        unsigned blockID = (virtualAddress-startAddress)/blockSize;
        backOneBlock(slabPool,now,blockID);
        return True;
        }
}

struct Page * getPageByPageID(unsigned long pageID){
    return memory.zone[RAM].pages+pageID;
}

//生成一个slab，获取一个空page
struct Slab * getOneSlab(unsigned long blockSize) {
    //获取一个内存PageID
    unsigned long PageID = findFreeBlockInBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, 1);
    modifyBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, PageID);
    unsigned long PageVirAddress = KernalPhyAddressToVirAddress(getPageByPageID(PageID)->physicsAddress);
    set(PageVirAddress,(char)0x00,PAGE_2M_SIZE);
    struct Slab *temSlab = (struct Slab *) PageVirAddress;
    setPageAttribute(getPageByPageID(PageID),1,SLAB);
    initOneSlabNode(temSlab, PageID, blockSize);
    return temSlab;
}
//获取一个内存块线性地址
unsigned long  getMemoryBlock(unsigned long blockSize){
    unsigned long block;
    if(blockSize<=32) block=BlockSize_32B;
    else if(blockSize<=64) block=BlockSize_64B;
    else if(blockSize<=128) block=BlockSize_128B;
    else if(blockSize<=256) block=BlockSize_256B;
    else if(blockSize<=512) block=BlockSize_512B;
    else if(blockSize<=1024) block=BlockSize_1KB;
    else if(blockSize<=4096) block=BlockSize_4KB;
    else return NO;
    struct SlabPool  * slabPool = &memory.slabPool[block];
    //如果Slab链表位空，则创建一个Slab。
    if(slabPool->head==NULL){
        struct Slab * slab = getOneSlab(slabPool->blockSize);
        slabPool->head = slab;
        slabPool->freeCount += slab->freeCount;
        slabPool->useCount += slab->usingCount;
    }
    //如果没有空闲,添加一个slab
    if(slabPool->freeCount==0){
        struct Slab * slab = getOneSlab(slabPool->blockSize);
        slabPool->freeCount += slab->freeCount;
        slabPool->useCount += slab->usingCount;
        insertBehand(slabPool->head->prev,slab);
    }
    struct Slab * slab = getFristFreeSlab(slabPool->head);
    unsigned long v = allocateOneBlockInSlap(slabPool,slab);
    if(slab!=NO)  {
        set(v,0x00,slabPool->blockSize);
        //color_printk(BLUE,WHITE,"GET ADD%#018lx\n",v);
        return v;
    }
    return NO;
}
//初始化内存结构体
void memoryInit(){
    StartStruct = (unsigned long)&StartStruct;
    color_printk(GREEN,BLACK,"StartStruct is:%#018lx\n",StartStruct);
    int i;
    struct E820 *p = (struct E820 *)E820Address;
    struct Zone temZone[5] = {
            DefaultZone,DefaultZone,DefaultZone,DefaultZone,DefaultZone,
    };
    for(i =0;i<5;i++){
        memory.zone[i] = temZone[i];
    }
    struct SlabPool temSlabPool[7] = {
            {32,NULL,NULL,NULL,0,0},
            {64,NULL,NULL,NULL,0,0},
            {128,NULL,NULL,NULL,0,0},
            {256,NULL,NULL,NULL,0,0},
            {512,NULL,NULL,NULL,0,0},
            {1024,NULL,NULL,NULL,0,0},
            {4096,NULL,NULL,NULL,0,0},
    };
    for(i = 0;i<7;i++){
        memory.slabPool[i] = temSlabPool[i];
    }
    for(i = 0;i < 32;i++)
    {
        color_printk(ORANGE,BLACK,"Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
        unsigned long tmp = 0;
        if(p->type == 1)
            totalMemory +=  p->length;

        memory.e820[i].address += p->address;

        memory.e820[i].length	 += p->length;

        memory.e820[i].type	 = p->type;

        memory.e820Length = i;

        p++;
        if(p->type > 4 || p->length == 0 || p->type < 1)
            break;
    }
    color_printk(GREEN,BLACK,"OS Can Used Total RAM:%#018lx\n",totalMemory);
    memory.zone[RAM].pages = (struct Page *) LongAlignment(StartStruct);
    struct Page * tempage = memory.zone[RAM].pages;
    for(i = 0;i <= memory.e820Length;i++)
    {
        unsigned long start,end;
        if(memory.e820[i].type != 1)
            continue;
        start = PAGE_2M_ALIGN(memory.e820[i].address);
        end   = ((memory.e820[i].address + memory.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        int j = 0;
        unsigned long length = (end - start) >> PAGE_2M_SHIFT;
        if(end <= start)
            continue;
        for(j = 0;j<length;j++){
            tempage->physicsAddress = start;
            start+=PAGE_2M_SIZE;
            tempage->attribute = 0;
            tempage->zoneAttribute = RAM;
            tempage++;
        }
        memory.zone[RAM].length += length;
    }
    StartStruct = (unsigned long )tempage;
    color_printk(GREEN,BLACK,"Page end StartStruct is:%#018lx\n",StartStruct);
    memory.zone[RAM].bitmap = (unsigned long *)LongAlignment(StartStruct);
    StartStruct = (unsigned long )memory.zone[RAM].bitmap;
    StartStruct +=((memory.zone[RAM].length>>6)+1)<<3;
    color_printk(GREEN,BLACK,"Bitmap end StartStruct is:%#018lx\n",StartStruct);
    color_printk(GREEN,BLACK,"Page start address is:%#018lx,Bitmap Start address is:%#018lx,Page count is:%#010x=%010d\n",memory.zone[RAM].pages,memory.zone[RAM].bitmap,memory.zone[RAM].length,memory.zone[RAM].length);
//    for( i =0;i<1;i++){
//        color_printk(GREEN,BLACK,"start Page PHYadd:%#018lx,Page attr:%#018lx\n",memory.zone[RAM].pages[i].physicsAddress,memory.zone[RAM].pages[i].attribute);
//    }
//    color_printk(GREEN,BLACK,"end Page PHYadd:%#018lx,Page attr:%#018lx\n",memory.zone[RAM].pages[memory.zone[RAM].length-1].physicsAddress,memory.zone[RAM].pages[memory.zone[RAM].length-1].attribute);
    //初始化内核代码占用的页
    unsigned long kernelLength = (PAGE_2M_ALIGN(StartStruct)-0x200000-0xffff800000000000) >> PAGE_2M_SHIFT;
    color_printk(GREEN,BLACK,"kernelLength:%#018lx\n",kernelLength);
    struct Page * temp = allocatePage(RAM,kernelLength);
    for(i=0;i<kernelLength;i++){
        temp->attribute = KERNEL;
        temp+=1;
    }
}

//修改Bitmap某一位(0>1,1>0),bitID即要修改的位(0~length)
unsigned long modifyBitmap(unsigned long * bitmap,unsigned long length,unsigned long bitID){
    if(length<=bitID) return False;
    unsigned long * now8Bit = bitmap+(bitID>>6);
    unsigned long now =1UL<<(bitID % LongSize);
    (* now8Bit) ^= now;
    return True;
}
//修改由bitID开始的count个位，同上
unsigned long modifyBitmapSome(unsigned long * bitmap,unsigned long length,unsigned long bitID,unsigned long count){
    if(bitID+count>length) return False;
    unsigned long i=0;
    for(i=0;i<count;i++){
        modifyBitmap(bitmap,length,bitID+i);
    }
}
//从biemap中找到count个空闲块，可用的块总数是length,count最大为63,返回开始块号(从0开始)。
unsigned long findFreeBlockInBitmap(unsigned long * bitmap,unsigned long length,unsigned long count){
    if(count>length) return NO;
    unsigned long now = 0;
    unsigned long  now8Bit;
    unsigned long detect8Bit = (1UL<<count)-1;
    for(;now<=length-count;now++){
        now8Bit = bitmap[now>>6];
        unsigned long i = now % LongSize;
        if(i<=LongSize-count){
            if(opposeOr(now8Bit>>i,~detect8Bit) == detect8Bit){
                //找到了，分配内存
                goto find;
            }else{
                continue;
            }
        }else{
            unsigned long next8Bit = bitmap[(now>>6)+1];
            unsigned long nextMovNum = 2*LongSize-count-i;
            if((now8Bit>>i == 0)&&(next8Bit<<nextMovNum)==0){
                //找到了，分配内存
                goto find;
            }else{
                continue;
            }
        }
    }
    return NO;
    find:
        return now;
}
//分配连续的几页，返回第一页,最多一次连续分配32页(下面程序最多支持一次64页，32页只是系统设计考虑)
struct Page * allocatePage(int type,unsigned int count){
    if(count>32) return (struct Page *) NULL;
    unsigned long PageID = findFreeBlockInBitmap(memory.zone[type].bitmap,memory.zone[type].length,count);
    if(PageID == NO){
        return (struct Page *) NULL;
    }else{
        modifyBitmapSome(memory.zone[type].bitmap,memory.zone[type].length,PageID,count);
        return memory.zone[type].pages+PageID;
    }
}
unsigned long setPageAttribute(struct Page * page,unsigned long count,unsigned long attr){
    unsigned long i;
    for(i=0;i<count;i++){
        page->attribute = attr;
        page+=1;
    }
}

//申请一页内存，设置属性
unsigned long allocateOnePage(unsigned long attrubute){
    unsigned long newPageID1 =  findFreeBlockInBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, 1);
    modifyBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, newPageID1);
    unsigned long phyAddress1 = getPageByPageID(newPageID1)->physicsAddress;
    set(KernalPhyAddressToVirAddress(phyAddress1),(char)0x00,PAGE_2M_SIZE);
    setPageAttribute(getPageByPageID(newPageID1),1,attrubute);
    return KernalPhyAddressToVirAddress(phyAddress1);
}
//归还一块page内存，pageAddress为page的虚拟地址
void backOnePage(unsigned long pageVirAddress){
    unsigned long pagePhyAddress = KernelVirAddressToPhyAddress(pageVirAddress);
    unsigned long pageId = 0;
    int i ;
    for(;i<memory.zone->length;i++){
        if(memory.zone[RAM].pages[i].physicsAddress == pagePhyAddress){
            pageId  = i;
            break;
        }
    }
    if(pageId != memory.zone->length){
        modifyBitmap(memory.zone[RAM].bitmap, memory.zone[RAM].length, pageId);
        set(pageVirAddress,(char)0x00,PAGE_2M_SIZE);
        setPageAttribute(getPageByPageID(pageId),1,0);
    }
}

void memoryMain(){
    //初始化内存结构体
    memoryInit();

    //初始化页表，并且更改CR3
    initKernelPage(0x100000);


}