
.section .text

.globl _start

_start:
	leaq starttask(%rip) ,%rax
    callq *%rax
    nop




