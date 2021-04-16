
#include "printk.h"
#include "trap.h"
#include "global.h"

unsigned long  getGDTbase(){
     unsigned long  tmp = 0;
    __asm__ __volatile__	(
            "leaq GDT_Table(%%rip), %%rdx	\n\t"
		    "movq  %%rdx, %0	\n\t"
        :"=r"(tmp)
        :
        :"memory"
    );
    return tmp;
}
unsigned long  getIDTbase(){
    unsigned long  tmp = 0;
    __asm__ __volatile__	(
        "leaq IDT_Table(%%rip), %%rdx	\n\t"
        "movq  %%rdx, %0	\n\t"
    :"=r"(tmp)
    :
    :"memory"
    );
    return tmp;
}
unsigned long  getTSS64base(){
    unsigned long  tmp = 0;
    __asm__ __volatile__	(
    "leaq TSS64_Table(%%rip), %%rdx	\n\t"
    "movq  %%rdx, %0	\n\t"
    :"=r"(tmp)
    :
    :"memory"
    );
    return tmp;
}
/*
    初始化TR寄存器
    输入:TSS描述符在GDT表中的开始编号
*/
void setTR(unsigned long n){
    __asm__ __volatile__(	"ltr	%%ax"
    :
    :"a"(n << 3)
    :"memory");
}
//设置IDT表中的描述符
void setIDTItem(unsigned long n,unsigned long ist,unsigned long attribute,unsigned long codeAddress){
    unsigned long * descriptAddr = (unsigned long *)(IDTbase+(n<<4));
    unsigned long low;
    unsigned long high;
    high = codeAddress >> 32;
    low = (0x08) << 16;
    low += (ist << 32);
    low += (attribute << 40);
    low += ((codeAddress >> 16) << 48);
    low += (codeAddress & ((1UL << 16)-1));
    (* descriptAddr) = low;
    (* (descriptAddr+1)) = high;
}
//设置TSS结构数据
void setTss64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{
    color_printk(ORANGE,BLACK,"TSS64 init \n");
    *(unsigned long *)(TSS64base+4) = rsp0;
    *(unsigned long *)(TSS64base+12) = rsp1;
    *(unsigned long *)(TSS64base+20) = rsp2;

    *(unsigned long *)(TSS64base+36) = ist1;
    *(unsigned long *)(TSS64base+44) = ist2;
    *(unsigned long *)(TSS64base+52) = ist3;
    *(unsigned long *)(TSS64base+60) = ist4;
    *(unsigned long *)(TSS64base+68) = ist5;
    *(unsigned long *)(TSS64base+76) = ist6;
    *(unsigned long *)(TSS64base+84) = ist7;
}
void setKernelTrapItem(unsigned long n,unsigned long ist,void * addr){
    setIDTItem( n , ist ,0x8F , (unsigned long )addr);
}
void setKernelInterruptItem(unsigned long n,unsigned long ist,void * addr){
    setIDTItem( n , ist ,0x8E , (unsigned long )addr);
}
void setUserTrapItem(unsigned long n,unsigned long ist,void * addr){
    setIDTItem( n , ist ,0xEF , (unsigned long )addr);
}
void setUserInterruptItem(unsigned long n,unsigned long ist,void * addr){
    setIDTItem( n , ist ,0xEE , (unsigned long )addr);
}


noErrorCodeTrapInit(0)
noErrorCodeTrapInit(1)
noErrorCodeTrapInit(2)
noErrorCodeTrapInit(3)
noErrorCodeTrapInit(4)
noErrorCodeTrapInit(5)
noErrorCodeTrapInit(6)
noErrorCodeTrapInit(7)
ErrorCodeTrapInit(8)
noErrorCodeTrapInit(9)
ErrorCodeTrapInit(10)
ErrorCodeTrapInit(11)
ErrorCodeTrapInit(12)
ErrorCodeTrapInit(13)
ErrorCodeTrapInit(14)

noErrorCodeTrapInit(16)
ErrorCodeTrapInit(17)
noErrorCodeTrapInit(18)
noErrorCodeTrapInit(19)
noErrorCodeTrapInit(20)
noErrorCodeTrapInit(32)
noErrorCodeTrapInit(33)
noErrorCodeTrapInit(34)
noErrorCodeTrapInit(35)
noErrorCodeTrapInit(36)
noErrorCodeTrapInit(37)
noErrorCodeTrapInit(38)
noErrorCodeTrapInit(39)
noErrorCodeTrapInit(40)
noErrorCodeTrapInit(41)
noErrorCodeTrapInit(42)
noErrorCodeTrapInit(43)
noErrorCodeTrapInit(44)
noErrorCodeTrapInit(45)
noErrorCodeTrapInit(46)
noErrorCodeTrapInit(47)

void doError(unsigned long rsp,unsigned long errorVector){
    if(errorVector == 0) doDivideError(rsp);
    else if (errorVector == 1) doDebug(rsp);
    else if (errorVector == 2) doNmi(rsp);
    else if (errorVector == 3) doInt3(rsp);
    else if (errorVector == 4) doOverflow(rsp);
    else if (errorVector == 5) doBounds(rsp);
    else if (errorVector == 6) doUndefinedOpcode(rsp);
    else if (errorVector == 7) doDevNotaAvailable(rsp);
    else if (errorVector == 8) doDoubleFault(rsp);
    else if (errorVector == 9) doCoprocessorSegmentOverrun(rsp);
    else if (errorVector == 10)doInvalidTSS(rsp);
    else if (errorVector == 11) doSegmentNotPresent(rsp);
    else if (errorVector == 12) doStackSegmentFault(rsp);
    else if (errorVector == 13) doGeneralProtection(rsp);
    else if (errorVector == 14) doPageFault(rsp);

    else if (errorVector == 16) doX87FPUError(rsp);
    else if (errorVector == 17) doAlignmentCheck(rsp);
    else if (errorVector == 18) doMachineCheck(rsp);
    else if (errorVector == 19) doSIMDException(rsp);
    else if (errorVector == 20) doVirtualizationException(rsp);
    else if (errorVector >=32)  doInterrupt(rsp,errorVector);
    else {
        color_printk(RED,BLACK,"undefined Error Vector:%#018lx\n",errorVector);
        while(True);
    }
}
//0
void doDivideError(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//1
void doDebug(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_debug(1),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//2
void doNmi(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_Nmi(2),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//3
void doInt3(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_Int3(3),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//4
void doOverflow(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_Overflow(4),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//5
void doBounds(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_Bounds(5),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//6
void doUndefinedOpcode(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_undefined_opcode(6),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//7
void doDevNotaAvailable(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"doDevNotaAvailable(7),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//8
void doDoubleFault(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"doDoubleFault(8),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//9
void doCoprocessorSegmentOverrun(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"doCoprocessorSegmentOverrun(9),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//10
void doInvalidTSS(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"doInvalidTSS(10),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);

    if((*errorCode) & 0x01)
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

    if((*errorCode) & 0x02)
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    else
        color_printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

    if(((*errorCode) & 0x02) == 0)
        if((*errorCode) & 0x04)
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        else
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",(*errorCode) & 0xfff8);

    while(True);
}
//11
void doSegmentNotPresent(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_segment_not_present(11),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    unsigned long error_code = *errorCode;
    if(error_code & 0x01)
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

    if(error_code & 0x02)
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    else
        color_printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

    if((error_code & 0x02) == 0)
        if(error_code & 0x04)
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        else
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",error_code & 0xfff8);

    while(True);
}
//12
void doStackSegmentFault(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    unsigned long error_code = *errorCode;
    color_printk(RED,BLACK,"do_stack_segment_fault(12),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);

    if(error_code & 0x01)
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

    if(error_code & 0x02)
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    else
        color_printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

    if((error_code & 0x02) == 0)
        if(error_code & 0x04)
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        else
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",error_code & 0xfff8);

    while(True);
}
//13
void doGeneralProtection(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    unsigned long error_code = *errorCode;
    unsigned long * trapRsp = (unsigned long *)(rsp + RSP);
    color_printk(RED,BLACK,"do_general_protection(13),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , *trapRsp , *rip);

    if(error_code & 0x01)
        color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

    if(error_code & 0x02)
        color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
    else
        color_printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

    if((error_code & 0x02) == 0)
        if(error_code & 0x04)
            color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
        else
            color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

    color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",error_code & 0xfff8);

    while(True);
}
//14
void doPageFault(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    unsigned long error_code = *errorCode;
    unsigned long * trapRsp = (unsigned long *)(rsp + RSP);
    unsigned long cr2 = 0;

    __asm__	__volatile__("movq	%%cr2,	%0":"=r"(cr2)::"memory");

    color_printk(RED,BLACK,"do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , * trapRsp , *rip);

    if(!(error_code & 0x01))
        color_printk(RED,BLACK,"Page Not-Present,\t");

    if(error_code & 0x02)
        color_printk(RED,BLACK,"Write Cause Fault,\t");
    else
        color_printk(RED,BLACK,"Read Cause Fault,\t");

    if(error_code & 0x04)
        color_printk(RED,BLACK,"Fault in user(3)\t");
    else
        color_printk(RED,BLACK,"Fault in supervisor(0,1,2)\t");

    if(error_code & 0x08)
        color_printk(RED,BLACK,",Reserved Bit Cause Fault\t");

    if(error_code & 0x10)
        color_printk(RED,BLACK,",Instruction fetch Cause Fault");

    color_printk(RED,BLACK,"\n");

    color_printk(RED,BLACK,"CR2:%#018lx\n",cr2);

    while(True);
}
//15

//16
void doX87FPUError(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_x87_FPU_error(16),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//17
void doAlignmentCheck(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_alignment_check(17),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//18
void doMachineCheck(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_machine_check(18),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//19
void doSIMDException(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_SIMD_exception(19),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}
//20
void doVirtualizationException(unsigned long rsp){
    unsigned long * rip = NULL;
    rip = (unsigned long *)(rsp + RIP);
    unsigned long * errorCode = NULL;
    errorCode = (unsigned long *)(rsp + ERRCODE);
    color_printk(RED,BLACK,"do_virtualization_exception(20),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",*errorCode , rsp , *rip);
    while(True);
}



//异常处理主函数
void trapMain(){
    GDTbase = getGDTbase();
    IDTbase = getIDTbase();
    TSS64base = getTSS64base();
    color_printk(GREEN,BLACK,"GDT base:%#018lx \n",GDTbase);
    color_printk(GREEN,BLACK,"IDT base:%#018lx \n",IDTbase);
    color_printk(GREEN,BLACK,"TSS base:%#018lx \n",TSS64base);
    //设置异常描述符
    setKernelTrapItem(0,1,fun0call);
    setKernelTrapItem(1,1,fun1call);
    setKernelInterruptItem(2,1,fun2call);
    setUserTrapItem(3,1,fun3call);
    setUserTrapItem(4,1,fun4call);
    setUserTrapItem(5,1,fun5call);
    setKernelTrapItem(6,1,fun6call);
    setKernelTrapItem(7,1,fun7call);
    setKernelTrapItem(8,1,fun8call);
    setKernelTrapItem(9,1,fun9call);
    setKernelTrapItem(10,1,fun10call);
    setKernelTrapItem(11,1,fun11call);
    setKernelTrapItem(12,1,fun12call);
    setKernelTrapItem(13,1,fun13call);
    setKernelTrapItem(14,1,fun14call);

    setKernelTrapItem(16,1,fun16call);
    setKernelTrapItem(17,1,fun17call);
    setKernelTrapItem(18,1,fun18call);
    setKernelTrapItem(19,1,fun19call);
    setKernelTrapItem(20,1,fun20call);
    //设置中断描述符
    setKernelInterruptItem(32,2,fun32call);
    setKernelInterruptItem(33,2,fun33call);
    setKernelInterruptItem(34,2,fun34call);
    setKernelInterruptItem(35,2,fun35call);
    setKernelInterruptItem(36,2,fun36call);
    setKernelInterruptItem(37,2,fun37call);
    setKernelInterruptItem(38,2,fun38call);
    setKernelInterruptItem(39,2,fun39call);
    setKernelInterruptItem(40,2,fun40call);
    setKernelInterruptItem(41,2,fun41call);
    setKernelInterruptItem(42,2,fun42call);
    setKernelInterruptItem(43,2,fun43call);
    setKernelInterruptItem(44,2,fun44call);
    setKernelInterruptItem(45,2,fun45call);
    setKernelInterruptItem(46,2,fun46call);
    setKernelInterruptItem(47,2,fun47call);

    setTR(8);
    setTss64(0xffff8000001fffff, 0xffff8000001fffff, 0xffff8000001fffff, 0xffff8000001fefff,0xffff8000001fdfff, 0xffff8000001fdfff, 0xffff8000001fdfff, 0xffff8000001fdfff, 0xffff8000001fdfff, 0xffff8000001fdfff);
}
