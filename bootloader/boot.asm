;KePOS boot程序

org	0x7c00	

BaseOfStack	equ	0x7c00

BaseOfLoader	equ	0x1000          ;设置loader.bin代码的起始物理地址
OffsetOfLoader	equ	0x00

;=========================软盘文件系统(FAT12)相关数据====================================================
RootDirSectors	equ	14              ;软盘根目录占14个扇区
SectorNumOfRootDirStart	equ	19      ;根目录的开始扇区号
SectorNumOfFAT1Start	equ	1       ;FAT1表的开始扇区号
SectorBalance	equ	17	            ;FAT表中的簇号(软盘每簇1个扇区)和数据区扇区号的对应关系----->簇号+SectorBalance+RootDirSectors=扇区号

	jmp	short Label_Start
	nop
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
	BPB_HiddSec	dd	0
	BPB_TotSec32	dd	0
	BS_DrvNum	db	0
	BS_Reserved1	db	0
	BS_BootSig	db	0x29
	BS_VolID	dd	0
	BS_VolLab	db	'boot loader'
	BS_FileSysType	db	'FAT12   '

Label_Start:
;初始化cs,ds,es,ss,sp寄存器值
	mov	ax,	cs
	mov	ds,	ax          ;ds,es,ss=0
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack ;sp=7c00H

;=======	clear screen

	mov	ax,	0600h
	mov	bx,	0700h
	mov	cx,	0
	mov	dx,	0184fh
	int	10h

;=======	set focus,焦点位于(0,0)

	mov	ax,	0200h
	mov	bx,	0000h
	mov	dx,	0000h
	int	10h

    call Lable_Show_Start_boot      ;显示 [KePOS]: Start Boot

;=======	reset floppy

	xor	ah,	ah
	xor	dl,	dl
	int	13h

;=======	search loader.bin==========================================================================================
	mov	word	[SectorNo],	SectorNumOfRootDirStart     ;SectorNo变量保存：根目录的开始扇区;该变量保存根目录当前被读取的扇区号

;在根目录中查找文件
Lable_Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0       ;变量RootDirSizeForLoop保存根目录包含的扇区数
	jz	Label_No_LoaderBin                      ;遍历每个根目录扇区，如果没有找到loader.bin则跳到Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]	        
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h                               ;Func_ReadOneSector参数->[es:bx]-扇区加载到内存的地址
	mov	ax,	[SectorNo]                          ;Func_ReadOneSector参数->ax-查找的扇区号
	mov	cl,	1                                   ;Func_ReadOneSector参数->cl-一次从磁盘取几个扇区
	call	Func_ReadOneSector                  ;将上述参数传给Func_ReadOneSector函数
	mov	si,	LoaderFileName                      ;将加载的文件名字首地址保存在si中
	mov	di,	8000h                               ;di设置为刚读取的扇区所在的内存开始地址
	cld                                         ;将DF设置为0，则lodsb指令会使得si自动增加
	mov	dx,	10h                                 ;每个扇区可容纳的目录项个数（512 / 32 = 16 = 0x10）

;在某个目录项中寻找该文件	
Label_Search_For_LoaderBin:

	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir      ;如该扇区所有的目录项中都不包括待加载的文件名字，则读取下一个扇区
	dec	dx
	mov	cx,	11                                  ;FAT12目录项中，文件占据11个字节:文件名(8B),文件后缀(3B)
;比较某个目录项中是否有上述的文件
Label_Cmp_FileName:                             

	cmp	cx,	0
	jz	Label_FileName_Found                    ;有则跳转到Label_FileName_Found
	dec	cx
	lodsb	                                    ;从[DS:(R|E)SI]寄存器指定的内存地址中读取数据到AL寄存器,因为DF=0,读完一个字节给si+1
	cmp	al,	byte	[es:di]                     ;比较是否相等
	jz	Label_Go_On                             ;相等则继续读取下一个字节比较，调用Label_Go_On
	jmp	Label_Different                         

Label_Go_On:
	
	inc	di                  
	jmp	Label_Cmp_FileName

Label_Different:

	and	di,	0ffe0h                              ;目录项大小是32B，将di对齐到该目录项开始地址处
	add	di,	20h                                 ;将di+32指向下一个目录项开始地址
	mov	si,	LoaderFileName                      ;重新设置si指向文件名的开始地址
	jmp	Label_Search_For_LoaderBin              ;在下一个目录项中寻找该文件

;读取根目录的下一个扇区
Label_Goto_Next_Sector_In_Root_Dir:
	
	add	word	[SectorNo],	1         ;扇区号加1
	jmp	Lable_Search_In_Root_Dir_Begin          



;=======	found loader.bin name in root director struct

Label_FileName_Found:

	mov	ax,	RootDirSectors                      ;ax=根目录占的扇区数
	and	di,	0ffe0h                              ;对齐di到该目录项的开始地址
	add	di,	01ah                                ;簇号在目录项的1aH偏移处,di->簇号
	mov	cx,	word	[es:di]                     ;将簇号保存在cx中
	push	cx                                  ;将开始簇号保存在栈中
    ;簇号+SectorBalance+RootDirSectors=数据区扇区号                                
	add	cx,	ax                                 
	add	cx,	SectorBalance    
    ;设置loader.bin文件的内存物理地址[es:bx]=10000H                   
	mov	ax,	BaseOfLoader                         ;加载该文件到内存的段值
	mov	es,	ax
	mov	bx,	OffsetOfLoader                       ;加载该文件到内存的偏移值
	mov	ax,	cx
    ;加载文件到内存(ax->文件所在开始扇区号，[es:bx]->加载到的内存地址)
Label_Go_On_Loading_File:
	push	ax
	push	bx
	call    Label_Show_Point                      ;调用前保存ax,和bx的值到栈中
	pop	bx
	pop	ax

	mov	cl,	1                                     
	call	Func_ReadOneSector                    
	pop	ax                                        ;之前保存在栈中的簇号
	call	Func_GetFATEntry                      ;调用Func_GetFATEntry获取下一个簇号,返回值->ax
	cmp	ax,	0fffh                                 ;FAT12,簇号为0fffH,表示文件读取完成
	jz	Label_File_Loaded
	push	ax                                    ;将簇号保存在栈中
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
	add	bx,	[BPB_BytesPerSec]
	jmp	Label_Go_On_Loading_File                      

;loader.bin文件加载完成，跳转到loader程序
Label_File_Loaded:
	
	jmp	BaseOfLoader:OffsetOfLoader             ;跳转到loader.bin程序

;=======	display on screen : ERROR:No LOADER Found

Label_No_LoaderBin:

	mov	ax,	1301h
	mov	bx,	008ch
	mov	dx,	0100h    ;1行,0列
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

;=======    fun 调用=======================================================================
;=======	display on screen : [KePOS] Start Booting......

Label_Show_Point:           ;在当前光标位置显示".",显示后光标后移
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
    ret

Lable_Show_Start_boot:
    mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0000h   ;0行，0列
	mov	cx,	19
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage
	int	10h
    ret



;=======	read one sector from floppy

Func_ReadOneSector:
	;将LBA（Logical Block Address逻辑块寻址->转成->CHS（Cylinder/Head/Sector，柱面/磁头/扇区）格式的磁盘扇区号。
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
    ;地上转换完成，调用int 13h中断，读取一个扇区
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
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
	and	ax,	0fffh
	pop	bx
	pop	es
	ret
;=======	tmp variable

RootDirSizeForLoop	dw	RootDirSectors
SectorNo		dw	0
Odd			db	0

;=======	display messages==============================================================

StartBootMessage:	db	"[KePOS.boot]: Start"
NoLoaderMessage:	db	"ERROR:Can't find loader"
LoaderFileName:		db	"LOADER  BIN",0


;=======	fill zero until whole sector

	times	510 - ($ - $$)	db	0
	dw	0xaa55