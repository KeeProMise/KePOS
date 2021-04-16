
object:= head.o main.o printk.o interrupt.o trap.o memory.o window.o task.o device.o system.o

all: System
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary System kernel.bin

System:	${object}
	ld -b elf64-x86-64 -z muldefs -o System ${object} -T Kernel.lds 

head.o:	head.S
	gcc -E  head.S > head.s
	as --64 -o head.o head.s
	
main.o:	main.c
	gcc  -mcmodel=large -fno-builtin -m64 -c main.c
	
printk.o: printk.c
	gcc  -mcmodel=large -fno-builtin -m64 -c printk.c
trap.o: trap.c
	gcc  -mcmodel=large -fno-builtin -m64 -c trap.c
memory.o: memory.c
	gcc  -mcmodel=large -fno-builtin -m64 -c memory.c
window.o: window.c
	gcc  -mcmodel=large -fno-builtin -m64 -c window.c
interrupt.o: interrupt.c
	gcc  -mcmodel=large -fno-builtin -m64 -c interrupt.c
task.o: task.c
	gcc  -mcmodel=large -fno-builtin -m64 -c task.c
device.o: device.c
	gcc  -mcmodel=large -fno-builtin -m64 -c device.c
system.o: system.c
	gcc  -mcmodel=large -fno-builtin -m64 -c system.c
clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ System  Makefile~ Kernel.lds~ kernel.bin 

