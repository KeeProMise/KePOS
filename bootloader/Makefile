

all: boot.bin loader.bin

loader.bin:
	nasm loader.asm -o loader.bin

boot.bin:
	nasm boot.asm -o boot.bin

clean:
	rm -rf *.bin *.asm~ Makefile~ loader.bin boot.bin
