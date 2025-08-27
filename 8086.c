// test suite https://www.vogons.org/viewtopic.php?t=73677

// GC 28/11/2019 -> 2025

//#warning ANCORA 2024 OCCHIO OVERFLOW, in ADD e in SUB/CMP, verificare con Z80 e 68000 2022
//per LSL ecc https://stackoverflow.com/questions/1712744/left-shift-overflow-on-68k-x86

//v. per Ovf metodo usato su 68000 senza byte alto... provare
// la cosa di inEI... chissà... e anche in Z80; però SERVE su segment override!! (ovvio; dice anche che i primi 8088 avevano questo bug, vedi CPU TEST in BIOS https://cosmodoc.org/topics/processor-detection/ )  


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
//#include <graph.h>
//#include <dos.h>
//#include <malloc.h>
//#include <memory.h>
//#include <fcntl.h>
//#include <io.h>
#include <string.h>
#include <xc.h>

#include "Adafruit_ST77xx.h"
#include "Adafruit_ST7735.h"
#include "adafruit_gfx.h"

#include "8086_PIC.h"


BYTE fExit=0;
BYTE debug=0;

BYTE CPUPins=DoReset;
BYTE CPUDivider=2000000L/CPU_CLOCK_DIVIDER;
extern BYTE ColdReset;

extern BYTE ram_seg[];
extern BYTE CGAreg[],MDAreg[];
extern uint8_t i8259ICW[],i8259IMR,i8259IRR,i8259ISR
#ifdef PCAT
	,i8259ICW2[],i8259IMR2,i8259IRR2,i8259ISR2
#endif
	;
extern uint8_t MachineFlags,MachineFlags2;
extern uint16_t i8253TimerR[],i8253TimerW[];
extern uint8_t i8253RegR[],i8253RegW[],i8253Mode[];
extern uint8_t i8255RegR[];
extern uint8_t i8237Mode[],i8237Mode2[],i8237Command,i8237Command2;
extern uint8_t i8237RegR[],i8237RegW[],i8237Reg2R[],i8237Reg2W[];
extern uint16_t i8237DMAAddr[],i8237DMALen[],i8237DMAAddr2[],i8237DMALen2[];
extern uint16_t i8237DMACurAddr[],i8237DMACurLen[],i8237DMACurAddr2[],i8237DMACurLen2[];
extern uint16_t floppyTimer;
extern uint8_t ExtIRQNum;
extern uint16_t VICRaster;

BYTE Pipe1;
union PIPE Pipe2;

volatile BYTE VIDIRQ=0;
extern uint8_t LCDdirty;



#ifndef EXT_80386
	union REGISTERS regs;
#else
	union REGISTERS regs;
#endif
#ifndef EXT_80386
	union REGISTERS16 segs;
#else
	union REGISTERS32 segs;
	union SEGMENT_DESCRIPTOR seg_descr[6];
#endif


#if 0

	// boh van bene entrambi i gruppi... 8/8/25 MORTE AGLI UMANI
	// quindi il blocco sotto sembra migliore, e ev. usare le CARRY del primo gruppo

#define SIGN_8() (!!(res3.b & 0x80))
#define ZERO_8() (!res3.b)
#define SIGN_16() (!!(res3.x & 0x8000))
#define ZERO_16() (!res3.x)
#define	ZERO_32() (!res3.d)		// solo per moltiplicazione NECV20
#define AUX_ADD_8() (((res1.b & 0xf) + (res2.b & 0xf)) & 0xf0 ? 1 : 0)
#define AUX_ADC_8() (((res1.b & 0xf) + (res2.b & 0xf)+_f.Carry) & 0xf0 ? 1 : 0)
	// secondo V20 emulator sembra servire anche questa/e...
#define AUX_SUB_8() (((res1.b & 0xf) - (res2.b & 0xf)) & 0xf0 ? 1 : 0)
#define AUX_SBB_8() (((res1.b & 0xf) - (res2.b & 0xf)-_f.Carry) & 0xf0 ? 1 : 0)
/*AUX flag: 1-carry out from bit 3 on addition or borrow into bit 3 on subtraction
	0-otherwise*/

/*// da Makushi 68000 o come cazzo si chiama :D  + vari PD...
// res2 è Source e res1 è Dest ossia quindi res3=Result  OCCHIO QUA*/
	// https://github.com/kxkx5150/CPU-8086-cpp/blob/main/src/Intel8086.cpp
	// https://github.com/zsteve/hard86/blob/master/src/emulator/emulator_engine/src/flags.c#L105
#define CARRY_ADD_8()  (!!(((res2.b & res1.b) | (~res3.b & (res2.b | res1.b))) & 0x80))		// ((S & D) | (~R & (S | D)))
//#define CARRY_ADD_8()  (!!(res3.b < res1.b))
//#define CARRY_ADD_8()  ((((uint16_t)res1.b+res2.b) >> 8 ? 1 : 0))
#define CARRY_ADC_8()  CARRY_ADD_8()
//#define CARRY_ADC_8()  ((((uint16_t)res1.b+res2.b+_f.Carry) >> 8 ? 1 : 0))
#define OVF_ADD_8()    (!!(((res2.b ^ res3.b) & ( res1.b ^ res3.b)) & 0x80))			// ((S^R) & (D^R))
#define OVF_ADC_8()    (!!((((res2.b+_f.Carry) ^ res3.b) & ( res1.b ^ res3.b)) & 0x80))			// ((S^R) & (D^R))
#define CARRY_ADD_16() (!!(((res2.x & res1.x) | (~res3.x & (res2.x | res1.x))) & 0x8000))
//#define CARRY_ADD_16() (!!(res3.x < res1.x))
//#define CARRY_ADD_16() ((((uint32_t)res1.x+res2.x) >> 16 ? 1 : 0))
#define CARRY_ADC_16() CARRY_ADD_16()
//#define CARRY_ADC_16() ((((uint32_t)res1.x+res2.x+_f.Carry) >> 16 ? 1 : 0))
#define OVF_ADD_16()   (!!(((res2.x ^ res3.x) & ( res1.x ^ res3.x)) & 0x8000))
#define OVF_ADC_16()   (!!((((res2.x+_f.Carry)^ res3.x) & ( res1.x ^ res3.x)) & 0x8000))
#define CARRY_SUB_8()  (!!(((res2.b & res3.b) | (~res1.b & (res2.b | res3.b))) & 0x80))		// ((S & R) | (~D & (S | R)))
//#define CARRY_SUB_8()  (!!(res1.b < res2.b))
//#define CARRY_SUB_8() ((((uint16_t)res1.b-res2.b) >> 8 ? 1 : 0))
#define CARRY_SBB_8()  CARRY_SUB_8()
//#define CARRY_SBB_8()  (!!((((res2.b-_f.Carry) & res3.b) | (~res1.b & ((res2.b-_f.Carry) | res3.b))) & 0x80))		// ((S & R) | (~D & (S | R)))
//#define CARRY_SBB_8() ((((uint16_t)res1.b-res2.b-_f.Carry) >> 8 ? 1 : 0))
#define OVF_SUB_8()    (!!(((res2.b ^ res1.b) & ( res3.b ^ res1.b)) & 0x80))			// ((S^D) & (R^D))
#define OVF_SBB_8()    (!!((((res2.b-_f.Carry) ^ res1.b) & ( res3.b ^ res1.b)) & 0x80))			// ((S^D) & (R^D))
#define CARRY_SUB_16() (!!(((res2.x & res3.x) | (~res1.x & (res2.x | res3.x))) & 0x8000))
//#define CARRY_SUB_16() (!!(res1.x < res2.x))
//#define CARRY_SUB_16() ((((uint32_t)res1.x-res2.x) >> 16 ? 1 : 0))
#define CARRY_SBB_16() CARRY_SUB_16()
//#define CARRY_SBB_16() (!!((((res2.x-_f.Carry) & res3.x) | (~res1.x & ((res2.x-_f.Carry) | res3.x))) & 0x8000))
//#define CARRY_SBB_16() ((((uint32_t)res1.x-res2.x-_f.Carry) >> 16 ? 1 : 0))
#define OVF_SUB_16()   (!!(((res2.x ^ res1.x) & ( res3.x ^ res1.x)) & 0x8000))
#define OVF_SBB_16()   (!!((((res2.x-_f.Carry) ^ res1.x) & ( res3.x ^ res1.x)) & 0x8000))

#else

#define SIGN_8() (!!(res3.b & 0x80))
#define ZERO_8() (!res3.b)
#define SIGN_16() (!!(res3.x & 0x8000))
#define ZERO_16() (!res3.x)
#define	ZERO_32() (!res3.d)		// solo per moltiplicazione NECV20
#define AUX_ADD_8() (((res1.b ^ res2.b ^ res3.b)) & 0x10 ? 1 : 0)
#define AUX_ADC_8() AUX_ADD_8()
#define AUX_SUB_8() AUX_ADD_8()
#define AUX_SBB_8() AUX_ADD_8()
#define CARRY_ADD_8()  (res3.b < res1.b)
#define CARRY_ADC_8()  (_f.Carry ? (res3.b <= res1.b) : (res3.b < res1.b))		// carry == 1 ? res <= dst : res < dst
#define OVF_ADD_8()    (!!(((res2.b ^ res1.b ^ 0xff) & ( res3.b ^ res1.b)) & 0x80))
#define OVF_ADC_8()    OVF_ADD_8()
#define CARRY_ADD_16() (res3.x < res1.x)
#define CARRY_ADC_16() (_f.Carry ? (res3.x <= res1.x) : (res3.x < res1.x))
#define OVF_ADD_16()   (!!(((res2.x ^ res1.x ^ 0xffff) & ( res3.x ^ res1.x)) & 0x8000))
#define OVF_ADC_16()   OVF_ADD_16()
#define CARRY_SUB_8()  (res1.b < res2.b)
#define CARRY_SBB_8()  (_f.Carry ? (res1.b <= res2.b) : (res1.b < res2.b))		// carry > 0 ? dst <= src : dst < src
#define OVF_SUB_8()    (!!(((res2.b ^ res1.b) & ( res3.b ^ res1.b)) & 0x80))			// ((S^D) & (R^D))
#define OVF_SBB_8()    OVF_SUB_8()
#define CARRY_SUB_16() (res1.x < res2.x)
#define CARRY_SBB_16() (_f.Carry ? (res1.x <= res2.x) : (res1.x < res2.x))
#define OVF_SUB_16()   (!!(((res2.x ^ res1.x) & ( res3.x ^ res1.x)) & 0x8000))
#define OVF_SBB_16()   OVF_SUB_16()
#endif

	/*
void cmp_flags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16> {
    commonflags(a,b,result);
    set_flag(F_OVERFLOW, ((a ^ b) & (a ^ result)) >> (sizeof(T)*8-1));
    set_flag(F_CARRY, a < b);
}


result = a-b
oh SBB is a bit more cursed

case 0x03: //SBB
    out = p1-(p2+flag(F_CARRY));
    commonflags(p1,p2,out);
    set_flag(F_AUX_CARRY, (p1&0xF)-((p2&0xF)+flag(F_CARRY)) < 0x00);
    set_flag(F_OVERFLOW, ((p1 ^ p2) & (p1 ^ out)) >> (sizeof(T)*8-1));
    set_flag(F_CARRY, ((p1-(p2+flag(F_CARRY)))>>(sizeof(T)*8)) < 0);
    break;


for ADD it's set_flag(F_AUX_CARRY, ((a ^ b ^ result) & 0x10) != 0);
for ADC it's set_flag(F_AUX_CARRY, (p1&0xF)+(p2&0xF)+flag(F_CARRY) >= 0x10);
*/


int Emulate(int mode) {
  // in termini di memoria usata, tenere locali le variabili è molto meglio...
#define _ah regs.r[0].b.h
#define _al regs.r[0].b.l
#define _ch regs.r[1].b.h
#define _cl regs.r[1].b.l
#define _dh regs.r[2].b.h
#define _dl regs.r[2].b.l
#define _bh regs.r[3].b.h
#define _bl regs.r[3].b.l
#ifdef EXT_80386
#define _eax regs.r[0].d
#define _ecx regs.r[1].d
#define _edx regs.r[2].d
#define _ebx regs.r[3].d
#define _esp regs.r[4].d
#define _ebp regs.r[5].d
#define _esi regs.r[6].d
#define _edi regs.r[7].d
#define _ax regs.r[0].x.l
#define _cx regs.r[1].x.l
#define _dx regs.r[2].x.l
#define _bx regs.r[3].x.l
#define _sp regs.r[4].x.l
#define _bp regs.r[5].x.l
#define _si regs.r[6].x.l
#define _di regs.r[7].x.l
#else
#define _ax regs.r[0].x
#define _cx regs.r[1].x
#define _dx regs.r[2].x
#define _bx regs.r[3].x
#define _sp regs.r[4].x
#define _bp regs.r[5].x
#define _si regs.r[6].x
#define _di regs.r[7].x
#endif
#define _es segs.r[0].x
#define _cs segs.r[1].x
#define _ss segs.r[2].x
#define _ds segs.r[3].x
#ifdef EXT_80386
#define _fs segs.r[4].x
#define _gs segs.r[5].x
#endif
	uint16_t _ip=0;
	register union REGISTRO_F _f;
	union REGISTRO_F _f1;
  uint8_t inRep=0;
	uint8_t inRepStep=0;
  BYTE segOverride=0,segOverrideIRQ=0,inEI=0;
	uint8_t inLock=0;
	uint16_t *theDs;
#ifdef EXT_80386
  BYTE sizeOverride=0,sizeOverrideA=0;
#endif
#ifdef EXT_80x87
  uint16_t status8087=0,control8087=0;
#endif
  register union OPERAND op1,op2;
  register union RESULT res1,res2,res3;
  BYTE immofs;		// usato per dato #imm che segue indirizzamento 0..2 (NON registro)
		// è collegato a GetMorePipe, inoltre
  register uint16_t i;
	int c=0;
  uint8_t screenDivider=0;
	uint8_t timerDivider=0;




#ifdef EXT_80286
  _ip=0xfff0;
  _cs=0x000f;
#else
  _ip=0x0000;
  _cs=0xffff;
#endif
  _f.x=_f1.x=0; _f.unused=1;
  inRep=0; inRepStep=0; segOverride=0; inEI=0;
#ifdef EXT_80386
  sizeOverride=0; sizeOverrideA=0;
	memset(seg_descr,0,sizeof(seg_descr));
#endif

  CPUPins = 0; ExtIRQNum=0;
	do {

		c++;
#ifdef USING_SIMULATOR
#define VIDEO_DIVIDER 0x0000000f
#else
#define VIDEO_DIVIDER 0x0000007f
#endif
    if(!(c & VIDEO_DIVIDER)) {   //~64uS
  	  CGAreg[0xa] ^= 0b0001;  //retrace H (patch...
      if(!(c & 0xffff)) {
        ClrWdt();
// yield()
        }
      }

		if(ColdReset) {
			ColdReset=0;
			initHW();
			CPUPins = DoReset;
			continue;
			}

//		if(debug) {
#ifdef __DEBUG
//			printf("%04x:%04x    %02x\n",_cs,_ip,GetValue(MAKE20BITS(_cs,_ip)));
//			printf("%04x:%04x\n",_cs,_ip);
//    puts("pippo");
#endif

//			}
		/*if(kbhit()) {
			getch();
			printf("%04x    %02x\n",_ip,GetValue(_ip));
			printf("281-284: %02x %02x %02x %02x\n",*(p1+0x281),*(p1+0x282),*(p1+0x283),*(p1+0x284));
			printf("2b-2c: %02x %02x\n",*(p1+0x2b),*(p1+0x2c));
			printf("33-34: %02x %02x\n",*(p1+0x33),*(p1+0x34));
			printf("37-38: %02x %02x\n",*(p1+0x37),*(p1+0x38));
			}*/
    if(VIDIRQ) {
#ifndef USING_SIMULATOR
      if(!screenDivider) {
        // usare LCDdirty...
        UpdateScreen(VICRaster,VICRaster+8 /*MIN_RASTER,MAX_RASTER*/);
        VICRaster+=8;					 	 // [raster pos count, 200 al sec... v. C64]
        if(VICRaster >= MAX_RASTER) {		 // 
          VICRaster=MIN_RASTER; 
          screenDivider++;
          }
        }
      else {
        VICRaster+=32;					 	 // più veloce qua, 7 passate
        if(VICRaster >= MAX_RASTER) {		 // 
          VICRaster=MIN_RASTER; 
          LED2 ^= 1;      // VIDIRQ attivato come flag qua, ogni 10mS => 60mS qua
          screenDivider++;
          screenDivider %= 4;
          }
        }
#endif
      VIDIRQ=0;
          
      if(!SW1) {       // test tastiera, me ne frego del repeat/rientro :)
        if(keysFeedPtr==255)      // debounce...
          keysFeedPtr=254;
        }
      if(!SW2) {        // 
        __delay_ms(100); ClrWdt();
        CPUPins=DoReset;
        }
    
#ifdef MOUSE_TYPE 
      manageTouchScreen();
#endif
      }

//questa parte equivale a INTA sequenza ecc  http://www.icet.ac.in/Uploads/Downloads/3_mod3.pdf
    if((i8259IRR & 0x1) && !(i8259IMR & 0x1)) {
			if(_f.IF) {			// www.brokenthorn.com/Resources/OSDevPic.html  mah, controintuitivo!
        CPUPins |= DoIRQ;
  // [COGLIONI! sembra che gli IRQ si attivino PRIMA del ram-test e così fallisce... per il resto sarebbe ok, non si pianta, 17/7/24
        ExtIRQNum=8;      // Timer 0 IRQ; http://www.delorie.com/djgpp/doc/ug/interrupts/inthandlers1.html
        i8259ISR |= 1;	i8259IRR &= ~1;
        if(i8259ICW[3] & 0b00000010)		// AutoEOI
          i8259ISR &= ~0x1;
        }
      }
    if((i8259IRR & 0x2) && !(i8259IMR & 0x2)) {
			if(_f.IF) {
        CPUPins |= DoIRQ;
        ExtIRQNum=9;      // IRQ 9 Keyboard
        i8259ISR |= 2;	i8259IRR &= ~2;
        if(i8259ICW[3] & 0b00000010)		// AutoEOI
          i8259ISR &= ~0x2;
        }
      }
    if((i8259IRR & 0x10) && !(i8259IMR & 0x10)) {
			if(_f.IF) {
        CPUPins |= DoIRQ;
        ExtIRQNum=0x0c;      // IRQ 12 COM1
        i8259ISR |= 0x10;	i8259IRR &= ~0x10;
        if(i8259ICW[3] & 0b00000010)		// AutoEOI
          i8259ISR &= ~0x10;
        }
      }
    if((i8259IRR & 0x20) && !(i8259IMR & 0x20)) {
			if(_f.IF) {
        CPUPins |= DoIRQ;
        ExtIRQNum=13;      // IRQ 5 Hard disc
        i8259ISR |= 0x20;	i8259IRR &= ~0x20;
        if(i8259ICW[3] & 0b00000010)		// AutoEOI
          i8259ISR &= ~0x20;
        }
      }
    if((i8259IRR & 0x40) && !(i8259IMR & 0x40)) {
			if(_f.IF) {
        CPUPins |= DoIRQ;
        ExtIRQNum=14;      // IRQ 6 Floppy disc
        i8259ISR |= 0x40;	i8259IRR &= ~0x40;
        if(i8259ICW[3] & 0b00000010)		// AutoEOI
          i8259ISR &= ~0x40;
        }
      }
#ifdef PCAT
    if((i8259IRR2 & 0x1) && !(i8259IMR2 & 0x1)) {
			if(_f.IF) {
        CPUPins |= DoIRQ;
        ExtIRQNum=0x70;      // IRQ RTC
        i8259ISR2 |= 1;	i8259IRR2 &= ~1;
        if(i8259ICW[3] & 0b00000010)		// AutoEOI
          i8259ISR2 &= ~0x1;
      }
#endif
      
		if(i8255RegR[2]/*MachineFlags*/ & 0b11000000)		//https://www.minuszerodegrees.net/5150/misc/5150_nmi_generation.jpg
			CPUPins |= DoNMI;
		// e sembra che cmq debba essere attivo in i8259Reg2R[0] OPPURE da b7 scritto a 0xA0! (pcxtbios
            
		if(CPUPins & DoReset) {
			initHW();
			_f.x=_f1.x=0;     // https://thestarman.pcministry.com/asm/debug/8086REGs.htm
			_f.unused=1; _f.unused2=_f.unused3=0; 
#ifdef EXT_80x87 
			status8087=0; control8087=0;			// VERIFICARE!
#endif
#ifdef EXT_80286
      _ip=0xfff0;
      _cs=0x000f;
			_f.IOPL=0; _f.NestedTask=0;			// fare...
			_f.unused4=0;			// https://en.wikipedia.org/wiki/FLAGS_register
#else
      _ip=0x0000;
      _cs=0xffff;
			_f.unused3_=7;		// v. POPF / fix_flags sotto
#ifdef EXT_NECV20
			_f.MD=1;					// così al boot 
#else
			_f.unused4=1;			// v. POPF / fix_flags sotto
#endif
#endif
      
#ifdef __DEBUG
//      _ip=0x01a2;
//      _cs=0xfe00;
#endif     
      
      inRep=0; inRepStep=0; segOverride=0; inEI=0;
			CPUPins=0; ExtIRQNum=0;
#ifdef EXT_80386
      sizeOverride=0;
			memset(seg_descr,0,sizeof(seg_descr));
#endif
			}

rallenta:
		if(++timerDivider >= 2          *1/**CPUdivider*/) {			// 4.77 ->1.19  (MA SERVE rallentare ulteriormente, per i cicli/istruzione (questa merda è indispensabile per GLABios che fa un test ridicolo sui timer... #nerd #froci [diventano troppo lenti i timer...
      // OCCHIO con 3 non va più floppy dopo aver ritarato i timer!! 26/8/25
      // Ma in effetti se consideriamo che ogni istruzione impiega da 3-4 a 100 cicli, diciamo max 20 http://aturing.umcs.maine.edu/~meadow/courses/cos335/80x86-Integer-Instruction-Set-Clocks.pdf
      // e passiamo di qua dopo ognuna, allora il divisore quasi non ha senso... anzi andrebbe invertito!
			timerDivider=0;
			// https://stanislavs.org/helppc/8253.html   http://wiki.osdev.org/Programmable_Interval_Timer#Mode_0_-_Interrupt_On_Terminal_Count
			// devono andare a ~1.1MHz qua
			// aggiungere modo BCD a tutti i conteggi!! :)  i8253Mode[0] & 0b00000001)
			switch((i8253Mode[0] & 0b00001110) >> 1) {// VERIFICARE altri modi a parte free running :)
				case 0:		// INTerrupt on terminal count
					// (continous) output is low and goes high at counting end
					i8253TimerR[0]--;
					if(!i8253TimerR[0]) {
						if(i8253Mode[0] & 0b01000000)          // reloaded
							;
						i8253Mode[0] |= 0b10000000;          // OUT=1
					//      TIMIRQ=1;  //
							i8259IRR |= 1;
						}
					else {
						i8253Mode[0] &= ~0b10000000;          // OUT=0
						}
					break;
				case 1:
					// one-shot output is low and goes high at counting end (retriggerable)
					if(!i8253TimerR[0]) {
						if(i8253Mode[0] & 0b01000000)          // reloaded
							;
						i8253Mode[0] |= 0b10000000;          // OUT=1
					//      TIMIRQ=1;  //
							i8259IRR |= 1;
						}
					else {
						i8253Mode[0] &= ~0b10000000;          // OUT=0
						}
					break;
				case 2:
				case 6:		// boh
					// free counter / rate generator
					i8253TimerR[0]--;
					if(i8253TimerR[0]==1) {
						i8253Mode[0] &= ~0b10000000;          // OUT=0
					//      TIMIRQ=1;  //
							i8259IRR |= 1;
						}
					else if(!i8253TimerR[0]) {
						i8253Mode[0] |= 0b10000000;          // OUT=1
						}
					else {
						}
					break;
				case 3:
				case 7:		// boh
					// free counter / square wave rate generator
					i8253TimerR[0]-=2;
					if(!i8253TimerR[0]) {
						i8253Mode[0] ^= 0b10000000;          // OUT=!OUT
						// IRQ solo sul fronte salita...
						if((i8253Mode[0] & 0b10000000)) {
					//      TIMIRQ=1;  //
							i8259IRR |= 1;
							}
						i8253TimerR[0]=i8253TimerW[0];
						if(i8253TimerR[0] & 1)
							i8253TimerR[0]--;
						}
					else {
						// 
						}
					break;
				case 4:
					// software triggered strobe
					if(!i8253TimerR[0]) {
						if(i8253Mode[0] & 0b01000000)          // reloaded
							;
						i8253Mode[0] &= ~0b10000000;          // OUT=0
					//      TIMIRQ=1;  //
							i8259IRR |= 1;
						}
					else {
						i8253TimerR[0]--;
						i8253Mode[0] |= 0b10000000;          // OUT=1
						}
					break;
				case 5:
					// hardware triggered strobe
					if(!i8253TimerR[0]) {
						if(i8253Mode[0] & 0b01000000)          // reloaded
							;
						i8253Mode[0] &= ~0b10000000;          // OUT=0
					//      TIMIRQ=1;  //
							i8259IRR |= 1;
						}
					else {
						i8253TimerR[0]--;
						i8253Mode[0] |= 0b10000000;          // OUT=1
						}
					break;
				}
			switch((i8253Mode[1] & 0b00001110) >> 1) {// VERIFICARE altri modi a parte free running :)
				case 0:		// INTerrupt on terminal count
					// (continous) output is low and goes high at counting end
					i8253TimerR[1]--;
					if(!i8253TimerR[1]) {
						if(i8253Mode[1] & 0b01000000)          // reloaded
							;
						i8253Mode[1] |= 0b10000000;          // OUT=1
						}
					else {
						i8253Mode[1] &= ~0b10000000;          // OUT=0
						}
					break;
				case 1:
					// one-shot output is low and goes high at counting end (retriggerable)
					if(!i8253TimerR[1]) {
						i8253Mode[1] |= 0b10000000;          // OUT=1
						if(i8253Mode[1] & 0b01000000)          // reloaded
							;
						}
					else {
						i8253TimerR[1]--;
						i8253Mode[1] &= ~0b10000000;          // OUT=0
						}
					break;
				case 2:
				case 6:
					// free counter / rate generator
					// QUESTO VA CON DMA QUI!
					i8253TimerR[1]--;
					if(i8253TimerR[1]==1) {
						i8253Mode[1] &= ~0b10000000;          // OUT=0
						}
					else if(!i8253TimerR[1]) {
						i8253Mode[1] |= 0b10000000;          // OUT=1
						}
					else {
						}
					break;
				case 3:
				case 7:
					// free counter / square wave rate generator
					i8253TimerR[1]-=2;
					if(!i8253TimerR[1]) {
						i8253Mode[1] ^= 0b10000000;          // OUT=!OUT
						i8253TimerR[1]=i8253TimerW[1];
						if(i8253TimerR[1] & 1)
							i8253TimerR[1]--;
						}
					else {
						// 
						}
					break;
				case 4:
					// software triggered strobe
					if(!i8253TimerR[1]) {
						i8253Mode[1] &= ~0b10000000;          // OUT=0
						if(i8253Mode[1] & 0b01000000)          // reloaded
							;
						}
					else {
						i8253TimerR[1]--;
						i8253Mode[1] |= 0b10000000;          // OUT=1
						}
					break;
				case 5:
					// hardware triggered strobe
					if(!i8253TimerR[1]) {
						i8253Mode[1] &= ~0b10000000;          // OUT=0
						if(i8253Mode[1] & 0b01000000)          // reloaded
							;
						}
					else {
						i8253TimerR[1]--;
						i8253Mode[1] |= 0b10000000;          // OUT=1
						}
					break;
				}
			switch((i8253Mode[2] & 0b00001110) >> 1) {// VERIFICARE altri modi a parte free running :)
				case 0:		// INTerrupt on terminal count
					// (continous) output is low and goes high at counting end
					i8253TimerR[2]--;
//test	5150 cassette				i8253Mode[2] |= 0b10000000;
					if(i8253Mode[2] & 0b01000000) {         // reloaded
						if(!i8253TimerR[2]) {
							i8253Mode[2] &= ~0b01000000;
							i8253Mode[2] |= 0b10000000;          // OUT=1
						}
					else {
						i8253Mode[2] &= ~0b10000000;          // OUT=0
						}
							}
					break;
				case 1:
					// one-shot output is low and goes high at counting end (retriggerable)
					if(!i8253TimerR[2]) {
						if(i8253Mode[2] & 0b01000000)          // reloaded
							;
						i8253Mode[2] |= 0b10000000;          // OUT=1
						}
					else {
						i8253TimerR[2]--;
						i8253Mode[2] &= ~0b10000000;          // OUT=0
						}
					break;
				case 2:
				case 6:
					// free counter / rate generator
					i8253TimerR[2]--;
					if(i8253TimerR[2]==1) {
						i8253Mode[2] &= ~0b10000000;          // OUT=0
						}
					else if(!i8253TimerR[2]) {
						i8253Mode[2] |= 0b10000000;          // OUT=1
						}
					else {
						}
					break;
				case 3:
				case 7:
					// free counter / square wave rate generator
					i8253TimerR[2]-=2;
					if(!i8253TimerR[2]) {
						i8253Mode[2] ^= 0b10000000;          // OUT=!OUT
						i8253TimerR[2]=i8253TimerW[2];
						if(i8253TimerR[2] & 1)
							i8253TimerR[2]--;
						}
					else {
						// 
						}
					break;
				case 4:
					// software triggered strobe
					if(!i8253TimerR[2]) {
						i8253Mode[2] &= ~0b10000000;          // OUT=0
						if(i8253Mode[2] & 0b01000000)          // reloaded
							;
						}
					else {
						i8253TimerR[2]--;
						i8253Mode[2] |= 0b10000000;          // OUT=1
						}
					break;
				case 5:
					// hardware triggered strobe
					if(!i8253TimerR[2]) {
						i8253Mode[2] &= ~0b10000000;          // OUT=0
						if(i8253Mode[2] & 0b01000000)          // reloaded
							;
						}
					else {
						i8253TimerR[2]--;
						i8253Mode[2] |= 0b10000000;          // OUT=1
						}
					break;
				}
			if(i8253Mode[2] & 0b10000000)
				i8255RegR[2] |= 0b00100000;			// monitor timer ch 2...
			else
				i8255RegR[2] &= ~0b00100000;			// 
#ifdef PC_IBM5150
			if(i8253Mode[2] & 0b10000000)
				i8255RegR[2] |= 0b00010000;			// loopback cassette
			else
				i8255RegR[2] &= ~0b00010000;			// 
//				i8255RegR[2] |= 0b00010000;			// cassette...
#endif
#ifdef PC_IBM5160		// anche altri? boh
/*			if(i8253Mode[2] & 0b10000000)
				i8255RegR[2] |= 0b00010000;			// monitor speaker...
			else
				i8255RegR[2] &= ~0b00010000;			*/
#endif

			if(floppyTimer) {
				floppyTimer--;
				if(!floppyTimer) {
					i8259IRR |= 0x40;		// simulo IRQ...
					}
				}
			}

		if(!(i8237RegW[8] & 0b00000001)) {		// controller enable
			// canale 0 = RAM refresh (ignoro gli altri, v. floppy ecc in loco!
			switch(i8237Mode[0] & 0b00001100) {		// DMA mode
				case 0b00000000:			// verify
					break;
				case 0b00001000:			// write
					break;
				case 0b00000100:			// read
					break;
				}
			i8237RegR[13] /* = */; // ultimo byte trasferito :)
			if(!(i8237RegW[15] & (1 << 0)))	{	// mask...
				switch(i8237Mode[0] & 0b11000000) {		// DMA mode
					case 0b00000000:			// demand
						break;
					case 0b11000000:			// cascade
						break;
					case 0b10000000:			// single
					case 0b01000000:			// block
//						i8237RegR[8] |= ((1 << 0) << 4);  // request??... NO! IBM5160 si incazza; v. anche nel floppy anche se la va
						if(i8237Mode[0] & 0b00100000)
							i8237DMACurAddr[0]--;
						else
							i8237DMACurAddr[0]++;
						i8237DMACurLen[0]--;
						if(!i8237DMACurLen[0]) {
							i8237RegR[8] |= (1 << 0);		// TC 0
							if(i8237Mode[0] & 0b00010000) {
								i8237DMACurAddr[0]=i8237DMAAddr[0];
								i8237DMACurLen[0]=i8237DMALen[0];
								i8237RegR[8] &= ~((1 << 0) << 4);  // request done??... se confermato, mettere anchein floppy, HD
								}
							}
						break;
					}
				}
			}
    
/* 			c2++;
			if(c2<CPUDivider)
				continue;
			else
				c2=0;*/
/*		do {
			QueryPerformanceCounter(&c4);
			} while(c4.QuadPart<c3.QuadPart);*/
//		QueryPerformanceCounter(&c4);
//		if(c4.QuadPart<c3.QuadPart)		//(ovviamente si pianta nei test timer/DMA... GLABios, PCXTBios va
//			goto rallenta;


//printf("Pipe1: %02x, Pipe2w: %04x, Pipe2b1: %02x,%02x\n",Pipe1,Pipe2.word,Pipe2.bytes.byte1,Pipe2.bytes.byte2);
// http://ref.x86asm.net/coder32.html#two-byte
    switch(inRep) {
      case 0:
        break;
      case 1:   // REPZ/REPE
				if(!inRepStep)			// se era stato usato REP con istruzione non-stringa...
          goto fineRep;
        if(--_cx && _f.Zero) {
          _ip -= inRepStep;      // v. bug 8088!! così dovrebbe andare.. 22/7/24
					inEI++;
					}
        else
          goto fineRep;
        break;
      case 2:   // REPNZ/REPNE
				if(!inRepStep)			// se era stato usato REP con istruzione non-stringa...
          goto fineRep;
        if(--_cx && !_f.Zero) {
          _ip -= inRepStep;
					inEI++;
					}
        else
          goto fineRep;
        break;
      case 3:   // REP (v.singoli casi)
				if(!inRepStep)			// se era stato usato REP con istruzione non-stringa...
          goto fineRep;
        if(--_cx) {
          _ip -= inRepStep;
					inEI++;
					}
        else {
fineRep:
          inRep=inRepStep=0;
					segOverride=0;
#ifdef EXT_80386
		      sizeOverride=0;
#endif
					inEI=0;

					}
        break;
#ifdef EXT_NECV20
      case 5:   // REPNC
				if(!inRepStep)			// se era stato usato REP con istruzione non-stringa...
          goto fineRep;
        if(--_cx && _f.Carry) {
          _ip -= inRepStep;
					inEI++;
					}
        else
          goto fineRep;
        break;
      case 6:   // REPC
				if(!inRepStep)			// se era stato usato REP con istruzione non-stringa...
          goto fineRep;
        if(--_cx && !_f.Carry) {
          _ip -= inRepStep;
					inEI++;
					}
        else
          goto fineRep;
        break;
#endif
      default:      // al primo giro...
        inRep &= 0xf;
				inEI++;
        break;
      }


 
/*    if(_ip == 0x01ee) {
      Nop();
      Nop();
      }*/
    

    
     LED1 ^= 1;      // ~ 500/700nS  2/12/19 (con opt=1); un PC/XT @4.77 va a 200nS e impiega una media di 10/20 cicli per opcode => 2-4uS, ergo siamo 6-8 volte più veloci
                    // 500-1uS 22/7/24 con O2! [.9-2uS 16/7/24 senza ottimizzazioni (con O1 si pianta emulatore...
     // 2025 dopo aggiunte varie, specie seg:ofs, siamo sui 1-1.5uS con O2
     
#ifdef USING_SIMULATOR
// fa cagare, lentissimo anche con baud rate alto     printf("CS:IP=%04X:%04X\n",_cs,_ip);
#endif
// http://www.mlsite.net/8086/  per hex-codes
		switch(GetPipe(_cs,_ip++)) {

			case 0:     // ADD registro a r/m
			case 1:
			case 2:     // ADD r/m a registro
			case 3:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
								res3.b = res1.b+res2.b;      
                PutValue(*theDs,op2.mem,res3.b);
                goto aggFlagAB;
                break;
              case 1:
#ifdef EXT_80386
								if(sizeOverride) {
									sizeOverrideA;
									res1.d=GetIntValue(*theDs,op2.mem);
									sizeOverride=0;
									}
								else
#endif
									res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
        				res3.x = res1.x+res2.x;   
                PutShortValue(*theDs,op2.mem,res3.x);
                goto aggFlagAW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
								res3.b = res1.b+res2.b;      
                *op1.reg8 = res3.b;
                goto aggFlagAB;
                break;
              case 3:
                res1.x=*op1.reg16;
#ifdef EXT_80386
								if(sizeOverride) {
									sizeOverrideA;
									res2.d=GetIntValue(*theDs,op2.mem));
									sizeOverride=0;
									}
								else
#endif
	                res2.x=GetShortValue(*theDs,op2.mem);
        				res3.x = res1.x+res2.x;   
                *op1.reg16 = res3.x;
                goto aggFlagAW;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.b = res1.b+res2.b;      
                *op2.reg8=res3.b;
                goto aggFlagAB;
                break;
              case 1:
                res1.x=*op2.reg16;
#ifdef EXT_80386
								if(sizeOverride) {
									sizeOverrideA;
									res2.d=*op1.reg32;
									sizeOverride=0;
									}
								else
#endif
	                res2.x=*op1.reg16;
        				res3.x = res1.x+res2.x;   
                *op2.reg16=res3.x;
                goto aggFlagAW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.b = res1.b+res2.b;      
                *op1.reg8=res3.b;
                goto aggFlagAB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
        				res3.x = res1.x+res2.x;   
#ifdef EXT_80386
								if(sizeOverride) {
									sizeOverrideA;
									*op1.reg32=res3.d;
									sizeOverride=0;
									}
								else
#endif
	                *op1.reg16=res3.x;
                goto aggFlagAW;
                break;
              }
            break;
          }
				break;
        
			case 4:       // ADDB immediate
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b+res2.b;      
        _al = res3.b;
				_ip++;
				
aggFlagAB:     // carry, ovf, aux, zero, sign, parity
        _f.Ovf = OVF_ADD_8();
				_f.Carry=CARRY_ADD_8();
        
aggFlagABA:    // aux, zero, sign, parity
        _f.Aux = AUX_ADD_8();

aggFlagBSZP:    // zero, sign, parity
				_f.Zero=ZERO_8();
				_f.Sign=SIGN_8();

aggParity:
        {
        BYTE par;
        par= res3.b >> 1;			// Microchip AN774; https://stackoverflow.com/questions/17350906/computing-the-parity
        par ^= res3.b;
        res3.b= par >> 2;
        par ^= res3.b;
        res3.b= par >> 4;
        par ^= res3.b;
        _f.Parity=par & 1 ? 0 : 1;		// invertito qua pd £$%&/
        }
				break;

			case 5:       // ADDW immediate
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x+res2.x;   
        _ax = res3.x;
				_ip+=2;
        
aggFlagAW:     // carry, ovf, aux, zero, sign, parity
        _f.Ovf = OVF_ADD_16();
				_f.Carry=CARRY_ADD_16();

aggFlagAWA:    // aux, zero, sign, parity
        _f.Aux = AUX_ADD_8();
        
aggFlagWSZP:    // zero, sign, parity
				_f.Zero=ZERO_16();
				_f.Sign=SIGN_16();
//        parn= par >> 8;   // no! dice che cmq è solo sul byte basso https://stackoverflow.com/questions/29292455/parity-of-a-number-assembly-8086
//        par ^= parn;
				goto aggParity;
				break;
        
			case 6:       // PUSH segment
			case 0xe:
			case 0x16:
			case 0x1e:
				PUSH_STACK(segs.r[(Pipe1 >> 3) & 3].x);
				break;
        
			case 0xf:       // non esiste POP CS (abbastanza logico); istruzioni MMX/386/(V20) qua...
				_ip++;
//     		switch(GetPipe(_cs,++_ip))) {
//#warning PATCH MiniBIOS per caricare DOS...
        
        switch(Pipe2.b.l) {
          case 0x0:      // "putchar" da emulatore Tiny... lo lascio per alcuni casi tipo int10h tty
#ifndef __DEBUG
            _mon_putc(_al);
#else
            cwrite(_al);
#endif
            break;
          case 0x01:      // leggere da 146818 o meglio farlo da bios!
            break;
#ifdef RAM_DOS 
          case 0x2:      // read-disk, pseudo-load MBR :)
            switch(_al) {
              case 0:   // (pseudo) settori :) ma è in CH/CL...
                memcpy(&ram_seg[_bx /*0x007c00*/],MBR, 512 /*sizeof(MBR)*/);
                _ah=0;
                _al=1  /*sizeof(MBR)/512*/;
                _f.Carry=0;     //ritorno OK
                break;
              case 1:
                memcpy(&ram_seg[_bx /*0x000700*/],iosys, 512 /*sizeof(iosys)*/);
                _ah=0;
                _al=1  /*sizeof(iosys)/512*/;
                _f.Carry=0;     //ritorno OK
                break;
#if 0
/*

	https://github.com/microsoft/MS-DOS/tree/master/v2.0

	anche


SEG_DOS_TEMP    equ     0xE0            ; segment in which DOS was loaded
SEG_DOS         equ     0xB1            ; segment in which DOS will run
SEG_BIO         equ     0x60            ; segment in which BIO is running



0000:0000                Interrupt vector table
0040:0000                ROM BIOS data area
0050:0000                DOS parameter area
0070:0000                IBMBIO.COM / IO.SYS * opp 60!
mmmm:mmmm                IBMDOS.COM / MSDOS.SYS *
mmmm:mmmm                CONFIG.SYS - specified information
                         (device drivers and internal buffers
mmmm:mmmm                Resident COMMAND.COM
mmmm:mmmm                Master environment
mmmm:mmmm                Environment block #1
mmmm:mmmm                Application program #1
     .                        .      .                        .      .                        .
mmmm.mmmm                Environment block #n
mmmm:mmmm                Application #n
xxxx:xxxx                Transient COMMAND.COM
A000:0000                Video buffers and ROM
FFFF:000F                Top of 8086 / 88 address space*/
              case 1:
                memcpy(&ram_seg[_di /*0x000700*/],iosys,sizeof(iosys));
                break;
              case 2:
                memcpy(&ram_seg[_di /*(0x000700+sizeof(iosys)+15) & 0xffff0*/],msdossys,sizeof(msdossys));
                break;
              case 3:
                memcpy(&ram_seg[_di /*(0x000700+sizeof(iosys)+sizeof(msdossys)+15) & 0xffff0*/],commandcom,sizeof(commandcom));
                break;
#endif
              default:
                _f.Carry=1;     //ritorno no
                break;
              }
            break;
#endif
            
//          case 0x1 0x2 0x3:      // altro da emulatore Tiny... CAMBIARE!
          case 0x3:      // write-disk, usare Flash! DEE ecc
            _f.Carry=1;     //ritorno no
            break;
#ifndef EXT_NECV20
          case 0xff:      // TRAP
//            Nop();
            break;
#endif
            

#ifdef EXT_NECV20
					case 0x10:						// TEST1, CLR1, SET1, NOT1 , cl
					case 0x11:// (OCCHIO pare che SET1 possa andare anche su Carry o su DIR... verificare, anche le altre!
					case 0x12:// ah no che frociata, sono le altre istruzioni, STC STD porcamadonna gli umani col cancro!!
					case 0x13:
					case 0x14:
					case 0x15:
					case 0x16:
					case 0x17:
						// servirà //        GetMorePipe(_cs,_ip-1));
						COMPUTE_RM
								if(!(Pipe1 & 1)) {
									res1.b=GetValue(*theDs,op2.mem);
									}
								else {
									res1.x=GetShortValue(*theDs,op2.mem);
									}
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (_cl & 0x7));
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (_cl & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (_cl & 0xf));
											}
										break;
									case 2:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (_cl & 0xf));
											}
										break;
									case 3:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (_cl & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
									PutValue(*theDs,op2.mem,res3.b);
									}
								else {
									PutShortValue(*theDs,op2.mem,res3.x);
									}
								break;
							case 3:
								GET_REGISTER_8_16_2
								res1.b=*op2.reg8;
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (_cl & 0x7));
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (_cl & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (_cl & 0xf));
											}
										break;
									case 2:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (_cl & 0xf));
											}
										break;
									case 3:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (_cl & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
	                *op2.reg8=res3.b;
									}
								else {
	                *op2.reg16=res3.x;
									}
								break;
							}
            break;
					case 0x18:						// TEST1, CLR1, SET1, NOT1 , #imm
					case 0x19:
					case 0x1a:
					case 0x1b:
					case 0x1c:
					case 0x1d:
					case 0x1e:
					case 0x1f:
						// servirà //        GetMorePipe(_cs,_ip-1));

						COMPUTE_RM
								if(!(Pipe1 & 1)) {
									res1.b=GetValue(*theDs,op2.mem);
									}
								else {
									res1.x=GetShortValue(*theDs,op2.mem);
									}
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (Pipe2.b.u & 0x7));			// VERIFICARE DOV'è IMM8!!
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (Pipe2.b.u & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 2:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 3:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (Pipe2.b.u & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
									PutValue(*theDs,op2.mem,res3.b);
									}
								else {
									PutShortValue(*theDs,op2.mem,res3.x);
									}
								break;
							case 3:
								GET_REGISTER_8_16_2
								res1.b=*op2.reg8;
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (Pipe2.b.u & 0x7));
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (Pipe2.b.u & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 2:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 3:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (Pipe2.b.u & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
	                *op2.reg8=res3.b;
									}
								else {
	                *op2.reg16=res3.x;
									}
								break;
							}
            break;
					case 0x20:					// ADD4S
						{BYTE j=0;
						uint16_t di1=_di,si1=_si;
						i=(((uint16_t)_cl)+1) >> 1;
						_f.Zero=1;
						while(i--) {
							res1.b=GetValue(*theDs,si1);
							res2.b=GetValue(_es,di1);
              res1.b = (res1.b >> 4)*10 + (res1.b & 0xf);
              res2.b = (res2.b >> 4)*10 + (res2.b & 0xf);
              res3.b = res1.b + res2.b + j;
              if(res3.b>99) 
								j=1; 
							else 
								j=0;
              res3.b = res3.b % 100;
              res3.b = ((res3.b/10) << 4) | (res3.b % 10);

							if(j)
								_f.Carry=1;
							if(res3.b)
								_f.Zero=0;

							PutValue(_es,_di,res3.b);
							si1++;
							di1++;
							}
						}
            break;
					case 0x22:					// SUB4S
						{BYTE j=0;
						uint16_t di1=_di,si1=_si;
						i=(((uint16_t)_cl)+1) >> 1;
						_f.Zero=1;
						while(i--) {
							res1.b=GetValue(*theDs,si1);
							res2.b=GetValue(_es,di1);
              res1.b = (res1.b >> 4)*10 + (res1.b & 0xf);
              res2.b = (res2.b >> 4)*10 + (res2.b & 0xf);
              if(res1.b < (res2.b+j)) {
                res1.b = res1.b + 100;
                res3.b = res1.b - (res2.b+j);
                i= 1;
	              }
              else  {
                res3.b = res1.b - (res2.b+j);
                i= 0;
								}
              res3.b = ((res3.b/10)<<4) | (res3.b % 10);

							if(j)
								_f.Carry=1;
							if(res3.b)
								_f.Zero=0;

							PutValue(_es,_di,res3.b);
							si1++;
							di1++;
							}
						}
            break;
					case 0x26:					// CMP4S
						{BYTE j=0;
						uint16_t di1=_di,si1=_si;
						i=(((uint16_t)_cl)+1) >> 1;
						_f.Zero=1;
						while(i--) {
							res1.b=GetValue(*theDs,si1);
							res2.b=GetValue(_es,di1);
              res1.b = (res1.b >> 4)*10 + (res1.b & 0xf);
              res2.b = (res2.b >> 4)*10 + (res2.b & 0xf);
              if(res1.b < (res2.b+j)) {
                res1.b = res1.b + 100;
                res3.b = res1.b - (res2.b+j);
                i= 1;
	              }
              else  {
                res3.b = res1.b - (res2.b+j);
                i= 0;
								}
              res3.b = ((res3.b/10)<<4) | (res3.b % 10);

							if(j)
								_f.Carry=1;
							if(res3.b)
								_f.Zero=0;

							si1++;
							di1++;
							}
						}
            break;
					case 0x28:					// ROL4
					case 0x2a:					// ROR4
  					_ip++;
						COMPUTE_RM
								res1.b=GetValue(*theDs,op2.mem);
								res2.b=*op1.reg8;
								if(Pipe2.b.l == 0x28) {
                  res2.b = (_al << 4) | ((res1.b >> 4) & 0x000F) ;
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=res2.b;
									}
								else {
                  res2.b = (res1.b >> 4) | ((_al << 4) & 0x00F0);
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=(_al & 0xFF00) | (res2.b & 0x00FF);
									}
								PutValue(*theDs,op2.mem,res3.b);
							break;
						case 3:
							GET_REGISTER_8_16_2
								res1.b=*op2.reg8;
								res2.b=*op1.reg8;
								if(Pipe2.b.l == 0x28) {
                  res2.b = (_al << 4) | ((res1.b >> 4) & 0x000F);
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=res2.b;
									}
								else {
                  res2.b = (_al << 4) | ((res1.b >> 4) & 0x000F);
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=res2.b;
									}
                *op2.reg8=res3.b;
								break;
							}
            break;
					case 0x31:					// INS
						{BYTE j1,j2;
						if(!theDs)
							theDs=&_es;
						res1.b=GetValue(*theDs,_di);
						res3.x=_ax;

						PutShortValue(*theDs,_di,res3.x);
						}
            break;
					case 0x39:					// INS
						{BYTE j1,j2;
						if(!theDs)
							theDs=&_es;
						j2=Pipe2.b.u & 0xf;			// VERIFICARE DOV'è IMM8!!
						res3.x=_ax;

						PutShortValue(*theDs,_di,res3.x);
						}
            break;
					case 0x33:					// EXT
						{BYTE j1,j2;
						res1.x=GetShortValue(*theDs,_si);

						_ax=res3.x;
						}
            break;
					case 0x3b:					// EXT
						{BYTE j1,j2;
						j2=Pipe2.b.u & 0xf;			// VERIFICARE DOV'è IMM8!!
						res1.x=GetShortValue(*theDs,_si);

						_ax=res3.x;
						}
            break;
					case 0xff:					// BRKMM
						// entra in modalità 8080!!
						_f.MD=0;
            break;
#endif

#ifdef EXT_80286
          case 0x0:      // LLDT/LTR/SLDT/SMSW/VERW
            break;
          case 0x1:      // LGDT/LIDT/LMSW/SGDT/SIDT/STR
            break;
          case 0x2:      // LAR
            break;
          case 0x3:      // LSL
            break;
          case 0x6:      // CLTS
            break;
#endif
#ifdef EXT_80386
          case 0x5:      // SYSCALL
            break;
          case 0x31:      // RDTSC; si potrebbe mettere anche in 8086??!
            _eax=c;
            break;
          case 0x20:      // MOV
          case 0x21:
          case 0x22:
          case 0x23:
          case 0x24:
          case 0x26:
            break;
          case 0x34:      // SYSENTER
            break;
          case 0x35:      // SYSEXIT
            break;
          case 0x6e:      // MOVD
            break;
          case 0x6f:      // MOVQ
            break;
          case 0x7e:      // MOVD
            break;
          case 0x7f:      // MOVQ
            break;
          case 0x80:      // JO
            break;
          case 0x82:      // JB
            break;
          case 0x83:      // JAE
            break;
          case 0x84:      // JZ
            break;
          case 0x85:      // JNZ
            break;
          case 0x86:      // JBE
            break;
          case 0x87:      // JA
            break;
          case 0x88:      // JS
            break;
          case 0x89:      // JNS
            break;
          case 0x8a:      // JP
            break;
          case 0x8b:      // JNP
            break;
          case 0x8c:      // JL
            break;
          case 0x8d:      // JGE
            break;
          case 0x8e:      // JLE
            break;
          case 0x8f:      // JG
            break;
          case 0x90:      // SETO
            
            break;
          case 0x91:      // SETNO
            break;
          case 0x92:      // SETB
            break;
          case 0x93:      // SETAE
            break;
          case 0x94:      // SETZ
            break;
          case 0x95:      // SETNZ
            break;
          case 0x96:      // SETBE
            break;
          case 0x98:      // SETS
            break;
          case 0x99:      // SETNS
            break;
          case 0x9a:      // SETP
            break;
          case 0x9b:      // SETNP
            break;
          case 0x9c:      // SETL
            break;
          case 0x9d:      // SETGE
            break;
          case 0x9e:      // SETLE
            break;
          case 0x9f:      // SETG
            break;
          case 0xa4:      // SHLD
          case 0xa5:
            break;
          case 0xa0:      // PUSHFS
    				PUSH_STACK(_fs);
            break;
          case 0xa1:      // POPFS
    				POP_STACK(_fs);
            break;
          case 0xa2:      // CPUID
            _eax= GenuineIntel  GDIntel2025
            break;
          case 0xa8:      // PUSHGS
    				PUSH_STACK(_gs);
            break;
          case 0xa9:      // POPGS
    				POP_STACK(_gs);
            break;
          case 0xac:      // SHRD
          case 0xad:
            break;
          case 0xaf:      // IMUL
            break;
          case 0xa3:      // BT
          case 0xba:      // BT, BTC, BTR, BTS
            break;
          case 0xab:      // BTS
            break;
          case 0xaf:      // MUL
            break;
          case 0xb2:      // LSS
            break;
          case 0xb3:      // BTR
            break;
          case 0xb4:      // LFS
            break;
          case 0xb5:      // LGS
            break;
          case 0xbb:      // BTC
            break;
          case 0xbc:      // BSF
            break;
          case 0xbd:      // BSR
            break;
          case 0xb6:      // MOVZX
          case 0xb7:
            break;
          case 0xbe:      // MOVSX
          case 0xbf:
            break;
          case 0xc8:      // BYTESWAP
          case 0xc9:
          case 0xca:
          case 0xcb:
          case 0xcc:
          case 0xcd:
          case 0xce:
          case 0xcf:
            break;
//      case 0x0f:				//LSS
        /* deve seguire B2.. verificare
#define WORKING_REG regs.r[(Pipe2.reg].x      // CONTROLLARE
				i=GetShortValue(Pipe2.xm.x);
				WORKING_REG=GetShortValue(i);
        i+=2;
				_ss=GetShortValue(i);
        _ip+=2;
        */
#ifdef EXT_80386
        			// e se segue b4 b5 è fs o gs!
#define WORKING_REG regs.r[(Pipe2.reg].x      // CONTROLLARE
				i=GetShortValue(Pipe2.xm.w);
				WORKING_REG=GetShortValue(i);
        i+=2;
				_fs=GetShortValue(i);
        _ip+=2;
#endif
//				break;
#endif
#ifdef EXT_80486
          case 0x1:      // INVLPG
            break;
          case 0x8:      // INVD
            break;
          case 0x9:      // WBINVD
            break;
          case 0x1f:      // NOP...
            break;
          case 0x34:      // SYSENTER
            break;
          case 0xa6:      // CMPXCHG/CMPXCHG8B
          case 0xa7:
          case 0xb0:
          case 0xb1:
          case 0xc7:
            break;
          case 0xc0:      // XADD
          case 0xc1:      // XADD
            break;
          case 0x80:      // Jcc near
          case 0x81:
          case 0x82:
          case 0x83:
          case 0x84:
          case 0x85:
          case 0x86:
          case 0x87:
          case 0x88:
          case 0x89:
          case 0x8a:
          case 0x8b:
          case 0x8c:
          case 0x8d:
          case 0x8e:
          case 0x8f:
            break;
          case 0x90:      // SETcc
          case 0x91:
          case 0x92:
          case 0x93:
          case 0x94:
          case 0x95:
          case 0x96:
          case 0x97:
          case 0x98:
          case 0x99:
          case 0x9a:
          case 0x9b:
          case 0x9c:
          case 0x9d:
          case 0x9e:
          case 0x9f:
            break;
#endif
          }
				break;
        
			case 7:         // POP segment
			case 0x17:
			case 0x1f:
				POP_STACK(segs.r[(Pipe1 >> 3) & 3].x);
#ifdef EXT_NECV20	
				inEI++;			// boh, NECV20 emulator lo fa...
#endif
				break;

			case 0x08:      // OR
			case 0x09:
			case 0x0a:
			case 0x0b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
                res3.b = res1.b | res2.b;
                PutValue(*theDs,op2.mem,res3.b);

aggFlagBZC:
								_f.Carry=_f.Ovf=0;   // Aux undefined
#ifdef UNDOCUMENTED_8086
							  _f.Aux=0;			// pare... da gloriouscow...
#endif
								goto aggFlagBSZP;
                break;
              case 1:
                res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
                res3.x = res1.x | res2.x;
                PutShortValue(*theDs,op2.mem,res3.x);

aggFlagWZC:
								_f.Carry=_f.Ovf=0;   // Aux undefined
#ifdef UNDOCUMENTED_8086
							  _f.Aux=0;			// pare... da gloriouscow...
#endif
								goto aggFlagWSZP;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
                res3.b = res1.b | res2.b;
                *op1.reg8 = res3.b;
								goto aggFlagBZC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(*theDs,op2.mem);
                res3.x = res1.x | res2.x;
                *op1.reg16 = res3.x;
								goto aggFlagWZC;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
                res3.b=res1.b | res2.b;
                *op2.reg8=res3.b;
								goto aggFlagBZC;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x=res1.x | res2.x;
                *op2.reg16=res3.x;
								goto aggFlagWZC;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
                res3.b=res1.b | res2.b;
                *op1.reg8=res3.b;
								goto aggFlagBZC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x=res1.x | res2.x;
                *op1.reg16=res3.x;
								goto aggFlagWZC;
                break;
              }
            break;
          }
				break;

			case 0x0c:      // OR
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b | res2.b;
        _al = res3.b;
				_ip++;
        goto aggFlagBZC;
				break;

			case 0x0d:      // OR
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x | res2.x;
        _ax = res3.x;
				_ip+=2;
        goto aggFlagWZC;
				break;

			case 0x10:     // ADC registro a r/m
			case 0x11:
			case 0x12:     // ADC r/m a registro
			case 0x13:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
								res3.b = res1.b+res2.b+_f.Carry;
                PutValue(*theDs,op2.mem,res3.b);

aggFlagABC:
								_f.Ovf = OVF_ADC_8();
								_f.Carry=CARRY_ADC_8();
				        _f.Aux = AUX_ADC_8();
								goto aggFlagBSZP;
                break;
              case 1:
                res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
                res3.x = res1.x+res2.x+_f.Carry;
                PutShortValue(*theDs,op2.mem,res3.x);

aggFlagAWC:
								_f.Ovf = OVF_ADC_16();
								_f.Carry=CARRY_ADC_16();
				        _f.Aux = AUX_ADC_8();
                goto aggFlagWSZP;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
								res3.b = res1.b+res2.b+_f.Carry;      
                *op1.reg8 = res3.b;
                goto aggFlagABC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(*theDs,op2.mem);
                res3.x = res1.x+res2.x+_f.Carry;
                *op1.reg16 = res3.x;
                goto aggFlagAWC;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.b = res1.b+res2.b+_f.Carry;      
                *op2.reg8=res3.b;
                goto aggFlagABC;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x = res1.x+res2.x+_f.Carry;
                *op2.reg16=res3.x;
                goto aggFlagAWC;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.b = res1.b+res2.b+_f.Carry;      
                *op1.reg8=res3.b;
                goto aggFlagABC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x = res1.x+res2.x+_f.Carry;
                *op1.reg16=res3.x;
                goto aggFlagAWC;
                break;
              }
            break;
          }
				break;

			case 0x14:        // ADC
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b+res2.b+_f.Carry;
        _al = res3.b;
				_ip++;
        goto aggFlagABC;
				break;

			case 0x15:        // ADC
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x+res2.x+_f.Carry;   
        _ax = res3.x;
				_ip+=2;
        goto aggFlagAWC;
				break;

			case 0x18:        // SBB
			case 0x19:
			case 0x1a:
			case 0x1b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
								res3.b = res1.b-res2.b-_f.Carry;
                PutValue(*theDs,op2.mem,res3.b);

aggFlagSBB:
								_f.Ovf = OVF_SBB_8();
								_f.Carry=CARRY_SBB_8();
				        _f.Aux = AUX_SBB_8();
                goto aggFlagBSZP;
                break;
              case 1:
                res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
                res3.x = res1.x-res2.x-_f.Carry;
                PutShortValue(*theDs,op2.mem,res3.x);

aggFlagSWB:
								_f.Ovf = OVF_SBB_16();
								_f.Carry=CARRY_SBB_16();
				        _f.Aux = AUX_SBB_8();
                goto aggFlagWSZP;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
								res3.b = res1.b-res2.b-_f.Carry;      
                *op1.reg8 = res3.b;
                goto aggFlagSBB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(*theDs,op2.mem);
                res3.x = res1.x-res2.x-_f.Carry;
                *op1.reg16 = res3.x;
                goto aggFlagSWB;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.b = res1.b-res2.b-_f.Carry;      
                *op2.reg8=res3.b;
                goto aggFlagSBB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x = res1.x-res2.x-_f.Carry;
                *op2.reg16=res3.x;
                goto aggFlagSWB;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.b = res1.b-res2.b-_f.Carry;      
                *op1.reg8=res3.b;
                goto aggFlagSBB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x = res1.x-res2.x-_f.Carry;
                *op1.reg16=res3.x;
                goto aggFlagSWB;
                break;
              }
            break;
          }
				break;

			case 0x1c:        // SBB
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b-res2.b-_f.Carry;      
        _al = res3.b;
				_ip++;
        goto aggFlagSBB;
				break;

			case 0x1d:        // SBB
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x-res2.x-_f.Carry;   
        _ax = res3.x;
				_ip+=2;
        goto aggFlagSWB;
				break;

			case 0x20:        // AND
			case 0x21:
			case 0x22:
			case 0x23:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
                res3.b = res1.b & res2.b;
                PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagBZC;
                break;
              case 1:
                res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
                res3.x = res1.x & res2.x;
                PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagWZC;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
                res3.b = res1.b & res2.b;
                *op1.reg8 = res3.b;
								goto aggFlagBZC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(*theDs,op2.mem);
                res3.x = res1.x & res2.x;
                *op1.reg16 = res3.x;
								goto aggFlagWZC;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
                res3.b = res1.b & res2.b;
                *op2.reg8=res3.b;
								goto aggFlagBZC;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x = res1.x & res2.x;
                *op2.reg16=res3.x;
								goto aggFlagWZC;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
                res3.b = res1.b & res2.b;
                *op1.reg8=res3.b;
								goto aggFlagBZC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x = res1.x & res2.x;
                *op1.reg16=res3.x;
								goto aggFlagWZC;
                break;
              }
            break;
          }
				break;

			case 0x24:        // AND
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b & res2.b;
        _al = res3.b;
				_ip++;
        goto aggFlagBZC;
				break;

			case 0x25:        // AND
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x & res2.x;
        _ax = res3.x;
				_ip+=2;
        goto aggFlagWZC;
				break;

			case 0x26:
				segOverride=0+1;			// ES
        inEI++;
				break;

			case 0x27:				// DAA
        res3.b=_al;
				i=_f.Carry;
        if((_al & 0xf) > 9 || _f.Aux) {
          res3.b+=6;
          _f.Carry= i || (res3.b<5 ? 1 : 0);
          _f.Aux=1;
          }
        else
          _f.Aux=0;
        if(_al>0x99 || i) {
          res3.b+=0x60;  
          _f.Carry=1;
          }
        else
          _f.Carry=0;

#ifdef UNDOCUMENTED_8086
/*      u8 old_AL = registers[AX]&0xFF;
        bool weird_special_case = (!flag(F_CARRY)) && flag(F_AUX_CARRY);

        u8 added{};

        set_flag(F_AUX_CARRY, (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
        if (flag(F_AUX_CARRY))
            added += 0x06;

        set_flag(F_CARRY, old_AL > 0x99+(weird_special_case?6:0) || flag(F_CARRY));
        if (flag(F_CARRY))
            added += 0x60;

        get_r8(0) += added;

        set_flag(F_OVERFLOW, (old_AL ^ registers[AX]) & (added ^ registers[AX])&0x80);
				*/

				_f.Ovf=!!(((_al ^ res3.b) ) & 0x80);			// pare... da gloriouscow... 
#endif

        _al=res3.b;
#ifdef UNDOCUMENTED_8086
        goto aggFlagBSZP;
#endif
        break;
        
			case 0x28:      // SUB
			case 0x29:
			case 0x2a:
			case 0x2b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
								res3.b = res1.b-res2.b;      
                PutValue(*theDs,op2.mem,res3.b);
                goto aggFlagSB;
                break;
              case 1:
                res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
                res3.x = res1.x-res2.x;
                PutShortValue(*theDs,op2.mem,res3.x);
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
								res3.b = res1.b-res2.b;      
                *op1.reg8 = res3.b;
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(*theDs,op2.mem);
                res3.x = res1.x-res2.x;
                *op1.reg16 = res3.x;
                goto aggFlagSW;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.b = res1.b-res2.b;      
                *op2.reg8=res3.b;
                goto aggFlagSB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x = res1.x-res2.x;
                *op2.reg16=res3.x;
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.b = res1.b-res2.b;      
                *op1.reg8=res3.b;
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x = res1.x-res2.x;
                *op1.reg16=res3.x;
                goto aggFlagSW;
                break;
              }
            break;
          }
				break;

			case 0x2c:      // SUBB
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b-res2.b;      
        _al = res3.b;
				_ip++;

aggFlagSB:     // carry, ovf, aux, zero, sign, parity
        _f.Ovf = OVF_SUB_8();
				_f.Carry=CARRY_SUB_8();
        
aggFlagSBA:    // aux, zero, sign, parity
        _f.Aux = AUX_SUB_8();
        goto aggFlagBSZP;
				break;

			case 0x2d:			// SUBW
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x-res2.x;   
        _ax = res3.x;
				_ip+=2;
        
aggFlagSW:     // carry, ovf, aux, zero, sign, parity
        _f.Ovf = OVF_SUB_16();
				_f.Carry=CARRY_SUB_16();

aggFlagSWA:    // aux, zero, sign, parity
        _f.Aux = AUX_SUB_8();
        goto aggFlagWSZP;
				break;

			case 0x2e:
				segOverride=1+1;			// CS
        inEI++;
				break;

			case 0x2f:				// DAS
        res3.b=_al;
				i=_f.Carry;
        _f.Carry=0;
        if((_al & 0xf) > 9 || _f.Aux) {
          res3.b-=6;
          _f.Carry= i || (res3.b>=250 ? 1 : 0);
          _f.Aux=1;
          }
        else
          _f.Aux=0;
        if(_al>0x9f || i) {
          res3.b-=0x60;  
          _f.Carry=1;
          }

#ifdef UNDOCUMENTED_8086
/*            u8 old_AL = registers[AX] & 0xFF;
          bool weird_special_case = (!flag(F_CARRY)) && flag(F_AUX_CARRY);

          u8 subtracted{};

          bool sub_al = ((registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
          if (sub_al)
              subtracted += 0x06;

          set_flag(F_AUX_CARRY, sub_al);
          bool sub_al2 = (old_AL > (0x99+(weird_special_case?6:0)) || flag(F_CARRY));
          if (sub_al2)
              subtracted += 0x60;

          get_r8(0) -= subtracted;
          set_flag(F_CARRY, sub_al2);
          set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
          set_flag(F_SIGN, (registers[AX] & 0x80));
          set_flag(F_PARITY, byte_parity[registers[AX] & 0xFF]);
          set_flag(F_OVERFLOW, ((old_AL ^ subtracted) & (old_AL ^ registers[AX]))&0x80);
					*/
				_f.Ovf=!!(((_al ^ res3.b) ) & 0x80);			// pare... da gloriouscow... 
#endif

        _al=res3.b;
#ifdef UNDOCUMENTED_8086
        goto aggFlagBSZP;
#endif
				break;
        
			case 0x30:        // XOR
			case 0x31:
			case 0x32:
			case 0x33:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
                res3.b = res1.b ^ res2.b;
                PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagBZC;
                break;
              case 1:
                res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
                res3.x = res1.x ^ res2.x;
                PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagWZC;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
                res3.b = res1.b ^ res2.b;
                *op1.reg8 = res3.b;
								goto aggFlagBZC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(*theDs,op2.mem);
                res3.x = res1.x ^ res2.x;
                *op1.reg16 = res3.x;
								goto aggFlagWZC;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
                res3.b= res1.b ^ res2.b;
                *op2.reg8=res3.b;
								goto aggFlagBZC;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x= res1.x ^ res2.x;
                *op2.reg16=res3.x;
								goto aggFlagWZC;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
                res3.b= res1.b ^ res2.b;
                *op1.reg8=res3.b;
								goto aggFlagBZC;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x= res1.x ^ res2.x;
                *op1.reg16=res3.x;
								goto aggFlagWZC;
                break;
              }
            break;
          }
        break;
        
			case 0x34:        // XOR
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b ^ res2.b;
        _al = res3.b;
				_ip++;
        goto aggFlagBZC;
				break;

			case 0x35:        // XOR
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x ^ res2.x;
        _ax = res3.x;
				_ip+=2;
        goto aggFlagWZC;
				break;

			case 0x36:
        segOverride=2+1;			// SS
        inEI++;
				break;

			case 0x37:				// AAA
        if((_al & 0xf) > 9 || _f.Aux) {
          _ax+=0x106;
#ifdef UNDOCUMENTED_8086
					// FARE SEPARATI 1 e 6 gloriouscow
#endif
          _f.Aux=1;
          }
        else {
          _f.Aux=0;
          }
				_f.Carry=_f.Aux;

#ifdef UNDOCUMENTED_8086
/*          u16 old_AX = registers[AX];
          bool add_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
          if (add_ax)
          {
              get_r8(0) += 0x06; //AL
              get_r8(4) += 0x01; //AH
          }
          u16 added = registers[AX]-old_AX;

          set_flag(F_AUX_CARRY, add_ax);
          set_flag(F_CARRY, add_ax);
          set_flag(F_PARITY, byte_parity[registers[AX]&0xFF]);
          set_flag(F_ZERO, (registers[AX]&0xFF) == 0);
          set_flag(F_SIGN, (registers[AX] & 0x80));
          set_flag(F_OVERFLOW, (old_AX ^ registers[AX]) & (added ^ registers[AX])&0x80);

          get_r8(0) &= 0x0F;
					*/
//				_f.Ovf=!!((old_AX ^ _ax) & (added ^ _ax) & 0x80);			// pare... da gloriouscow... 
#endif

        _al &= 0xf;
#ifdef UNDOCUMENTED_8086
        goto aggFlagBSZP;
#endif
				break;

			case 0x38:      // CMP
			case 0x39:
			case 0x3a:
			case 0x3b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(*theDs,op2.mem);
                res2.b=*op1.reg8;
								res3.b = res1.b-res2.b;      
                goto aggFlagSB;
                break;
              case 1:
                res1.x=GetShortValue(*theDs,op2.mem);
                res2.x=*op1.reg16;
                res3.x = res1.x-res2.x;
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(*theDs,op2.mem);
								res3.b = res1.b-res2.b;      
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(*theDs,op2.mem);
                res3.x = res1.x-res2.x;
                goto aggFlagSW;
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.b = res1.b-res2.b;      
                goto aggFlagSB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x = res1.x-res2.x;
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.b = res1.b-res2.b;      
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x = res1.x-res2.x;
                goto aggFlagSW;
                break;
              }
            break;
          }
				break;
        
			case 0x3c:      // CMPB
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.b = res1.b-res2.b;      
				_ip++;
        goto aggFlagSB;
				break;

			case 0x3d:			// CMPW
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x-res2.x;   
				_ip+=2;
        goto aggFlagSW;
				break;

			case 0x3e:
        segOverride=3+1;			// DS
        inEI++;
				break;

 			case 0x3f:      // AAS
        if((_al & 0xf) > 9 || _f.Aux) {
          _ax-=6;
          _ah--;
          _f.Aux=1;
          }
        else {
          _f.Aux=0;
          }
				_f.Carry=_f.Aux;

#ifdef UNDOCUMENTED_8086
          /*u16 old_AX = registers[AX];
          bool sub_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
          if (sub_ax)
          {
              get_r8(4) -= 1;
              get_r8(0) -= 6;
          }
          u16 subtracted = old_AX-registers[AX];
          set_flag(F_AUX_CARRY, sub_ax);
          set_flag(F_CARRY, sub_ax);
          set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
          set_flag(F_SIGN, (registers[AX] & 0x80));
          set_flag(F_PARITY, byte_parity[registers[AX] & 0xFF]);
          set_flag(F_OVERFLOW, (old_AX ^ subtracted) & (old_AX ^ registers[AX])&0x80);

          get_r8(0) &= 0x0F;
					*/
//					_f.Ovf=!!((old_AX ^ subtracted) & (old_AX ^ _ax) & 0x80);			// pare... da gloriouscow... 
#endif

        _al &= 0xf;
#ifdef UNDOCUMENTED_8086
        goto aggFlagBSZP;
#endif
				break;
        
			case 0x40:      // INC
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
        op2.reg16 = &regs.r[Pipe1 & 0x7].x;
				res1.x = *op2.reg16;
				res2.x = 1;
				res3.x = res1.x+res2.x;
				*op2.reg16 = res3.x;

aggFlagIncW:
      	_f.Ovf= res3.x == 0x8000 ? 1 : 0; //V = 1 if x=7FFFH before, else 0
        goto aggFlagAWA;
				break;

			case 0x48:      // DEC
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4e:
			case 0x4f:
        op2.reg16 = &regs.r[Pipe1 & 0x7].x;
				res1.x = *op2.reg16;
				res2.x = 1;
				res3.x = res1.x-res2.x;
				*op2.reg16 = res3.x;

aggFlagDecW:
      	_f.Ovf= res3.x == 0x7fff ? 1 : 0; //V = 1 if x=8000H before, else 0
        goto aggFlagSWA;
				break;

			case 0x50:		// PUSH
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x55:
			case 0x56:
			case 0x57:
        op2.reg16 = &regs.r[Pipe1 & 0x7].x;
				PUSH_STACK(*op2.reg16);
				break;
			case 0x54:
#ifdef EXT_80286
				PUSH_STACK(_sp);
#else
				PUSH_STACK(_sp/*-2*/);
#endif
				break;

			case 0x58:		// POP
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x5d:
			case 0x5e:
			case 0x5f:
        op2.reg16 = &regs.r[Pipe1 & 0x7].x;
				POP_STACK(*op2.reg16);
				break;
			case 0x5c:
        op2.reg16 = &_sp;
#ifdef EXT_80286
				POP_STACK(*op2.reg16);
#else
				POP_STACK(*op2.reg16);
				_sp-=2;
#endif
				break;

#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0x60:      // PUSHA
        res3.x=regs.r[4].x;      // SS pushato INIZIALE!
        for(i=0; i<4; i++)    // 
          PUSH_STACK(regs.r[i].x);
        PUSH_STACK(res3.x);
        for(i=5; i<8; i++)    // 
          PUSH_STACK(regs.r[i].x);
				break;
			case 0x61:      // POPA
        for(i=7; i>=4; i--)    // 
          POP_STACK(regs.r[i].x);
        POP_STACK(_ax);     // dummy
        for(i=3; (int16_t)i>=0; i--)
          POP_STACK(regs.r[i].x);
				break;
			case 0x62:        // BOUND
#define WORKING_REG regs.r[Pipe2.reg].x
//        GetMorePipe(_cs,_ip-1));
        _ip+=3;
        if(WORKING_REG < Pipe2.x.l || WORKING_REG > Pipe2.x.h) {   // finire...
          // dovrebbe causare un INT 5
					i=5;
          goto do_irq;
					}

				break;
#endif
#ifdef EXT_80286
			case 0x63:        // ARPL
				break;
#endif
#ifdef EXT_NECV20
			case 0x63:        // undefined, dice, su V20...
				COMPUTE_RM
					}
				break;
#endif
        
#ifdef EXT_80386
			case 0x64:
				segOverride=4+1;			// FS
        inEI++;
				break;
        
			case 0x65:
				segOverride=5+1;			// GS
        inEI++;
				break;
#endif
#ifdef EXT_NECV20
			case 0x64:
				inRep=0x15;					// REPNC
        inEI++;
				break;
        
			case 0x65:
				inRep=0x16;					// REPC
				inEI++;		// forse ne fa uno di troppo alla fine, ma ok...
				break;
#endif
        
#ifdef EXT_80386
			case 0x66:
				//invert operand size next instr
        sizeOverride=2;
				inEI++;
        
     		switch(GetPipe(_cs,++_ip))) {
          case 0x99:    // anche CDQ
            _edx= _eax & 0x80000000 ? 0xffffffff : 0;
    				break;
          case 0x98:    // anche CWDE
            _eax |= _ax & 0x8000 ? 0xffff0000 : 0;
          	break;
          }
        
				break;
			case 0x67:
        sizeOverrideA=2;
				inEI++;
        
     		switch(GetPipe(_cs,++_ip))) {
          case 0x63:      // JCXZ
            break;
          }
				//invert address size next instr
				break;
#endif
#ifdef EXT_NECV20
			case 0x66:		// altre ESC per coprocessore NEC...
			case 0x67:
				COMPUTE_RM
					}
				break;
#endif
        
#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0x68:
				PUSH_STACK(Pipe2.x.l);
        _ip+=2;
				break;
        
			case 0x69:			//IMUL16
				{
			  union OPERAND op3;

        op1.reg16 = &regs.r[(Pipe2.b.l >> 3) & 0x7].x;		// 3 o 7??

				COMPUTE_RM_OFS
						GetPipe(_cs,_ip);
            op3.mem=Pipe2.x.l;
            res1.x=GetShortValue(*theDs,op2.mem);
            res2.x=op3.mem;
            res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;   // (sicuro 32? per i flag
#ifdef EXT_80386
            *op1.reg32=res3.d;		// finire!
#else
            *op1.reg16=res3.x;
#endif
						_f.Carry=_f.Ovf= (((res3.d & 0xffff8000) == 0xffff8000)  ||  
							((res3.d & 0xffff8000) == 0))  ? 0 : 1;
#ifdef EXT_NECV20

#else
//qua no						_f.Zero=0;
#endif
            break;
          case 3:
						//GET_REGISTER_8_16_2
            op2.reg16= &regs.r[Pipe2.b.l & 0x7].x;			// 3 o 7 ??
						GetMorePipe(_cs,_ip); 
            op3.mem=Pipe2.xm.w;
            res1.x=*op2.reg16;
            res2.x=op3.mem;
            res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;   // (sicuro 32? per i flag
#ifdef EXT_80386
            *op1.reg32=res3.d;		// finire!
#else
            *op1.reg16=res3.x;
#endif
						_f.Carry=_f.Ovf= (((res3.d & 0xffff8000) == 0xffff8000)  ||  
							((res3.d & 0xffff8000) == 0))  ? 0 : 1;
#ifdef EXT_NECV20

#else
//qua no						_f.Zero=0;
#endif
            break;
          }
 				_ip+=2;      // imm16
				}
				
				break;

			case 0x6a:        // PUSH imm8
				PUSH_STACK((int16_t)(int8_t)Pipe2.b.l);
        _ip++;
        break;

			case 0x6b:			//IMUL8
				{
			  union OPERAND op3;

        op1.reg16 = &regs.r[(Pipe2.b.l >> 3) & 0x7].x;		// 3 o 7??
       
				COMPUTE_RM_OFS
						GetPipe(_cs,_ip);
            op3.mem=(int16_t)(int8_t)Pipe2.b.l;
            res1.x=GetShortValue(*theDs,op2.mem);
            res2.x=op3.mem;
						res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;
#ifdef EXT_80386
            *op1.reg32=res3.d;		// finire!
#else
            *op1.reg16=res3.x;
#endif
						_f.Carry=_f.Ovf= (((res3.d & 0xffff8000) == 0xffff8000)  ||  
							((res3.d & 0xffff8000) == 0))  ? 0 : 1;
#ifdef EXT_NECV20

#else
//qua no						_f.Zero=0;
#endif
            break;
          case 3:
						//GET_REGISTER_8_16_2
            op2.reg16= &regs.r[Pipe2.b.l & 0x7].x;			// 3 o 7 ??
						GetMorePipe(_cs,_ip); 
            op3.mem=(int16_t)(int8_t)Pipe2.b.h;
            res1.x=*op2.reg16;
            res2.x=op3.mem;
						res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;
#ifdef EXT_80386
            *op1.reg32=res3.d;		// finire!
#else
            *op1.reg16=res3.x;
#endif
						_f.Carry=_f.Ovf= (((res3.d & 0xffff8000) == 0xffff8000)  ||  
							((res3.d & 0xffff8000) == 0))  ? 0 : 1;
#ifdef EXT_NECV20

#else
//qua no						_f.Zero=0;
#endif
            break;
          }
    		_ip++;      // imm8
				}
				
				break;

			case 0x6c:        // INSB; NO OVERRIDE qua  https://www.felixcloutier.com/x86/ins:insb:insw:insd
        if(inRep) {   // .. ma prefisso viene accettato cmq...
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
				PutValue(_es,_di,InValue(_dx));
        if(_f.Dir)
          _di--;
        else
          _di++;
				break;

			case 0x6d:        // INSW
        if(inRep) {   // .. ma prefisso viene accettato cmq...
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
#ifdef EXT_80386
#endif
				PutShortValue(_es,_di,InShortValue(_dx));
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          }
        else {
          _di+=2;
          }
				break;

			case 0x6e:        // OUTSB
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
				OutValue(_dx,GetValue(_es,_di));
        if(_f.Dir)
          _di--;
        else
          _di++;
				break;

			case 0x6f:        // OUTSW
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
#ifdef EXT_80386
#endif
				OutShortValue(_dx,GetShortValue(_es,_di));
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          }
        else {
          _di+=2;
          }
				break;
#endif			// 186, V20

#ifdef UNDOCUMENTED_8086				// su 8086 questi sono alias per 0x70..7f  (!)
			case 0x60:    // JO
				_ip++;
				if(_f.Ovf)
					goto jmp_short;
				break;

			case 0x61:    // JNO
				_ip++;
				if(!_f.Ovf)
					goto jmp_short;
				break;

			case 0x62:    // JB, JC, JNAE
				_ip++;
				if(_f.Carry)
					goto jmp_short;
				break;

			case 0x63:    // JAE, JNB, JNC
				_ip++;
				if(!_f.Carry)
					goto jmp_short;
				break;

			case 0x64:      // JE, JZ
				_ip++;
				if(_f.Zero)
					goto jmp_short;
				break;

			case 0x65:      // JNE, JNZ
				_ip++;
				if(!_f.Zero)
					goto jmp_short;
				break;

			case 0x66:    // JBE, JNA
				_ip++;
				if(_f.Zero || _f.Carry)
					goto jmp_short;
				break;

			case 0x67:      // JA, JNBE
				_ip++;
				if(!_f.Zero && !_f.Carry)
					goto jmp_short;
				break;

			case 0x68:          // JS
				_ip++;
				if(_f.Sign)
					goto jmp_short;
				break;

			case 0x69:          // JNS
				_ip++;
				if(!_f.Sign)
					goto jmp_short;
				break;

      case 0x6a:          // JP, JPE
				_ip++;
				if(_f.Parity)
					goto jmp_short;
				break;
        
			case 0x6b:          // JNP, JPO
				_ip++;
				if(!_f.Parity)
					goto jmp_short;
				break;

			case 0x6c:          // JL, JNGE
				_ip++;
				if(_f.Sign!=_f.Ovf)
					goto jmp_short;
				break;

			case 0x6d:
				_ip++;
				if(_f.Sign==_f.Ovf)     // JGE, JNL
					goto jmp_short;
				break;

			case 0x6e:      // JLE, JNG
				_ip++;
				if(_f.Zero || _f.Sign!=_f.Ovf)
					goto jmp_short;
				break;

			case 0x6f:      // JG, JNLE
				_ip++;
				if(!_f.Zero && _f.Sign==_f.Ovf)
					goto jmp_short;
				break;
#endif

			case 0x70:    // JO
				_ip++;
				if(_f.Ovf)

jmp_short:
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x71:    // JNO
				_ip++;
				if(!_f.Ovf)
					goto jmp_short;
				break;

			case 0x72:    // JB, JC, JNAE
				_ip++;
				if(_f.Carry)
					goto jmp_short;
				break;

			case 0x73:    // JAE, JNB, JNC
				_ip++;
				if(!_f.Carry)
					goto jmp_short;
				break;

			case 0x74:      // JE, JZ
				_ip++;
				if(_f.Zero)
					goto jmp_short;
				break;

			case 0x75:      // JNE, JNZ
				_ip++;
				if(!_f.Zero)
					goto jmp_short;
				break;

			case 0x76:    // JBE, JNA
				_ip++;
				if(_f.Zero || _f.Carry)
					goto jmp_short;
				break;

			case 0x77:      // JA, JNBE
				_ip++;
				if(!_f.Zero && !_f.Carry)
					goto jmp_short;
				break;

			case 0x78:          // JS
				_ip++;
				if(_f.Sign)
					goto jmp_short;
				break;

			case 0x79:          // JNS
				_ip++;
				if(!_f.Sign)
					goto jmp_short;
				break;

      case 0x7a:          // JP, JPE
				_ip++;
				if(_f.Parity)
					goto jmp_short;
				break;
        
			case 0x7b:          // JNP, JPO
				_ip++;
				if(!_f.Parity)
					goto jmp_short;
				break;

			case 0x7c:          // JL, JNGE
				_ip++;
				if(_f.Sign!=_f.Ovf)
					goto jmp_short;
				break;

			case 0x7d:
				_ip++;
				if(_f.Sign==_f.Ovf)     // JGE, JNL
					goto jmp_short;
				break;

			case 0x7e:      // JLE, JNG
				_ip++;
				if(_f.Zero || _f.Sign!=_f.Ovf)
					goto jmp_short;
				break;

			case 0x7f:      // JG, JNLE
				_ip++;
				if(!_f.Zero && _f.Sign==_f.Ovf)
					goto jmp_short;
				break;

			case 0x80:				// ADD ecc rm8, immediate8
			case 0x81:				// ADD ecc rm16, immediate16
			case 0x82:				// undefined/nop... (ADD ecc rm8, immediate8  con sign-extend   LO USA EDLIN!@#£$%
			case 0x83:				// ADD ecc rm16, immediate8 con sign-extend
			  if(Pipe2.mod<3)
					GetMorePipe(_cs,_ip-1);   // 
				if(!(Pipe1 & 2)) 			// vuol dire che l'operando è 8bit ma esteso a 16
          _ip++;
        
				COMPUTE_RM_OFS
					GET_MEM_OPER            
					if(!(Pipe1 & 1)) {
            res1.b=GetValue(*theDs,op2.mem);
            op1.mem=Pipe2.bd[immofs];
						res2.b=(uint8_t)op1.mem;
						switch(Pipe2.reg) {
			        case 0:       // ADD
								res3.b=res1.b + res2.b;
		            PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagAB;
								break;
							case 1:       // OR
								res3.b=res1.b | res2.b;
		            PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagBZC;
								break;
							case 2:       // ADC
								res3.b=res1.b + res2.b+_f.Carry;
		            PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagABC;
								break;
							case 3:       // SBB
								res3.b=res1.b - res2.b-_f.Carry;
		            PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagSBB;
								break;
							case 4:       // AND
								res3.b=res1.b & res2.b;
		            PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagBZC;
								break;
							case 5:       // SUB
								res3.b=res1.b - res2.b;
		            PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagSB;
								break;
							case 6:       // XOR
								res3.b=res1.b ^ res2.b;
		            PutValue(*theDs,op2.mem,res3.b);
								goto aggFlagBZC;
								break;
							case 7:       // CMP
								res3.b=res1.b - res2.b;
								goto aggFlagSB;
								break;
							}
						}
					else {
            _ip++;
            res1.x=GetShortValue(*theDs,op2.mem);
						if(Pipe1 & 2) 			// sign extend
              op1.mem=(int16_t)(int8_t)Pipe2.bd[immofs];
						else
              op1.mem=MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
            res2.x=op1.mem;
						switch(Pipe2.reg) {
			        case 0:       // ADD
								res3.x = res1.x + res2.x;
		            PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagAW;
								break;
							case 1:       // OR
								res3.x=res1.x | res2.x;
#ifdef EXT_80386
								if(Pipe1 & 2) {			// sign extend
									_dx = _ax & 0x8000 ? 0xffff : 0;
									}
#endif
		            PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagWZC;
								break;
			        case 2:       // ADC
								res3.x = res1.x + res2.x+ _f.Carry;
		            PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagAWC;
								break;
			        case 3:       // SBB
								res3.x = res1.x - res2.x-_f.Carry;
		            PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagSWB;
								break;
			        case 4:       // AND
								res3.x=res1.x & res2.x;
#ifdef EXT_80386
								if(Pipe1 & 2) {			// sign extend
									_dx = _ax & 0x8000 ? 0xffff : 0;
									}
#endif
		            PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagWZC;
								break;
			        case 5:       // SUB
								res3.x = res1.x - res2.x;
		            PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagSW;
								break;
			        case 6:       // XOR
								res3.x=res1.x ^ res2.x;
#ifdef EXT_80386
								if(Pipe1 & 2) {			// sign extend
									_dx = _ax & 0x8000 ? 0xffff : 0;
									}
#endif
		            PutShortValue(*theDs,op2.mem,res3.x);
								goto aggFlagWZC;
								break;
			        case 7:       // CMP
								res3.x = res1.x - res2.x;
								goto aggFlagSW;
								break;
							}
            }
          break;
        case 3:
					GET_REGISTER_8_16_2
					if(!(Pipe1 & 1)) {
            res1.b=*op2.reg8;
						op1.mem=Pipe2.b.h;
						res2.b=(uint8_t)op1.mem;
						switch(Pipe2.reg) {
			        case 0:       // ADD
								res3.b=res1.b + res2.b;
								*op2.reg8=res3.b;
								goto aggFlagAB;
								break;
							case 1:       // OR
								res3.b=res1.b | res2.b;
								*op2.reg8=res3.b;
								goto aggFlagBZC;
								break;
							case 2:       // ADC
								res3.b=res1.b + res2.b+_f.Carry;
								*op2.reg8=res3.b;
								goto aggFlagABC;
								break;
							case 3:       // SBB
								res3.b=res1.b - res2.b-_f.Carry;
								*op2.reg8=res3.b;
								goto aggFlagSBB;
								break;
							case 4:       // AND
								res3.b=res1.b & res2.b;
								*op2.reg8=res3.b;
								goto aggFlagBZC;
								break;
							case 5:       // SUB
								res3.b=res1.b - res2.b;
								*op2.reg8=res3.b;
								goto aggFlagSB;
								break;
							case 6:       // XOR
								res3.b=res1.b ^ res2.b;
								*op2.reg8=res3.b;
								goto aggFlagBZC;
								break;
							case 7:       // CMP
								res3.b=res1.b - res2.b;
								goto aggFlagSB;
								break;
							}
						} 
					else {
      			_ip++;      // imm16...
            res1.x=*op2.reg16;
						if(Pipe1 & 2) 			// sign extend  BOH verificare come va sotto... 2024
							op1.mem = (int16_t)(int8_t)Pipe2.b.h;
						else
  						op1.mem=Pipe2.xm.w;
						res2.x=op1.mem;
						switch(Pipe2.reg) {
			        case 0:       // ADD
								res3.x = res1.x + res2.x;
								*op2.reg16=res3.x;
								goto aggFlagAW;
								break;
							case 1:       // OR
								res3.x=res1.x | res2.x;
								*op2.reg16=res3.x;
#ifdef EXT_80386
								if(Pipe1 & 2) {			// sign extend
									_dx = _ax & 0x8000 ? 0xffff : 0;
									}
#endif
								goto aggFlagWZC;
								break;
			        case 2:       // ADC
								res3.x = res1.x + res2.x+_f.Carry;
								*op2.reg16=res3.x;
								goto aggFlagAWC;
								break;
			        case 3:       // SBB
								res3.x = res1.x - res2.x-_f.Carry;
								*op2.reg16=res3.x;
								goto aggFlagSWB;
								break;
			        case 4:       // AND
								res3.x=res1.x & res2.x;
								*op2.reg16=res3.x;
#ifdef EXT_80386
								if(Pipe1 & 2) {			// sign extend
									_dx = _ax & 0x8000 ? 0xffff : 0;
									}
#endif
								goto aggFlagWZC;
								break;
			        case 5:       // SUB
								res3.x = res1.x - res2.x;
								*op2.reg16=res3.x;
								goto aggFlagSW;
								break;
			        case 6:       // XOR
								res3.x=res1.x ^ res2.x;
								*op2.reg16=res3.x;
#ifdef EXT_80386
								if(Pipe1 & 2) {			// sign extend
									_dx = _ax & 0x8000 ? 0xffff : 0;
									}
#endif
								goto aggFlagWZC;
								break;
			        case 7:       // CMP
								res3.x = res1.x - res2.x;
								goto aggFlagSW;
								break;
							}
						}
          break;
          }
        break;
        
			case 0x84:        // TESTB
			case 0x85:        // TESTW
				COMPUTE_RM
            if(!(Pipe1 & 1)) {
              res1.b=GetValue(*theDs,op2.mem);
              res2.b=*op1.reg8;
              res3.b= res1.b & res2.b;
							goto aggFlagBZC;
							}
						else {
              res1.x=GetShortValue(*theDs,op2.mem);
              res2.x=*op1.reg16;
              res3.x= res1.x & res2.x;
							goto aggFlagWZC;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            if(!(Pipe1 & 1)) {
              res1.b=*op2.reg8;
              res2.b=*op1.reg8;
              res3.b= res1.b & res2.b;
							goto aggFlagBZC;
							}
						else {
              res1.x=*op2.reg16;
              res2.x=*op1.reg16;
              res3.x= res1.x & res2.x;
							goto aggFlagWZC;
              }
            break;
          }
				break;

			case 0x86:				// XCHG
			case 0x87:
				inLock++;
				COMPUTE_RM
            if(!(Pipe1 & 1)) {
              res1.b=GetValue(*theDs,op2.mem);
              PutValue(*theDs,op2.mem,*op1.reg8);
              *op1.reg8=res1.b;
							}
						else {
              res1.x=GetShortValue(*theDs,op2.mem);
              PutShortValue(*theDs,op2.mem,*op1.reg16);
              *op1.reg16=res1.x;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            if(!(Pipe1 & 1)) {
              res1.b=*op2.reg8;
              *op2.reg8=*op1.reg8;
              *op1.reg8=res1.b;
							}
						else {
              res1.x=*op2.reg16;
              *op2.reg16=*op1.reg16;
              *op1.reg16=res1.x;
              }
            break;
          }
				break;
        
			case 0x88:				//MOV8
			case 0x89:				//MOV16
			case 0x8a:
			case 0x8b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
	              PutValue(*theDs,op2.mem,*op1.reg8);
                break;
              case 1:
	              PutShortValue(*theDs,op2.mem,*op1.reg16);
                break;
              case 2:
                *op1.reg8=GetValue(*theDs,op2.mem);
                break;
              case 3:
                *op1.reg16=GetShortValue(*theDs,op2.mem);
                break;
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
            switch(Pipe1 & 0x3) {
              case 0:
	              *op2.reg8=*op1.reg8;
                break;
              case 1:
	              *op2.reg16=*op1.reg16;
                break;
              case 2:
	              *op1.reg8=*op2.reg8;
                break;
              case 3:
	              *op1.reg16=*op2.reg16;
                break;
              }
            break;
          }
				break;

			case 0x8c:        // MOV rm16,SREG
#ifdef EXT_80286
				op1.reg16= &segs.r[Pipe2.reg].x;
#else
				op1.reg16= &segs.r[Pipe2.reg & 0x3].x;
#endif
       
				COMPUTE_RM_OFS
					GET_MEM_OPER
            PutShortValue(*theDs,op2.mem,*op1.reg16);
            break;
          case 3:
    				op2.reg16= &regs.r[Pipe2.rm].x;
            *op2.reg16=*op1.reg16;
            break;
          }
				break;
        
			case 0x8d:        // LEA
        op1.reg16 = &regs.r[Pipe2.reg].x;
				COMPUTE_RM_OFS
					GET_MEM_OPER
    				*op1.reg16 = op2.mem;
            break;
          case 3:
    				op2.reg16= &regs.r[Pipe2.rm].x;
            *op1.reg16=*op2.reg16;
            break;
          }
				break;
        
			case 0x8e:        // MOV SREG,rm16
#ifdef EXT_80286
				op1.reg16= &segs.r[Pipe2.reg].x;
#else
				op1.reg16= &segs.r[Pipe2.reg & 0x3].x;
#endif
       
				COMPUTE_RM_OFS
					GET_MEM_OPER
            *op1.reg16=GetShortValue(*theDs,op2.mem);
            break;
          case 3:
    				op2.reg16= &regs.r[Pipe2.rm].x;
            *op1.reg16=*op2.reg16;
            break;
          }
        if(op1.reg16 == &segs.r[2].x)   // se SS...
          inEI=1;
        break;
        
			case 0x8f:      // POP imm
        // i 3 bit "reg" qua sempre 0! 
				COMPUTE_RM_OFS
					GET_MEM_OPER
						POP_STACK(res3.x);
						PutShortValue(*theDs,op2.mem,res3.x);
            break;
          case 3:			// 
            op2.reg16= &regs.r[Pipe2.rm].x;
						POP_STACK(*op2.reg16);
            break;
          }
#ifdef EXT_NECV20		// dice che fa cose strane
#endif
				break;

			case 0x90:    // NOP, v. anche single byte instructions http://xxeo.com/single-byte-or-small-x86-opcodes
      case 0x91:		// XCHG AX
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
        op2.reg16 = &regs.r[Pipe1 & 0x7].x;
				res3.x=*op2.reg16;
				*op2.reg16=_ax;
        _ax=res3.x;
        break;
        
			case 0x98:		// CBW
        _ah = _al & 0x80 ? 0xff : 0;
        break;
        
			case 0x99:		// CWD
        _dx = _ax & 0x8000 ? 0xffff : 0;
        break;
        
			case 0x9a:      // CALLF
        GetMorePipe(_cs,_ip-1);
        _ip+=4;
				PUSH_STACK(_cs);		// 
				PUSH_STACK(_ip);
				_ip=Pipe2.x.l;
				_cs=Pipe2.x.h;
				break;

   		case 0x9b:
				// WAIT Wait for BUSY pin (usually FPU 
//				while()
//					;
#ifdef EXT_80x87
          status8087,control8087;
            
#endif
				break;
        
   		case 0x9c:        // PUSHF
				PUSH_STACK(_f.x);		// 
				break;
        
   		case 0x9d:        // POPF
				POP_STACK(_f.x);		// 
#ifdef EXT_NECV20	
				inEI++;			// boh, NECV20 emulator lo fa...
#endif
				goto fix_flags;
				break;

			case 0x9e:      // SAHF
				_f.x = (_f.x & 0xff00) | _ah;

fix_flags:
				_f.unused=1; _f.unused2=_f.unused3=0; 
#ifdef EXT_80286
//				_f.unused3_=0; _f.unused4=0;			// beh.. 
#else
				_f.unused3_=7 /*1 o 3=386*/; 		// così viene visto come 8086 o Nec, azzerando unused4 e 3_ 80286 (logico)
#ifdef EXT_NECV20
				_f.MD=1;		// mah direi cmq... dato che siamo in modo 8086/native!
#else
				_f.unused4=1 /*0=386*/;			// così viene visto come 8086 o Nec, azzerando unused4 e 3_ 80286 (logico)
#endif
#endif
				break;
        
			case 0x9f:      // LAHF
				_ah=_f.x & 0x00ff;
				break;

 			case 0xa0:      // MOV al,[ofs]
 			case 0xa1:      // MOV ,al
 			case 0xa2:      // MOV ax,
 			case 0xa3:      // MOV ,ax
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				if(!(Pipe1 & 2)) {
					if(Pipe1 & 1)
						_ax=GetShortValue(*theDs,Pipe2.x.l);
					else
						_al=GetValue(*theDs,Pipe2.x.l);
					}
				else {        
					if(Pipe1 & 1)
						PutShortValue(*theDs,Pipe2.x.l,_ax);
					else
						PutValue(*theDs,Pipe2.x.l,_al);
					}
        _ip+=2;
				break;
        
			case 0xa4:      // MOVSB
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
	        if(!inRep)
	          segOverride=0;
          }
        else
          theDs=&_ds;	
        PutValue(_es,_di,GetValue(*theDs,_si));
        if(_f.Dir) {
          _di--;
          _si--;
          }
        else {
          _di++;
          _si++;
          }
				break;

			case 0xa5:      // MOVSW
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
	        if(!inRep)
	          segOverride=0;
          }
        else
          theDs=&_ds;
				PutShortValue(_es,_di,GetShortValue(*theDs,_si));
#ifdef EXT_80386
#endif
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          _si-=2;
          }
        else {
          _di+=2;
          _si+=2;
          }
				break;
        
			case 0xa6:      // CMPSB
        if(inRep) {
//          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
	        if(!inRep)
	          segOverride=0;
          }
        else
          theDs=&_ds;
				res1.b= GetValue(*theDs,_si);
				res2.b= GetValue(_es,_di);
				res3.b= res1.b-res2.b;   // https://pdos.csail.mit.edu/6.828/2008/readings/i386/CMPS.htm
        if(_f.Dir) {
          _di--;
          _si--;
          }
        else {
          _di++;
          _si++;
          }
        goto aggFlagSB;
				break;

			case 0xa7:      // CMPSW
        if(inRep) {
//          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
	        if(!inRep)
	          segOverride=0;
          }
        else
          theDs=&_ds;
#ifdef EXT_80386
#endif
				res1.x= GetShortValue(*theDs,_si);
				res2.x= GetShortValue(_es,_di);
				res3.x= res1.x-res2.x;
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          _si-=2;
          }
        else {
          _di+=2;
          _si+=2;
          }
        goto aggFlagSW;
				break;

			case 0xa8:        // TESTB
				res3.b = _al & Pipe2.b.l; 
        _ip++;
        goto aggFlagBZC;
				break;

			case 0xa9:        // TESTW
				res3.x = _ax & Pipe2.x.l;
        _ip+=2;
        goto aggFlagWZC;
				break;
        
			case 0xaa:      // STOSB
        // no override! da pdf 286
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;		// ma credo accetti cmq il prefisso, v.LODSB
          }
				PutValue(_es,_di,_al);
        if(_f.Dir)
          _di--;
        else
          _di++;
				break;

			case 0xab:      // STOSW
        // idem
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;		// ma credo accetti cmq il prefisso, v.LODSB
          }
#ifdef EXT_80386
#endif
				PutShortValue(_es,_di,_ax);
        if(_f.Dir)
          _di-=2;   // anche 32bit??
        else
          _di+=2;
				break;

			case 0xac:      // LODSB; [was NO OVERRIDE qua! ... v. bug 8088
        if(inRep) {     // (questo ha poco senso qua :)
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
	        if(!inRep)
	          segOverride=0;
          }
        else
          theDs=&_ds;
				_al=GetValue(*theDs,_si);
        if(_f.Dir)
          _si--;
        else
          _si++;
				break;

			case 0xad:      // LODSW
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
	        if(!inRep)
	          segOverride=0;
          }
        else
          theDs=&_ds;
#ifdef EXT_80386
#endif
				_ax=GetShortValue(*theDs,_si);
        if(_f.Dir)
          _si-=2;   // anche 32bit??
        else
          _si+=2;
				break;

			case 0xae:      // SCASB; NO OVERRIDE qua!  http://www.nacad.ufrj.br/online/intel/vtune/users_guide/mergedProjects/analyzer_ec/mergedProjects/reference_olh/mergedProjects/instructions/instruct32_hh/vc285.htm
        // però uno può infilarci il prefisso cmq, quindi v. inRep
        if(inRep) {
//          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
				res1.b= _al;
        res2.b= GetValue(_es,_di);
				res3.b= res1.b - res2.b;
        if(_f.Dir)
          _di--;
        else
          _di++;
        goto aggFlagSB;
				break;

			case 0xaf:      // SCASW
        if(inRep) {
//          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
				res1.x= _ax;
        res2.x= GetShortValue(_es,_di);
				res3.x= res1.x - res2.x;
#ifdef EXT_80386
#endif
        if(_f.Dir)
          _di-=2;   // anche 32bit??
        else
          _di+=2;
        goto aggFlagSW;
				break;

      case 0xb0:      // altre MOVB
      case 0xb1:
      case 0xb2:
      case 0xb3:
      case 0xb4:
      case 0xb5:
      case 0xb6:
      case 0xb7:
        if(Pipe1 & 0x4)
          regs.r[Pipe1 & 0x3].b.h=Pipe2.b.l;
        else
          regs.r[Pipe1 & 0x3].b.l=Pipe2.b.l;
        _ip++;
				break;
        
      case 0xb8:      // altre MOVW
      case 0xb9:
      case 0xba:
      case 0xbb:
      case 0xbc:
      case 0xbd:
      case 0xbe:
      case 0xbf:
        regs.r[Pipe1 & 0x7].x=Pipe2.x.l;
        _ip+=2;
				break;
        
#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0xc0:      // RCL, RCR, SHL ecc
			  if(Pipe2.mod<3)
					GetMorePipe(_cs,_ip-1);   // 
				COMPUTE_RM_OFS
					GET_MEM_OPER
            op1.mem=Pipe2.bd[immofs];
						i=res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
						res2.b &= 31;		// non va dentro la macro...
#endif
#endif
						res3.b=res1.b=GetValue(*theDs,op2.mem);
						i=0;
						ROTATE_SHIFT8
						PutValue(*theDs,op2.mem,res3.b);
            break;
          case 3:
            op2.reg8= Pipe2.rm & 0x4 ? &regs.r[Pipe2.rm & 0x3].b.h : &regs.r[Pipe2.rm & 0x3].b.l;
						op1.mem=Pipe2.b.h;
						i=res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
						res2.b &= 31;		// non va dentro la macro...
#endif
#endif
    				_ip++;      // imm8
						res3.b=res1.b=*op2.reg8;
						i=0;
						ROTATE_SHIFT8
						*op2.reg8=res3.b;
						break;
					}
				if(Pipe2.reg>=4)		// solo SAL/SHL/SHR/SAR
					goto aggFlagBSZP;
				break;

			case 0xc1:
			  if(Pipe2.mod<3)
					GetMorePipe(_cs,_ip-1);   // 
				COMPUTE_RM_OFS
					GET_MEM_OPER
            op1.mem=Pipe2.bd[immofs];
						i=res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
						res2.b &= 31;		// non va dentro la macro...
#endif
#endif
						res3.x=res1.x=GetShortValue(*theDs,op2.mem);
						i=0;
						ROTATE_SHIFT16
						PutShortValue(*theDs,op2.mem,res3.x);
            break;
          case 3:
            op2.reg16= &regs.r[Pipe2.b.l & 0x7].x;
						op1.mem=Pipe2.b.h;
						i=res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
						res2.b &= 31;		// non va dentro la macro...
#endif
#endif
    				_ip++;      // imm8
						res3.x=res1.x=*op2.reg16;
						i=0;
						ROTATE_SHIFT16
						*op2.reg16=res3.x;
						break;
					}
				if(Pipe2.reg>=4)		// solo SAL/SHL/SHR/SAR
					goto aggFlagWSZP;
				break;
#endif
        
#if !defined(EXT_80186) && !defined(EXT_NECV20)
			case 0xc0:
#endif

			case 0xc2:      // RETN
				POP_STACK(_ip);
        _sp+=Pipe2.x.l;
				break;
        
#if !defined(EXT_80186) && !defined(EXT_NECV20)
			case 0xc1:
#endif

			case 0xc3:      // RET
				POP_STACK(_ip);
				break;

			case 0xc4:				//LES
			case 0xc5:				//LDS
				op1.reg16= &regs.r[Pipe2.reg].x;
				COMPUTE_RM_OFS
					GET_MEM_OPER
						*op1.reg16=GetShortValue(*theDs,op2.mem);
						op2.mem+=2;
						if(Pipe1 & 1)
							_ds=GetShortValue(*theDs,op2.mem);
						else
							_es=GetShortValue(*theDs,op2.mem);
            break;
          case 3:		// 
						goto unknown_istr;
            break;
          }
				break;

 			case 0xc6:				//MOV imm8...
 			case 0xc7:				//MOV imm16
			  if(Pipe2.mod<3)
					GetMorePipe(_cs,_ip-1);   // 
        // i 3 bit "reg" qua sempre 0! 
				COMPUTE_RM_OFS
					GET_MEM_OPER
					  if(!(Pipe1 & 1)) {
              _ip++;
              op1.mem=Pipe2.bd[immofs];
              PutValue(*theDs,op2.mem,(uint8_t)op1.mem);
							}
						else {
              _ip+=2;
              op1.mem=MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
              PutShortValue(*theDs,op2.mem,op1.mem);
							}
            break;
          case 3:		// 
						GET_REGISTER_8_16_2
            if(!(Pipe1 & 1)) {
							_ip++;
              op1.mem=Pipe2.b.h;
              *op2.reg8=(uint8_t)op1.mem;
							}
						else {
							_ip+=2;
              op1.mem=Pipe2.xm.w;
              *op2.reg16=op1.mem;
              }
            break;
          }
				break;


#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0xc8:        //ENTER
				PUSH_STACK(_bp);
        res3.x=_sp;
        switch(Pipe2.b.u) {
          case 0:
            break;
          case 1:
            PUSH_STACK(_bp);
            break;
          default:
            for(i=0; i<Pipe2.b.u; i++) {		// max nesting 32...
              _bp-=2;
              PUSH_STACK(_bp);
              }
            break;
          }
				PUSH_STACK(res3.x);
				_bp=res3.x;
				_sp=_bp-Pipe2.b.u;
        _ip+=3;
        break;
#endif
        
#ifdef EXT_80186
			case 0xc9:        // LEAVE
        _sp=_bp;
				POP_STACK(_bp);
        break;
#endif
        
#if !defined(EXT_80186) && !defined(EXT_NECV20)     // bah, così dicono..
			case 0xc8:
#endif
			case 0xca:      // RETF
				POP_STACK(_ip);
				POP_STACK(_cs);
        _sp+=Pipe2.x.l;
        break;
        
#ifndef EXT_80186     // bah, così dicono..
			case 0xc9:
#endif
			case 0xcb:
Return32:
				POP_STACK(_ip);
				POP_STACK(_cs);
				break;

			case 0xcc:
				i=3 /*INT3*/ ;
				goto do_irq;
				break;

			case 0xcd:

#ifdef EXT_80286
    /*IF ((vector_number « 2) + 3) is not within IDT limit
        THEN #GP; FI; 
    IF stack not large enough for a 6-byte return information
        THEN #SS; FI;*/
#endif
        i=Pipe2.b.l;

				_ip++;


/*				if(i==0x21) {
					char myBuf[128];
			wsprintf(myBuf,"%04x:%04x INT %02X-> %04x %04x %04x %04x %04x:%04x %04x:%04x %04x:%04x  %02X %02X\n",_cs,_ip,i,
				_ax,_bx,_cx,_dx,_ss,_sp,
				_ds,_si,_es,_di, GetPipe(_cs,_ip)) , GetPipe(_cs,_ip+1)));
			_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/


do_irq:
        _f.Trap=0;// ????	_f.Aux=0;
        if(i < 0x100) {
#ifdef EXT_80286
          if(_sp>=6) {
#endif
            PUSH_STACK(_f.x);
            PUSH_STACK(_cs);
    /*- all interrupts except the internal CPU exceptions push the
	  flags and the CS:IP of the next instruction onto the stack.
	  CPU exception interrupts are similar but push the CS:IP of the
	  causal instruction.	8086/88 divide exceptions are different,
	  they return to the instruction following the division*/
            PUSH_STACK(_ip);
            _f.IF=0;
            _ip=GetShortValue(0,i *4);
            _cs=GetShortValue(0,(i *4) +2);
#ifdef EXT_80286
            }
          else {
// tecnicamente è un eccezione, non trap...            _f.Trap=1;
            }
#endif
          }
        else {
//          _f.Trap=1;
          }
				break;

			case 0xce:
				if(_f.Ovf) {
					i=4 /*INTO*/ ;
					goto do_irq;
					}
				break;

			case 0xcf:        // IRET
          if(_sp>=6) {		// fare controllo
            }
          else {
// tecnicamente è un eccezione, non trap...            _f.Trap=1;
            }
					;
				POP_STACK(_ip);
				POP_STACK(_cs);
				POP_STACK(_f.x);
#ifdef EXT_NECV20	
				inEI++;			// boh, NECV20 emulator lo fa...
#endif
#ifdef UNDOCUMENTED_8086
				goto fix_flags;			// bah non dovrebbe servire ma gloriouscow
#endif
				break;

			case 0xd0:      // RCL, RCR ecc 8
				op1.mem=1;
				i=1;
				goto do_rcl8;
			case 0xd2:
				op1.mem=_cl;
				i=0;

do_rcl8:
				res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
				res2.b &= 31;		// non va dentro la macro...
#endif
#endif
				COMPUTE_RM_OFS
					GET_MEM_OPER
						res1.b=res3.b=GetValue(*theDs,op2.mem);
						ROTATE_SHIFT8
#ifdef UNDOCUMENTED_8086			// andrebbe anche messo nella macro ROTATE ma non si può...
						switch(Pipe2.reg) {
							case 6:
								if(i || _cl) {			// SETMO SETMOC https://www.os2museum.com/wp/undocumented-8086-opcodes-part-i/
																		// https://www.reenigne.org/blog/8086-microcode-disassembled/
									res3.b=0xff;
									_f.Carry=_f.Ovf=_f.Zero=_f.Aux=0;
									_f.Sign=_f.Parity=1;
									}
								break;
							case 4:
								_f.Aux=res3.b & 0x10 ? 1 : 0;		// gloriouscow, granite
								break;
							case 5:
							case 7:
								_f.Aux=0;												// gloriouscow, granite
								break;
							}
#endif
						PutValue(*theDs,op2.mem,res3.b);
            break;
          case 3:
            op2.reg8= Pipe2.rm & 0x4 ? &regs.r[Pipe2.rm & 0x3].b.h : &regs.r[Pipe2.rm & 0x3].b.l;
						res1.b=res3.b=*op2.reg8;
						ROTATE_SHIFT8 
#ifdef UNDOCUMENTED_8086
						switch(Pipe2.reg) {
							case 6:
								if(i || _cl) {			// SETMO SETMOC https://www.os2museum.com/wp/undocumented-8086-opcodes-part-i/
																		// https://www.reenigne.org/blog/8086-microcode-disassembled/
									res3.b=0xff;
									_f.Carry=_f.Ovf=_f.Zero=_f.Aux=0;
									_f.Sign=_f.Parity=1;
									}
								break;
							case 4:
								_f.Aux=res3.b & 0x10 ? 1 : 0;
								break;
							case 5:
							case 7:
								_f.Aux=0;
								break;
							}
#endif
						*op2.reg8=res3.b;
						break;
					}
				if(Pipe2.reg>=4 )		// solo SAL/SHL/SHR/SAR
					goto aggFlagBSZP;
				break;

			case 0xd1:      // RCL, RCR ecc 16
				op1.mem=1;
				i=1;
				goto do_rcl16;
			case 0xd3:
				op1.mem=_cl;
				i=0;

do_rcl16:
				res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
				res2.b &= 31;		// non va dentro la macro...
#endif
#endif
				COMPUTE_RM_OFS
					GET_MEM_OPER
						res1.x=res3.x=GetShortValue(*theDs,op2.mem);
						ROTATE_SHIFT16
#ifdef UNDOCUMENTED_8086
						switch(Pipe2.reg) {
							case 6:
								if(i || _cl) {			// SETMO SETMOC https://www.os2museum.com/wp/undocumented-8086-opcodes-part-i/
																		// https://www.reenigne.org/blog/8086-microcode-disassembled/
									res3.x=0xffff;
									_f.Carry=_f.Ovf=_f.Zero=_f.Aux=0;
									_f.Sign=_f.Parity=1;
									}
								break;
							case 4:
								_f.Aux=res3.b & 0x10 ? 1 : 0;
								break;
							case 5:
							case 7:
								_f.Aux=0;
								break;
							}
#endif
						PutShortValue(*theDs,op2.mem,res3.x);
            break;
          case 3:
            op2.reg16= &regs.r[Pipe2.rm & 0x7].x;
						res1.x=res3.x=*op2.reg16;
						ROTATE_SHIFT16
#ifdef UNDOCUMENTED_8086
						switch(Pipe2.reg) {
							case 6:
								if(i || _cl) {			// SETMO SETMOC https://www.os2museum.com/wp/undocumented-8086-opcodes-part-i/
																		// https://www.reenigne.org/blog/8086-microcode-disassembled/
									res3.x=0xffff;
									_f.Carry=_f.Ovf=_f.Zero=_f.Aux=0;
									_f.Sign=_f.Parity=1;
									}
								break;
							case 4:
								_f.Aux=res3.b & 0x10 ? 1 : 0;
								break;
							case 5:
							case 7:
								_f.Aux=0;
								break;
							}
#endif
						*op2.reg16=res3.x;
						break;
					}
				if(Pipe2.reg>=4 )		// solo SAL/SHL/SHR/SAR
					goto aggFlagWSZP;
				break;

			case 0xd4:      // AAM
        _ip++;
				if(Pipe2.b.l) {
					_ah=_al / Pipe2.b.l;    // 10 fisso in teoria, ma si può usare genericamente come base per la conversione...
					_al=_al % Pipe2.b.l;    //
					}
				else {
#if defined(EXT_NECV20)
					_ah=0xff;    // dice
#elif defined(EXT_80186)			// https://forum.vcfed.org/index.php?threads/exploring-the-nec-v20.1248301/   bah...
					;
#else
#ifdef UNDOCUMENTED_8086
/*          u8 imm = read_inst<u8>();
          if (imm) {
              u8 tempAL = (registers[AX]&0xFF);
              u8 tempAH = tempAL/imm;
              tempAL = tempAL%imm;
              registers[AX] = (tempAH<<8)|tempAL;

              set_flag(F_SIGN, tempAL&0x80);
              set_flag(F_ZERO, tempAL==0);
              set_flag(F_PARITY, byte_parity[tempAL]);
              set_flag(F_OVERFLOW,false);
              set_flag(F_AUX_CARRY,false);
              set_flag(F_CARRY,false);
          }
          else {
              set_flag(F_SIGN, false);
              set_flag(F_ZERO, true);
              set_flag(F_PARITY, true);
              set_flag(F_OVERFLOW,false);
              set_flag(F_AUX_CARRY,false);
              set_flag(F_CARRY,false);
              interrupt(0, true);
          }*/
					_f.Sign=_f.Aux=_f.Carry=_f.Ovf=0;
					_f.Parity=_f.Zero=1;
#endif
					goto divide0;
#endif
					}
        res3.b=_al;
#ifdef UNDOCUMENTED_8086
				_f.Aux=_f.Carry=_f.Ovf=0;
#endif
        goto aggFlagBSZP;			// 
				break;
        
			case 0xd5:      // AAD
#ifdef EXT_NECV20
				Pipe2.b.l=10; // dice...
#endif
				res1.b=_al;
				res2.b=Pipe2.b.l;
        res3.b=res1.b+(_ah * res2.b);    // v. AAM
#ifdef UNDOCUMENTED_8086
	/*          u8 imm = read_inst<u8>();
            u16 orig16 = registers[AX];
            u16 temp16 = (registers[AX]&0xFF) + (registers[AX]>>8)*imm;
            registers[AX] = (temp16&0xFF);

            set_flag(F_SIGN,temp16&0x80);
            set_flag(F_PARITY, byte_parity[temp16&0xFF]);

            u8 a = orig16;
            u8 b = (orig16>>8)*imm;
            u8 result = a+b;

            set_flag(F_ZERO, result==0);

            set_flag(F_CARRY,result < a); //this is now correct

            bool of = ((a ^ result) & (b ^ result)) & 0x80;
            bool af = ((a ^ b ^ result) & 0x10);
            set_flag(F_OVERFLOW,of);
            set_flag(F_AUX_CARRY,af);
*/
				_f.Aux=AUX_ADD_8();
				_f.Ovf=OVF_ADD_8();			// pare... da gloriouscow...
				_f.Carry=CARRY_ADD_8();
#endif
				_al=res3.b;
        _ah=0;
        _ip++;
        goto aggFlagBSZP;			// 
				break;
        
			case 0xd6:      // dice che non è documentata (su 8088)...
#ifndef EXT_NECV20
				_al=_f.Carry ? 0xff : 0;
				break;
#endif								// altrimenti è come D7!

			case 0xd7:      // XLAT
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
        _al=GetValue(*theDs,_bx+_al);
        break;
        
#ifdef EXT_80x87      // https://www.ic.unicamp.br/~celio/mc404/opcodes.html#Co
			case 0xd8:
			case 0xd9:
			case 0xda:
			case 0xdb:
			case 0xdc:
			case 0xdd:
			case 0xde:
			case 0xdf:
				i=2;
				switch(Pipe1 & 7) {
					case 0:
							status8087,control8087;
						break;
					case 1: 
						switch(Pipe2.b.l) {
							case 0xc0:    // FLD st(0)

								break;
							case 0xd0:    // FNOP
								break;
							case 0xE0:    // FCHS
// complement sign of st(0)
								break;
							case 0xE1:    // FABS
								break;
							case 0xE5:    // FXAM
								break;
							case 0xEE:    // FLDZ

//Push +0.0 onto the FPU register stack.                

  dataPtr8087;
	R8087[8];
								break;
							case 0xE8:    // FLD1

//								Push +1.0 onto the FPU register stack.
  dataPtr8087;
	R8087[8];
								break;
							case 0xE9:    // FLDL2T
								break;
							case 0xEB:    // FLDPI
								break;
							case 0xEC:    // FLDLG2
								break;
							case 0xED:    // FLDLN2
								break;
							case 0xF0:    // F2XM1
								break;
							case 0xF1:    // FYL2X
								break;
							case 0xF2:    // FPTAN
								break;
							case 0xF3:    // FPATAN
								break;
							case 0xF4:    // FXTRACT
								break;
							case 0xF6:    // FDECSTP
								break;
							case 0xF7:    // FINCSTP
								break;
							case 0xF8:    // FPREM
								break;
							case 0xF9:    // FYL2XP1
								break;
							case 0xFA:    // FSQRT
								break;
							case 0xFC:    // FRNDINT
								break;
							case 0xFD:    // FSCALE
								break;
							default:      // FNSTCW
		//(#warning NO! finire con tutti indirizzamenti...
								res3.x=control8087;
//								PutShortValue(*theDs,op2.mem,res3.x);
//								_si=control8087;

								break;
							}
							status8087,control8087;
						break;
					case 2:
							status8087,control8087;
						break;
					case 3:
						switch(Pipe2.b.l) {
							case 0xe0:      // FENI
								break;
							case 0xe1:      // FDISI

// disabilita IRQ da 8087...
								break;
							case 0xe2:      // FCLEX
status8087 &= 0x7f00;

								break;
							case 0xe3:      // FNINIT
								i=0;
status8087=0; 
control8087=0x033f /*0x103f*/;
res3.x=0;			// boh
  dataPtr8087=0; instrPtr8087=0;

								break;
							}
							status8087,control8087;
						break;
					case 4:
							status8087,control8087;
						break;
					case 5:
						// FSTSW
						_ax=status8087;
res3.d=MAKELONG(_ax,0);			// boh, finire
i=4;
//								PutShortValue(*theDs,op2.mem,res3.x);
						break;
					case 6:
						switch(Pipe2.b.l) {
							case 0xF9:    // Divide ST(1) by ST(0), store result in ST(1), and pop the register stack.
								break;
							case 0xD9:    // FCOMPP
								break;
							default:
								break;
							}
						// FADD FDIV ecc...
						//DE F9 Divide ST(1) by ST(0), store result in ST(1), and pop the register stack.
						// DE D9 FCOMPP
							status8087,control8087;
						break;
					case 7:
						switch(Pipe2.b.l) {
							case 0xe0:      // FNSTSW AX
								_ax=status8087;
res3.x=_ax;
//								PutShortValue(*theDs,op2.mem,res3.x);
								break;
							}
						status8087,control8087;
						break;
					}

				COMPUTE_RM			// (per non replicare la macro 8 volte...
						switch(i) {
							case 2:
								PutShortValue(*theDs,op2.mem,res3.x);
								break;
							case 4:
								PutShortValue(*theDs,op2.mem,LOWORD(res3.x));
// si schianta!!! boh...								PutShortValue(*theDs,op2.mem+2,HIWORD(res3.x));
								break;
							default:
								break;
							}
						break;
          case 3:
						GET_REGISTER_8_16_2
						switch(i) {
							case 2:
		            *op2.reg16=res3.x;
								break;
							case 4:
								// ??
								break;
							default:
								break;
							}
						break;
					}
        break;
          
#else            
			case 0xd8:
			case 0xd9:
			case 0xda:
			case 0xdb:
			case 0xdc:
			case 0xdd:
			case 0xde:
			case 0xdf:
        // coprocessore matematico
/*        switch(Pipe2.mod) {      // faccio "nulla" :)   https://stackoverflow.com/questions/42543905/what-are-8086-esc-instruction-opcodes
					case 0:
						_ip+=1;
						break;
					case 1:
					case 3:
						_ip+=2;
						break;
					case 2:
						_ip+=3;
						break;
					}
					*/
				COMPUTE_RM 
					}


#ifdef EXT_80286
      // causare INT 07
#endif

#ifdef EXT_80186		// boh
				i=7;			// sarebbe 6 qua dicono   i=6; NON SI CAPISCE
//				goto do_irq;
#else
				i=7;
//				goto do_irq;
#endif
#endif


        break;
        
			case 0xe0:      // LOOPNZ/LOOPNE
				_ip++;
        if(--_cx && !_f.Zero)
 					goto jmp_short;
				break;
        
			case 0xe1:      // LOOPZ/LOOPE
				_ip++;
        if(--_cx && _f.Zero)
          goto jmp_short;
				break;
        
			case 0xe2:      // LOOP
				_ip++;
        if(--_cx)
          goto jmp_short;
				break;

			case 0xe3:      // JCXZ
				_ip++;
        if(!_cx)
          goto jmp_short;
				break;

			case 0xe4:        // INB
				_al=InValue(Pipe2.b.l);
				_ip++;
				break;

			case 0xe5:        // INW
				_ax=InShortValue(Pipe2.b.l);
				_ip++;
				break;

			case 0xe6:      // OUTB
				OutValue(Pipe2.b.l,_al);
				_ip++;
				break;

			case 0xe7:      // OUTW
				OutShortValue(Pipe2.b.l,_ax);
				_ip++;
				break;

			case 0xe8:      // CALL rel16
        _ip+=2;
				PUSH_STACK(_ip);
				_ip+=(int16_t)Pipe2.x.l;
				break;

			case 0xe9:      // JMP rel16 (32 su 386)
				_ip+=(int16_t)Pipe2.x.l+2;
				break;
        
			case 0xea:      // JMP abs  (32 su 386)
        GetMorePipe(_cs,_ip-1);
				_ip=Pipe2.x.l;
				_cs=Pipe2.x.h;
				break;

			case 0xeb:      // JMP rel8
				_ip++;
				goto jmp_short;
				break;
			
			case 0xec:        // INB
				_al=InValue(_dx);
				break;

			case 0xed:        // INW
				_ax=InShortValue(_dx);

#ifdef EXT_NECV20
				// dice che ED FD è RETEM, ritorno da modo 8080...
//			_f.MD=1;
				// e ED ED CALLN
#endif
				break;

			case 0xee:        // OUTB
				OutValue(_dx,_al);
				break;

			case 0xef:        // OUTW
				OutShortValue(_dx,_ax);
				break;

			case 0xf0:				// LOCK (prefisso...
				inLock=2;
				break;

			case 0xf1:
#ifdef EXT_NECV20
#endif
Trap:
				i=1 /*INT1*/ ;
				goto do_irq;
				break;

			case 0xf2:				//REPNZ/REPNE; entrambi vengono mutati in 3 nelle istruzioni che non usano Z
        if(_cx) {			// 0 salta tutto!
					inRep=0x12;
					inEI++;		// forse ne fa uno di troppo alla fine, ma ok...
					}
				else {
/*			    Pipe1=GetPipe(_cs,_ip++));
					if(Pipe1==0x26 || Pipe1==0x2e || Pipe1==0x36 || Pipe1==0x3e		// salto anche ev. segment override messo DOPO (di solito andrebbe prima...
					ma perché???
#if defined(EXT_80386)
						|| Pipe1==0x64 || Pipe1==0x65
#endif
						)*/
						_ip++;
					}
				break;

			case 0xf3:				//REPZ/REPE
        if(_cx) {			// 0 salta tutto!
					inRep=0x11;
					inEI++;
					}
				else {
/*			    Pipe1=GetPipe(_cs,_ip++));
					if(Pipe1==0x26 || Pipe1==0x2e || Pipe1==0x36 || Pipe1==0x3e		// salto anche ev. segment override messo DOPO (di solito andrebbe prima...
					ma perché???
#if defined(EXT_80386)
						|| Pipe1==0x64 || Pipe1==0x65
#endif
						)*/
						_ip++;
					}
				break;

			case 0xf4:				// HLT
			  CPUPins |= DoHalt;
				break;

			case 0xf5:				// CMC
				_f.Carry = !_f.Carry;
				break;

			case 0xf6:        // altre MUL ecc //	; mul on V20 does not affect the zero flag,   but on an 8088 the zero flag is used
			case 0xf7:
			  if(Pipe2.mod<3)
					GetMorePipe(_cs,_ip-1);		// 
				COMPUTE_RM_OFS
					GET_MEM_OPER
						if(!(Pipe1 & 1)) {
              res2.b=GetValue(*theDs,op2.mem);
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
                  _ip++;
        					res1.b = Pipe2.bd[immofs];
									res3.b = res1.b & res2.b;
									goto aggFlagBZC;
									break;
								case 2:			// NOT
									res3.b = ~res2.b;
									PutValue(*theDs,op2.mem,res3.b);
									break;
								case 3:			// NEG
									res1.b = 0;
									res3.b = res1.b-res2.b;
									PutValue(*theDs,op2.mem,res3.b);
									goto aggFlagSB;
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									break;
								case 4:       // MUL8
                  op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = ((uint16_t)res1.b)*res2.b;
									_ax=res3.x;			// non è bello ma...		
									_f.Carry=_f.Ovf= !!_ah;
#ifdef EXT_NECV20
//									_f.Zero=ZERO_16();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.b=_ah;
									_f.Sign=SIGN_8();
									goto aggParity;
#endif
									break;
								case 5:       // IMUL8
                  op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = ((int16_t)(int8_t)res1.b)*(int8_t)res2.b;
									_ax=res3.x;			// (non è bello ma...		
									_f.Carry=_f.Ovf= (((res3.x & 0xff80) == 0xff80)  ||  
										((res3.x & 0xff80) == 0))  ? 0 : 1;
#ifdef EXT_NECV20
//									_f.Zero=ZERO_16();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.b=_ah;
									_f.Sign=SIGN_8();
									goto aggParity;
#endif
									break;
								case 6:       // DIV8
									if(res2.b) {
										res3.x = (uint16_t)_ax / res2.b;
										if(res3.x > 0xFF) {
#ifdef UNDOCUMENTED_8086
											_f.Parity=0;
											_f.Sign=!!(_ah & 0x80);
#endif
											goto divide0; //divide error
											}
#ifdef UNDOCUMENTED_8086
										_ah = _ax % res2.b;			// non è bello ma...
										_al = res3.b;
										_f.Parity=0;
										_f.Sign=!!(_ah & 0x80);
#endif
										}
									else {
divide0:
										i=0;
										goto do_irq;
										}
									break;
								case 7:       // IDIV8
									if(res2.b) {
										res3.x = ((int16_t)_ax) / (int8_t)res2.b;
										_ah =  ((int16_t)_ax) % (int8_t)res2.b;
										_al = res3.b;
#ifdef UNDOCUMENTED_8086
										if(inRep)			// undocumented! e PRIMA del controllo, anche se non so se va pure su res3.x
											_al=-_al;
#endif
										if(((int16_t)res3.x) > 127 || ((int16_t)res3.x) < -128) 
											goto divide0; //divide error
#ifdef UNDOCUMENTED_8086
										_f.Parity=0;
#endif
										}
									else
										goto divide0;
									break;
								}
							}
						else {
              res2.x=GetShortValue(*theDs,op2.mem);
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
                  _ip+=2;
        					res1.x = MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
									res3.x = res1.x & res2.x;
									goto aggFlagWZC;
									break;
								case 2:			// NOT
									res3.x = ~res2.x;
									PutShortValue(*theDs,op2.mem,res3.x);
									break;
								case 3:			// NEG
									res1.x=0;
									res3.x = res1.x-res2.x;			// 
									PutShortValue(*theDs,op2.mem,res3.x);
									goto aggFlagSW;
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									break;
								case 4:       // MUL16
                  op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = ((uint32_t)res1.x)*res2.x;
									_ax = LOWORD(res3.d);			// 
									_dx=HIWORD(res3.d);			// non è bello ma...	???perché?
									_f.Carry=_f.Ovf= !!_dx;
#ifdef EXT_NECV20
//									_f.Zero=ZERO_32();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.x=_dx;
									_f.Sign=SIGN_16();
									goto aggParity;
#endif
									break;
								case 5:       // IMUL16
                  op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = ((int32_t)(int16_t)res1.x)*(int16_t)res2.x;
									_ax=LOWORD(res3.d);			// 
									_dx=HIWORD(res3.d);			// non è bello ma...	???perché?
									_f.Carry=_f.Ovf= (((res3.d & 0xffff8000) == 0xffff8000)  ||  
										((res3.d & 0xffff8000) == 0))  ? 0 : 1;

// boh		on 8088 at least, SZP flags are calculated with the value of AH for 8-bit MUL and DX for 16-bit MUL
#ifdef EXT_NECV20
//									_f.Zero=ZERO_32();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.x=_dx;
									_f.Sign=SIGN_16();
									goto aggParity;
#endif
									break;
								case 6:       // DIV16
									if(res2.x) {
										res3.d = ((uint32_t)MAKELONG(_ax,_dx)) / res2.x;
										_dx = ((uint32_t)MAKELONG(_ax,_dx)) % res2.x;			// non è bello ma...
			              _ax = res3.x;
										if(res3.d > 0xFFFF) 
											goto divide0; //divide error
										}
									else
										goto divide0;
									break;
								case 7:       // IDIV16
									if(res2.x) {
										res3.d = ((int32_t)MAKELONG(_ax,_dx)) / (int16_t)res2.x;
										_dx = ((int32_t)MAKELONG(_ax,_dx)) % (int16_t)res2.x;
			              _ax = res3.x;
#ifdef UNDOCUMENTED_8086
										if(inRep)			// undocumented! e PRIMA del controllo, anche se non so se va pure su res3.d
											_ax=-_ax;
										_f.Parity=0;
#endif
										if(((int32_t)res3.d) > 32767 || ((int32_t)res3.d) < -32768)
											goto divide0; //divide error
											}
									else
										goto divide0;
									break;
								}
              }
            break;
          case 3:
						GET_REGISTER_8_16_2
						if(!(Pipe1 & 1)) {
              res2.b=*op2.reg8;
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
                  _ip++;
        					op1.mem = Pipe2.b.h;
                  res1.b=(uint8_t)op1.mem;
									res3.b = res1.b & res2.b;
									goto aggFlagBZC;
									break;
								case 2:			// NOT
                  op1.reg8=op2.reg8;
									res3.b = ~res2.b;
									*op1.reg8 = res3.b;
									break;
								case 3:			// NEG
                  op1.reg8=op2.reg8;
									res1.b=0;
									res3.b = res1.b-res2.b;
									*op1.reg8 = res3.b;
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									goto aggFlagSB;
									break;
								case 4:       // MUL8
        					op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = ((uint16_t)res1.b)*res2.b;
									_ax=res3.x;			// 
									_f.Carry=_f.Ovf= !!_ah;
#ifdef EXT_NECV20
//									_f.Zero=ZERO_16();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.b=_ah;
									_f.Sign=SIGN_8();
									goto aggParity;
#endif
									break;
								case 5:       // IMUL8
        					op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = ((int16_t)(int8_t)res1.b)*(int8_t)res2.b;
									_ax=res3.x;			// 
									_f.Carry=_f.Ovf= (((res3.x & 0xff80) == 0xff80)  ||  
										((res3.x & 0xff80) == 0))  ? 0 : 1;
#ifdef EXT_NECV20
//									_f.Zero=ZERO_16();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.b=_ah;
									_f.Sign=SIGN_8();
									goto aggParity;
#endif
									break;
        // DIV NON aggiorna flag!
								case 6:       // DIV8
									if(res2.b) {
										res3.x = _ax / res2.b;
										_ah = _ax % res2.b;			// non è bello ma...
										_al = res3.b;
										if(res3.x > 0xFF) 
											goto divide0; //divide error
										}
									else
										goto divide0;
									break;
								case 7:       // IDIV8
									/*									if(res2.b) {
										if((int16_t)_ax >= 0) {
											res3.x = ((int16_t)_ax) / res2.b;
											_ah =  ((int16_t)_ax) % res2.b;
											_al = res3.b;
											if((int16_t)res3.x > 127)
												goto divide0; //divide error
											}
										else {
											res3.x = ((int16_t)_ax) / res2.b;
											_ah =  ((int16_t)_ax) % res2.b;
											_al = -res3.b;
											if((int16_t)res3.x < -128) 
												goto divide0; //divide error
											}
										}
*/
									if(res2.b) {
										res3.x = ((int16_t)_ax) / (int8_t)res2.b;
										_ah =  ((int16_t)_ax) % (int8_t)res2.b;
										_al = res3.b;
#ifdef UNDOCUMENTED_8086
										if(inRep)			// undocumented! e PRIMA del controllo, anche se non so se va pure su res3.x
											_al=-_al;
#endif
										if(((int16_t)res3.x) > 127 || ((int16_t)res3.x) < -128) 
											goto divide0; //divide error
#ifdef UNDOCUMENTED_8086
										_f.Parity=0;
#endif
										}
									else
										goto divide0;
									break;
								}
							} 
						else {
              res2.x=*op2.reg16;
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
                  _ip+=2;
        					op1.mem = Pipe2.xm.w;
                  res1.x=op1.mem;
									res3.x = res1.x & res2.x;
									goto aggFlagWZC;
									break;
								case 2:			// NOT
                  op1.reg16=op2.reg16;
									res3.x = ~res2.x;
		              *op1.reg16 = res3.x;
									break;
								case 3:			// NEG
                  op1.reg16=op2.reg16;
									res1.x=0;
									res3.x = res1.x-res2.x;
		              *op1.reg16 = res3.x;
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									goto aggFlagSW;
									break;
								case 4:       // MUL16
        					op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = ((uint32_t)res1.x)*res2.x;
									_ax=LOWORD(res3.d);			// 
									_dx=HIWORD(res3.d);			// non è bello ma...	???perché?
									_f.Carry=_f.Ovf= !!_dx;
#ifdef EXT_NECV20
//									_f.Zero=ZERO_32();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.x=_dx;
									_f.Sign=SIGN_16();
									goto aggParity;
#endif
									break;
								case 5:       // IMUL16
        					op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = ((int32_t)(int16_t)res1.x)*(int16_t)res2.x;
									_ax=LOWORD(res3.d);			// 
									_dx=HIWORD(res3.d);			// non è bello ma...	???perché?
									_f.Carry=_f.Ovf= (((res3.d & 0xffff8000) == 0xffff8000)  ||  
										((res3.d & 0xffff8000) == 0))  ? 0 : 1;
#ifdef EXT_NECV20
//									_f.Zero=ZERO_32();     // (ricontrollare 2023 per moltiplicazione
#else
									_f.Zero=0;
#endif
#ifdef UNDOCUMENTED_8086
									_f.Aux=0;
									res3.x=_dx;
									_f.Sign=SIGN_16();
									goto aggParity;
#endif
									break;
        // DIV NON aggiorna flag!
								case 6:       // DIV16
									if(res2.x) {
										res3.d = ((uint32_t)MAKELONG(_ax,_dx)) / res2.x;
										_dx = ((uint32_t)MAKELONG(_ax,_dx)) % res2.x;			// non è bello ma...
										_ax = res3.x;
										if(res3.d > 0xFFFF) 
											goto divide0; //divide error
										}
									else
										goto divide0;
									break;
								case 7:       // IDIV16
									if(res2.x) {
										res3.d = ((int32_t)MAKELONG(_ax,_dx)) / (int16_t)res2.x;
										_dx =  ((int32_t)MAKELONG(_ax,_dx)) % (int16_t)res2.x;
										_ax = res3.x;
#ifdef UNDOCUMENTED_8086
										if(inRep)			// undocumented! e PRIMA del controllo, anche se non so se va pure su res3.d
											_ax=-_ax;
										_f.Parity=0;
#endif
										if(((int32_t)res3.d) > 32767 || ((int32_t)res3.d) < -32768)
											goto divide0; //divide error
										}
									else
										goto divide0;
									break;
								}
              }
            break;
          }
				break;
        
			case 0xf8:				// CLC
				_f.Carry=0;
				break;

			case 0xf9:				// STC
				_f.Carry=1;
				break;
        
			case 0xfa:        // CLI
				_f.IF=0;
				break;

			case 0xfb:        // STI
				_f.IF=1;
        // (The delayed effect of this instruction is provided to allow interrupts to be enabled just before returning from a procedure or subroutine. For instance, if an STI instruction is followed by an RET instruction, the RET instruction is allowed to execute before external interrupts are recognized.
        inEI=1;
				break;
        
			case 0xfc:				// CLD
				_f.Dir=0;
				break;

			case 0xfd:				// STD
				_f.Dir=1;
				break;
        
			case 0xfe:    // altre INC DEC 8bit
			case 0xff:    // 
				COMPUTE_RM_OFS
					GET_MEM_OPER
						if(!(Pipe1 & 1)) {
              res1.b=GetValue(*theDs,op2.mem);
							switch(Pipe2.reg) {
								case 0:       // INC
									res2.b = 1;
									res3.b = res1.b+res2.b;
									PutValue(*theDs,op2.mem,res3.b);

aggFlagIncB:
                	_f.Ovf= res3.b == 0x80 ? 1 : 0; //V = 1 if x=7FH before, else 0
									goto aggFlagABA;
									break;
								case 1:       // DEC
									res2.b = 1;
									res3.b = res1.b-res2.b;
									PutValue(*theDs,op2.mem,res3.b);

aggFlagDecB:
                  _f.Ovf= res3.b == 0x7f ? 1 : 0; //V = 1 if x=80H before, else 0
									goto aggFlagSBA;
									break;
#ifdef EXT_NECV20
								case 3:
								case 5:			// sono "nonsense" a 8 bit, dice!
								  CPUPins |= DoHalt;
									break;
#endif
								default:
									goto unknown_istr;
									break;
								}
							}
						else {
              res1.x=GetShortValue(*theDs,op2.mem);
							switch(Pipe2.reg) {
								case 0:       // INC
									res2.x = 1;
									res3.x = res1.x+res2.x;
									PutShortValue(*theDs,op2.mem,res3.x);
									goto aggFlagIncW;
									break;
								case 1:       // DEC
									res2.x = 1;
									res3.x = res1.x-res2.x;
									PutShortValue(*theDs,op2.mem,res3.x);
									goto aggFlagDecW;
									break;
								case 2:			// CALL (D)WORD PTR
									PUSH_STACK(_ip);
									_ip=res1.x;
									break;
								case 3:			// LCALL (FAR) (D)WORD PTR
									PUSH_STACK(_cs);
									PUSH_STACK(_ip);
									_ip=res1.x;
									_cs=GetShortValue(*theDs,op2.mem+2);
									break;
								case 4:       // JMP DWORD PTR jmp [100]
									_ip=res1.x;
									break;
								case 5:       // JMP FAR DWORD PTR
									_ip=res1.x;
									_cs=GetShortValue(*theDs,op2.mem+2);

									/*{
												spoolFile=_lcreat("ibmbioetc.bin",0);
			_lwrite(spoolFile,&ram_seg[0x500],0x3700-0x500);
_lclose(spoolFile);
}*/

									break;
								case 6:       // PUSH 
#ifdef UNDOCUMENTED_8086
								case 7:
#endif
									PUSH_STACK(res1.x);
									break;
								default:
									goto unknown_istr;
									break;
								}
              }
            break;
          case 3:
						GET_REGISTER_8_16_2 
						if(!(Pipe1 & 1)) {
              res1.b=*op2.reg8;
							switch(Pipe2.reg) {
								case 0:       // INC
									res2.b = 1;
									res3.b = res1.b+res2.b;
									*op2.reg8=res3.b;
									goto aggFlagIncB;
									break;
								case 1:       // DEC
									res2.b = 1;
									res3.b = res1.b-res2.b;
									*op2.reg8=res3.b;
									goto aggFlagDecB;
									break;
								default:
									goto unknown_istr;
									break;
								}
							} 
						else {
              res1.x=*op2.reg16;
							switch(Pipe2.reg) {
								case 0:       // INC
									res2.x = 1;
									res3.x = res1.x+res2.x;
									*op2.reg16=res3.x;
									goto aggFlagIncW;
									break;
								case 1:       // DEC
									res2.x = 1;
									res3.x = res1.x-res2.x;
									*op2.reg16=res3.x;
									goto aggFlagDecW;
									break;
								case 2:			// CALL (D)WORD PTR 
									PUSH_STACK(_ip);
									_ip=res1.x;
									break;
								case 3:			// CALL FAR DWORD32 PTR
									PUSH_STACK(_cs);
									PUSH_STACK((_ip   /*+2*/));
									_ip=res1.x;			// VERIFICARE COME SI FA! o forse non c'è
									_cs=*op2.reg16+2;
									break;
								case 4:       // JMP DWORD PTR jmp [100]
									_ip=res1.x;
									break;
								case 5:       // JMP FAR DWORD PTR
									_ip=res1.x;			// VERIFICARE COME SI FA! o forse non c'è
									_cs=*op2.reg16+2;
									break;
								case 6:       // PUSH 
#ifdef UNDOCUMENTED_8086
								case 7:
#endif
#ifdef EXT_80286
#else
									if(op2.reg16==&_sp)
										res1.x-=2;
#endif
									PUSH_STACK(res1.x);
									break;
								default:
									goto unknown_istr;
									break;
								}
              }
            break;
          }
				break;
        
			default:
				{
					char myBuf[128];
unknown_istr:
/*				wsprintf(myBuf,"Istruzione sconosciuta a %04x:%04x: %02x",_cs,_ip-1,GetValue(_cs,_ip-1));
				SetWindowText(hStatusWnd,myBuf);
				strcat(myBuf," *********\n");
				_lwrite(spoolFile,myBuf,strlen(myBuf));*/
        ;
				}

#ifdef EXT_80186
				i=6 /*UD*/ ;
				goto do_irq;
#endif

				break;
			}
    
    if(_f.Trap) {
      i=0xf1;
      goto Trap;   // eseguire IRQ 0xf1??
      }

#ifdef EXT_80286
    if(_sp <= 2)
      // causare INT 0C
			;
#endif

		if(CPUPins & DoHalt) {
#ifndef EXT_NECV20		// dice che su 8088 NON si sveglia se IRQ/NMI sono disattivati!
			if(!_f.IF) {
				_ip--;
				continue;		// resta fermo!
				}
#endif
			if(!(CPUPins & (DoNMI | DoIRQ))) {
				_ip--;
				continue;		// NON esegue cmq IRQ! risveglia solo
				}
			}
   


//		if((CPUPins & DoNMI) && !inEI) {
		if((CPUPins & DoNMI) ) {
			if(inEI) 
				goto skipnmi;
      CPUPins &= ~(DoNMI | DoHalt);

			// OCCHIO IRQ rientranti! verificare

			PUSH_STACK(_f.x);
			PUSH_STACK(_cs);
      PUSH_STACK(_ip);
			_ip=GetShortValue(0,2*4);
			_cs=GetShortValue(0,2*4+2);
			_f.Trap=0; _f.IF=0;
skipnmi:
				;
			}
//		if((CPUPins & DoIRQ) && !inEI    /*&& !inRep && !segOverride*/) {
		if(CPUPins & DoIRQ) {		// andrebbe fatto su edge cmq...
			if(inEI) 
				goto skipirq;
//			if(_f.IF) { v.sopra
        CPUPins &= ~(DoIRQ | DoHalt);

				PUSH_STACK(_f.x);
				PUSH_STACK(_cs);
				PUSH_STACK(_ip);
				_f.Trap=0; _f.IF=0;	_f.Aux=0;
				_ip=GetShortValue(0,/* (i8259ICW[1] << 11) | */ (uint16_t)ExtIRQNum /*bus dati*/ *4);   // https://sw0rdm4n.wordpress.com/2014/09/09/old-knowledge-of-x86-architecture-8086-interrupt-mechanism/
				_cs=GetShortValue(0,((uint16_t)ExtIRQNum /*bus dati*/ *4) +2);
        ExtIRQNum=0;
//				}
skipirq: 
				;
			}

    if(inEI)
      inEI--;
		if(inLock)
			inLock--;			// così al secondo giro si pulisce MA OCCHIO a REP ecc
#ifdef EXT_80386
		if(sizeOverride)
      sizeOverride--;
		if(sizeOverrideA)
      sizeOverrideA--;
#endif



		} while(!fExit);
	}

