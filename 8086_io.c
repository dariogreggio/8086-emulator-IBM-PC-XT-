//https://wiki.osdev.org/Floppy_Disk_Controller#Bit_MT



#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//#include <graph.h>
//#include <dos.h>
//#include <malloc.h>
//#include <memory.h>
//#include <fcntl.h>
//#include <io.h>
#include <xc.h>

#include "Adafruit_ST77xx.h"
#include "Adafruit_ST7735.h"
#include "adafruit_gfx.h"

#include "8086_PIC.h"
#include "8086_io.h"
#include "8086.h"

extern BYTE CPUPins;
#define UNIMPLEMENTED_MEMORY_VALUE 0x6868		//:)		

extern uint16_t _ip;
#if defined(EXT_80386)
	extern union REGISTERS32 segs;
	extern union SEGMENTS_DESCRIPTOR seg_descr;
#elif defined(EXT_80286)
//	union REGISTERS16 segs;
	extern union SEGMENTS_DESCRIPTOR segs;
#else
	extern union SEGMENTS_DESCRIPTOR segs;
#endif

#define _cs (&segs.r[1])



uint8_t ram_seg[RAM_SIZE+RAM_SIZE_EXTENDED];
//uint8_t rom_seg[ROM_SIZE];			bios_rom
#ifdef RAM_DOS
uint8_t disk_ram[0x400];
#endif
uint16_t VICRaster=0;
#ifdef CGA
#define CGA_BASE 0xB8000L
#define CGA_SIZE (16384 /*640*200/8*/)				// video (AMIbios sincazza se non sono 16384!
#endif
#ifdef MDA
#define CGA_BASE 0xB0000L
#define CGA_SIZE (32768 /*720*350/8*/)				// video
#endif
#ifdef VGA
#define VGA_BASE 0xA0000L		// 4 blocchi da 64K
#define VGA_SIZE (65536L*4)
		// v. VGAgraphReg[6] & 0x0c per dimensione e posizione finestra grafica...
#endif
#if defined(CGA) || defined(MDA) 
uint8_t CGAram[CGA_SIZE /* 640*200/8 320*200/2 */];		// anche MDA :) e VGA
#endif
#ifdef VGA
uint8_t VGAram[VGA_SIZE];
#define VGA_BIOS_BASE 0xC0000L					// https://flint.cs.yale.edu/feng/cos/resources/BIOS/mem.htm
uint8_t VGABios[32768];
//uint8_t VGAram[640*480*(24/8)];
uint8_t VGAreg[16],  // https://web.stanford.edu/class/cs140/projects/pintos/specs/freevga/vga/vga.htm
	VGAcrtcReg[32],VGAactlReg[32],VGAgraphReg[16],
	VGAFF,VGAptr;
uint16_t VGAsignature;
#endif
uint8_t CGAreg[18];  // https://www.seasip.info/VintagePC/cga.html
uint8_t MDAreg[18];  // https://www.seasip.info/VintagePC/mda.html
//  http://www.oldskool.org/guides/oldonnew/resources/cgatech.txt
uint8_t i8237RegR[16],i8237RegW[16],i8237FF=0,i8237Mode[4];
uint16_t i8237DMAAddr[4],i8237DMALen[4];
uint16_t i8237DMACurAddr[4],i8237DMACurLen[4];
#ifdef PCAT
uint8_t i8237FF2=0,i8237Mode2[4];
uint8_t i8237Reg2R[16],i8237Reg2W[16],
  i8237DMAAddr2[4],i8237DMALen2[4];
uint16_t i8237DMACurAddr2[4],i8237DMACurLen2[4];
#endif
uint8_t DMApage[8],DMArefresh, DIAGPort/*DTK 286 rilegge port 80h*/;
uint8_t i8259RegR[2],i8259RegW[2],i8259ICW[4],i8259OCW[3],i8259ICWSel,i8259IMR,i8259IRR,i8259ISR
#ifdef PCAT
	,i8259Reg2R[2],i8259Reg2W[2],i8259ICW2[4],i8259OCW2[3],i8259ICW2Sel,i8259IMR2,i8259IRR2,i8259ISR2,
	DMApage2[8] // per DTK
#endif
	;
#ifdef PCAT
uint8_t i8042RegR[2],i8042RegW[2],KBRAM[32],
/*Location 0 is the Command byte, see above.
	Location 0x13 (on MCA) is nonzero when a password is enabled.
	Location 0x14 (on MCA) is nonzero when the password was matched.
	Locations 0x16-0x17 (on MCA) give two make codes to be discarded during password matching*/
	i8042Input,i8042OutPtr;
#else
uint8_t i8255RegR[4],i8255RegW[4];
#endif

uint8_t i8253RegR[4],i8253RegW[4],i8253Mode[3],i8253Flags[3];	// uso MSB di Mode come "OUT" e b6 come "reload" (serve anche un "loaded" flag, credo, per one shot ecc
uint16_t i8253TimerR[3],i8253TimerW[3],i8253TimerL[3];
uint8_t LPT[3/**/][4 /*3*/];		// per ora, al volo!
uint8_t i8250Reg[8],i8250Regalt[3];
#ifdef PCAT
uint8_t i8042RegR[2],i8042RegW[2],KBRAM[32],
/*Location 0 is the Command byte, see above.
	Location 0x13 (on MCA) is nonzero when a password is enabled.
	Location 0x14 (on MCA) is nonzero when the password was matched.
	Locations 0x16-0x17 (on MCA) give two make codes to be discarded during password matching*/
	i8042Input,i8042OutPtr;
#else
uint8_t i8255RegR[4],i8255RegW[4];
#endif
uint8_t i8042Output;  //b1=A20, b0=Reset,  SERVE cmq per 286 :) A20
uint8_t KBCommand=0b01010000,
/* |7|6|5|4|3|2|1|0|	8042 Command Byte  OCCHIO è diverso da XT!!
		| | | | | | | `---- 1=enable output register full interrupt; OVVERO su XT lo uso per segnalare Reset KB
		|	| | | | | `----- should be 0 (Second PS/2 port interrupt (1 = enabled, 0 = disabled, only if 2 PS/2 ports supported)
		| | | | | `------ 1=set status register system, 0=clear (System Flag (1 = system passed POST, 0 = your OS shouldn't be running) 
		| | | | `------- 1=override keyboard inhibit, 0=allow inhibit
		| | | `-------- disable keyboard I/O by driving clock line low
		| | `--------- disable Second/auxiliary device, drives clock line low
		| `---------- IBM scancode translation 0=AT, 1=PC/XT; OVVERO RESET KEYBOARD su XT: 0=hold keyboard clock low, 1=enable clock 
		`----------- reserved, should be 0; OVVERO blocca linea su XT: 0=enable keyboard read, 1=clear */
KBStatus;  
/*|7|6|5|4|3|2|1|0|  8042 Status Register
	 | | | | | | | `---- output register (60h) has data for system
	 | | | | | | `----- input register (60h/64h) has data for 8042
	 | | | | | `------ system flag (set to 0 after power on reset)
	 | | | | `------- data in input register is command (1) or data (0)
	 | | | `-------- 1=keyboard enabled, 0=keyboard disabled (via switch su PCAT) PARE l'OPPOSTO cmq :(
	 | | `--------- 1=transmit timeout (data transmit not complete)
	 | `---------- 1=receive timeout (data transmit not complete)
	 `----------- 1=even parity rec'd, 0=odd parity rec'd (should be odd)*/
uint8_t MachineFlags=0b00000000;		// b0-1=Speaker on; b2-3=TURBO PC (o select); b4-5=Parity/I/O errors su TINYBIOS; b66= 0:KC clk low; b7= 0 enable KB    https://wiki.osdev.org/Non_Maskable_Interrupt
// v. cmq i8255RegR[2]
uint8_t MachineSettings=0b00101101;		// b0=floppy (0=sì); b1=FPU; bits 2,3 memory size (64K bytes); b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25; b7:6 #floppies-1
		// ma su IBM5160 b0=1 SEMPRE dice!
uint8_t i6845RegR[18],i6845RegW[18];
uint8_t i146818RegR[2],i146818RegW[2],i146818RAM[64];
uint8_t FloppyContrRegR[2],FloppyContrRegW[2],FloppyFIFO[16],FloppyFIFOPtrR,FloppyFIFOPtrW; // https://www.ardent-tool.com/floppy/Floppy_Programming.html#FDC_Registers
uint8_t floppyState[4]={FLOPPY_STATE_DISKPRESENT,0x00,0,0} /* b0=stepdir,b1=disk-switch,b2=motor,b5=xfer,b6=DIR,b7=diskpresent*/,
  floppyHead[4]={0,0,0,0},floppyTrack[4]={0,0,0,0},floppySector[4]={0,0,0,0},
	floppylastCmd[4],fdDisk;
uint8_t floppyTimer /*simula attività floppy*/;
#ifndef PCAT
#ifdef ROM_HD_BASE
uint8_t HDContrRegR[4],HDContrRegW[4],HDFIFOR[16],HDFIFOW[16],HDFIFOPtrR,HDFIFOPtrW, // 
  hdControl=0,hdState=0,hdHead=0,hdSector=0;
#endif
#else
uint8_t HDContrRegR[8],HDContrRegW[8],// OVVIAMENTE se uso HD AT NON uso quello XT :)
  hdControl=0,hdState=0,hdError=0,hdDisc=0,hdHead=0,hdSector=0,hdPrecomp=0,numSectors=0;
uint16_t HDFIFOPtrR,HDFIFOPtrW;
#endif
uint16_t hdCylinder=0;
uint8_t hdTimer /*simula attività HD*/;
#define HD_SECTORS_PER_CYLINDER 17				// 305/2/17 = 5MB, dovrebbe; 374/2/17 = 24MB  305/4/17 = 10MB
#define HD_CYLINDERS 305
#define HD_HEADS 4
#define SECTOR_SIZE 512			// sia floppy che HD

#if MS_DOS_VERSION==1
BYTE sectorsPerTrack[4]={8,8,8,8} /*floppy320*/;
BYTE totalTracks=80;		/*floppy720*/
#elif MS_DOS_VERSION==2
BYTE sectorsPerTrack[4]={9,9,9,9} /*floppy360 v. tracks*/;
//BYTE totalTracks=40;		/*floppy360*/
BYTE totalTracks=80;		/*floppy720*/
#elif MS_DOS_VERSION==3
//BYTE sectorsPerTrack[4]={18,18,18,18} /*floppy1.4*/;
BYTE sectorsPerTrack[4]={9,9,9,9} /*floppy720 v. tracks*/;
BYTE totalTracks=80;		/*floppy1.4*/
#endif
static uint8_t sectorData[SECTOR_SIZE];

extern const uint8_t *MSDOS_DISK[2];
const unsigned char *msdosDisk;
uint32_t getDiscSector(uint8_t);
uint8_t *encodeDiscSector(uint8_t);
void setFloppyIRQ(uint8_t);
#define DEFAULT_FLOPPY_DELAYED_IRQ 3
#define SEEK_FLOPPY_DELAYED_IRQ 30

extern const unsigned char MSDOS_HDISK[]; //:)  se lo lascio qua lo mette in ram sto frocio, PERCHE'???
const unsigned char *msdosHDisk;
uint32_t getHDiscSector(uint8_t,uint8_t lba);
uint8_t *encodeHDiscSector(uint8_t);
void setHDIRQ(uint8_t);
#define DEFAULT_HD_DELAYED_IRQ 3
#define SLOW_HD_DELAYED_IRQ 7

extern BYTE CPUDivider;
BYTE mouseState=0b10000000;
BYTE COMDataEmuFifo[5];
BYTE COMDataEmuFifoCnt;
uint8_t Keyboard[1]={0};
uint8_t ColdReset=1;
union INTERRUPT_WORD ExtIRQNum,IntIRQNum;
uint8_t LCDdirty;
extern uint8_t Pipe1;
extern union PIPE Pipe2;


#if defined(EXT_80186) // no, pare|| defined(EXT_NECV20)
#define MAKE20BITS(a,b) (((((uint32_t)(a)) << 4) + ((uint16_t)(b))))		// somma, non OR - e il bit20/A20 può essere usato per HIMEM
#else
#define MAKE20BITS(a,b) (0xfffff & ((((uint32_t)(a)) << 4) + ((uint16_t)(b))))		// somma, non OR - e il bit20/A20 può essere usato per HIMEM
#endif
// su 8088 NON incrementa segmento se offset è FFFF ... 

#define ADDRESS_286_PRE() {\
	if(seg->s.x < 0x1000) {/*PATCH  LMSW   sistemare*/ \
		if(seg->s.x<3) {\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		if(!seg->d.Access.P) {\
			Exception86.descr.ud=EXCEPTION_NP;\
			goto exception286;\
			}\
		if(seg->d.Limit		&& ofs>(seg->d.Limit+1  /*SISTEMARE, MIGLIORARE*/)) {/*0=65536, ma v. anche DOWN Expand*/\
			Exception86.descr.ud=EXCEPTION_STACK;\
			goto exception286;\
			}\
		}\
	}
#define ADDRESS_286_POST() {\
	t=MAKELONG(seg->d.Base+ofs,seg->d.BaseH);\
	seg->d.Access.A=1;\
	}
#define ADDRESS_286_R() {\
	if(seg->s.x >= 0x1000)		/*PATCH  LMSW   sistemare*/ {\
		t=MAKE20BITS(seg->s.x,ofs);\
		}\
	else {\
		ADDRESS_286_PRE();\
		if((seg->d.Access.b & 0 /*in effetti anche code è ok... B8(00001000)*/) != 0b00000000) {/*boh data */\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		ADDRESS_286_POST();\
		}\
	}
#define ADDRESS_286_E() {\
	struct SEGMENT_DESCRIPTOR *sd;\
	if(seg->s.x >= 0x1000)		/*PATCH  LMSW   sistemare*/ {\
		t=MAKE20BITS(seg->s.x,ofs);\
		}\
	else {\
		ADDRESS_286_PRE();\
		if((seg->d.Access.b & 0b00001010) != 0b00001010) {/* code, readable*/\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		ADDRESS_286_POST();\
		}\
	}
#define ADDRESS_286_W() {\
	struct SEGMENT_DESCRIPTOR *sd;\
	if(seg->s.x >= 0x1000)		/*PATCH  LMSW   sistemare*/ {\
		t=MAKE20BITS(seg->s.x,ofs);\
		}\
	else {\
		ADDRESS_286_PRE();\
		if((seg->d.Access.b & 0b00001010) != 0b00000010) {/*data, writable*/\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		ADDRESS_286_POST();\
		}\
	}


uint8_t GetValue(struct REGISTERS_SEG *seg,uint16_t ofs) {
	register uint8_t i;
	uint32_t t;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_R();
		}
	else
#endif
	t=MAKE20BITS(seg->s.x,ofs);
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif

#ifdef EXT_80286
	if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
		i=ram_seg[t-0x100000L+RAM_SIZE];
		}
	else 
#endif
	if(t >= (ROM_END-ROM_SIZE)) {
		i=bios_rom[t-(ROM_END-ROM_SIZE)];
		}
#ifdef ROM_BASIC
	else if(t >= 0xF6000) {
		i=IBMBASIC[t-0xF6000];
		}
#endif
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		i=bios_rom_opt[t-ROM_START2];
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE && t<ROM_HD_BASE+32768) {
		i=HDbios[t-ROM_HD_BASE];
		}
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		i=VGABios[t-VGA_BIOS_BASE];
    }
#endif
#ifdef VGA_BASE
	else if(t >= VGA_BASE && t < VGA_BASE+0x20000L /*VGA_SIZE*/) {
		DWORD vgaBase,vgaSegm,vgaPlane;
		switch(VGAgraphReg[6] & 0x0c) {
			case 0x00:
				vgaBase=0xA0000L-VGA_BASE;
				vgaSegm=0x20000L;
				vgaPlane=0x10000;
				break;
			case 0x04:
				vgaBase=0xA0000L-VGA_BASE;
				vgaSegm=0x10000L;
				vgaPlane=0x10000;
				break;
			case 0x08:
				vgaBase=0xB0000L-VGA_BASE;
				vgaSegm=0x8000L;
				vgaPlane=0x2000;
				break;
			case 0x0c:
				vgaBase=0xB8000L-VGA_BASE;
				vgaSegm=0x8000L;
				vgaPlane=0x2000;
				break;
			}
		if(VGAgraphReg[4] & 0x08) {		// chain 4 mode
			goto vga_chain4;
//	??	Read mode 0 ignores the value in the Read Map Select Register when the VGA is in display mode 13 hex. In this mode, all four planes are chained together and a pixel is represented by one byte.
			}
		if(!(VGAseqReg[4] & 4)) {			//odd/even
			// v. anche b5 in Miscellaneous https://www.vogons.org/viewtopic.php?t=42454
			BOOL oldT=t & 1;
			t/=2;
			t &= (vgaPlane-1);
			t+=vgaBase;
			if(!oldT) {
				VGAlatch.bd[0]=VGAram[t];
				VGAlatch.bd[2]=VGAram[t];
				VGAlatch.bd[1]=VGAram[t+vgaPlane*2];
				VGAlatch.bd[3]=VGAram[t+vgaPlane*2];
				}
			else {
				VGAlatch.bd[0]=VGAram[t+vgaPlane];
				VGAlatch.bd[2]=VGAram[t+vgaPlane];
				VGAlatch.bd[1]=VGAram[t+vgaPlane*3];
				VGAlatch.bd[3]=VGAram[t+vgaPlane*3];
				}
			}
		else {
vga_chain4:
			t &= (vgaSegm-1); 
			t+=vgaBase;
			// forse qua c'entrano gli enable? ma cmq sarebbero superflui
			VGAlatch.bd[0]=VGAram[t];
			VGAlatch.bd[1]=VGAram[t+vgaPlane*1];
			VGAlatch.bd[2]=VGAram[t+vgaPlane*2];
			VGAlatch.bd[3]=VGAram[t+vgaPlane*3];
			}
		if(!(VGAgraphReg[5] & 0x08)) {
			if(VGAgraphReg[4] & 0x08) {		// chain 4 mode








// no, cazzata



				switch(t & 0x03) {		// read mode
					case 0x00:
						i=VGAlatch.bd[0];
						break;
					case 0x01:
						i=VGAlatch.bd[1];
						break;
					case 0x02:
						i=VGAlatch.bd[2];
						break;
					case 0x03:
						i=VGAlatch.bd[3];
						break;
					}
				}
			else {
				switch(VGAgraphReg[4] & 0x03) {		// read mode
					case 0x00:
						i=VGAlatch.bd[0];
						break;
					case 0x01:
						i=VGAlatch.bd[1];
						break;
					case 0x02:
						i=VGAlatch.bd[2];
						break;
					case 0x03:
						i=VGAlatch.bd[3];
						break;
					}
				}
			}
		else {		// comparison...
			BYTE j,k,k1;
			i=0xff;
			for(j=1; j; j<<=1) {
				for(k=1,k1=0; k & 0xf; k<<=1,k1++) {
					if(!(VGAgraphReg[7] & k)) {			// color don't care map
						if(!!(VGAlatch.bd[k1] & j) != !!(VGAgraphReg[2] & k)) {			// color compare map
							i &= ~j;
							}
						}
					}
				}
			}
    }
#endif
#ifdef CGA_BASE
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		i=CGAram[t];
    }
#endif
#ifdef RAM_DOS
	else if(t >= 0x7c000 && t < 0x80000) {
		i=disk_ram[t-0x7c000];
//		i=iosys[t-0x7c00];
		}
#endif
	else if(t < RAM_SIZE) {
		i=ram_seg[t];
		}
// else _f.Trap=1?? v. anche eccezione PIC
	else {
		i=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
#ifdef EXT_80286
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa: ;
#endif
		}

	return i;
	}

uint8_t InValue(uint16_t t) {
	register uint8_t i;

  switch(t >> 4) {
    case 0:        // 00-1f DMA 8237 controller
    case 1:					// solo PS/2 dice!
      t &= 0xf;
			switch(t) {
				case 0:
				case 2:
				case 4:
				case 6:		// reading returns only the current (remaining) value...
					if(!i8237FF)
						i8237RegR[t]=LOBYTE(i8237DMACurAddr[t/2]);
					else
						i8237RegR[t]=HIBYTE(i8237DMACurAddr[t/2]);
					i8237FF++;
					i8237FF &= 1;
					break;
				case 1:
				case 3:
				case 5:
				case 7:		// reading returns only the current (remaining) value...
					if(!i8237FF)
						i8237RegR[t]=LOBYTE(i8237DMACurLen[t/2]);
					else
						i8237RegR[t]=HIBYTE(i8237DMACurLen[t/2]);
					i8237FF++;
					i8237FF &= 1;
					break;
				case 8:			// Status / Command
//					if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
//						i8237RegR[8] |= DMA_REQUEST1;  // request??...
//						if(!(i8237RegW[15] & (1 << 0)))		// mask...
//							i8237RegR[8] |= 1;		// simulo attività ch0 (refresh... (serve a glabios o dà POST error 400h, E3BA
//						}
					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					break;
				case 0xd:			// Reset , ultimo byte trasferito??
//					i=i8237RegR[13];
					break;
				case 0xe:			//  Clear Masks
					break;
				case 0xf:			//  Masks
					break;
				}
      i=i8237RegR[t];
			switch(t) {
				case 0x8:			// Status
					i8237RegR[8] &= 0xf0;  // clear activity flags
					break;
				}
      break;

    case 2:         // 20-21 PIC 8259 controller
#ifdef PCAT
      t &= 0x3;
#else
      t &= 0x1;
#endif
      switch(t) {
        case 0:
					switch(i8259OCW[2] & 0b00000011) {		// 
						case 2:
							i8259RegR[0]=i8259IRR;
							break;
						case 3:
							i8259RegR[0]=i8259ISR;
							break;
						default:
							break;
						}
		      i=i8259RegR[0];




//					i=i8259ISR;

// dEVE restituire ISR!! o vedi 0xb a 0x21

//					i8259ICWSel=1;
          break;
        case 1:
// no...					i=i8259ICW[i8259ICWSel];
//					i8259ICWSel++;
//					i8259ICWSel %= 4;
/*					if(i8259RegW[0] == 0b00000011))
						i=i8259RegW[1];
					else 

					// se no si blocca... PCXTBIOS */
						i=i8259RegR[1];



					i8259IMR;
          break;

#ifdef PCAT
				case 2:			// DTK 286 scrive 6a a 22 e legge a 23 al boot!
// v. datasheet chipset 82C211 (Neat-286)
					break;
				case 3:
					break;
#endif
        }
      break;

    case 4:         // 40-43 PIT 8253/4 controller (incl. speaker)
		// https://en.wikipedia.org/wiki/Intel_8253
      t &= 0x3;
      switch(t) {
        case 0:
        case 1:
        case 2:
					// la cosa dei Latch non è ancora perfetta credo... non si capisce bene PUTTANALAMADONNA
					switch((i8253Mode[t] & 0b00110000) >> 4) {
						case PIT_LOADLOW:		// 
							i8253RegR[t]=i8253Flags[t] & PIT_LATCHED ? LOBYTE(i8253TimerL[t]) : LOBYTE(i8253TimerR[t]);
		          i8253Flags[t] &= ~PIT_LATCHED;
							break;
						case PIT_LOADHIGH:		// 
							i8253RegR[t]=i8253Flags[t] & PIT_LATCHED ? HIBYTE(i8253TimerL[t]) : HIBYTE(i8253TimerR[t]);
							i8253Flags[t] &= ~PIT_LATCHED;
							break;
						case PIT_LOADBOTH:
						case PIT_LATCH:
							if(i8253Flags[t] & PIT_LOHIBYTE) {
								i8253RegR[t]=i8253Flags[t] & PIT_LATCHED ? HIBYTE(i8253TimerL[t]) : HIBYTE(i8253TimerR[t]);
								i8253Flags[t] &= ~PIT_LATCHED;
								}
							else {
								i8253RegR[t]=i8253Flags[t] & PIT_LATCHED ? LOBYTE(i8253TimerL[t]) : LOBYTE(i8253TimerR[t]);
								}
							if((i8253Mode[t] & 0b00110000) == (PIT_LOADBOTH << 4)) 
								i8253Flags[t] ^= PIT_LOHIBYTE;
							break;
						}
          i=i8253RegR[t];
          break;
        case 3:
#ifdef PCAT
          i=i8253RegR[3];		// questo in effetti vale solo per 8254, non 8253!
					// o forse no... dice 3state...

					// e v. altre opzioni
#endif
          break;
        }
      break;

    case 6:        // 60-6F 8042/8255 keyboard; speaker; config byte    https://www.phatcode.net/res/223/files/html/Chapter_20/CH20-2.html
#ifndef PCAT		// bah, chissà... questo dovrebbe usare 8255 e AT 8042...
      t &= 0x3;
      switch(t) {
        case 0:
					if(KBStatus & KB_OUTPUTFULL)		// SOLO se c'è stato davvero un tasto! (altrimenti siamo dopo reset o clock ENable
						i8255RegR[0]=Keyboard[0];
					else
						i8255RegR[0]=0;
//          KBStatus &= ~KB_OUTPUTFULL;

#ifdef PC_IBM5150
					if(i8255RegW[1] & 0x80)		//https://www.tek-tips.com/threads/port-61h.258828/
						i=MachineSettings;
					else
#endif
	          i=i8255RegR[0];

          break;
        case 1:     // (machine flags... (da V20)
//					i8255RegR[1]=MachineFlags;
          i=i8255RegR[1];
          break;
        case 2:     // machine settings... (da V20)
//          i=0b01101100);			// bits 2,3 memory size (64K bytes); b7:6 #floppies; b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25
//					i8255RegR[2]=MachineSettings;


					// TENERE CONTO di timer2 replicato qua su b5!!

					// 5160-5150  https://www.minuszerodegrees.net/5160/diff/5160_to_5150_8255.htm
#ifdef PC_IBM5150
#define SWITCH1 0x2 /*128KB, 0=64KB opp 640, v.sotto */
					if(i8255RegW[1] & 4)				// 5150
						i8255RegR[2]=SWITCH1 & 0xf;
					else
						i8255RegR[2]=((SWITCH1 >> 4) & 1) | 0xe /* dice forzati a 1 */;
#else		// ossia tutti gli altri!
					if(i8255RegW[1] & 8)				// 5160
						i8255RegR[2]=MachineSettings >> 4;
					else
						i8255RegR[2]=MachineSettings & 0xf;
#endif



					i8255RegR[2] |= 0x0 << 4;		// b4=monitor cassette/speaker, b5=monitor timer ch2, b6=RAM parity err. on exp board, b7=RAM parity err. on MB
					if(!(i8255RegW[1] & 0x10))	{	// https://www.tek-tips.com/threads/port-61h.258828/
						if(0)
							i8255RegR[2] |= 0x80;		// b7=RAM parity err. on MB
						}
					if((i8255RegW[1] & 0x20))	{	//
						if(0)
							i8255RegR[2] |= 0x40;		// b6=RAM parity err. on exp board
						}

//					i8255RegR[2] = 0b00000000);		// test

#ifdef PC_IBM5150
			if(i8253Mode[2] & 0b10000000))
				i8255RegR[2] |= 0b00010000);			// loopback cassette
			else
				i8255RegR[2] &= ~0b00010000);			// 
#endif

          i=i8255RegR[2];   // 


          break;
        }

#else

			// https://wiki.osdev.org/A20_Line
      t &= 0x7;
      switch(t) {
        case 0:
					if(KBStatus & KB_COMMAND)	{		// se avevo ricevuto comando...
						switch(i8042RegW[1]) {
							case 0xAA:      // self test
								i8042RegR[0]=Keyboard[i8042OutPtr-1];
								break;
							case 0xAB:      // diagnostics
								i8042RegR[0]=0b00000000;
								break;
/*		          case 0xAE:      // enable
							case 0xAD:      // disable
								i8042RegR[0]=Keyboard[i8042OutPtr-1];
								break;*/
							case 0x20:
								i8042RegR[0]=KBCommand;
								break;
							case 0xA1:			// read firmware version
								i8042RegR[0]=Keyboard[i8042OutPtr-1];
								break;
							case 0xC0:			// read data
								i8042RegR[0]=Keyboard[i8042OutPtr-1];
								break;
							case 0xC1:			// read data 4 lower bit port of ?? jumper?? into 4..7
								i8042RegR[0]=Keyboard[i8042OutPtr-1];
								break;
							case 0xC2:			// read data 4 upper bit port of ?? jumper?? into 4..7
								i8042RegR[0]=Keyboard[i8042OutPtr-1];
								break;
							case 0xD0:			// read data
								if(KBCommand & 0b00010000)			// se disabilitata
									i8042RegR[0]=i8042Output;
								else
									i8042RegR[0]=i8042Output;				// boh...
								break;
							case 0xE1:			// read test lines
								i8042RegR[0]=0b00000011;				// b1=data, b0=clock
								break;
							case 0xEE:     //echo
								i8042RegR[0]=0xee;
								break;
							case 0xF2:			// read ID
								i8042RegR[0]=0xAB;		i8042RegR[0]=0x83;
								break;
							default:
								if(i8042RegW[1]>=0x00 && i8042RegW[1]<0x20) {   // read ram
									i8042RegR[0]=KBRAM[i8042RegW[1] & 0x1f];
									}
								else if(i8042RegW[1]>=0x20 && i8042RegW[1]<0x40) {   // alias AMI
									i8042RegR[0]=KBRAM[i8042RegW[1] & 0x1f];
									}
								else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
									}
								else {
									if(KBStatus & KB_OUTPUTFULL)		// SOLO se c'è stato davvero un tasto! (altrimenti siamo dopo reset o clock ENable
										i8042RegR[0]=Keyboard[i8042OutPtr-1];
									}
								break;
							}
						}
					else {			// se avevo ricevuto dato
						switch(i8042RegW[1]) {
							}


						i8042RegR[0]=Keyboard[i8042OutPtr-1];
						}


					if(i8042OutPtr) {
						if(!--i8042OutPtr) {
		          KBStatus &= ~KB_OUTPUTFULL;
//							i8042Output &= ~B8(00010000);
							i8042RegW[1]=0;
							}
						}
					else {		// per sicurezza... verificare come mai, nel caso
		        KBStatus &= ~KB_OUTPUTFULL;
//						i8042Output &= ~B8(00010000);
						}




//          i8042RegW[1]=0;     // direi, così la/le prossima lettura restituisce il tasto...



          i=i8042RegR[0];
	//				KBStatus &= ~B8(00001000);		// è dato (non comando)


          break;
        case 1:     // 		system control port for compatibility with 8255
		 /*bit 7	 parity check occurred
		 bit 6	 channel check occurred
		 bit 5	 mirrors timer 2 output condition
		 bit 4	 toggles with each refresh request
		 bit 3	 channel check status
		 bit 2	 parity check status
		 bit 1	 speaker data status
		 bit 0	 timer 2 gate to speaker status*/
//					i8255RegR[1]=MachineFlags;

//					i8042Input ^= 0x20;		// SIMULO speaker out (dal timer non va...

//					i8042Input ^= 0x10;		// SIMULO refresh... messo di là

          i=i8042Input;
          break;
        case 4:
          i=i8042RegR[1]=KBStatus; // 
          KBStatus &= ~KB_INPUTFULL;			// cmq i dati son stati usati :)
//					KBStatus |= B8(00001000);		// è comando
          break;
        }
#endif
      break;

		case 7:        //   70-7F CMOS RAM/RTC (Real Time Clock  MC146818)
			// https://web.archive.org/web/20160314084340/https://www.bioscentral.com/misc/cmosmap.htm
			// anche (TD3300, superXT) Port 070h
			// Controls the wait state for board RAM, ROM, I/O and also bus RAM and I/O. See (page B-1, B-2) from the UX-12 manual.

      t &= 0x1;
      switch(t) {
        case 0:
          i=i146818RegR[0];
          break;
        case 1:     // il bit 7 attiva/disattiva NMI
          switch(i146818RegW[0] & 0x3f) {
            case 0:
              i146818RegR[1]=currentTime.sec;
              break;
              // in mezzo c'è Alarm...
            case 2:
              i146818RegR[1]=currentTime.min;
              break;
            case 4:
              i146818RegR[1]=currentTime.hour;
              break;
            case 6:              // 6 è day of week...
              i146818RegR[1]=currentDate.weekday;
              break;
            case 7:
              i146818RegR[1]=currentDate.mday;
              break;
            case 8:
              i146818RegR[1]=currentDate.mon;
              break;
            case 9:
              i146818RegR[1]=currentDate.year;
              break;
            case 10:
              i146818RegR[1]=(i146818RAM[10] & 0b10000000) | 0b00100110;			// dividers + UIP
              break;
            case 11:
              i146818RegR[1]=0b00000010;			// 12/24: 24; BCD mode
              break;
            case 12:
              i146818RegR[1]=i146818RAM[12] = 0;   // flag IRQ
// no non va così      i146818RAM[10] &= ~0b10000000);		// o glatick si incazza, v. sopra
              break;
            case 13:
              i146818RegR[1]=i146818RAM[13];
							i146818RAM[13] |= 0b10000000;			// VBat ok dopo una lettura  EVIDENTE CAZZATA v. datasheet e valori cmos sotto!
              break;

            case 14:
//              i146818RegR[1]=0b00000000);			// diag in GLATick...
//              break;
#ifdef PCAT
            case 15:			// flag, Award scrive 6		https://www.rcollins.org/ftp/source/pmbasics/tspec_a1.l1
#endif
            default:      // qua ci sono i 4 registri e poi la RAM
              i146818RegR[1]=i146818RAM[i146818RegW[0] & 0x3f];
              break;
            }
          i=i146818RegR[1];
          break;
        }
      break;
    case 0x8:        // 8237 DMA??
      t &= 0xf;
#ifndef PCAT
      switch(t) {
				case 0:
//					i= ;   //
//        LED1 = t1 & 1;
//        LED2 = t1 & 2 ? 1 : 0;
//        LED3 = t1 & 4 ? 1 : 0;
					break;
				case 1:			// 64K page number DMA    https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
					i=DMApage[2] & 0xf;
					break;
				case 2:
					i=DMApage[3] & 0xf;
					break;
				case 3:
					i=DMApage[1] & 0xf;
					break;
				case 7:
					i=DMApage[0] & 0xf;
					break;
				case 9:
					i=DMApage[6] & 0xf;
					break;
				case 0xa:
					i=DMApage[7] & 0xf;
					break;
				case 0xb:
					i=DMApage[5] & 0xf;
					break;
				case 0xf:		// refresh page register...solo AT?
					i=DMArefresh;
					break;
				}
#else
      switch(t) {
				case 0:
//					i= ;   //
//        LED1 = t1 & 1;
//        LED2 = t1 & 2 ? 1 : 0;
//        LED3 = t1 & 4 ? 1 : 0;
					i=DIAGPort;

					break;
				case 1:			// 64K page number DMA    https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
					i=DMApage[2];
					break;
				case 2:
					i=DMApage[3];
					break;
				case 3:
					i=DMApage[1];
					break;
				case 4:
					i=DMApage2[0];		// DTK
					break;
				case 5:
					i=DMApage2[1];		// DTK
					break;
				case 6:
					i=DMApage2[2];		// DTK
					break;
				case 7:
					i=DMApage[0];
					break;
				case 8:
					i=DMApage2[3];		// DTK
					break;
				case 9:
					i=DMApage[6];
					break;
				case 0xa:
					i=DMApage[7];
					break;
				case 0xb:
					i=DMApage[5];
					break;
				case 0xc:
					i=DMApage2[4];		// DTK
					break;
				case 0xd:
					i=DMApage2[5];		// DTK
					break;
				case 0xe:
					i=DMApage2[6];		// DTK
					break;
				case 0xf:		// refresh page register...solo AT?
					i=DMArefresh;
					break;
				}
#endif
      break;

    case 0x9:        // SuperXT TD3300A
			t &= 0xf;
			switch(t) {
				case 0:
			//Controls the clock speed using the following:
			//If port 90h == 1, send 2 (0010b) for Normal -> Turbo
			//If port 90h == 0, send 3 (0011b) for Turbo -> Normal
//			CPUDivider;		// finire! v sopra 61h pure
					break;
				case 2:
			// linea A20 in alcuni computer http://independent-software.com/operating-system-development-enabling-a20-line.html
					break;
				}
			break;

  // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
    case 0xa:        // PIC 8259 controller #2 su AT; porta machine setting su XT (occhio a volte non leggibile)
#ifdef PCAT
      t &= 0x1;
      switch(t) {
        case 0:
					switch(i8259OCW2[2] & 0b00000011) {		// 
						case 2:
							i8259Reg2R[0]=i8259IRR2;
							break;
						case 3:
							i8259Reg2R[0]=i8259ISR2;
							break;
						default:
							break;
						}
		      i=i8259Reg2R[0];
          break;
        case 1:
						i=i8259Reg2R[1];
          break;
        }
#else
i=t;  //DEBUG
#endif
      break;

#ifdef PCAT
		case 0xc:		// DMA 2	(second Direct Memory Access controller 8237)  https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
		case 0xd:		//  Phoenix 286 scrive C0 a D6 all'inizio
      t &= 0x1f;
			t >>=1 ;
			switch(t) {
				case 0:
				case 2:
				case 4:
				case 6:		// reading returns only the current (remaining) value...
					if(!i8237FF2)
						i8237Reg2R[t]=LOBYTE(i8237DMACurAddr2[t/2]);
					else
						i8237Reg2R[t]=HIBYTE(i8237DMACurAddr2[t/2]);
					i8237FF2++;
					i8237FF2 &= 1;
					break;
				case 1:
				case 3:
				case 5:
				case 7:		// reading returns only the current (remaining) value...
					if(!i8237FF2)
						i8237Reg2R[t]=LOBYTE(i8237DMACurLen2[t/2]);
					else
						i8237Reg2R[t]=HIBYTE(i8237DMACurLen2[t/2]);
					i8237FF2++;
					i8237FF2 &= 1;
					break;
				case 8:			// Status / Command
//					if(!(i8237Reg2W[8] & 0b00000001))) {			// controller enable
//						i8237Reg2R[8] |= DMA_REQUEST1;  // request??...
//						if(!(i8237Reg2W[15] & (1 << 0)))		// mask...
//							i8237RegR2[8] |= 1;		// simulo attività ch0 (refresh... (serve a glabios o dà POST error 400h, E3BA
//						}
					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					break;
				case 0xd:			// Reset , ultimo byte trasferito??
//					i=i8237Reg2R[13];
					break;
				case 0xe:			//  Clear Masks
					break;
				case 0xf:			//  Masks
					break;
				}
      i=i8237Reg2R[t];
			switch(t) {
				case 0x8:			// Status
					i8237Reg2R[8] &= 0xf0;  // clear activity flags
					break;
				}
			break;
#endif


    case 0xe:        // TD3300A , SuperXT
			//Controls the bank switching for the on-board upper memory, as mentioned in this post.
			// if port 0Eh == 0, addresses 2000:0000-9000:FFFF are mapped to the low contiguous RAM
			// if port 0Eh == 1, addresses 2000:0000-9000:FFFF are mapped to the upper RAM (if installed)
			break;
    
    case 0xf:        // F0 = coprocessor
      break;
  // 108 diag display opp. 80 !
  // 278-378 printer
  // 300 = POST diag
  // 2f8-3f8 serial

#ifdef PCAT
    case 0x17:      // 170-177   // hard disc 1 su AT		
    case 0x1f:      // 1f0-1f7   // hard disc 0 su AT		http://www.techhelpmanual.com/897-at_hard_disk_ports.html   http://www.techhelpmanual.com/892-i_o_port_map.html
			if(t & 0x80)
				hdDisc &= ~2;
			else
				hdDisc |= 2;
      t &= 0x7;
      switch(t) {
        case 0:			// data MA è a 16BIT!! v. sotto
/*					HDContrRegR[t]=sectorData[HDFIFOPtrR];
					HDFIFOPtrR++;
					if(HDFIFOPtrR >= SECTOR_SIZE)
						HDFIFOPtrR=0;
					hdState &= ~HD_AT_STATUS_BUSY;		// tolgo busy :)
					*/
          break;
        case 1:			// error
					HDContrRegR[t]=hdError;
//					hdError=0;
          break;
        case 2:			// sector count
        case 3:			// sector number
        case 4:			// cyl high
        case 5:			// cyl low
					// se ATAPI, 4 = 0x14 e 5 = 0xeb; se SATA 0x3c e 0xc3
        case 6:			// drive & head
					HDContrRegR[t]=HDContrRegW[t];
          break;
        case 7:			// status
					HDContrRegR[t]=hdState;
					if((HDContrRegW[7] & 0xf0) == 0x20 || (HDContrRegW[7] & 0xf0) == 0x30)
						;		//  (per read e write andrebbe fatto a fine operazione, ma è complicato
					else
						hdState &= ~HD_AT_STATUS_BUSY;		// tolgo busy :)
// cagata, chissà come dovrebbe essere					i8259IRR2 &= hdDisc >=2 ? ~0b10000000) : ~0b01000000);		// tolgo IRQ... v.3f6
					// e OCCHIO cmq di là pulisco busy su IRQ, è più semplice!

          break;
/*        case 0x0F: // 3F7  // da granite
            u8 result = 0;
            result |= (drive_head&0x10)?1:2;
            result |= (drive_head&0x0F)<<2;
            data = result;
            break;				
						*/
				}
			i=HDContrRegR[t];
      break;
#endif

    case 0x20:    //200-207   joystick
      t &= 1;
      switch(t) {
        case 1:
          i=0xff;     // no joystick...
          i=0xf0;     // sì joystick :)
          break;
        }
      break;
      
    case 0x22:    //220-223   Sound Blaster; -22F se Pro
      break;
      
    case 0x24:    // RTC SECONDO PCXTBIOS, il clock è a 240 o 2c0 o 340 ed è mappato con tutti i registri... pare!
      t &= 0xf;
      switch(t) {
        case 1:
          i=currentTime.sec;
          break;
        case 2:
          i=currentTime.min;
          break;
        case 3:
          i=currentTime.hour;
          break;
        case 4:
          i=currentDate.mday;
          break;
        case 5:
          i=currentDate.mon;
          break;
        case 6:
          i=currentDate.year;
          break;
        case 7:
          break;
        default:      
          break;
        }
      break;
      
#ifdef ROM_HD_BASE
		case 0x32:    // 320- (hard disk su XT	http://www.techhelpmanual.com/898-xt_hard_disk_ports.html  https://forum.vcfed.org/index.php?threads/ibm-xt-i-o-addresses.1237247/		
      t &= 3;
      switch(t) {
				BYTE disk;
				int n,addr;
        case 0:			// READ DATA
					/*7 6 5		4		3		2		1		0
						d d IRQ DRQ BSY C/D I/O REQ*/
					//NB è meglio effettuare le operazioni qua, alla rilettura, che non alla scrittura dell'ultimo parm (v anche FD)
					// perché pare che tipo il DMA nei HD viene attivato DOPO la conferma della rilettura


					switch(HDContrRegW[0] & 0b11100000) {		// i 3 bit alti sono class, gli altri il comando
						case 0b00000000:
							disk=HDFIFOW[1] & 0b00100000 ? 1 : 0;
							switch(HDContrRegW[0] & HD_XT_COMMAND_MASK) {		// i 3 bit alti sono class, gli altri il comando
								case HD_XT_TEST_READY:			// test ready
									if(!disk)
										HDFIFOR[0]=0b00000000;
									else
										HDFIFOR[0]=0b00100000;		// errore su disco 2 ;) se ne sbatte

endHDcommandWithIRQ:
									if(hdControl & 0b00000010)
										setHDIRQ(DEFAULT_HD_DELAYED_IRQ);
									hdState |= HD_XT_STATUS_IRQ;		// IRQ pending
endHDcommand:
									hdState &= ~HD_XT_STATUS_DRQ;		// passo a read
									hdState &= ~HD_XT_STATUS_BUSY;			// BSY
									HDFIFOPtrW=HDFIFOPtrR=0;
									break;
								case 0x01:			// recalibrate

									if(!disk)
										HDFIFOR[0]=0b00000000;
									else
										HDFIFOR[0]=0b00100010;		// errore su disco 2 ;) QUA VA! solo che poi resta a provarci 1000 volte...

									// ritardo...
									hdCylinder=0;

									if(HDFIFOR[0]) {
										HDFIFOR[1]=4; HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4]=0;		// ci sarebbero cilindro ecc dell'errore
										}
									else {
										HDFIFOR[1]=HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4]=0;
										}

//							hdState |= HD_XT_STATUS_IRQ;		// IRQ pending
//										if(hdControl & 0b00000010))		// sparisce C anche se il disco rimane visibile... strano...
//											i8259IRR |= 0b00100000);		// simulo IRQ...
									goto endHDcommandWithIRQ;
									break;
								case 0x02:			// reserved
									break;
								case HD_XT_READ_STATUS:			// request sense

									if(HDFIFOR[0]) {		// se errore ultima op...
//											HDFIFOR[1]=HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4];		// ci sarebbero cilindro ecc dell'errore
										}
									else {
//											HDFIFOR[1]=HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4];
										}


									goto endHDcommand;
									break;

								case HD_XT_FORMAT_DRIVE:			// format drive

									goto endHDcommand;

									break;
								case HD_XT_VERIFY:			// read verify

									hdState |= HD_XT_STATUS_DRQ;			// attivo DMA (?? su hdControl<1>
									if(!disk) {
										hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
										hdHead=HDFIFOW[1] & 0b00011111;
										hdSector=HDFIFOW[2] & 0b00011111;
										HDFIFOW[4];	HDFIFOW[5];		// 
										addr=i8237DMACurAddr[3]=i8237DMAAddr[3];
										i8237DMACurLen[3]=i8237DMALen[3];
										n=i8237DMALen[3]   +1;		// 1ff ...
										}

									if(!disk)		// 
										HDFIFOR[0]=0b00000000;
									else
										HDFIFOR[0]=0b00100010;		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~HD_XT_STATUS_DDR;		// passo a read (lungh.diversa qua
									hdState &= ~HD_XT_STATUS_DRQ;			// disattivo DMA 

									goto endHDcommandWithIRQ;
									break;

								case HD_XT_FORMAT_TRACK:			// format track
							hdState |= HD_XT_STATUS_DRQ;			// attivo DMA (?? su hdControl<1>

									hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
									hdHead=HDFIFOW[1] & 0b00011111;

									if(!disk)
										HDFIFOR[0]=0b00000000;
									else
										HDFIFOR[0]=0b00100010;		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~HD_XT_STATUS_DRQ;			// disattivo DMA 

									goto endHDcommandWithIRQ;
									break;

								case HD_XT_FORMAT_BAD_TRACK:			// format bad track

									goto endHDcommand;
									break;

								case HD_XT_READ_DATA:			// read

									hdState |= HD_XT_STATUS_BUSY;			// BSY

									if(!disk) {

										hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
										hdHead=HDFIFOW[1] & 0b00011111;
										hdSector=HDFIFOW[2] & 0b00011111;
										HDFIFOW[4];	HDFIFOW[5];		// 
										addr=i8237DMACurAddr[3]=i8237DMAAddr[3];
										i8237DMACurLen[3]=i8237DMALen[3];
										n=i8237DMALen[3]   +1;		// 1ff ...
										do {
											encodeHDiscSector(disk);

											if(hdControl & 1) {		// scegliere se DMA o PIO in base a flag HDcontrol qua...
												if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
													i8237RegR[8] |= DMA_REQUEST3;  // request??...
													if(!(i8237RegW[15] & DMA_MASK3))	{	// mask...
                      			switch((i8237Mode[3] & 0b00001100) >> 2) {		// DMA mode
															case DMA_MODEVERIFY:			// verify
																memcmp(&ram_seg[MAKELONG(addr,DMApage[3])],sectorData,SECTOR_SIZE);
																break;
															case DMA_MODEWRITE:			// write
																memcpy(&ram_seg[MAKELONG(addr,DMApage[3])],sectorData,SECTOR_SIZE);
																break;
															case DMA_MODEREAD:			// read
																memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[3])],SECTOR_SIZE);
																break;
															}
														}
													i8237Mode[3] & DMA_TYPE == DMA_READ;		// Read
													i8237Mode[3] & DMA_MODE == DMA_SINGLE;		// Single mode
													i8237RegR[8] |= DMA_TC3;  // fatto, TC 3
													i8237RegR[8] &= ~DMA_REQUEST3;  // request finita
													i8237RegR[13]=ram_seg[SECTOR_SIZE-1]; // ultimo byte trasferito :)
													}
												}
											else {
												}
											hdSector++;
											if(hdSector>=HD_SECTORS_PER_CYLINDER) {		// 
												hdSector=0;
												hdHead++;
												if(hdHead>=HD_HEADS) {
													hdHead=0;
													hdCylinder++;
													}
												}
											addr+=SECTOR_SIZE;		// verificare block count
											// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
											//i8237DMALen[2]=0;		// remaining. idem?
											n -= SECTOR_SIZE;
											} while(n>0);

										i8237DMACurAddr[3]+=i8237DMACurLen[3]+1;
										i8237DMACurLen[3]=0;
										HDFIFOR[0] = 0b10000000;		//ST0: B7=address valid, B5:4=error type, B3:0=error code
										HDFIFOR[1] = (disk ? 0b00100000 : 0b00000000) | (hdHead & 31);
										HDFIFOR[2] = hdCylinder & 0xff; //		//C
										HDFIFOR[3] = (hdCylinder >> 8) | hdSector;
										}
									else
										HDFIFOR[0]=0b00100000;		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~HD_XT_STATUS_DRQ;			// disattivo DMA 
//											goto endHDcommand;
									goto endHDcommandWithIRQ;
									break;

								case 0x09:			// reserved
									break;

								case HD_XT_WRITE_DATA:			// write

									hdState |= HD_XT_STATUS_BUSY;			// BSY

									if(!disk) {
										hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
										hdHead=HDFIFOW[1] & 0b00011111;
										hdSector=HDFIFOW[2] & 0b00011111;
										HDFIFOW[4];	HDFIFOW[5];		// 
										addr=i8237DMACurAddr[3]=i8237DMAAddr[3];
										i8237DMACurLen[3]=i8237DMALen[3];
										n=i8237DMALen[3]   +1;		// 1ff ...
										do {
											msdosHDisk=((uint8_t*)MSDOS_HDISK)+getHDiscSector(disk,0)*SECTOR_SIZE;

											if(hdControl & 1) {		// scegliere se DMA o PIO in base a flag HDcontrol qua...
												if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
													i8237RegR[8] |= DMA_REQUEST3;  // request??...
													if(!(i8237RegW[15] & DMA_MASK3))	{	// mask...
                      			switch((i8237Mode[3] & 0b00001100) >> 2) {		// DMA mode
															case DMA_MODEVERIFY:			// verify
																memcmp(&ram_seg[MAKELONG(addr,DMApage[3])],msdosHDisk,SECTOR_SIZE);
																break;
															case DMA_MODEWRITE:			// write
																memcpy(&ram_seg[MAKELONG(addr,DMApage[3])],msdosHDisk,SECTOR_SIZE);
																break;
															case DMA_MODEREAD:			// read
																memcpy(msdosHDisk,&ram_seg[MAKELONG(addr,DMApage[3])],SECTOR_SIZE);
																break;
															}
														}
													i8237Mode[3] & DMA_TYPE == DMA_READ;		// Read
													i8237Mode[3] & DMA_MODE == DMA_SINGLE;		// Single mode
													i8237RegR[8] |= DMA_TC3;  // fatto, TC 3
													i8237RegR[8] &= ~DMA_REQUEST3;  // request finita
													i8237RegR[13]=ram_seg[SECTOR_SIZE-1]; // ultimo byte trasferito :)
													}
												}
											else {
												}

											hdSector++;
											if(hdSector>=HD_SECTORS_PER_CYLINDER) {		// ma NON dovrebbe accadere!
												hdSector=0;
												hdHead++;
												if(hdHead>=HD_HEADS) {
													hdHead=0;
													hdCylinder++;
													}
												}
											addr+=SECTOR_SIZE;		// verificare block count
											// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
											//i8237DMALen[2]=0;		// remaining. idem?
											n -= SECTOR_SIZE;
											} while(n>0);

										i8237DMACurAddr[3]+=i8237DMACurLen[3]+1;
										i8237DMACurLen[3]=0;
										HDFIFOR[0] = 0b00000000;		//ST0: B7=address valid, B5:4=error type, B3:0=error code
										HDFIFOR[1] = (disk ? 0b00100000 : 0b00000000) | (hdHead & 31);
										HDFIFOR[2] = hdCylinder & 0xff; //		//C
										HDFIFOR[3] = (hdCylinder >> 8) | hdSector;
										}
									else
										HDFIFOR[0]=0b00100000;		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~HD_XT_STATUS_DRQ;			// disattivo DMA 
									goto endHDcommandWithIRQ;
									break;
								case HD_XT_SEEK:			// seek

									hdState |= HD_XT_STATUS_BUSY;			// BSY

									if(!disk)
										HDFIFOR[0]=0b00000000;
									else
										HDFIFOR[0]=0b00100000;		// errore su disco 2 ;) MA NON SPARISCE

									hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
									hdHead=HDFIFOW[1] & 0b00011111;
									// ritardo...

									goto endHDcommandWithIRQ;
									break;
								case HD_XT_INITIALIZE:			// initialize
									if(HDFIFOPtrW==14) {

										if(!disk)
											HDFIFOR[0]=0b00000000;
										else
											HDFIFOR[0]=0b00100000   /*00100010 muore tutto*/;		// errore su disco 2 ;) MA NON SPARISCE

										// restituire 4 in Error code poi (però tanto non lo chiede, non arriva SENSE...
//										HDFIFO[1]=4;

										hdState &= ~HD_XT_STATUS_DDR;		// passo a read (lungh.diversa qua
										goto endHDcommandWithIRQ;
										}
									hdState &= ~0b00001111;		// 
									break;
								case HD_XT_READ_ECC_BURST_ERROR_LENGTH:			// read ECC burst
									goto endHDcommand;
									break;
								case HD_XT_READ_SECTOR_BUFFER:			// read data from sector buffer

										HDFIFOR[0]=0;
									goto endHDcommandWithIRQ;
									break;
								case HD_XT_WRITE_SECTOR_BUFFER:			// write data to sector buffer

										HDFIFOR[0]=0;
									goto endHDcommandWithIRQ;
									break;
								default:		// invalid, 
									break;
								}
							break;
						case 0b00100000:
							break;
						case 0b01000000:
							break;
						case 0b01100000:
							break;
						case 0b10000000:
							break;
						case 0b10100000:
							break;
						case 0b11000000:
							break;
						case 0b11100000:
							disk=HDFIFOW[1] & 0b00100000 ? 1 : 0;
							switch(HDContrRegW[0] & HD_XT_COMMAND_MASK) {		// i 3 bit alti sono class, gli altri il comando
								case 0x00:			// RAM diagnostic
//											hdState=0b00001101);			// mah; e basare DMA su hdControl<1>


									HDFIFOR[0]=0b00000000;									// ok (vuole questo, 20h pare ok e 30h sarebbe ram error
									goto endHDcommandWithIRQ;
									break;
								case 0x01:			// reserved
									break;
								case 0x02:			// reserved
									break;
								case 0x03:			// drive diagnostic

									HDFIFOR[0]=0b00000000;									// ok
									goto endHDcommandWithIRQ;
									break;
								case 0x04:			// controller internal diagnostic

									HDFIFOR[0]=0b00000000;									// ok(vuole questo, 20h pare ok e 30h sarebbe ram error
									goto endHDcommandWithIRQ;
									break;
								case 0x05:			// read long

									goto endHDcommand;
									break;
								case 0x07:			// write long

									goto endHDcommand;
									break;
								default:		// invalid, 
									break;
								}
							break;
						}

					HDContrRegR[0]=HDFIFOR[HDFIFOPtrR++];				// FINIRE
          break;
        case 1:     // READ HARDWARE STATUS
					HDContrRegR[1]=hdState;
					if(!(hdState & HD_XT_STATUS_XFER))
						hdState |= HD_XT_STATUS_XFER;			// handshake

					hdState |= HD_XT_STATUS_IRQ;		// IRQ pending
          break;
        case 2:			// READ DRIVE CONFIGURATION INFO
					HDContrRegR[2]=0b00001111;			// 10MB entrambi (00=5, 01=24, 10=15, 11=10
					hdState &= ~(HD_XT_STATUS_IRQ | HD_XT_STATUS_DDR);			// input, no IRQ
          break;
        case 3:			// not used
          break;
				}
			i=HDContrRegR[t];
			break;
#endif

    case 0x37:    // 370-377 (secondary floppy, AT http://www.techhelpmanual.com/896-diskette_controller_ports.html); 378-37A  LPT1
      t &= 3;
			i=LPT[0][t];		// per ora, al volo!
      switch(t) {
        case 0:
//          i=(PORTE & 0xff) & ~0b00010000);
//          i |= PORTGbits.RG6 ? 0b00010000) : 0;   // perché LATE4 è led...
          break;
        case 1:   // finire
//          i=PORTB & 0xff;
          break;
        case 2:    // qua c'è lo strobe
//          i = PORTGbits.RG7;      // finire
          break;
        }
      break;

#ifdef MDA
    case 0x3b:    // 3b0-3bf  MDA https://www.seasip.info/VintagePC/mda.html
      t &= 0xf;
      switch(t) {
        case 0:     // 0, 2, 4, 6: register select
        case 2:
        case 4:
        case 6:
         i=MDAreg[4];
          break;
        case 1:     // 
        case 3:
        case 5:
        case 7:
          MDAreg[5]=i6845RegR[MDAreg[4] % 18];
          break;
        case 0x8:
          // Mode
          break;
        case 0xa:
          // Status
          break;
        case 0xc:
					MDAreg[0xc]=LPT[2][0];		// per ora, al volo!
          break;
				case 0xe:
					MDAreg[0xe]=LPT[2][2];
          break;
        default:
          break;
        }
      i=MDAreg[t];
      break;
#endif

#ifdef VGA				// http://www.osdever.net/FreeVGA/vga/portidx.htm
    case 0x3b:
      t &= 0xf;
      switch(t) {
				case 0xa:
					// b3=Vretrace, b0=display disabled ovv.Hretrace, se in mono
					i=CGAreg[0xa];		// riciclo questo! v. di là
					break;
				}
			break;
    case 0x3c:    // 3c0-3cf  VGA http://wiki.osdev.org/VGA_Hardware#Port_0x3C0
      t &= 0xf;
      switch(t) {
        case 0:     // index/data, write only
					break;
        case 1:			// read register actl
          i=VGAactlReg[VGAptr & 0x1f];		// 
					//VGAPalette[];
          break;
				case 0x2:
//          i=VGAstatus0 ;VGAreg[2];		// read input status...
					i=VGAreg[2];
					break;
        case 0x4+1:
          i=VGAseqReg[VGAreg[0x4] & 0x7];		// 
          break;
        case 0x6:
          i=VGAreg[t];		// DAC register
					VGADAC;
          break;
				case 0xc:
          i=VGAreg[2];		// read misc output...
					break;
        case 0x7:
          i=VGAreg[t];		// read DAC register (indexed da 3c8
          break;
        case 0x8:
          i=VGAreg[t];		// read DAC register index
          break;
        case 0xe:
          i=VGAreg[t];		// 
          break;
        case 0xf:			// read register graph
          i=VGAgraphReg[VGAreg[0xe] & 0xf];		// 
          break;
        default:
          break;
        }
//      i=VGAreg[t];
      break;
    case 0x3d:    // 
      t &= 0xf;
      switch(t) {
        case 0xa:
					VGAFF=0;			// reset FF
					// b3=Vretrace, b0=display disabled ovv.Hretrace, se in mono
					i=CGAreg[0xa];		// riciclo questo! v. di là
          break;
        case 0x4+1:
          i=VGAreg[t];		// 
          break;
				}
      break;
    case 0x92e:    // 
      t &= 0xf;
      switch(t) {
				case 0x8:
					// Trident si aspetta 5555,AAAA a 16bit 
					i=LOBYTE(VGAsignature);		// 
					break;
				case 0x9:
					// (segue
					i=HIBYTE(VGAsignature);		// 
					break;
				}
			break;
#endif

#ifdef CGA
    case 0x3d:    // 3d0-3df  CGA (forse son solo 8 ma non si capisce, 3d8 3df   http://nerdlypleasures.blogspot.com/2016/05/ibms-cga-hardware-explained.html
      t &= 0xf;
      switch(t) {
        case 4:     // 3dc, 3dd sono reg. 6845..
        case 0xc:
          break;
        case 5:     // 
        case 0xd:
          CGAreg[5]=i6845RegR[CGAreg[4] % 18];
          break;
  // v. file salvato: da qualche parte dice che dovrebbero essercene 16... http://www.techhelpmanual.com/901-color_graphics_adapter_i_o_ports.html
        case 0xa:
// v. in updatescreen e dove la chiamiamo					CGAreg[0xa]=(CGAreg[8] & 8 ? 1 : 0) | (CGAreg[8] & 8 ? 8 : 0)
          break;
        default:
          break;
        }
      i=CGAreg[t];
      break;
#endif

    case 0x3f:    // 3f2-3f5: floppy disk controller (http://www.techhelpmanual.com/896-diskette_controller_ports.html); 3f8-3ff  COM1
			// v. anche https://www.cpcwiki.eu/forum/hardware-related/interpreting-upd765-fdc-result-bytes/
//							  https://wiki.osdev.org/Floppy_Disk_Controller#Accessing_Floppies_in_Real_Mode
      t &= 0xf;
      switch(t) {
				int n,addr;

        case 2:   // Write: digital output register (DOR)   MOTD MOTC MOTB MOTA DMA ENABLE SEL1 SEL0
					// questo registro in effetti non appartiene al uPD765 ma è esterno...
          break;
        case 4:   // Read-only: main status register (MSR)  REQ DIR DMA BUSY D C B A
//					FloppyContrRegR[0] |= FLOPPY_STATUS_REQ;		// DIO ready for data exchange
					// nella prima word FIFO i 3 bit più bassi sono in genere HD US1 US0 ossia Head e Floppy in uso (0..3)
					//NB è meglio effettuare le operazioni qua, alla rilettura, che non alla scrittura dell'ultimo parm (v anche HD)
					// perché pare che tipo il DMA nei HD viene attivato DOPO la conferma della rilettura (qua pare di no ma ok)
					switch(FloppyContrRegW[1] & FLOPPY_COMMAND_MASK) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
						case FLOPPY_READ_DATA:			// read data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??

								floppyTrack[fdDisk]=FloppyFIFO[2];
								floppyHead[fdDisk]=FloppyFIFO[3];
								floppySector[fdDisk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];

								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
	//								getDiscSector();
									encodeDiscSector(fdDisk);


				/*							 {
			char myBuf[256];
			wsprintf(myBuf,"T%u H%u S%u , %06x: %04x, %08x \n",floppyTrack,floppyHead,floppySector,MAKELONG(addr,DMApage[2]),n,getDiscSector()*SECTOR_SIZE);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			wsprintf(myBuf,"  %02X %02X %02X %02X  %c%c%c%c\n",sectorData[0],sectorData[1],sectorData[2],sectorData[3],
				isprint(sectorData[0]) ? sectorData[0] : '.',
				isprint(sectorData[1]) ? sectorData[1] : '.',
				isprint(sectorData[2]) ? sectorData[2] : '.',
				isprint(sectorData[3]) ? sectorData[3] : '.'
				);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}*/


									// SIMULO DMA qua...
									if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
										i8237RegR[8] |= DMA_REQUEST2;  // request??...
										if(!(i8237RegW[15] & DMA_MASK2))	{	// mask...
											switch((i8237Mode[2] & 0b00001100) >> 2) {		// DMA mode
												case DMA_MODEVERIFY:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEWRITE:			// write
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEREAD:			// read
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & DMA_TYPE == DMA_READ;		// Read
										i8237Mode[2] & DMA_MODE == DMA_SINGLE;		// Single mode
										i8237RegR[8] |= DMA_TC2;  // fatto, TC 2
										i8237RegR[8] &= ~DMA_REQUEST2;  // request finita
										i8237RegR[13]=ram_seg[MAKELONG(addr,DMApage[2])+(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			char myBuf[256],mybuf2[32];
			wsprintf(myBuf,"*** %s  D%u  T%u H%u S%u , %06x: %04x, %08x \n",_strtime(mybuf2),
				fdDisk,floppyTrack[fdDisk],floppyHead[fdDisk],floppySector[fdDisk],MAKELONG(addr,DMApage[2]),n,getDiscSector(fdDisk)*SECTOR_SIZE);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif

									floppySector[fdDisk]++;
									if(floppySector[fdDisk]>sectorsPerTrack[fdDisk]) {		// ma NON dovrebbe accadere!
										floppySector[fdDisk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

											floppyHead[fdDisk]++;

											if(floppyHead[fdDisk]>=2) {
												floppyHead[fdDisk]=0;
												floppyTrack[fdDisk]++;
												}
											}
										}
									addr+=0x80 << FloppyFIFO[5];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									//i8237DMALen[2]=0;		// remaining. idem?
									n -= 0x80 << FloppyFIFO[5];
									} while(n>0);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[fdDisk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[fdDisk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[fdDisk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
//								FloppyFIFO[2] = 0 | (floppySector[fdDisk]==sectorsPerTrack[fdDisk] ? B8(10000000) : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								//Bit 7 (value = 0x80) is set if the floppy had too few sectors on it to complete a read/write.
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								if(!(floppyState[fdDisk] & FLOPPY_STATE_DISKPRESENT))		// forzo errore se disco non c'è...
									FloppyFIFO[1] |= FLOPPY_ERROR_INVALID | FLOPPY_ERROR_ABNORMAL;
								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								}

							else if(FloppyFIFOPtrR) {		// NO! glabios lo fa, PCXTbios no: pensavo ci passasse più volte dopo la fine preparazione, ma non è così quindi si può togliere if
								//FloppyContrRegR[0]=FloppyFIFO[FloppyFIFOPtrR++];togliere
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY)  | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;
						
						case FLOPPY_READ_TRACK:			// read track
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??

								floppyTrack[fdDisk]=FloppyFIFO[2];
								floppyHead[fdDisk]=FloppyFIFO[3];
								floppySector[fdDisk]=1 /*FloppyFIFO[4]*/;
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// qui ci dev'essere abbastanza spazio per tutti i settori di una traccia, e lo uso quindi anche come contatore come al solito!
								do {
									encodeDiscSector(fdDisk);

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
										i8237RegR[8] |= DMA_REQUEST2;  // request??...
										if(!(i8237RegW[15] & DMA_MASK2))	{	// mask...
											switch((i8237Mode[2] & 0b00001100) >> 2) {		// DMA mode
												case DMA_MODEVERIFY:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEWRITE:			// write
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEREAD:			// read
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & DMA_TYPE == DMA_READ;		// read
										i8237Mode[2] & DMA_MODE == DMA_SINGLE;		// Single mode
										i8237RegR[8] |= DMA_TC2;  // fatto, TC 2
										i8237RegR[8] &= ~DMA_REQUEST2;  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[fdDisk]++;
									if(floppySector[fdDisk]>sectorsPerTrack[fdDisk]) {		// 
										break;
										}
									addr+=0x80 << FloppyFIFO[5];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									//i8237DMALen[2]=0;		// remaining. idem?
									n -= 0x80 << FloppyFIFO[5];
									} while(n>0);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[fdDisk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[fdDisk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[fdDisk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY) | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;
						
						case FLOPPY_READ_DELETED_DATA:			// read deleted
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??

								floppyTrack[fdDisk]=FloppyFIFO[2];
								floppyHead[fdDisk]=FloppyFIFO[3];
								floppySector[fdDisk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
									encodeDiscSector(fdDisk);

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
										i8237RegR[8] |= DMA_REQUEST2;  // request??...
										if(!(i8237RegW[15] & DMA_MASK2))	{	// mask...
											switch((i8237Mode[2] & 0b00001100) >> 2) {		// DMA mode
												case DMA_MODEVERIFY:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEWRITE:			// write
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEREAD:			// read
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & DMA_TYPE == DMA_READ;		// read
										i8237Mode[2] & DMA_MODE == DMA_SINGLE;		// Single mode
										i8237RegR[8] |= DMA_TC2;  // fatto, TC 2
										i8237RegR[8] &= ~DMA_REQUEST2;  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[fdDisk]++;
									if(floppySector[fdDisk]>sectorsPerTrack[fdDisk]) {		// ma NON dovrebbe accadere!
										floppySector[fdDisk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

											floppyHead[fdDisk]++;

											if(floppyHead[fdDisk]>=2) {
												floppyHead[fdDisk]=0;
												floppyTrack[fdDisk]++;
												}
											}
										}
									addr+=0x80 << FloppyFIFO[5];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									//i8237DMALen[2]=0;		// remaining. idem?
									n -= 0x80 << FloppyFIFO[5];
									} while(n>0);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[fdDisk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[fdDisk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[fdDisk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY) | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;
						
						case FLOPPY_READ_ID:			// read ID
							if(FloppyFIFOPtrW==2 && !FloppyFIFOPtrR) {
								FloppyFIFO[6] = FloppyFIFO[5];
								FloppyFIFO[5] = FloppyFIFO[4];
								FloppyFIFO[4] = FloppyFIFO[3];
								FloppyFIFO[3] = FloppyFIFO[2];
								FloppyFIFO[0] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[1] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[2] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyContrRegR[0]=FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY;
								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY) | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;
						
						case FLOPPY_WRITE_DATA:			// write data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??

								floppyTrack[fdDisk]=FloppyFIFO[2];
								floppyHead[fdDisk]=FloppyFIFO[3];
								floppySector[fdDisk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[fdDisk])+getDiscSector(fdDisk)*SECTOR_SIZE;

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
										i8237RegR[8] |= DMA_REQUEST2;  // request??...
										if(!(i8237RegW[15] & DMA_MASK2))	{	// mask...
											switch((i8237Mode[2] & 0b00001100) >> 2) {		// DMA mode
												case DMA_MODEVERIFY:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEWRITE:			// write
													memcpy(msdosDisk,sectorData,0x80 << FloppyFIFO[5]);// verificare, non usato cmq :)
													break;
												case DMA_MODEREAD:			// read
													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & DMA_TYPE == DMA_READ;		// Write
										i8237Mode[2] & DMA_MODE == DMA_SINGLE;		// Single mode
										i8237RegR[8] |= DMA_TC2;  // fatto, TC 2
										i8237RegR[8] &= ~DMA_REQUEST2;  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[fdDisk]++;
									if(floppySector[fdDisk]>sectorsPerTrack[fdDisk]) {		// ma NON dovrebbe accadere!
										floppySector[fdDisk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

											floppyHead[fdDisk]++;

											if(floppyHead[fdDisk]>=2) {
												floppyHead[fdDisk]=0;
												floppyTrack[fdDisk]++;
												}
											}
										}
									addr+=0x80 << FloppyFIFO[5];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									//i8237DMALen[2]=0;		// remaining. idem?
									n -= 0x80 << FloppyFIFO[5];
									} while(n>0);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[fdDisk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[fdDisk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[fdDisk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR);
								floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read
								floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY) | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;

						case FLOPPY_FORMAT_TRACK:			// format track
							if(FloppyFIFOPtrW==6) {

								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??

								FloppyFIFO[2];		// bytes per sector	ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								FloppyFIFO[3];		// sector per track
								FloppyFIFO[4];		// gap3
								FloppyFIFO[5];		// filler byte
								floppySector[fdDisk]=1;
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[fdDisk])+getDiscSector(fdDisk)*SECTOR_SIZE;

									// SIMULO DMA qua... o forse no?
									if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
										i8237RegR[8] |= DMA_REQUEST2;  // request??...
										if(!(i8237RegW[15] & DMA_MASK2))	{	// mask...
											switch((i8237Mode[2] & 0b00001100) >> 2) {		// DMA mode
												case DMA_MODEVERIFY:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[2]);
													break;
												case DMA_MODEWRITE:			// write
// bah non è chiaro, ma SET funziona!													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[2]);
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[2]);
													break;
												case DMA_MODEREAD:			// read
													memset(msdosDisk,FloppyFIFO[5],0x80 << FloppyFIFO[2]);
													break;
												}
											}
										i8237Mode[2] & DMA_TYPE == DMA_READ;		// Write
										i8237Mode[2] & DMA_MODE == DMA_SINGLE;		// Single mode
										i8237RegR[8] |= DMA_TC2;  // fatto, TC 2
										i8237RegR[8] &= ~DMA_REQUEST2;  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[2])-1]; // ultimo byte trasferito :)
										}

									floppySector[fdDisk]++;
									addr+=0x80 << FloppyFIFO[2];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									} while(floppySector[fdDisk]<=FloppyFIFO[3]);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[fdDisk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[fdDisk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[fdDisk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY) | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;

						case FLOPPY_WRITE_DELETED_DATA:			// write deleted data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??

								floppyTrack[fdDisk]=FloppyFIFO[2];
								floppyHead[fdDisk]=FloppyFIFO[3];
								floppySector[fdDisk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[fdDisk])+getDiscSector(fdDisk)*SECTOR_SIZE;

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & DMA_DISABLED)) {			// controller enable
										i8237RegR[8] |= DMA_REQUEST2;  // request??...
										if(!(i8237RegW[15] & DMA_MASK2))	{	// mask...
											switch((i8237Mode[2] & 0b00001100) >> 2) {		// DMA mode
												case DMA_MODEVERIFY:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEWRITE:			// write
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case DMA_MODEREAD:			// read
													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & DMA_TYPE == DMA_READ;		// Write
										i8237Mode[2] & DMA_MODE == DMA_SINGLE;		// Single mode
										i8237RegR[8] |= DMA_TC2;  // fatto, TC 2
										i8237RegR[8] &= ~DMA_REQUEST2;  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[fdDisk]++;
									if(floppySector[fdDisk]>sectorsPerTrack[fdDisk]) {		// ma NON dovrebbe accadere!
										floppySector[fdDisk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

											floppyHead[fdDisk]++;

											if(floppyHead[fdDisk]>=2) {
												floppyHead[fdDisk]=0;
												floppyTrack[fdDisk]++;
												}
											}
										}
									addr+=0x80 << FloppyFIFO[5];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									//i8237DMALen[2]=0;		// remaining. idem?
									n -= 0x80 << FloppyFIFO[5];
									} while(n>0);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[fdDisk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[fdDisk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[fdDisk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY) | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;

						case FLOPPY_SCAN_EQUAL:			// scan equal
						case FLOPPY_SCAN_LOW_OR_EQUAL:			// scan low or equal
						case FLOPPY_SCAN_HIGH_OR_EQUAL:			// scan high or equal
							// finire...
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??

								floppyTrack[fdDisk]=FloppyFIFO[2];
								floppyHead[fdDisk]=FloppyFIFO[3];
								floppySector[fdDisk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[fdDisk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[fdDisk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[fdDisk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=1;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? ((FLOPPY_STATUS_REQ | FLOPPY_STATUS_BUSY) | (1 << fdDisk)) : FLOPPY_STATUS_REQ;		// busy
								}
							break;

						case FLOPPY_RECALIBRATE:			// recalibrate
						case FLOPPY_SPECIFY:			// specify
						case FLOPPY_SEEK:			// seek (goto cylinder
							FloppyContrRegR[0]=FLOPPY_STATUS_BUSY | (1 << fdDisk);		// busy, lo vuole qua
							if(floppyState[fdDisk] & FLOPPY_STATE_XFER) {
								FloppyContrRegR[0] |= FLOPPY_STATUS_REQ;		// ok to xfer
//								FloppyFIFOPtrW=0; 
								if((((FloppyContrRegW[1] & FLOPPY_COMMAND_MASK /*vuole extra parentesi PD*/) == FLOPPY_RECALIBRATE) && FloppyFIFOPtrW==2)
									|| (FloppyFIFOPtrW==3)) {
// ma a volte non fa una extra lettura alla fine straporcamadonna morte ai programmatori


//												_lwrite(spoolFile,"FINITO\r\n",8);

									FloppyFIFOPtrW=0;
									floppyState[fdDisk] &= ~(FLOPPY_STATE_XFER);	// xfer ora basta
									}
								}
							floppyState[fdDisk] |= FLOPPY_STATE_XFER;	// xfer dopo la prima lettura (award286...
							break;
						
						case FLOPPY_SENSE_INTERRUPT:			// sense interrupt status	
							
							if(!FloppyFIFOPtrR) {

								switch(floppylastCmd[fdDisk]) {		// https://wiki.osdev.org/Floppy_Disk_Controller#Sense_Interrupt
									case FLOPPY_SEEK: // seek
										FloppyFIFO[1] = FLOPPY_ERROR_SEEKEND | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
										break;
									case FLOPPY_RECALIBRATE: // recalibrate
										FloppyFIFO[1] = FLOPPY_ERROR_SEEKEND | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
										break;
									case 0xff: // uso io per reset
										FloppyFIFO[1] = 0b11000000 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								// v. anomalia link froci del cazzo (OVVIAMENTE è strano, ma ESIGE 1100xxxx ... boh
#ifndef PCAT
								FloppyFIFO[1] &= 0b11100000; // vaffanculo
#else
#endif
								// su AMIBIOs 286 cmq va anche senza la AND...
										break;
									default:
										FloppyFIFO[1] = 0b00000000 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
										break;
									}

								FloppyFIFO[2] = floppyTrack[fdDisk];
//								FloppyFIFO[3] = floppyTrack[fdDisk];
								FloppyFIFOPtrR=1;
//								if(!(floppyState[fdDisk] & 0b10000000))		// forzo errore se disco non c'è... QUA dà errore POST!
//									FloppyFIFO[1] |= FLOPPY_ERROR_INVALID | FLOPPY_ERROR_ABNORMAL;

//								FloppyContrRegR[0]=0b00000000;
								FloppyContrRegR[0]=FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY; // (con 11000000 phoenix si schianta a cazzo!! ma si blocca cmq...
								}
							else {
								if(FloppyFIFOPtrR > 2) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);					// busy
								}
							break;

						case FLOPPY_SENSE_DRIVE_STATUS:			// sense drive status
							if(FloppyFIFOPtrR) {
								floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read
								}
							if(FloppyFIFOPtrR>1) {		// 1 byte da inviare al pc
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR | FLOPPY_STATE_XFER);	// passo a write, tolgo xfer
								FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
								FloppyFIFOPtrW=0;
								}
							else {
								if(FloppyFIFOPtrW > 0) {
									FloppyContrRegR[0]=FLOPPY_STATUS_BUSY | (1 << fdDisk);	// busy
									if(floppyState[fdDisk] & FLOPPY_STATE_XFER) {
										FloppyContrRegR[0] |= FLOPPY_STATUS_REQ;		// ok to xfer
										}
									floppyState[fdDisk] |= FLOPPY_STATE_XFER;	// xfer dopo la prima lettura (award286...
									}
								else
									FloppyContrRegR[0]=0b00000000 | (1 << fdDisk);		// non busy
								}
							FloppyContrRegR[0] |= (floppyState[fdDisk] & FLOPPY_STATE_DDR);
							break;

						case FLOPPY_VERSION:		// version
							if(!FloppyFIFOPtrR) {

								FloppyFIFO[1] = 0x90;		// 82077A
								FloppyFIFOPtrR=1;

								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 1) {
									floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR);	// passo a write
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
									}
								else
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY;					// busy, qua no fdDisk direi, è controller
								}
							break;
						case FLOPPY_LOCK:		// lock...
							break;
						case FLOPPY_PERPENDICULAR_MODE:		// perpendicular
							break;
						case FLOPPY_DUMPREG:		// dumpreg
							break;
						case 0x00:			// (all'inizio comandi...

#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: COMANDO 00\n",timeGetTime()
				);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif

							FloppyContrRegR[0]=FLOPPY_STATUS_REQ | (floppyState[fdDisk] & FLOPPY_STATE_DDR);
								// b7=1 ready!;	b6 è direction (0=write da CPU a FDD, 1=read); b4 è busy/ready=0

//							floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read NO
							FloppyFIFOPtrW=0;
//							floppylastCmd[fdDisk]=FloppyContrRegW[1] & B8(00011111);
							break;

						default:		// invalid, 
							FloppyFIFO[0];
//								FloppyContrRegR[4] |= B8(10000000);		// 
							if(!FloppyFIFOPtrR) {
								FloppyFIFO[1] = FLOPPY_ERROR_INVALID | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)


#pragma message("verificare ST0")

								if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
									setFloppyIRQ(SEEK_FLOPPY_DELAYED_IRQ);
								// anche qua??


								floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
								}
							FloppyContrRegR[0]=FloppyFIFO[FloppyFIFOPtrR++];
							if(FloppyFIFOPtrR==1) {
								FloppyFIFOPtrW=0;
								floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read
								}
							break;
						}
					i=FloppyContrRegR[0];
          break;

        case 5:   // Read/Write: FDC command/data register
//        case 6:   // ************ TEST BUG GLABIOS 0.4
					switch(FloppyContrRegW[1] & FLOPPY_COMMAND_MASK) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
						case 0x01:
						case FLOPPY_READ_TRACK:
						case FLOPPY_WRITE_DATA:
						case FLOPPY_READ_DATA:
						case FLOPPY_SENSE_INTERRUPT:
						case FLOPPY_WRITE_DELETED_DATA:
						case FLOPPY_READ_ID:
						case FLOPPY_READ_DELETED_DATA:
						case FLOPPY_FORMAT_TRACK:
						case FLOPPY_SCAN_EQUAL:
						case FLOPPY_SCAN_LOW_OR_EQUAL:
						case FLOPPY_SCAN_HIGH_OR_EQUAL:
						case FLOPPY_VERSION:
						case FLOPPY_SENSE_DRIVE_STATUS:
							FloppyContrRegR[1]=FloppyFIFO[FloppyFIFOPtrR++];
							break;
						case FLOPPY_SPECIFY:
						case FLOPPY_RECALIBRATE:
						case FLOPPY_SEEK:
							FloppyContrRegR[1]=FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR;
							break;
						default:
							FloppyContrRegR[1]=FLOPPY_STATUS_REQ;
							break;
						}
					i=FloppyContrRegR[1];
          break;

#ifdef PCAT
				case 0:		// 
					break;
				case 1:		// digital output
					i=0;		// AMI legge, fa and con F8h e confronta con 50h...
					break;
				case 6:		// dice come HD status register... boh?? sarebbe 1f7?? secondo granite, cmq https://www.mjm.co.uk/how-hard-disk-ata-registers-work.html
					i=hdState;
					break;
				case 7:		// solo AT http://isdaman.com/alsos/hardware/fdc/floppy.htm
					if(floppyState[fdDisk] & FLOPPY_STATE_DISKCHANGED) {			// disk changed
						i=0b10000000;
						floppyState[fdDisk] &= ~FLOPPY_STATE_DISKCHANGED;
						}
					else
						i=0b00000000;
          break;
#endif


    //_UART0_DATA      .equ $FF80        		 ; data
    //_UART0_DLAB_0    .equ $FF80        		 ; divisor latch low uint8_t
    //_UART0_DLAB_1    .equ $FF81        		 ; divisor latch high uint8_t
    //_UART0_IER       .equ $FF81        		 ; Interrupt enable register
    //_UART0_FCR       .equ $FF82        		 ; FIFO control register
    //_UART0_LCR       .equ $FF83        		 ; line control register
    // 4= Modem CTRL
    //_UART0_LSR       .equ $FF85        		 ; line status register
    // 6= Modem Status
    // 7= scratchpad
        case 0+8:
          if(i8250Reg[3] & 0x80) {
            i=i8250Regalt[0];
            }
          else {
//            i=ReadUART1();
#ifdef MOUSE_TYPE
						i=COMDataEmuFifo[COMDataEmuFifoCnt++];
#if MOUSE_TYPE==1
						COMDataEmuFifoCnt %= 3;		// 3 byte per mouse microsoft seriale
#elif MOUSE_TYPE==2
						COMDataEmuFifoCnt %= 5;		// 5 byte per mouse mouse system seriale
#endif
						if(!COMDataEmuFifoCnt) {
				      mouseState |= 128;  // marker 
							}
						else {							     // ri-simulo per altre 2/4 volte!
							i8250Reg[2] |= 0b00000001;
							if(i8250Reg[1] & 0b00000001)			// se IRQ attivo
								i8259IRR |= 0b00010000;
							}
#endif
            }
          break;
        case 1+8:
          if(i8250Reg[3] & 0x80) {
            i=i8250Regalt[1];
            }
          else {
/*            if(DataRdyUART1())
              i8250Reg[1] |= 1;
            else
              i8250Reg[1] &= ~1;*/
						if(!(mouseState & 128))
              i8250Reg[1] |= 1;
						else
              i8250Reg[1] &= ~1;

            /*if(!BusyUART1())
              i8250Reg[1] |= 2;
            else
              i8250Reg[1] &= ~2;*/
            i8250Reg[1] |= 2;
            i=i8250Reg[1];
            }
          break;
        case 2+8:
          i=i8250Reg[2];
          break;
        case 3+8:
          i=i8250Reg[3];
          break;
        case 4+8:
          i=i8250Reg[4];
          break;
        case 5+8:
          if(i8250Reg[3] & 0x80) {
  //          i=i8250Regalt[2];
            }
          else {
#ifdef MOUSE_TYPE
						if(COMDataEmuFifoCnt || (mouseState & 128))
              i8250Reg[5] |= 1;
            else
              i8250Reg[5] &= ~1;

#else
/*            if(DataRdyUART1())
              i8250Reg[5] |= 1;
            else
              i8250Reg[5] &= ~1;
            if(U1STAbits.FERR)
              i8250Reg[5] |= 8;
            else
              i8250Reg[5] &= ~8;
            if(U1STAbits.OERR)
              i8250Reg[5] |= 2;
            else
              i8250Reg[5] &= ~2;
            if(U1STAbits.PERR)
              i8250Reg[5] |= 4;
            else
              i8250Reg[5] &= ~4;
            if(!BusyUART1())    // THR
              i8250Reg[5] |= 0x20;
            else
              i8250Reg[5] &= ~0x20;
            if(!U1STAbits.UTXBF)    // TX
              i8250Reg[5] |= 0x40;
            else
              i8250Reg[5] &= ~0x40;*/
#endif
            i=i8250Reg[5];
            }
          break;
        case 6+8:
          i=i8250Reg[6];
          break;
        case 7+8:
          i=i8250Reg[7];
          break;
        }

      break;
		default:
			i=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
#ifdef EXT_80286
			Exception86.descr.ud=EXCEPTION_NP;
			goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
			Exception86.active=1;
			Exception86.addr=t;
			Exception86.descr.in=1;
			Exception86.descr.rw=1;

skippa:
#endif
			break;
    }
  
	return i;
	}

uint16_t InShortValue(uint16_t t) {
	register uint16_t i;

return MAKEWORD(InValue(t),InValue(t+1));			// per ora, v. cmq GLABios
	return i;
	}

uint16_t GetShortValue(struct REGISTERS_SEG *seg,uint16_t ofs) {
	register union DWORD_BYTES i;
	uint32_t t,t1;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_R();
		t1=t+1;
		}
	else
#endif
		{
#if !defined(EXT_80186)
	//A word write to to 0FFFFH will write to 00000H with the 8086, and 010000H with the 80186.
	t=MAKE20BITS(seg->s.x,ofs);
	t1=MAKE20BITS(seg->s.x,ofs+1);
// secondo i test V20 è uguale all'altro...
#else
	t=MAKE20BITS(seg->s.x,ofs);
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif
	t1=t+1;
#endif
#ifdef EXT_80286
	if(!_msw.PE) {
		if(ofs==0xffff)			// real-mode exception
			;
		}
#endif
		}

#ifdef EXT_80286
	if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
		i.b.l=ram_seg[t-0x100000L+RAM_SIZE];
		i.b.h=ram_seg[t1-0x100000L+RAM_SIZE];
		}
	else 
#endif
	if(t >= (ROM_END-ROM_SIZE)) {
		t-=ROM_END-ROM_SIZE;
		t1-=ROM_END-ROM_SIZE;
		i.b.l=bios_rom[t];
		i.b.h=bios_rom[t1];
		}
#ifdef ROM_BASIC
	else if(t >= 0xF6000) {
		t-=0xF6000;
		t1-=0xF6000;
		i.b.l=IBMBASIC[t];
		i.b.h=IBMBASIC[t1];
		}
#endif
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		t-=ROM_START2;
		t1-=ROM_START2;
		i.b.l=bios_rom_opt[t];
		i.b.h=bios_rom_opt[t1];
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE && t<ROM_HD_BASE+32768) {
		t-=ROM_HD_BASE;
		t1-=ROM_HD_BASE;
		i.b.l=HDbios[t];
		i.b.h=HDbios[t1];
		}
#endif
#ifdef VGA_BASE
	else if(t >= VGA_BASE && t < VGA_BASE+0x20000L /*VGA_SIZE*/) {
		i.b.h=GetValue(seg,ofs+1);		// inoltre i doc dicono che in genere si dovrebbe accedere solo a byte...
		i.b.l=GetValue(seg,ofs);	// faccio così, date tutte le complicazioni; 
		}
#endif
#ifdef CGA_BASE
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		t1-=CGA_BASE;
		i.b.l=CGAram[t];
		i.b.h=CGAram[t1];
    }
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < (VGA_BIOS_BASE+32768)) {
		t-=VGA_BIOS_BASE;
		t1-=VGA_BIOS_BASE;
		i.b.l=VGABios[t];
		i.b.h=VGABios[t1];
    }
#endif
#ifdef RAM_DOS
	else if(t >= 0x7c000 && t < 0x80000)) {
		t-=0x7c000;
		t1-=0x7c000;
		i.b.l=disk_ram[t-0x7c000];
		i.b.h=disk_ram[t1-0x7c000];
		}
//		i=iosys[t-0x7c000];
#endif
	else if(t < RAM_SIZE) {
		i.b.l=ram_seg[t];
		i.b.h=ram_seg[t1];
		}
// else _f.Trap=1?? v. anche eccezione PIC
	else {
		i.w.l=UNIMPLEMENTED_MEMORY_VALUE;
#ifdef EXT_80286
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa: ;
#endif
	}
  
	return i.w.l;
	}

#ifdef EXT_80386
uint32_t GetIntValue(struct struct REGISTERS_SEG *seg,uint32_t ofs) {
	register DWORD_BYTES i;
	DWORD t,t1;

	if(_msw.PE) {
		ADDRESS_286_R();
		}
	else {
		t=MAKE20BITS(seg->x,ofs);
		}
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif
		t1=t+1;

#ifdef EXT_80286
	else if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
		i.b.l=ram_seg[t-0x100000L+RAM_SIZE];
		i.b.h=ram_seg[t+1-0x100000L+RAM_SIZE];
		i.b.u=ram_seg[t+2-0x100000L+RAM_SIZE];
		i.b.u2=ram_seg[t+3-0x100000L+RAM_SIZE];
		}
	else 
#endif
	if(t >= (ROM_END-ROM_SIZE)) {
		t-=ROM_END-ROM_SIZE;
		i.b.l=bios_rom[t];
		i.b.h=bios_rom[t+1];
		i.b.u=bios_rom[t+2];
		i.b.u2=bios_rom[t+3];
		}
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		t-=ROM_START2;
		i.b.l=bios_rom_opt[t];
		i.b.h=bios_rom_opt[t+1];
		i.b.u=bios_rom_opt[t+2];
		i.b.u2=bios_rom_opt[t+3];
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE && t<ROM_HD_BASE+32768) {
		t-=ROM_HD_BASE;
		i.b.l=HDbios[t];
		i.b.h=HDbios[t+1];
		i.b.u=HDbios[t+2];
		i.b.u2=HDbios[t+3];
		}
#endif
#ifdef VGA_BASE
	else if(t >= VGA_BASE && t < VGA_BASE+0x20000L /*VGA_SIZE*/) {
		i=MAKELONG(MAKEWORD(CGAram[t],CGAram[t+1]),MAKEWORD(CGAram[t+2],CGAram[t+3]));
		i.b.u2=GetValue(seg,ofs+3);
		i.b.u=GetValue(seg,ofs+2);
		i.b.h=GetValue(seg,ofs+1);		// inoltre i doc dicono che in genere si dovrebbe accedere solo a byte...
		i.b.l=GetValue(seg,ofs);	// faccio così, date tutte le complicazioni; 
		}
#endif
#ifdef CGA_BASE
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		i.b.l=CGAram[t];
		i.b.h=CGAram[t+1];
		i.b.u=CGAram[t+2];
		i.b.u2=CGAram[t+3];
    }
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		t-=VGA_BIOS_BASE;
		i.b.l=VGABios[t];
		i.b.h=VGABios[t+1];
		i.b.u=VGABios[t+2];
		i.b.u2=VGABios[t+3];
    }
#endif
	else if(t < RAM_SIZE) {
		i.b.l=ram_seg[t];
		i.b.h=ram_seg[t+1];
		i.b.u=ram_seg[t+2];
		i.b.u2=ram_seg[t+3];
		}
	else
		i=UNIMPLEMENTED_MEMORY_VALUE;
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
		Exception86.parm=seg->s.x;
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa:

	return i;
	}
#endif

uint8_t GetPipe(struct REGISTERS_SEG *seg,uint16_t ofs) {
	uint32_t t,t1;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_E();
		t1=t+1;
		}
	else
#endif
	{
#if !defined(EXT_80186)
	t=MAKE20BITS(seg->s.x,ofs);
	t1=MAKE20BITS(seg->s.x,ofs+1);
// secondo i test V20 è uguale all'altro...
#else
	t=MAKE20BITS(seg->s.x,ofs);
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif
	t1=t+1;
#endif
	}

#ifdef EXT_80286
	if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
		Pipe1=ram_seg[t++ -0x100000L+RAM_SIZE];
		Pipe2.b.l=ram_seg[t++ -0x100000L+RAM_SIZE];
		Pipe2.b.h=ram_seg[t++ -0x100000L+RAM_SIZE];
		Pipe2.b.u=ram_seg[t-0x100000L+RAM_SIZE];
		}
	else 
#endif
	if(t >= (ROM_END-ROM_SIZE)) {
		t-=(ROM_END-ROM_SIZE);
	  Pipe1=bios_rom[t++];
		Pipe2.b.l=bios_rom[t++];
		Pipe2.b.h=bios_rom[t++];
		Pipe2.b.u=bios_rom[t];
//		Pipe2.b.u2=bios_rom[t];
		}
#ifdef ROM_BASIC
	else if(t >= 0xF6000) {
		t-=0xF6000;
	  Pipe1=IBMBASIC[t++];
		Pipe2.b.l=IBMBASIC[t++];
		Pipe2.b.h=IBMBASIC[t++];
		Pipe2.b.u=IBMBASIC[t];
//		Pipe2.b.u2=IBMBASIC[t];
		}
#endif
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		t-=ROM_START2;
	  Pipe1=bios_rom_opt[t++];
		Pipe2.b.l=bios_rom_opt[t++];
		Pipe2.b.h=bios_rom_opt[t++];
		Pipe2.b.u=bios_rom_opt[t];
//		Pipe2.b.u2=bios_rom_opt[t];
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE && t<ROM_HD_BASE+32768) {
		t-=ROM_HD_BASE;
	  Pipe1=HDbios[t++];
		Pipe2.b.l=HDbios[t++];
		Pipe2.b.h=HDbios[t++];
		Pipe2.b.u=HDbios[t];
//		Pipe2.b.u2=HDbios[t];
		}
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		t-=VGA_BIOS_BASE;
	  Pipe1=VGABios[t++];
		Pipe2.b.l=VGABios[t++];
		Pipe2.b.h=VGABios[t++];
		Pipe2.b.u=VGABios[t];
//		Pipe2.b.u2=bios_rom[t];
    }
#endif
#ifdef RAM_DOS
	else if(t >= 0x7c000 && t < 0x80000) {
		t-=0x7c000;
	  Pipe1=disk_ram[t++];
		Pipe2.b.l=disk_ram[t++];
		Pipe2.b.h=disk_ram[t++];
		Pipe2.b.u=disk_ram[t];
//		Pipe2.b.u2=disk_ram[t];
//		i=iosys[t-0x7c00];
    }
#endif
	else if(t < RAM_SIZE) {
	  Pipe1=ram_seg[t++];
		Pipe2.b.l=ram_seg[t++];
		Pipe2.b.h=ram_seg[t++];
		Pipe2.b.u=ram_seg[t];
//		Pipe2.b.u2=ram_seg[t];
		}
	else {
	  Pipe1=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
		Pipe2.b.l=LOBYTE(UNIMPLEMENTED_MEMORY_VALUE);
		Pipe2.b.h=HIBYTE(UNIMPLEMENTED_MEMORY_VALUE);
		Pipe2.b.u=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
#ifdef EXT_80286
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
		Exception86.parm=seg->s.x;
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa: ;
#endif
		}

	return Pipe1;
	}

uint8_t GetMorePipe(struct REGISTERS_SEG *seg,uint16_t ofs) {
	uint32_t t,t1;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_E();
		t1=t+1;
		}
	else
#endif
	{
#if !defined(EXT_80186)
	t=MAKE20BITS(seg->s.x,ofs);
	t1=MAKE20BITS(seg->s.x,ofs+1);
// secondo i test V20 è uguale all'altro...
#else
	t=MAKE20BITS(seg->s.x,ofs);
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif
	t1=t+1;
#endif
	}

#ifdef EXT_80286
	if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
		Pipe2.bd[3]=ram_seg[t++ -0x100000L+RAM_SIZE];
		Pipe2.bd[4]=ram_seg[t++ -0x100000L+RAM_SIZE];
		Pipe2.bd[5]=ram_seg[t-0x100000L+RAM_SIZE];
		}
	else 
#endif
	if(t >= (ROM_END-ROM_SIZE)) {
		t-=(ROM_END-ROM_SIZE -4);
		Pipe2.bd[3]=bios_rom[t++];
		Pipe2.bd[4]=bios_rom[t++];
		Pipe2.bd[5]=bios_rom[t];
		}
/*	else if(t >= 0x7c00 && t < 0x8000) {
		i=iosys[t-0x7c00];
		}*/
#ifdef ROM_BASIC
	else if(t >= 0xF6000) {
		t-=(0xF6000 -4);
		Pipe2.bd[3]=IBMBASIC[t++];
		Pipe2.bd[4]=IBMBASIC[t++];
		Pipe2.bd[5]=IBMBASIC[t];
		}
#endif
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		t-=(ROM_START2-4);
		Pipe2.bd[3]=bios_rom_opt[t++];
		Pipe2.bd[4]=bios_rom_opt[t++];
		Pipe2.bd[5]=bios_rom_opt[t];
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE && t<ROM_HD_BASE+32768) {
		t-=(ROM_HD_BASE -4);
		Pipe2.bd[3]=HDbios[t++];
		Pipe2.bd[4]=HDbios[t++];
		Pipe2.bd[5]=HDbios[t];
		}
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		t-=(VGA_BIOS_BASE-4);
		Pipe2.bd[3]=VGABios[t++];
		Pipe2.bd[4]=VGABios[t++];
		Pipe2.bd[5]=VGABios[t];
    }
#endif
	else if(t < RAM_SIZE) {
    t+=4;
		Pipe2.bd[3]=ram_seg[t++];
		Pipe2.bd[4]=ram_seg[t++];
		Pipe2.bd[5]=ram_seg[t];
		}
	else {
		Pipe2.bd[3]=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
		Pipe2.bd[4]=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
		Pipe2.bd[5]=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
#ifdef EXT_80286
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
		Exception86.parm=seg->s.x;
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa: ;
#endif
		}

	return Pipe1;
	}

void PutValue(struct REGISTERS_SEG *seg,uint16_t ofs,uint8_t t1) {
	uint32_t t;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_W();
		}
	else
#endif
	t=MAKE20BITS(seg->s.x,ofs);
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif

// printf("ram_seg: %04x, p: %04x\n",rom_seg,p);
#ifdef RAM_DOS
	if(t >= 0x7c000 && t < 0x80000) {
		disk_ram[t-0x7c000]=t1;
		} else 
#endif
#ifdef VGA_BASE
	if(t >= VGA_BASE && t < VGA_BASE+0x20000L /*VGA_SIZE*/) {
		BYTE v1,v2, mask;
		BYTE t2,t3,t4;
		DWORD vgaBase,vgaSegm,vgaPlane;

		switch(VGAgraphReg[6] & 0x0c) {
			case 0x00:
				vgaBase=0xA0000L-VGA_BASE;
				vgaSegm=0x20000L;
				vgaPlane=0x10000;
				break;
			case 0x04:
				vgaBase=0xA0000L-VGA_BASE;
				vgaSegm=0x10000L;
				vgaPlane=0x10000;
				break;
			case 0x08:
				vgaBase=0xB0000L-VGA_BASE;
				vgaSegm=0x8000L;
				vgaPlane=0x2000;
				break;
			case 0x0c:
				vgaBase=0xB8000L-VGA_BASE;
				vgaSegm=0x8000L;
				vgaPlane=0x2000;
				break;
			}
		switch(VGAgraphReg[5] & 0x03) {			// write mode
			case 0x00:
				t1 = (t1 >> (VGAgraphReg[3] & 0x7)) | (t1 << (8-(VGAgraphReg[3] & 0x7)));		// rotate viene prima di tutto, dice https://wiki.osdev.org/VGA_Hardware#The_Latches
				if(VGAgraphReg[1] & 8)			// S/R enable
					t4=VGAgraphReg[0] & 8 ? 255 : 0;		// S/R
				else
					t4=t1;
				if(VGAgraphReg[1] & 4)
					t3=VGAgraphReg[0] & 4 ? 255 : 0;
				else
					t3=t1;
				if(VGAgraphReg[1] & 2)
					t2=VGAgraphReg[0] & 2 ? 255 : 0;
				else
					t2=t1;
				if(VGAgraphReg[1] & 1)
					t1=VGAgraphReg[0] & 1 ? 255 : 0;
				else
					t1=t1;
				mask=VGAgraphReg[8];
				break;
			case 0x01:
				t1=VGAlatch.bd[0];
				t2=VGAlatch.bd[1];
				t3=VGAlatch.bd[2];
				t4=VGAlatch.bd[3];
				mask=255;

				// NO mask qua??? pare di no v.granite

				break;
			case 0x02:
				t4=t1 & 8 ? 255 : 0;
				t3=t1 & 4 ? 255 : 0;
				t2=t1 & 2 ? 255 : 0;
				t1=t1 & 1 ? 255 : 0;


				t1 = (t1 >> (VGAgraphReg[3] & 0x7)) | (t1 << (8-(VGAgraphReg[3] & 0x7)));		// rotate viene prima di tutto, dice https://wiki.osdev.org/VGA_Hardware#The_Latches
// secondo granite, rotate c'è anche qua, DOPO

				mask=VGAgraphReg[8];
				break;
			case 0x03:
				mask = (t1 >> (VGAgraphReg[3] & 0x7)) | (t1 << (8-(VGAgraphReg[3] & 0x7)));		// rotate viene prima di tutto, dice https://wiki.osdev.org/VGA_Hardware#The_Latches
				mask &= VGAgraphReg[8];
				t4=VGAgraphReg[0] & 8 ? 255 : 0;
				t3=VGAgraphReg[0] & 4 ? 255 : 0;
				t2=VGAgraphReg[0] & 2 ? 255 : 0;
				t1=VGAgraphReg[0] & 1 ? 255 : 0;
				break;
			}
		switch(VGAgraphReg[5] & 0x03) {			// write mode
//granite			case 0x00:


			case 0x01:
// non si capisce libro del cazzo programmatori col cancro


			case 0x02:
// granite			case 0x03:
				switch(VGAgraphReg[3] & 0x18) {			// function select
					case 0x00:
						break;
					case 0x08:
						t4 &= VGAlatch.bd[3];
						t3 &= VGAlatch.bd[2];
						t2 &= VGAlatch.bd[1];
						t1 &= VGAlatch.bd[0];
						break;
					case 0x10:
						t4 |= VGAlatch.bd[3];
						t3 |= VGAlatch.bd[2];
						t2 |= VGAlatch.bd[1];
						t1 |= VGAlatch.bd[0];
						break;
					case 0x18:
						t4 ^= VGAlatch.bd[3];
						t3 ^= VGAlatch.bd[2];
						t2 ^= VGAlatch.bd[1];
						t1 ^= VGAlatch.bd[0];
						break;
					}
				break;
			}
		if(VGAgraphReg[5] & 0x10) {		// odd/even (p.406), dice SEMPRE opposto di VGAseqReg[4] b2
			}
		if(!(VGAseqReg[4] & 4)) {		// Note that the value of this bit should be the complement of the value in the OE field of the Mode Register.
			// v. anche b5 in Miscellaneous https://www.vogons.org/viewtopic.php?t=42454
			BOOL oldT=t & 1;
			t/=2;
			t &= (vgaPlane-1);
//			if((VGAgraphReg[5] & 0x20)) {		// patch per mode 4/5 (CGA shift register
//				t+=oldT;
//				}
			t+=vgaBase;
			if(!oldT) {
				if(VGAseqReg[2] & 1) {
					v1 = t1 & mask;		// salvo i bit a 1 del risultato...
					v2 = VGAram[t] & ~mask;	// ...e i bit a 0 originali...
					VGAram[t]=v1 | v2;			// ...e unisco
					}
				if(VGAseqReg[2] & 4) {
					v1 = t3 & mask;
					v2 = VGAram[t+vgaPlane*2] & ~mask;
					VGAram[t+vgaPlane*2]=v1 | v2;
					}
				}
			else {
				if(VGAseqReg[2] & 2) {
					v1 = t2 & mask;
					v2 = VGAram[t+vgaPlane] & ~mask;
					VGAram[t+vgaPlane]=v1 | v2;
					}
				if(VGAseqReg[2] & 8) {
					v1 = t4 & mask;
					v2 = VGAram[t+vgaPlane*3] & ~mask;
					VGAram[t+vgaPlane*3]=v1 | v2;
					}
				}
			}
		else {
			t &= (vgaSegm-1);
			t+=vgaBase;
			if(VGAseqReg[2] & 1) {
				v1 = t1 & mask;		// salvo i bit a 1 del risultato...
				v2 = VGAram[t] & ~mask;	// ...e i bit a 0 originali...
				VGAram[t]=v1 | v2;			// ...e unisco
				}
			if(VGAseqReg[2] & 2) {
				v1 = t2 & mask;
				v2 = VGAram[t+vgaPlane] & ~mask;
				VGAram[t+vgaPlane]=v1 | v2;
				}
			if(VGAseqReg[2] & 4) {
				v1 = t3 & mask;
				v2 = VGAram[t+vgaPlane*2] & ~mask;
				VGAram[t+vgaPlane*2]=v1 | v2;
				}
			if(VGAseqReg[2] & 8) {
				v1 = t4 & mask;
				v2 = VGAram[t+vgaPlane*3] & ~mask;
				VGAram[t+vgaPlane*3]=v1 | v2;
				}
			}
		}
#endif
#ifdef CGA_BASE
	if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		CGAram[t]=t1;
    }
#endif
#ifdef EXT_80286
	else if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
	  ram_seg[t-0x100000L+RAM_SIZE]=t1;
		}
#endif
	else if(t < RAM_SIZE) {
	  ram_seg[t]=t1;
		}
	else {//_f.Trap=1?? v. anche eccezione PIC
#ifdef EXT_80286
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
		Exception86.parm=seg->s.x;
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=0;

skippa: ;
#endif

		}
	}

void  PutShortValue(struct REGISTERS_SEG *seg,uint16_t ofs,uint16_t t2) {
	uint32_t t,t1;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_W();
		t1=t+1;
		}
	else
#endif
	{
#if !defined(EXT_80186) 
	t=MAKE20BITS(seg->s.x,ofs);
	t1=MAKE20BITS(seg->s.x,ofs+1);
	//A word write to to 0FFFFH will write to 00000H with the 8086, and 010000H with the 80186.
// secondo i test V20 è uguale all'altro...
#else
	t=MAKE20BITS(seg->s.x,ofs);
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif
	t1=t+1;
#endif
#ifdef EXT_80286
	if(!_msw.PE) {
		if(ofs==0xffff)			// real-mode exception
			;
		}
#endif
	}

#ifdef RAM_DOS
	if(t >= 0x7c000 && t < 0x80000) {
		disk_ram[t-0x7c000]=t2;
		} else 
#endif
#ifdef VGA_BASE
	if(t >= VGA_BASE && t < VGA_BASE+0x20000L /*VGA_SIZE*/) {
		PutValue(seg,ofs,LOBYTE(t2));		// faccio così, date tutte le complicazioni; 
		PutValue(seg,ofs+1,HIBYTE(t2));		// inoltre i doc dicono che in genere si dovrebbe accedere solo a byte...
		}
#endif
#ifdef CGA_BASE
	if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		t1-=CGA_BASE;
		CGAram[t]=LOBYTE(t2);
		CGAram[t1]=HIBYTE(t2);
    }
#endif
#ifdef EXT_80286
	else if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
	  ram_seg[t-0x100000L+RAM_SIZE]=LOBYTE(t2);
	  ram_seg[t1-0x100000L+RAM_SIZE]=HIBYTE(t2);
		}
#endif
	else if(t < RAM_SIZE) {
	  ram_seg[t]=LOBYTE(t2);
	  ram_seg[t1]=HIBYTE(t2);
		}
	else {//_f.Trap=1?? v. anche eccezione PIC
#ifdef EXT_80286
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
		Exception86.parm=seg->s.x;
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=0;

skippa: ;
#endif
		}

	}


#ifdef EXT_80386
void PutIntValue(struct struct REGISTERS_SEG *seg,uint32_t ofs,uint32_t t1) {
	register uint16_t i;
	DWORD t,t1;

	if(_msw.PE) {
		ADDRESS_286_W();
		}
	else {
		t=MAKE20BITS(seg->x,ofs);
		}
#ifdef EXT_80286
	if(!(i8042Output & 2))/*A20*/
		t &= 0xffefffff;
#endif
	t1=t+1;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

#ifdef VGA_BASE
	if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		PutValue(seg,ofs,LOBYTE(LOWORD(t2)));		// faccio così, date tutte le complicazioni; 
		PutValue(seg,ofs+1,HIBYTE(LOWORD(t2)));		// inoltre i doc dicono che in genere si dovrebbe accedere solo a byte...
		PutValue(seg,ofs+2,LOBYTE(HIWORD(t2)));		// faccio così, date tutte le complicazioni; 
		PutValue(seg,ofs+3,HIBYTE(HIWORD(t2)));		// inoltre i doc dicono che in genere si dovrebbe accedere solo a byte...
    }
	else 
#endif
#ifdef CGA_BASE
	if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		CGAram[t++]=LOBYTE(LOWORD(t1));
		CGAram[t++]=HIBYTE(LOWORD(t1));
		CGAram[t++]=LOBYTE(HIWORD(t1));
		CGAram[t]=HIBYTE(HIWORD(t1));
    }
	else 
#endif
#ifdef EXT_80286
	if(t >= 0x100000L && t < 0x100000L+RAM_SIZE_EXTENDED) {
	  ram_seg[t++ -0x100000L+RAM_SIZE]=LOBYTE(LOWORD(t1));
	  ram_seg[t++ -0x100000L+RAM_SIZE]=HIBYTE(LOWORD(t1));
	  ram_seg[t++ -0x100000L+RAM_SIZE]=LOBYTE(HIWORD(t1));
	  ram_seg[t-0x100000L+RAM_SIZE]=HIBYTE(HIWORD(t1));
		}
#endif
	else if(t < RAM_SIZE) {
	  ram_seg[t++]=LOBYTE(LOWORD(t1));
	  ram_seg[t++]=HIBYTE(LOWORD(t1));
	  ram_seg[t++]=LOBYTE(HIWORD(t1));
	  ram_seg[t]=HIBYTE(HIWORD(t1));
		}
		else {
#ifdef EXT_80286
		Exception86.descr.ud=EXCEPTION_NP;
		goto skippa; // mmm no... pare di no (il test memoria estesa esploderebbe

exception286:
		Exception86.parm=seg->s.x;
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=0;

skippa:
#endif
		}
	}
#endif

void OutValue(uint16_t t,uint8_t t1) {      // https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
	register uint16_t i;
  int j;

  switch(t >> 4) {
    case 0:        // 00-1f DMA 8237 controller (usa il 2 per il floppy
    case 1:				// solo PS2 dice!
      t &= 0xf;
			switch(t) {
				case 0:
				case 2:
				case 4:
				case 6:
					t /= 2;
					if(!i8237FF)
						i8237DMAAddr[t]=(i8237DMAAddr[t] & 0xff00) | t1;
					else
						i8237DMAAddr[t]=(i8237DMAAddr[t] & 0xff) | ((uint16_t)t1 << 8);
					i8237DMACurAddr[t]=i8237DMAAddr[t];
					i8237FF++;
					i8237FF &= 1;
		      i8237RegR[t]=i8237RegW[t]=t1;
					break;
				case 1:
				case 3:
				case 5:
				case 7:
					t /= 2;
					if(!i8237FF)
						i8237DMALen[t]=(i8237DMALen[t] & 0xff00) | t1;
					else
						i8237DMALen[t]=(i8237DMALen[t] & 0xff) | ((uint16_t)t1 << 8);
					i8237DMACurLen[t]=i8237DMALen[t];
					i8237FF++;
					i8237FF &= 1;
		      i8237RegR[t]=i8237RegW[t]=t1;
					break;
				case 0x8:			// Command/status (reg.8
		      /*i8237RegR[8]=*/i8237RegW[8]=t1;
					break;
				case 0xa:			// set/clear Mask
					if(t1 & 0b00000100)
						i8237RegW[15] |= (1 << (t1 & 0b00000011));
					else
						i8237RegW[15] &= ~(1 << (t1 & 0b00000011));
					i8237RegR[15]=i8237RegW[15];
		      i8237RegR[0xa]=i8237RegW[0xa]=t1;
					break;
				case 0xb:			// Mode
					i8237Mode[t1 & 0b00000011]= t1 & 0b11111100;
					i8237DMACurAddr[t1 & 0b00000011]=i8237DMAAddr[t1 & 0b00000011];			// bah credo, qua
					i8237DMACurLen[t1 & 0b00000011]=i8237DMALen[t1 & 0b00000011];
		      i8237RegR[0xb]=i8237RegW[0xb]=t1;

					i8237RegR[8] |= ((1 << t1 & (0b00000011)) << 4);  // request??... boh

					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					i8237FF=0;
		      i8237RegR[0xc]=i8237RegW[0xc]=t1;
					break;
				case 0xd:			// Reset
					memset(i8237RegR,0,sizeof(i8237RegR));
					memset(i8237RegW,0,sizeof(i8237RegW));
					i8237RegR[15]=i8237RegW[15] = 0b00001111;
					i8237RegR[8]=0b00000000;
		      i8237RegR[0xd]=i8237RegW[0xd]=t1;
					break;
				case 0xe:			//  Clear Masks
					i8237RegR[15]=i8237RegW[15] &= ~0b00001111;
					break;
				case 0xf:			//  Masks
					i8237RegR[15]=i8237RegW[15] = (t1 & 0b00001111);
					break;
				default:
		      i8237RegR[t]=i8237RegW[t]=t1;
					break;
				}
      break;

    case 2:         // 20-21 PIC 8259 controller
//https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
#ifdef PCAT
      t &= 0x3;		// DTK legge/scrive a 22 23
#else
      t &= 0x1;
#endif
      switch(t) {
        case 0:     // 0x20 per EOI interrupt, 0xb per "chiedere" quale... subito dopo c'è lettura a 0x20
          i8259RegW[0]=t1;      // 
					if(t1 & 0b00010000) {// se D4=1 qua (0x10) è una sequenza di Init,
						i8259OCW[2] = PIC_WANTSIRR;		// dice, default su lettura IRR
//						i8259RegR[0]=0;
						// boh i8259IMR=0xff; disattiva tutti
						i8259ICW[0]=t1;
						i8259ICWSel=2;		// >=1 indica che siamo in scrittura ICW
						}
					else if(t1 & 0b00001000) {// se D3=1 qua (0x8) è una richiesta
						// questa è OCW3 
						i8259OCW[2]=t1;
						switch(i8259OCW[2] & 0b00000011) {		// riportato anche sopra, serve (dice, in polled mode)
							case 2:
								i8259RegR[0]=i8259IRR;
								break;
							case 3:
								i8259RegR[0]=i8259ISR;
								break;
							default:
								break;
							}
						// b2 è poll/nopoll...
						}
          else {
						// questa è OCW2 "di lavoro" https://k.lse.epita.fr/internals/8259a_controller.html
						i8259OCW[1]=t1;
						switch(i8259OCW[1] & 0b11100000) {
							case 0b00000000:// rotate pty in automatic (set clear
								break;
							case 0b00100000:			// non-spec, questo pulisce IRQ con priorità + alta in essere (https://rajivbhandari.wordpress.com/wp-content/uploads/2014/12/unitii_8259.pdf
//						i8259ISR=0;		// MIGLIORARE! pulire solo il primo
								for(i=1; i; i<<=1) {
									if(i8259ISR & i) {
										i8259ISR &= ~i;
										break;
										}
									}
//				      CPUPins &= ~DoIRQ;		// SEMPRE! non serve + v. di là
								break;
							case 0b01000000:			// no op
								break;
							case 0b01100000:			// 0x60 dovrebbe indicare uno specifico IRQ, nei 3bit bassi 0..7
								i8259ISR &= ~(1 << (t1 & 7));
								break;
							case 0b10000000:			// rotate pty in automatic (set
								break;
							case 0b10100000:			// rotate pty on non-specific
								break;
							case 0b11000000:			// set pty
								break;
							case 0b11100000:			// rotate pty on specific
								break;
							}


          // (pulire QUA i flag TMRIRQ ecc??  //(o toglierli del tutto e usare i bit di 8259
						}
          break;
        case 1:
          i8259RegR[1]=i8259RegW[1]=t1;      //  0xbc su bios3.1 (kbd, tod, floppy)
          // (cmq FINIRE! ci sono più comandi a seconda dei bit... all inizio manda 0x13 a 20 e 0x8 0x9 0xff a 21; poi per pulire un IRQ si manda il valore letto da 0x20 qua
					switch(i8259ICWSel) {
						case 2:
							i8259ICW[1]=t1;			// qua c'è base address per interrupt table (8 cmq)
							i8259ICWSel++;
							break;
						case 3:
							i8259ICWSel++;
							i8259ICW[2]=t1;			
							if(!(i8259ICW[0] & 0b00000001)) 		// ICW4 needed?
								i8259ICWSel=0;
							break;
						case 4:
							i8259ICW[3]=t1;
							i8259ICWSel=0;
							break;
						default:
							i8259IMR=t1;
							i8259OCW[0]=t1;
							break;
						}

          break;
#ifdef PCAT
				case 2:			// DTK 286 scrive 6a a 22 e legge a 23 al boot!
// v. datasheet chipset 82C211 (Neat-286), ci sono cose tipo fast-reset e fast gate A20

					break;
				case 3:
					break;
#endif
        }
      break;

    case 4:         // 40-43 PIT 8253/4 controller  https://en.wikipedia.org/wiki/Intel_8253
      t &= 0x3;
  //Set the PIT to the desired frequency
 	//Div = 1193180 / nFrequence;
 	//outb(0x43, 0xb6);
 	//outb(0x42, (uint8_t) (Div) );
 	//outb(0x42, (uint8_t) (Div >> 8));
      switch(t) {
        case 0:
        case 1:
        case 2:
					// NO, v. sopra (non si capisce un cazzo, ma diciamo che caricamenti singoli ATTIVANO sempre, il doppio disattiva su low e attiva su high
					// e scrittura a Mode attiva
          switch((i8253Mode[t] & 0b00110000) >> 4) {
            case PIT_LATCH:
              // latching operation... v. sotto
              break;
            case PIT_LOADLOW:
              i8253TimerL[t]=i8253TimerR[t]=i8253TimerW[t]=MAKEWORD(t1,0/*glabios usa così a me pare cagata HIBYTE(i8253TimerW[t])*/);
//		          i8253Flags[t] &= ~PIT_LOHIBYTE;
							i8253Flags[t] &= ~(PIT_ACTIVE | PIT_LOHIBYTE);          // 
#if defined(PC_IBM5150) || defined(PC_IBM5160)
								i8253Flags[t] |= PIT_ACTIVE;          // award del c, 5150 di merda PATCH perché così vanno tutti gli altri @#£$%
#endif
							goto reload0;
              break;
            case PIT_LOADHIGH:
              i8253TimerL[t]=i8253TimerR[t]=i8253TimerW[t]=MAKEWORD(0/* glabios usa così a me pare cagata LOBYTE(i8253TimerW[t])*/,
								t1);
								// 5160 ANDAVA senza patch qua sopra, se uso HIBYTE/LOBYTE ma SPARATEVI CRISTO!
//		          i8253Flags[t] &= ~PIT_LOHIBYTE;
							i8253Flags[t] &= ~(PIT_ACTIVE | PIT_LOHIBYTE);          // 
							goto reload0;
              break;
            case PIT_LOADBOTH:
		          if(i8253Flags[t] & PIT_LOHIBYTE)
                i8253TimerW[t]=MAKEWORD(LOBYTE(i8253TimerW[t]),t1);
              else
                i8253TimerW[t]=MAKEWORD(t1,HIBYTE(i8253TimerW[t]));
		          i8253Flags[t] ^= PIT_LOHIBYTE;
							if(!(i8253Flags[t] & PIT_LOHIBYTE)) {		// qua, se carico entrambi i byte
								i8253TimerL[t]=i8253TimerR[t]=i8253TimerW[t];

								i8253Flags[t] |= PIT_ACTIVE;          // award del c, 5150 di merda

reload0:
		          i8253Flags[t] |= PIT_LOADED;		// USARE di là per fare ACTIVE SOLO dopo un clock?!

			          i8253Flags[t] &= ~PIT_LATCHED;
//								i8253Flags[t] |= PIT_ACTIVE;          // award del c


reload0_:

								switch((i8253Mode[t] & 0b00001110) >> 1) {
									case PIT_MODE_INTONTERMINAL:
										i8253Flags[t] &= ~PIT_OUTPUT;          // OUT=0
										break;
									case PIT_MODE_ONESHOT:
										i8253Flags[t] &= ~PIT_OUTPUT;          // OUT=0
										break;
									case PIT_MODE_FREECOUNTER:
										i8253Flags[t] |= PIT_OUTPUT;          // OUT=1
										break;
									case PIT_MODE_SQUAREWAVEGEN:
										i8253Flags[t] |= PIT_OUTPUT;          // OUT=1
										i8253TimerR[t] &= 0xfffe;
										break;
									case PIT_MODE_SWTRIGGERED:
										break;
									case PIT_MODE_HWTRIGGERED:
										break;
									}

								}
              break;
            }

          i8253RegW[t]=t1;
//          PR4=i8253TimerW[0] ? i8253TimerW[0] : 65535;
          break;
        case 3:
					i=t1 >> 6;
          if(i < 3) {
						switch((t1 & 0b00110000) >> 4) {
							case PIT_LATCH:
								i8253TimerL[i]=i8253TimerR[i];		// 
			          i8253Flags[i] |= PIT_LATCHED;
/*								if(t1 & B8(00000010)) METTERE verificare
									i8253TimerL[0]=i8253TimerR[0];		// 
								if(t1 & B8(00000100))
									i8253TimerL[1]=i8253TimerR[1];		// 
								if(t1 & B8(00000100))
									i8253TimerL[2]=i8253TimerR[2];		// 
									*/
//						i8253TimerL[i]=i8253TimerR[i]=i8253TimerW[i];		// 
								break;
							case PIT_LOADLOW:		// 
							case PIT_LOADHIGH:		// 
            i8253Mode[i]= t1 & 0x3f;
						i8253TimerL[i]=i8253TimerR[i]=i8253TimerW[i];		// 
			          i8253Flags[i] &= ~PIT_LATCHED;

						i8253Flags[i] |= PIT_ACTIVE;          // se lo metto va DTK e si pianta award, e viceversa PORCODDIO v.sopra
								break;
							case PIT_LOADBOTH:
            i8253Mode[i]= t1 & 0x3f;
						i8253TimerL[i]=i8253TimerR[i]=i8253TimerW[i];		// 
			          i8253Flags[i] &= ~PIT_LATCHED;
						i8253Flags[i] |= PIT_ACTIVE;          // 
								break;
							}
	          i8253Flags[i] &= ~PIT_LOHIBYTE;
						}
					else {
						switch(t1 & 0b00110000) {
							case 0b00000000:
								break;
#ifdef PCAT
							case 0b00010000:		// Status (8254)
								;		// 
								break;
#endif
							case 0b00100000:		// Control
								;
								break;
							case 0b00110000:
								break;
							}
						}

          i8253RegR[3]=i8253RegW[3]=t1;
          break;
        }
      break;

    case 6:        // 60-6F 8042/8255 keyboard; speaker; config byte
#ifndef PCAT		// bah, chissà... questo dovrebbe usare 8255 e AT 8042...
      t &= 0x3;
      switch(t) {
        case 0:     // solo read
          break;
        case 1:     // brutalmente, per buzzer!  e machine flags... (da V20)
					{
             /*PB1 0 turn off speaker
             1 enable speaker data
             PB0 0 turn off timer 2
             1 turn on timer 2, gate speaker with    square wave*/
          j=i8253TimerW[2]/3  /* vale 0x528 vado a 0x190=400 */;      // 2025 (was: 980Hz 5/12/19
					// 0x54c da GLAbios al boot, 0x390-484 in SPEED test (???
          if(t1 & 2) {
            if(t1 & 1) {
              PR2 = j;		 // 
  #ifdef ST7735
              OC1RS = j/2;		 // 
              OC1CONbits.ON = 1;
  #endif
  #ifdef ILI9341
              OC7RS = j/2;		 // 
              OC7CONbits.ON = 1;
  #endif
              }
            else {
              PR2 = j;
  #ifdef ST7735
              OC1RS = j;		 // bah
              OC1CONbits.ON = 1;
  #endif
  #ifdef ILI9341
              OC7RS = j;		 // bah
              OC7CONbits.ON = 1;
  #endif
              }
            }
          else {
            PR2 = j;		 // 
  #ifdef ST7735
            OC1RS = 0;		 // 
  #endif
  #ifdef ILI9341
            OC7RS = 0;		 // 
  #endif
// lascio cmq acceso...
            }
//          MachineFlags=t1;
					// A5 non-turbo E070 (pcxtbios
					// B5 per pulire parity error E191
					// 85 enable parity E194
					// AD memory size nibble E1AA
					// 08 read low switches... E1C9
					// 04 toggle speed (letto da 0x61 e XOR 0c) E289
					//  set b4-5 per disabilitare, poi b6-b7 per pulire errore e quindi reset b4-5 per riattivare (NMI handler in pcxtbios
					// AD KB hold low+disable, NMI on, spkr off (glabios E0AE

// così va (STRAPUTTANODIO) anche se alcuni BIOS danno ancora errore nonostante la sequenza sia ok (forse mettere ritardo dopo reset??
					if((t1 & 0b10000000) && !(i8255RegW[1] & 0b10000000)) {		// ack kbd, clear (b7)
						KBStatus &= ~KB_OUTPUTFULL;
						Keyboard[0]=0;
						}
					if(t1 & 0b01000000) {		// reset (b6 sale)
						if(!(i8255RegW[1] & 0b01000000)) {
							KBStatus |= KB_OUTPUTFULL;			// output disponibile
							Keyboard[0]=0xAA;			//
							//    KBDIRQ=1;
							i8259IRR |= 0b00000010;
							}
						}

/*
					if(t1 & 0b00001000)
//						SendMessage(hWnd,WM_COMMAND,ID_OPZIONI_VELOCIT_LENTO,0,0);
						CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER);
					else
//						SendMessage(hWnd,WM_COMMAND,ID_OPZIONI_VELOCIT_VELOCE,0,0);
						CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER*2);
          */

					i8255RegR[1]=i8255RegW[1]=t1;
					}

          break;
        case 2:     // machine settings... (da V20), NMI
					i8255RegR[2]=i8255RegW[2]=t1;
					i8255RegR[2] &= ~0b11000000;		// bah direi! questi sarebbero parity error ecc, da settare in HW
          break;
        case 3:     // PIA 8255, port A & C dice, a E06C...  però sarebbe anche a 0x61 per machineflags
					i8255RegR[3]=i8255RegW[3]=t1;		// glabios mette 0x99 ossia PA=input, PC=tutta input, PB=output
          break;
        }

#else

      t &= 0x7;
      switch(t) {
        case 0:     // 
					KBStatus &= ~KB_COMMAND;		// è dato (non comando)
          switch(i8042RegW[1]) {
						case 0x60:
							KBCommand=t1;
							KBStatus &= ~KB_POWERON;
							KBStatus |= KBCommand & 0b00000100;
							break;
						case 0xD1:
							i8042Output=t1;			// https://wiki.osdev.org/A20_Line
#ifdef _DEBUG
#ifdef DEBUG_KB
			{
				extern HFILE spoolFile;
				extern uint16_t _ip;
				char myBuf[256];
				wsprintf(myBuf,"%u: %04x  A20=%u\n",timeGetTime(),
					_ip,i8042Output & 2 ? 1 : 0);
				_lwrite(spoolFile,myBuf,strlen(myBuf));

	//			if(i8042Output & 2)
	//				debug=1;
				}
#endif
#endif
/*							if(i8042Output & B8(00010000))
								KBStatus |= B8(00000001);
							else
								KBStatus &= ~B8(00000001);
							if(i8042Output & B8(00100000))
								KBStatus |= B8(00000010);
							else
								KBStatus &= ~B8(00000010);*/
							break;
						case 0xD0:			//read output port
							break;
						case 0xD3:			// write aux/2nd
							break;
						case 0xD4:			// write aux/2nd
							break;
						case 0xED:			// LED, patch per ricevere secondo byte
							t1;
							break;
						case 0xF3:			// set typematic, patch per ricevere secondo byte
							t1;
							break;
						default:
							if(!i8042RegW[1]) {     //comandi inviati qua (per la vera tastiera, sarebbero)
								i8042RegW[0]=t1;     // 
								i8042RegW[1]=0;     // direi, così pulisco
		// qualsiasi byte qua che non segue un comando RIABILITA la tastiera https://www.os2museum.com/wp/ibm-pcat-8042-keyboard-controller-commands/
								KBCommand &= ~0b00010000;
								switch(t1) {
									case 0xED:     //led 
		//            LED1 = t1 & 1;
		//            LED2 = t1 & 2 ? 1 : 0;
		//            LED3 = t1 & 4 ? 1 : 0;

		// DEVO ASPETTARE stato led qua!! come fare?
										i8042RegW[1]=0xED;
										KBStatus &= ~KB_OUTPUTFULL;		// 

kb_sendACK:
										KBStatus |= KB_ENABLED | KB_OUTPUTFULL;		// b4=enabled, data available
										KBCommand |= KB_IRQENABLE;		// specialmente per DTK
										i8042Output |= 0b00010000;
										Keyboard[0]=0xFA;		// ACK
										i8042OutPtr=1;
										i8042RegW[0]=t1;  		
										goto kb_setirq;
										break;
									case 0xFE:     //resend (sarebbe ultimo byte a meno che non fosse Resend...
										i8042OutPtr=1;
										i8042RegW[0]=t1;  		i8042RegW[1]=0;
										goto kb_setirq;
										break;
									case 0xEE:     //echo
										Keyboard[0]=i8042RegW[1];
										i8042RegW[1]=0;
										goto kb_sendACK;
										break;
									case 0xF4:     //enable QUESTO enable cmq è ENABLE SCANNING e sarebbe diverso da KBCommand<4>...
										i8042RegW[1]=0;
										goto kb_sendACK;
										break;
									case 0xF5:     //disable
										KBCommand |= 0b00010000;		//v. sopra
										i8042RegW[1]=0;
										goto kb_sendACK;
										break;
									case 0xF6:     //set default
										i8042RegW[1]=0;
										goto kb_sendACK;
										break;
									case 0xF3:     //set typematic
										i8042RegW[1]=0xF3;
										goto kb_sendACK;
										break;
									case 0xF2:     //read ID
										i8042RegW[1]=0xF2;

										Keyboard[2]=0xfa;		Keyboard[1]=0xAB;		Keyboard[0]=0x83;

										KBStatus |= KB_ENABLED | KB_OUTPUTFULL;		// b4=enabled, data available
										i8042Output |= 0b00010000;

										i8042OutPtr=3;

//										goto kb_sendACK;
//										debug=1;


										break;
//									case 0xF0:     //sarebbe Set translate mode bit , su MCA
										//KBCommand |= B8(01000000)
//										break;
									case 0xFF:     //reset
kb_reset:
										memset(&Keyboard,0,sizeof(Keyboard));
										KBCommand=0b01010001 /*XT,disabled,irq on  FORSE questo non andrebbe toccato*/;  
										/*i8042Input=0;*/ i8042Output=2/*A20*/; i8042OutPtr=2 /* power-on e byte AA pronto + ACK*/;
										Keyboard[0]=0xaa; Keyboard[1]=0xfa;
										KBStatus |= KB_ENABLED | KB_OUTPUTFULL;		// b4=enabled, data available
										i8042Output |= 0b00010000;
										break;
									default:
										if((t1>=0xef && t1<=/*0xf2 read ID!*/0xf1) || (t1>=0xf7 && t1<=0xfd)) {	// nop
											i8042RegW[1]=0;
											goto kb_sendACK;
											}
										break;
									}
								}
							else if(i8042RegW[1]>=0x60 && i8042RegW[1]<0x80) {		// write ram
kb_writeram_:
								KBRAM[i8042RegW[1] & 0x1f]=t1;
								}
							else if(i8042RegW[1]>=0x40 && i8042RegW[1]<0x60) {		// alias AMI
								goto kb_writeram_;
								}
							else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {  // boh... 2024
								//KBRAM[i8042RegW[1] & 0x1f]
								}
							else if(t1==0x92) {		// DTK Bios manda questo, pare al posto di reset
								goto kb_reset;
								}
							break;

//							i8042RegW[1]

            }
          i8042RegW[0]=t1;     // 
          i8042RegW[1]=0;     // direi, così pulisco
          break;

        case 1:     // brutalmente, per buzzer!  		system control port
		 /*		 bit 7	(1= IRQ 0 reset )
		 bit 6-4    reserved
		 bit 3 = 1  channel check enable
		 bit 2 = 1  parity check enable
		 bit 1 = 1  speaker data enable
		 bit 0 = 1  timer 2 gate to speaker enable*/
					{
						// secondo granite questo va salvato in i8042input, i 4 bit bassi
						i8042Input= (i8042Input & 0xf0) | (t1 & 0xf);

             /*PB1 0 turn off speaker
             1 enable speaker data
             PB0 0 turn off timer 2
             1 turn on timer 2, gate speaker with    square wave*/
          j=i8253TimerW[2]/3  /* vale 0x528 vado a 0x190=400 */;      // 2025 (was: 980Hz 5/12/19
          if(t1 & 2) {
            if(t1 & 1) {
              PR2 = j;		 // 
  #ifdef ST7735
              OC1RS = j/2;		 // 
              OC1CONbits.ON = 1;
  #endif
  #ifdef ILI9341
              OC7RS = j/2;		 // 
              OC7CONbits.ON = 1;
  #endif
              }
            else {
              PR2 = j;
  #ifdef ST7735
              OC1RS = j;		 // bah
              OC1CONbits.ON = 1;
  #endif
  #ifdef ILI9341
              OC7RS = j;		 // bah
              OC7CONbits.ON = 1;
  #endif
              }
            }
          else {
            PR2 = j;		 // 
  #ifdef ST7735
            OC1RS = 0;		 // 
  #endif
  #ifdef ILI9341
            OC7RS = 0;		 // 
  #endif
// lascio cmq acceso...
            }
					}

          break;

        case 4:     // 
					KBStatus |= KB_COMMAND;		// è comando
					if(t1 & 0b00001000)
						KBStatus |= KB_ENABLED;		// fa override di KB inhibit
          i8042RegW[1]=t1;
          switch(i8042RegW[1]) {
						case 0xAA: 					// self test
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_POWERON | KB_COMMAND;				// command, data input full, has data output, alzo power-on
							i8042Output |= 0b00010000;
	            Keyboard[1]=0x55; Keyboard[0]=0x55 /*ribadisco*/ /*1 /*ID*/; i8042OutPtr=2;  //(su AMI si incula tutto!
//							Keyboard[0]=0x55; i8042OutPtr=1;
							goto kb_setirq;
							break;
						case 0xA9:			// test lines aux / #2 device
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;

							// FARE!
							goto kb_setirq;
							break;
						case 0xAB:			// test lines 
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
	//gestito sopra cmq, UNIRE            Keyboard[0]=0 /*ok*/; i8042OutPtr=1;
							goto kb_setirq;
							break;
						case 0xAD:			// disable
							KBStatus |= KB_COMMAND | KB_INPUTFULL;		// command,data input full, 
//							KBStatus &= ~B8(00000001);				// pulisco NO! il coso lo manda dopo ogni tasto ricevuto...
	//						i8259IRR &= ~B8(00000010);	// tutto
              KBRAM[0] |= 0x10; //set bit 4 -> disable kbd  da granite...
//							KBCommand |= B8(00010000);
							break;
						case 0xAE:			// enable
							KBStatus |= KB_COMMAND | KB_INPUTFULL;				// command, data input full
//							KBStatus &= ~B8(00000001);				// pulisco
              KBRAM[0] &= ~0x10; //clear bit 4 -> enable kbd  da granite...
//							KBCommand &= ~B8(00010000);
							break;
						case 0x20: // r/w control
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							Keyboard[0]=i8042RegW[1]; i8042OutPtr=1;
							goto kb_setirq;
							break;
						case 0x60: // r/w control
							KBStatus |= KB_COMMAND | KB_INPUTFULL;				// command, data input full, has data output
							break;
						case 0xA1:		//firmware version  https://aeb.win.tue.nl/linux/kbd/scancodes-11.html#inputport
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							Keyboard[0]= 1 /*smartest blob*/;	i8042OutPtr=1;
							goto kb_setirq;
							break;
						case 0xC0:		//read output data (ossia MAchineSettings, DIVERSI da XT!
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							Keyboard[0]=MachineSettings;
							i8042OutPtr=1;
// forse no						goto kb_setirq;
							break;
						case 0xC1:		//read output data (ossia MAchineSettings 0..3
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							Keyboard[0]=MachineSettings << 4;
							i8042OutPtr=1;
// forse no						goto kb_setirq;
							break;
						case 0xC2:		//read output data (ossia MAchineSettings, 4..7
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							Keyboard[0]=MachineSettings & 0xf0;
							i8042OutPtr=1;
// forse no						goto kb_setirq;
							break;
						case 0xD0:		// read output port
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							Keyboard[0]=i8042Output;		i8042OutPtr=1;
// forse no						goto kb_setirq;
							break;
						case 0xD1:		// write output port
/*							if(KBCommand & B8(00010000))			// se disabilitata
								i8042Output=t1;
							else
								i8042Output=t1;				// boh...*/
							// e in teoria b0 è RESET CPU, fisso :D ovviamente
							KBStatus |= KB_COMMAND | KB_INPUTFULL;		// command, data input full
//							Keyboard[0]=i8042RegW[1];		// bah ok
//							i8042OutPtr=1;
							break;
						case 0xD2:		// https://stanislavs.org/helppc/8042.html
							goto kb_echo;
							break;
            case 0xDF: //enable A20 (hp vectra) / (quadtel?)  da granite/smartestblob
//							i8042Output |= 2;
							break;
            case 0xDD: //disable A20 (hp vectra) / (quadtel?)
//							i8042Output &= ~2;
							break;
						case 0xE0:		// read test lines
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							if(!(KBStatus & KB_ENABLED) /*KBRAM[0] & B8(00000100)*/)
								Keyboard[0]=0;		// ovvio
							else
								Keyboard[0]=rand() & 3;		// ideuzza di smartestblob :)
							i8042OutPtr=1;
							goto kb_setirq;
							break;
						case 0xEE:     //echo, ora non lo vedo nella pagina 8042 ma ok
kb_echo:
							Keyboard[0]=i8042RegW[1];
							KBStatus |= KB_OUTPUTFULL | KB_INPUTFULL | KB_COMMAND;				// command, data input full, has data output
							i8042Output |= 0b00010000;
							i8042OutPtr=1;

kb_setirq:
							if(KBCommand & 0b00000001) {     //..e se interrupt attivi...
							//    KBDIRQ=1;
								i8259IRR |= 0b00000010;
								}
							break;
						default:
							if(i8042RegW[1]>=0x00 && i8042RegW[1]<0x20) {		// read ram
kb_readram:
								//KBRAM[i8042RegW[1] & 0x1f]
								KBStatus |= KB_COMMAND | KB_INPUTFULL;		// command
//								goto kb_setirq;
								}
							else if(i8042RegW[1]>=0x20 && i8042RegW[1]<0x40) {		// alias AMI
								goto kb_readram;
								}
							else if(i8042RegW[1]>=0x60 && i8042RegW[1]<0x80) {		// write ram
kb_writeram:
								//KBRAM[i8042RegW[1] & 0x1f]
								KBStatus |= KB_COMMAND | KB_INPUTFULL;		// command
//								goto kb_setirq;
								}
							else if(i8042RegW[1]>=0x40 && i8042RegW[1]<0x60) {		// alias AMI
								goto kb_writeram;
								}
							else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {		// pulse output pin
								KBStatus |= KB_COMMAND | KB_INPUTFULL;		// command,data input full, 
		// FE equivale a pulsare il bit 0 ossia RESET!

								if(!(t1 & 1)) {
									CPUPins |= DoReset;
		//			debug=0;
#ifdef _DEBUG
#ifdef DEBUG_KB
					_lwrite(spoolFile,"*------ RESET!!\r\n",17);
#endif
#endif

									i8042RegW[1]=0;
									KBStatus &= ~KB_INPUTFULL;


		//			ram_seg[0x472]=0x34;
		//			ram_seg[0x473]=0x12; forzo warm boot STRAPUTTANO dio che non muore insieme a crosetto
									}

								}
							break;
	          }
//        KBStatus &= ~B8(00000010);    // vabbe' :)
        break;
        }
#endif
      break;

    case 7:        //   70-7F CMOS RAM/RTC (Real Time Clock  MC146818)
      // SECONDO PCXTBIOS, il clock è a 240 o 2c0 o 340 ed è mappato con tutti i registri... pare!
			// https://web.archive.org/web/20160314084340/https://www.bioscentral.com/misc/cmosmap.htm
			// anche (TD3300, superXT) Port 070h
			// Controls the wait state for board RAM, ROM, I/O and also bus RAM and I/O. See (page B-1, B-2) from the UX-12 manual.
      t &= 0x1;
      switch(t) {
        case 0:     // il bit 7 attiva/disattiva NMI  https://www.singlix.com/trdos/archive/pdf_archive/real-time-clock-nmi-enable-paper.pdf
          i146818RegR[0]=i146818RegW[0]=t1;
  //        time_t;
					//PCAT manda 0x8d qua al boot; poi scrive b7 e legge da 71 (credo CMOS), ed ev. (credo) scrive b7 e 0c
					// poi 8f e 06
          break;
        case 1:
          i146818RegW[t]=t1;
          switch(i146818RegW[0] & 0x3f) {
            case 0:
              currentTime.sec=t1;
              break;
              // in mezzo c'è Alarm...
            case 2:
              currentTime.min=t1;
              break;
            case 4:
              currentTime.hour=t1;
              break;
            case 6:
            // 6 è day of week... si può scrivere??
              currentDate.weekday=t1;
              break;
            case 7:
              currentDate.mday=t1;
              break;
            case 8:
              currentDate.mon=t1;
              break;
            case 9:
              currentDate.year=t1;
              break;
            case 10:
              t1 &= 0x7f;     // vero hardware!
              t1 |= i146818RAM[10] & 0x80;
              goto writeRegRTC;
              break;
            case 11:
              if(t1 & 0x80)
                i146818RAM[10] &= 0x7f;
              goto writeRegRTC;
              break;
            case 12:
              t1 &= 0xf0;     // vero hardware!
              goto writeRegRTC;
              break;
            case 13:
              t1 &= 0x80;     // vero hardware!
              goto writeRegRTC;
              break;

            case 14:			// diag in GLATick...
#ifdef PCAT
            case 15:			// flag, Award scrive 6			https://www.rcollins.org/ftp/source/pmbasics/tspec_a1.l1
#endif

            default:      // in effetti ci sono altri 4 registri... RAM da 14 in poi 
writeRegRTC:            
              i146818RAM[i146818RegW[0] & 0x3f] = t1;

              break;
            }
          break;
        }
      break;

    case 8:
      t &= 0xf;
#ifndef PCAT
      switch(t) {
				case 0:			// POST diag code (v. anche 300...
					//if(t==0x80 /*t>=0x108 && t<=0x10f*/) {      // diag display, uso per led qua!
    // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
					t1 &= 0x7;   //[per ora tolti xché usati per debug! 
//        LED1 = t1 & 1;
//        LED2 = t1 & 2 ? 1 : 0;
//        LED3 = t1 & 4 ? 1 : 0;
					break;
				case 1:			// 64K page number DMA    https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
					DMApage[2]=t1 & 0xf;
					break;
				case 2:
					DMApage[3]=t1 & 0xf;
					break;
				case 3:
					DMApage[1]=t1 & 0xf;
					break;
				case 7:
					DMApage[0]=t1 & 0xf;
					break;
				case 9:
					DMApage[6]=t1 & 0xf;
					break;
				case 0xa:
					DMApage[7]=t1 & 0xf;
					break;
				case 0xb:
					DMApage[5]=t1 & 0xf;
					break;
				case 0xf:		// refresh page register...
					break;
				}
#else
      switch(t) {
				case 0:			// POST diag code (v. anche 300...
					//if(t==0x80 /*t>=0x108 && t<=0x10f*/) {      // diag display, uso per led qua!
    // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
		// https://blog.theretroweb.com/2024/01/20/award-bios-beep-and-post-codes-list/#Version_33

					DIAGPort=t1;
          
#if 0
          // ev....
        char myBuf[80];
        sprintf(myBuf,"DIAG: %02X",DIAGPort);
        setTextColor(BRIGHTRED);
        LCDXY(16,20);
        gfx_print(myBuf);
          
#endif

					t1 &= 0x7;   //[per ora tolti xché usati per debug! 
//        LED1 = t1 & 1;
//        LED2 = t1 & 2 ? 1 : 0;
//        LED3 = t1 & 4 ? 1 : 0;
					break;
				case 1:			// 64K page number DMA    https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
					DMApage[2]=t1;
					break;
				case 2:
					DMApage[3]=t1;
					break;
				case 3:
					DMApage[1]=t1;
					break;
				case 4:
					DMApage2[0]=t1;
					break;
				case 5:
					DMApage2[1]=t1;
					break;
				case 6:
					DMApage2[2]=t1;
					break;
				case 7:
					DMApage[0]=t1;
					break;
				case 8:
					DMApage2[3]=t1;
					break;
				case 9:
					DMApage[6]=t1;
					break;
				case 0xa:
					DMApage[7]=t1;
					break;
				case 0xb:
					DMApage[5]=t1;
					break;
				case 0xc:
					DMApage2[4]=t1;
					break;
				case 0xd:
					DMApage2[5]=t1;
					break;
				case 0xe:
					DMApage2[6]=t1;
					break;
				case 0xf:		// refresh page register...
					DMArefresh=t1;
					break;
				}
#endif
      break;

    case 0x9:        // SuperXT TD3300A
			t &= 0xf;
			switch(t) {
				case 0:
			//Controls the clock speed using the following:
			//If port 90h == 1, send 2 (0010b) for Normal -> Turbo
			//If port 90h == 0, send 3 (0011b) for Turbo -> Normal
					CPUDivider=2000000L/CPU_CLOCK_DIVIDER;		// finire! v. 61h sopra pure
					break;
				case 2:		//https://wiki.osdev.org/Non_Maskable_Interrupt
			// linea A20 in alcuni computer http://independent-software.com/operating-system-development-enabling-a20-line.html
//0 	Alternate hot reset, 1 	Alternate gate A20,2 	Reserved,3 	Security Lock,4* 	Watchdog timer status,5 	Reserved,6 	HDD 2 drive activity,7 	HDD 1 drive activity
#ifdef PCAT
					if(t1 & 2)
            i8042Output=t1;
					else
						;
#endif
					break;
				case 9:
					// PCAT , AMI scrive 0 al boot
					break;
				}
			break;

    case 0xa:        // PIC 8259 controller #2 su AT; machine settings su XT
#ifdef PCAT
      t &= 0x1;
      switch(t) {
        case 0:     // 0x20 per EOI interrupt, 0xb per "chiedere" quale... subito dopo c'è lettura a 0x20
          i8259Reg2W[0]=t1;      // 
					if(t1 & 0b00010000) {// se D4=1 qua (0x10) è una sequenza di Init,
						i8259OCW2[2] = 2;		// dice, default su lettura IRR
//						i8259RegR[0]=0;
						// boh i8259IMR=0xff; disattiva tutti
						i8259ICW2[0]=t1;
						i8259ICW2Sel=2;		// >=1 indica che siamo in scrittura ICW
						}
					else if(t1 & 0b00001000) {// se D3=1 qua (0x8) è una richiesta
						// questa è OCW3 
						i8259OCW2[2]=t1;
						switch(i8259OCW2[2] & 0b00000011) {		// riportato anche sopra, serve (dice, in polled mode)
							case 2:
								i8259Reg2R[0]=i8259IRR2;
								break;
							case 3:
								i8259Reg2R[0]=i8259ISR2;
								break;
							default:
								break;
							}
						// b2 è poll/nopoll...
						}
          else {
						// questa è OCW2 "di lavoro" https://k.lse.epita.fr/internals/8259a_controller.html
						i8259OCW2[1]=t1;
						switch(i8259OCW2[1] & 0b11100000) {
							case 0b00000000:// rotate pty in automatic (set clear
								break;
							case 0b00100000:			// non-spec, questo pulisce IRQ con priorità + alta in essere (https://rajivbhandari.wordpress.com/wp-content/uploads/2014/12/unitii_8259.pdf
//						i8259ISR=0;		// MIGLIORARE! pulire solo il primo
								for(i=1; i; i<<=1) {
									if(i8259ISR2 & i) {
										i8259ISR2 &= ~i;
										break;
										}
									}
//				      CPUPins &= ~DoIRQ;		// SEMPRE! non serve + v. di là
								break;
							case 0b01000000:			// no op
								break;
							case 0b01100000:			// 0x60 dovrebbe indicare uno specifico IRQ, nei 3bit bassi 0..7
								i8259ISR2 &= ~(1 << (t1 & 7));
								break;
							case 0b10000000:			// rotate pty in automatic (set
								break;
							case 0b10100000:			// rotate pty on non-specific
								break;
							case 0b11000000:			// set pty
								break;
							case 0b11100000:			// rotate pty on specific
								break;
							}

						}
          break;
        case 1:
          i8259Reg2R[1]=i8259Reg2W[1]=t1;      //  
					switch(i8259ICW2Sel) {
						case 2:
							i8259ICW2[1]=t1;			// qua c'è base address per interrupt table (8 cmq)
							i8259ICW2Sel++;
							break;
						case 3:
							i8259ICW2Sel++;
							i8259ICW2[2]=t1;			
							if(!(i8259ICW2[0] & 0b00000001)) 		// ICW4 needed?
								i8259ICW2Sel=0;
							break;
						case 4:
							i8259ICW2[3]=t1;
							i8259ICW2Sel=0;
							break;
						default:
							i8259IMR2=t1;			// 0xbd .. RTC e hd 1 
							i8259OCW2[0]=t1;
							break;
						}

          break;
        }
#else
			i=t;		//DEBUG
					// questo BLOCCA NMI inoltre! (pcxtbios; anche AMI ci scrive 00
#endif

      break;

#ifdef PCAT
		case 0xc:		// DMA 2	(second Direct Memory Access controller 8237)  https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
		case 0xd:		//  Phoenix 286 scrive C0 a D6 all'inizio
      t &= 0x1f;
			t >>=1 ;
			switch(t) {
				case 0:
				case 2:
				case 4:
				case 6:
					t /= 2;
					if(!i8237FF2)
						i8237DMAAddr2[t]=(i8237DMAAddr2[t] & 0xff00) | t1;
					else
						i8237DMAAddr2[t]=(i8237DMAAddr2[t] & 0xff) | ((uint16_t)t1 << 8);
					i8237DMACurAddr2[t]=i8237DMAAddr2[t];
					i8237FF2++;
					i8237FF2 &= 1;
		      i8237Reg2R[t]=i8237Reg2W[t]=t1;
					break;
				case 1:
				case 3:
				case 5:
				case 7:
					t /= 2;
					if(!i8237FF2)
						i8237DMALen2[t]=(i8237DMALen2[t] & 0xff00) | t1;
					else
						i8237DMALen2[t]=(i8237DMALen2[t] & 0xff) | ((uint16_t)t1 << 8);
					i8237DMACurLen2[t]=i8237DMALen2[t];
					i8237FF2++;
					i8237FF2 &= 1;
		      i8237Reg2R[t]=i8237Reg2W[t]=t1;
					break;
				case 0x8:			// Command/status (reg.8
		      /*i8237RegR[8]=*/i8237Reg2W[8]=t1;
					break;
				case 0xa:			// set/clear Mask
					if(t1 & 0b00000100)
						i8237Reg2W[15] |= (1 << (t1 & 0b00000011));
					else
						i8237Reg2W[15] &= ~(1 << (t1 & 0b00000011));
					i8237Reg2R[15]=i8237Reg2W[15];
		      i8237Reg2R[0xa]=i8237Reg2W[0xa]=t1;
					break;
				case 0xb:			// Mode
					i8237Mode2[t1 & 0b00000011]= t1 & 0b11111100;
					i8237DMACurAddr2[t1 & 0b00000011]=i8237DMAAddr2[t1 & 0b00000011];			// bah credo, qua
					i8237DMACurLen2[t1 & 0b00000011]=i8237DMALen2[t1 & 0b00000011];
		      i8237Reg2R[0xb]=i8237Reg2W[0xb]=t1;

					i8237Reg2R[8] |= ((1 << t1 & (0b00000011)) << 4);  // request??... boh

					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					i8237FF2=0;
		      i8237Reg2R[0xc]=i8237Reg2W[0xc]=t1;
					break;
				case 0xd:			// Reset
					memset(i8237Reg2R,0,sizeof(i8237Reg2R));
					memset(i8237Reg2W,0,sizeof(i8237Reg2W));
					i8237Reg2R[15]=i8237Reg2W[15] = 0b00001111;
					i8237Reg2R[8]=0b00000000;
		      i8237Reg2R[0xd]=i8237Reg2W[0xd]=t1;
					break;
				case 0xe:			//  Clear Masks
					i8237Reg2R[15]=i8237Reg2W[15] &= ~0b00001111;
					break;
				case 0xf:			//  Masks
					i8237Reg2R[15]=i8237Reg2W[15] = (t1 & 0b00001111);
					break;
				default:
		      i8237Reg2R[t]=i8237Reg2W[t]=t1;
					break;
				}
			break;
#endif

    case 0xe:        // TD3300A , SuperXT
			//Controls the bank switching for the on-board upper memory, as mentioned in this post.
			// if port 0Eh == 0, addresses 2000:0000-9000:FFFF are mapped to the low contiguous RAM
			// if port 0Eh == 1, addresses 2000:0000-9000:FFFF are mapped to the upper RAM (if installed)
			t &= 0xf;
			switch(t) {
				case 9:
					// PCAT scrive 0 al boot
					break;
				}
			break;
    
    case 0xf:  // F0 = coprocessor
			// AWARD 286 scrive 00 a F1
      break;

  // 108 diag display opp. 80
  // 278-378 printer
  // 300 = POST diag
  // 2f8-3f8 serial

    case 0x10:
      t &= 0x7;
      switch(t) {
				case 0:			// POST diag code 0x108..10f (v. anche 300... o 80
    // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
					break;
				}
			break;

#ifdef PCAT
    case 0x17:      // 170-177   // hard disc 1 su AT		
    case 0x1f:      // 1f0-1f7   // hard disc 0 su AT		http://www.techhelpmanual.com/897-at_hard_disk_ports.html   http://www.techhelpmanual.com/892-i_o_port_map.html
			if(t & 0x80)
				hdDisc &= ~2;
			else
				hdDisc |= 2;
      t &= 0x7;
      switch(t) {
        case 0:			// data  MA è 16bit!
/*					sectorData[HDFIFOPtrW]=t1;
					HDFIFOPtrW++;
					if(HDFIFOPtrW >= SECTOR_SIZE) {
						msdosHDisk=((uint8_t*)MSDOS_HDISK)+getHDiscSector(disk)*SECTOR_SIZE;
						memcpy(msdosHDisk,&sectorData,SECTOR_SIZE);
						HDFIFOPtrW=0;
						}
					hdState &= ~HD_AT_STATUS_BUSY;		// tolgo busy :)*/
          break;
        case 1:			// precomp o anche feature se comando 0xB0
					hdPrecomp=t1;
					// hdSmart=
          break;
        case 2:			// sector count
					numSectors=t1;
          break;
        case 3:			// sector number o LBA-lo  FINIRE!
					if(hdSector<=HD_SECTORS_PER_CYLINDER)
//					if(LBA)
//						;
						hdSector=t1    -1;
					else {
						hdError=0b00000100;		// aborted
						hdState |= HD_AT_STATUS_ERROR;		// error
						hdSector=0;
						}
          break;
        case 4:			// cyl high [FROCIO pagina errata]  o LBA-mid
					hdCylinder=MAKEWORD(t1,HIBYTE(hdCylinder));
					if(hdCylinder>HD_CYLINDERS) {
						hdError=0b00000100;		// aborted
						hdState |= HD_AT_STATUS_ERROR;		// error
						hdCylinder=0;
						}
          break;
        case 5:			// cyl low  o LBA-hi  FINIRE!
//					if(LBA)
//						;
					hdCylinder=MAKEWORD(LOBYTE(hdCylinder),(t1 & 3));
					if(hdCylinder>HD_CYLINDERS) {
						hdError=0b00000100;		// aborted
						hdState |= HD_AT_STATUS_ERROR;		// error
						hdCylinder=0;
						}
          break;
        case 6:			// drive & head
					hdHead=t1 & 0xf;
					if(hdHead>HD_HEADS) {
						hdError=0b00000100;		// aborted
						hdState |= HD_AT_STATUS_ERROR;		// error
						}
					if(t1 & 0x10)
						hdDisc |= 1;
					else
						hdDisc &= ~1;
					// drive è 0xa opp 0xb... high nibble! il controller # è nell'address 170 1f0
					// 1 	LBA 	1 	Drive 	Head
					if(!hdDisc)
						hdState = HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK;		// sì xché sembra usarlo anche come "test" ogni tanto
					else
						hdState = 0b00000000;
					if(t1 & 0b01000000)		// se LBA...
						;
          break;
        case 7:
					switch(t1 >> 4) {
						case HD_AT_RESTORE>>4:		// restore
							hdCylinder=0;
							hdHead=0;
							hdError=0;
							hdState=HD_AT_STATUS_BUSY | HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK;		// busy, ok, seek ok

endHDcommandWithIRQ:
							if(!(hdControl & 0b00000010))
								setHDIRQ(SLOW_HD_DELAYED_IRQ /* maggiore per phoenix*/);
							break;
						case HD_AT_READ_DATA>>4:		// read
							encodeHDiscSector(hdDisc);

							HDFIFOPtrR=0;
							hdState |= HD_AT_STATUS_BUSY | HD_AT_STATUS_DRQ;		// busy, aspetta data
							hdState &= ~(HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK);		// not ready
							goto endHDcommandWithIRQ;
							break;
						case HD_AT_SEEK>>4:		// seek
							hdState &= ~HD_AT_STATUS_READYTOSEEK;
							hdState |= HD_AT_STATUS_BUSY;
// attesa...
							hdState |= HD_AT_STATUS_READYTOSEEK;
							hdState &= ~(HD_AT_STATUS_BUSY | HD_AT_STATUS_ERROR);	// ok
							hdError=0;
							goto endHDcommandWithIRQ;
							break;
						case HD_AT_WRITE_DATA>>4:		// write
							msdosHDisk=((uint8_t*)MSDOS_HDISK)+getHDiscSector(hdDisc,0)*SECTOR_SIZE;
							HDFIFOPtrW=0;
							hdState |= HD_AT_STATUS_BUSY | HD_AT_STATUS_DRQ;		// busy, aspetta data
							hdState &= ~0b01010001;		// not ready, ok
							hdError=0;
							goto endHDcommandWithIRQ;
							break;
						case 4:		// read verify
							encodeHDiscSector(hdDisc);
							// è una verifica tutta interna! confronta BYTE con ECC ecc

							HDFIFOPtrR=0;
							hdState |= HD_AT_STATUS_BUSY;		// busy
							hdState &= ~(HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK);		// not ready
// :)
							hdState &= ~(HD_AT_STATUS_BUSY | HD_AT_STATUS_ERROR);		// not busy, ok
							hdState |= HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK;		// ready
							hdError=1;		// v. award
							goto endHDcommandWithIRQ;
							break;
						case HD_AT_WRITE_MULTIPLE>>4:		// read, write multiple
							break;
						case HD_AT_FORMAT>>4:		// format, secondo granite è Identify!!
							// aspettare 256 con info per format, come in Write direi
							HDFIFOPtrW=0;
							hdState |= HD_AT_STATUS_BUSY | HD_AT_STATUS_DRQ;		// busy, aspetta data
							hdState &= ~(HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK | HD_AT_STATUS_ERROR);		// not ready, ok
							hdError=0;

							goto endHDcommandWithIRQ;

							break;
						case HD_AT_DIAGNOSE>>4:		// diagnose/set parameters
							if(!(t1 & 1)) {		// diagnose
								if(!hdDisc) {
									hdState=HD_AT_STATUS_BUSY | HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK;		// busy, ok, seek ok
									hdPrecomp=32;		//dice
									hdError=1;
									goto endHDcommandWithIRQ;		// dice anche IRQ SOLO se ok, no se errore...
									}
								else {
									hdState |= 0b00000000;		// error qua??
									hdError=0;
									}
								}
							else {		// set parm
								hdState=HD_AT_STATUS_BUSY | HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK;		// busy, ok, seek ok
								hdError=0;
								goto endHDcommandWithIRQ;
								}

							break;
						case HD_AT_DEVICE_CONTROL>>4:		// device control  https://www.mjm.co.uk/how-hard-disk-ata-registers-work.html

							break;
						case HD_AT_IDENTIFY>>4:		// identify ecc https://www.os2museum.com/wp/whence-identify-drive/
							//https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ata/ns-ata-_identify_device_data
							switch(t1 & 0xf) {
								case 0xc:			// IDentify
									if(!hdDisc) {
										sectorData[0]=0b01000010;	sectorData[1]=0b00000001;		// General configuration
/*							General configuration bit-significant information:
15 0 reserved for non-magnetic drives
14 1=format speed tolerance gap required
13 1=track offset option available
12 1=data strobe offset option available
11 1=rotational speed tolerance is > 0,5%
10 1=disk transfer rate > 10 Mbs
 9 1=disk transfer rate > 5Mbs but <= 10Mbs
 8 1=disk transfer rate <= 5Mbs
 7 1=removable cartridge drive
 6 1=fixed drive
 5 1=spindle motor control option implemented
 4 1=head switch time > 15 usec
 3 1=not MFM encoded
 2 1=soft sectored
 1 1=hard sectored
 0 0=reserved*/
										memset(&sectorData[20],0,54+40-20);
										sectorData[2]=LOBYTE(HD_CYLINDERS);	sectorData[3]=HIBYTE(HD_CYLINDERS);		// cilindri
										sectorData[4]=LOBYTE(HD_CYLINDERS);	sectorData[5]=HIBYTE(HD_CYLINDERS);		// reserved ma v. HDINFO.EXE
										sectorData[6]=LOBYTE(HD_HEADS);	sectorData[7]=HIBYTE(HD_HEADS);		// testine
										sectorData[8]=LOBYTE(HD_SECTORS_PER_CYLINDER*SECTOR_SIZE);	sectorData[9]=HIBYTE(HD_SECTORS_PER_CYLINDER*SECTOR_SIZE);		// byte unformatted per track
										sectorData[10]=LOBYTE(SECTOR_SIZE);	sectorData[11]=HIBYTE(SECTOR_SIZE);		// dimensione settore
										sectorData[12]=LOBYTE(HD_SECTORS_PER_CYLINDER);	sectorData[13]=HIBYTE(HD_SECTORS_PER_CYLINDER);		// settori per cilindro
			//							sectorData[14]=LOBYTE(0);	sectorData[15]=HIBYTE();		// byte in inter-sector gaps
			//							sectorData[16]=LOBYTE(0);	sectorData[17]=HIBYTE();		// byte in sync fields
										sectorData[42]=LOBYTE(64/*vabbe'!*/);	sectorData[43]=HIBYTE(0);		// dim. buffer controller, *512 e /1024
										sectorData[44]=LOBYTE(4/*boh*/);	sectorData[45]=HIBYTE(0);		// byte ECC
										sectorData[94]=LOBYTE(32/*boh*/);	sectorData[95]=HIBYTE(0);		// settori/interrupt per letture multiple
										sectorData[96]=LOBYTE(0);	sectorData[97]=HIBYTE(0);		// double-word transfer capable (1=sì)
										sectorData[120]=LOBYTE(LOWORD(HD_SECTORS_PER_CYLINDER*HD_CYLINDERS*HD_HEADS));	sectorData[121]=HIBYTE(LOWORD(HD_SECTORS_PER_CYLINDER*HD_CYLINDERS*HD_HEADS));		// settori in modo LBA28
										sectorData[122]=LOBYTE(HIWORD(HD_SECTORS_PER_CYLINDER*HD_CYLINDERS*HD_HEADS));	sectorData[123]=HIBYTE(HIWORD(HD_SECTORS_PER_CYLINDER*HD_CYLINDERS*HD_HEADS));		// settori in modo LBA28
										sectorData[166]=0b00000000;	sectorData[167]=0b00000000;		// B10=1 se LBA48
										sectorData[176]=0b00000001;	sectorData[177]=0b00000000;		// DMA mode: lo-byte quelli supportati e hi-byte quelli attivi (dovrebbero essere uguali, tipo 3 e 3 per Mode 1 e 2  https://wiki.osdev.org/ATA_PIO_Mode#Interesting_information_returned_by_IDENTIFY
										// OCCHIO se usiamo poi un BIOS che usa davvero DMA!!
										sectorData[186]=0b00000000;	sectorData[187]=0b00000000;		// se Master drive, B11 indica cavo 80 pin presente
										sectorData[200]=LOBYTE(LOWORD(0));	sectorData[201]=HIBYTE(LOWORD(0));		// settori in modo LBA48
										sectorData[202]=LOBYTE(HIWORD(0));	sectorData[203]=HIBYTE(HIWORD(0));		// settori in modo LBA48
										sectorData[204]=LOBYTE(LOWORD(0));	sectorData[205]=HIBYTE(LOWORD(0));		// settori in modo LBA48
										sectorData[206]=LOBYTE(HIWORD(0));	sectorData[207]=HIBYTE(HIWORD(0));		// settori in modo LBA48
		sectorData[212];		// multiple sector per logical sector  https://f.osdev.org/viewtopic.php?t=40956

										strcpyhl(&sectorData[20],"0123456789");		// 20 char
										{char myBuf[128];
										sprintf(myBuf,"8086win HD %uMB (Dario's Automation)",
											HD_SECTORS_PER_CYLINDER*HD_CYLINDERS*HD_HEADS*SECTOR_SIZE/1048576L);		// 40 char
										strcpyhl(&sectorData[54],myBuf);		// 40 char
										}
										strcpyhl(&sectorData[46],"0001");		// 8 char

										HDFIFOPtrR=0;
										hdState |= HD_AT_STATUS_BUSY | HD_AT_STATUS_DRQ;		// busy, aspetta data
										hdState &= ~(HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK);		// not ready
										hdError=0;
										goto endHDcommandWithIRQ;
										}
									else {
										hdState &= ~(HD_AT_STATUS_READY | HD_AT_STATUS_READYTOSEEK);		// not ready
										hdState |= HD_AT_STATUS_ERROR;		// error
										hdError=99;
										}
									break;
								case 0x7:			// flush cache
								case 0xf:			// flush cache boh
									hdState |= HD_AT_STATUS_BUSY;		// busy


									break;
								case 0x4:			// read buffer
									break;
								case 0x8:			// write buffer
									break;
								case 0x0:			// raw ESDI command
									break;
								}
							break;
						default:
							break;
						}
          break;
				}
			HDContrRegR[t]=HDContrRegW[t]=t1;

      break;
#endif
    
    case 0x20:    //200-207   joystick
      break;
      
    case 0x22:    //220-223   Sound Blaster; -22F se Pro
      break;
      
    case 0x24:    // RTC SECONDO PCXTBIOS, il clock è a 240 o 2c0 o 340 ed è mappato con tutti i registri... pare!
      t &= 0xf;
      switch(t) {
        case 0:
          break;
        case 1:     // 
          currentTime.sec=t1;
          break;
        case 2:
          currentTime.min=t1;;
          break;
        case 3:
          currentTime.hour=t1;
          break;
        case 4:
          currentDate.mday=t1;
          break;
        case 5:
          currentDate.mon=t1;
          break;
        case 6:
          currentDate.year=t1;
          break;
        default:
          break;
        }
      break;

    case 0x30:    //300  POST diag code (anche 80??, AWARD 286 lo usa (stesso che a 80h
			i=1;
      break;

			/*scrive a 1   reset   40
				legge a 1		e aspetta b1 (02) = 0			// 017c

				scrive a 2   2
				scrive a 3   2
				legge a 1 e aspetta 0d								// 0573

				scrive a 0  111 00000
				scrive a 0  5 byte (0), HDFIFOPtrW=6
				legge a 1 e aspetta 20  NO ASPETTA b0 (01)=0				// 0591
				073b

				scrive...
				legge a 1 e aspetta 0d

			*/

#ifdef ROM_HD_BASE
    case 0x32:    // 320- (hard disk su XT  http://www.techhelpmanual.com/898-xt_hard_disk_ports.html   https://forum.vcfed.org/index.php?threads/ibm-xt-i-o-addresses.1237247/
      t &= 3;
      switch(t) {
        case 0:			// WRITE DATA
					if(!HDFIFOPtrW) {
						HDContrRegW[0]=t1;
						HDFIFOPtrW=1; HDFIFOPtrR=0;
						}
					else {
						HDFIFOW[HDFIFOPtrW++] = t1;

						if(HDFIFOPtrW==6) {
							switch(HDContrRegW[0] & HD_XT_COMMAND_MASK) {
								case HD_XT_INITIALIZE:
									hdState &= ~0b00001111;			// 
									break;
								default:
									hdState &= ~0b00000101;			// handshake, e passo a Status
									hdState |= 0b00000010;		// passo a write
		//						hdTimer=100;
									break;
								}

							}

						else {		// comandi con dati a seguire (tipo 0xC
							switch(HDContrRegW[0] & HD_XT_COMMAND_MASK) {
								case HD_XT_INITIALIZE:
									if(HDFIFOPtrW==14) {		// 8 byte, v.

												// restituire 4 in Error code poi (però tanto non lo chiede, non arriva SENSE...
		//										HDFIFO[1]=4;

										hdState &= ~0b00100101;			// handshake, e passo a status, no IRQ
										hdState |= 0b00000010;		// passo a write
//											goto endHDcommand;
										}

									break;
								default:
									break;

								}

							}
						}

          break;
        case 1:     // RESET
					HDContrRegW[1]=t1;
					memset(HDContrRegR,0,sizeof(HDContrRegR));		// reset
					memset(HDContrRegW,0,sizeof(HDContrRegW));
					HDFIFOPtrR=HDFIFOPtrW=0;
					hdHead=0;
					hdCylinder=0;
					hdSector=0;
					hdState=hdControl=0;
					hdState=0b00010101;			// mah; e basare DMA su hdControl<1>

//					debug=1;

          break;
        case 2:			// SELECT
					/*Writing to port 322 selects the WD 1002S-WX2,
						sets the Busy bit in the Status Register and prepares
						it to receive a command. When writing to port
						322, the data byte is ignored.*/
					hdState=0b00001101;			// ESIGE BSY e C/D (strano.. sarà doc sbagliato ancora); e basare DMA su hdControl<1>
					HDFIFOPtrW=HDFIFOPtrR=0;
					HDContrRegW[2]=t1;
          break;
        case 3:			// WRITE DMA AND INTERRUPT MASK REGISTER
					hdControl=HDContrRegW[3]=t1;
          break;
				}
			break;
#endif

    case 0x37:    // 370-377 (secondary floppy, AT http://www.techhelpmanual.com/896-diskette_controller_ports.html); 378-37A  LPT1
      t &= 3;
			LPT[0][t]=t1;		// per ora, al volo!
      switch(t) {
        case 0:
//  #ifndef ILI9341
//          LATE = (LATE & 0xff00) | t1;
//          LATGbits.LATG6 = t1 & 0b00010000) ? 1 : 0;   // perché LATE4 è led...
//  #endif
          break;
        case 2:    // qua c'è lo strobe
//          LATGbits.LATG7 = t1 & 0b00000001) ? 1 : 0;   // strobe, FARE gli altri :)
          break;
        }
      break;
    
#ifdef MDA
    case 0x3b:    // 3b0-3bf  MDA/Hercules https://www.seasip.info/VintagePC/mda.html
      t &= 0xf;
      switch(t) {
        case 0:     // 0, 2, 4, 6: register select
        case 2:
        case 4:
        case 6:
          MDAreg[4]=t1;
          break;
        case 1:     // 
        case 3:
        case 5:
        case 7:
          MDAreg[5]=i6845RegR[MDAreg[4] % 18]=i6845RegW[MDAreg[4] % 18]=t1;		// cmq dice che è write-only
          break;
        case 0x8:
          // Mode
          MDAreg[8]=t1;
          break;
        case 0xa:
          // Status
          break;
        case 0xc:
					MDAreg[0xc]=LPT[2][0]=t1;		// per ora, al volo!
          break;
        case 0xe:
					MDAreg[0xe]=LPT[2][2]=t1;
					break;
        default:
          MDAreg[t]=t1;
          break;
        }
      break;
#endif

#ifdef VGA			// http://www.osdever.net/FreeVGA/vga/portidx.htm
    case 0x3b:    // 
      t &= 0xf;
      switch(t) {
				case 0xf:
					// Trident scrive 03
					break;
				case 0x4:
					// Trident scrive 34, poi dopo 17
// compatibilità??          MDAreg[4]=t1;
					break;
				case 0x5:
					// Trident scrive a3
					break;
				case 0x8:
					// Trident scrive A0
					break;
				}
			break;
    case 0x3c:    // 3c0-3cf  VGA http://wiki.osdev.org/VGA_Hardware#Port_0x3C0
      t &= 0xf;
			// http://www.osdever.net/FreeVGA/vga/vgareg.htm tutti i registri...
      switch(t) {
				case 0:		// write index & data actl reg
					if(!VGAFF)
						VGAptr=t1;
					else {
						VGAactlReg[VGAptr & 0x1f]=t1;
						if(VGAptr>=0 && VGAptr<=15) {
							Colori[VGAptr]=coloreDaPaletteVGA(t1);
							applyPalette();
							}
						}
					VGAFF = !VGAFF;
          break;
        case 0x1:		// qua dice che c'è la palette, indici da 0 a F; a 10h attribute mode control; ecc.
					//VGAPalette[];
//					Colori[]=coloreDaPaletteVGA(t1);
          break;
        case 0x2:		
// ovviamente è un altra cosa...          
					VGAreg[2]=t1;		// misc output... ; b0 rimappa da 3Dx a 3Bx...    63h 67h  c3h e3h
          break;
        case 0x3:
          //VGAreg[t]=t1;		// 
					// Trident scrive 01 qua alla partenza...
          break;
        case 0x4:
          VGAreg[4]=t1;		// Trident scrive 07
          break;
        case 0x4+1:
          VGAseqReg[VGAreg[0x4] & 0x7]=t1;		// http://www.o3one.org/hwdocs/vga/vga_app.html 
          break;
        case 0x6:
          VGAreg[6]=t1;		// DAC register
          break;
        case 0x8:
          VGAreg[8]=t1;		// write DAC register index
          break;
        case 0x9:
          VGAreg[9]=t1;		// write DAC register (indexed da 3c8
					VGADAC;
          break;
        case 0xe:
          VGAreg[0xe]=t1;		// 
          break;
        case 0xf:			// write register graph
          VGAgraphReg[VGAreg[0xe] & 0xf]=t1;		// 
          break;
        default:
          VGAreg[t]=t1;
          break;
        }
      break;
    case 0x3d:    // 
      t &= 0xf;
      switch(t) {
        case 0x4:
          CGAreg[4]=t1;		// compatibilità CGA...
          break;
        case 5:     // http://www.osdever.net/FreeVGA/vga/vgareg.htm
					// CRTC protect è b7 del registro 11h
//					if(!(VGAcrtcReg[0x11] & 0x80))
						VGAcrtcReg[CGAreg[4] & 31]=t1;
          break;
        case 9:     // questo arriva per la palette selection
          CGAreg[9]=t1;
          break;
				default:
// boh?          CGAreg[t]=t1;
          break;
				}
			break;
    case 0x42e:    // 
      t &= 0xf;
      switch(t) {
				case 0x8:
					// Trident scrive 9000 a 16bit poi 5000
					break;
				}
			break;
    case 0x46e:    // 
      t &= 0xf;
      switch(t) {
				case 0x8:
					// Trident scrive 08 
					break;
				}
			break;
    case 0x4ae:    // 
      t &= 0xf;
      switch(t) {
				case 0x8:
					// Trident scrive 0000 a 16bit
					break;
				}
			break;
    case 0x92e:    // 
      t &= 0xf;
      switch(t) {
				case 0x8:
					// Trident scrive 5555 a 16bit 
					VGAsignature=(VGAsignature & 0xff00) | t1;
					break;
				case 0x9:
					// Trident scrive 5555 a 16bit 
					VGAsignature=(VGAsignature & 0x00ff) | ((uint16_t)t1 << 8);
					break;
				}
			break;
#endif

#ifdef CGA
    case 0x3d:    // 3d0-3df  CGA (forse son solo 8 ma non si capisce, 3d8 3df   http://nerdlypleasures.blogspot.com/2016/05/ibms-cga-hardware-explained.html
      t &= 0xf;
      switch(t) {
        case 0xb:
          CGAreg[0xa] &= ~0b00000110;      // pulisco trigger light pen
          break;
        case 4:     // 3d4, 3d5 sono reg. 6845..
        case 0xc:
          CGAreg[4]=t1;
          break;
        case 5:     // 
        case 0xd:
          CGAreg[5]=i6845RegR[CGAreg[4] % 18]=i6845RegW[CGAreg[4] % 18]=t1;
          break;
  // [da qualche parte dice che dovrebbero essercene 16... http://www.techhelpmanual.com/901-color_graphics_adapter_i_o_ports.html
        default:
          CGAreg[t]=t1;
          break;
        }
      break;
#endif
    
    case 0x3f:    // 3f8-3ff  COM1; 3f2-5 NEC floppy (http://www.techhelpmanual.com/896-diskette_controller_ports.html)
									// https://wiki.osdev.org/Floppy_Disk_Controller#Accessing_Floppies_in_Real_Mode
      t &= 0xf;

      switch(t) {
        case 2:   // Write: digital output register (DOR)  MOTD MOTC MOTB MOTA DMA ENABLE SEL1 SEL0
					// questo registro in effetti non appartiene al uPD765 ma è esterno...
					// ma secondo ASM 8 è Interrupt on e 4 è reset ! direi che sono 2 diciture per la stessa cosa £$%@#
					fdDisk=t1 & 3;
					/*FloppyContrRegR[0]=*/FloppyContrRegW[0]=t1;
					FloppyContrRegW[1]=0;
					floppyState[fdDisk] &= ~(FLOPPY_STATE_XFER);	// xfer dopo la prima lettura (award286...
					if(!(FloppyContrRegW[0] & 0b00000100)) {		// così è RESET!
						floppylastCmd[fdDisk]=0xff;
						FloppyFIFOPtrW=FloppyFIFOPtrR=0;
						floppyHead[fdDisk]=0;
						floppyTrack[fdDisk]=0;
						floppySector[fdDisk]=0,
						floppyTimer=0;

/* no.. lascio in InitHW						if(disk <= ((MachineSettings & B8(11000000)) >> 6))		// (2 presenti!
							floppyState[fdDisk]=0x00; // b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent
						else
							floppyState[fdDisk]=0x00; //
						*/

#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			extern HFILE spoolFile;
			char myBuf[256];
			wsprintf(myBuf,"  RESET FDC!\n");
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
						}
					else  {		// interrupt su rilascio del reset, ok
//						FloppyFIFOPtrW=FloppyFIFOPtrR=0;
						floppyState[fdDisk] &= ~(FLOPPY_STATE_DDR);	// passo a write
						if(FloppyContrRegW[0] & 0b00010000)	
							floppyState[0] |= FLOPPY_STATE_MOTORON;		// motor
						else
							floppyState[0] &= ~FLOPPY_STATE_MOTORON;
						if(FloppyContrRegW[0] & 0b00100000)
							floppyState[1] |= FLOPPY_STATE_MOTORON;
						else
							floppyState[1] &= ~FLOPPY_STATE_MOTORON;
						if(FloppyContrRegW[0] & 0b01000000)
							floppyState[2] |= FLOPPY_STATE_MOTORON;
						else
							floppyState[2] &= ~FLOPPY_STATE_MOTORON;
						if(FloppyContrRegW[0] & 0b10000000)
							floppyState[3] |= FLOPPY_STATE_MOTORON;
						else
							floppyState[3] &= ~FLOPPY_STATE_MOTORON;
						if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ /*in asm dice che questo è "Interrupt ON"*/)
							setFloppyIRQ(SEEK_FLOPPY_DELAYED_IRQ);
						}
          break;
        case 4:   // Read-only: main status register (MSR)  REQ DIR DMA BUSY D C B A
          break;
        case 5:   // Read/Write: FDC command/data register
					if(!FloppyFIFOPtrW) {
						FloppyContrRegW[1]=t1;
						floppyState[fdDisk] &= ~(FLOPPY_STATE_XFER);	// xfer tolgo
						switch(t1 & FLOPPY_COMMAND_MASK) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
							case FLOPPY_READ_DATA:			// read data
							case FLOPPY_READ_TRACK:			// read track
							case FLOPPY_READ_DELETED_DATA:			// read deleted
							case FLOPPY_READ_ID:			// read ID
							case FLOPPY_WRITE_DATA:			// write data
							case FLOPPY_FORMAT_TRACK:			// format track
							case FLOPPY_WRITE_DELETED_DATA:			// write deleted data
							case FLOPPY_SCAN_EQUAL:			// scan equal
							case FLOPPY_SCAN_LOW_OR_EQUAL:			// scan low or equal
							case FLOPPY_SCAN_HIGH_OR_EQUAL:			// scan high or equal
							case FLOPPY_RECALIBRATE:			// recalibrate
							case FLOPPY_SPECIFY:			// specify
							case FLOPPY_SEEK:			// seek (goto cylinder
							case FLOPPY_SENSE_DRIVE_STATUS:			// sense drive status
							case FLOPPY_VERSION:
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case FLOPPY_LOCK:		// lock...
							case FLOPPY_PERPENDICULAR_MODE:		// perpendicular
							case FLOPPY_DUMPREG:		// dumpreg
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case FLOPPY_SENSE_INTERRUPT:			// sense interrupt status
								FloppyFIFOPtrW=0; FloppyFIFOPtrR=0;
								FloppyContrRegR[0]=FLOPPY_STATUS_REQ;
								if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
									setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
								break;
							case 0x00:			// arriva a inizio comandi, sopra: ossia dopo una write a 3F2, la gestisco così...
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							default:		// invalid, 
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								FloppyFIFO[0];
	//								FloppyContrRegR[4] |= B8(10000000);		// 
								break;
							}
						}
					else {
						FloppyFIFO[FloppyFIFOPtrW++] = t1;
						FloppyFIFOPtrW &= 15;		// safety! errore ev.
						floppyState[fdDisk] &= ~(FLOPPY_STATE_XFER);	// tolgo xfer

						switch(FloppyContrRegW[1] & FLOPPY_COMMAND_MASK) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
							case FLOPPY_WRITE_DATA:			// write data
							case FLOPPY_READ_DATA:			// read data
							case FLOPPY_READ_TRACK:			// read track
							case FLOPPY_READ_DELETED_DATA:			// read deleted
							case FLOPPY_WRITE_DELETED_DATA:			// write deleted data
							case FLOPPY_SCAN_EQUAL:			// scan equal
							case FLOPPY_SCAN_LOW_OR_EQUAL:			// scan low or equal
							case FLOPPY_SCAN_HIGH_OR_EQUAL:			// scan high or equal
							case FLOPPY_VERSION:
								fdDisk=FloppyFIFO[1] & 3;	//serve?
								if(FloppyFIFOPtrW==9) {
									floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read
//									FloppyFIFOPtrW=0;
									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
//									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR);
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
									}
								break;
							case FLOPPY_FORMAT_TRACK:			// format track
								fdDisk=FloppyFIFO[1] & 3;	//serve?
								if(FloppyFIFOPtrW==6) {
									floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read
//									FloppyFIFOPtrW=0;
									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
//									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR);
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
									}
								break;
							case FLOPPY_SENSE_DRIVE_STATUS:			// sense drive status
								fdDisk=FloppyFIFO[1] & 3;	//serve?
								if(FloppyFIFOPtrW==2) {
									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR | FLOPPY_STATUS_BUSY) | (1 << fdDisk);		// Busy :) (anche 1=A??
									FloppyFIFO[1] = 0b00100000 | (!floppyTrack[fdDisk] ? 0b00010000 : 0) | 
										// 2 side come?? head & 1 forse?
										(FloppyFIFO[1] & 0b00000111);		//ST3: B7 Fault, B6 Write Protected, B5 Ready, B4 Track 0, B3 Two Side, B2 Head (side select), B1:0 Disk Select (US1:0)

	//								if(!(floppyState[fdDisk] & B8(10000000)))		// forzo errore se disco non c'è... se ne sbatte
	//									FloppyFIFO[0] |= B8(11000000);

									FloppyContrRegR[0]=FLOPPY_STATUS_REQ;
//									FloppyFIFOPtrW=0;
									FloppyFIFOPtrR=1;
									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
									}
								break;
							case FLOPPY_SPECIFY:			// specify
								if(FloppyFIFOPtrW==3) {
								// in FIFO[1] : StepRateTime[7..4] | HeadUnloadTime[3..0]				20h
								// in FIFO[2] : HeadLoadTime[7..1] | NonDMAMode[0]							07h
									floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read
//									FloppyFIFOPtrW=0;
//									FloppyFIFOPtrR=0;
									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
									FloppyContrRegR[0]=FLOPPY_STATUS_REQ;
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: SPECIFY ok\n",timeGetTime()
				);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
									}
								break;
							case FLOPPY_RECALIBRATE:			// recalibrate
								fdDisk=FloppyFIFO[1] & 3;	//serve?
								if(FloppyFIFOPtrW==2) {
								// in FIFO[1] : x x x x x 0 US1 US0									00h
									FloppyContrRegR[0]=FLOPPY_STATUS_BUSY | (1 << fdDisk);					// busy
									// __delay_ms(12*floppyTrack[fdDisk])
									floppyTrack[fdDisk]=0;
									FloppyContrRegR[0]=0b00000000;					// v.read sopra
#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: RECALIBRATE 2\n",timeGetTime()
				);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif

//									FloppyFIFOPtrW=0;
//									FloppyFIFOPtrR=0;
									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
									}
								break;
							case FLOPPY_SEEK:			// seek (goto cylinder
								fdDisk=FloppyFIFO[1] & 3;	//serve?
								if(FloppyFIFOPtrW==3) {
									// in FIFO[1] : x x x x x Head US1 US0						00h
									// in FIFO[2] : New Cylinder Number								00h
									if(FloppyFIFO[2] < totalTracks) {
										FloppyContrRegR[0]=FLOPPY_STATUS_BUSY | (1 << fdDisk);					// busy
										floppyTrack[fdDisk]=FloppyFIFO[2];
									// __delay_ms(12*tracce)
										FloppyContrRegR[0]=FLOPPY_STATUS_REQ;					// finito :)
										}
									else {
										FloppyContrRegR[0]=FLOPPY_STATUS_REQ;
										}
//									FloppyFIFOPtrW=0;
//									FloppyFIFOPtrR=0;
									floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read cmq
									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
									}
								break;
							case FLOPPY_READ_ID:			// read ID
								fdDisk=FloppyFIFO[1] & 3;	//serve?
								if(FloppyFIFOPtrW==2) {
									floppyState[fdDisk] |= FLOPPY_STATE_DDR;		// passo a read
//									FloppyFIFOPtrW=0;
									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
//									FloppyContrRegR[0]=(FLOPPY_STATUS_REQ | FLOPPY_STATUS_DDR);
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;

									if(FloppyContrRegW[0] & FLOPPY_CONTROL_IRQ)
										setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
									floppylastCmd[fdDisk]=FloppyContrRegW[1] & FLOPPY_COMMAND_MASK;
									}
								break;

							case FLOPPY_LOCK:		// lock...
							case FLOPPY_PERPENDICULAR_MODE:		// perpendicular
							case FLOPPY_DUMPREG:		// dumpreg
								
								break;
							case FLOPPY_SENSE_INTERRUPT:			// sense interrupt status
								break;
							case 0x00:			// arriva a inizio comandi, sopra: ossia dopo una write a 3F2, la gestisco così...
								
								break;
							default:		// invalid, 
								
								
	
								break;
							}

						}
//            i8259IRR |= B8(01000000);		// simulo IRQ...
//					if(FloppyContrRegW[0] & B8(10000000))		// RESET si autopulisce
//						FloppyContrRegW[0] &= ~B8(10000000);		// 
//					FloppyContrRegR[0] &= ~B8(00010000);		// non BUSY :)
          break;
#ifdef PCAT
				case 1:		// 
					break;
				case 6:		// 3F6 (dice come HD status register... boh?? sarebbe 1f7
//					AMI scrive 00, 04 a caso, phoenix pure 04 00
					hdControl=t1;
					if(t1 & 4) {		// Reset, secondo PDF WD1003 e https://www.mjm.co.uk/how-hard-disk-ata-registers-work.html
//						hdState=B8(01010000); // NO per Diagnose  hdError=0;

						// (PARE RESETTARE ANCHE IL FLOPPY!! verificare
						HDFIFOPtrR=HDFIFOPtrW=0;
						memset(HDContrRegR,0,sizeof(HDContrRegR));		
						memset(HDContrRegW,0,sizeof(HDContrRegW));
						hdHead=0;
						hdCylinder=0;
						hdSector=0;
						hdState=hdControl=0;

						FloppyFIFOPtrW=FloppyFIFOPtrR=0;
						memset(floppyHead,0,sizeof(floppyHead));
						memset(floppyTrack,0,sizeof(floppyTrack));
						memset(floppySector,0,sizeof(floppySector));
						memset(floppylastCmd,0xff,sizeof(floppylastCmd));
						for(i=0; i<4; i++)
							floppyState[i] &= FLOPPY_STATE_DISKPRESENT;
						floppyTimer=0;

						}
					// b1 se set disattiva IRQ HD...  b7 per leggere LBA
					break;
				case 7:		// solo AT http://isdaman.com/alsos/hardware/fdc/floppy.htm
					// b0 e b1 dovrebbero essere data trasnfer rate del floppy
          break;
#endif

          
  //_UART0_DATA      .equ $FF80        		 ; data
  //_UART0_DLAB_0    .equ $FF80        		 ; divisor latch low uint8_t
  //_UART0_IER       .equ $FF81        		 ; Interrupt enable register
  //_UART0_DLAB_1    .equ $FF81        		 ; divisor latch high uint8_t
  //_UART0_FCR       .equ $FF82        		 ; FIFO control register
  //_UART0_LCR       .equ $FF83        		 ; line control register
  // 4= Modem CTRL
  //_UART0_LSR       .equ $FF85        		 ; line status register
  // 6= Modem Status
  // 7= scratchpad
        case 0+8:
          if(i8250Reg[3] & 0x80) {
            uint32_t baudRateDivider;
            i8250Regalt[0]=t1;
set_uart1:
            if(MAKEWORD(i8250Regalt[0],i8250Regalt[1]) != 0) {
//              baudRateDivider = (GetPeripheralClock()/(4*MAKEWORD(i8250Regalt[0],i8250Regalt[1]))-1)
                /*i8250Regalt[2]*/;
//              U6BRG=baudRateDivider;
              }
            }
          else {
            i8250Reg[0]=t1;
//            WriteUART1(t1);
            }
//					COMDataEmuFifo[0]=COMDataEmuFifo[1]=COMDataEmuFifo[2]='M';  COMDataEmuFifoCnt=0;
          break;
        case 1+8:
          if(i8250Reg[3] & 0x80) {
            i8250Regalt[1]=t1;
            goto set_uart1;
            }
          else {
            i8250Reg[1]=t1;		// IER: b0=RX, b1=TX, b2= RStatus, b3=Modem
            }
          break;
        case 2+8:
          i8250Reg[2]=t1;			// IIR: b0=0 se IRQ pending, b1-2=interrupt ID
          break;
        case 3+8:
          i8250Reg[3]=t1;
          break;
        case 4+8:
          i8250Reg[4]=t1;
					if(i8250Reg[4] & 0)		// verificare RTS CTS ecc per mouse power-on!
						;
#ifdef MOUSE_TYPE
					COMDataEmuFifo[0]=COMDataEmuFifo[1]=COMDataEmuFifo[2]='M';  COMDataEmuFifoCnt=0;		// Line, RTS CTS...
					// va bene così per entrambi i modelli...
		      mouseState |= 128;  // marker 
#endif

          break;
        case 5+8:
          if(i8250Reg[3] & 0x80) {
            i8250Regalt[2]=t1;
            }
          else {
  //          i8250Reg[5]=t1;
            }
          break;
        case 6+8:
  //         i8250Reg[6]=t1;
          break;
        case 7+8:
          i8250Reg[7]=t1;
          break;
        }
      break;
    }

	}

void OutShortValue(uint16_t t,uint16_t t1) {
	register uint16_t i;


	OutValue(t,LOBYTE(t1));			// per ora (usato da GLAbios
	OutValue(t+1,HIBYTE(t1));			// 
	}


void setFloppyIRQ(uint8_t delay) {

	// mah non aiuta con phoenix e ami e a volte fa incazzare PCXTbios
					i8259IRR |= 0x40;		// simulo IRQ...
//	floppyTimer=delay;
	}

uint32_t getDiscSector(uint8_t d) {
  uint8_t i;
  uint32_t n;

  n=0;
  for(i=0; i<floppyTrack[d]; i++) {
//    n+=sectorsPerTrack[i];       // single-side
    n+=sectorsPerTrack[d]*2;     // double-side  https://aeb.win.tue.nl/linux/hdtypes/hdtypes-4.html
    }
  if(floppyHead[d])             // (head prima sotto poi sopra, 1=sopra (v.
    n+=sectorsPerTrack[d];
  
  n+= (floppySector[d] -1 /* PARTE DA 1....*/);

  return n;  
	}
  

uint8_t *encodeDiscSector(BYTE disk) {
  int i;
  uint8_t c;

  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*512;

	memcpy(sectorData,msdosDisk,512);
  
  return sectorData;
  }

#if defined(PCAT) || defined(ROM_HD_BASE)
uint32_t getHDiscSector(uint8_t d,uint8_t lba) {
  uint8_t i;
  uint32_t n;

/*  n=0;
  for(i=0; i<hdCylinder; i++) {
    n+=HD_SECTORS_PER_CYLINDER;
    }
  for(i=0; i<hdHead; i++) {        // head VERIFICARE
    n+=HD_SECTORS_PER_CYLINDER;
		}
  
  n+= (hdSector);*/

	if(!lba)
		n=((hdCylinder*HD_HEADS+hdHead)*HD_SECTORS_PER_CYLINDER+hdSector);		//da smartestblob
		/* PARTE DA 0....*/
	else
		n=MAKELONG(MAKEWORD(hdSector,LOBYTE(hdCylinder)),MAKEWORD(HIBYTE(hdCylinder),0));
	// finire, v.sopra

  return n;  
	}
  
void setHDIRQ(uint8_t delay) {

#ifdef PCAT
	//										i8259IRR2 |= 0b01000000);		// simulo IRQ...
#else
//											i8259IRR |= 0b00100000);		// simulo IRQ...
#endif

	hdTimer=delay;
	}

uint8_t *encodeHDiscSector(uint8_t disk) {
  int i;
  uint8_t c;

  msdosHDisk=((uint8_t*)MSDOS_HDISK)+getHDiscSector(disk,0)*SECTOR_SIZE;

	memcpy(sectorData,msdosHDisk,SECTOR_SIZE);
 
  return sectorData;
  }
#endif



void initHW(void) {
	int i;
  
	MachineSettings=0b00101101;			//1 floppy
#ifdef MDA 
	MachineSettings=0b01111101;			//2 floppy
#endif
#ifdef VGA
	MachineSettings=0b00001101;
#endif
#ifdef EXT_80x87 
	MachineSettings |= 2;
#endif
		// b0=floppy (0=sì); b1=FPU; bits 2,3 memory size (64K bytes); b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25; b7:6 #floppies-1
		// ma su IBM5160 b0=1 SEMPRE dice!
#ifdef PC_IBM5150
	MachineSettings=0b00101101);		// 64KB, v.sopra
//	MachineSettings=0b00100001);		// 640KB
#endif
#ifdef PCAT
	MachineSettings=0b10110000;		// non cambia un c su Award ma anche sugli altri, il video si autogestisce  https://stanislavs.org/helppc/8042.html
//BASE_MEM8	EQU	00001000B	; BASE PLANAR R/W MEMORY EXTENSION 640/X
//BASE_MEM	EQU	00010000B	; BASE PLANAR R/W MEMORY SIZE 256/512		0=256
//MFG_LOOP	EQU	00100000B	; LOOP POST JUMPER BIT FOP MANUFACTURING  0=installed
//DSP_JMP		EQU	01000000B	; DISPLAY TYPE SWITCH JUMPER BIT, 0=CGA 1=MDA
//KEY_BD_INHIB	EQU	10000000B	; KEYBOARD INHIBIT SWITCH BIT, 0=inibita
#endif
	MachineFlags=0b00000000;
		// b0-1=Speaker on; b2-3=TURBO PC (o select); b4-5=Parity/I/O errors su TINYBIOS; b66= 0:KC clk low; b7= 0 enable KB    https://wiki.osdev.org/Non_Maskable_Interrupt
  memset(CGAreg,0,sizeof(CGAreg));
  CGAreg[0]=0;              // disattivo video, default cmq
	memset(&MDAreg,0,sizeof(MDAreg));  // 
  i6845RegR[10]=i6845RegW[10]=0x20;     //disattivo cursore!
#ifdef VGA
	memset(&VGAreg,0,sizeof(VGAreg));  // 
  memset(&VGAcrtcReg,0,sizeof(VGAcrtcReg)); 
  memset(&VGAactlReg,0,sizeof(VGAactlReg)); 
  memset(&VGAgraphReg,0,sizeof(VGAgraphReg)); 
	VGAFF=VGAptr=0;
#endif
  
  memset(i8237RegR,0,sizeof(i8237RegR));
  memset(i8237RegW,0,sizeof(i8237RegW));
	i8237RegR[15]=i8237RegW[15] = 0b00001111;
#ifdef PCAT
  memset(i8237Reg2R,0,sizeof(i8237Reg2R));
  memset(i8237Reg2W,0,sizeof(i8237Reg2W));
	i8237Reg2R[15]=i8237Reg2W[15] = 0b00001111;
#endif
	memset(DMApage,0,sizeof(DMApage));
#ifdef PCAT
	memset(DMApage2,0,sizeof(DMApage2));
#endif
  i8259RegR[0]=i8259RegW[0]=0x00; i8259RegR[1]=i8259RegW[1]=0xff; i8259IMR=0xff; i8259ISR=i8259IRR=0;
	memset(i8259ICW,0,sizeof(i8259ICW));
	memset(i8259OCW,0,sizeof(i8259OCW));
	i8259ICWSel=0;
#ifdef PCAT
  i8259Reg2R[0]=i8259Reg2W[0]=0x00; i8259Reg2R[1]=i8259Reg2W[1]=0xff; i8259IMR2=0xff; i8259ISR2=i8259IRR2=0;
	memset(i8259ICW2,0,sizeof(i8259ICW2));
	memset(i8259OCW2,0,sizeof(i8259OCW2));
	i8259ICW2Sel=0;
#endif
  memset(i8253RegR,0,sizeof(i8253RegR));
  memset(i8253RegW,0,sizeof(i8253RegW));
  memset(i8253Mode,0,sizeof(i8253Mode));
  memset(i8253Flags,0,sizeof(i8253Flags));
  memset(i8253TimerR,0,sizeof(i8253TimerR));
  memset(i8253TimerL,0,sizeof(i8253TimerL));
  memset(i8253TimerW,0,sizeof(i8253TimerW));
  memset(i8250Reg,0,sizeof(i8250Reg));
  memset(i8250Regalt,0,sizeof(i8250Regalt));
  memset(i6845RegR,0,sizeof(i6845RegR));
  memset(i6845RegW,0,sizeof(i6845RegW));
  memset(i146818RegR,0,sizeof(i146818RegR));
  memset(i146818RegW,0,sizeof(i146818RegW));
//  memset(i146818RAM,0,sizeof(i146818RAM));			no meglio di no, il PCAT ci fa delle cose al reset!
#ifdef PCAT
  memset(i8042RegR,0,sizeof(i8042RegR));
  memset(i8042RegW,0,sizeof(i8042RegW));
	memset(&KBRAM,0,sizeof(KBRAM));
#else
  memset(i8255RegR,0,sizeof(i8255RegR));
  memset(i8255RegW,0,sizeof(i8255RegW));
#endif
	memset(&Keyboard,0,sizeof(Keyboard));
#ifdef PCAT
  KBCommand=0b01010000 /*XT,disabled*/;  i8042Input=0; i8042Output=2/*A20*/; i8042OutPtr=1 /* power-on, manda 00*/;
//	KBStatus=0x00;		b2=0 al power, 1 dopo
	KBStatus |= 0b00010001;		// b4=enabled(switch/key 
#else
  KBCommand=0b01000000 /* enabled */; KBStatus=0x00;
#endif

  memset(FloppyContrRegR,0,sizeof(FloppyContrRegR));
  memset(FloppyContrRegW,0,sizeof(FloppyContrRegW));
	memset(FloppyFIFO,0,sizeof(FloppyFIFO));
	FloppyFIFOPtrW=FloppyFIFOPtrR=0;
	for(i=0; i<4; i++) {
		floppyState[i]=0x00; /* b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent*/
		floppyHead[i]=0;
		floppyTrack[i]=0;
		floppySector[i]=0;
		floppylastCmd[i]=0;
		}
	floppyState[0]=FLOPPY_STATE_DISKPRESENT;			// 00 per boot da HD!
#ifdef PCAT
	floppyState[1]=FLOPPY_STATE_DISKPRESENT;
#else
	floppyState[1]=0x00;
#endif
	floppyState[2]=0;
	floppyState[3]=0;
#if defined(PCAT) || defined(ROM_HD_BASE)
	memset(HDContrRegR,0,sizeof(HDContrRegR));
	memset(HDContrRegW,0,sizeof(HDContrRegW));
#ifdef ROM_HD_BASE
	memset(HDFIFOR,0,sizeof(HDFIFOR));
	memset(HDFIFOW,0,sizeof(HDFIFOW));
#endif
	HDFIFOPtrR=HDFIFOPtrW=0;
	hdHead=0;
	hdCylinder=0;
	hdSector=0;
	hdState=hdControl=0;
#ifdef PCAT
	hdDisc=0;
	hdError=0;
	hdPrecomp=0;
#endif
#endif
  
	mouseState=0b10000000;
	memset(COMDataEmuFifo,0,sizeof(COMDataEmuFifo));
	COMDataEmuFifoCnt=0;
  
	i146818RAM[13] = 0b10000000;		// VRT RTC batt ok!

	if(!(i146818RAM[11] & 0b00000100)) {			// sarebbe b2 del registro 11 ma al boot non c'è...
/*    currentTime.hour=to_bcd(currentTime.hour);
    currentTime.min=to_bcd(currentTime.min);
    currentTime.sec=to_bcd(currentTime.sec);*/
    
    i146818RAM[0x32] = 0x19; //to_bcd((((WORD)currentDate.year)+1900) / 100);		// secolo NON c'è in PICC_DATE :)
// ok  #warning GLATICK dà clock error.... boh 7/8/25    provare 19!
    
/*    currentDate.year=to_bcd(currentDate.year % 100);			// anno, e v. secolo in RAM[32], GLATick
    currentDate.mon=to_bcd(currentDate.mon  +1);
    currentDate.mday=to_bcd(currentDate.mday);
 * qua sono già BCD... v init in main */
    }

#ifdef PCAT
		{const uint8_t valori_cmos[64]= {		// https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/cmos_ram#floppy
//      448KB base 64 extended
  /*00:*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA6, 0x02, 0x50, 0x80, 0x20, 0x02, 
  /*10:*/ 0x30, 0x00, 0x00, 0x00, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*20:*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x12, 
  /*30:*/ 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x32, 0x00, 0x01, 0x00, 0x00, 0x00, 
			};
		memcpy(&i146818RAM[16],&valori_cmos[16],48);
    }
#endif

  
	ColdReset=0;
	ExtIRQNum.x=IntIRQNum.x=0;
  
#ifdef ST7735
  OC1CONbits.ON = 0;   // spengo buzzer/audio
  PR2 = 65535;		 // 
  OC1RS = 65535;		 // 
#endif
#ifdef ILI9341
  OC7CONbits.ON = 0;   // spengo buzzer
  PR2 = 65535;		 // 
  OC7RS = 65535;		 // 
#endif

  IEC0bits.T3IE=1;       // enable Timer 3 interrupt 
  IEC0bits.T4IE=1;       // enable Timer 4 interrupt 
  IEC0bits.T5IE=0;       // enable Timer 5 interrupt INUTILE!!! e molto veloce... 0x12, dice 64K in 1mS al DMA fanno 66KHz...
  
  }

BYTE to_bcd(BYTE n) {
	
	return (n % 10) | ((n / 10) << 4);
	}

BYTE from_bcd(BYTE n) {
	
	return (n & 15) + (10*(n >> 4));
	}

char *strcpyhl(char *dest,const char *src) {
	char *d=dest;
	BOOL odd=0;

	while(*src) {
		if(odd) {
			*d=*src;
			d+=2;
			*d=0;
			}
		else {
			*(d+1)=*src;
			*(d+2)=0;
			}
		odd = !odd;
		src++;
		}

	return dest;
	}

