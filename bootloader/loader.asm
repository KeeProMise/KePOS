;loader


org	10000h
	jmp	Label_Start

RootDirSectors	equ	14
SectorNumOfRootDirStart	equ	19
SectorNumOfFAT1Start	equ	1
SectorBalance	equ	17	

	BS_OEMName	db	'KePOSFAT'
	BPB_BytesPerSec	dw	512
	BPB_SecPerClus	db	1
	BPB_RsvdSecCnt	dw	1
	BPB_NumFATs	db	2
	BPB_RootEntCnt	dw	224
	BPB_TotSec16	dw	2880
	BPB_Media	db	0xf0
	BPB_FATSz16	dw	9
	BPB_SecPerTrk	dw	18
	BPB_NumHeads	dw	2
	BPB_hiddSec	dd	0
	BPB_TotSec32	dd	0
	BS_DrvNum	db	0
	BS_Reserved1	db	0
	BS_BootSig	db	29h
	BS_VolID	dd	0
	BS_VolLab	db	'boot loader'
	BS_FileSysType	db	'FAT12   '



;kernel.bin被加载到的内存地址
BaseOfKernelFile	equ	0x00
OffsetOfKernelFile	equ	0x200000	

OffsetofuserFile    equ 0xd0000000

;从FAT12将kernel.bin加载到的临时地址，此后将其移动到上面的地址
BaseTmpOfKernelAddr	equ	0x00
OffsetTmpOfKernelFile	equ	0x7E00

MemoryStructBufferAddr	equ	0x7E00

;======================================GDT表--=====================================
;=================保护模式下的GDT表
[SECTION gdt]

LABEL_GDT:		dd	0,0
LABEL_DESC_CODE32:	dd	0x0000FFFF,0x00CF9A00		;代码段描述符，地址空间0-4G，特权级0
LABEL_DESC_DATA32:	dd	0x0000FFFF,0x00CF9200		;数据段描述符，地址空间0-4G，特权级0

GdtLen	equ	$ - LABEL_GDT
GdtPtr	dw	GdtLen - 1
	dd	LABEL_GDT

SelectorCode32	equ	LABEL_DESC_CODE32 - LABEL_GDT
SelectorData32	equ	LABEL_DESC_DATA32 - LABEL_GDT

;===================IA-32e模式下的GDT表
[SECTION gdt64]

LABEL_GDT64:		dq	0x0000000000000000
LABEL_DESC_CODE64:	dq	0x0020980000000000
LABEL_DESC_DATA64:	dq	0x0000920000000000

GdtLen64	equ	$ - LABEL_GDT64
;保存到GDTR寄存器的值
GdtPtr64	dw	GdtLen64 - 1
		dd	LABEL_GDT64

SelectorCode64	equ	LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64	equ	LABEL_DESC_DATA64 - LABEL_GDT64

;======================================Loader.bin=============================================
[SECTION .s16]
[BITS 16]

Label_Start:
	call Label_Show_Start_loader
	
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ax,	0x00
	mov	ss,	ax
	mov	sp,	0x7c00			;栈基址设置位0x7c00处

	call Label_Show_Start_loader	
	call Label_Open_A20		;使得寻址模式超过1M，FS寄存器保存保护模式的数据段选择子

;=======	reset floppy

	xor	ah,	ah
	xor	dl,	dl
	int	13h

;=======	search kernel.bin
	mov	word	[SectorNo],	SectorNumOfRootDirStart

;同boot.asm
Lable_Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0
	jz	Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]	
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h
	mov	ax,	[SectorNo]
	mov	cl,	1
	call	Func_ReadOneSector
	mov	si,	KernelFileName
	mov	di,	8000h
	cld
	mov	dx,	10h
	
Label_Search_For_LoaderBin:

	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11

Label_Cmp_FileName:

	cmp	cx,	0
	jz	Label_FileName_Found
	dec	cx
	lodsb	
	cmp	al,	byte	[es:di]
	jz	Label_Go_On
	jmp	Label_Different

Label_Go_On:
	
	inc	di
	jmp	Label_Cmp_FileName

Label_Different:

	and	di,	0FFE0h
	add	di,	20h
	mov	si,	KernelFileName
	jmp	Label_Search_For_LoaderBin

Label_Goto_Next_Sector_In_Root_Dir:
	
	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin

;=======	found kernel.bin name in root director struct
;将kernel.bin加载到7e00H并移动到200000H
Label_FileName_Found:

	call Label_Show_is_loading
	
	mov	ax,	RootDirSectors
	and	di,	0FFE0h
	add	di,	01Ah
	mov	cx,	word	[es:di]
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance
	mov	eax,	BaseTmpOfKernelAddr;BaseOfKernelFile
	mov	es,	eax
	mov	bx,	OffsetTmpOfKernelFile;OffsetOfKernelFile
	mov	ax,	cx

Label_Go_On_Loading_File:
	push	ax
	push	bx

	call Label_Show_Point

	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax	;保存着当前的簇号

;;;;;;;;;;;;;;;;;;;;;;;	将读取到的扇区复制到1M以上的地址空间
	push	cx
	push	eax
	push	fs
	push	edi
	push	ds
	push	esi

	mov	cx,	200h
	mov	ax,	BaseOfKernelFile	;0x00
	mov	fs,	ax					;0x00
	mov	edi,	dword	[OffsetOfKernelFileCount]	;0x200000

	mov	ax,	BaseTmpOfKernelAddr
	mov	ds,	ax					;0x00
	mov	esi,	OffsetTmpOfKernelFile	;0x7e00

Label_Mov_Kernel:	;------------------
	;将源地址的数据，复制到目的地址(0x7e00------>0x100000)
	mov	al,	byte	[ds:esi]	
	mov	byte	[fs:edi],	al		

	inc	esi
	inc	edi

	loop	Label_Mov_Kernel

	mov	eax,	0x1000
	mov	ds,	eax

	mov	dword	[OffsetOfKernelFileCount],	edi

	pop	esi
	pop	ds
	pop	edi
	pop	fs
	pop	eax
	pop	cx
;;;;;;;;;;;;;;;;;;;;;;;	上面的复制过程结束
	
	call	Func_GetFATEntry		;获取下一块簇号，继续
	cmp	ax,	0fffh
	jz	Label_File_Loaded
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
;	add	bx,	[BPB_BytesPerSec]	;和boot.asm不同，此时将复用0x7e00地址，保存下一块扇区

	jmp	Label_Go_On_Loading_File


;===================加载kernrl.bin完成(kernel.bin--->2M)====================================================

Label_File_Loaded:
	call Label_Show_File_Loaded		;显示完成加载kernel信息

;==================搜索并加载myMain.bin======================================================================
mov	word	[SectorNo],	SectorNumOfRootDirStart

;同boot.asm
Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0
	jz	Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]	
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h
	mov	ax,	[SectorNo]
	mov	cl,	1
	call	Func_ReadOneSector
	mov	si,	userFileName
	mov	di,	8000h
	cld
	mov	dx,	10h
	
Search_For_LoaderBin:

	cmp	dx,	0
	jz	Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11

Cmp_FileName:

	cmp	cx,	0
	jz	FileName_Found
	dec	cx
	lodsb	
	cmp	al,	byte	[es:di]
	jz	Go_On
	jmp	Different

Go_On:
	
	inc	di
	jmp	Cmp_FileName

Different:

	and	di,	0FFE0h
	add	di,	20h
	mov	si,	userFileName
	jmp	Search_For_LoaderBin

Goto_Next_Sector_In_Root_Dir:
	
	add	word	[SectorNo],	1
	jmp	Search_In_Root_Dir_Begin

FileName_Found:

	mov	ax,	RootDirSectors
	and	di,	0FFE0h
	add	di,	01Ah
	mov	cx,	word	[es:di]
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance
	mov	eax,	BaseTmpOfKernelAddr;BaseOfKernelFile
	mov	es,	eax
	mov	bx,	OffsetTmpOfKernelFile;OffsetOfKernelFile
	mov	ax,	cx

Go_On_Loading_File:
	push	ax
	push	bx

	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax	;保存着当前的簇号

;;;;;;;;;;;;;;;;;;;;;;;	将读取到的扇区复制到1M以上的地址空间
	push	cx
	push	eax
	push	fs
	push	edi
	push	ds
	push	esi

	mov	cx,	200h
	mov	ax,	BaseOfKernelFile	;0x00
	mov	fs,	ax					;0x00
	mov	edi,	dword	[OffsetOfuserFileCount]	;0x200000

	mov	ax,	BaseTmpOfKernelAddr
	mov	ds,	ax					;0x00
	mov	esi,	OffsetTmpOfKernelFile	;0x7e00

Mov_Kernel:	;------------------
	
	mov	al,	byte	[ds:esi]	
	mov	byte	[fs:edi],	al		

	inc	esi
	inc	edi

	loop	Mov_Kernel

	mov	eax,	0x1000
	mov	ds,	eax

	mov	dword	[OffsetOfuserFileCount],	edi

	pop	esi
	pop	ds
	pop	edi
	pop	fs
	pop	eax
	pop	cx
;;;;;;;;;;;;;;;;;;;;;;;	上面的复制过程结束
	
	call	Func_GetFATEntry		;获取下一块簇号，继续
	cmp	ax,	0fffh
	jz	File_Loaded
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance

	jmp	Go_On_Loading_File

File_Loaded:
	nop 
		
KillMotor:	;关闭软盘驱动
	
	push	dx
	mov	dx,	03F2h
	mov	al,	0	
	out	dx,	al
	pop	dx

;=======	get memory address size type
	call Label_Show_Mem_Message

	mov	ebx,	0
	mov	ax,	0x00
	mov	es,	ax
	mov	di,	MemoryStructBufferAddr	;0x7E00
;获取内存结构信息,将信息保存到物理地址0x7e00
Label_Get_Mem_Struct:

	mov	eax,	0x0E820
	mov	ecx,	20
	mov	edx,	0x534D4150
	int	15h
	jc	Label_Get_Mem_Fail
	add	di,	20				;每条内存信息占据20B
	inc	dword	[MemStructNumber]

	cmp	ebx,	0
	jne	Label_Get_Mem_Struct
	call Label_Show_Get_Mem_OK   

;=======	get SVGA information

	call Label_Show_SVGA_Mwssage
	mov	ax,	0x00
	mov	es,	ax
	mov	di,	0x8000		;将VbeInfoBlock信息保存到物理地址(0x8000)
	mov	ax,	4F00h		;调用该功能号，获取上面的信息

	int	10h

	cmp	ax,	004Fh		

	jz	.KO				;相等则支持该功能
	jmp Label_Get_SVGA_Fail

.KO:
	call label_Show_SVGA_finish

;=======	Get SVGA Mode Info

	call Label_Show_SVGAMode_Message

	mov	ax,	0x00
	mov	es,	ax
	mov	si,	0x800e

	mov	esi,	dword	[es:si]
	mov	edi,	0x8200

Label_SVGA_Mode_Info_Get:

	mov	cx,	word	[es:esi]
	cmp	cx,	0FFFFh
	jz	Label_SVGA_Mode_Info_Finish

	mov	ax,	4F01h
	int	10h

	cmp	ax,	004Fh

	jnz	Label_SVGA_Mode_Info_FAIL	

	inc	dword		[SVGAModeCounter]
	add	esi,	2
	add	edi,	0x100

	jmp	Label_SVGA_Mode_Info_Get

Label_SVGA_Mode_Info_Finish:

	call Label_Show_SVGA_Mode_Info_Finish

;=======	set the SVGA mode(VESA VBE)

	mov	ax,	4F02h
	mov	bx,	4143h	;========================mode : 0x180 or 0x143

	call Label_Show_Enter
	call Label_Read_Enter
	int 	10h

	cmp	ax,	004Fh
	jnz	Label_SET_SVGA_Mode_VESA_VBE_FAIL

;========================获取计算机信息完毕，进入保护模式,并运行head.s=========================================================


	jmp IA_32e_Mode  
	  
;=======================================失败信息================================================================
;不支持IA-32e


Label_SET_SVGA_Mode_VESA_VBE_FAIL:
	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0B00h		;row 11
	mov	cx,	33
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	SETSVGAModeVESAVBEFAIL
	int	10h
	jmp $

Label_SVGA_Mode_Info_FAIL:

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0A00h		;row 10
	mov	cx,	30
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAModeInfoErrMessage
	int	10h
	jmp $

Label_Get_SVGA_Fail:
	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0800h		;row 8
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAVBEInfoErrMessage
	int	10h
	jmp	$

;获取内存结构信息失败
Label_Get_Mem_Fail:

	mov	dword	[MemStructNumber],	0

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0600h		;row 6
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructErrMessage
	int	10h
	jmp	$

;没有找到kernel.bin
Label_No_LoaderBin:

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0300h		;row 3
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$
;================================FUN 函数调用=============================================

;=======	test support long mode or not(IA-32e)
[SECTION .s32]
[BITS 32]

support_long_mode:

	mov	eax,	0x80000000
	cpuid
	cmp	eax,	0x80000001
	setnb	al	
	jb	support_long_mode_done
	mov	eax,	0x80000001
	cpuid
	bt	edx,	29
	setc	al
support_long_mode_done:
	
	movzx	eax,	al
	ret

[SECTION .s16]
[BITS 16]

Label_Read_Enter:
	push ax
	mov ax, 0
	int 16h
	cmp al, 0Dh
	jnz Label_Read_Enter
	pop ax
	ret

Func_ReadOneSector:
	
	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl
	push	bx
	mov	bl,	[BPB_SecPerTrk]
	div	bl
	inc	ah
	mov	cl,	ah
	mov	dh,	al
	shr	al,	1
	mov	ch,	al
	and	dh,	1
	pop	bx
	mov	dl,	[BS_DrvNum]
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

Label_Show_Point:           ;在当前光标位置显示".",显示后光标后移
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
    ret

Label_Show_Enter:
	push ax
	push bx
	push dx
	push cx
	push es
	push bp
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0B00h		;row 11
	mov	cx,	31
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	EnterIn
	int	10h
	pop bp
	pop es
	pop cx
	pop dx
	pop bx
	pop ax
	ret

Label_Show_SVGA_Mode_Info_Finish:
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0A00h		;row 10
	mov	cx,	45
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAModeInfoOKMessage
	int	10h
	ret

Label_Show_SVGAMode_Message:
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0900h		;row 9
	mov	cx,	39
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetSVGAModeInfoMessage
	int	10h
	ret
label_Show_SVGA_finish:
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0800h		;row 8
	mov	cx,	44
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAVBEInfoOKMessage
	int	10h
	ret
Label_Show_SVGA_Mwssage:
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0700h		;row 7
	mov	cx,	38
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetSVGAVBEInfoMessage
	int	10h
	ret

Label_Show_Get_Mem_OK:
	
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0600h		;row 6
	mov	cx,	44
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructOKMessage
	int	10h	
	ret

Label_Show_Mem_Message:
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0500h		;row 5
	mov	cx,	38
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetMemStructMessage
	int	10h
	ret

Label_Show_File_Loaded:

	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0400h		;row 4
	mov	cx,	52
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	LoadFinish
	int	10h
	ret

Label_Show_Start_loader:
	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0200h		;row 2
	mov	cx,	20
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartLoaderMessage
	int	10h
	ret

Label_Show_is_loading:
	push ax
	push bx
	push dx
	push cx
	push es
	push bp
	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0300h		;row 3
	mov	cx,	30
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	isLoading
	int	10h
	pop bp
	pop es
	pop cx
	pop dx
	pop bx
	pop ax
	ret
;=======	open address A20
Label_Open_A20:
	;使用A20 快速们开启A20功能，使得可以寻址1M以上的地址空间
	push	ax
	in	al,	92h
	or	al,	00000010b
	out	92h,	al
	pop	ax

	cli

	;初始GDTR寄存器
	lgdt	[GdtPtr]	

	mov	eax,	cr0
	or	eax,	1
	mov	cr0,	eax			;将CR0寄存器的第0位设置为1，开启保护模式

	mov	ax,	SelectorData32	
	mov	fs,	ax				;将保护模式的数据段选择子加载到FS寄存器
	mov	eax,	cr0
	and	al,	11111110b
	mov	cr0,	eax			;将CR0寄存器的第0位设置为1，关闭保护模式

	sti
	ret

;=======	get FAT Entry

Func_GetFATEntry:

	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0
	mov	bx,	3
	mul	bx
	mov	bx,	2
	div	bx
	cmp	dx,	0
	jz	Label_Even
	mov	byte	[Odd],	1

Label_Even:

	xor	dx,	dx
	mov	bx,	[BPB_BytesPerSec]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	SectorNumOfFAT1Start
	mov	cl,	2
	call	Func_ReadOneSector
	
	pop	dx
	add	bx,	dx
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1
	jnz	Label_Even_2
	shr	ax,	4

Label_Even_2:
	and	ax,	0FFFh
	pop	bx
	pop	es
	ret



;========================================================================进入保护模式和IA-32e模式===========================================



;=======	init IDT GDT goto protect mode 
IA_32e_Mode:
	mov	ax,	cs
	mov	ds,	ax
	mov	fs,	ax
	mov	es,	ax

	cli
	

	db	66h
	lgdt	[GdtPtr]

	mov	eax,	cr0
	or	eax,	1
	mov	cr0,	eax				;开启保护模式
	
	jmp	dword SelectorCode32:GO_TO_TMP_Protect

[SECTION .s32]
[BITS 32]

GO_TO_TMP_Protect:

;=======	go to tmp long mode
	;将数据段选择子保存到数据段寄存器
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax

	mov	ss,	ax

	mov	esp,	7E00h

	call	support_long_mode
	test	eax,	eax

	jz	no_support
;====================================================支持IA-32e模式,设置页表开启该模式==================================================
;=======	init template page table 0x90000 make sure there is not dirty data

	;	线性地址(64位)[00000000000,00000H-0000000000a,00000H]------映射----->/[0,00000H-a,00000H]
	;   线性地址 [FFFF8000000,00000-FFFF800000a,00000]--------------------->/
	;每条页表项占8B
	;PML4页表，基址是90000H
	mov	dword	[0x90000],	0x91007
	mov	dword	[0x90004],	0x00000

	mov	dword	[0x90800],	0x91007
	mov	dword	[0x90804],	0x00000

	;PDPT页表
	mov	dword	[0x91000],	0x92007
	mov	dword	[0x91004],	0x00000

	;PDT页表，最终的页表。
	;本次的页大小的2M，不是4k
	mov	dword	[0x92000],	0x000083
	mov	dword	[0x92004],	0x000000

	mov	dword	[0x92008],	0x200083
	mov	dword	[0x9200c],	0x000000

	mov	dword	[0x92010],	0x400083
	mov	dword	[0x92014],	0x000000

	mov	dword	[0x92018],	0x600083
	mov	dword	[0x9201c],	0x000000

	mov	dword	[0x92020],	0x800083
	mov	dword	[0x92024],	0x000000

	mov	dword	[0x92028],	0xa00083
	mov	dword	[0x9202c],	0x000000

;=======	load GDTR

	db	66h
	lgdt	[GdtPtr64]
	
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	ss,	ax

	mov	esp,	7E00h

;=======	open PAE
	
	mov	eax,	cr4
	bts	eax,	5
	mov	cr4,	eax

;=======	load	cr3
	
	mov	eax,	0x90000
	mov	cr3,	eax
	
;=======	enable long-mode

	mov	ecx,	0C0000080h		;IA32_EFER
	rdmsr

	mov	eax,	00000101h
	wrmsr
	
;=======	open PE and paging
	
	mov	eax,	cr0
	bts	eax,	0
	bts	eax,	31
	mov	cr0,	eax
	
	;跳转到kernel代码

	jmp	SelectorCode64:OffsetOfKernelFile 

no_support:
	jmp $

;=======	tmp IDT
[SECTION .s16]
[BITS 16]

IDT:
	times	0x50	dq	0
IDT_END:

IDT_POINTER:
		dw	IDT_END - IDT - 1
		dd	IDT

;=======	tmp variable======================================================================================

RootDirSizeForLoop	dw	RootDirSectors
SectorNo		dw	0
Odd			db	0
OffsetOfKernelFileCount	dd	OffsetOfKernelFile

MemStructNumber		dd	0

SVGAModeCounter		dd	0

DisplayPosition		dd	0

OffsetOfuserFileCount dd OffsetofuserFile

;File name

KernelFileName:		db	"KERNEL  BIN",0
userFileName:		db	"MYMAIN  BIN",0
;=======	display messages

StartLoaderMessage:	db	"[KePOS.loader] Start"
NoLoaderMessage:	db	"ERROR:No KERNEL Found"

isLoading:			db	"[KePOS.loader] Load kernel.bin"
LoadFinish:			db	"[KePOS.loader] Load kernel.bin(at 0x200000) complete"

StartGetMemStructMessage:	db	"[KePOS.loader] Start Get Memory Struct"
GetMemStructErrMessage:	db	"ERROR:Get Memory Struct ERROR"
GetMemStructOKMessage:	db	"[KePOS.loader] Get Memory Struct SUCCESSFUL!"

StartGetSVGAVBEInfoMessage:	db	"[KePOS.loader] Start Get SVGA VBE Info"
GetSVGAVBEInfoErrMessage:	db	"ERROR:Get SVGA VBE Info ERROR"
GetSVGAVBEInfoOKMessage:	db	"[KePOS.loader] Get SVGA VBE Info SUCCESSFUL!"

StartGetSVGAModeInfoMessage:	db	"[KePOS.loader] Start Get SVGA Mode Info"
GetSVGAModeInfoErrMessage:	db	"ERROR:Get SVGA Mode Info ERROR"
GetSVGAModeInfoOKMessage:	db	"[KePOS.loader] Get SVGA Mode Info SUCCESSFUL!"

SETSVGAModeVESAVBEFAIL:		db 	"ERROR:Set SVGA Mode VESA VBE FAIL"

EnterIn:                    db  "Please press enter to continue:"
