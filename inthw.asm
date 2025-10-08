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

		times	16 dq 0

int13h:										; int 13h (futuro disk I/O)
		dw	doint13							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

		dq 0

int15h:										; int 15h (futuro BIOS)
		dw	doint15							; offset
		dw	gdt_code_ring0 - gdt_start		; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

		times	10 dq 0

int8:										; int 32 (timer remapped)
		dw	irqtimer						; offset
		dw	gdt_code_ring0 - gdt_start	; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

int9:										; int 33 (keyboard remapped)
		dw	irqkb						; offset
		dw	gdt_code_ring0 - gdt_start	; destination selector
		db	0								; unused
		db	10000110b						; present, privilege level 0 (max privilege), system, interrupt
		dw	0								; (not used in 80286)

		times 30 dq 0			; boh 64 tanto per!
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

start_pm_ring0:							; now entered into the 16 bit-protected mode ring 0 from the real mode
		cli								; To avoid unintended interrupts. It is noted that software interrupt is not maskable. 

		mov ax, data_seg_ring0
		mov ds, ax
		mov ss, ax							; I know the ED bit is not set but it does not have to be set for use in SS
		mov sp, stack_ring0
		mov ax, (tss_seg) | 0
		ltr ax

		mov byte [textcolor2],0x1d
		mov byte [textcolor3],0x1e

		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		xor di, di
		mov ah, textcolor1			; clear screen
		mov al, ' ' 
		mov cx, 80 * 25
		cld
		rep stosw

; rimappo offset per non andare in conflitto
		mov al,0x15		;outb(PIC1_COMMAND, ICW1_INIT | ICW1_INTERVAL8 | ICW1_ICW4);  // starts the initialization sequence (in cascade mode, 286)
		out  0x20,al
		jmp $+2
		mov al,0x20		;outb(PIC1_DATA, 0x20);                 // ICW2: Master PIC vector offset = 32 ora, 0=Timer 1=Kbd
		out  0x21,al
		jmp $+2
		mov al,1<<2		;outb(PIC1_DATA, 1 << CASCADE_IRQ);        // ICW3: tell Master PIC that there is a slave PIC at IRQ2
		out  0x21,al
		jmp $+2
		mov al,0x1		;outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
		out  0x21,al
		jmp $+2
		mov al,0x0		;enable all ??
		out  0x21,al
		jmp $+2


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
		mov ax, data_seg_ring0				; era ring3...
		mov ds, ax							; reload DS
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		xor di, di							; print a greeting message at the uppper left corner of the screen
		lea si, hello_msg
		mov ah, [textcolor2]
		inc byte [ds:textcolor2]
.loop:
		lodsb
		cmp al, 0
		jz irqtimer_end
		stosw
		jmp .loop

irqtimer_end:
		mov al,0x20
		out  0x20,al
		pop es
		pop ds
		popa
;		sti
		iret

irqkb:
		pusha
		push ds
		push es
		in al,0x60
		and al,0x7f
		mov bl,al
		mov ax, data_seg_ring0				; era ring3...
		mov ds, ax							; reload DS
		mov ax, vram_seg
		mov es, ax							; reload ES to make it point to video memory
		mov di, 0x200						; print a message 
		lea si, kb_msg
		mov ah, [textcolor3]
		inc byte [ds:textcolor3]
.loop3:
		lodsb
		cmp al, 0
		jz irqkb_end
		stosw
		jmp .loop3

irqkb_end:
		mov al,bl
		stosw
		mov al,0x20
		out  0x20,al
		pop es
		pop ds
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


doreset:
		mov al,0xfe							; forzo reset!
		out 0x64,al
		iret


start_pm_ring3:
.loop2:
		inc byte [es:0x400]
		jmp .loop2

hello_msg:
		db  "TIMER", 0
kb_msg:
		db  "KEYBOARD:", 0

; un po' di stack :)
		dd	0
		dd	0
		dd	0
		dd	0
		dd	0
		dd	0
		dd	0
		dd	0
		dd	0
		dd	0


stack_ring3:

		dw 0x4447              ; ;) bootable sector marker
