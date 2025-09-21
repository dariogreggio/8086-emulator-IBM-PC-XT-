//https://wiki.osdev.org/Floppy_Disk_Controller#Bit_MT


#include <windows.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include "8086win.h"

#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )


BYTE bios_rom[ROM_SIZE];
#ifdef ROM_SIZE2
BYTE bios_rom_opt[ROM_SIZE2 /* per GLATICK per ora*/];
#endif
#ifdef ROM_BASIC
BYTE IBMBASIC[0x8000];
#endif
BYTE HDbios[0x2000];			// la metto a E800
extern BYTE CPUPins;
#define UNIMPLEMENTED_MEMORY_VALUE 0xffff //6868  //0xcccc		//:)		// INT3 


#ifdef EXT_80286
extern struct _EXCEPTION Exception86;
extern union _DTR GDTR;
extern union _DTR LDTR;
extern union MACHINE_STATUS_WORD _msw;
#endif


extern LARGE_INTEGER pcFrequency;


extern BOOL debug;
extern HFILE spoolFile;
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
//#define CGA_BASE 0xB0000L		// dice b0000 o b8000 se 32KB, a0000 se 64 o 128
//#define CGA_SIZE 65536L				// video; https://web.stanford.edu/class/cs140/projects/pintos/specs/freevga/vga/vgamem.htm
#define VGA_BASE 0xA0000L		// 4 blocchi da 64K
#define VGA_SIZE (65536L*4)
		// v. VGAgraphReg[6] & 0x0c per dimensione e posizione finestra grafica...
#endif
#if defined(CGA) || defined(MDA) 
uint8_t CGAram[CGA_SIZE /* 640*200/8 320*200/2 */];		// anche MDA :) e VGA
#endif
#ifdef VGA
uint8_t VGABios[32768];
uint8_t VGAram[VGA_SIZE];
uint8_t VGAreg[16],  // https://web.stanford.edu/class/cs140/projects/pintos/specs/freevga/vga/vga.htm
	VGAcrtcReg[32],VGAactlReg[32],VGAgraphReg[16],VGAseqReg[8],
	VGAFF,VGAptr;
uint32_t VGADAC[256];		// 18 bit per VGA "base"
uint16_t VGAsignature;
union DWORD_BYTES VGAlatch;
#endif
uint8_t CGAreg[18];  // https://www.seasip.info/VintagePC/cga.html
uint8_t MDAreg[18];  // https://www.seasip.info/VintagePC/mda.html
//  http://www.oldskool.org/guides/oldonnew/resources/cgatech.txt
extern COLORREF Colori[];
uint8_t i8237RegR[16],i8237RegW[16],i8237FF=0,i8237Mode[4];
uint16_t i8237DMAAddr[4],i8237DMALen[4];
uint16_t i8237DMACurAddr[4],i8237DMACurLen[4];
#ifdef PCAT
uint8_t i8237Reg2R[16],i8237Reg2W[16],i8237FF2=0,i8237Mode2[4];
uint16_t i8237DMAAddr2[4],i8237DMALen2[4];
uint16_t i8237DMACurAddr2[4],i8237DMACurLen2[4];
#endif
uint8_t DMApage[8],DMArefresh, DIAGPort/*DTK 286 rilegge port 80h*/;
uint8_t i8259RegR[2],i8259RegW[2],i8259ICW[4],i8259OCW[3],i8259ICWSel,i8259IMR,i8259IRR,i8259ISR
#ifdef PCAT
	,i8259Reg2R[2],i8259Reg2W[2],i8259ICW2[4],i8259OCW2[3],i8259ICW2Sel,i8259IMR2,i8259IRR2,i8259ISR2,
	DMApage2[8] // per DTK
#endif
	;
uint8_t i8253RegR[4],i8253RegW[4],i8253Mode[3],i8253ModeSel;	// uso MSB di Mode come "OUT" e b6 come "reload" (serve anche un "loaded" flag, credo, per one shot ecc
uint16_t i8253TimerR[3],i8253TimerW[3],i8253TimerL[3];
uint8_t i8250Reg[8],i8250Regalt[3];
uint8_t LPT[3/**/][4 /*3*/];		// per ora, al volo!
#ifdef PCAT
uint8_t i8042RegR[2],i8042RegW[2],KBRAM[32],
	i8042Input,i8042OutPtr;
#else
uint8_t i8255RegR[4],i8255RegW[4];
#endif
uint8_t i8042Output;  //b1=A20, b0=Reset,  SERVE cmq per 286 :) A20
uint8_t KBCommand=B8(01010000),
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
uint8_t MachineFlags=B8(00000000);		// b0-1=Speaker on; b2-3=TURBO PC (o select); b4-5=Parity/I/O errors su TINYBIOS; b66= 0:KC clk low; b7= 0 enable KB    https://wiki.osdev.org/Non_Maskable_Interrupt
// v. cmq i8255RegR[2]
uint8_t MachineSettings=B8(01101101);		// b0=floppy (0=sì); b1=FPU; bits 2,3 memory size (64K bytes); b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25; b7:6 #floppies-1
		// ma su IBM5160 b0=1 SEMPRE dice!
uint8_t i6845RegR[18],i6845RegW[18];
uint8_t i146818RegR[2],i146818RegW[2],i146818RAM[64];
uint8_t FloppyContrRegR[2],FloppyContrRegW[2],FloppyFIFO[16],FloppyFIFOPtrR,FloppyFIFOPtrW; // https://www.ardent-tool.com/floppy/Floppy_Programming.html#FDC_Registers
uint8_t floppyState[4]={0x80,0x00,0,0} /* b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent*/,
  floppyHead[4]={0,0,0,0},floppyTrack[4]={0,0,0,0},floppySector[4]={0,0,0,0},
	floppylastCmd[4];
uint16_t floppyTimer /*simula attività floppy*/;
uint8_t HDContrRegR[4],HDContrRegW[4],HDFIFOR[16],HDFIFOW[16],HDFIFOPtrR,HDFIFOPtrW, // 
  hdControl=0,hdState=0,hdHead=0,hdSector=0;
uint16_t hdCylinder=0;
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
#elif MS_DOS_VERSION==4
//BYTE sectorsPerTrack[4]={18,18,18,18} /*floppy1.4*/;
BYTE sectorsPerTrack[4]={9,9,9,9} /*floppy720 v. tracks*/;
BYTE totalTracks=80;		/*floppy1.4*/
#endif
static uint8_t sectorData[SECTOR_SIZE];

unsigned char MSDOS_DISK[2][1440/*360*/*SECTOR_SIZE*2];
const unsigned char *msdosDisk;
uint32_t getDiscSector(uint8_t);
uint8_t *encodeDiscSector(uint32_t);
void setFloppyIRQ(uint16_t);
#define DEFAULT_FLOPPY_DELAYED_IRQ 1
#define SEEK_FLOPPY_DELAYED_IRQ 10

unsigned char MSDOS_HDISK[40     *1048576L];
const unsigned char *msdosHDisk;
uint32_t getHDiscSector(uint8_t);
uint8_t *encodeHDiscSector(uint8_t);
extern BYTE CPUDivider;
BYTE mouseState=128;
BYTE COMDataEmuFifo[5];
uint8_t COMDataEmuFifoCnt;
uint8_t Keyboard[1]={0};
uint8_t ColdReset=1;
union INTERRUPT_WORD ExtIRQNum,IntIRQNum;
volatile struct tm currentDateTime;
extern uint8_t Pipe1;
extern union PIPE Pipe2;



#if defined(EXT_80186) // no, pare|| defined(EXT_NECV20)
#define MAKE20BITS(a,b) (0xfffff & ((((uint32_t)(a)) << 4) + ((uint16_t)(b))))		// somma, non OR - e il bit20/A20 può essere usato per HIMEM
#else
#define MAKE20BITS(a,b) (0xfffff & ((((uint32_t)(a)) << 4) + ((uint16_t)(b))))		// somma, non OR - e il bit20/A20 può essere usato per HIMEM
#endif
// su 8088 NON incrementa segmento se offset è FFFF ... 

#define ADDRESS_286_PRE() {\
	if(seg->s.x < 0xf000    && seg->s.x > GDTR.Limit) /*PATCH  LMSW   sistemare*/ {\
		Exception86.descr.ud=EXCEPTION_GP;\
		goto exception286;\
		}\
	if(seg->d.Limit		&& ofs>seg->d.Limit) {/*0=65536, ma v. anche DOWN Expand*/\
		Exception86.descr.ud=EXCEPTION_STACK;\
		goto exception286;\
		}\
	}
#define ADDRESS_286_POST() {\
	t=MAKELONG(seg->d.Base+ofs,seg->d.BaseH);\
	seg->d.Access.A=1;\
	if(!(i8042Output & 2))/*A20*/\
		t &= 0xfffff;\
	}
#define ADDRESS_286_R() {\
	ADDRESS_286_PRE();\
	if(seg->s.x >= 0xe000)		/*PATCH  LMSW   sistemare*/ {\
		t=MAKE20BITS(seg->s.x,ofs);\
		}\
	else {\
		if((seg->d.Access.b & B8(00000001)) == B8(00000000)) {/*boh data */\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		ADDRESS_286_POST();\
		}\
	}
#define ADDRESS_286_E() {\
	struct SEGMENT_DESCRIPTOR *sd;\
	if(seg->s.x >= 0xe000)		/*PATCH  LMSW   sistemare*/ {\
		t=MAKE20BITS(seg->s.x,ofs);\
		}\
	else {\
		ADDRESS_286_PRE();\
		if((seg->d.Access.b & B8(00001010)) != B8(00001010)) {/* code, readable*/\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		ADDRESS_286_POST();\
		}\
	}
#define ADDRESS_286_W() {\
	struct SEGMENT_DESCRIPTOR *sd;\
	ADDRESS_286_PRE();\
	if((seg->d.Access.b & B8(00001010)) != B8(00000010)) {/*data, writable*/\
		Exception86.descr.ud=EXCEPTION_GP;\
		goto exception286;\
		}\
	ADDRESS_286_POST();\
	}

uint8_t _fastcall GetValue(struct REGISTERS_SEG *seg,uint16_t ofs) {
	register uint8_t i;
	DWORD t;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_R();
		}
	else
#endif
	t=MAKE20BITS(seg->s.x,ofs);

/*	if(t==0x400) {
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read 400 : %02X \n",timeGetTime(),
				_ip,ram_seg[0x400]);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
		}*/

#if DEBUG_TESTSUITE
		i=ram_seg[t];
#else
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
#ifdef _DEBUG
#ifdef DEBUG_VGA
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  readRAMV %08X : %02X  memCfg=%u,rMemMd=%X, rdMd=%u,gMode=%02X, planes=%X\n",timeGetTime(),
				_ip,t,i,(VGAgraphReg[6] & 0x0c) >> 2,VGAseqReg[4] & 0xf,
				VGAgraphReg[4] & 3,VGAgraphReg[5],
				VGAseqReg[2] & 0xf);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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

skippa:
#endif
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x:%04x  UNIMPLEMENTED memoryR B %08X : %02X \n",timeGetTime(),
#ifdef EXT_80286
				_msw.PE ? _cs->d.BaseH : _cs->s.x,_ip,t,i);
#else
				_cs,_ip,t,i);
#endif
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}

		}
  
#endif
	return i;
	}

uint8_t _fastcall InValue(uint16_t t) {
	register uint8_t i;

#if DEBUG_TESTSUITE
	return 0xff;
#else
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
//					if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
//						i8237RegR[8] |= ((1 << 0) << 4);  // request??...
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
#ifdef _DEBUG
#ifdef DEBUG_8237
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 0%X  : %02X  \n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      break;

    case 2:         // 20-21 PIC 8259 controller
			// DTK 286 scrive a 6a a 22 e legge a 23 al boot!
#if defined(EXT_80286)
      t &= 0x3;
#else
      t &= 0x1;
#endif
      switch(t) {
        case 0:
					switch(i8259OCW[2] & B8(00000011)) {		// 
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
/*					if(i8259RegW[0] == B8(00000011))
						i=i8259RegW[1];
					else 

					// se no si blocca... PCXTBIOS */
						i=i8259RegR[1];



					i8259IMR;
          break;
        }
#ifdef _DEBUG
#ifdef DEBUG_8259
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 02%X  : %02X  \n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      break;

    case 4:         // 40-43 PIT 8253/4 controller (incl. speaker)
		// https://en.wikipedia.org/wiki/Intel_8253
      t &= 0x3;
      switch(t) {
        case 0:
					// la cosa dei Latch non è ancora perfetta credo... non si capisce bene
          if(!i8253ModeSel) {
	          i8253RegR[0]=LOBYTE(i8253TimerR[0]);
            i8253TimerL[0]=MAKEWORD(LOBYTE(i8253TimerR[0]),HIBYTE(i8253TimerL[0]));
//            i=LOBYTE(i8253RegR[0]);  //OCCHIO non deve mai essere >2
						}
          else {
	          i8253RegR[0]=HIBYTE(i8253TimerR[0]);
            i8253TimerL[0]=MAKEWORD(LOBYTE(i8253TimerL[0]),HIBYTE(i8253TimerR[0]));
						}
          i=i8253RegR[0];
          i8253ModeSel++;
          i8253ModeSel &= 1;
          break;
        case 1:
          if(!i8253ModeSel) {
            i8253TimerL[1]=MAKEWORD(LOBYTE(i8253TimerR[1]),HIBYTE(i8253TimerL[1]));
	          i8253RegR[1]=LOBYTE(i8253TimerL[1]);
//            i=LOBYTE(i8253RegR[0]);  //OCCHIO non deve mai essere >2
						}
          else {
            i8253TimerL[1]=MAKEWORD(LOBYTE(i8253TimerL[1]),HIBYTE(i8253TimerR[1]));
	          i8253RegR[1]=HIBYTE(i8253TimerL[1]);
						}
          i=i8253RegR[1];
          i8253ModeSel++;
          i8253ModeSel &= 1;
          break;
        case 2:
          if(!i8253ModeSel) {
	          i8253RegR[2]=LOBYTE(i8253TimerR[2]);
            i8253TimerL[2]=MAKEWORD(LOBYTE(i8253TimerR[2]),HIBYTE(i8253TimerL[2]));
//          i=LOBYTE(i8253RegR[2]);  //OCCHIO non deve mai essere >2
						}
          else {
	          i8253RegR[2]=HIBYTE(i8253TimerR[2]);
            i8253TimerL[2]=MAKEWORD(LOBYTE(i8253TimerL[2]),HIBYTE(i8253TimerR[2]));
						}
          i=i8253RegR[2];
          i8253ModeSel++;
          i8253ModeSel &= 1;
          break;
        case 3:
          i=i8253RegR[3];		// questo in effetti vale solo per 8254, non 8253!
          break;
        }
#ifdef _DEBUG
#ifdef DEBUG_8253
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 4%X  : %02X \n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      break;

    case 6:        // 60-6F 8042/8255 keyboard; speaker; config byte    https://www.phatcode.net/res/223/files/html/Chapter_20/CH20-2.html
#ifndef PCAT		// bah, chissà... questo dovrebbe usare 8255 e AT 8042...
      t &= 0x3;
      switch(t) {
        case 0:
					if(KBStatus & B8(00000001))		// SOLO se c'è stato davvero un tasto! (altrimenti siamo dopo reset o clock ENable
						i8255RegR[0]=Keyboard[0];
					else
						i8255RegR[0]=0;
//          KBStatus &= ~B8(00000001);

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
//          i=B8(01101100);			// bits 2,3 memory size (64K bytes); b7:6 #floppies; b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25
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

//					i8255RegR[2] = B8(00000000);		// test

#ifdef PC_IBM5150
			if(i8253Mode[2] & B8(10000000))
				i8255RegR[2] |= B8(00010000);			// loopback cassette
			else
				i8255RegR[2] &= ~B8(00010000);			// 
#endif

          i=i8255RegR[2];   // 


          break;
        }

#else

			// https://wiki.osdev.org/A20_Line
      t &= 0x7;
      switch(t) {
        case 0:
          if(i8042RegW[1]==0xAA) {      // self test
						i8042RegR[0]=Keyboard[i8042OutPtr-1];
            }
          else if(i8042RegW[1]==0xAB) {      // diagnostics
            i8042RegR[0]=B8(00000000);
            }
/*          else if(i8042RegW[1]==0xAE) {      // enable
            KBCommand &= ~B8(00010000);
            i8042RegR[0]=B8(00000000);
            }
          else if(i8042RegW[1]==0xAD) {      // disable
            KBCommand |= B8(00010000);
            i8042RegR[0]=B8(00000000);
            }*/
          else if(i8042RegW[1]==0x20) {
            i8042RegR[0]=KBCommand;
            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {   // boh, non risulta...
            i8042RegR[0]=KBRAM[i8042RegW[1] & 0x1f];
            }
          else if(i8042RegW[1]==0xA1) {			// read data AMI286
						i8042RegR[0]=Keyboard[i8042OutPtr-1];
            }
          else if(i8042RegW[1]==0xC0) {			// read data
						i8042RegR[0]=Keyboard[i8042OutPtr-1];
            }
          else if(i8042RegW[1]==0xC1) {			// read data 4 lower bit port of ?? jumper?? into 4..7
						i8042RegR[0]=Keyboard[i8042OutPtr-1];
            }
          else if(i8042RegW[1]==0xC2) {			// read data 4 upper bit port of ?? jumper?? into 4..7
						i8042RegR[0]=Keyboard[i8042OutPtr-1];
            }
          else if(i8042RegW[1]==0xD0) {			// read data
						if(KBCommand & B8(00010000))			// se disabilitata
							i8042RegR[0]=i8042Output;
						else
							i8042RegR[0]=i8042Output;				// boh...
            }
          else if(i8042RegW[1]==0xEE) {     //echo
            i8042RegR[0]=0xee;
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
            }
          else {
						if(KBStatus & B8(00000001))		// SOLO se c'è stato davvero un tasto! (altrimenti siamo dopo reset o clock ENable
							i8042RegR[0]=Keyboard[i8042OutPtr-1];
            }


					if(i8042OutPtr) {
						if(!--i8042OutPtr) 
		          KBStatus &= ~B8(00000001);
					// aspetta 2 byte dopo AA... non so perché GESTIRE?  ah forse ID
						}




//          i8042RegW[1]=0;     // direi, così la/le prossima lettura restituisce il tasto...



          i=i8042RegR[0];



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
          KBStatus &= ~B8(00000010);			// cmq i dati son stati usati :)
          break;
        }
#endif
#ifdef _DEBUG
#ifdef DEBUG_KB
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 6%X  : %02X \n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
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
              i146818RegR[1]=currentDateTime.tm_sec;
              break;
              // in mezzo c'è Alarm...
            case 2:
              i146818RegR[1]=currentDateTime.tm_min;
              break;
            case 4:
              i146818RegR[1]=currentDateTime.tm_hour;
              break;
            case 6:
              i146818RegR[1]=currentDateTime.tm_wday;
              break;
            case 7:
              i146818RegR[1]=currentDateTime.tm_mday;
              break;
            case 8:
              i146818RegR[1]=currentDateTime.tm_mon;
              break;
            case 9:
              i146818RegR[1]=currentDateTime.tm_year;
              break;
            case 10:
              i146818RegR[1]=(i146818RAM[10] & B8(10000000)) | B8(00100110);			// dividers + UIP
              break;
            case 11:
              i146818RegR[1]=B8(00000010);			// 12/24: 24; BCD mode
              break;
            case 12:
              i146818RegR[1]=i146818RAM[12] = 0;   // flag IRQ
              break;
            case 13:
              i146818RegR[1]=i146818RAM[13];
							i146818RAM[13] |= B8(10000000);			// VBat ok dopo una lettura
              break;

            case 14:
//              i146818RegR[1]=B8(00000000);			// diag in GLATick...
//              break;
#ifdef PCAT
            case 15:			// flag, Award scrive 6		https://www.rcollins.org/ftp/source/pmbasics/tspec_a1.l1
#endif
            default:      // qua ci sono i 4 registri e poi la RAM
              i146818RegR[1]=i146818RAM[i146818RegW[0] & 0x3f];

/*							if((i146818RegW[0] & 0x3f) == 14 || (i146818RegW[0] & 0x3f)==15)
							{	char myBuf[256],myBuf2[32];
							wsprintf(myBuf,"%u RTCRAM R: ",timeGetTime());
							for(i=0; i<64; i++) {		// 
								wsprintf(myBuf2,"%02X ",i146818RAM[i]);
								strcat(myBuf,myBuf2);
								if(i==9 || i==13)
									strcat(myBuf," ");
								}
							strcat(myBuf,"\r\n");
							_lwrite(spoolFile,myBuf,strlen(myBuf));
							}*/

              break;
            }
          i=i146818RegR[1];
          break;
        }
#ifdef _DEBUG
#ifdef DEBUG_RTC
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 7%X [%x]  : %02X  \n",timeGetTime(),
				_ip,t,i146818RegW[0] & 0x3f,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
#ifdef _DEBUG
#ifdef DEBUG_8237
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 8%X  : %02X  \n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
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
					switch(i8259OCW2[2] & B8(00000011)) {		// 
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
#ifdef _DEBUG
#ifdef DEBUG_8259
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at A%X  : %02X  \n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
//					if(!(i8237Reg2W[8] & B8(00000001))) {			// controller enable
//						i8237Reg2R[8] |= ((1 << 0) << 4);  // request??...
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
#ifdef _DEBUG
#ifdef DEBUG_8237
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at C%X  : %02X  \n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
    case 0x1f:      // 1f0-1ff   // hard disc 0 su AT		http://www.techhelpmanual.com/897-at_hard_disk_ports.html   http://www.techhelpmanual.com/892-i_o_port_map.html
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
          i=currentDateTime.tm_sec;
          break;
        case 2:
          i=currentDateTime.tm_min;
          break;
        case 3:
          i=currentDateTime.tm_hour;
          break;
        case 4:
          i=currentDateTime.tm_mday;
          break;
        case 5:
          i=currentDateTime.tm_mon;
          break;
        case 6:
          i=currentDateTime.tm_year;
          break;
        case 7:
          break;
        default:      
          break;
        }
      break;
      
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


					switch(HDContrRegW[0] & B8(11100000)) {		// i 3 bit alti sono class, gli altri il comando
						case B8(00000000):
							disk=HDFIFOW[1] & B8(00100000) ? 1 : 0;
							switch(HDContrRegW[0] & B8(00011111)) {		// i 3 bit alti sono class, gli altri il comando
								case 0x00:			// test ready
									if(!disk)
										HDFIFOR[0]=B8(00000000);
									else
										HDFIFOR[0]=B8(00100000);		// errore su disco 2 ;) se ne sbatte

endHDcommandWithIRQ:
									if(hdControl & B8(00000010))
											i8259IRR |= B8(00100000);		// simulo IRQ...
									hdState |= B8(00100000);		// IRQ pending
endHDcommand:
									hdState &= ~B8(00000010);		// passo a read
									hdState &= ~B8(00001000);			// BSY
									HDFIFOPtrW=HDFIFOPtrR=0;
									break;
								case 0x01:			// recalibrate

									if(!disk)
										HDFIFOR[0]=B8(00000000);
									else
										HDFIFOR[0]=B8(00100010);		// errore su disco 2 ;) QUA VA! solo che poi resta a provarci 1000 volte...

									// ritardo...
									hdCylinder=0;

									if(HDFIFOR[0]) {
										HDFIFOR[1]=4; HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4]=0;		// ci sarebbero cilindro ecc dell'errore
										}
									else {
										HDFIFOR[1]=HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4]=0;
										}

//							hdState |= B8(00100000);		// IRQ pending
//										if(hdControl & B8(00000010))		// sparisce C anche se il disco rimane visibile... strano...
//											i8259IRR |= B8(00100000);		// simulo IRQ...
									goto endHDcommandWithIRQ;
									break;
								case 0x02:			// reserved
									break;
								case 0x03:			// request sense

									if(HDFIFOR[0]) {		// se errore ultima op...
//											HDFIFOR[1]=HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4];		// ci sarebbero cilindro ecc dell'errore
										}
									else {
//											HDFIFOR[1]=HDFIFOR[2]=HDFIFOR[3]=HDFIFOR[4];
										}


									goto endHDcommand;
									break;
								case 0x04:			// format drive

#ifdef _DEBUG
#ifdef DEBUG_HD
						{
						char myBuf[256],mybuf2[32];
						wsprintf(myBuf,"****F %s  D%u  Drive \n",_strtime(mybuf2),
							disk,hdCylinder,hdHead,hdSector,MAKELONG(addr,DMApage[3]),n,getHDiscSector(disk)*SECTOR_SIZE);
						_lwrite(spoolFile,myBuf,strlen(myBuf));
						}
#endif
#endif

									goto endHDcommand;

									break;
								case 0x05:			// read verify

									hdState |= B8(00010000);			// attivo DMA (?? su hdControl<1>
									if(!disk) {
										hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
										hdHead=HDFIFOW[1] & B8(00011111);
										hdSector=HDFIFOW[2] & B8(00011111);
										HDFIFOW[4];	HDFIFOW[5];		// 
										addr=i8237DMACurAddr[3]=i8237DMAAddr[3];
										i8237DMACurLen[3]=i8237DMALen[3];
										n=i8237DMALen[3]   +1;		// 1ff ...
										}

#ifdef _DEBUG
#ifdef DEBUG_HD
					{
					char myBuf[256],mybuf2[32];
					wsprintf(myBuf,"****V %s  D%u  C%u H%u S%u , %06x: %04x, %08x \n",_strtime(mybuf2),
						disk,hdCylinder,hdHead,hdSector,MAKELONG(addr,DMApage[3]),n,getHDiscSector(disk)*SECTOR_SIZE);
					_lwrite(spoolFile,myBuf,strlen(myBuf));
					}
#endif
#endif



									if(!disk)		// 
										HDFIFOR[0]=B8(00000000);
									else
										HDFIFOR[0]=B8(00100010);		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~B8(00000010);		// passo a read (lungh.diversa qua
									hdState &= ~B8(00010000);			// disattivo DMA 

									goto endHDcommandWithIRQ;
									break;
								case 0x06:			// format track
							hdState |= B8(00010000);			// attivo DMA (?? su hdControl<1>

									hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
									hdHead=HDFIFOW[1] & B8(00011111);

#ifdef _DEBUG
#ifdef DEBUG_HD
					{
					char myBuf[256],mybuf2[32];
					wsprintf(myBuf,"****F %s  D%u  C%u H%u S%u , %06x: %04x, %08x \n",_strtime(mybuf2),
						disk,hdCylinder,hdHead,hdSector,MAKELONG(addr,DMApage[3]),n,getHDiscSector(disk)*SECTOR_SIZE);
					_lwrite(spoolFile,myBuf,strlen(myBuf));
					}
#endif
#endif

									if(!disk)
										HDFIFOR[0]=B8(00000000);
									else
										HDFIFOR[0]=B8(00100010);		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~B8(00010000);			// disattivo DMA 

									goto endHDcommandWithIRQ;
									break;
								case 0x07:			// format bad track

									goto endHDcommand;
									break;
								case 0x08:			// read

									hdState |= B8(00001000);			// BSY

									if(!disk) {

										hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
										hdHead=HDFIFOW[1] & B8(00011111);
										hdSector=HDFIFOW[2] & B8(00011111);
										HDFIFOW[4];	HDFIFOW[5];		// 
										addr=i8237DMACurAddr[3]=i8237DMAAddr[3];
										i8237DMACurLen[3]=i8237DMALen[3];
										n=i8237DMALen[3]   +1;		// 1ff ...
										do {
											encodeHDiscSector(disk);

											if(hdControl & 1) {		// scegliere se DMA o PIO in base a flag HDcontrol qua...
												if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
													i8237RegR[8] |= ((1 << 3) << 4);  // request??...
													if(!(i8237RegW[15] & (1 << 3)))	{	// mask...
														switch(i8237Mode[3] & B8(00001100)) {		// DMA mode
															case B8(00000000):			// verify
																memcmp(&ram_seg[MAKELONG(addr,DMApage[3])],sectorData,SECTOR_SIZE);
																break;
															case B8(00001000):			// write
																memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[3])],SECTOR_SIZE);
																break;
															case B8(00000100):			// read
																memcpy(&ram_seg[MAKELONG(addr,DMApage[3])],sectorData,SECTOR_SIZE);
																break;
															}
														}
													i8237Mode[3] & B8(00001100) == B8(00000100);		// Read
													i8237Mode[3] & B8(11000000) == B8(01000000);		// Single mode
													i8237RegR[8] |= 1 << 3;  // fatto, TC 3
													i8237RegR[8] &= ~((1 << 3) << 4);  // request finita
													i8237RegR[13]=ram_seg[SECTOR_SIZE-1]; // ultimo byte trasferito :)
													}
												}
											else {
												}

#ifdef _DEBUG
#ifdef DEBUG_HD
					{
					char myBuf[256],mybuf2[32];
					wsprintf(myBuf,"****R %s  D%u  C%u H%u S%u , %06x: %04x, %08x \n",_strtime(mybuf2),
						disk,hdCylinder,hdHead,hdSector,MAKELONG(addr,DMApage[3]),n,getHDiscSector(disk)*SECTOR_SIZE);
					_lwrite(spoolFile,myBuf,strlen(myBuf));
					}
#endif
#endif

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
										HDFIFOR[0] = B8(10000000);		//ST0: B7=address valid, B5:4=error type, B3:0=error code
										HDFIFOR[1] = (disk ? B8(00100000) : B8(00000000)) | (hdHead & 31);
										HDFIFOR[2] = hdCylinder & 0xff; //		//C
										HDFIFOR[3] = (hdCylinder >> 8) | hdSector;
										}
									else
										HDFIFOR[0]=B8(00100000);		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~B8(00010000);			// disattivo DMA 
//											goto endHDcommand;
									goto endHDcommandWithIRQ;
									break;
								case 0x09:			// reserved
									break;
								case 0x0a:			// write

									hdState |= B8(00001000);			// BSY

									if(!disk) {
										hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
										hdHead=HDFIFOW[1] & B8(00011111);
										hdSector=HDFIFOW[2] & B8(00011111);
										HDFIFOW[4];	HDFIFOW[5];		// 
										addr=i8237DMACurAddr[3]=i8237DMAAddr[3];
										i8237DMACurLen[3]=i8237DMALen[3];
										n=i8237DMALen[3]   +1;		// 1ff ...
										do {
											msdosHDisk=((uint8_t*)MSDOS_HDISK)+getHDiscSector(disk)*SECTOR_SIZE;

											if(hdControl & 1) {		// scegliere se DMA o PIO in base a flag HDcontrol qua...
												if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
													i8237RegR[8] |= ((1 << 3) << 4);  // request??...
													if(!(i8237RegW[15] & (1 << 3)))	{	// mask...
														switch(i8237Mode[3] & B8(00001100)) {		// DMA mode
															case B8(00000000):			// verify
																memcmp(&ram_seg[MAKELONG(addr,DMApage[3])],msdosHDisk,SECTOR_SIZE);
																break;
															case B8(00001000):			// write
																memcpy(msdosHDisk,&ram_seg[MAKELONG(addr,DMApage[3])],SECTOR_SIZE);
																break;
															case B8(00000100):			// read
																memcpy(&ram_seg[MAKELONG(addr,DMApage[3])],msdosHDisk,SECTOR_SIZE);
																break;
															}
														}
													i8237Mode[3] & B8(00001100) == B8(00000100);		// Read
													i8237Mode[3] & B8(11000000) == B8(01000000);		// Single mode
													i8237RegR[8] |= 1 << 3;  // fatto, TC 3
													i8237RegR[8] &= ~((1 << 3) << 4);  // request finita
													i8237RegR[13]=ram_seg[SECTOR_SIZE-1]; // ultimo byte trasferito :)
													}
												}
											else {
												}

#ifdef _DEBUG
#ifdef DEBUG_HD
					{
					char myBuf[256],mybuf2[32];
					wsprintf(myBuf,"****W %s  D%u  C%u H%u S%u , %06x: %04x, %08x \n",_strtime(mybuf2),
						disk,hdCylinder,hdHead,hdSector,MAKELONG(addr,DMApage[3]),n,getHDiscSector(disk)*SECTOR_SIZE);
					_lwrite(spoolFile,myBuf,strlen(myBuf));
					}
#endif
#endif

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
										HDFIFOR[0] = B8(00000000);		//ST0: B7=address valid, B5:4=error type, B3:0=error code
										HDFIFOR[1] = (disk ? B8(00100000) : B8(00000000)) | (hdHead & 31);
										HDFIFOR[2] = hdCylinder & 0xff; //		//C
										HDFIFOR[3] = (hdCylinder >> 8) | hdSector;
										}
									else
										HDFIFOR[0]=B8(00100000);		// errore su disco 2 ;) MA NON SPARISCE

									hdState &= ~B8(00010000);			// disattivo DMA 
									goto endHDcommandWithIRQ;
									break;
								case 0x0b:			// seek

									hdState |= B8(00001000);			// BSY

									if(!disk)
										HDFIFOR[0]=B8(00000000);
									else
										HDFIFOR[0]=B8(00100000);		// errore su disco 2 ;) MA NON SPARISCE

									hdCylinder=MAKEWORD(HDFIFOW[3],HDFIFOW[2] >> 6);
									hdHead=HDFIFOW[1] & B8(00011111);
									// ritardo...

									goto endHDcommandWithIRQ;
									break;
								case 0x0c:			// initialize
									if(HDFIFOPtrW==14) {

										if(!disk)
											HDFIFOR[0]=B8(00000000);
										else
											HDFIFOR[0]=B8(00100000   /*00100010 muore tutto*/);		// errore su disco 2 ;) MA NON SPARISCE

										// restituire 4 in Error code poi (però tanto non lo chiede, non arriva SENSE...
//										HDFIFO[1]=4;

										hdState &= ~B8(00000010);		// passo a read (lungh.diversa qua
										goto endHDcommandWithIRQ;
										}
									hdState &= ~B8(00001111);		// 
									break;
								case 0x0d:			// read ECC burst
									goto endHDcommand;
									break;
								case 0x0e:			// read data from sector buffer

										HDFIFOR[0]=0;
									goto endHDcommandWithIRQ;
									break;
								case 0x0f:			// write data to sector buffer

										HDFIFOR[0]=0;
									goto endHDcommandWithIRQ;
									break;
								default:		// invalid, 
									break;
								}
							break;
						case B8(00100000):
							break;
						case B8(01000000):
							break;
						case B8(01100000):
							break;
						case B8(10000000):
							break;
						case B8(10100000):
							break;
						case B8(11000000):
							break;
						case B8(11100000):
							disk=HDFIFOW[1] & B8(00100000) ? 1 : 0;
							switch(HDContrRegW[0] & B8(00011111)) {		// i 3 bit alti sono class, gli altri il comando
								case 0x00:			// RAM diagnostic
//											hdState=B8(00001101);			// mah; e basare DMA su hdControl<1>


									HDFIFOR[0]=B8(00000000);									// ok (vuole questo, 20h pare ok e 30h sarebbe ram error
									goto endHDcommandWithIRQ;
									break;
								case 0x01:			// reserved
									break;
								case 0x02:			// reserved
									break;
								case 0x03:			// drive diagnostic

									HDFIFOR[0]=B8(00000000);									// ok
									goto endHDcommandWithIRQ;
									break;
								case 0x04:			// controller internal diagnostic

									HDFIFOR[0]=B8(00000000);									// ok(vuole questo, 20h pare ok e 30h sarebbe ram error
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
					if(!(hdState & B8(00000001)))
						hdState |= B8(00000001);			// handshake

					hdState |= B8(00100000);		// IRQ pending
          break;
        case 2:			// READ DRIVE CONFIGURATION INFO
					HDContrRegR[2]=B8(00001111);			// 10MB entrambi (00=5, 01=24, 10=15, 11=10
					hdState &= ~B8(00100010);			// input, no IRQ
          break;
        case 3:			// not used
          break;
				}
			i=HDContrRegR[t];
#ifdef _DEBUG
#ifdef DEBUG_HD
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 32%X  : %02X  cmd=%02X, fifoW=%u fifoR=%u\n",timeGetTime(),
				_ip,t,i,HDContrRegW[1],HDFIFOPtrW,HDFIFOPtrR);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
			break;

    case 0x37:    // 370-377 (secondary floppy, AT http://www.techhelpmanual.com/896-diskette_controller_ports.html); 378-37A  LPT1
      t &= 3;
			i=LPT[0][t];		// per ora, al volo!
      switch(t) {
        case 0:
//          i=(PORTE & 0xff) & ~B8(00010000);
//          i |= PORTGbits.RG6 ? B8(00010000) : 0;   // perché LATE4 è led...
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
#ifdef _DEBUG
#ifdef DEBUG_VGA
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 3B/C/D%X  : %02X  c\n",timeGetTime(),
				_ip,t,i);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
				BYTE disk;
				int n,addr;

				case 0:		// solo AT http://isdaman.com/alsos/hardware/fdc/floppy.htm
				case 1:
					break;

        case 2:   // Write: digital output register (DOR)   MOTD MOTC MOTB MOTA DMA ENABLE SEL1 SEL0
					// questo registro in effetti non appartiene al uPD765 ma è esterno...
          break;
        case 4:   // Read-only: main status register (MSR)  REQ DIR DMA BUSY D C B A
					disk=FloppyContrRegW[0] & 3;
//					FloppyContrRegR[0] |= B8(10000000);		// DIO ready for data exchange
					// nella prima word FIFO i 3 bit più bassi sono in genere HD US1 US0 ossia Head e Floppy in uso (0..3)
					//NB è meglio effettuare le operazioni qua, alla rilettura, che non alla scrittura dell'ultimo parm (v anche HD)
					// perché pare che tipo il DMA nei HD viene attivato DOPO la conferma della rilettura (qua pare di no ma ok)
					switch(FloppyContrRegW[1] & B8(00011111)) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
						case 0x06:			// read data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];

								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
	//								getDiscSector();
									encodeDiscSector(disk);


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
									if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegW[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & B8(00001100)) {		// DMA mode
												case B8(00000000):			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case B8(00001000):			// write
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case B8(00000100):			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & B8(00001100) == B8(00000100);		// Read
										i8237Mode[2] & B8(11000000) == B8(01000000);		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto, TC 2
										i8237RegR[8] &= ~((1 << 2) << 4);  // request finita
										i8237RegR[13]=ram_seg[MAKELONG(addr,DMApage[2])+(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			char myBuf[256],mybuf2[32];
			wsprintf(myBuf,"*** %s  D%u  T%u H%u S%u , %06x: %04x, %08x \n",_strtime(mybuf2),
				disk,floppyTrack[disk],floppyHead[disk],floppySector[disk],MAKELONG(addr,DMApage[2]),n,getDiscSector(disk)*SECTOR_SIZE);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & B8(10000000)) {		// solo se MT?

											floppyHead[disk]++;

											if(floppyHead[disk]>=2) {
												floppyHead[disk]=0;
												floppyTrack[disk]++;
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
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
//								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? B8(10000000) : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								//Bit 7 (value = 0x80) is set if the floppy had too few sectors on it to complete a read/write.
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								if(!(floppyState[disk] & B8(10000000)))		// forzo errore se disco non c'è...
									FloppyFIFO[1] |= B8(11000000);

endFDcommandWithIRQ2:
								FloppyFIFOPtrR=1;

endFDcommandWithIRQ:
								FloppyContrRegR[0]=B8(11000000);
endFDcommandWithIRQ3:
								floppyState[disk] |= B8(01000000);		// passo a read
								FloppyFIFOPtrW=0;
								if(FloppyContrRegW[0] & B8(00001000 /*in asm dice che questo è "Interrupt ON"*/))
									setFloppyIRQ(DEFAULT_FLOPPY_DELAYED_IRQ);
								floppylastCmd[disk]=FloppyContrRegW[1] & B8(00011111);
								}

							else if(FloppyFIFOPtrR) {		// NO! glabios lo fa, PCXTbios no: pensavo ci passasse più volte dopo la fine preparazione, ma non è così quindi si può togliere if
								//FloppyContrRegR[0]=FloppyFIFO[FloppyFIFOPtrR++];togliere
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write

//								FloppyContrRegR[0]=B8(10000000) | (floppyState[disk] & B8(01000000));

									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x02:			// read track
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=1 /*FloppyFIFO[4]*/;
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
									encodeDiscSector(disk);

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegW[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & B8(00001100)) {		// DMA mode
												case B8(00000000):			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case B8(00001000):			// write
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case B8(00000100):			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & B8(00001100) == B8(00000100);		// read
										i8237Mode[2] & B8(11000000) == B8(01000000);		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto, TC 2
										i8237RegR[8] &= ~((1 << 2) << 4);  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// 
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
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x0c:			// read deleted
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
									encodeDiscSector(disk);


									// SIMULO DMA qua...
									if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegW[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & B8(00001100)) {		// DMA mode
												case B8(00000000):			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case B8(00001000):			// write
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case B8(00000100):			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & B8(00001100) == B8(00000100);		// read
										i8237Mode[2] & B8(11000000) == B8(01000000);		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto, TC 2
										i8237RegR[8] &= ~((1 << 2) << 4);  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & B8(10000000)) {		// solo se MT?

											floppyHead[disk]++;

											if(floppyHead[disk]>=2) {
												floppyHead[disk]=0;
												floppyTrack[disk]++;
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
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
	              goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x0a:			// read ID
							if(FloppyFIFOPtrW==2 && !FloppyFIFOPtrR) {
								FloppyFIFO[6] = FloppyFIFO[5];
								FloppyFIFO[5] = FloppyFIFO[4];
								FloppyFIFO[4] = FloppyFIFO[3];
								FloppyFIFO[3] = FloppyFIFO[2];
								FloppyFIFO[0] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[1] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[2] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyContrRegR[0]=B8(11010000);
                goto endFDcommandWithIRQ;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x05:			// write data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*SECTOR_SIZE;

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegW[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & B8(00001100)) {		// DMA mode
												case B8(00000000):			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case B8(00001000):			// write
													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case B8(00000100):			// read
													memcpy(msdosDisk,sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & B8(00001100) == B8(00001000);		// Write
										i8237Mode[2] & B8(11000000) == B8(01000000);		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto, TC 2
										i8237RegR[8] &= ~((1 << 2) << 4);  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & B8(10000000)) {		// solo se MT?

											floppyHead[disk]++;

											if(floppyHead[disk]>=2) {
												floppyHead[disk]=0;
												floppyTrack[disk]++;
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
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x0d:			// format track
							if(FloppyFIFOPtrW==6) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								FloppyFIFO[2];		// bytes per sector	ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								FloppyFIFO[3];		// sector per track
								FloppyFIFO[4];		// gap3
								FloppyFIFO[5];		// filler byte
								floppySector[disk]=1;
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*SECTOR_SIZE;

									// SIMULO DMA qua... o forse no?
									if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegW[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & B8(00001100)) {		// DMA mode
												case B8(00000000):			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[2]);
													break;
												case B8(00001000):			// write
// bah non è chiaro, ma SET funziona!													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[2]);
													memset(msdosDisk,FloppyFIFO[5],0x80 << FloppyFIFO[2]);
													break;
												case B8(00000100):			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[2]);
													break;
												}
											}
										i8237Mode[2] & B8(00001100) == B8(00001000);		// Write
										i8237Mode[2] & B8(11000000) == B8(01000000);		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto, TC 2
										i8237RegR[8] &= ~((1 << 2) << 4);  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[2])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									addr+=0x80 << FloppyFIFO[2];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									} while(floppySector[disk]<=FloppyFIFO[3]);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x09:			// write deleted data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*SECTOR_SIZE;

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & B8(00000001))) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegW[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & B8(00001100)) {		// DMA mode
												case B8(00000000):			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case B8(00001000):			// write
													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case B8(00000100):			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & B8(00001100) == B8(00001000);		// Write
										i8237Mode[2] & B8(11000000) == B8(01000000);		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto, TC 2
										i8237RegR[8] &= ~((1 << 2) << 4);  // request finita
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & B8(10000000)) {		// solo se MT?

											floppyHead[disk]++;

											if(floppyHead[disk]>=2) {
												floppyHead[disk]=0;
												floppyTrack[disk]++;
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
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x11:			// scan equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x19:			// scan low or equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x1d:			// scan high or equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x07:			// recalibrate
							if(FloppyFIFOPtrW==2) {
								// in FIFO[1] : x x x x x 0 US1 US0									00h
								disk=FloppyFIFO[1] & 3;


#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: RECALIBRATE\n",timeGetTime()
				);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif


								FloppyContrRegR[0]=B8(00010000) | (1 << disk);					// busy
								// __delay_ms(12*floppyTrack[disk])
								floppyTrack[disk]=0;
								FloppyContrRegR[0]=B8(01000000);					// finito :)  NON 11000000 per phoenix manco PCXTbios...
								// si potrebbe provare a ritardare b7, mettendolo in floppytimer...

								FloppyFIFOPtrR=0;

//								if(!(floppyState[disk] & B8(10000000)))		// forzo errore se disco non c'è... se ne sbatte qua
//									FloppyFIFO[1] |= B8(11000000);

                goto endFDcommandWithIRQ3;

								}
							else if(FloppyFIFOPtrR) {
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x08:			// sense interrupt status	
							if(!FloppyFIFOPtrR) {

								switch(floppylastCmd[disk]) {		// https://wiki.osdev.org/Floppy_Disk_Controller#Sense_Interrupt
									case 0x0f: // seek
									case 0x07: // recalibrate
										FloppyFIFO[1] = B8(00100000) | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
										break;
									case 0xff: // uso io per reset
										FloppyFIFO[1] = B8(11000000) | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								// v. anomalia link froci del cazzo (OVVIAMENTE è strano, ma ESIGE 1100xxxx ... boh
								FloppyFIFO[1] &= B8(11100000); // vaffanculo
										break;
									default:
										FloppyFIFO[1] = B8(00000000) | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
										break;
									}


								FloppyFIFO[2] = floppyTrack[disk];
//								FloppyFIFO[3] = floppyTrack[disk];
								FloppyFIFOPtrR=1;
//								if(!(floppyState[disk] & B8(10000000)))		// forzo errore se disco non c'è... QUA dà errore POST!
//									FloppyFIFO[1] |= B8(11000000);

//								FloppyContrRegR[0]=B8(00000000);
								FloppyContrRegR[0]=B8(11000000); // con 11000000 phoenix si schianta a cazzo!! ma si blocca cmq...

								goto endFDcommandWithIRQ3;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 2) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							break;
						case 0x03:			// specify
							if(FloppyFIFOPtrW==3) {
								// in FIFO[1] : StepRateTime[7..4] | HeadUnloadTime[3..0]				20h
								// in FIFO[2] : HeadLoadTime[7..1] | NonDMAMode[0]							07h
								FloppyFIFOPtrR=0;

//								if(!(floppyState[disk] & B8(10000000)))		// forzo errore se disco non c'è... se ne sbatte qua
//									FloppyFIFO[0] |= B8(11000000);

                goto endFDcommandWithIRQ;
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x04:			// sense drive status
							if(FloppyFIFOPtrW==2) {
								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??
								FloppyFIFO[0] = B8(00100000) | (!floppyTrack ? B8(00010000) : 0) | 
									(FloppyFIFO[1] & B8(00000111));		//ST3: B7 Fault, B6 Write Protected, B5 Ready, B4 Track 0, B3 Two Side, B2 Head (side select), B1:0 Disk Select (US1:0)

//								if(!(floppyState[disk] & B8(10000000)))		// forzo errore se disco non c'è... se ne sbatte
//									FloppyFIFO[0] |= B8(11000000);

								FloppyFIFOPtrR=1;
                goto endFDcommandWithIRQ;
								}
							else if(FloppyFIFOPtrR) {		// NO! glabios lo fa, PCXTbios no: pensavo ci passasse più volte dopo la fine preparazione, ma non è così quindi si può togliere if
								if(FloppyFIFOPtrR > 1) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x1b:			// scan high or equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=B8(11010000);		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, metà se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark

                goto endFDcommandWithIRQ2;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x0f:			// seek (goto cylinder
							if(FloppyFIFOPtrW==3) {
								// in FIFO[1] : x x x x x Head US1 US0						00h
								// in FIFO[2] : New Cylinder Number								00h
								if(FloppyFIFO[2] < totalTracks) {
									FloppyContrRegR[0]=B8(00010000) | (1 << disk);					// busy
									floppyTrack[disk]=FloppyFIFO[2];
								// __delay_ms(12*tracce)
									FloppyContrRegR[0]=B8(10000000);					// finito :)
									}
								else {
									}
								FloppyFIFOPtrR=0;
								floppyState[disk] |= B8(01000000);		// passo a read cmq

//								if(!(floppyState[disk] & B8(10000000)))		// forzo errore se disco non c'è... se ne sbatte
//									FloppyFIFO[0] |= B8(11000000);

                goto endFDcommandWithIRQ3;
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? B8(10010000) : B8(10000000);		// busy
								}
							break;
						case 0x10:		// version
							if(!FloppyFIFOPtrR) {

								FloppyFIFO[1] = 0x90;		// 82077A
								FloppyFIFOPtrR=1;

                goto endFDcommandWithIRQ;
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 1) {
									floppyState[disk] &= ~B8(01000000);	// passo a write
									FloppyContrRegR[0]=B8(10000000);
									}
								else
									FloppyContrRegR[0]=B8(11010000);					// busy
								}
							break;
						case 0x20:		// lock...
							break;
						case 0x18:		// perpendicular
							break;
						case 0x14:		// dumpreg
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

							FloppyContrRegR[0]=B8(10000000)	|
								(floppyState[disk] & B8(01000000));
								// b7=1 ready!;	b6 è direction (0=write da CPU a FDD, 1=read); b4 è busy/ready=0

							floppyState[disk] |= B8(01000000);		// passo a read
							FloppyFIFOPtrW=0;
//							floppylastCmd[disk]=FloppyContrRegW[1] & B8(00011111);
							break;
						default:		// invalid, 
							FloppyFIFO[0];
//								FloppyContrRegR[4] |= B8(10000000);		// 
							if(!FloppyFIFOPtrR) {
								FloppyFIFO[1] = B8(10000000) | (FloppyFIFO[1] & B8(00000111));		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)


#pragma message("verificare ST0")

								if(FloppyContrRegW[0] & B8(00001000 /*in asm dice che questo è "Interrupt ON"*/))
									setFloppyIRQ(SEEK_FLOPPY_DELAYED_IRQ);
								// anche qua??


								floppylastCmd[disk]=FloppyContrRegW[1] & B8(00011111);
								}
							FloppyContrRegR[0]=FloppyFIFO[FloppyFIFOPtrR++];
							if(FloppyFIFOPtrR==1) {
								FloppyFIFOPtrW=0;
								floppyState[disk] |= B8(01000000);		// passo a read
								}
							break;
						}
					i=FloppyContrRegR[0];
          break;

        case 5:   // Read/Write: FDC command/data register
//        case 6:   // ************ TEST BUG GLABIOS 0.4
					disk=FloppyContrRegW[0] & 3;
					switch(FloppyContrRegW[1] & B8(00011111)) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
						case 0x01:
						case 0x02:
						case 0x05:
						case 0x06:
						case 0x08:
						case 0x09:
						case 0x0a:
						case 0x0c:
						case 0x0d:
						case 0x19:
						case 0x1b:
						case 0x1d:
						case 0x10:
							FloppyContrRegR[1]=FloppyFIFO[FloppyFIFOPtrR++];
							break;
						case 0x03:
						case 0x07:
						case 0x0f:
							FloppyContrRegR[1]=B8(11000000);
							break;
						default:
							FloppyContrRegR[1]=B8(10000000);
							break;
						}
					i=FloppyContrRegR[1];
          break;

				case 7:		// solo AT http://isdaman.com/alsos/hardware/fdc/floppy.htm
					if(floppyState[disk] & B8(00000010)) {			// disk changed
						i=B8(10000000);
						floppyState[disk] &= ~B8(00000010);
						}
					else
						i=B8(00000000);
          break;

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
							i8250Reg[2] |= B8(00000001);
							if(i8250Reg[1] & B8(00000001))			// se IRQ attivo
								i8259IRR |= B8(00010000);
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

#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read at 3F%X  : %02X  cmd=%02X, fifoW=%u fifoR=%u\n",timeGetTime(),
				_ip,t,i,FloppyContrRegW[1],FloppyFIFOPtrW,FloppyFIFOPtrR);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
  
#endif
	return i;
	}

uint16_t _fastcall InShortValue(uint16_t t) {
	register uint16_t i;

#if DEBUG_TESTSUITE
	return 0xffff;
#else
return MAKEWORD(InValue(t),InValue(t+1));			// per ora, v. cmq GLABios e trident vga
	return i;
#endif
	}

uint16_t _fastcall GetShortValue(struct REGISTERS_SEG *seg,uint16_t ofs) {
	register union DWORD_BYTES i;
	DWORD t,t1;

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
	t1=t+1;
#endif
#ifdef EXT_80286
	if(!_msw.PE) {
		if(ofs==0xffff)			// real-mode exception
			;
		}
#endif
		}

#if DEBUG_TESTSUITE
	i.b.l=ram_seg[t];
	i.b.h=ram_seg[t1];
#else
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

skippa:
#endif
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x:%04x  UNIMPLEMENTED memoryR W %08X : %04X \n",timeGetTime(),
#ifdef EXT_80286
				_msw.PE ? _cs->d.BaseH : _cs->s.x,_ip,t,i.w.l);
#else
				_cs,_ip,t,i.w.l);
#endif
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
	}
  
#endif

/*	if(t==0x472) {
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  read 472 : %04X \n",timeGetTime(),
				_ip,i.w.l);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
		}*/



	return i.w.l;
	}


#ifdef EXT_80386
uint32_t GetIntValue(struct struct REGISTERS_SEG *seg,uint32_t ofs) {
	register DWORD_BYTES i;
	DWORD t,t1;

	if(_msw.PE) {
		ADDRESS_286_R();
		t1=t+1;
		}
	else {
		t=MAKE20BITS(seg->x,ofs);
		t1=t+1;
		}

#if DEBUG_TESTSUITE
	i.b.l=ram_seg[t];
	i.b.h=ram_seg[t+1];
	i.b.u=ram_seg[t+2];
	i.b.u2=ram_seg[t+3];
#else
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
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa:
#endif

	return i;
	}
#endif

uint8_t _fastcall GetPipe(struct REGISTERS_SEG *seg,uint16_t ofs) {
	DWORD t,t1;

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
	t1=t+1;
#endif
	}

#if DEBUG_TESTSUITE
	  Pipe1=ram_seg[t++];
		Pipe2.b.l=ram_seg[t++];
		Pipe2.b.h=ram_seg[t++];
		Pipe2.b.u=ram_seg[t];
//		Pipe2.b.u2=ram_seg[t];
#else
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
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa: ;
#endif
		}
#endif

	return Pipe1;
	}

uint8_t _fastcall GetMorePipe(struct REGISTERS_SEG *seg,uint16_t ofs) {
	DWORD t,t1;

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
	t1=t+1;
#endif
	}

#if DEBUG_TESTSUITE
    t+=4;
		Pipe2.bd[3]=ram_seg[t++];
		Pipe2.bd[4]=ram_seg[t++];
		Pipe2.bd[5]=ram_seg[t];
#else
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
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=1;

skippa: ;
#endif
		}
#endif

	return Pipe1;
	}

void _fastcall PutValue(struct REGISTERS_SEG *seg,uint16_t ofs,uint8_t t1) {
	DWORD t;

#ifdef EXT_80286
	if(_msw.PE) {
		ADDRESS_286_W();
		}
	else
#endif
	t=MAKE20BITS(seg->s.x,ofs);

/*	if(t==0xf400)
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write f400 : %02X \n",timeGetTime(),
				_ip,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}*/

#if DEBUG_TESTSUITE
	  ram_seg[t]=t1;
#else
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

#ifdef _DEBUG
#ifdef DEBUG_VGA
//if(t==0xb8140) 
{		char myBuf[256];
			wsprintf(myBuf,"%u: %04x  writeRAMV %08X : %02X  memCfg=%u,rMemMd=%X, wFSel=%u,rot=%u,wMode=%02X,sR=%X,%X planes=%X\n",timeGetTime(),
				_ip,t,t1,(VGAgraphReg[6] & 0x0c) >> 2,VGAseqReg[4] & 0xf,
				(VGAgraphReg[3] & 0x18) >> 3,VGAgraphReg[3] & 0x7, VGAgraphReg[5],VGAgraphReg[0],VGAgraphReg[1],

				VGAseqReg[2] & 0xf);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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

				// NO mask qua???

				break;
			case 0x02:
				t4=t1 & 8 ? 255 : 0;
				t3=t1 & 4 ? 255 : 0;
				t2=t1 & 2 ? 255 : 0;
				t1=t1 & 1 ? 255 : 0;
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
			case 0x00:


			case 0x01:
// non si capisce libro del cazzo programmatori col cancro


			case 0x02:
			case 0x03:
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
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=0;

skippa:
#endif
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x:%04x  UNIMPLEMENTED memoryW B %08X : %02X \n",timeGetTime(),
#ifdef EXT_80286
				_msw.PE ? _cs->d.BaseH : _cs->s.x,_ip,t,t1);
#else
				_cs,_ip,t,(WORD)t1);
#endif
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
		}
	}

void _fastcall PutShortValue(struct REGISTERS_SEG *seg,uint16_t ofs,uint16_t t2) {
	DWORD t,t1;

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
	t1=t+1;
#endif
#ifdef EXT_80286
	if(!_msw.PE) {
		if(ofs==0xffff)			// real-mode exception
			;
		}
#endif
	}

/*	if(t==0x472)
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write 472 W : %04X \n",timeGetTime(),
				_ip,t2);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}*/


#if DEBUG_TESTSUITE
	  ram_seg[t]=LOBYTE(t2);
	  ram_seg[t1]=HIBYTE(t2);
#else

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
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=0;

skippa:
#endif
			{
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x:%04x  UNIMPLEMENTED memoryW W %08X : %04X \n",timeGetTime(),
#ifdef EXT_80286
				_msw.PE ? _cs->d.BaseH : _cs->s.x,_ip,t,(WORD)t2);
#else
				_cs,_ip,t,(WORD)t2);
#endif
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
		}

	}


#ifdef EXT_80386
void _fastcall PutIntValue(struct struct REGISTERS_SEG *seg,uint32_t ofs,uint32_t t1) {
	register uint16_t i;
	DWORD t,t1;

	if(_msw.PE) {
		ADDRESS_286_W();
		t1=t+1;
		}
	else {
		t=MAKE20BITS(seg->x,ofs);
		t1=t+1;
		}

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

#if DEBUG_TESTSUITE
	  ram_seg[t++]=LOBYTE(LOWORD(t1));
	  ram_seg[t++]=HIBYTE(LOWORD(t1));
	  ram_seg[t++]=LOBYTE(HIWORD(t1));
	  ram_seg[t]=HIBYTE(HIWORD(t1));
#else
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
    Exception86.active=1;
    Exception86.addr=t;
    Exception86.descr.in=1;
    Exception86.descr.rw=0;

skippa:
#endif
		}
#endif
	}
#endif

void _fastcall OutValue(uint16_t t,uint8_t t1) {      // https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
	register uint16_t i;
  int j;

#if DEBUG_TESTSUITE
#else
  switch(t >> 4) {
    case 0:        // 00-1f DMA 8237 controller (usa il 2 per il floppy
    case 1:				// solo PS2 dice!
      t &= 0xf;
#ifdef _DEBUG
#ifdef DEBUG_8237
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 0%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
					if(t1 & B8(00000100))
						i8237RegW[15] |= (1 << (t1 & B8(00000011)));
					else
						i8237RegW[15] &= ~(1 << (t1 & B8(00000011)));
					i8237RegR[15]=i8237RegW[15];
		      i8237RegR[0xa]=i8237RegW[0xa]=t1;
					break;
				case 0xb:			// Mode
					i8237Mode[t1 & B8(00000011)]= t1 & B8(11111100);
					i8237DMACurAddr[t1 & B8(00000011)]=i8237DMAAddr[t1 & B8(00000011)];			// bah credo, qua
					i8237DMACurLen[t1 & B8(00000011)]=i8237DMALen[t1 & B8(00000011)];
		      i8237RegR[0xb]=i8237RegW[0xb]=t1;

					i8237RegR[8] |= ((1 << t1 & (B8(00000011))) << 4);  // request??... boh

					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					i8237FF=0;
		      i8237RegR[0xc]=i8237RegW[0xc]=t1;
					break;
				case 0xd:			// Reset
					memset(i8237RegR,0,sizeof(i8237RegR));
					memset(i8237RegW,0,sizeof(i8237RegW));
					i8237RegR[15]=i8237RegW[15] = B8(00001111);
					i8237RegR[8]=B8(00000000);
		      i8237RegR[0xd]=i8237RegW[0xd]=t1;
					break;
				case 0xe:			//  Clear Masks
					i8237RegR[15]=i8237RegW[15] &= ~B8(00001111);
					break;
				case 0xf:			//  Masks
					i8237RegR[15]=i8237RegW[15] = (t1 & B8(00001111));
					break;
				default:
		      i8237RegR[t]=i8237RegW[t]=t1;
					break;
				}
      break;

    case 2:         // 20-21 PIC 8259 controller
//https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
#if defined(EXT_80286)
      t &= 0x3;		// DTK legge/scrive a 22 23
#else
      t &= 0x1;
#endif
#ifdef _DEBUG
#ifdef DEBUG_8259
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 02%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      switch(t) {
        case 0:     // 0x20 per EOI interrupt, 0xb per "chiedere" quale... subito dopo c'è lettura a 0x20
          i8259RegW[0]=t1;      // 
					if(t1 & B8(00010000)) {// se D4=1 qua (0x10) è una sequenza di Init,
						i8259OCW[2] = 2;		// dice, default su lettura IRR
//						i8259RegR[0]=0;
						// boh i8259IMR=0xff; disattiva tutti
						i8259ICW[0]=t1;
						i8259ICWSel=2;		// >=1 indica che siamo in scrittura ICW
						}
					else if(t1 & B8(00001000)) {// se D3=1 qua (0x8) è una richiesta
						// questa è OCW3 
						i8259OCW[2]=t1;
						switch(i8259OCW[2] & B8(00000011)) {		// riportato anche sopra, serve (dice, in polled mode)
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
						switch(i8259OCW[1] & B8(11100000)) {
							case B8(00000000):// rotate pty in automatic (set clear
								break;
							case B8(00100000):			// non-spec, questo pulisce IRQ con priorità + alta in essere (https://rajivbhandari.wordpress.com/wp-content/uploads/2014/12/unitii_8259.pdf
//						i8259ISR=0;		// MIGLIORARE! pulire solo il primo
								for(i=1; i; i<<=1) {
									if(i8259ISR & i) {
										i8259ISR &= ~i;
										break;
										}
									}
//				      CPUPins &= ~DoIRQ;		// SEMPRE! non serve + v. di là
								break;
							case B8(01000000):			// no op
								break;
							case B8(01100000):			// 0x60 dovrebbe indicare uno specifico IRQ, nei 3bit bassi 0..7
								i8259ISR &= ~(1 << (t1 & 7));
								break;
							case B8(10000000):			// rotate pty in automatic (set
								break;
							case B8(10100000):			// rotate pty on non-specific
								break;
							case B8(11000000):			// set pty
								break;
							case B8(11100000):			// rotate pty on specific
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
							if(!(i8259ICW[0] & B8(00000001))) 		// ICW4 needed?
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
        }
      break;

    case 4:         // 40-43 PIT 8253/4 controller  https://en.wikipedia.org/wiki/Intel_8253
      t &= 0x3;
  //Set the PIT to the desired frequency
 	//Div = 1193180 / nFrequence;
 	//outb(0x43, 0xb6);
 	//outb(0x42, (uint8_t) (Div) );
 	//outb(0x42, (uint8_t) (Div >> 8));
#ifdef _DEBUG
#ifdef DEBUG_8253
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 4%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      switch(t) {
        case 0:
          switch((i8253Mode[0] & B8(00110000)) >> 4) {
            case 0:
              // latching operation... v. sotto
              break;
            case 2:
              i8253TimerW[0]=MAKEWORD(LOBYTE(i8253TimerW[0]),t1);
//							i8253Mode[0] |= B8(01000000);          // reloaded OCCHIO FORSE SOLO QUANDO BYTE ALTO, verificare
              i8253ModeSel = 0;
							goto reload0;
              break;
            case 1:
              i8253TimerW[0]=MAKEWORD(t1,HIBYTE(i8253TimerW[0]));
              i8253ModeSel = 0;
#ifndef PCAT		// per AWARD286, patch per ora (non si capisce, doc a cazzo
// solo byte alto?				
							goto reload0;
#else
							goto reload0_;
#endif
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[0]=MAKEWORD(LOBYTE(i8253TimerW[0]),t1);
              else
                i8253TimerW[0]=MAKEWORD(t1,HIBYTE(i8253TimerW[0]));
              i8253ModeSel++;
              i8253ModeSel &= 1;
							if(!i8253ModeSel) {		// qua, se carico entrambi i byte
reload0:
								i8253Mode[0] |= B8(01000000);          // reloaded
reload0_:
								i8253TimerL[0]=i8253TimerR[0]=i8253TimerW[0];
								if(((i8253Mode[0] & B8(00001110)) >> 1) == 3)
									i8253TimerR[0] &= 0xfffe;
								}
              break;
            }
          i8253RegW[0]=t1;
//          PR4=i8253TimerW[0] ? i8253TimerW[0] : 65535;
          break;
        case 1:
          switch((i8253Mode[1] & B8(00110000)) >> 4) {
            case 0:
              // latching operation... v. sotto
              break;
            case 2:
              i8253TimerW[1]=MAKEWORD(LOBYTE(i8253TimerW[1]),t1); 
              i8253ModeSel = 0;
							goto reload1;
              break;
            case 1:
              i8253TimerW[1]=MAKEWORD(t1,HIBYTE(i8253TimerW[1]));
              i8253ModeSel = 0;
#ifndef PCAT		// per AWARD286, patch per ora (non si capisce, doc a cazzo
// solo byte alto?		no, per Award 286
							goto reload1;
#else
							goto reload1_;
#endif
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[1]=MAKEWORD(LOBYTE(i8253TimerW[1]),t1);
              else
                i8253TimerW[1]=MAKEWORD(t1,HIBYTE(i8253TimerW[1]));
              i8253ModeSel++;
              i8253ModeSel &= 1;
							if(!i8253ModeSel) {		// qua, se carico entrambi i byte
reload1:
								i8253Mode[1] |= B8(01000000);          // reloaded
reload1_:
								i8253TimerL[1]=i8253TimerR[1]=i8253TimerW[1];
								if(((i8253Mode[1] & B8(00001110)) >> 1) == 3)
									i8253TimerR[1] &= 0xfffe;
								}
              break;
            }
          i8253RegW[1]=t1;
//          PR5=i8253TimerW[1] ? i8253TimerW[1] : 65535;
          break;
        case 2:
          switch((i8253Mode[2] & B8(00110000)) >> 4) {
            case 0:
              // latching operation... v. sotto
              break;
            case 2:
              i8253TimerW[2]=MAKEWORD(LOBYTE(i8253TimerW[2]),t1);  
              i8253ModeSel = 0;
							goto reload2;
              break;
            case 1:
              i8253TimerW[2]=MAKEWORD(t1,HIBYTE(i8253TimerW[2]));
              i8253ModeSel = 0;
#ifndef PCAT		// per AWARD286, patch per ora (non si capisce, doc a cazzo
// solo byte alto?		
							goto reload2;
#else
							goto reload2_;
#endif
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[2]=MAKEWORD(LOBYTE(i8253TimerW[2]),t1);
              else
                i8253TimerW[2]=MAKEWORD(t1,HIBYTE(i8253TimerW[2]));
              i8253ModeSel++;
              i8253ModeSel &= 1;
							if(!i8253ModeSel) {		// qua, se carico entrambi i byte
reload2:
								i8253Mode[2] |= B8(01000000);          // reloaded
reload2_:
								i8253TimerL[2]=i8253TimerR[2]=i8253TimerW[2];
								if(((i8253Mode[2] & B8(00001110)) >> 1) == 3)
									i8253TimerR[2] &= 0xfffe;
								}
              break;
            }
          i8253RegW[2]=t1;
//          PR2=i8253TimerW[2] ? i8253TimerW[2] : 65535;
          break;
        case 3:
					i=t1 >> 6;
          if(i < 3) {
            i8253Mode[i]= (i8253Mode[i] & 0xc0) | (t1 & 0x3f);
						switch(t1 & B8(00110000)) {
							case B8(00000000):
								i8253TimerL[i]=i8253TimerR[i];		// 
								break;
							case B8(00010000):		// 
								break;
							case B8(00100000):		// 
								break;
							case B8(00110000):
								break;
							}
						}
					else {
						switch(t1 & B8(00110000)) {
							case B8(00000000):
								break;
							case B8(00010000):		// Status (8254)
								;		// 
								break;
							case B8(00100000):		// Control
								;
								break;
							case B8(00110000):
								break;
							}
						}

          i8253RegR[3]=i8253RegW[3]=t1;
          i8253ModeSel=0;
#ifdef _DEBUG
#ifdef DEBUG_8253
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  TIMER %u : mode %02X, count=%04x / %04x/%04x\n",timeGetTime(),
				_ip,t1 >> 6,t1 & 0x3f, i8253TimerW[t1 >> 6],i8253TimerR[t1 >> 6],i8253TimerL[t1 >> 6]);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
          break;
        }
      break;

    case 6:        // 60-6F 8042/8255 keyboard; speaker; config byte
#ifndef PCAT		// bah, chissà... questo dovrebbe usare 8255 e AT 8042...
      t &= 0x3;
#ifdef _DEBUG
#ifdef DEBUG_KB
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 6%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
//              PR2 = j;		 // 
              }
            else {
//              PR2 = j;
              }
						PlayResource(MAKEINTRESOURCE(IDR_WAVE_SPEAKER),FALSE);
#ifdef _DEBUG
#ifdef DEBUG_KB
			if((t1 & B8(00000010)) && !(i8255RegW[1] & B8(00000010))) {
				extern HFILE spoolFile;
				char myBuf[256];
				wsprintf(myBuf,"%u  BEEP\n",timeGetTime());
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}
#endif
#endif
            }
          else {
//            PR2 = j;		 // 
						PlayResource(NULL,FALSE);
#ifdef _DEBUG
#ifdef DEBUG_KB
			if(!(t1 & B8(00000010)) && (i8255RegW[1] & B8(00000010))) {
				extern HFILE spoolFile;
				char myBuf[256];
				wsprintf(myBuf,"%u  endBEEP\n",timeGetTime());
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}
#endif
#endif
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
					if((t1 & B8(10000000)) && !(i8255RegW[1] & B8(10000000))) {		// ack kbd, clear (b7)
						KBStatus &= ~B8(00000001);
						Keyboard[0]=0;
			/*{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"clear kb \n");
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}*/
						}
					if(t1 & B8(01000000)) {		// reset (b6 sale)
						if(!(i8255RegW[1] & B8(01000000))) {
							KBStatus |= B8(00000001);			// output disponibile
							Keyboard[0]=0xAA;			//
							//    KBDIRQ=1;
							i8259IRR |= B8(00000010);
			/*{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"reset kb \n");
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}*/
							}
						}


					if(t1 & B8(00001000))
//						SendMessage(hWnd,WM_COMMAND,ID_OPZIONI_VELOCIT_LENTO,0,0);
						CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER);
					else
//						SendMessage(hWnd,WM_COMMAND,ID_OPZIONI_VELOCIT_VELOCE,0,0);
						CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER*2);

					i8255RegR[1]=i8255RegW[1]=t1;
					}

          break;
        case 2:     // machine settings... (da V20), NMI
					i8255RegR[2]=i8255RegW[2]=t1;
					i8255RegR[2] &= ~B8(11000000);		// bah direi! questi sarebbero parity error ecc, da settare in HW
          break;
        case 3:     // PIA 8255, port A & C dice, a E06C...  però sarebbe anche a 0x61 per machineflags
					i8255RegR[3]=i8255RegW[3]=t1;		// glabios mette 0x99 ossia PA=input, PC=tutta input, PB=output
          break;
        }

#else

			// https://wiki.osdev.org/A20_Line

      t &= 0x7;
#ifdef _DEBUG
#ifdef DEBUG_KB
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 6%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      switch(t) {
        case 0:     // 
          if(i8042RegW[1]==0x60) {
            KBCommand=t1;
						KBStatus &= ~B8(00000100);
						KBStatus |= KBCommand & B8(00000100);

						/* patch award, no... if(KBCommand & 1) {
							KBStatus |= 1;				// has data output
							}
*/



            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {  // boh... 2024
            //KBRAM[i8042RegW[1] & 0x1f]
            }
          else if(i8042RegW[1]==0xD1) {
						i8042Output=t1;
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
            }
          else if(i8042RegW[1]==0xD2) {			//write output port
            }
          else if(i8042RegW[1]==0xD3) {
            }
          else if(i8042RegW[1]==0xD4) {
            }
          else if(i8042RegW[1]==0xED) {     //led 
//            LED1 = t1 & 1;
//            LED2 = t1 & 2 ? 1 : 0;
//            LED3 = t1 & 4 ? 1 : 0;
            }
          else if(i8042RegW[1]==0xEE) {     //echo
            Keyboard[0]=i8042RegW[1];
						i8042OutPtr=1;
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
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
             /*PB1 0 turn off speaker
             1 enable speaker data
             PB0 0 turn off timer 2
             1 turn on timer 2, gate speaker with    square wave*/
          j=i8253TimerW[2]/3  /* vale 0x528 vado a 0x190=400 */;      // 2025 (was: 980Hz 5/12/19
          if(t1 & 2) {
            if(t1 & 1) {
//              PR2 = j;		 // 
              }
            else {
//              PR2 = j;
              }
						PlayResource(MAKEINTRESOURCE(IDR_WAVE_SPEAKER),FALSE);
#ifdef _DEBUG
#ifdef DEBUG_KB
			if(t1 & B8(00000010)) {
			extern HFILE spoolFile;
			char myBuf[256];
			wsprintf(myBuf,"%u  BEEP\n",timeGetTime());
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
            }
          else {
//            PR2 = j;		 // 
						PlayResource(NULL,FALSE);
#ifdef _DEBUG
#ifdef DEBUG_KB
			if(!(t1 & B8(00000010))) {
			extern HFILE spoolFile;
			char myBuf[256];
			wsprintf(myBuf,"%u  endBEEP\n",timeGetTime());
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
            }
					}

          break;
        case 4:     // 
          i8042RegW[1]=t1;
          if(i8042RegW[1]==0xAA) {					// self test
            KBStatus |= B8(00001011);				// command, data input full, has data output
//            Keyboard[1]=0x55; Keyboard[0]=0x55 /*ribadisco*/ /*1 /*ID*/; i8042OutPtr=2;
            Keyboard[0]=0x55; i8042OutPtr=1;
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xA9) {			// test lines aux / #2 device
            KBStatus |= B8(00001011);				// command, data input full, has data output

						// FARE!
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xAB) {			// test lines 
            KBStatus |= B8(00001011);				// command, data input full, has data output
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xAD) {			// disable
						KBStatus |= B8(00001010);		// command,data input full, 
//						KBStatus &= ~1;				// pulisco, pare!
//						i8259IRR &= ~B8(00000010);	// tutto
            KBCommand |= B8(00010000);
            }
          else if(i8042RegW[1]==0xAE) {			// enable
						KBStatus |= B8(00001010);		// command,data input full, 
            KBCommand &= ~B8(00010000);
            }
          else if(i8042RegW[1]==0x20) { // r/w control
            KBStatus |= B8(00001011);				// command, data input full, has data output
            Keyboard[0]=i8042RegW[1]; i8042OutPtr=1;
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0x60) { // r/w control
						KBStatus |= B8(00001010);		// command,data input full, 
            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {
            //KBRAM[i8042RegW[1] & 0x1f]
						KBStatus |= B8(00001010);		// command
						goto kb_setirq;
            }


          else if(i8042RegW[1]==0xA1) {		//BOH ami286
            KBStatus |= B8(00001011);				// command, data input full, has data output
            Keyboard[0]=0;
						i8042OutPtr=1;
						goto kb_setirq;
            }


          else if(i8042RegW[1]==0xC0) {		//read output data (ossia MAchineSettings, DIVERSI da XT!
            KBStatus |= B8(00001011);				// command, data input full, has data output
            Keyboard[0]=MachineSettings;
						i8042OutPtr=1;
// forse no						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xC1) {		//read output data (ossia MAchineSettings 0..3
            KBStatus |= B8(00001011);				// command, data input full, has data output
            Keyboard[0]=MachineSettings << 4;
						i8042OutPtr=1;
// forse no						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xC2) {		//read output data (ossia MAchineSettings, 4..7
            KBStatus |= B8(00001011);				// command, data input full, has data output
            Keyboard[0]=MachineSettings & 0xf0;
						i8042OutPtr=1;
// forse no						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xD0) {		// read output port
            KBStatus |= B8(00001011);				// command, data input full, has data output
            Keyboard[0]=i8042Output;
						i8042OutPtr=1;
// forse no						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xD1) {		// write output port
						if(KBCommand & B8(00010000))			// se disabilitata
							i8042Output=t1;
						else
							i8042Output=t1;				// boh...
						KBStatus |= B8(00001010);		// command, data input full
            Keyboard[0]=i8042RegW[1];		// bah ok
						i8042OutPtr=1;
            }
          else if(i8042RegW[1]==0xD2) {		// https://stanislavs.org/helppc/8042.html
						goto kb_echo;
            }
          else if(i8042RegW[1]==0xED) {     //led 
						KBStatus |= B8(00001010);		// command,data input full, 
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xEE) {     //echo, ora non lo vedo nella pagina 8042 ma ok
kb_echo:
            Keyboard[0]=i8042RegW[1];
            KBStatus |= B8(00001011);				// command, data input full, has data output
						i8042OutPtr=1;
						goto kb_setirq;
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {		// pulse output pin
						KBStatus |= B8(00001010);		// command,data input full, 
// FE equivale a pulsare il bit 0 ossia RESET!

						if(!(t1 & 1)) {
							CPUPins |= DoReset;
//			debug=0;
			_lwrite(spoolFile,"*------ RESET!!\r\n",17);


//			ram_seg[0x472]=0x34;
//			ram_seg[0x473]=0x12; forzo warm boot STRAPUTTANO dio che non muore insieme a crosetto
						}

kb_setirq:
						if(KBCommand & B8(00000001)) {     //..e se interrupt attivi...
						//    KBDIRQ=1;
							i8259IRR |= B8(00000010);
							}
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
#ifdef _DEBUG
#ifdef DEBUG_RTC
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 7%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
              currentDateTime.tm_sec=t1;
              break;
              // in mezzo c'è Alarm...
            case 2:
              currentDateTime.tm_min=t1;
              break;
            case 4:
              currentDateTime.tm_hour=t1;
              break;
            case 6:
            // 6 è day of week... si può scrivere??
              currentDateTime.tm_wday=t1;
              break;
            case 7:
              currentDateTime.tm_mday=t1;
              break;
            case 8:
              currentDateTime.tm_mon=t1;
              break;
            case 9:
              currentDateTime.tm_year=t1;
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

/*							if((i146818RegW[0] & 0x3f) == 14 || (i146818RegW[0] & 0x3f)==15)
							{	char myBuf[256],myBuf2[32];
							wsprintf(myBuf,"%u RTCRAM W: ",timeGetTime());
							for(i=0; i<64; i++) {		// 
								wsprintf(myBuf2,"%02X ",i146818RAM[i]);
								strcat(myBuf,myBuf2);
								if(i==9 || i==13)
									strcat(myBuf," ");
								}
							strcat(myBuf,"\r\n");
							_lwrite(spoolFile,myBuf,strlen(myBuf));
							}*/

              break;
            }
          break;
        }
      break;

    case 8:
      t &= 0xf;
#ifdef _DEBUG
#ifdef DEBUG_8237
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 8%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  DIAG status %02X ---------------------------------- A20=%u\n",timeGetTime(),
				_ip,t1,i8042Output & 2 ? 1 : 0);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			wsprintf(myBuf,"DIAG status %02X",t1);
			SetWindowText(hStatusWnd,myBuf);
			}


/*			if(t1==0x24)
				debug=1;
			if(t1==0x25)
				debug=0;*/



					DIAGPort=t1;
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
				case 2:
			// linea A20 in alcuni computer http://independent-software.com/operating-system-development-enabling-a20-line.html
#ifdef PCAT
					if(t1 & 2)
            i8042Output=t1;
					else
						;
#endif
					break;
				case 9:
					// PCAT scrive 0 al boot
					break;
				}
			break;

    case 0xa:        // PIC 8259 controller #2 su AT; machine settings su XT
#ifdef PCAT
      t &= 0x1;
#ifdef _DEBUG
#ifdef DEBUG_8259
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at A%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      switch(t) {
        case 0:     // 0x20 per EOI interrupt, 0xb per "chiedere" quale... subito dopo c'è lettura a 0x20
          i8259Reg2W[0]=t1;      // 
					if(t1 & B8(00010000)) {// se D4=1 qua (0x10) è una sequenza di Init,
						i8259OCW2[2] = 2;		// dice, default su lettura IRR
//						i8259RegR[0]=0;
						// boh i8259IMR=0xff; disattiva tutti
						i8259ICW2[0]=t1;
						i8259ICW2Sel=2;		// >=1 indica che siamo in scrittura ICW
						}
					else if(t1 & B8(00001000)) {// se D3=1 qua (0x8) è una richiesta
						// questa è OCW3 
						i8259OCW2[2]=t1;
						switch(i8259OCW2[2] & B8(00000011)) {		// riportato anche sopra, serve (dice, in polled mode)
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
						switch(i8259OCW2[1] & B8(11100000)) {
							case B8(00000000):// rotate pty in automatic (set clear
								break;
							case B8(00100000):			// non-spec, questo pulisce IRQ con priorità + alta in essere (https://rajivbhandari.wordpress.com/wp-content/uploads/2014/12/unitii_8259.pdf
//						i8259ISR=0;		// MIGLIORARE! pulire solo il primo
								for(i=1; i; i<<=1) {
									if(i8259ISR2 & i) {
										i8259ISR2 &= ~i;
										break;
										}
									}
//				      CPUPins &= ~DoIRQ;		// SEMPRE! non serve + v. di là
								break;
							case B8(01000000):			// no op
								break;
							case B8(01100000):			// 0x60 dovrebbe indicare uno specifico IRQ, nei 3bit bassi 0..7
								i8259ISR2 &= ~(1 << (t1 & 7));
								break;
							case B8(10000000):			// rotate pty in automatic (set
								break;
							case B8(10100000):			// rotate pty on non-specific
								break;
							case B8(11000000):			// set pty
								break;
							case B8(11100000):			// rotate pty on specific
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
							if(!(i8259ICW2[0] & B8(00000001))) 		// ICW4 needed?
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
#ifdef _DEBUG
#ifdef DEBUG_8237
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at C%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
					if(t1 & B8(00000100))
						i8237Reg2W[15] |= (1 << (t1 & B8(00000011)));
					else
						i8237Reg2W[15] &= ~(1 << (t1 & B8(00000011)));
					i8237Reg2R[15]=i8237Reg2W[15];
		      i8237Reg2R[0xa]=i8237Reg2W[0xa]=t1;
					break;
				case 0xb:			// Mode
					i8237Mode2[t1 & B8(00000011)]= t1 & B8(11111100);
					i8237DMACurAddr2[t1 & B8(00000011)]=i8237DMAAddr2[t1 & B8(00000011)];			// bah credo, qua
					i8237DMACurLen2[t1 & B8(00000011)]=i8237DMALen2[t1 & B8(00000011)];
		      i8237Reg2R[0xb]=i8237Reg2W[0xb]=t1;

					i8237Reg2R[8] |= ((1 << t1 & (B8(00000011))) << 4);  // request??... boh

					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					i8237FF2=0;
		      i8237Reg2R[0xc]=i8237Reg2W[0xc]=t1;
					break;
				case 0xd:			// Reset
					memset(i8237Reg2R,0,sizeof(i8237Reg2R));
					memset(i8237Reg2W,0,sizeof(i8237Reg2W));
					i8237Reg2R[15]=i8237Reg2W[15] = B8(00001111);
					i8237Reg2R[8]=B8(00000000);
		      i8237Reg2R[0xd]=i8237Reg2W[0xd]=t1;
					break;
				case 0xe:			//  Clear Masks
					i8237Reg2R[15]=i8237Reg2W[15] &= ~B8(00001111);
					break;
				case 0xf:			//  Masks
					i8237Reg2R[15]=i8237Reg2W[15] = (t1 & B8(00001111));
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
    case 0x1f:      // 1f0-1ff   // hard disc 0 su AT		http://www.techhelpmanual.com/897-at_hard_disk_ports.html   http://www.techhelpmanual.com/892-i_o_port_map.html
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
          currentDateTime.tm_sec=t1;
          break;
        case 2:
          currentDateTime.tm_min=t1;
          break;
        case 3:
          currentDateTime.tm_hour=t1;
          break;
        case 4:
          currentDateTime.tm_mday=t1;
          break;
        case 5:
          currentDateTime.tm_mon=t1;
          break;
        case 6:
          currentDateTime.tm_year=t1;
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
    case 0x32:    // 320- (hard disk su XT  http://www.techhelpmanual.com/898-xt_hard_disk_ports.html   https://forum.vcfed.org/index.php?threads/ibm-xt-i-o-addresses.1237247/
      t &= 3;
#ifdef _DEBUG
#ifdef DEBUG_HD
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 32%X : %02x  cmd=%02X; fifoW=%u fifoR=%u  %s %s\n",timeGetTime(),
				_ip,t,t1,HDContrRegW[1] & B8(00011111),HDFIFOPtrW,HDFIFOPtrR,
				t==0 ? "CMD" : "",
				(HDContrRegW[1] & B8(00011111))==8 ? "READ" : ((HDContrRegW[1] & B8(00011111))==3 ? "SENSE" : 
				((HDContrRegW[1] & B8(00011111))==0x06 ? "FORMAT" : ((HDContrRegW[1] & B8(00011111))==0xa ? "WRITE" : ""))));
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
      switch(t) {
        case 0:			// WRITE DATA
					if(!HDFIFOPtrW) {
						HDContrRegW[0]=t1;
						HDFIFOPtrW=1; HDFIFOPtrR=0;
						}
					else {
						HDFIFOW[HDFIFOPtrW++] = t1;

						if(HDFIFOPtrW==6) {
							switch(HDContrRegW[0] & B8(00011111)) {
								case 0xc:
									hdState &= ~B8(00001111);			// 
									break;
								default:
									hdState &= ~B8(00000101);			// handshake, e passo a Status
									hdState |= B8(00000010);		// passo a write
		//						hdTimer=100;
									break;
								}

							}

						else {		// comandi con dati a seguire (tipo 0xC
							switch(HDContrRegW[0] & B8(00011111)) {
								case 0xc:
									if(HDFIFOPtrW==14) {		// 8 byte, v.

												// restituire 4 in Error code poi (però tanto non lo chiede, non arriva SENSE...
		//										HDFIFO[1]=4;

										hdState &= ~B8(00100101);			// handshake, e passo a status, no IRQ
										hdState |= B8(00000010);		// passo a write
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
					hdState=B8(00010101);			// mah; e basare DMA su hdControl<1>

//					debug=1;

          break;
        case 2:			// SELECT
					/*Writing to port 322 selects the WD 1002S-WX2,
						sets the Busy bit in the Status Register and prepares
						it to receive a command. When writing to port
						322, the data byte is ignored.*/
					hdState=B8(00001101);			// ESIGE BSY e C/D (strano.. sarà doc sbagliato ancora); e basare DMA su hdControl<1>
					HDFIFOPtrW=HDFIFOPtrR=0;
					HDContrRegW[2]=t1;
          break;
        case 3:			// WRITE DMA AND INTERRUPT MASK REGISTER
					hdControl=HDContrRegW[3]=t1;
          break;
				}

			break;

    case 0x37:    // 370-377 (secondary floppy, AT http://www.techhelpmanual.com/896-diskette_controller_ports.html); 378-37A  LPT1
      t &= 3;
			LPT[0][t]=t1;		// per ora, al volo!
      switch(t) {
        case 0:
//  #ifndef ILI9341
//          LATE = (LATE & 0xff00) | t1;
//          LATGbits.LATG6 = t1 & B8(00010000) ? 1 : 0;   // perché LATE4 è led...
//  #endif
          break;
        case 2:    // qua c'è lo strobe
//          LATGbits.LATG7 = t1 & B8(00000001) ? 1 : 0;   // strobe, FARE gli altri :)
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
#ifdef _DEBUG
#ifdef DEBUG_VGA
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 3C%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
#ifdef _DEBUG
#ifdef DEBUG_VGA
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 3D%X : %02x \n",timeGetTime(),
				_ip,t,t1);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif
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
          CGAreg[0xa] &= ~B8(00000110);      // pulisco trigger light pen
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

#ifdef _DEBUG
#ifdef DEBUG_FD
			{
			extern HFILE spoolFile;
			extern uint16_t _ip;
			char myBuf[256];
			wsprintf(myBuf,"%u: %04x  write at 3F%X : %02x  cmd=%02X; fifoW=%u fifoR=%u  %s %s\n",timeGetTime(),
				_ip,t,t1,FloppyContrRegW[1] & B8(00011111),FloppyFIFOPtrW,FloppyFIFOPtrR,
				t==2 ? "CMD" : "",
				(FloppyContrRegW[1] & B8(00011111))==6 ? "READ" : ((FloppyContrRegW[1] & B8(00011111))==8 ? "SENSE" : 
				((FloppyContrRegW[1] & B8(00011111))==0x0b ? "FORMAT" : ((FloppyContrRegW[1] & B8(00011111))==5 ? "WRITE" : ""))));
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
#endif
#endif

      switch(t) {
				uint8_t disk;
        case 2:   // Write: digital output register (DOR)  MOTD MOTC MOTB MOTA DMA ENABLE SEL1 SEL0
					// questo registro in effetti non appartiene al uPD765 ma è esterno...
					// ma secondo ASM 8 è Interrupt on e 4 è reset ! direi che sono 2 diciture per la stessa cosa £$%@#
					disk=t1 & 3;
					/*FloppyContrRegR[0]=*/FloppyContrRegW[0]=t1;
					FloppyContrRegW[1]=0;
					if(!(FloppyContrRegW[0] & B8(00000100))) {		// così è RESET!
						floppylastCmd[disk]=0xff;
						FloppyFIFOPtrW=FloppyFIFOPtrR=0;
						floppyHead[disk]=0;
						floppyTrack[disk]=0;
						floppySector[disk]=0,
						floppyTimer=0;

/* no.. lascio in InitHW						if(disk <= ((MachineSettings & B8(11000000)) >> 6))		// (2 presenti!
							floppyState[disk]=0x00; // b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent
						else
							floppyState[disk]=0x00; //
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
else  /*					boh */{
//						FloppyFIFOPtrW=FloppyFIFOPtrR=0;
						floppyState[disk] &= ~B8(01000000);	// passo a write
						if(FloppyContrRegW[0] & B8(00010000))	
							floppyState[0] |= B8(00000100);		// motor
						else
							floppyState[0] &= ~B8(00000100);
						if(FloppyContrRegW[0] & B8(00100000))
							floppyState[1] |= B8(00000100);
						else
							floppyState[1] &= ~B8(00000100);
						if(FloppyContrRegW[0] & B8(01000000))
							floppyState[2] |= B8(00000100);
						else
							floppyState[2] &= ~B8(00000100);
						if(FloppyContrRegW[0] & B8(10000000))
							floppyState[3] |= B8(00000100);
						else
							floppyState[3] &= ~B8(00000100);
						if(FloppyContrRegW[0] & B8(00001000 /*in asm dice che questo è "Interrupt ON"*/))
							setFloppyIRQ(SEEK_FLOPPY_DELAYED_IRQ);
						}
          break;
        case 4:   // Read-only: main status register (MSR)  REQ DIR DMA BUSY D C B A
          break;
        case 5:   // Read/Write: FDC command/data register
					if(!FloppyFIFOPtrW) {
						FloppyContrRegW[1]=t1;
						switch(t1 & B8(00011111)) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
							case 0x06:			// read data
							case 0x02:			// read track
							case 0x0c:			// read deleted
							case 0x0a:			// read ID
							case 0x05:			// write data
							case 0x0b:			// format track
							case 0x09:			// write deleted data
							case 0x11:			// scan equal
							case 0x19:			// scan low or equal
							case 0x07:			// recalibrate
							case 0x03:			// specify
							case 0x04:			// sense drive status
							case 0x1b:			// scan high or equal
							case 0x0f:			// seek (goto cylinder
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x20:		// lock...
							case 0x18:		// perpendicular
							case 0x14:		// dumpreg
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x08:			// sense interrupt status
								FloppyFIFOPtrW=0; FloppyFIFOPtrR=0;
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
						}
//            i8259IRR |= B8(01000000);		// simulo IRQ...
//					if(FloppyContrRegW[0] & B8(10000000))		// RESET si autopulisce
//						FloppyContrRegW[0] &= ~B8(10000000);		// 
//					FloppyContrRegR[0] &= ~B8(00010000);		// non BUSY :)
          break;
				case 7:		// solo AT http://isdaman.com/alsos/hardware/fdc/floppy.htm
          break;

          
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
#endif

	}

void _fastcall OutShortValue(uint16_t t,uint16_t t1) {
	register uint16_t i;

//	i=1;

#if DEBUG_TESTSUITE
#else
	OutValue(t,LOBYTE(t1));			// per ora (usato da GLAbios, trident VGA
	OutValue(t+1,HIBYTE(t1));			// 
#endif
	}


void setFloppyIRQ(uint16_t delay) {

	// mah non aiuta con phoenix e ami e a volte fa incazzare PCXTbios
//										i8259IRR |= B8(01000000);		// simulo IRQ...
	floppyTimer=delay;
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
// usare, da smartestblob		n=((hdCylinder*HD_HEADS+hdHead)*HD_SECTORS_PER_CYLINDER+hdSector)-1;

  return n;  
	}
  

uint8_t *encodeDiscSector(BYTE disk) {
  int i;
  uint8_t c;

  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*SECTOR_SIZE;

	memcpy(sectorData,msdosDisk,SECTOR_SIZE);
 
  return sectorData;
  }

uint32_t getHDiscSector(uint8_t d) {
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

	n=((hdCylinder*HD_HEADS+hdHead)*HD_SECTORS_PER_CYLINDER+hdSector);		//da smartestblob
	/* PARTE DA 0....*/

  return n;  
	}
  
uint8_t *encodeHDiscSector(uint8_t disk) {
  int i;
  uint8_t c;

  msdosHDisk=((uint8_t*)MSDOS_HDISK)+getHDiscSector(disk)*SECTOR_SIZE;

	memcpy(sectorData,msdosHDisk,SECTOR_SIZE);
 
  return sectorData;
  }



BOOL PlayResource(LPSTR lpName,BOOL bStop) { 
  BOOL bRtn; 
  LPSTR lpRes; 
  HANDLE hResInfo, hRes; 

	if(!lpName) {
    sndPlaySound(NULL, 0);
		return TRUE;
		}

  // Find the WAVE resource.  
  hResInfo = FindResource(g_hinst, lpName, "WAVE"); 
  if(hResInfo == NULL) 
    return FALSE; 

  // Load the WAVE resource. 
  hRes = LoadResource(g_hinst, hResInfo); 
  if(hRes == NULL) 
    return FALSE; 

  // Lock the WAVE resource and play it. 
  lpRes = LockResource(hRes); 
  if(lpRes != NULL) { 
    bRtn = sndPlaySound(lpRes, SND_MEMORY | SND_ASYNC | SND_NODEFAULT | (bStop ? 0 : SND_NOSTOP));
    UnlockResource(hRes); 
		} 
  else 
    bRtn = 0; 
 
  // Free the WAVE resource and return success or failure. 
 
  FreeResource(hRes); 
  return bRtn; 
	}

void initHW(void) {
	int i;
	DWORD l;
  
	MachineSettings=B8(01101101);			//2 floppy
#ifdef MDA 
	MachineSettings=B8(01111101);			//2 floppy
#endif
#ifdef VGA
	MachineSettings=B8(01001101);		// 2 floppy
#endif
#ifdef EXT_80x87 
	MachineSettings |= 2;
#endif
		// b0=floppy (0=sì); b1=FPU; bits 2,3 memory size (64K bytes); b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25; b7:6 #floppies-1
		// ma su IBM5160 b0=1 SEMPRE dice!
#ifdef PC_IBM5150
	MachineSettings=B8(00101101);		// 64KB, v.sopra
//	MachineSettings=B8(00100001);		// 640KB
#endif
#ifdef PCAT
	MachineSettings=B8(10110000);		// non cambia un c su Award ma anche sugli altri, il video si autogestisce  https://stanislavs.org/helppc/8042.html
//BASE_MEM8	EQU	00001000B	; BASE PLANAR R/W MEMORY EXTENSION 640/X
//BASE_MEM	EQU	00010000B	; BASE PLANAR R/W MEMORY SIZE 256/512		0=256
//MFG_LOOP	EQU	00100000B	; LOOP POST JUMPER BIT FOP MANUFACTURING  0=installed
//DSP_JMP		EQU	01000000B	; DISPLAY TYPE SWITCH JUMPER BIT, 0=CGA 1=MDA
//KEY_BD_INHIB	EQU	10000000B	; KEYBOARD INHIBIT SWITCH BIT, 0=inibita
#endif
	MachineFlags=B8(00000000);
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
	memset(&VGAseqReg,0,sizeof(VGAseqReg)); 
	VGAFF=VGAptr=0;
	VGAgraphReg[6]=0x0c;		// RAM addr=B8000
#endif
  
  memset(i8237RegR,0,sizeof(i8237RegR));
  memset(i8237RegW,0,sizeof(i8237RegW));
	i8237RegR[15]=i8237RegW[15] = B8(00001111);
#ifdef PCAT
  memset(i8237Reg2R,0,sizeof(i8237Reg2R));
  memset(i8237Reg2W,0,sizeof(i8237Reg2W));
	i8237Reg2R[15]=i8237Reg2W[15] = B8(00001111);
#endif
	memset(DMApage,0,sizeof(DMApage));
	DMArefresh=0;
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
  i8253ModeSel=0;
  memset(i8253TimerR,0,sizeof(i8253TimerR));
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
  KBCommand=B8(01010000) /*XT,disabled*/;  i8042Input=i8042Output=0; i8042OutPtr=1 /* power-on*/;
//	KBStatus=0x00;		b2=0 al power, 1 dopo
	KBStatus |= 0x00;		// b4=enabled(switch/key
#else
  KBCommand=B8(01000000) /* enabled */; KBStatus=0x00;
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
	floppyState[0]=0x80;			// 00 per boot da HD!
	floppyState[1]=0x00;
	floppyState[2]=0;
	floppyState[3]=0;
	memset(HDContrRegR,0,sizeof(HDContrRegR));
	memset(HDContrRegW,0,sizeof(HDContrRegW));
	memset(HDFIFOR,0,sizeof(HDFIFOR));
	memset(HDFIFOW,0,sizeof(HDFIFOW));
	HDFIFOPtrR=HDFIFOPtrW=0;
	hdHead=0;
	hdCylinder=0;
	hdSector=0;
	hdState=hdControl=0;
	
	mouseState=128;
	memset(COMDataEmuFifo,0,sizeof(COMDataEmuFifo));
	COMDataEmuFifoCnt=0;
  
	ColdReset=0;
	ExtIRQNum.x=IntIRQNum.x=0;
	memset(&currentDateTime,0,sizeof(currentDateTime));
	{
		struct tm *myt;
		DWORD l;
		l=time(NULL);
		myt=localtime(&l);
		currentDateTime=*myt;
#ifndef PCAT
		i146818RAM[13] = B8(10000000);		// VRT RTC batt ok! forzato per XT/glatick ma 286 fa lui
#endif

		if(!(i146818RAM[11] & B8(00000100))) {			// BCD sarebbe b2 del registro 11 ma al boot non c'è...
			currentDateTime.tm_hour=to_bcd(currentDateTime.tm_hour);
			currentDateTime.tm_min=to_bcd(currentDateTime.tm_min);
			currentDateTime.tm_sec=to_bcd(currentDateTime.tm_sec);
			i146818RAM[0x32] = to_bcd((((WORD)currentDateTime.tm_year)+1900) / 100);		// secolo
			currentDateTime.tm_year=to_bcd(currentDateTime.tm_year % 100);			// anno, e v. secolo in RAM[32], GLATick
			currentDateTime.tm_mon=to_bcd(currentDateTime.tm_mon  +1);
			currentDateTime.tm_mday=to_bcd(currentDateTime.tm_mday);
			}
#ifdef PCAT
		{const uint8_t valori_cmos[64]= {
			/*00:*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x02, 0xD0, 0x80, 0x00, 0x00, 
			/*10:*/ /*fdd1*/0x33,  0x00, 0x00, 0x00,  0x41, 0x80, 0x02, 0x80,  0x05,  /*tipo hd*/0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			/*20:*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /*cylhd*/0x01,0x7b, 
			/*30:*/ 0x80, 0x05, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
			};
		memcpy(&i146818RAM[16],&valori_cmos[16],48);

		}
#endif

	}
  
//  OC7CONbits.ON = 0;   // spengo buzzer
//  PR2 = 65535;		 // 
//  OC7RS = 65535;		 // 
  
  }

BYTE to_bcd(BYTE n) {
	
	return (n % 10) | ((n / 10) << 4);
	}

BYTE from_bcd(BYTE n) {
	
	return (n & 15) + (10*(n >> 4));
	}

