#ifndef TRAP
#define TRAP
#include "printk.h"
#include "global.h"

#define R15	0x00
#define R14	0x08
#define R13	0x10
#define R12	0x18
#define R11	0x20
#define R10	0x28
#define R9	0x30
#define R8	0x38
#define RBX	0x40
#define RCX	0x48
#define RDX	0x50
#define RSI	0x58
#define RDI	0x60
#define RBP	0x68
#define DS	0x70
#define ES	0x78
#define RAX	0x80
//有errorcode则硬件自动保存
#define FUNC 0x88
#define ERRCODE	0x90
//硬件自动保存
#define RIP	0x98
#define CS	0xa0
#define RFLAGS	0xa8
#define RSP	0xb0
#define SS	0xb8

#define funTrap(errorVector) fun##errorVector##call(void)
//保护现场
#define protectRegsiter  \
    "cld			\n\t"         \
    "cli            \n\t"          \
	"pushq	%rax		\n\t"		\
	"pushq	%rax		\n\t"		\
	"movq	%es,	%rax	\n\t"		\
	"pushq	%rax		\n\t"		\
	"movq	%ds,	%rax	\n\t"		\
	"pushq	%rax		\n\t"		\
	"xorq	%rax,	%rax	\n\t"		\
	"pushq	%rbp		\n\t"		\
	"pushq	%rdi		\n\t"		\
	"pushq	%rsi		\n\t"		\
	"pushq	%rdx		\n\t"		\
	"pushq	%rcx		\n\t"		\
	"pushq	%rbx		\n\t"		\
	"pushq	%r8		\n\t"		\
	"pushq	%r9		\n\t"		\
	"pushq	%r10		\n\t"		\
	"pushq	%r11		\n\t"		\
	"pushq	%r12		\n\t"		\
	"pushq	%r13		\n\t"		\
	"pushq	%r14		\n\t"		\
	"pushq	%r15		\n\t"		\
	"movq	$0x10,	%rdx	\n\t"		\
	"movq	%rdx,	%ds	\n\t"		\
	"movq	%rdx,	%es	\n\t"
//恢复现场并返回
extern void backRegsiter();
__asm__ (
"backRegsiter:	\n\t"
"popq	%r15 \n\t"
"popq	%r14 \n\t"
"popq	%r13 \n\t"
"popq	%r12 \n\t"
"popq	%r11 \n\t"
"popq	%r10 \n\t"
"popq	%r9 \n\t"
"popq	%r8 \n\t"
"popq	%rbx \n\t"
"popq	%rcx \n\t"
"popq	%rdx \n\t"
"popq	%rsi \n\t"
"popq	%rdi \n\t"
"popq	%rbp \n\t"
"popq	%rax \n\t"
"movq	%rax,	%ds \n\t"
"popq	%rax \n\t"
"movq	%rax,	%es \n\t"
"popq	%rax \n\t"
"addq	$0x10,	%rsp \n\t"
"sti                \n\t"
"iretq \n\t"
);

//无错误码的异常入口程序
#define noErrorCodeTrapInit(errorVector)    \
extern void funTrap(errorVector) ;\
__asm__ (  "fun"#errorVector"call: \n\t"   \
            "pushq $0              \n\t"   \
            protectRegsiter  \
            "movq %rsp , %rdi  \n\t"                                  \
            "leaq backRegsiter(%rip) , %rax \n\t"\
            "pushq %rax \n\t"                    \
            "movq $"#errorVector",	%rsi \n\t" \
            "jmp doError \n\t"\
);

//有错误码的异常入口程序
#define ErrorCodeTrapInit(errorVector)    \
extern void funTrap(errorVector) ;\
__asm__ (  "fun"#errorVector"call: \n\t"   \
            protectRegsiter  \
            "movq %rsp , %rdi  \n\t"                                  \
            "leaq backRegsiter(%rip) , %rax \n\t"\
            "pushq %rax \n\t"                    \
            "movq $"#errorVector",	%rsi \n\t" \
            "jmp doError \n\t"\
);

unsigned long GDTbase = 0;
unsigned long IDTbase = 0;
unsigned long TSS64base = 0;


unsigned long getGDTbase();
unsigned long getIDTbase();
unsigned long  getTSS64base();

void setIDTItem(unsigned long n,unsigned long ist,unsigned long attribute,unsigned long codeAddress);
void setKernelTrapItem(unsigned long n,unsigned long ist,void * addr);
void setKernelInterruptItem(unsigned long n,unsigned long ist,void * addr);
void setUserTrapItem(unsigned long n,unsigned long ist,void * addr);
void setUserInterruptItem(unsigned long n,unsigned long ist,void * addr);
void setTss64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7);


//异常处理方法
void doDivideError(unsigned long rsp); //0
void doDebug(unsigned long rsp); //1
void doNmi(unsigned long rsp); //2
void doInt3(unsigned long rsp); //3
void doOverflow(unsigned long rsp); //4
void doBounds(unsigned long rsp); //5
void doUndefinedOpcode(unsigned long rsp); //6
void doDevNotaAvailable(unsigned long rsp); //7
void doDoubleFault(unsigned long rsp); //8
void doCoprocessorSegmentOverrun(unsigned long rsp); //9
void doInvalidTSS(unsigned long rsp); //10
void doSegmentNotPresent(unsigned long rsp); //11
void doStackSegmentFault(unsigned long rsp); //12
void doGeneralProtection(unsigned long rsp); //13
void doPageFault(unsigned long rsp); //14
//15没有
void doX87FPUError(unsigned long rsp); //16
void doAlignmentCheck(unsigned long rsp); //17
void doMachineCheck(unsigned long rsp); //18
void doSIMDException(unsigned long rsp); //19
void doVirtualizationException(unsigned long rsp); //20
//21-31 保留
//32-255
void doInterrupt(unsigned long rsp,unsigned long vector);

void trapMain();
void interruptMain();
void setTR(unsigned long n);
void init8259A();
#endif
