// GC 28/11/2019 -> 2024

//#warning ANCORA 2024 OCCHIO OVERFLOW, in ADD e in SUB/CMP, verificare con Z80 e 68000 2022
//per LSL ecc https://stackoverflow.com/questions/1712744/left-shift-overflow-on-68k-x86

//v. per Ovf metodo usato su 68000 senza byte alto... provare
// la cosa di inEI... chissà... e anche in Z80; però SERVE su segment override!! (ovvio; dice anche che i primi 8088 avevano questo bug, vedi CPU TEST in BIOS https://cosmodoc.org/topics/processor-detection/ )  

// (ok con BIOS migliore, per minibios serve la patch [#warning NO lo fa ancora 14/7, rivedere i clear 8259
//  Più o meno ora va tutto [, anche se dopo la schermata sembra piantarsi in eccezione (a volte) .. credo sia ancora un problema di interrupt e hw]
//   [ora VA! 21/7/24, non esce la scritta "press setup" ma direi ok, verificare bios se si vuole (ora il miniBIOS si blocca dopo il test memoria... 16/7/24
//   [ORA VA!! c'era altro bug istruzioni 20/7/24 (e con ottimizzazioni 1 e 2 si schianta... sembra aspettare la tastiera o altro, credo ancora problemi hw
//   [ OK 20/7/24 [no! v. irq timer (in debug va ma in release riporta 0K RAM e System Error 03, senza schiantarsi...


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


#define EXT_80186
#undef EXT_80286
#undef EXT_80386
#undef EXT_80486
#define EXT_80x87

BYTE fExit=0;
BYTE debug=0;

BYTE DoReset=0,DoIRQ=0,DoNMI=0,DoHalt=0;
extern BYTE ColdReset;

extern BYTE ram_seg[];
extern BYTE CGAreg[];
extern BYTE i8259RegR[2],i8259Reg2R[2];
extern BYTE ExtIRQNum;
extern uint32_t VICRaster;

BYTE Pipe1;
union PIPE Pipe2;


int Emulate(int mode) {
  // in termini di memoria usata, tenere locali le variabili è molto meglio...
#ifndef EXT_80386
	union __attribute__((__packed__)) REGISTERS {
		BYTE b[16];
	  union __attribute__((__packed__)) REG_8086 r[8];
		} regs;
#else
	union __attribute__((__packed__)) REGISTERS {
		BYTE b[32];
	  union __attribute__((__packed__)) REG_8086 r[8];
		} regs;
#endif
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
#ifndef EXT_80386
	union __attribute__((__packed__)) REGISTERS16 {
	  union __attribute__((__packed__)) REG_8086 r[4];
		} segs;
#else
	union __attribute__((__packed__)) REGISTERS32 {
	  union __attribute__((__packed__)) REG_8086 r[6];
		} segs;
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
#define ID_CARRY 0x1
#define ID_PARITY 0x4
#define ID_AUX 0x10
#define ID_ZERO 0x40
#define ID_SIGN 0x80
#define ID_TRAP 0x100
#define ID_IF  0x200
#define ID_DIR 0x400
#define ID_OVF 0x800
#define ID_IOPL 0x1000
#define ID_NESTEDTASK 0x4000
	union __attribute__((__packed__)) REGISTRO_F {
    uint16_t x;
		struct __attribute__((__packed__)) {
			unsigned int Carry: 1;
			unsigned int unused: 1;
			unsigned int Parity: 1;
			unsigned int unused2: 1;
			unsigned int Aux: 1;
			unsigned int unused3: 1;
			unsigned int Zero: 1;
			unsigned int Sign: 1;
			unsigned int HalfCarry: 1;
			unsigned int Trap: 1;
			unsigned int IF: 1;
			unsigned int Dir: 1;
			unsigned int Ovf: 1;
#ifdef EXT_80286
			unsigned int IOPL: 2;
			unsigned int NestedTask: 1;
#endif
      };
		};
	register union REGISTRO_F _f;
	union REGISTRO_F _f1;
  uint16_t inRep=0;
  BYTE segOverride=0,inEI=0;
	uint16_t *theDs;
#ifdef EXT_80386
  BYTE sizeOverride=0;
#endif
#ifdef EXT_80x87
  uint16_t status8087=0,control8087=0;
#endif
  register union OPERAND op1,op2;
  register union RESULT res1,res2,res3;
  BYTE immofs;
  register uint16_t i;
	int c=0;


	_ip=0;
  _cs=0xffff;
  _f.x=_f1.x=0; _f.unused=1;
  inRep=0; segOverride=0; inEI=0;
  DoNMI=0; DoIRQ=0; DoHalt=0; ExtIRQNum=0;
	do {

		c++;
    if(!(c & 0x3f)) {   //~64uS
  	  CGAreg[0xa] ^= 0b0001;  //retrace H (patch...
      if(!(c & 0xffff)) {
        ClrWdt();
// yield()
        }
      }
		if(ColdReset)
			continue;

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
      UpdateScreen(VICRaster,VICRaster+8 /*MIN_RASTER,MAX_RASTER*/);
      VICRaster+=8;					 	 // [raster pos count, 200 al sec... v. C64]
      if(VICRaster >= MAX_RASTER) {		 // 
        VICRaster=MIN_RASTER; 
        LED2 ^= 1;      // circa 3.5mS per 8 righe, lo faccio ogni 25mS = ~625mS 13/7/24
        }
      VIDIRQ=0;
          
      if(!SW2)        // test tastiera, me ne frego del repeat/rientro :)
        keysFeedPtr=254;
      if(!SW1)        // 
        DoReset=1;

      }
//[#warning VERIFICARE dopo bug fix; PATCH pulizia IRQ, manca out 0x20, 0x20 nel bios... 2024 MA TANTO NON VA LO STESSO PORCOCRISTO 14/7/24]
    if(i8259RegR[0] & 0x1) {
      DoIRQ=1;
// [COGLIONI! sembra che gli IRQ si attivino PRIMA del ram-test e così fallisce... per il resto sarebbe ok, non si pianta, 17/7/24
      ExtIRQNum=8;      // Timer 0 IRQ; http://www.delorie.com/djgpp/doc/ug/interrupts/inthandlers1.html
      i8259RegR[0] &= ~0x1;
      }
    if(i8259RegR[0] & 0x2) {
      DoIRQ=1;
      ExtIRQNum=9;      // IRQ 9 Keyboard
      i8259RegR[0] &= ~0x2;
      }
    if(i8259RegR[0] & 0x10) {
      DoIRQ=1;
      ExtIRQNum=0x0c;      // IRQ 12 COM1
      i8259RegR[0] &= ~0x10;
      }
    if(i8259RegR[0] & 0x40) {
      DoIRQ=1;
      ExtIRQNum=14;      // IRQ 6 Floppy disc
      i8259RegR[0] &= ~0x40;
      }
    if(i8259Reg2R[0] & 0x1) {
      DoIRQ=1;
      ExtIRQNum=0x70;      // IRQ RTC
      i8259Reg2R[0] &= ~0x1;
      }
            
		if(DoReset) {
			initHW();
      _ip=0;
      _cs=0xffff;
      
#ifdef __DEBUG
//      _ip=0x01a2;
//      _cs=0xfe00;
#endif     
      
			_f.x=_f1.x=0; _f.unused=1;    // https://thestarman.pcministry.com/asm/debug/8086REGs.htm
      inRep=0; segOverride=0; inEI=0;
			DoNMI=0; DoIRQ=0; DoHalt=0; ExtIRQNum=0;
#ifdef EXT_80386
      sizeOverride=0;
#endif
      DoReset=0;
			}
		if(DoNMI && !inEI) {
			DoNMI=0; DoHalt=0;

			PUSH_STACK(_f.x);
			PUSH_STACK(_cs);
      PUSH_STACK(_ip);
			_ip=GetShortValue(0x0008);
			_cs=GetShortValue(0x000a);
			_f.Trap=0; _f.IF=0;
			}
		if(DoIRQ && !inEI) {
			if(_f.IF) {
				DoIRQ=0; DoHalt=0; 
				PUSH_STACK(_f.x);
				PUSH_STACK(_cs);
				PUSH_STACK(_ip);
				_f.Trap=0; _f.IF=0;
				_ip=GetShortValue((uint16_t)ExtIRQNum /*bus dati*/ *4);   // https://sw0rdm4n.wordpress.com/2014/09/09/old-knowledge-of-x86-architecture-8086-interrupt-mechanism/
				_cs=GetShortValue(((uint16_t)ExtIRQNum /*bus dati*/ *4) +2);
        ExtIRQNum=0;
				}
			}

		if(DoHalt)
			continue;		// esegue cmq IRQ
   
//printf("Pipe1: %02x, Pipe2w: %04x, Pipe2b1: %02x,%02x\n",Pipe1,Pipe2.word,Pipe2.bytes.byte1,Pipe2.bytes.byte2);
// http://ref.x86asm.net/coder32.html#two-byte
    switch(inRep) {
      case 0:
        break;
      case 1:   // REPZ
        _cx--;
        if(_f.Zero && _cx)
          _ip--;
        else
          inRep=0;
        break;
      case 2:   // REPNZ
        _cx--;
        if(!_f.Zero && _cx)
          _ip--;
        else
          inRep=0;
        break;
      case 3:   // REP (v.singoli casi)
        if(--_cx)
          _ip--;
        else
          inRep=0;
        break;
      default:      // al primo giro...
        inRep &= 0xff;
        break;
      }


 
/*    if(_ip == 0x01ee) {
      Nop();
      Nop();
      }*/
    
    if(inEI)
      inEI=0;

    
     LED1 ^= 1;      // ~ 500/700nS  2/12/19 (con opt=1); un PC/XT @4.77 va a 200nS e impiega una media di 10/20 cicli per opcode => 2-4uS, ergo siamo 6-8 volte più veloci
                    // .9-2uS 16/7/24 senza ottimizzazioni (con O1 si pianta emulatore...
     
#ifdef USING_SIMULATOR
// fa cagare, lentissimo anche con baud rate alto     printf("CS:IP=%04X:%04X\n",_cs,_ip);
#endif
// http://www.mlsite.net/8086/  per hex-codes
		switch(GetPipe(MAKE20BITS(_cs,_ip++))) {

			case 0:     // ADD registro a r/m
			case 1:
			case 2:     // ADD r/m a registro
			case 3:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b+res2.b;      // cast così ottengo il carry!
                PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                goto aggFlagAB;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
        				res3.d = (uint32_t)res1.x+res2.x;   // cast così ottengo il carry!
                PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
                goto aggFlagAW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
								res3.x = (uint16_t)res1.b+res2.b;      // cast così ottengo il carry!
                *op1.reg8 = res3.b;
                goto aggFlagAB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
        				res3.d = (uint32_t)res1.x+res2.x;   // cast così ottengo il carry!
                *op1.reg16 = res3.x;
                goto aggFlagAW;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b+res2.b;      // cast così ottengo il carry!
                *op2.reg8=res3.b;
                goto aggFlagAB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
        				res3.d = (uint32_t)res1.x+res2.x;   // cast così ottengo il carry!
                *op2.reg16=res3.x;
                goto aggFlagAW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.x = (uint16_t)res1.b+res2.b;      // cast così ottengo il carry!
                *op1.reg8=res3.b;
                goto aggFlagAB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
        				res3.d = (uint32_t)res1.x+res2.x;   // cast così ottengo il carry!
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
				res3.x = (uint16_t)res1.b+res2.b;      // cast così ottengo il carry!
        _al = res3.b;
				_ip++;
				
aggFlagAB:     // carry, ovf, aux, zero, sign, parity
				_f.Carry=res3.x & 0xff00 ? 1 : 0;
//        _f.Ovf = (res1.b & 0x40 + res2.b & 0x40) != (res1.b & 0x80 + res2.b & 0x80);
//        _f.Ovf = !!(((res1.b & 0x40) + (res2.b & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
//        _f.Ovf = !!_f.Carry != !!((res3.b & res1.b & res2.b) & 0x80);
        _f.Ovf = !!(res3.x & 0xff00) != !!((res3.b & 0x80) ^ (res1.b & 0x80) ^ (res2.b & 0x80));
//				_f.Ovf = ((res1.b ^ res2.b) & 0x80) ? 1 : 0;		
        /*The OVERFLOW flag is the XOR of the carry coming into the sign bit (if
any) with the carry going out of the sign bit (if any).  Overflow happens
if the carry in does not equal the carry out.*/
        
aggFlagABA:    // aux, zero, sign, parity
        _f.Aux = (res1.b & 0xf + res2.b & 0xf) >= 0x10;   // SOTTRAZIONE??
/*AUX flag: 1-carry out from bit 3 on addition or borrow into bit 3 on subtraction
0-otherwise*/

aggFlagBZ:    // zero, sign, parity
				_f.Zero=res3.b ? 0 : 1;
				_f.Sign=(((int8_t)res3.b) < 0) ? 1 : 0;
        {
        BYTE par,parn;
        parn=res3.b;
        par= parn >> 1;			// Microchip AN774; https://stackoverflow.com/questions/17350906/computing-the-parity
        par ^= parn;
        parn= par >> 2;
        par ^= parn;
        parn= par >> 4;
        par ^= parn;
        _f.Parity=par & 1;
        }
				break;

			case 5:       // ADDW immediate
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.d = (uint32_t)res1.x+res2.x;   // cast così ottengo il carry!
        _ax = res3.x;
				_ip+=2;
        
aggFlagAW:     // carry, ovf, aux, zero, sign, parity
				_f.Carry=res3.d & 0xff0000 ? 1 : 0;
//        _f.Ovf = (res1.x & 0x4000 + res2.x & 0x4000) != (res1.x & 0x8000 + res2.x & 0x8000);
//        _f.Ovf = !!(((res1.x & 0x4000) + (res2.x & 0x4000)) & 0x8000) != !!(((res1.d & 0x8000) + (res2.d & 0x8000)) & 0x10000);
//        _f.Ovf = !!_f.Carry != !!((res3.x & res1.x & res2.x) & 0x8000);
        _f.Ovf = !!(res3.d & 0xffff00) != !!((res3.x & 0x8000) ^ (res1.x & 0x8000) ^ (res2.x & 0x8000));
//				_f.Ovf = ((res1.x ^ res2.x) & 0x8000) ? 1 : 0;		
        /*The OVERFLOW flag is the XOR of the carry coming into the sign bit (if
any) with the carry going out of the sign bit (if any).  Overflow happens
if the carry in does not equal the carry out.*/
		// per overflow e halfcarry https://stackoverflow.com/questions/8034566/overflow-and-carry-flags-on-z80
aggFlagAWA:    // aux, zero, sign, parity
        _f.Aux = (res1.b & 0xf + res2.b & 0xf) >= 0x10;   // SEMPRE byte basso; SOTTRAZIONE??
/*1-carry out from bit 7 on addition or borrow into bit 7 on subtraction
0-otherwise*/
        
aggFlagWZ:    // zero, sign, parity
				_f.Zero=res3.x ? 0 : 1;
				_f.Sign=(((int16_t)res3.x) < 0) ? 1 : 0;
        {
        BYTE par,parn;
        parn=res3.b;
        par= parn >> 1;			// https://stackoverflow.com/questions/49763042/how-to-calculate-parity-using-xor
        par ^= parn;
        parn= par >> 2;
        par ^= parn;
        parn= par >> 4;
        par ^= parn;
//        parn= par >> 8;   // no! dice che cmq è solo sul byte basso https://stackoverflow.com/questions/29292455/parity-of-a-number-assembly-8086
//        par ^= parn;
        _f.Parity=par & 1;
        }
				break;
        
			case 6:       // PUSH segment
			case 0xe:
			case 0x16:
			case 0x1e:
				PUSH_STACK(segs.r[(Pipe1 >> 3) & 3].x);
				break;
        
			case 0xf:       // non esiste POP CS (abbastanza logico); istruzioni MMX/386 qua...
				_ip++;
//     		switch(GetPipe(MAKE20BITS(_cs,++_ip))) {
#warning PATCH MiniBIOS per caricare DOS...
        
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
          case 0xff:      // TRAP
            Nop();
            break;
            
#ifdef EXT_80x87
          status8087,control8087;     // ma v. 0x9b!! boh...
            
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
          case 0x31:      // RDTSC; si potrebbe mettere anche in 8086
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
            _eax=
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
          case 0xa6:      // CMPXCHG/CMPXCHG8B
          case 0xa7:
          case 0xb0:
          case 0xb1:
          case 0xc7:
            break;
#endif
          }
				break;
        
			case 7:         // POP segment
			case 0x17:
			case 0x1f:
				POP_STACK(segs.r[(Pipe1 >> 3) & 3].x);
				break;

			case 0x08:      // OR
			case 0x09:
			case 0x0a:
			case 0x0b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
                res3.b=res1.b | res2.b;
                PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
                res3.x=res1.x | res2.x;
                PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res3.b=res1.b | res2.b;
                *op1.reg8 = res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res3.x=res1.x | res2.x;
                *op1.reg16 = res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
                res3.b=res1.b | res2.b;
                *op2.reg8=res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x=res1.x | res2.x;
                *op2.reg16=res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
                res3.b=res1.b | res2.b;
                *op1.reg8=res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x=res1.x | res2.x;
                *op1.reg16=res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              }
            break;
          }
				break;

			case 0x0c:      // OR
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.x = (uint16_t)res1.b | res2.b;
        _al = res3.b;
				_ip++;
        _f.Carry=0; _f.Ovf=0;   // Aux undefined
        goto aggFlagBZ;
				break;

			case 0x0d:      // OR
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x | res2.x;
        _ax = res3.x;
				_ip+=2;
        _f.Carry=0; _f.Ovf=0;   // Aux undefined
        goto aggFlagWZ;
				break;


/*      case 0x0f:				//LSS
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
				/*break;*/

        
			case 0x10:     // ADC registro a r/m
			case 0x11:
			case 0x12:     // ADC r/m a registro
			case 0x13:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b+res2.b+_f.Carry;      // cast così ottengo il carry!
                PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                goto aggFlagAB;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x+res2.x+_f.Carry;
                PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
                goto aggFlagAW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
								res3.x = (uint16_t)res1.b+res2.b+_f.Carry;      // cast così ottengo il carry!
                *op1.reg8 = res3.b;
                goto aggFlagAB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res3.d=(uint32_t)res1.x+res2.x+_f.Carry;
                *op1.reg16 = res3.x;
                goto aggFlagAW;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b+res2.b+_f.Carry;      // cast così ottengo il carry!
                *op2.reg8=res3.b;
                goto aggFlagAB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x+res2.x+_f.Carry;
                *op2.reg16=res3.x;
                goto aggFlagAW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.x = (uint16_t)res1.b+res2.b+_f.Carry;      // cast così ottengo il carry!
                *op1.reg8=res3.b;
                goto aggFlagAB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.d=(uint32_t)res1.x+res2.x+_f.Carry;
                *op1.reg16=res3.x;
                goto aggFlagAW;
                break;
              }
            break;
          }
				break;

			case 0x14:        // ADC
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.x = (uint16_t)res1.b+res2.b+_f.Carry;      // cast così ottengo il carry!
        _al = res3.b;
				_ip++;
        goto aggFlagAB;
				break;

			case 0x15:        // ADC
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.d = (uint32_t)res1.x+res2.x+_f.Carry;   // cast così ottengo il carry!
        _ax = res3.x;
				_ip+=2;
        goto aggFlagAW;
				break;

			case 0x18:        // SBB
			case 0x19:
			case 0x1a:
			case 0x1b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b-res2.b-_f.Carry;      // cast così ottengo il carry!
                PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                goto aggFlagAB;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x-res2.x-_f.Carry;
                PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
                goto aggFlagAW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
								res3.x = (uint16_t)res1.b-res2.b-_f.Carry;      // cast così ottengo il carry!
                *op1.reg8 = res3.b;
                goto aggFlagAB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res3.d=(uint32_t)res1.x-res2.x-_f.Carry;
                *op1.reg16 = res3.x;
                goto aggFlagAW;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b-res2.b-_f.Carry;      // cast così ottengo il carry!
                *op2.reg8=res3.b;
                goto aggFlagSB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x+res2.x-_f.Carry;
                *op2.reg16=res3.x;
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.x = (uint16_t)res1.b-res2.b-_f.Carry;      // cast così ottengo il carry!
                *op1.reg8=res3.b;
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.d=(uint32_t)res1.x-res2.x-_f.Carry;
                *op1.reg16=res3.x;
                goto aggFlagSW;
                break;
              }
            break;
          }
				break;

			case 0x1c:        // SBB
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.x = (uint16_t)res1.b-res2.b-_f.Carry;      // cast così ottengo il carry!
        _al = res3.b;
				_ip++;
        goto aggFlagSB;
				break;

			case 0x1d:        // SBB
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.d = (uint32_t)res1.x-res2.x-_f.Carry;   // cast così ottengo il carry!
        _ax = res3.x;
				_ip+=2;
        goto aggFlagSW;
				break;

			case 0x20:        // AND
			case 0x21:
			case 0x22:
			case 0x23:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
                res3.b=res1.b & res2.b;
                PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
                res3.x=res1.x & res2.x;
                PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res3.b=res1.b & res2.b;
                *op1.reg8 = res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res3.x=res1.x & res2.x;
                *op1.reg16 = res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
                res3.b=res1.b & res2.b;
                *op2.reg8=res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x=res1.x & res2.x;
                *op2.reg16=res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
                res3.b=res1.b & res2.b;
                *op1.reg8=res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x=res1.x & res2.x;
                *op1.reg16=res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              }
            break;
          }
				break;

			case 0x24:        // AND
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.x = (uint16_t)res1.b & res2.b;
        _al = res3.b;
				_ip++;
        _f.Carry=0; _f.Ovf=0;   // Aux undefined
        goto aggFlagBZ;
				break;

			case 0x25:        // AND
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x & res2.x;
        _ax = res3.x;
				_ip+=2;
        _f.Carry=0; _f.Ovf=0;   // Aux undefined
        goto aggFlagWZ;
				break;

			case 0x26:
				segOverride=0+1;			// ES
        inEI=1;
				break;

			case 0x27:				// DAA
        res3.x=res1.x=_al;
        i=_f.Carry;
        _f.Carry=0;
        if((_al & 0xf) > 9 || _f.Aux) {
          res3.x+=6;
          _al=res3.b;
          _f.Carry= i || HIBYTE(res3.x);
          _f.Aux=1;
          }
        else
          _f.Aux=0;
        if((res1.b>0x99) || i) {
          _al+=0x60;  
          _f.Carry=1;
          }
        else
          _f.Carry=0;
        break;
        
			case 0x28:      // SUB
			case 0x29:
			case 0x2a:
			case 0x2b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                goto aggFlagSB;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x-res2.x;
                PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                *op1.reg8 = res3.b;
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res3.d=(uint32_t)res1.x-res2.x;
                *op1.reg16 = res3.x;
                goto aggFlagSW;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                *op2.reg8=res3.b;
                goto aggFlagSB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x-res2.x;
                *op2.reg16=res3.x;
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                *op1.reg8=res3.b;
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.d=(uint32_t)res1.x-res2.x;
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
				res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
        _al = res3.b;
				_ip++;

aggFlagSB:     // carry, ovf, aux, zero, sign, parity
				_f.Carry=res3.x & 0xff00 ? 1 : 0;
//        _f.Ovf = (res1.b & 0x40 + res2.b & 0x40) != (res1.b & 0x80 + res2.b & 0x80);
//        _f.Ovf = !!(((res1.b & 0x40) + (res2.b & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
//        _f.Ovf = !!_f.Carry != !!((res3.b & res1.b & res2.b) & 0x80);
        _f.Ovf = !!(res3.x & 0xff00) != !!((res3.b & 0x80) ^ (res1.b & 0x80) ^ (res2.b & 0x80));
//				_f.Ovf = ((res1.b ^ res2.b) & 0x80) ? 1 : 0;		
        /*The OVERFLOW flag is the XOR of the carry coming into the sign bit (if
any) with the carry going out of the sign bit (if any).  Overflow happens
if the carry in does not equal the carry out.*/
        
aggFlagSBA:    // aux, zero, sign, parity
        _f.Aux = (res1.b & 0xf + res2.b & 0xf) >= 0x10;   // VERIFICARE SOTTRAZIONE
/*AUX flag: 1-carry out from bit 3 on addition or borrow into bit 3 on subtraction
0-otherwise*/

        goto aggFlagBZ;
				break;

			case 0x2d:			// SUBW
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.d = (uint32_t)res1.x-res2.x;   // cast così ottengo il carry!
        _ax = res3.x;
				_ip+=2;
        
aggFlagSW:     // carry, ovf, aux, zero, sign, parity
				_f.Carry=res3.d & 0xff0000 ? 1 : 0;
//        _f.Ovf = (res1.x & 0x4000 + res2.x & 0x4000) != (res1.x & 0x8000 + res2.x & 0x8000);
//        _f.Ovf = !!(((res1.x & 0x4000) + (res2.x & 0x4000)) & 0x8000) != !!(((res1.d & 0x8000) + (res2.d & 0x8000)) & 0x10000);
//        _f.Ovf = !!_f.Carry != !!((res3.x & res1.x & res2.x) & 0x8000);
        _f.Ovf = !!(res3.d & 0xffff0000) != !!((res3.x & 0x8000) ^ (res1.x & 0x8000) ^ (res2.x & 0x8000));

//				_f.Ovf = ((res1.x ^ res2.x) & 0x8000) ? 1 : 0;		
        /*The OVERFLOW flag is the XOR of the carry coming into the sign bit (if
any) with the carry going out of the sign bit (if any).  Overflow happens
if the carry in does not equal the carry out.*/
		// per overflow e halfcarry https://stackoverflow.com/questions/8034566/overflow-and-carry-flags-on-z80
aggFlagSWA:    // aux, zero, sign, parity
        _f.Aux = (res1.b & 0xf + res2.b & 0xf) >= 0x10;   // SEMPRE byte basso; SOTTRAZIONE??
/*1-carry out from bit 7 on addition or borrow into bit 7 on subtraction
0-otherwise*/
        
        goto aggFlagWZ;
				break;

			case 0x2e:
				segOverride=1+1;			// CS
        inEI=1;
				break;

			case 0x2f:				// DAS
        /*     old_AL ? AL;
        old_CF ? CF;
        CF ? 0;
        IF (((AL AND 0FH) > 9) or AF = 1)
            THEN
                    AL ? AL - 6;
                    CF ? old_CF or (Borrow from AL ? AL ? 6);
                    AF ? 1;
            ELSE
                    AF ? 0;
        FI;
        IF ((old_AL > 99H) or (old_CF = 1))
                THEN
                    AL ? AL ? 60H;
                    CF ? 1;
        FI;*/
        goto aggFlagABA;			// ????
				break;
        
			case 0x30:        // XOR
			case 0x31:
			case 0x32:
			case 0x33:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
                res3.b=res1.b ^ res2.b;
                PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
                res3.x=res1.x ^ res2.x;
                PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res3.b=res1.b ^ res2.b;
                *op1.reg8 = res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res3.x=res1.x ^ res2.x;
                *op1.reg16 = res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
                res3.b=res1.b ^ res2.b;
                *op2.reg8=res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.x=res1.x ^ res2.x;
                *op2.reg16=res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
                res3.b=res1.b ^ res2.b;
                *op1.reg8=res3.b;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagBZ;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.x=res1.x ^ res2.x;
                *op1.reg16=res3.x;
								_f.Carry=0; _f.Ovf=0;   // Aux undefined
								goto aggFlagWZ;
                break;
              }
            break;
          }
        break;
        
			case 0x34:        // XOR
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.x = (uint16_t)res1.b ^ res2.b;
        _al = res3.b;
				_ip++;
        _f.Carry=0; _f.Ovf=0;   // Aux undefined
        goto aggFlagBZ;
				break;

			case 0x35:        // XOR
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.x = res1.x ^ res2.x;
        _ax = res3.x;
				_ip+=2;
        _f.Carry=0; _f.Ovf=0;   // Aux undefined
        goto aggFlagWZ;
				break;

			case 0x36:
        segOverride=2+1;			// SS
        inEI=1;
				break;

			case 0x37:				// AAA
        if((_al & 0xf) > 9 || _f.Aux) {
          _al+=6;
          _ah++;
          _f.Aux=1;
          _f.Carry=1;
          }
        else {
          _f.Aux=0;
          _f.Carry=0;
          }
        _al &= 0xf;
				break;

			case 0x38:      // CMP
			case 0x39:
			case 0x3a:
			case 0x3b:
				COMPUTE_RM
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                goto aggFlagSB;
                break;
              case 1:
                res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x-res2.x;
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                res3.d=(uint32_t)res1.x-res2.x;
                goto aggFlagSW;
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1)
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            switch(Pipe1 & 0x3) {
              case 0:
                res1.b=*op2.reg8;
                res2.b=*op1.reg8;
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                goto aggFlagSB;
                break;
              case 1:
                res1.x=*op2.reg16;
                res2.x=*op1.reg16;
                res3.d=(uint32_t)res1.x-res2.x;
                goto aggFlagSW;
                break;
              case 2:
                res1.b=*op1.reg8;
                res2.b=*op2.reg8;
								res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
                goto aggFlagSB;
                break;
              case 3:
                res1.x=*op1.reg16;
                res2.x=*op2.reg16;
                res3.d=(uint32_t)res1.x-res2.x;
                goto aggFlagSW;
                break;
              }
            break;
          }
				break;
        
			case 0x3c:      // CMPB
        res1.b=_al;
        res2.b=Pipe2.b.l;
				res3.x = (uint16_t)res1.b-res2.b;      // cast così ottengo il carry!
				_ip++;
        goto aggFlagSB;
				break;

			case 0x3d:			// CMPW
        res1.x=_ax;
        res2.x=Pipe2.x.l;
				res3.d = (uint32_t)res1.x-res2.x;   // cast così ottengo il carry!
				_ip+=2;
        goto aggFlagSW;
				break;

			case 0x3e:
        segOverride=3+1;			// DS
        inEI=1;
				break;

 			case 0x3f:      // AAS
        if((_al & 0xf) > 9 || _f.Aux) {
          _ax-=6;
          _ah--;
          _f.Aux=1;
          _f.Carry=1;
          }
        else {
          _f.Aux=0;
          _f.Carry=0;
          }
        _al &= 0xf;
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
				res3.x = *op2.reg16+1;
				*op2.reg16 = res3.x;
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
				res3.x = *op2.reg16-1;
				*op2.reg16 = res3.x;
      	_f.Ovf= res3.x == 0x7fff ? 1 : 0; //V = 1 if x=8000H before, else 0
        goto aggFlagSWA;
				break;

			case 0x50:
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x54:
			case 0x55:
			case 0x56:
			case 0x57:
        op2.reg16 = &regs.r[Pipe1 & 0x7].x;
				PUSH_STACK(*op2.reg16);
				break;

			case 0x58:
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x5c:
			case 0x5d:
			case 0x5e:
			case 0x5f:
        op2.reg16 = &regs.r[Pipe1 & 0x7].x;
				POP_STACK(*op2.reg16);
				break;

#ifdef EXT_80186
			case 0x60:      // PUSHA
        i=regs.r[4].x;      // SS pushato INIZIALE!
        for(i=0; i<4; i++)    // 
          PUSH_STACK(regs.r[i].x);
        PUSH_STACK(i);
        for(i=5; i<8; i++)    // 
          PUSH_STACK(regs.r[i].x);
				break;
			case 0x61:      // POPA
        for(i=7; i>4; i++)    // 
          POP_STACK(regs.r[i].x);
        POP_STACK(_ax);     // dummy
        for(i=3; i>=0; i++)
          POP_STACK(regs.r[i].x);
				break;
			case 0x62:        // BOUND
#define WORKING_REG regs.r[Pipe2.reg].x
        GetMorePipe(MAKE20BITS(_cs,_ip-1));
        if(WORKING_REG < Pipe2.x.l || WORKING_REG > Pipe2.x.h)    // finire...
          // dovrebbe causare un INT 5
          ;
        _ip+=4;
				break;
#endif
#ifdef EXT_80286
			case 0x63:        // ARPL
				break;
#endif
        
#ifdef EXT_80386
			case 0x64:
				segOverride=4+1;			// FS
        inEI=1;
				break;
        
			case 0x65:
				segOverride=5+1;			// GS
        inEI=1;
				break;
#endif
        
#ifdef EXT_80386
			case 0x66:
				//invert operand size next instr
        sizeOverride;
        
     		switch(GetPipe(MAKE20BITS(_cs,++_ip))) {
          case 0x99:    // anche CDQ
            _edx= _eax & 0x80000000 ? 0xffffffff : 0;
    				break;
          case 0x98:    // anche CWDE
            _eax |= _ax & 0x8000 ? 0xffff0000 : 0;
          	break;
          }
        
				break;
			case 0x67:
        sizeOverride;
        
     		switch(GetPipe(MAKE20BITS(_cs,++_ip))) {
          case 0x63:      // JCXZ
            break;
          }
				//invert address size next instr
				break;
#endif
        
#ifdef EXT_80286
			case 0x68:
				PUSH_STACK(Pipe2.x.l);
        _ip+=2;
				break;
#endif
        
#ifdef EXT_80186
			case 0x69:			//IMUL16
// usare         immofs ??
        immofs=1;
				{
			  union OPERAND op3;

				GetPipe(MAKE20BITS(_cs,_ip++));		// mi sposto avanti!
        GetMorePipe(MAKE20BITS(_cs,_ip-2));   // RISISTEMARE!!

        op1.reg16 = &regs.r[(Pipe1 >> 3) & 0x7].x;		// 3 o 7??

				GetPipe(MAKE20BITS(_cs,_ip++));		// mi sposto ancora avanti!
				COMPUTE_RM_OFS
            if((Pipe1 & 0xc0) == 0x40) {
              op2.mem+=(int8_t)Pipe2.b.l;
              op3.mem=Pipe2.xm.w;
							}
            else if((Pipe1 & 0xc0) == 0x80) {
              op2.mem+=(int16_t)Pipe2.x.l;
//              GetMorePipe(MAKE20BITS(_cs,_ip-1));
              op3.mem=Pipe2.x.h;
							}
            res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
            res2.x=op3.mem;
            res3.d=(int32_t)res1.x*res2.x;   // sicuro 32?
            *op1.reg16=res3.x;
//            _dx=HIWORD(res3.x); qua no!
            goto aggFlagAW;     // ricontrollare 2023 per moltiplicazione
            break;
          case 3:
            op1.reg16= &regs.r[Pipe1 & 0x7].x;			// 3 o 7 ??
            op3.mem=Pipe2.x.l;
    				_ip++;      // imm16
            res1.x=*op2.reg16;
            res2.x=op3.mem;
            res3.d=(int32_t)res1.x*res2.x;   // sicuro 32??
            *op1.reg16=res3.x;
//            _dx=HIWORD(res3.x); qua no!
            goto aggFlagAW;     // ricontrollare 2023 per moltiplicazione
            break;
          }
				}
				

#ifdef EXT_80386
#endif
				break;

			case 0x6a:        // PUSH imm8
				PUSH_STACK(Pipe2.b.l);
        _ip++;
        break;

			case 0x6b:			//IMUL8
				{
			  union OPERAND op3;
// usare         immofs ??
        immofs=1;

				GetPipe(MAKE20BITS(_cs,_ip++));		// mi sposto avanti!

        op1.reg16= &regs.r[(Pipe1 >> 3) & 0x3].x;
       
				GetPipe(MAKE20BITS(_cs,_ip++));		// mi sposto ancora avanti!
				COMPUTE_RM_OFS
            if((Pipe1 & 0xc0) == 0x40) {
              op2.mem+=(int8_t)Pipe2.b.l;
              op3.mem=Pipe2.b.h;
							}
            else if((Pipe1 & 0xc0) == 0x80) {
              op2.mem+=(int16_t)Pipe2.x.l;
              op3.mem=Pipe2.b.u;
							}
            res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
            res2.x=op3.mem;
            res3.d=(uint32_t)res1.x*res2.x;   // sicuro 32?
            *op1.reg16=res3.x;
//            _dx=HIWORD(res3.d); qua no!
            goto aggFlagAW;     // ricontrollare 2023 per moltiplicazione
            break;
          case 3:
            op1.reg16= &regs.r[Pipe1 & 0x7].x;			// 3 o 7 ??
            op3.mem=Pipe2.b.l;
    				_ip++;      // imm8
            res1.x=*op2.reg16;
            res2.x=op3.mem;
            res3.d=(uint32_t)res1.x*res2.x;   // sicuro 32?
            *op1.reg16=res3.x;
//            _dx=HIWORD(res3.d); qua no!
            goto aggFlagAW;       // ricontrollare 2023 per moltiplicazione
            break;
          }
				}
				
#ifdef EXT_80386
#endif
				break;

			case 0x6c:        // INSB
				PutValue(MAKE20BITS(_es,_di),InValue(_dx));
        if(_f.Dir)
          _di--;
        else
          _di++;
        if(inRep) 
          inRep=3;
				break;

			case 0x6d:        // INSW
				PutShortValue(MAKE20BITS(_es,_di),InShortValue(_dx));
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          }
        else {
          _di+=2;
          }
        if(inRep) 
          inRep=3;
				break;

			case 0x6e:        // OUTSB
				OutValue(_dx,GetValue(MAKE20BITS(_es,_di)));
        if(_f.Dir)
          _di--;
        else
          _di++;
        if(inRep) 
          inRep=3;
				break;

			case 0x6f:        // OUTSW
				OutShortValue(_dx,GetShortValue(MAKE20BITS(_es,_di)));
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          }
        else {
          _di+=2;
          }
        if(inRep) 
          inRep=3;
				break;
#endif

			case 0x70:    // JO
				_ip++;
				if(_f.Ovf)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x71:    // JNO
				_ip++;
				if(!_f.Ovf)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x72:    // JB, JC, JNAE
				_ip++;
				if(_f.Carry)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x73:    // JAE, JNB, JNC
				_ip++;
				if(!_f.Carry)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x74:      // JE, JZ
				_ip++;
				if(_f.Zero)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x75:      // JNE, JNZ
				_ip++;
				if(!_f.Zero)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x76:    // JBE, JNA
				_ip++;
				if(_f.Zero || _f.Carry)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x77:      // JA, JNBE
				_ip++;
				if(!_f.Zero && !_f.Carry)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x78:          // JS
				_ip++;
				if(_f.Sign)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x79:          // JNS
				_ip++;
				if(!_f.Sign)
					_ip+=(int8_t)Pipe2.b.l;
				break;

      case 0x7a:          // JP, JPE
				_ip++;
				if(_f.Parity)
					_ip+=(int8_t)Pipe2.b.l;
				break;
        
			case 0x7b:          // JNP, JPO
				_ip++;
				if(!_f.Parity)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x7c:          // JL, JNGE
				_ip++;
				if(_f.Sign!=_f.Ovf)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x7d:
				_ip++;
				if(_f.Sign==_f.Ovf)     // JGE, JNL
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x7e:      // JLE, JNG
				_ip++;
				if(_f.Zero || _f.Sign!=_f.Ovf)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x7f:      // JG, JNLE
				_ip++;
				if(!_f.Zero && _f.Sign==_f.Ovf)
					_ip+=(int8_t)Pipe2.b.l;
				break;

			case 0x80:				// ADD ecc rm8, immediate8
			case 0x81:				// ADD ecc rm16, immediate16
			case 0x82:				// ADD ecc rm8, immediate8  con sign-extend
			case 0x83:				// ADD ecc rm16, immediate16 con sign-extend
        GetMorePipe(MAKE20BITS(_cs,_ip-1));   // andrebbe fatto solo se necessario... RISISTEMARE!
        _ip++;
				if(!(Pipe1 & 2)) 			// vuol dire che l'operando è 8bit ma esteso a 16
          //ma non so bene che fa 82... v. anche sotto
          _ip++;
        
        switch(Pipe2.mod) {
          case 0x2:
    				_ip++;
          case 0x1:
            _ip++;
          case 0x00:
            immofs=1;
            switch(Pipe2.rm) {
              case 0:
                theDs=&_ds; op2.mem=_bx+_si;    // UNSIGNED!
                break;
              case 1:
                theDs=&_ds; op2.mem=_bx+_di;
                break;
              case 2:
                theDs=&_ss; op2.mem=_bp+_si;
                break;
              case 3:
                theDs=&_ss; op2.mem=_bp+_di;
                break;
              case 4:
                theDs=&_ds; op2.mem=_si;
                break;
              case 5:
                theDs=&_ds; op2.mem=_di;
                break;
              case 6:
								if(Pipe2.mod) {
                  theDs=&_ss; 
                  op2.mem=_bp;
                  }
                else {
                  theDs=&_ds; 
                  op2.mem=Pipe2.xm.w;
                  immofs+=2;
                  _ip+=2;
                  }
                break;
              case 7:
                theDs=&_ds; op2.mem=_bx;
                break;
              }
            if(Pipe2.mod == 1) {
              op2.mem+=(int8_t)Pipe2.b.h;
              immofs++;
              }
            else if(Pipe2.mod == 2) {
              op2.mem+=(int16_t)Pipe2.xm.w;
              immofs+=2;
              }
						if(segOverride) {
							theDs=&segs.r[segOverride-1].x;
							segOverride=0;
							}
            // usare COMPUTE_RM_OFS 2024
            
//            if(immofs>3)
//              GetMorePipe(MAKE20BITS(_cs,_ip-1));
						if(!(Pipe1 & 1)) {
// 3/12              _ip++;
              
              res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
              op1.mem=Pipe2.bd[immofs];
							res2.b=op1.mem;
							switch(Pipe2.reg) {
			          case 0:       // ADD
									res3.x=(uint16_t)res1.b + res2.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
		              PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagAB;
									break;
								case 1:       // OR
									res3.b=res1.b | res2.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
		              PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagBZ;
									break;
								case 2:       // ADC
									res3.x=(uint16_t)res1.b + res2.b + _f.Carry;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
		              PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagAB;
									break;
								case 3:       // SBB
									res3.x=(uint16_t)res1.b - res2.b - _f.Carry;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
		              PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagSB;
									break;
								case 4:       // AND
									res3.b=res1.b & res2.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
		              PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagBZ;
									break;
								case 5:       // SUB
									res3.x=(uint16_t)res1.b - res2.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
		              PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagSB;
									break;
								case 6:       // XOR
									res3.b=res1.b ^ res2.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
		              PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagBZ;
									break;
								case 7:       // CMP
									res3.x=(uint16_t)res1.b - res2.b;
									if(Pipe1 & 2) {			// sign extend
										//??
										}
									goto aggFlagSB;
									break;
								}
							}
						else {
              _ip++;
              res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
							if(Pipe1 & 2) 			// sign extend
                op1.mem=(int16_t)Pipe2.bd[immofs];
							else
                op1.mem=MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
              res2.x=op1.mem;
							switch(Pipe2.reg) {
			          case 0:       // ADD
									res3.d=(uint32_t)res1.x + res2.x;
		              PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									goto aggFlagAW;
									break;
								case 1:       // OR
									res3.x=res1.x | res2.x;
#ifdef EXT_80386
									if(Pipe1 & 2) {			// sign extend
										_dx = _ax & 0x8000 ? 0xffff : 0;
										}
#endif
		              PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									goto aggFlagWZ;
									break;
			          case 2:       // ADC
									res3.d=(uint32_t)res1.x + res2.x + _f.Carry;
		              PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									goto aggFlagAW;
									break;
			          case 3:       // SBB
									res3.d=(uint32_t)res1.x - res2.x - _f.Carry;
		              PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									goto aggFlagSW;
									break;
			          case 4:       // AND
									res3.x=res1.x & res2.x;
#ifdef EXT_80386
									if(Pipe1 & 2) {			// sign extend
										_dx = _ax & 0x8000 ? 0xffff : 0;
										}
#endif
		              PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									goto aggFlagWZ;
									break;
			          case 5:       // SUB
									res3.d=(uint32_t)res1.x - res2.x;
		              PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									goto aggFlagSW;
									break;
			          case 6:       // XOR
									res3.x=res1.x ^ res2.x;
#ifdef EXT_80386
									if(Pipe1 & 2) {			// sign extend
										_dx = _ax & 0x8000 ? 0xffff : 0;
										}
#endif
		              PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									goto aggFlagWZ;
									break;
			          case 7:       // CMP
									res3.d=(uint32_t)res1.x - res2.x;
									goto aggFlagSW;
									break;
								}
              }
            break;
          case 3:
            immofs=0;
						if(!(Pipe1 & 1)) {
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
              res1.b=*op2.reg8;
							op1.mem=Pipe2.b.h; // CONTROLLARE!
//Pipe2.bd[immofs];
							res2.b=op1.mem;
							switch(Pipe2.reg) {
			          case 0:       // ADD
									res3.x=(uint16_t)res1.b + res2.b;
									*op2.reg8=res3.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
									goto aggFlagAB;
									break;
								case 1:       // OR
									res3.b=res1.b | res2.b;
									*op2.reg8=res3.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
									goto aggFlagBZ;
									break;
								case 2:       // ADC
									res3.x=(uint16_t)res1.b + res2.b + _f.Carry;
									*op2.reg8=res3.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
									goto aggFlagAB;
									break;
								case 3:       // SBB
									res3.x=(uint16_t)res1.b - res2.b - _f.Carry;
									*op2.reg8=res3.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
									goto aggFlagSB;
									break;
								case 4:       // AND
									res3.b=res1.b & res2.b;
									*op2.reg8=res3.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
									goto aggFlagBZ;
									break;
								case 5:       // SUB
									res3.x=(uint16_t)res1.b - res2.b;
									*op2.reg8=res3.b;
									if(Pipe1 & 2) {			// sign extend
                    _ah = _al & 0x80 ? 0xff : 0;
										}
									goto aggFlagSB;
									break;
								case 6:       // XOR
									res3.b=res1.b ^ res2.b;
									*op2.reg8=res3.b;
									if(Pipe1 & 2) {			// sign extend
										_ah = _al & 0x80 ? 0xff : 0;
										}
									goto aggFlagBZ;
									break;
								case 7:       // CMP
									res3.x=(uint16_t)res1.b - res2.b;
									if(Pipe1 & 2) {			// sign extend
										//??
										}
									goto aggFlagSB;
									break;
								}
							} 
						else {
      				_ip++;      // imm16...
	            op2.reg16= &regs.r[Pipe2.rm].x;
              res1.x=*op2.reg16;
							if(Pipe1 & 2) 			// sign extend
								op1.mem = (int16_t)Pipe2.b.h;
							else
  							op1.mem=Pipe2.xm.w;   // CONTROLLARE!
//        MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
							res2.x=op1.mem;
							switch(Pipe2.reg) {
			          case 0:       // ADD
									res3.d=(uint32_t)res1.x + res2.x;
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
									goto aggFlagWZ;
									break;
			          case 2:       // ADC
									res3.d=(uint32_t)res1.x + res2.x + _f.Carry;
									*op2.reg16=res3.x;
									goto aggFlagAW;
									break;
			          case 3:       // SBB
									res3.d=(uint32_t)res1.x - res2.x - _f.Carry;
									*op2.reg16=res3.x;
									goto aggFlagSW;
									break;
			          case 4:       // AND
									res3.x=res1.x & res2.x;
									*op2.reg16=res3.x;
#ifdef EXT_80386
									if(Pipe1 & 2) {			// sign extend
										_dx = _ax & 0x8000 ? 0xffff : 0;
										}
#endif
									goto aggFlagWZ;
									break;
			          case 5:       // SUB
									res3.d=(uint32_t)res1.x - res2.x;
									*op2.reg16=res3.x;
									goto aggFlagAW;
									break;
			          case 6:       // XOR
									res3.x=res1.x ^ res2.x;
									*op2.reg16=res3.x;
#ifdef EXT_80386
									if(Pipe1 & 2) {			// sign extend
										_dx = _ax & 0x8000 ? 0xffff : 0;
										}
#endif
									goto aggFlagWZ;
									break;
			          case 7:       // CMP
									res3.d=(uint32_t)res1.x - res2.x;
									goto aggFlagSW;
									break;
								}
							}
            break;
          }
        break;
        
			case 0x84:        // TESTB
			case 0x85:        // TESTW
				_ip++;
				if(Pipe1 & 1)
					op1.reg16= &regs.r[Pipe2.reg].x;
				else
					op1.reg8= Pipe2.b.l & 0x20 ? &regs.r[(Pipe2.b.l >> 3) & 0x3].b.h : &regs.r[(Pipe2.b.l >> 3) & 0x3].b.l;
       
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
            if(!(Pipe1 & 1)) {
              res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
              res2.b=*op1.reg8;
              res3.b=res1.b & res2.b;
							_f.Carry=0; _f.Ovf=0;   // Aux undefined
							goto aggFlagBZ;
							}
						else {
              res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
              res2.x=*op1.reg16;
              res3.x=res1.x & res2.x;
							_f.Carry=0; _f.Ovf=0;   // Aux undefined
							goto aggFlagWZ;
              }
            break;
          case 3:
            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
            if(!(Pipe1 & 1)) {
              res1.b=*op2.reg8;
              res2.b=*op1.reg8;
              res3.b=res1.b & res2.b;
							_f.Carry=0; _f.Ovf=0;   // Aux undefined
							goto aggFlagBZ;
							}
						else {
              res1.x=*op2.reg16;
              res2.x=*op1.reg16;
              res3.x=res1.x & res2.x;
							_f.Carry=0; _f.Ovf=0;   // Aux undefined
							goto aggFlagWZ;
              }
            break;
          }
				break;

			case 0x86:				// XCHG
			case 0x87:
				_ip++;
				if(Pipe1 & 1)
					op1.reg16= &regs.r[Pipe2.reg].x;
				else
					op1.reg8= Pipe2.b.l & 0x20 ? &regs.r[(Pipe2.b.l >> 3) & 0x3].b.h : &regs.r[(Pipe2.b.l >> 3) & 0x3].b.l;
       
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
            if(!(Pipe1 & 1)) {
              res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
              PutValue(MAKE20BITS(*theDs,op2.mem),*op1.reg8);
              *op1.reg8=res1.b;
							}
						else {
              res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
              PutShortValue(MAKE20BITS(*theDs,op2.mem),*op1.reg16);
              *op1.reg16=res1.x;
              }
            break;
          case 3:
            if(!(Pipe1 & 1)) {
              op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
              res1.b=*op2.reg8;
              *op2.reg8=*op1.reg8;
              *op1.reg8=res1.b;
							}
						else {
              op2.reg16= &regs.r[Pipe2.rm].x;
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
        _ip++;
/*2024??*/				if(Pipe1 & 1)
					op1.reg16= &regs.r[Pipe2.reg].x;
				else
					op1.reg8= Pipe2.b.l & 0x20 ? &regs.r[(Pipe2.b.l >> 3) & 0x3].b.h : &regs.r[(Pipe2.b.l >> 3) & 0x3].b.l;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
            switch(Pipe1 & 0x3) {
              case 0:
	              PutValue(MAKE20BITS(*theDs,op2.mem),*op1.reg8);
                break;
              case 1:
	              PutShortValue(MAKE20BITS(*theDs,op2.mem),*op1.reg16);
                break;
              case 2:
                *op1.reg8=GetValue(MAKE20BITS(*theDs,op2.mem));
                break;
              case 3:
                *op1.reg16=GetShortValue(MAKE20BITS(*theDs,op2.mem));
                break;
              }
            break;
          case 3:
						if(Pipe1 & 1) 
							op2.reg16= &regs.r[Pipe2.rm].x;
						else
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
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
				_ip++;
#ifdef EXT_80286
				op2.reg16= &segs.r[(Pipe2.reg].x;
#else
				op2.reg16= &segs.r[(Pipe2.b.l >> 3) & 0x3].x;
#endif
       
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
            PutShortValue(MAKE20BITS(*theDs,op1.mem),*op2.reg16);
            break;
          case 3:
    				op1.reg16= &regs.r[Pipe2.rm].x;
            *op1.reg16=*op2.reg16;
            break;
          }
				break;
        
			case 0x8d:        // LEA
        op1.reg16 = &regs.r[Pipe2.reg].x;
        _ip++;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
    				*op1.reg16 = op2.mem;
            break;
          case 3:
    				op2.reg16= &regs.r[Pipe2.rm].x;
            *op1.reg16=*op2.reg16;
            break;
          }
				break;
        
			case 0x8e:        // MOV SREG,rm16
        _ip++;
#ifdef EXT_80286
				op1.reg16= &segs.r[(Pipe2.reg].x;
#else
				op1.reg16= &segs.r[(Pipe2.b.l >> 3) & 0x3].x;
#endif
       
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
            *op1.reg16=GetShortValue(MAKE20BITS(*theDs,op2.mem));
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
        _ip++;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						POP_STACK(res1.x);
						PutShortValue(MAKE20BITS(*theDs,op2.mem),res1.x);
            break;
          case 3:
            op2.reg16= &regs.r[Pipe2.rm].x;
						POP_STACK(*op2.reg16);
            break;
          }
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
        
			case 0x98:
        _ah = _al & 0x80 ? 0xff : 0;
        break;
        
			case 0x99:
        _dx = _ax & 0x8000 ? 0xffff : 0;
        break;
        
			case 0x9a:      // CALLF
        GetMorePipe(MAKE20BITS(_cs,_ip-1));
        _ip+=4;
				PUSH_STACK(_cs);		// 
				PUSH_STACK(_ip);
				_ip=Pipe2.x.l;
				_cs=Pipe2.x.h;
				break;

   		case 0x9b:
#ifdef EXT_80x87
          status8087,control8087;
            
#endif
				// WAIT se NON c'è 8087!
				break;
        
   		case 0x9c:        // PUSHF
				PUSH_STACK(_f.x);		// 
				break;
        
   		case 0x9d:        // POPF
				POP_STACK(_f.x);		// 
				break;

			case 0x9e:      // SAHF
				_f.x = _ah | (_f.x & 0xff00);
				break;
        
			case 0x9f:      // LAHF
				_ah=_f.x & 0x00ff;
				break;

 			case 0xa0:      // MOV [ofs]
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				_al=GetValue(MAKE20BITS(*theDs,Pipe2.x.l));
        _ip+=2;
				break;
        
 			case 0xa1:      // MOV
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				_ax=GetShortValue(MAKE20BITS(*theDs,Pipe2.x.l));
        _ip+=2;
				break;

 			case 0xa2:      // MOV
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				PutValue(MAKE20BITS(*theDs,Pipe2.x.l),_al);
        _ip+=2;
				break;
        
 			case 0xa3:      // MOV
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				PutShortValue(MAKE20BITS(*theDs,Pipe2.x.l),_ax);
        _ip+=2;
				break;
        
			case 0xa4:      // MOVSB
        // FORSE ds può fare override!
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;	
        PutValue(MAKE20BITS(_es,_di),GetValue(MAKE20BITS(*theDs,_si)));
        if(_f.Dir) {
          _di--;
          _si--;
          }
        else {
          _di++;
          _si++;
          }
        if(inRep) 
          inRep=3;
				break;

			case 0xa5:      // MOVSW
        // FORSE ds può fare override!
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				PutShortValue(MAKE20BITS(_es,_di),GetShortValue(MAKE20BITS(*theDs,_si)));
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          _si-=2;
          }
        else {
          _di+=2;
          _si+=2;
          }
        if(inRep) 
          inRep=3;
				break;
        
			case 0xa6:      // CMPSB
        // FORSE ds può fare override!
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				res3.b= GetValue(MAKE20BITS(_es,_di)) - GetValue(MAKE20BITS(*theDs,_si));
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
        // FORSE ds può fare override!
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
				res3.x= GetShortValue(MAKE20BITS(_es,_di)) - GetShortValue(MAKE20BITS(*theDs,_si));
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
        _f.Carry=0;
        _f.Ovf=0;
				res3.b = _al & Pipe2.b.l; 
        _ip++;
        goto aggFlagBZ;
				break;

			case 0xa9:        // TESTW
        _f.Carry=0;
        _f.Ovf=0;
				res3.x = _ax & Pipe2.x.l;
        _ip+=2;
        goto aggFlagWZ;
				break;
        
			case 0xaa:      // STOSB
				PutValue(MAKE20BITS(_es,_di),_al);
        if(_f.Dir)
          _di--;
        else
          _di++;
        if(inRep) 
          inRep=3;
				break;

			case 0xab:      // STOSW
				PutShortValue(MAKE20BITS(_es,_di),_ax);
        if(_f.Dir)
          _di-=2;   // anche 32bit??
        else
          _di+=2;
        if(inRep) 
          inRep=3;
				break;

			case 0xac:      // LODSB; NO OVERRIDE qua!
				_al=GetValue(MAKE20BITS(_ds,_si));
        if(_f.Dir)
          _si--;
        else
          _si++;
        if(inRep)     // questo ha poco senso qua :)
          inRep=3;
				break;

			case 0xad:      // LODSW
				_ax=GetShortValue(MAKE20BITS(_ds,_si));
        if(_f.Dir)
          _si-=2;   // anche 32bit??
        else
          _si+=2;
        if(inRep) 
          inRep=3;
				break;

			case 0xae:      // SCASB
				res1.b= _al;
        res2.b= GetValue(MAKE20BITS(_es,_di));
				res3.x= (uint16_t)res1.b - res2.b;
        if(_f.Dir)
          _di--;
        else
          _di++;
        goto aggFlagSB;
				break;

			case 0xaf:      // SCASW
				res1.x= _ax;
        res2.x= GetShortValue(MAKE20BITS(_es,_di));
				res3.d= (uint32_t)res1.x - res2.x;
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
//#define WORKING_REG Pipe1 & 0x4 ? regs.r[Pipe1 & 0x3].b.h : regs.r[Pipe1 & 0x3].b.l
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
        
#ifdef EXT_80186
			case 0xc0:      // RCL, RCR, SHL ecc
// usare         immofs ??
        immofs=1;
  			_ip++;
        
        switch(Pipe2.mod) {
          case 2:
    				_ip++;
          case 1:
            _ip++;
          case 0:
            switch(Pipe2.rm) {
              case 0:
                theDs=&_ds; op2.mem=_bx+_si;    // UNSIGNED!
                break;
              case 1:
                theDs=&_ds; op2.mem=_bx+_di;
                break;
              case 2:
                theDs=&_ss; op2.mem=_bp+_si;
                break;
              case 3:
                theDs=&_ss; op2.mem=_bp+_di;
                break;
              case 4:
                theDs=&_ds; op2.mem=_si;
                break;
              case 5:
                theDs=&_ds; op2.mem=_di;
                break;
              case 6:
								if(Pipe2.mod) {
                theDs=&_ss; 
                  op2.mem=_bp;
                  }
                else {
                theDs=&_ds; 
                  op2.mem=Pipe2.xm.w;
                  _ip+=2;
                  immofs+=2;
                  }
                break;
              case 7:
                theDs=&_ds; op2.mem=_bx;
                break;
              }
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						if(segOverride) {
							theDs=&segs.r[segOverride-1].x;
							segOverride=0;
							} 
            // usare COMPUTE_RM_OFS 2024
            
						op1.mem=GetPipe(MAKE20BITS(_cs,_ip++));		// la posizione è variabile... MIGLIORARE!
						res3.b=res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
						i=res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									res3.b |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res3.b & 1;
									res3.b >>= 1;
									if(_f.Carry)
										res3.b |= 0x80;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									res3.b |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.b & 1;
									res3.b >>= 1;
									if(_f1.Carry)
										res3.b |= 0x80;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res3.b & 1;
									res3.b = ((uint8_t)res3.b) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res3.b & 1;
									res3.b = ((int8_t)res3.b) >> 1;
									}
								break;
							}
						if(i==1)
              _f.Ovf=(res3.b ^ *op1.reg8) & 0x80 ? 1 : 0;		// Ovf=1 se il segno è preservato
						PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
            if(i)
              goto aggFlagBZ;
            break;
          case 3:
            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
						op1.mem=Pipe2.b.h;
    				_ip++;      // imm8
						res3.b=res1.b=*op2.reg8;
						i=res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									res3.b |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res3.b & 1;
									res3.b >>= 1;
									if(_f.Carry)
										res3.b |= 0x80;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									res3.b |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.b & 1;
									res3.b >>= 1;
									if(_f1.Carry)
										res3.b |= 0x80;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res3.b & 1;
									res3.b = ((uint8_t)res3.b) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res3.b & 0x80 ? 1 : 0;
									res3.b <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res3.b & 1;
									res3.b = ((int8_t)res3.b) >> 1;
									}
								break;
							}
						if(i==1)
  						_f.Ovf=(res3.b ^ *op2.reg8) & 0x80 ? 1 : 0;		// Ovf=1 se il segno è preservato
						*op2.reg8=res3.b;
						if(i)
              goto aggFlagBZ;
						break;
					}
				break;

			case 0xc1:
// usare         immofs ??
        immofs=1;
				_ip++;
        
        switch(Pipe2.mod) {
          case 2:
    				_ip++;
          case 1:
            _ip++;
          case 0:
            switch(Pipe2.rm) {
              case 0:
                theDs=&_ds; op2.mem=_bx+_si;    // UNSIGNED!
                break;
              case 1:
                theDs=&_ds; op2.mem=_bx+_di;
                break;
              case 2:
                theDs=&_ss; op2.mem=_bp+_si;
                break;
              case 3:
                theDs=&_ss; op2.mem=_bp+_di;
                break;
              case 4:
                theDs=&_ds; op2.mem=_si;
                break;
              case 5:
                theDs=&_ds; op2.mem=_di;
                break;
              case 6:
								if(Pipe2.mod) {
                theDs=&_ss;
                  op2.mem=_bp;
                  }
                else {
                theDs=&_ds; 
                  op2.mem=Pipe2.xm.w;
                  _ip+=2;
                  immofs+=2;
                  }
                break;
              case 7:
                theDs=&_ds; op2.mem=_bx;
                break;
              }
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						if(segOverride) {
							theDs=&segs.r[segOverride-1].x;
							segOverride=0;
							}
            // usare COMPUTE_RM_OFS 2024
            
						op1.mem=GetPipe(MAKE20BITS(_cs,_ip++));		// la posizione è variabile... migliorare!
						res3.x=res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
						i=res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									res3.x |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res3.x & 1;
									res3.x >>= 1;
									if(_f.Carry)
										res3.x |= 0x8000;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									res3.x |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.x & 1;
									res3.x >>= 1;
									if(_f1.Carry)
										res3.x |= 0x8000;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res3.x & 1;
									res3.x = ((uint16_t)res3.x) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res3.x & 1;
									res3.x = ((int16_t)res3.x) >> 1;
									}
								break;
							}
						if(i==1)
  						_f.Ovf=(res3.x ^ *op1.reg16) & 0x8000 ? 1 : 0;		// Ovf=1 se il segno è preservato
						PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
            if(i)
              goto aggFlagWZ;
            break;
          case 3:
            op2.reg16= &regs.r[Pipe2.rm].x;
						op1.mem=Pipe2.b.h;
    				_ip++;      // imm8
						res3.x=res1.x=*op2.reg16;
						i=res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									res3.x |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res3.x & 1;
									res3.x >>= 1;
									if(_f.Carry)
										res3.x |= 0x8000;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									res3.x |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res3.x & 1;
									res3.x >>= 1;
									if(_f1.Carry)
										res3.x |= 0x8000;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res3.x & 1;
									res3.x = ((uint16_t)res3.x) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res3.x & 0x8000 ? 1 : 0;
									res3.x <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res3.x & 1;
									res3.x = ((int16_t)res3.x) >> 1;
									}
								break;
							}
						if(i==1)
  						_f.Ovf=(res3.x ^ *op2.reg16) & 0x8000 ? 1 : 0;		// Ovf=1 se il segno è preservato
						*op2.reg16=res3.x;
            if(i)
              goto aggFlagWZ;
						break;
					}
				break;
#endif
        
#ifndef EXT_80186     // bah, così dicono..
			case 0xc0:
#endif

			case 0xc2:      // RETN
				POP_STACK(_ip);
        _sp+=Pipe2.x.l;
				break;
        
#ifndef EXT_80186     // bah, così dicono..
			case 0xc1:
#endif

			case 0xc3:      // RET
				POP_STACK(_ip);
				break;

			case 0xc4:				//LDS
			case 0xc5:				//LES
				_ip++;
				op1.reg16= &regs.r[Pipe2.reg].x;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						*op1.reg16=GetShortValue(MAKE20BITS(*theDs,op2.mem));
						if(Pipe1 & 1)
							_es=GetShortValue(MAKE20BITS(*theDs,op2.mem+2));
						else
							_ds=GetShortValue(MAKE20BITS(*theDs,op2.mem+2));
            break;
          }
				break;

 			case 0xc6:				//MOV imm8...
 			case 0xc7:				//MOV imm16
        GetMorePipe(MAKE20BITS(_cs,_ip-1));   // andrebbe fatto solo se necessario... RISISTEMARE!
        immofs=1;
				_ip++;
        
        switch(Pipe2.mod) {
          case 2:
    				_ip++;
          case 1:
            _ip++;
          case 0:
            switch(Pipe2.rm) {
              case 0:
                theDs=&_ds; op2.mem=_bx+_si;    // UNSIGNED!
                break;
              case 1:
                theDs=&_ds; op2.mem=_bx+_di;
                break;
              case 2:
                theDs=&_ss; op2.mem=_bp+_si;
                break;
              case 3:
                theDs=&_ss; op2.mem=_bp+_di;
                break;
              case 4:
                theDs=&_ds; op2.mem=_si;
                break;
              case 5:
                theDs=&_ds; op2.mem=_di;
                break;
              case 6:
								if(Pipe2.mod) {
                theDs=&_ss;
                  op2.mem=_bp;
                  }
                else {
                theDs=&_ds;
                  op2.mem=Pipe2.xm.w;
                  immofs+=2;
                  _ip+=2;
                  }
                break;
              case 7:
                theDs=&_ds; op2.mem=_bx;
                break;
              }
            if(Pipe2.mod == 1) {
              op2.mem+=(int8_t)Pipe2.b.h;
              immofs++;
              }
            else if(Pipe2.mod == 2) {
              op2.mem+=(int16_t)Pipe2.xm.w;
              immofs+=2;
              }
						if(segOverride) {
							theDs=&segs.r[segOverride-1].x;
							segOverride=0;
							}
            // usare COMPUTE_RM_OFS 2024
            
//            if(immofs>3)
//              GetMorePipe(MAKE20BITS(_cs,_ip-1));
            if(!(Pipe1 & 1)) {
              _ip++;
              op1.mem=Pipe2.bd[immofs];
              PutValue(MAKE20BITS(*theDs,op2.mem),op1.mem);
							}
						else {
              _ip+=2;
              op1.mem=MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
              PutShortValue(MAKE20BITS(*theDs,op2.mem),op1.mem);
							}
            break;
          case 3:		// non è chiaro se c'è qua...
            if(!(Pipe1 & 1)) {
	            op1.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
              op2.mem=Pipe2.bd[immofs];
              *op2.reg8=op2.mem;
							}
						else {
							op1.reg16= &regs.r[Pipe2.rm].x;
							GetPipe(MAKE20BITS(_cs,_ip++));		// mi sposto avanti per il 6°byte; //verificare...
              op2.mem=MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
              *op2.reg16=op2.mem;
              }
            break;
          }
				break;


#ifdef EXT_80186
			case 0xc8:        //ENTER
				PUSH_STACK(_bp);
        _bp=_sp;
        _sp-=Pipe2.x.l;
        switch(Pipe2.b.u) {
          case 0:
            break;
          case 1:
            PUSH_STACK(_bp);
            break;
          default:
            for(i=0; i<Pipe2.b.u; i++) {
              _bp-=2;
              PUSH_STACK(_bp);
              }
            break;
          }
        _ip+=3;
        break;
#endif
        
#ifdef EXT_80186
			case 0xc9:        // LEAVE
        _sp=_bp;
				POP_STACK(_bp);
        break;
#endif
        
#ifndef EXT_80186     // bah, così dicono..
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
    /*IF ((vector_number « 2) + 3) is not within IDT limit
        THEN #GP; FI; 
    IF stack not large enough for a 6-byte return information
        THEN #SS; FI;*/
        i=Pipe2.b.l;
        
				_ip++;

do_irq:
        _f.Trap=0;
        if(i < 0x100) {
          if(_sp>=6) {
            PUSH_STACK(_f.x);
            PUSH_STACK(_cs);
    /*- all interrupts except the internal CPU exceptions push the
	  flags and the CS:IP of the next instruction onto the stack.
	  CPU exception interrupts are similar but push the CS:IP of the
	  causal instruction.	8086/88 divide exceptions are different,
	  they return to the instruction following the division*/
            PUSH_STACK(_ip);
            _f.IF=0;
            _ip=GetShortValue(i *4);
            _cs=GetShortValue((i *4) +2);
            }
          else {
// tecnicamente è un eccezione, non trap...            _f.Trap=1;
            }
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
				POP_STACK(_ip);
				POP_STACK(_cs);
				POP_STACK(_f.x);
				break;

			case 0xd0:      // RCL, RCR ecc 8
			case 0xd2:
				_ip++;
				if(Pipe1 & 2)
					op1.mem=_cl;
				else
					op1.mem=1;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
						res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									res1.b |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res1.b & 1;
									res1.b >>= 1;
									if(_f.Carry)
										res1.b |= 0x80;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									res1.b |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.b & 1;
									res1.b >>= 1;
									if(_f1.Carry)
										res1.b |= 0x80;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res1.b & 1;
									res1.b = ((uint8_t)res1.b) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res1.b & 1;
									res1.b = ((int8_t)res1.b) >> 1;
									}
								break;
							}
						_f.Ovf=(res1.b ^ GetValue(MAKE20BITS(*theDs,op2.mem)) ) & 0x80 ? 1 : 0;		// Ovf=1 se il segno è preservato
						PutValue(MAKE20BITS(*theDs,op2.mem),res1.b);
            break;
          case 3:
            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
						res1.b=*op2.reg8;
						res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									res1.b |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res1.b & 1;
									res1.b >>= 1;
									if(_f.Carry)
										res1.b |= 0x80;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									res1.x |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.b & 1;
									res1.b >>= 1;
									if(_f1.Carry)
										res1.b |= 0x80;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res1.b & 1;
									res1.b = ((uint8_t)res1.b) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res1.b & 0x80 ? 1 : 0;
									res1.b <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res1.b & 1;
									res1.b = ((int8_t)res1.b) >> 1;
									}
								break;
							}
						_f.Ovf=(res1.b ^ *op2.reg8) & 0x80 ? 1 : 0;		// Ovf=1 se il segno è preservato
						*op2.reg8=res1.b;
						break;
					}
				break;

			case 0xd1:				// RCL, RCR ecc 16
			case 0xd3:
				_ip++;
				if(Pipe1 & 2)
					op1.mem=_cl;
				else
					op1.mem=1;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
						res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									res1.x |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res1.x & 1;
									res1.x >>= 1;
									if(_f.Carry)
										res1.x |= 0x8000;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									res1.x |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.x & 1;
									res1.x >>= 1;
									if(_f1.Carry)
										res1.x |= 0x8000;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res1.x & 1;
									res1.x = ((uint16_t)res1.x) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res1.x & 1;
									res1.x = ((int16_t)res1.x) >> 1;
									}
								break;
							}
						_f.Ovf=(res1.x ^ GetShortValue(MAKE20BITS(*theDs,op2.mem)) ) & 0x8000 ? 1 : 0;		// Ovf=1 se il segno è preservato
						PutShortValue(MAKE20BITS(*theDs,op2.mem),res1.x);
            break;
          case 3:
            op2.reg16= &regs.r[Pipe2.rm].x;
						res1.x=*op2.reg16;
						res2.b=op1.mem;
						switch(Pipe2.reg) {
							case 0:       // ROL
								while(res2.b--) {
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									res1.x |= _f.Carry;
									}
								break;
							case 1:       // ROR
								while(res2.b--) {
									_f.Carry=res1.x & 1;
									res1.x >>= 1;
									if(_f.Carry)
										res1.x |= 0x8000;
									}
								break;
							case 2:       // RCL
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									res1.x |= _f1.Carry;
									}
								break;
							case 3:       // RCR
								while(res2.b--) {
									_f1.Carry=_f.Carry;
									_f.Carry=res1.x & 1;
									res1.x >>= 1;
									if(_f1.Carry)
										res1.x |= 0x8000;
									}
								break;
							case 4:       // SHL/SAL
								while(res2.b--) {
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									}
								break;
							case 5:       // SHR
								while(res2.b--) {
									_f.Carry=res1.x & 1;
									res1.x = ((uint16_t)res1.x) >> 1;
									}
								break;
							case 6:       // SAL/SHL ... bah mi sembrano identiche!
								while(res2.b--) {
									_f.Carry=res1.x & 0x8000 ? 1 : 0;
									res1.x <<= 1;
									}
								break;
							case 7:       // SAR
								while(res2.b--) {
									_f.Carry=res1.x & 1;
									res1.x = ((int16_t)res1.x) >> 1;
									}
								break;
							}
						_f.Ovf=(res1.x ^ *op2.reg16) & 0x8000 ? 1 : 0;		// Ovf=1 se il segno è preservato
						*op2.reg16=res1.x;
						break;
					}
				break;

			case 0xd4:      // AAM
        i=_al;
        _ah=i / Pipe2.b.l;    // 10 fisso in teoria, ma si può usare genericamente come base per la conversione...
        _al=i % Pipe2.b.l;    //
        res3.x=_ax;
        _ip++;
        goto aggFlagWZ;			// verificare flag! word o byte?
				break;
        
			case 0xd5:      // AAD
        i=_ax;
//        _ah=i;
        _al=res3.b=LOBYTE(i)+(HIBYTE(i) * Pipe2.b.l);    // v. AAM
        _ah=0;
        _ip++;
        goto aggFlagBZ;			// 
				break;
        
			case 0xd6:      // dice che non è documentata...
				_al=_f.Carry ? 0xff : 0;
				break;

			case 0xd7:      // XLAT
        if(segOverride) {
          theDs=&segs.r[segOverride-1].x;
          segOverride=0;
          }
        else
          theDs=&_ds;
        _al=GetValue(MAKE20BITS(*theDs,_bx+_al));
        break;
        
			case 0xd8:
			case 0xd9:
			case 0xda:
			case 0xdb:
			case 0xdc:
			case 0xdd:
			case 0xde:
			case 0xdf:
        // coprocessore matematico
#ifdef EXT_80x87
          status8087,control8087;
        if(Pipe2.mod==2)      // 
          _ip+=2;
        else
          _ip++;
          
#else            
        if(Pipe2.mod==2)      // faccio "nulla" :)   https://stackoverflow.com/questions/42543905/what-are-8086-esc-instruction-opcodes
          _ip+=2;
        else
          _ip++;
#endif
        break;
        
			case 0xe0:      // LOOPNZ
				_ip++;
        if(--_cx && _f.Zero)
          _ip+=(int8_t)Pipe2.b.l;
				break;
        
			case 0xe1:      // LOOPZ
				_ip++;
        if(--_cx && !_f.Zero)
          _ip+=(int8_t)Pipe2.b.l;
				break;
        
			case 0xe2:      // LOOP
				_ip++;
        if(--_cx)
          _ip+=(int8_t)Pipe2.b.l;
				break;

			case 0xe3:      // JCXZ
				_ip++;
        if(!_cx)
          _ip+=(int8_t)Pipe2.b.l;
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
        GetMorePipe(MAKE20BITS(_cs,_ip-1));
				_ip=Pipe2.x.l;
				_cs=Pipe2.x.h;
				break;

			case 0xeb:      // JMP rel8
				_ip+=(int8_t)Pipe2.b.l+1;
				break;
			
			case 0xec:        // INB
				_al=InValue(_dx);
				break;

			case 0xed:        // INW
				_ax=InShortValue(_dx);
				break;

			case 0xee:        // OUTB
				OutValue(_dx,_al);
				break;

			case 0xef:        // OUTW
				OutShortValue(_dx,_ax);
				break;

			case 0xf0:				// LOCK
				break;

			case 0xf1:
Trap:
				i=1 /*INT1*/ ;
				goto do_irq;
				break;

			case 0xf2:				//REPNZ/REPNE; entrambi vengono mutati in 3 nelle istruzioni che non usano Z
        inRep=0x102;
				break;

			case 0xf3:				//REPZ/REPE
        inRep=0x101;
				break;

			case 0xf4:
			  DoHalt=1;
				break;

			case 0xf5:				// CMC
				_f.Carry = !_f.Carry;
				break;

			case 0xf6:        // altre MUL ecc //	; mul on V20 does not affect the zero flag,   but on an 8088 the zero flag is used
			case 0xf7:
				_ip++;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						if(!(Pipe1 & 1)) {
              res2.b=GetValue(MAKE20BITS(*theDs,op2.mem));
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
    							GetPipe(MAKE20BITS(_cs,_ip-2));		// mi sposto per il 6°byte; //migliorare...
                  _ip++;
        					res1.b = Pipe2.b.h;
									res3.b = res1.b & res2.b;
									goto aggFlagBZ;
									break;
								case 2:			// NOT
									res3.b = ~res2.b;
									PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									break;
								case 3:			// NEG
									res3.x = -res2.b;			// x per flag...
									PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
									goto aggFlagSB;
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									break;
								case 4:       // MUL8
                  op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = (uint16_t)res1.b*res2.b;
									_ax=res3.x;			// non è bello ma...		
									if(!_ah)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
								case 5:       // IMUL8
                  op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = (int16_t)res1.b*(int8_t)res2.b;
									_ax=res3.x;			// non è bello ma...		
									if(!_ah)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
								case 6:       // DIV8
									res3.b = (uint16_t)_ax / res2.b;
									_ah = _ax % res2.b;			// non è bello ma...
									_al = res3.b;
									break;
								case 7:       // IDIV8
									res3.b = (int16_t)_ax / (int8_t)res2.b;
									_ah =  (int16_t)_ax % (int8_t)res2.b;
									_al = res3.b;
									break;
								}
							}
						else {
              res2.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
    							GetPipe(MAKE20BITS(_cs,_ip-2));		// mi sposto per il 6°byte; //migliorare...
                  _ip+=2;
        					op1.mem = Pipe2.xm.w;
									res3.x = res1.x & res2.x;
									goto aggFlagWZ;
									break;
								case 2:			// NOT
									res3.x = ~res2.x;
									PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
									break;
								case 3:			// NEG
									res3.x = -res2.x;			// x per flag...
									PutShortValue(MAKE20BITS(*theDs,op2.mem),res3.x);
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									goto aggFlagSW;
									break;
								case 4:       // MUL16
                  op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = (uint32_t)res1.x*res2.x;
									_ax = LOWORD(res3.d);			// 
									_dx = HIWORD(res3.d);			// non è bello ma...		
									if(!_dx)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
								case 5:       // IMUL16
                  op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = (int32_t)res1.x*(int16_t)res2.x;
									_ax = LOWORD(res3.d);			// 
									_dx = HIWORD(res3.d);			// non è bello ma...		
									if(!_dx)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
								case 6:       // DIV16
									res3.x = (uint16_t)MAKELONG(_ax,_dx) / res2.x;
									_dx = (uint16_t)MAKELONG(_ax,_dx) % res2.x;			// non è bello ma...
		              _ax = res3.x;
									break;
								case 7:       // IDIV16
									res3.x = (int32_t)MAKELONG(_ax,_dx) / (int16_t)res2.x;
									_dx = (int32_t)MAKELONG(_ax,_dx) % (int16_t)res2.x;
		              _ax = res3.x;
									break;
								}
              }
            break;
          case 3:
						if(!(Pipe1 & 1)) {
	            op2.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
              res2.b=*op2.reg8;
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
                  _ip++;
        					op1.mem = Pipe2.b.h;
                  res1.b=op1.mem;
									res3.b = res1.b & res2.b;
									goto aggFlagBZ;
									break;
								case 2:			// NOT
                  op1.reg8=op2.reg8;
									res3.b = ~res2.b;
									*op1.reg8 = res3.b;
									break;
								case 3:			// NEG
                  op1.reg8=op2.reg8;
									res3.x = -res2.b;			// x per flag...
									*op1.reg8 = res3.b;
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									goto aggFlagSB;
									break;
								case 4:       // MUL8
        					op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = (uint16_t)res1.b*res2.b;
									_al=LOBYTE(res3.x);			// 
									_ah=HIBYTE(res3.x);			// 
									if(!_ah)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
								case 5:       // IMUL8
        					op1.reg8= &_al;
                  res1.b=*op1.reg8;
									res3.x = (int16_t)res1.b*(int8_t)res2.b;
									_al=LOBYTE(res3.x);			// 
									_ah=HIBYTE(res3.x);			// 
									if(!_ah)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
        // DIV NON aggiorna flag!
								case 6:       // DIV8
									res3.b = _ax / res2.b;
									_ah = _ax % res2.b;			// non è bello ma...
									_al = res3.b;
									break;
								case 7:       // IDIV8
									res3.b = (int16_t)_ax / (int8_t)res2.b;
									_ah =  (int16_t)_ax % (int8_t)res2.b;
									_al = res3.b;
									break;
								}
							} 
						else {
	            op2.reg16= &regs.r[Pipe2.rm].x;
              res2.x=*op2.reg16;
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
                  _ip+=2;
        					op1.mem = Pipe2.xm.w;
                  res1.x=op1.mem;
									res3.x = res1.x & res2.x;
									goto aggFlagWZ;
									break;
								case 2:			// NOT
                  op1.reg16=op2.reg16;
									res3.x = ~res2.x;
		              *op1.reg16 = res3.x;
									break;
								case 3:			// NEG
                  op1.reg16=op2.reg16;
									res3.x = -res2.x;			// x per flag...
		              *op1.reg16 = res3.x;
                  //The CF flag set to 0 if the source operand is 0; otherwise it is set to 1. The OF, SF, ZF, AF, and PF flags are set according to the result. 
									goto aggFlagSW;
									break;
								case 4:       // MUL16
        					op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = (uint32_t)res1.x*res2.x;
									_ax=LOWORD(res3.d);			// 
									_dx=HIWORD(res3.d);			// non è bello ma...		
									if(!_dx)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
								case 5:       // IMUL16
        					op1.reg16= &_ax;
                  res1.x=*op1.reg16;
									res3.d = (int32_t)res1.x*(int16_t)res2.x;
									_ax=LOWORD(res3.d);			// 
									_dx=HIWORD(res3.d);			// non è bello ma...		
									if(!_dx)				// verificare...
										_f.Carry=_f.Ovf=0;
									else
										_f.Carry=_f.Ovf=1;
									break;
        // DIV NON aggiorna flag!
								case 6:       // DIV16
									res3.x = MAKELONG(_ax,_dx) / res2.x;
									_dx = MAKELONG(_ax,_dx) % res2.x;			// non è bello ma...
		              _ax = res3.x;
									break;
								case 7:       // IDIV16
									res3.x = (int32_t)MAKELONG(_ax,_dx) / (int16_t)res2.x;
									_dx =  (int32_t)MAKELONG(_ax,_dx) % (int16_t)res2.x;
									_ax = res3.x;
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
				_ip++;
				COMPUTE_RM_OFS
            if(Pipe2.mod == 1)
              op2.mem+=(int8_t)Pipe2.b.h;
            else if(Pipe2.mod == 2)
              op2.mem+=(int16_t)Pipe2.xm.w;
						if(!(Pipe1 & 1)) {
              res1.b=GetValue(MAKE20BITS(*theDs,op2.mem));
							switch(Pipe2.reg) {
								case 0:       // INC
									res3.b = res1.b+1;
									PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                	_f.Ovf= res3.b == 0x80 ? 1 : 0; //V = 1 if x=7FH before, else 0
									goto aggFlagABA;
									break;
								case 1:       // DEC
									res3.b = res1.b-1;
									PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                  _f.Ovf= res3.b == 0x7f ? 1 : 0; //V = 1 if x=80H before, else 0
									goto aggFlagSBA;
									break;
								}
							}
						else {
              res1.x=GetShortValue(MAKE20BITS(*theDs,op2.mem));
							switch(Pipe2.reg) {
								case 0:       // INC
									res3.x = res1.x+1;
									PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                	_f.Ovf= res3.x == 0x8000 ? 1 : 0; //V = 1 if x=7FFFH before, else 0
									goto aggFlagAWA;
									break;
								case 1:       // DEC
									res3.x = res1.x-1;
									PutValue(MAKE20BITS(*theDs,op2.mem),res3.b);
                  _f.Ovf= res3.x == 0x7fff ? 1 : 0; //V = 1 if x=8000H before, else 0
									goto aggFlagSWA;
									break;
								case 2:			// CALL DWORD PTR
									PUSH_STACK(_ip+2);
									_ip=res1.x;
									break;
								case 3:			// CALL FAR DWORD PTR
									PUSH_STACK(_cs);
									PUSH_STACK(_ip+2);
									_ip=res1.x;
									_cs=GetShortValue(MAKE20BITS(*theDs,op2.mem+2));
									break;
								case 4:       // JMP DWORD PTR jmp [100]
									_ip=res1.x;
									break;
								case 5:       // JMP FAR DWORD PTR
									_ip=res1.x;
									_cs=GetShortValue(MAKE20BITS(*theDs,op2.mem+2));
									break;
								case 6:       // PUSH
									PUSH_STACK(GetShortValue(MAKE20BITS(*theDs,op2.mem)));
									break;
								}
              }
            break;
          case 3:
						if(!(Pipe1 & 1)) {
	            op1.reg8= Pipe2.b.l & 0x4 ? &regs.r[Pipe2.b.l & 0x3].b.h : &regs.r[Pipe2.b.l & 0x3].b.l;
              res1.b=*op1.reg8;
							switch(Pipe2.reg) {
								case 0:       // INC
									res3.b = res1.b+1;
									*op1.reg8=res3.b;
                	_f.Ovf= res3.b == 0x80 ? 1 : 0; //V = 1 if x=7FH before, else 0
									goto aggFlagABA;
									break;
								case 1:       // DEC
									res3.b = res1.b-1;
									*op1.reg8=res3.b;
                  _f.Ovf= res3.b == 0x7f ? 1 : 0; //V = 1 if x=80H before, else 0
									goto aggFlagSBA;
									break;
								}
							} 
						else {
	            op1.reg16= &regs.r[Pipe2.rm].x;
              res1.x=*op1.reg16;
							switch(Pipe2.reg) {
								case 0:       // INC
									res3.x = res1.x+1;
									*op1.reg16=res3.x;
                	_f.Ovf= res3.x == 0x8000 ? 1 : 0; //V = 1 if x=7FFFH before, else 0
									goto aggFlagAWA;
									break;
								case 1:       // DEC
									res3.x = res1.x-1;
									*op1.reg16=res3.x;
                	_f.Ovf= res3.x == 0x7fff ? 1 : 0; //V = 1 if x=8000H before, else 0
									goto aggFlagSWA;
									break;
								case 2:			// CALL DWORD PTR
									PUSH_STACK(_ip+2);
									_ip=*op1.reg16;
									break;
								case 3:			// CALL FAR uintDWORD32_t PTR
									PUSH_STACK(_cs);
									PUSH_STACK(_ip+2);
									_ip=*op1.reg16;			// VERIFICARE COME SI FA! o forse non c'è
									_cs=*op1.reg16+2;
									break;
								case 4:       // JMP DWORD PTR jmp [100]
									_ip=*op1.reg16;
									break;
								case 5:       // JMP FAR DWORD PTR
									_ip=*op1.reg16;			// VERIFICARE COME SI FA! o forse non c'è
									_cs=*op1.reg16+2;
									break;
								case 6:       // PUSH
									PUSH_STACK(*op1.reg16);
									break;
								}
              }
            break;
          }
				break;
        
			default:
#ifdef EXT_80186
				i=6 /*UD*/ ;
				goto do_irq;
#endif
//				wsprintf(myBuf,"Istruzione sconosciuta a %04x: %02x",_pc-1,GetValue(_pc-1));
//				SetWindowText(hStatusWnd,myBuf);
				break;
			}
    
    if(_f.Trap) {
      i=0xf1;
      goto Trap;   // eseguire IRQ 0xf1??
      }

#ifdef EXT_80286
    if(_sp <= 2)
      // causare INT 0C
#endif

		} while(!fExit);
	}


#if 0
BYTE computeRM(void) {    // restituisce 1 se in memoria, 0 se registro
  
	_ip++;
	if(Pipe1 & 1)
		op1.reg16= &regs.r[(Pipe2.b.l >> 3) & 0x7].x;
	else
		op1.reg8= Pipe2.b.l & 0x20 ? &regs.r[(Pipe2.b.l >> 3) & 0x3].b.h : &regs.r[(Pipe2.b.l >> 3) & 0x3].b.l;
 
  switch(Pipe2.b.l & 0xc0) {
    case 0x80:
    	_ip++;
    case 0x40:
      _ip++;
    case 0x00:
      switch(Pipe2.b.l & 7) {
        case 0:
          theDs=&_ds; op2.mem=_bx+_si;    // SIGNED!
          break;
        case 1:
          theDs=&_ds; op2.mem=_bx+_di;
          break;
        case 2:
          theDs=&_ss; op2.mem=_bp+_si;
          break;
        case 3:
          theDs=&_ss; op2.mem=_bp+_di;
          break;
        case 4:
          theDs=&_ds; op2.mem=_si;
          break;
        case 5:
          theDs=&_ds; op2.mem=_di;
          break;
        case 6:
					if(Pipe2.b.l & 0xc0) {
          theDs=&_ss;
            op2.mem=_bp;
            }
          else {
          theDs=&_ds;
            op2.mem=Pipe2.xm.w;
            _ip+=2;
            }
          break;
        case 7:
          theDs=&_ds; op2.mem=_bx;
          break;
        }
      if((Pipe2.b.l & 0xc0) == 0x40)
        op2.mem+=(signed char)Pipe2.b.h;
      else if((Pipe2.b.l & 0xc0) == 0x80)
        op2.mem+=Pipe2.xm.w;
			if(segOverride) {
				theDs=&segs.r[segOverride-1].x;
				segOverride=0;
				}
      return 1;
      break;
    case 0xc0:
      return 0;
      break;
    }
  }

void computeRS8(void) {
  
  switch((Pipe2.b.l >> 3) & 7) {
    case 0:       // ROL
      while(res2.b--) {
        _f.Carry=res1.b & 0x80 ? 1 : 0;
        res1.b <<= 1;
        res1.b |= _f.Carry;
        }
      break;
    case 1:       // ROR
      while(res2.b--) {
        _f.Carry=res1.b & 1;
        res1.b >>= 1;
        if(_f.Carry)
          res1.b |= 0x80;
        }
      break;
    case 2:       // RCL
      while(res2.b--) {
        _f1.Carry=_f.Carry;
        _f.Carry=res1.b & 0x80 ? 1 : 0;
        res1.b <<= 1;
        res1.x |= _f1.Carry;
        }
      break;
    case 3:       // RCR
      while(res2.b--) {
        _f1.Carry=_f.Carry;
        _f.Carry=res1.b & 1;
        res1.b >>= 1;
        if(_f1.Carry)
          res1.b |= 0x80;
        }
      break;
    case 4:       // SHL/SAL
      while(res2.b--) {
        _f.Carry=res1.b & 0x80 ? 1 : 0;
        res1.b <<= 1;
        }
      break;
    case 5:       // SHR
      while(res2.b--) {
        _f.Carry=res1.b & 1;
        res1.b = ((unsigned char)res1.b) >> 1;
        }
      break;
    case 6:       // SAL/SHL ... bah mi sembrano identiche!
      while(res2.b--) {
        _f.Carry=res1.b & 0x80 ? 1 : 0;
        res1.b <<= 1;
        }
      break;
    case 7:       // SAR
      while(res2.b--) {
        _f.Carry=res1.b & 1;
        res1.b = ((signed char)res1.b) >> 1;
        }
      break;
    }
  }

void computeRS16(void) {
  
  switch((Pipe2.b.l >> 3) & 7) {
    case 0:       // ROL
      while(res2.b--) {
        _f.Carry=res1.x & 0x8000 ? 1 : 0;
        res1.x <<= 1;
        res1.x |= _f.Carry;
        }
      break;
    case 1:       // ROR
      while(res2.b--) {
        _f.Carry=res1.x & 1;
        res1.x >>= 1;
        if(_f.Carry)
          res1.x |= 0x8000;
        }
      break;
    case 2:       // RCL
      while(res2.b--) {
        _f1.Carry=_f.Carry;
        _f.Carry=res1.x & 0x8000 ? 1 : 0;
        res1.x <<= 1;
        res1.x |= _f1.Carry;
        }
      break;
    case 3:       // RCR
      while(res2.b--) {
        _f1.Carry=_f.Carry;
        _f.Carry=res1.x & 1;
        res1.x >>= 1;
        if(_f1.Carry)
          res1.x |= 0x8000;
        }
      break;
    case 4:       // SHL/SAL
      while(res2.b--) {
        _f.Carry=res1.x & 0x8000 ? 1 : 0;
        res1.x <<= 1;
        }
      break;
    case 5:       // SHR
      while(res2.b--) {
        _f.Carry=res1.x & 1;
        res1.x = ((unsigned short int)res1.x) >> 1;
        }
      break;
    case 6:       // SAL/SHL ... bah mi sembrano identiche!
      while(res2.b--) {
        _f.Carry=res1.x & 0x8000 ? 1 : 0;
        res1.x <<= 1;
        }
      break;
    case 7:       // SAR
      while(res2.b--) {
        _f.Carry=res1.x & 1;
        res1.x = ((signed short int)res1.x) >> 1;
        }
      break;
    }
  _f.Ovf=(res1.x ^ *op2.reg16) & 0x8000 ? 1 : 0;		// Ovf=1 se il segno è preservato

  }
#endif
