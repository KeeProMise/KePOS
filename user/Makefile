
object:= head.o main.o  system.o

all: System
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary System mymain.bin

System:	${object}
	ld -b elf64-x86-64 -z muldefs -o System ${object} -T user.lds 

head.o:	head.S
	gcc -E  head.S > head.s
	as --64 -o head.o head.s

main.o:	main.c
	gcc  -mcmodel=large -fno-builtin -m64 -c main.c
	
system.o: system.c
	gcc  -mcmodel=large -fno-builtin -m64 -c system.c
clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ System  Makefile~ user.lds~ mymain.bin 

