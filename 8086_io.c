// GD/C 12/7/2024 porcodio le nuvole morte allitaglia leucemia ai figli dei carabinieri



#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
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



#define RAM_SIZE 0x70000L
#define RAM_START 0L
uint8_t ram_seg[RAM_SIZE];
#define ROM_SIZE 8192				// BIOS
#define ROM_END 0x100000L
//uint8_t rom_seg[ROM_SIZE];			bios_rom
uint8_t disk_ram[0x400];
#define CGA_SIZE 16384				// video
uint8_t CGAram[CGA_SIZE /* 640*200/8 320*200/2 */];
#define CGA_BASE 0xB8000L
uint8_t CGAreg[16];  // https://www.seasip.info/VintagePC/cga.html
uint8_t MDAreg[16];  // https://www.seasip.info/VintagePC/mda.html
//  http://www.oldskool.org/guides/oldonnew/resources/cgatech.txt
uint8_t i8237RegR[32],i8237RegW[32],i8237Reg2R[32],i8237Reg2W[32];
uint8_t i8259RegR[2],i8259RegW[2],i8259Reg2R[2],i8259Reg2W[2];
uint8_t i8253RegR[4],i8253RegW[4],i8253Mode[3],i8253ModeSel;
uint16_t i8253TimerR[3],i8253TimerW[3];
uint8_t i8250Reg[8],i8250Regalt[3];
uint8_t i8042RegR[2],i8042RegW[2],KBRAM[32],KBControl,KBStatus;
uint8_t MachineFlags=0b00000000;		// b0-1=Speaker on; b2-3=TURBO PC; b4-5=Parity errors
uint8_t i6845RegR[18],i6845RegW[18];
uint8_t i146818RegR[2],i146818RegW[2],i146818RAM[64];
uint8_t FloppyContrRegR[8],FloppyContrRegW[8],FloppyFIFO[16],FloppyFIFOPtr;
uint8_t Keyboard[1]={0};
uint8_t ColdReset=1;
uint8_t ExtIRQNum=0;
extern uint8_t Pipe1;
extern union PIPE Pipe2;



uint8_t GetValue(uint32_t t) {
	register uint8_t i;

	if(t >= (ROM_END-ROM_SIZE)) {
		i=bios_rom[t-(ROM_END-ROM_SIZE)];
		}
#ifdef ROM_BASIC
	else if(t >= 0xF6000) {
		i=IBMBASIC[t-0xF6000];
		}
#endif
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		i=CGAram[t-CGA_BASE];
    }
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
  
	return i;
	}

uint8_t InValue(uint16_t t) {
	register uint8_t i;

  switch(t >> 4) {
    case 0:        // 00-1f DMA 8237 controller
    case 1: 
      t &= 0x1f;
      i=i8237RegR[t];
    
      break;
    case 2:         // 20-21 PIC 8259 controller
      t &= 0x1;
      i=i8259RegR[t];
    
      break;
    case 4:         // 40-43 PIT 8253/4 controller (incl. speaker)
		// https://en.wikipedia.org/wiki/Intel_8253
      t &= 0x3;
      switch(t) {
        case 0:
          switch((i8253Mode[0] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 1:
              i=HIBYTE(i8253RegR[0]);  //OCCHIO non deve mai essere >2
              i8253ModeSel = 0;
              break;
            case 2:
              i=LOBYTE(i8253RegR[0]);  //OCCHIO non deve mai essere >2
              i8253ModeSel = 0;
              break;
            case 3:
              if(!i8253ModeSel)
                i=LOBYTE(i8253RegR[0]);  //OCCHIO non deve mai essere >2
              else
                i=HIBYTE(i8253RegR[0]);
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          
          // VERIFICARE!!
          break;
        case 1:
          switch((i8253Mode[1] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 1:
              i=HIBYTE(i8253RegR[1]);
              i8253ModeSel = 0;
              break;
            case 2:
              i=LOBYTE(i8253RegR[1]);
              i8253ModeSel = 0;
              break;
            case 3:
              if(!i8253ModeSel)
                i=LOBYTE(i8253RegR[1]);
              else
                i=HIBYTE(i8253RegR[1]);
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          break;
        case 2:
          switch((i8253Mode[2] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 1:
              i=HIBYTE(i8253RegR[2]);
              i8253ModeSel = 0;
              break;
            case 2:
              i=LOBYTE(i8253RegR[2]);
              i8253ModeSel = 0;
              break;
            case 3:
              if(!i8253ModeSel)
                i=LOBYTE(i8253RegR[2]);
              else
                i=HIBYTE(i8253RegR[2]);
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          break;
        case 3:
          i=i8253RegR[3];
          break;
        }
      i=i8253RegR[t];
      break;
    case 6:        // 60-6F 8042 keyboard; speaker; config byte    https://www.phatcode.net/res/223/files/html/Chapter_20/CH20-2.html
      t &= 0xf;
      switch(t) {
        case 0:
          if(i8042RegW[1]==0xAA) {      // self test
            i8042RegR[0]=0x55;
            }
          else if(i8042RegW[1]==0xAB) {      // diagnostics
            i8042RegR[0]=0b00000000;
            }
          else if(i8042RegW[1]==0xAE) {      // enable
            KBStatus |= 0b00010000;
            i8042RegR[0]=0b00000000;
            }
          else if(i8042RegW[1]==0x20) {
            i8042RegR[0]=KBControl;
            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {   // boh, non risulta...
            i8042RegR[0]=KBRAM[i8042RegW[1] & 0x1f];
            }
          else if(i8042RegW[1]==0xC0) {
            i8042RegR[0]=KBStatus;
            }
          else if(i8042RegW[1]==0xD0) {
            i8042RegR[0]=KBControl;
            }
          else if(i8042RegW[1]==0xD1) {
            }
          else if(i8042RegW[1]==0xD2) {
            }
          else if(i8042RegW[1]==0xEE) {     //echo
            i8042RegR[0]=0xee;
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
            }
          else {
            i8042RegR[0]=Keyboard[0];
            }
          KBStatus &= ~0b00000001;
          i8042RegW[1]=0;     // direi, così la/le prossima lettura restituisce il tasto...

          i=i8042RegR[0];
          break;
        case 1:     // machine flags... (da V20)
          i=MachineFlags;
          break;
        case 2:     // machine settings... (da V20)
//          i=0b01101100;			// bits 2,3 memory size (64K bytes); b7:6 #floppies; b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25
          // e anche NMI status, b7-b6=parity error; coprocessor
          i=0b01101010;   // così va! pd ... forse dipende dal valore scritto in 0x62 anche se non è chiaro...
          break;
        case 4:
          i=i8042RegR[1]=KBStatus; // 
          break;
        }
      break;
    case 7:        //   70-7F CMOS RAM/RTC (Real Time Clock  MC146818)
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
              // 6 è day of week...
            case 7:
              i146818RegR[1]=currentDate.mday;
              break;
            case 8:
              i146818RegR[1]=currentDate.mon;
              break;
            case 9:
              i146818RegR[1]=currentDate.year;
              break;
            case 12:
              i146818RegR[1]=i146818RAM[12] = 0;   // flag IRQ
              break;
            default:      // qua ci sono i 4 registri e poi la RAM
              i146818RegR[1]=i146818RAM[i146818RegW[0] & 0x3f];
              break;
            }
          i=i146818RegR[1];
          break;
        }
      break;
  // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
    case 0xa:        // PIC 8259 controller #2
      t &= 0x1;
      i=i8259Reg2R[t];
      break;
    case 0xf:        // F0 = coprocessor
      break;
  // 108 diag display opp. 80 !
  // 278-378 printer
  // 300 = POST diag
  // 2f8-3f8 serial
    case 0x1f:      // 1f0-1ff   // hard disc 0
      break;

    case 0x20:    //200-207   joystick
      t &= 1;
      switch(t) {
        case 1:
          i=0xff;     // no joystick...
          i=0xfe;     // sì joystick :)
          break;
        }
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
      
    case 0x37:    // 378-37a  LPT1
      t &= 3;
      switch(t) {
        case 0:
          i=(PORTE & 0xff) & ~0b00010000;
          i |= PORTGbits.RG6 ? 0b00010000 : 0;   // perché LATE4 è led...
          break;
        case 1:   // finire
          i=PORTB & 0xff;
          break;
        case 2:    // qua c'è lo strobe
          i = PORTGbits.RG7;      // finire
          break;
        }
      break;
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
        case 0xc:
          // printer port...
          break;
        case 0x8:
          // Mode
          break;
        case 0xa:
          // Status
          break;
        default:
          break;
        }
      i=MDAreg[t];
      break;
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
        default:
          break;
        }
      i=CGAreg[t];
      break;
    case 0x3f:    // 3f4-3f5: floppy disk controller; 3f8-3ff  COM1
      t &= 0xf;
      switch(t) {
        case 2:   // command port MOTEN3 MOTEN2 MOTEN1 MOTEN0 DMAEN nRESET DRIVESEL1 DRIVESEL0
					FloppyContrRegR[2];
          i=0x00;     // NOT ready
          break;
        case 3:   // 
					FloppyContrRegR[3];
          i=0x00;     // NOT ready
          break;
        case 4:   // command status RQMDIO NONDMA CMDBUSY DRV3BUSY DRV2BUSY DRV1BUSY DRV0BUSY
					FloppyContrRegR[4] |= 0b10000000;		// DIO ready for data exchange
					i=FloppyContrRegR[4];
//          i=0x00;     // 0x10=cmd in progress...
          break;
        case 5:   // data port
//					if(FloppyContrRegR[5] & 0b01000000)		// direction
					i=FloppyContrRegR[5];
          break;
        case 7:   // disk change port, b7
					FloppyContrRegR[7]=0b00000000;
					i=FloppyContrRegR[7];
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
            i=ReadUART1();
            }
          break;
        case 1+8:
          if(i8250Reg[3] & 0x80) {
            i=i8250Regalt[1];
            }
          else {
            if(DataRdyUART1())
              i8250Reg[1] |= 1;
            else
              i8250Reg[1] &= ~1;
            if(!BusyUART1())
              i8250Reg[1] |= 2;
            else
              i8250Reg[1] &= ~2;
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
            if(DataRdyUART1())
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
              i8250Reg[5] &= ~0x40;
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
    }
  
	return i;
	}

uint16_t InShortValue(uint16_t t) {
	register uint8_t i;

	return i;
	}

uint16_t GetShortValue(uint32_t t) {
	register uint16_t i;

	if(t >= (ROM_END-ROM_SIZE)) {
		t-=ROM_END-ROM_SIZE;
		i=MAKEWORD(bios_rom[t],bios_rom[t+1]);
		}
#ifdef ROM_BASIC
	else if(t >= 0xF6000) {
		t-=0xF6000;
		i=MAKEWORD(IBMBASIC[t],IBMBASIC[t+1]);
		}
#endif
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		i=MAKEWORD(CGAram[t],CGAram[t+1]);
    }
#ifdef RAM_DOS
	else if(t >= 0x7c000 && t < 0x80000) {
		i=MAKEWORD(disk_ram[t-0x7c000],disk_ram[t-0x7c000+1]);
		}
//		i=iosys[t-0x7c000];
#endif
	else if(t < RAM_SIZE) {
		i=MAKEWORD(ram_seg[t],ram_seg[t+1]);
		}
// else _f.Trap=1?? v. anche eccezione PIC
  
	return i;
	}


#ifdef EXT_80386
uint32_t GetIntValue(uint32_t t) {
	register uint32_t i;

	if(t >= (ROM_END-ROM_SIZE)) {
		t-=ROM_END-ROM_SIZE;
		i=MAKELONG(MAKEWORD(bios_rom[t],bios_rom[t+1]),MAKEWORD(bios_rom[t+2],bios_rom[t+3]));
		}
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		i=MAKELONG(MAKEWORD(CGAram[t],CGAram[t+1]),CGAram(bios_rom[t+2],CGAram[t+3]));
    }
	else if(t < RAM_SIZE) {
		i=MAKELONG(MAKEWORD(ram_seg[t],ram_seg[t+1]),MAKEWORD(ram_seg[t+2],ram_seg[t+3]));
		}
	return i;
	}
#endif

uint8_t GetPipe(uint32_t t) {

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
	else if(t >= 0x7c000 && t < 0x80000) {
		t-=0x7c000;
	  Pipe1=disk_ram[t++];
		Pipe2.b.l=disk_ram[t++];
		Pipe2.b.h=disk_ram[t++];
		Pipe2.b.u=disk_ram[t];
//		Pipe2.b.u2=disk_ram[t];
//		i=iosys[t-0x7c00];
    }
	else if(t < RAM_SIZE) {
	  Pipe1=ram_seg[t++];
		Pipe2.b.l=ram_seg[t++];
		Pipe2.b.h=ram_seg[t++];
		Pipe2.b.u=ram_seg[t];
//		Pipe2.b.u2=ram_seg[t];
		}
	return Pipe1;
	}

uint8_t GetMorePipe(uint32_t t) {

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
	else if(t < RAM_SIZE) {
    t+=4;
		Pipe2.bd[3]=ram_seg[t++];
		Pipe2.bd[4]=ram_seg[t++];
		Pipe2.bd[5]=ram_seg[t];
		}
	return Pipe1;
	}

void PutValue(uint32_t t,uint8_t t1) {
	register uint16_t i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);
#ifdef RAM_DOS
	if(t >= 0x7c000 && t < 0x80000) {
		disk_ram[t-0x7c000]=t1;
		} else 
#endif
	if(t>=CGA_BASE && t < (CGA_BASE+CGA_SIZE)) {
		t-=CGA_BASE;
		CGAram[t]=t1;
    }
	else if(t < RAM_SIZE) {
	  ram_seg[t]=t1;
		}
// else _f.Trap=1?? v. anche eccezione PIC
	}

void PutShortValue(uint32_t t,uint16_t t1) {
	register uint16_t i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

#ifdef RAM_DOS
	if(t >= 0x7c000 && t < 0x80000) {
		disk_ram[t-0x7c000]=t1;
		} else 
#endif
	if(t>=CGA_BASE && t<(CGA_BASE+CGA_SIZE)) {
		t-=CGA_BASE;
		CGAram[t++]=LOBYTE(t1);
		CGAram[t]=HIBYTE(t1);
    }
	else if(t < RAM_SIZE) {
	  ram_seg[t++]=LOBYTE(t1);
	  ram_seg[t]=HIBYTE(t1);
		}
// else _f.Trap=1?? v. anche eccezione PIC

	}


#ifdef EXT_80386
void PutIntValue(uint32_t t,uint32_t t1) {
	register uint16_t i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

	if(t>=CGA_BASE && t<(CGA_BASE+CGA_SIZE)) {
		t-=CGA_BASE;
		CGAram[t++]=LOBYTE(LOWORD(t1));
		CGAram[t++]=HIBYTE(LOWORD(t1));
		CGAram[t++]=LOBYTE(HIWORD(t1));
		CGAram[t]=HIBYTE(HIWORD(t1));
    }
	else if(t < RAM_SIZE) {
	  ram_seg[t++]=LOBYTE(LOWORD(t1));
	  ram_seg[t++]=HIBYTE(LOWORD(t1));
	  ram_seg[t++]=LOBYTE(HIWORD(t1));
	  ram_seg[t]=HIBYTE(HIWORD(t1));
		}
	}
#endif

void OutValue(uint16_t t,uint8_t t1) {      // https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses
	register uint16_t i;
  int j;

  switch(t >> 4) {
    case 0:        // 00-1f DMA 8237 controller
    case 1: 
      t &= 0x1f;
      i8237RegW[t]=t1;

      break;
    case 2:         // 20-21 PIC 8259 controller
//https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
      t &= 0x1;
      switch(t) {
        case 0:     // 0x20 per EOI interrupt, 0xb per "chiedere" quale... subito dopo c'è lettura a 0x20
          i8259RegW[0]=t1;      // 
          if(t1==0x20)
            i8259RegR[0]=0;
          // pulire QUA i flag TMRIRQ ecc??
  //o toglierli del tutto e usare i bit di 8259
          break;
        case 1:
          i8259RegW[1]=t1;      // 0xe8?? 0xbc su bios3.1 (kbd, tod, floppy)
          //cmq FINIRE! ci sono più comandi a seconda dei bit... all inizio manda 0x13 a 20 e 0x8 0x9 0xff a 21; poi per pulire un IRQ si manda il valore letto da 0x20 qua
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
      switch(t) {
        case 0:
          switch((i8253Mode[0] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 2:
              i8253TimerW[0]=MAKEWORD(LOBYTE(i8253TimerW[0]),t1);
              i8253ModeSel = 0;
              break;
            case 1:
              i8253TimerW[0]=MAKEWORD(t1,HIBYTE(i8253TimerW[0]));
              i8253ModeSel = 0;
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[0]=MAKEWORD(LOBYTE(i8253TimerW[0]),t1);
              else
                i8253TimerW[0]=MAKEWORD(t1,HIBYTE(i8253TimerW[0]));
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          i8253RegW[0]=t1;
          PR4=i8253TimerW[0] ? i8253TimerW[0] : 65535;
          break;
        case 1:
          switch((i8253Mode[1] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 2:
              i8253TimerW[1]=MAKEWORD(LOBYTE(i8253TimerW[1]),t1); 
              i8253ModeSel = 0;
              break;
            case 1:
              i8253TimerW[1]=MAKEWORD(t1,HIBYTE(i8253TimerW[1]));
              i8253ModeSel = 0;
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[1]=MAKEWORD(LOBYTE(i8253TimerW[1]),t1);
              else
                i8253TimerW[1]=MAKEWORD(t1,HIBYTE(i8253TimerW[1]));
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          i8253RegW[1]=t1;
          PR5=i8253TimerW[1] ? i8253TimerW[1] : 65535;;
          break;
        case 2:
          switch((i8253Mode[2] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 2:
              i8253TimerW[2]=MAKEWORD(LOBYTE(i8253TimerW[2]),t1);  
              i8253ModeSel = 0;
              break;
            case 1:
              i8253TimerW[2]=MAKEWORD(t1,HIBYTE(i8253TimerW[2]));
              i8253ModeSel = 0;
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[2]=MAKEWORD(LOBYTE(i8253TimerW[2]),t1);
              else
                i8253TimerW[2]=MAKEWORD(t1,HIBYTE(i8253TimerW[2]));
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          i8253RegW[2]=t1;
          PR2=i8253TimerW[2] ? i8253TimerW[2] : 65535;;
          break;
        case 3:
          if((t1 >> 6) < 3)
            i8253Mode[t1 >> 6]=t1 & 0x3f;
          i8253RegR[3]=i8253RegW[3]=t1;
          i8253ModeSel=0;
          break;
        }
      break;
    case 6:        // 60-6F 8042 keyboard; speaker; config byte
      t &= 0xf;
      switch(t) {
        case 0:     // solo bit7? controllare
          if(i8042RegW[1]==0xAA) {
            Keyboard[0]=0;
            }
          else if(i8042RegW[1]==0x60) {
            KBControl=t1;
            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {  // boh... 2024
            //KBRAM[i8042RegW[1] & 0x1f]
            }
          else if(i8042RegW[1]==0xC0) {
            }
          else if(i8042RegW[1]==0xD0) {
            }
          else if(i8042RegW[1]==0xD1) {
            }
          else if(i8042RegW[1]==0xD2) {
            }
          else if(i8042RegW[1]==0xED) {     //led 
            LED1 = t1 & 1;
            LED2 = t1 & 2 ? 1 : 0;
            LED3 = t1 & 4 ? 1 : 0;
            }
          else if(i8042RegW[1]==0xEE) {     //echo
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
            }
          i8042RegW[0]=t1;     // 
          i8042RegW[1]=0;     // direi, così pulisco
          break;
        case 1:     // brutalmente, per buzzer!  e machine flags... (da V20)
             /*PB1 0 turn off speaker
             1 enable speaker data
             PB0 0 turn off timer 2
             1 turn on timer 2, gate speaker with    square wave*/
          j=400  /* USARE i8253TimerR[2] , vale 0x528 */;      // 980Hz 5/12/19
          if(t1 & 2) {
            if(t1 & 1) {
              PR2 = j;		 // 
  #ifdef ST7735
              OC1RS = j/2;		 // 
  #endif
  #ifdef ILI9341
              OC7RS = j/2;		 // 
  #endif
              }
            else {
              PR2 = j;
  #ifdef ST7735
              OC1RS = j;		 // bah
  #endif
  #ifdef ILI9341
              OC7RS = j;		 // bah
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
            }
          MachineFlags=t1;
          break;
        case 2:     // machine settings... (da V20), NMI
          // scrive 0x08 per "leggere switch"... ma non si capisce cos'altro cmq... anche AD
          Nop();
          break;
        case 4:     // 
          i8042RegW[1]=t1;
          KBStatus |= 0b00000010;
          if(i8042RegW[1]==0xAA) {
            KBControl |= 0b00000100;
            Keyboard[0]=0;
            }
          else if(i8042RegW[1]==0x20 || i8042RegW[1]==0x60) { // r/w control
            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {
            //KBRAM[i8042RegW[1] & 0x1f]
            }
          else if(i8042RegW[1]==0xC0) {
            }
          else if(i8042RegW[1]==0xD0) {
            }
          else if(i8042RegW[1]==0xD1) {
            }
          else if(i8042RegW[1]==0xD2) {
            }
          else if(i8042RegW[1]==0xED) {     //led 
            }
          else if(i8042RegW[1]==0xEE) {     //echo
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
          }
        KBStatus &= ~0b00000010;    // vabbe' :)
        break;
        }
      break;
    case 7:        //   70-7F CMOS RAM/RTC (Real Time Clock  MC146818)
      // SECONDO PCXTBIOS, il clock è a 240 o 2c0 o 340 ed è mappato con tutti i registri... pare!
      t &= 0x1;
      switch(t) {
        case 0:
          i146818RegR[0]=i146818RegW[0]=t1;
  //        time_t;
          break;
        case 1:     // il bit 7 attiva/disattiva NMI
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
            // 6 è day of week...
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
            default:      // in effetti ci sono altri 4 registri... RAM da 14 in poi 
  writeRegRTC:            
              i146818RAM[i146818RegW[0] & 0x3f] = t1;
              break;
            }
          break;
        }
      break;
    case 0xa:        // PIC 8259 controller #2
      t &= 0x1;
      switch(t) {
        case 0:     // 0x20 per EOI interrupt
          i8259Reg2W[0]=t1;
          if(t1==0x20)
            i8259Reg2R[0]=0;
          // pulire QUA i flag KBDIRQ ecc?? 
  //				o toglierli del tutto e usare i bit di 8259
          break;
        case 1:
          i8259Reg2W[1]=t1;
          break;
        }

      break;
    case 8:
      if(t==0x80 /*t>=0x108 && t<=0x10f*/) {      // diag display, uso per led qua!
    // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
        t &= 0x7;   //[per ora tolti xché usati per debug! 
        LED1 = t1 & 1;
        LED2 = t1 & 2 ? 1 : 0;
        LED3 = t1 & 4 ? 1 : 0;
        }
      break;
    case 0xf:  // F0 = coprocessor
      break;

  // 108 diag display opp. 80
  // 278-378 printer
  // 300 = POST diag
  // 2f8-3f8 serial
    case 0x1f:      // 1f0-1ff   // hard disc 0
      break;
    
    case 0x20:    //200-207   joystick
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
    case 0x37:    // 378-37a  LPT1
      t &= 3;
      switch(t) {
        case 0:
//  #ifndef ILI9341
          LATE = (LATE & 0xff00) | t1;
          LATGbits.LATG6 = t1 & 0b00010000 ? 1 : 0;   // perché LATE4 è led...
//  #endif
          break;
        case 2:    // qua c'è lo strobe
          LATGbits.LATG7 = t1 & 0b00000001 ? 1 : 0;   // strobe, FARE gli altri :)
          break;
        }
      break;
    
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
          MDAreg[5]=i6845RegW[MDAreg[4] % 18]=t1;
          break;
        case 0xc:
          // printer port...
          break;
        case 0x8:
          // Mode
          break;
        case 0xa:
          // Status
          break;
        default:
          MDAreg[t]=t1;
          break;
        }
      break;
    case 0x3d:    // 3d0-3df  CGA (forse son solo 8 ma non si capisce, 3d8 3df   http://nerdlypleasures.blogspot.com/2016/05/ibms-cga-hardware-explained.html
      t &= 0xf;
      switch(t) {
        case 0xb:
          CGAreg[0xa] &= ~0b0110;      // pulisco trigger light pen
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
    
    case 0x3f:    // 3f8-3ff  COM1; 3f4-5 NEC floppy
      t &= 0xf;
      switch(t) {
        case 2:   // DOR command port: MOTEN3 MOTEN2 MOTEN1 MOTEN0 DMAEN nRESET DRIVESEL1 DRIVESEL0
					FloppyContrRegR[2]=FloppyContrRegW[2]=t1;
          break;
        case 3:   // TDR 
					FloppyContrRegR[3]=FloppyContrRegW[3]=t1;
          break;
        case 4:   // DSR data rate select		S/W RESET  POWER DOWN  0  PRE-COMP2  PRE-COMP1  PRE-COMP0  DRATESEL1  DRATESEL0
					FloppyContrRegR[4] |= 0b00010000;		// BUSY
					FloppyContrRegW[4]=t1;
					switch(t1) {
						case 3:			// specify
							break;
						case 4:			// request status about FDD
							break;
						case 7:			// recalibrate
							break;
						case 8:			// request status after seek
							break;
						case 15:			// seek (goto cylinder
							break;
						case 0x10:	// version
							break;
						case 0x13:			// configure
							break;
						default:		// invalid, 
							FloppyFIFO[0];
							if((t1 & 0b101) == 0b101) {		// write data
								}
							else if((t1 & 0b1001) == 0b1001) {		// write deleted data
								}
							else if((t1 & 0b110) == 0b110) {		// read data
								}
							else if((t1 & 0b1100) == 0b1100) {		// read deleted data
								}
							else if((t1 & 0b10110) == 0b10110) {		// verify data
								}
							else if((t1 & 0b1111) == 0b1111) {		// relative seek
								}
							else
								FloppyContrRegR[4] |= 0b10000000;		// 
							break;
						}
          if(!(i8259RegW[1] & 0x40)) 
            i8259RegR[0] |= 0x40;		// simulo IRQ...
					if(FloppyContrRegW[4] & 0b10000000)		// RESET si autopulisce
						FloppyContrRegW[4] &= ~0b10000000;		// 
					FloppyContrRegR[4] &= ~0b00010000;		// non BUSY :)
          break;
        case 5:   // data port
					FloppyContrRegW[5]=t1;
          break;
        case 7:   // data rate sel port, b1-b0
					FloppyContrRegW[7]=t1;
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
            uint32_t baudRateDivider;
            i8250Regalt[0]=t;
  set_uart1:
            if(MAKEWORD(i8250Regalt[0],i8250Regalt[1]) != 0) {
              baudRateDivider = (GetPeripheralClock()/(4*MAKEWORD(i8250Regalt[0],i8250Regalt[1]))-1)
                /*i8250Regalt[2]*/;
              U6BRG=baudRateDivider;
              }
            }
          else {
            i8250Reg[0]=t1;
            WriteUART1(t1);
            }
          break;
        case 1+8:
          if(i8250Reg[3] & 0x80) {
            i8250Regalt[1]=t;
            goto set_uart1;
            }
          else {
            i8250Reg[1]=t1;
            }
          break;
        case 2+8:
          i8250Reg[2]=t1;
          break;
        case 3+8:
          i8250Reg[3]=t1;
          break;
        case 4+8:
          i8250Reg[4]=t1;
          break;
        case 5+8:
          if(i8250Reg[3] & 0x80) {
            i8250Regalt[2]=t;
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

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

	}
