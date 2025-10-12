; test IRQ HW protected mode, GD/C 5/10/25 DAI CON STA GUERRA CAZZO!!!

org 0x100

cpu 286

stack_ring0		equ	0x8000
stack			equ	0x8000

jmp boot                  ; was CS = 0x0000 on start up

gdt_start:
gdt_null:                   ; null descriptor (required)
		dq  0

gdt_code_ring0:							; code segment descriptor (0x000000 - 0x00ffff)
		dw  0xffff              ; limit
		dw  0                   ; base lower 16 bits
		db  0                   ; base upper 8 bits
		db  10011010b           ; present, privilege level 0 (highest privilege), code, non-conforming, readable
		dw  0                   ; (not used in 80286) D bit is cleaed then operand length and EA are assumed to be 16 bits long

gdt_data_ring0:							; data segment descriptor (0x000000 - 0x00ffff)
		dw  0xffff              ; limit
		dw  0                   ; base lower 16 bits
		db  0                   ; base upper 8 bits
		db  10010010b           ; present, privilege level 0 (highest privilege), data, expand-up, writable
		dw  0                   ; (not used in 80286) D bit is cleaed then operand length and EA are assumed to be 16 bits long

gdt_code_ring3:							; code segment descriptor (0x000000 - 0x00ffff)
		dw  0xffff              ; limit
		dw  0                   ; base lower 16 bits
		db  0                   ; base upper 8 bits
		db  11111010b           ; present, privilege level 3 (least privilege), code, non-conforming, readable
		dw  0                   ; (not used in 80286) D bit is cleaed then operand length and EA are assumed to be 16 bits long

gdt_data_ring3:							; data segment descriptor (0x000000 - 0x00ffff)
		dw  0xffff              ; limit
		dw  0                   ; base lower 16 bits
		db  0                   ; base upper 8 bits
		db  11110010b           ; present, privilege level 3 (least privilege), data, expand-up, writable
		dw  0                   ; (not used in 80286) D bit is cleaed then operand length and EA are assumed to be 16 bits long

gdt_vram:                   ; data segment descriptor (VGA text color video memory: 0x0b8000-0x0b8fff)
		dw  80 * 25 * 2 - 1     ; limit
		dw  0x8000              ; base lower 16 bits
		db  0x0b                ; base upper 8 bits
		db  11110010b           ; present, privilege level 3 (privilege), data, expand-up, writable
		dw  0                   ; (not used in 80286) D bit is cleaed then operand length and EA are assumed to be 16 bits long

gdt_tss:									; TSS descriptor
		dw	tss_end - tss  - 1	; limit
		dw	tss								; base lower 16 bits
		db	0								; base upper 8 bits
		db	10000001b						; present, privilege level 0 (highest privilege), system, available TSS
		dw	0								; (not used in 80286)
gdt_end:

idt_start:									; bare minimum IDT
		times 2	dq	0
; METTERE un dummy per eccezione/i!

nmi:										; NMI int 2
		dw	doreset							; offset
		dw	gdt_code_ring0 - gdt_start	; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

		times	(13+16) dq 0


; (fino a 32 sono riservati per eccezioni 286! rilocare...

int20h:										; int 20h (futuro BIOS/DOS)
		dw	doint20							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000111b						; present, privilege level 0 (max privilege), system, trap
		dw	0								; (not used in 80286)

int21h:										; int 21h (futuro BIOS/DOS)
		dw	doint21							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000111b						; present, privilege level 0 (max privilege), system, trap
		dw	0								; (not used in 80286)

		times	3 dq 0

int25h:										; int 25h (futuro BIOS/DOS)
		dw	doint25							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000111b						; present, privilege level 0 (max privilege), system, trap
		dw	0								; (not used in 80286)

int26h:										; int 26h (futuro BIOS/DOS)
		dw	doint26							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000111b						; present, privilege level 0 (max privilege), system, trap
		dw	0								; (not used in 80286)

		times	(9+16) dq 0

int10h:										; int 10h (futuro video) diventa 40h
		dw	doint10							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000111b						; present, privilege level 0 (max privilege), system, trap
		dw	0								; (not used in 80286)

		times	2 dq 0

int13h:										; int 13h (futuro disk I/O)  diventa 43h
		dw	doint13							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000111b						; present, privilege level 0 (max privilege), system, trap
		dw	0								; (not used in 80286)

		dq 0

int15h:										; int 15h (futuro BIOS) diventa 45h
		dw	doint15							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000111b						; present, privilege level 0 (max privilege), system, trap
		dw	0								; (not used in 80286)

		times	10 dq 0

		times	16 dq 0
		times	16 dq 0

int70h:
		times	8 dq 0

int8:										; int 78 (timer remapped)
		dw	irqtimer						; offset
		dw	gdt_code_ring0 - gdt_start	; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

int9:										; int 79 (keyboard remapped)
		dw	irqkb						; offset
		dw	gdt_code_ring0 - gdt_start	; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

		times 2 dq 0

int12:										; int 82 (mouse/com1 remapped)
		dw	irqmouse						; offset
		dw	gdt_code_ring0 - gdt_start	; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

		times 3 dq 0			; altri 3 hw (128 in tutto)

idt_end:

gdt_descriptor:
		dw  gdt_end - gdt_start - 1 ; limit of the gdt
		dd  gdt_start           ; physical starting address of the gdt

idt_descriptor:
		dw	idt_end - idt_start - 1	; limit of the idt
		dd	idt_start				; physical starting address of the idt

code_seg_ring0	equ gdt_code_ring0 - gdt_start
data_seg_ring0	equ gdt_data_ring0 - gdt_start
code_seg_ring3	equ gdt_code_ring3 - gdt_start
data_seg_ring3	equ gdt_data_ring3 - gdt_start
vram_seg				equ gdt_vram - gdt_start
tss_seg					equ	gdt_tss - gdt_start

; Interrupts also cause stack change. The new stack pointer value (for ring 0, in this program) is
; loaded from TSS and therefore, TSS is set up.
tss:
		dw	0					; previous TSS
		dw	stack_ring0				; sp for CPL 0
		dw	data_seg_ring0		; ss for CPL 0
		; everything below here is unused
		dw	0					; sp for CPL 1
		dw	0					; ss for CPL 1
		dw	0					; sp for CPL 2
		dw	0					; ss for CPL 2
		dw	0					; ip
		dw	0					; flags
		dw	0					; ax
		dw	0					; cx
		dw	0					; dx
		dw	0					; bx
		dw	0					; sp
		dw	0					; bp
		dw	0					; si
		dw	0					; di
		dw	0					; es selector
		dw	0					; cs selector
		dw	0					; ss selector
		dw	0					; ds selector
		dw	0					; task LDT selector
tss_end:

boot:
		cli

	mov ax,ds			; faccio relocation
	mov bl,ah			; da seg:ofs a phys
	shr bl,4			; byteH / baseH
	shl ax,4			; base
	add [gdt_code_ring0+2],ax
	add [gdt_code_ring0+4],bl
	add [gdt_data_ring0+2],ax
	add [gdt_data_ring0+4],bl
	add [gdt_code_ring3+2],ax
	add [gdt_code_ring3+4],bl
	add [gdt_data_ring3+2],ax
	add [gdt_data_ring3+4],bl
; NON fare su interrupt!

	add [gdt_tss+2],ax		
;	add [gdt_tss+4],bl		; no!

	add [gdt_descriptor+2],ax
	add [gdt_descriptor+4],bl
	add [idt_descriptor+2],ax
	add [idt_descriptor+4],bl


		lgdt    [gdt_descriptor]
		lidt		[idt_descriptor]

; codes for 80286           ; MSW register is a 16-bit register but only the lower 4 bits are used
		smsw ax             ; and it is a part of CR0 register in 80386 (or later).
		or ax, 1            ; set PE bit (bit 0)
		lmsw ax

; for 80386 and upwards     PG (Paging, bit 31) and ET (Extention Type, bit 4) are added to CR0.
;mov eax, cr0
;or  eax, 1
;mov cr0, eax
;
		jmp code_seg_ring0:start_pm_ring0

;[bits 32]  This directive should be removed. As the descriptors say, operands and effective address are 16 bits in size
textcolor1	equ 0x1b        ; BG color : blue, FG color : bright cyan, no blink
textcolor2	equ 0x8000		;0x1e + 0x80	; BG color : blue, FG color : yellow, with blink
textcolor3	equ 0x8001		;0x1e + 0x80	; BG color : blue, FG color : yellow, with blink
textcolor4	equ 0x8002

start_pm_ring0:							; now entered into the 16 bit-protected mode ring 0 from the real mode
		cli								; To avoid unintended interrupts. It is noted that software interrupt is not maskable. 
		cld

		mov ax, data_seg_ring0
		mov ds, ax
		mov ss, ax							; I know the ED bit is not set but it does not have to be set for use in SS
		mov sp, stack_ring0
		mov ax, (tss_seg) | 0
		ltr ax

		mov byte [textcolor2],0x1d
		mov byte [textcolor3],0x1e
		mov byte [textcolor4],0x18

		mov ax,vram_seg
		mov es, ax							; reload ES to make it point to video memory

		xor ah,ah
		mov al,3							; 80x25 color
		int 40h

; rimappo offset per non andare in conflitto
		mov al,0x15		;outb(PIC1_COMMAND, ICW1_INIT | ICW1_INTERVAL8 | ICW1_ICW4);  // starts the initialization sequence (in cascade mode, 286)
		out  0x20,al
		jmp $+2
		mov al,0x78		;outb(PIC1_DATA, 0x78);                 // ICW2: Master PIC vector offset = 78 ora, 0=Timer 1=Kbd
		out  0x21,al
		jmp $+2
		mov al,1<<2		;outb(PIC1_DATA, 1 << CASCADE_IRQ);        // ICW3: tell Master PIC that there is a slave PIC at IRQ2
		out  0x21,al
		jmp $+2
		mov al,0x1		;outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
		out  0x21,al
		jmp $+2
		mov al,0x0		;enable all ?? (TIMER, KB,  MOUSE)
		out  0x21,al
		jmp $+2

		mov al,0x0
		mov dx,0x3f8+3
		out dx,al
		mov al,0x0
		mov dx,0x3f8+4
		out dx,al
		mov al,0x1		; attivo COM1 IRQ (mouse
		mov dx,0x3f8+1
		out dx,al

		lea si, hello_msg
		xor di, di							; print a greeting message at the uppper left corner of the screen
;		mov ah, 0xf
;		call print_msg
		mov ah,0x13
		mov bl,0xf							; 80x25 color
		int 40h

		sti

; set up the stack frame iret expects
		push (data_seg_ring3) | 3	; data selector (ring 3 stack with bottom 2 bits set for ring 3)
		push stack_ring3					; sp (ring 3)
		pushf
		push (code_seg_ring3) | 3	; code selector (ring 3 code with bottom 2 bits set for ring 3)
		push start_pm_ring3
		iret								; make it get to ring 3


; interrupt handlers just for printing the message
irqtimer:
		pusha
		push ds
		push es
		mov al,0x20
		out  0x20,al
		mov di, 0x200						; print a message 
		lea si, timer_msg
		mov bl, [textcolor2]
		inc byte [ds:textcolor2]
		mov ah,0x13
		int 40h
;		call print_msg
		pop es
		pop ds
		popa
;		sti
		iret

irqkb:
		pusha
		push ds
		push es
		mov al,0x20
		out  0x20,al
		in al,0x60
		and al,0x7f
		mov dl,al
		mov di, 0x400						; print a message 
		lea si, kb_msg
		mov bl, [textcolor3]
		inc byte [ds:textcolor3]
		mov ah,0x13
		int 40h
;		call print_msg

		mov al,dl
		add di,9*2
		stosw
		pop es
		pop ds
		popa
;		sti
		iret

irqmouse:
		pusha
		push ds
		push es
		mov al,0x20
		out  0x20,al
		mov dx,0x3f8+0			; leggo 3 byte mouse
		in al,dx
		jmp $+2
		mov dx,0x3f8+5
		in al,dx

		mov dx,0x3f8+0			; 
		in al,dx
		jmp $+2
		mov dx,0x3f8+5
		in al,dx
		mov dx,0x3f8+0			; 
		in al,dx
		jmp $+2
		mov dx,0x3f8+5
		in al,dx

		mov di, 0x600						; print a message 
		lea si, mouse_msg
		mov bl, [textcolor4]
		inc byte [ds:textcolor4]
;		call print_msg
		mov ah,0x13
		int 40h

		pop es
		pop ds
		popa
;		sti
		iret


doint10:				; fare come https://www.stanislavs.org/helppc/int_10.html
		pusha
		pushf
		push ds
		push es
		push ax
		cld
		mov ax, data_seg_ring0				; era ring3...
		mov ds, ax							; reload DS
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		pop ax

		cmp  ah,0			; usare jump table
		jz  .mode
		cmp  ah,0x9
		jz  .writetextattrib
		cmp  ah,0xa
		jz  .writetext
		cmp  ah,0xe
		jz  .writechar
		cmp  ah,0x13
		jz  .writestring

		jmp .bad

.mode:					; https://www.stanislavs.org/helppc/int_10-0.html
						; if AL bit 7=1, prevents EGA,MCGA & VGA from clearing display
		test al, 0x80

		xor di, di
		mov ah, textcolor1			; clear screen
		mov al, ' ' 
		mov cx, 80 * 25
		cld
		rep stosw
		jmp .ok

.writetext:
.writetextattrib:
		mov ah,bl
.writetext_:
		stosw
		loop .writetext_
		jmp .ok

.writechar:
		mov ah,bl
		stosw
		jmp .ok

.writestring:
		mov ah,bl
.loop:
		lodsb
		or al,al
		jz .writestring_end
		stosw
		jmp .loop
.writestring_end:
.ok:
		clc
		jmp .end

.bad:
		stc

.end:

		pop es
		pop ds
		popf
		popa
;		sti
		iret

doint13:
		pusha
		push ds
		push es

		cmp  ah,0

		pop es
		pop ds
		popa
		sti
		iret

doint15:
		pusha
		push ds
		push es

		cmp  ah,0

		pop es
		pop ds
		popa
		sti
		iret

doint20:				; beh "Terminate" :)
		mov al,0xfe							; forzo reset!
		out 0x64,al
		iret

doint21:				; fare 
		pusha
		pushf
		push ds
		push es
		push ax
		cld
		mov ax, data_seg_ring0				; era ring3...
		mov ds, ax							; reload DS
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		pop ax

		cmp  ah,0			; usare jump table
		jz  .terminate
		cmp  ah,0x1
		jz  .readkeyecho
		cmp  ah,0x2
		jz  .writechar
		cmp  ah,0xf
		jz  .openfile
		cmp  ah,0x9
		jz  .writestring

		jmp .bad

.terminate:
		mov al,0xfe							; forzo reset!
		out 0x64,al

.readkeyecho:
		in al, 0x60
		jmp .ok

.writechar:
		mov al,dl
		mov ah,0xe
		int 40h
		jmp .ok

.openfile:

.writestring:
		mov ah,bl
.loop:
		mov al,[bx]
		inc bx
		cmp al,'$'
		jz .writestring_end
		stosw
		jmp .loop
.writestring_end:

.ok:
		clc
		jmp .end

.bad:
		stc

.end:
		iret

doint25:				; fare  disk read
		pusha
		pushf
		push ds
		push es
		push ax
		cld
		mov ax, data_seg_ring0				; era ring3...
		mov ds, ax							; reload DS
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		pop ax


		jmp .bad

.bad:
		stc
		iret

doint26:				; fare  disk write
		pusha
		pushf
		push ds
		push es
		push ax
		cld
		mov ax, data_seg_ring0				; era ring3...
		mov ds, ax							; reload DS
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		pop ax


		jmp .bad

.bad:
		stc
		iret


doreset:
		mov al,0xfe							; forzo reset!
		out 0x64,al
		iret


start_pm_ring3:
.loop2:
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		inc byte [es:0x800]
		jmp .loop2

print_msg:
		push ax
		mov ax, data_seg_ring0				; era ring3...
		mov ds, ax							; reload DS
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		pop ax
.loop:
		lodsb
		or al,al
		jz .end
		stosw
		jmp .loop
.end:
		ret

byte_hex:		; da glatick, fare anche per base 10
		push ax
		shr		al,4
		call	nib_hex
		pop ax

nib_hex:
		and	al, 0x0f
		cmp	al, 0x0a 
		sbb	al, -('0'+0x66+1)
		das
out_char:
		push	ax
		push	bx
		mov	bh, 0
		mov	ah, 0xe
		int	40H
		pop	bx
		pop	ax
		ret


hello_msg:
		db  "INTHW test 0.4", 0
timer_msg:
		db  "TIMER", 0
kb_msg:
		db  "KEYBOARD:", 0
mouse_msg:
		db  "MOUSE:", 0

; un po' di stack :)
		times 128 dw	0


stack_ring3:

		dw 0x4447              ; ;) bootable sector marker
