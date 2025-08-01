// GD/C 12/7/2024 porcodio le nuvole morte allitaglia leucemia ai figli dei carabinieri -> 2025
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

#define UNIMPLEMENTED_MEMORY_VALUE 0x6868		//:)		



uint8_t ram_seg[RAM_SIZE];
//uint8_t rom_seg[ROM_SIZE];			bios_rom
#ifdef RAM_DOS
uint8_t disk_ram[0x400];
#endif
uint16_t VICRaster=0;
#ifdef CGA
#define CGA_BASE 0xB8000L
#define CGA_SIZE 16384				// video
#endif
#ifdef MDA
#define CGA_BASE 0xB0000L
#define CGA_SIZE 65536L				// video
#endif
uint8_t CGAram[CGA_SIZE /* 640*200/8 320*200/2 */];		// anche MDA :)
#ifdef VGA
#define VGA_BIOS_BASE 0xC0000L					// https://flint.cs.yale.edu/feng/cos/resources/BIOS/mem.htm
uint8_t VGABios[32768];
uint8_t VGAram[640*480*(24/8)];
#endif
uint8_t CGAreg[18];  // https://www.seasip.info/VintagePC/cga.html
uint8_t MDAreg[18];  // https://www.seasip.info/VintagePC/mda.html
//  http://www.oldskool.org/guides/oldonnew/resources/cgatech.txt
uint8_t i8237RegR[16],i8237RegW[16],i8237Reg2R[16],i8237Reg2W[16],
	i8237FF=0,i8237FF2=0,
	i8237Mode[4],i8237Mode2[4];
uint16_t i8237DMAAddr[4],i8237DMALen[4],i8237DMAAddr2[4],i8237DMALen2[4];
uint16_t i8237DMACurAddr[4],i8237DMACurLen[4],i8237DMACurAddr2[4],i8237DMACurLen2[4];
uint8_t DMApage[8];
uint8_t i8259RegR[2],i8259RegW[2],i8259ICW[4],i8259ICWSel,i8259IMR,i8259IRR,i8259ISR
#ifdef PCAT
	,i8259Reg2R[2],i8259Reg2W[2],i8259ICW2[4],i8259ICW2Sel,i8259IMR2,i8259IRR2,i8259ISR2
#endif
	;
uint8_t i8255RegR[4],i8255RegW[4];
uint8_t i8253RegR[4],i8253RegW[4],i8253Mode[3],i8253ModeSel;	// uso MSB di Mode come "OUT" e b6 come "reload" (serve anche un "loaded" flag, credo, per one shot ecc
uint16_t i8253TimerR[3],i8253TimerW[3];
uint8_t i8250Reg[8],i8250Regalt[3];
uint8_t i8042RegR[2],i8042RegW[2],KBRAM[32],
KBCommand=0b01010000,
/* |7|6|5|4|3|2|1|0|	8042 Command Byte
		| | | | | | | `---- 1=enable output register full interrupt
		|	| | | | | `----- should be 0
		| | | | | `------ 1=set status register system, 0=clear
		| | | | `------- 1=override keyboard inhibit, 0=allow inhibit
		| | | `-------- disable keyboard I/O by driving clock line low
		| | `--------- disable auxiliary device, drives clock line low
		| `---------- IBM scancode translation 0=AT, 1=PC/XT
		`----------- reserved, should be 0*/
KBStatus;  
/*|7|6|5|4|3|2|1|0|  8042 Status Register
	 | | | | | | | `---- output register (60h) has data for system
	 | | | | | | `----- input register (60h/64h) has data for 8042
	 | | | | | `------ system flag (set to 0 after power on reset)
	 | | | | `------- data in input register is command (1) or data (0)
	 | | | `-------- 1=keyboard enabled, 0=keyboard disabled (via switch)
	 | | `--------- 1=transmit timeout (data transmit not complete)
	 | `---------- 1=receive timeout (data transmit not complete)
	 `----------- 1=even parity rec'd, 0=odd parity rec'd (should be odd)*/
uint8_t MachineFlags=0b00000000;		// b0-1=Speaker on; b2-3=TURBO PC (o select); b4-5=Parity/I/O errors su TINYBIOS; b66= 0:KC clk low; b7= 0 enable KB    https://wiki.osdev.org/Non_Maskable_Interrupt
// v. cmq i8255RegR[2]
uint8_t MachineSettings=0b00101101;		// b0=floppy (0=s�); b1=FPU; bits 2,3 memory size (64K bytes); b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25; b7:6 #floppies-1
		// ma su IBM5160 b0=1 SEMPRE dice!
uint8_t i6845RegR[18],i6845RegW[18];
uint8_t i146818RegR[2],i146818RegW[2],i146818RAM[64];
uint8_t FloppyContrRegR[2],FloppyContrRegW[2],FloppyFIFO[16],FloppyFIFOPtrR,FloppyFIFOPtrW; // https://www.ardent-tool.com/floppy/Floppy_Programming.html#FDC_Registers
BYTE floppyState[4]={0x80,0x80,0,0} /* b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent*/,
  floppyHead[4]={0,0,0,0},floppyTrack[4]={0,0,0,0},floppySector[4]={0,0,0,0},
	floppyTimer /*simula attivit� floppy*/;
uint8_t HDContrRegR[4],HDContrRegW[4],HDFIFO[8],HDFIFOPtrR,HDFIFOPtrW, // 
  hdControl=0,hdState=0,hdHead=0,hdCylinder=0,hdSector=0;
#define HD_SECTORS_PER_CYLINDER 40

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
static uint8_t sectorData[512];

extern const uint8_t *MSDOS_DISK[2];
const unsigned char *msdosDisk;
uint32_t getDiscSector(uint8_t);
uint8_t *encodeDiscSector(uint8_t);
uint8_t *encodeHDiscSector(uint8_t);

BYTE mouseState=255;
BYTE COMDataEmuFifo[5];
BYTE COMDataEmuFifoCnt;
uint8_t Keyboard[1]={0};
uint8_t ColdReset=1;
uint8_t ExtIRQNum=0;
uint8_t LCDdirty;
extern uint8_t Pipe1;
extern union PIPE Pipe2;



uint8_t GetValue(uint32_t t) {
	register uint8_t i;

	if(t >= (ROM_END-ROM_SIZE)) {
		i=bios_rom[t-(ROM_END-ROM_SIZE)];
		}
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		i=bios_rom_opt[t-ROM_START2];
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE) {
		i=HDbios[t-ROM_HD_BASE];
		}
#endif
#ifdef ROM_BASIC
	else if(t >= 0xF6000) {
		i=IBMBASIC[t-0xF6000];
		}
#endif
#ifdef CGA_BASE
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		i=CGAram[t-CGA_BASE];
    }
#endif
#ifdef RAM_DOS
	else if(t >= 0x7c000 && t < 0x80000) {
		i=disk_ram[t-0x7c000];
//		i=iosys[t-0x7c00];
		}
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		i=VGABios[t-VGA_BIOS_BASE];
    }
#endif
	else if(t < RAM_SIZE) {
		i=ram_seg[t];
		}
// else _f.Trap=1?? v. anche eccezione PIC
	else
		i=(BYTE)UNIMPLEMENTED_MEMORY_VALUE;
  
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
				case 8:			// Status
          /*
					if(!(i8237RegR[8] & 0b00000010)) {			// controller enable
						i8237RegR[8] |= ((1 << 0) << 4);  // request??...
						if(!(i8237RegR[15] & (1 << 0)))		// mask...
							i8237RegR[8] |= 1;		// simulo attivit� ch0 (refresh... (serve a glabios o d� POST error 400h, E3BA
						}*/
					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					break;
				case 0xd:			// Reset
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
      t &= 0x1;
      switch(t) {
        case 0:
		      i=i8259RegR[0];
					i8259ISR;
//					i8259ICWSel=1;
          break;
        case 1:
// no...					i=i8259ICW[i8259ICWSel];
//					i8259ICWSel++;
//					i8259ICWSel %= 4;
					if(i8259RegW[0] == 0b00000011)
						i=i8259RegW[1];
					else 

					// se no si blocca... PCXTBIOS
						i=i8259RegR[1];
					i8259IMR;
          break;
        }
      break;

    case 4:         // 40-43 PIT 8253/4 controller (incl. speaker)
		// https://en.wikipedia.org/wiki/Intel_8253
      t &= 0x3;
      switch(t) {
        case 0:
              if(!i8253ModeSel) {
	              i8253RegR[0]=LOBYTE(i8253TimerR[0]);
                i=LOBYTE(i8253RegR[0]);  //OCCHIO non deve mai essere >2
								}
              else {
	              i8253RegR[0]=HIBYTE(i8253TimerR[0]);
                i=HIBYTE(i8253RegR[0]);
								}
              i8253ModeSel++;
              i8253ModeSel &= 1;
          break;
        case 1:
              if(!i8253ModeSel) {
	              i8253RegR[1]=LOBYTE(i8253TimerR[1]);
                i=LOBYTE(i8253RegR[0]);  //OCCHIO non deve mai essere >2
								}
              else {
	              i8253RegR[1]=HIBYTE(i8253TimerR[1]);
                i=HIBYTE(i8253RegR[0]);
								}
              i8253ModeSel++;
              i8253ModeSel &= 1;
          break;
        case 2:
              if(!i8253ModeSel) {
	              i8253RegR[2]=LOBYTE(i8253TimerR[2]);
                i=LOBYTE(i8253RegR[2]);  //OCCHIO non deve mai essere >2
								}
              else {
	              i8253RegR[2]=HIBYTE(i8253TimerR[2]);
                i=HIBYTE(i8253RegR[2]);
								}
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
        case 3:
          i=i8253RegR[3];
          break;
        }
      i=i8253RegR[t];
      break;

    case 6:        // 60-6F 8042/8255 keyboard; speaker; config byte    https://www.phatcode.net/res/223/files/html/Chapter_20/CH20-2.html
      t &= 0xf;
      switch(t) {
        case 0:
          KBStatus &= ~0b00000010;
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
            i8042RegR[0]=KBCommand;
            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {   // boh, non risulta...
            i8042RegR[0]=KBRAM[i8042RegW[1] & 0x1f];
            }
          else if(i8042RegW[1]==0xC0) {
            i8042RegR[0]=KBStatus;
            }
          else if(i8042RegW[1]==0xD0) {
            i8042RegR[0]=KBCommand;
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

//					o decidere in base a data/command in KBCommand?

          else {
						if(KBStatus & 0b00000001)		// SOLO se c'� stato davvero un tasto! (altrimenti siamo dopo reset o clock ENable
							i8042RegR[0]=Keyboard[0];
            }
          KBStatus &= ~0b00000001;
          i8042RegW[1]=0;     // direi, cos� la/le prossima lettura restituisce il tasto...

					i8255RegR[0]=i8042RegR[0];
          i=i8042RegR[0];
					i8042RegR[0]=Keyboard[0]=0;		// si svuota! GLAbios...
          break;
        case 1:     // (machine flags... (da V20)
//					i8255RegR[1]=MachineFlags;
          i=i8255RegR[1];
          break;
        case 2:     // machine settings... (da V20)
//          i=0b01101100;			// bits 2,3 memory size (64K bytes); b7:6 #floppies; b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25
					i8255RegR[2]=MachineSettings;


					// 5160-5150  https://www.minuszerodegrees.net/5160/diff/5160_to_5150_8255.htm
					if(i8255RegW[1] & 4)				// 5150
						i8255RegR[2]=MachineSettings & 0xf;
					else
						i8255RegR[2]=(MachineSettings >> 4) & 1;
					if(i8255RegW[1] & 8)				// 5160
						i8255RegR[2]=MachineSettings >> 4;
					else
						i8255RegR[2]=MachineSettings & 0xf;




					i8255RegR[2] |= 0 << 4;		// b4=monitor cassette/speaker, b5=monitor timer ch2, b6=RAM parity err. on exp board, b7=RAM parity err. on MB


          i=i8255RegR[2];   // cos� va! pd ... forse dipende dal valore scritto in 0x62 anche se non � chiaro...
          break;
        case 3:
          i=i8255RegR[3];
          break;
        case 4:
          i=i8042RegR[1]=KBStatus; // 
          break;
        }
      break;
    case 7:        //   70-7F CMOS RAM/RTC (Real Time Clock  MC146818)
			// https://web.archive.org/web/20160314084340/https://www.bioscentral.com/misc/cmosmap.htm
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
              // in mezzo c'� Alarm...
            case 2:
              i146818RegR[1]=currentTime.min;
              break;
            case 4:
              i146818RegR[1]=currentTime.hour;
              break;
              // 6 � day of week...
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
              i146818RegR[1]=0b00100110;			// dividers
              break;
            case 11:
              i146818RegR[1]=0b00000010;			// 12/24: 24
              break;
            case 12:
              i146818RegR[1]=i146818RAM[12] = 0;   // flag IRQ
              break;
            case 13:
              i146818RegR[1]=i146818RAM[13];
              break;
            case 14:
//              i146818RegR[1]=0b00000000;			// diag in GLATick...
//              break;
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
				}
      break;

  // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
    case 0xa:        // PIC 8259 controller #2 su AT; porta machine setting su XT (occhio a volte non leggibile)
#ifdef PCAT
      t &= 0x1;
      i=i8259Reg2R[t];
#else
i=t;  //DEBUG
#endif
      break;

		case 0xc:		// DMA 2	(second Direct Memory Access controller 8237)  https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses

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
          i=0xf0;     // s� joystick :)
          break;
        }
      break;
      
    case 0x22:    //220-223   Sound Blaster; -22F se Pro
      break;
      
    case 0x24:    // RTC SECONDO PCXTBIOS, il clock � a 240 o 2c0 o 340 ed � mappato con tutti i registri... pare!
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
      
    case 0x32:    // 320- (hard disk su XT	http://www.techhelpmanual.com/898-xt_hard_disk_ports.html  https://forum.vcfed.org/index.php?threads/ibm-xt-i-o-addresses.1237247/		
      t &= 3;
      switch(t) {
				BYTE disk;
				int n,addr;
        case 0:			// READ DATA
          break;
        case 1:     // READ HARDWARE STATUS
					/*7 6 5		4		3		2		1		0
						d d IRQ DRQ BSY C/D I/O REQ*/
					switch(HDContrRegW[0] & 0b11100000) {		// i 3 bit alti sono class, gli altri il comando
						case 0b00000000:
							switch(HDContrRegW[0] & 0b00011111) {		// i 3 bit alti sono class, gli altri il comando
								case 0x00:			// test ready
									if(HDFIFOPtrW==1) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										hdState=0b00011101;			// C/D 1=input (relative to Host); REQ=1 se ready for data
										HDFIFO[0]=0b00000000;
										HDFIFOPtrW=0;  HDFIFOPtrR=0;
										if(hdControl & 0b00000010)
												i8259IRR |= 0b00100000;		// simulo IRQ...
										}
									// al reset cmq...
										hdState=0b00011101;			// va bene sia alla prima richiesta sia alla seconda, DD
										HDFIFO[0]=hdState;
										HDFIFOPtrR=0;
										if(hdControl & 0b00000010)
												i8259IRR |= 0b00100000;		// simulo IRQ...
											
									hdState &= ~0b00001000;			// BSY
									break;
								case 0x01:			// recalibrate
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;

										HDContrRegR[1]=0b00000000;
										}
									break;
								case 0x02:			// reserved
									break;
								case 0x03:			// request sense
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;

										HDContrRegR[1]=0b00000000;

										}
									break;
								case 0x04:			// format drive
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								case 0x05:			// ready verify
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								case 0x06:			// format track
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										hdCylinder=MAKEWORD(HDFIFO[3],HDFIFO[2] & 0b11000000);
										hdHead=HDFIFO[1] & 0b00011111;
										}
									break;
								case 0x07:			// format bad track
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								case 0x08:			// read
									if(HDFIFOPtrW==6) {

										HDContrRegR[1]=0b11010000;		// Busy :) (anche 1=A??

										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										hdCylinder=MAKEWORD(HDFIFO[3],HDFIFO[2] & 0b11000000);
										hdHead=HDFIFO[1] & 0b00011111;
										hdSector=HDFIFO[2] & 0b00111111;
										HDFIFO[5];		// 
    								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
        						i8237DMACurLen[2]=i8237DMALen[2];
										n=i8237DMALen[2]   +1;		// 1ff ...
										do {
											encodeHDiscSector(disk);

											// SIMULO DMA qua...
    									if(!(i8237RegW[8] & 0b00000001)) {			// controller enable
												i8237RegR[8] |= ((1 << 2) << 4);  // request??...
												if(!(i8237RegR[15] & (1 << 2)))	{	// mask...
													switch(i8237Mode[2] & 0b00001100) {		// DMA mode
														case 0b00000000:			// verify
															memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << HDFIFO[5]);
															break;
														case 0b00001000:			// write
															memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << HDFIFO[5]);
															break;
														case 0b00000100:			// read
															memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << HDFIFO[5]);
															break;
														}
													}
												i8237Mode[2] & 0b00001100 == 0b00000100;		// Read
												i8237Mode[2] & 0b11000000 == 0b01000000;		// Single mode
												i8237RegR[8] |= 1 << 2;  // fatto
												i8237RegR[13]=ram_seg[(0x80 << HDFIFO[5])-1]; // ultimo byte trasferito :)
												}

					// VERIFICARE qua non penso ci sia!
											hdSector++;
											if(hdSector>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
												hdSector=1;


													hdHead++;

												if(hdHead>=2) {
													hdHead=0;
		hdCylinder++;
												}
												}
											addr+=0x80 << HDFIFO[4];		// verificare block count
											// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
											//i8237DMALen[2]=0;		// remaining. idem?
											n -= 0x80 << HDFIFO[4];
											} while(n>0);

    								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
        						i8237DMACurLen[2]=0;
        						i8237RegR[8] |= 0b00000100;		// TC 2
										HDFIFO[4] = hdCylinder & 0xff; //		//C
										HDFIFO[3] = (hdCylinder >> 8) | hdSector;
										HDFIFO[1] = 0b00000000;		//ST0: B7=address valid, B5:4=error type, B3:0=error code
										HDFIFO[2] = (disk ? 0b00100000 : 0b00000000) | (hdHead);
										HDFIFOPtrR=1;

		//								if(FloppyFIFOPtrR==7   >=1  ) {		// pare che ne legga solo 1...
											HDFIFOPtrW=0;
											hdState |= 0b01000000;		// passo a read
		//									}

										HDContrRegR[1]=0b00000000;

										if(hdControl & 0b00000010)
												i8259IRR |= 0b00100000;		// simulo IRQ...
										}
									else if(HDFIFOPtrR) {		// 
										if(HDFIFOPtrR > 4) {
											hdState &= ~0b00000000;	// passo a write ??? qua

											HDContrRegR[1]=0b00000000;
											}
										else
											HDContrRegR[1]=0b00000000;					// ok
										}
									else {
										HDContrRegR[1]=HDFIFOPtrW>0 ? 0b00000000 : 0b00000000;		// ok
										}
									break;
								case 0x09:			// reserved
									break;
								case 0x0a:			// write
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										HDContrRegR[1]=0b00000000;					// ok
										}
									break;
								case 0x0b:			// seek
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										hdCylinder=MAKEWORD(HDFIFO[3],HDFIFO[2] & 0b11000000);
										hdHead=HDFIFO[1] & 0b00011111;
										HDContrRegR[1]=0b00000000;					// ok
										}
									break;
								case 0x0c:			// initialize
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								case 0x0d:			// read ECC burst
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								case 0x0e:			// read data from sector buffer
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								case 0x0f:			// write data to sector buffer
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
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
							switch(HDContrRegW[0] & 0b00011111) {		// i 3 bit alti sono class, gli altri il comando
								case 0x00:			// RAM diagnostic
									if(HDFIFOPtrW==6) {
										hdState=0b00000000;									// ok
										HDFIFO[0]=hdState;
										HDFIFO[1]=0b00100000;									// vuole questo, 20h pare ok e 30h sarebbe ram error
										HDFIFOPtrW=0;  HDFIFOPtrR=0;
										if(hdControl & 0b00000010)
												i8259IRR |= 0b00100000;		// simulo IRQ...
										hdState &= ~0b00001000;			// BSY
										}
									break;
								case 0x01:			// reserved
									break;
								case 0x02:			// reserved
									break;
								case 0x03:			// drive diagnostic
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										HDContrRegR[1]=0b00000000;					// ok
										}
									break;
								case 0x04:			// controller internal diagnostic
									break;
								case 0x05:			// read long
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								case 0x07:			// write long
									if(HDFIFOPtrW==6) {
										disk=HDFIFO[1] & 0b00100000 ? 1 : 0;
										}
									break;
								default:		// invalid, 
									break;
								}
							break;
						}
					HDContrRegR[1]=HDFIFO[HDFIFOPtrR++];				// FINIRE
          break;
        case 2:			// READ DRIVE CONFIGURATION INFO
					HDContrRegR[2]=0b00000010;			// 10MB
          break;
        case 3:			// not used
          break;
				}
			i=HDContrRegR[t];
			break;

    case 0x37:    // 370-377 (secondary floppy, AT http://www.techhelpmanual.com/896-diskette_controller_ports.html); 378-37A  LPT1
      t &= 3;
      switch(t) {
        case 0:
          i=(PORTE & 0xff) & ~0b00010000;
          i |= PORTGbits.RG6 ? 0b00010000 : 0;   // perch� LATE4 � led...
          break;
        case 1:   // finire
          i=PORTB & 0xff;
          break;
        case 2:    // qua c'� lo strobe
          i = PORTGbits.RG7;      // finire
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

        case 2:   // Write: digital output register (DOR)   MOTD MOTC MOTB MOTA DMA ENABLE SEL1 SEL0
					// questo registro in effetti non appartiene al uPD765 ma � esterno...
          break;
        case 4:   // Read-only: main status register  REQ DIR DMA BUSY D C B A
					disk=FloppyContrRegW[0] & 3;
//					FloppyContrRegR[0] |= 0b10000000;		// DIO ready for data exchange
					// nella prima word FIFO i 3 bit pi� bassi sono in genere HD US1 US0 ossia Head e Floppy in uso (0..3)
					switch(FloppyContrRegW[1] & 0b00011111) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
						case 0x06:			// read data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
	//								getDiscSector();
									encodeDiscSector(disk);


				/*							 {
			extern HFILE spoolFile;
			char myBuf[256];
			wsprintf(myBuf,"T%u H%u S%u , %06x: %04x, %08x \n",floppyTrack,floppyHead,floppySector,MAKELONG(addr,DMApage[2]),n,getDiscSector()*512);
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
									if(!(i8237RegW[8] & 0b00000001)) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegR[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & 0b00001100) {		// DMA mode
												case 0b00000000:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case 0b00001000:			// write
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case 0b00000100:			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & 0b00001100 == 0b00000100;		// Read
										i8237Mode[2] & 0b11000000 == 0b01000000;		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

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
     						i8237RegR[8] |= 0b00000100;		// TC 2
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

//								if(FloppyFIFOPtrR==7   >=1  ) {		// pare che ne legga solo 1...
									FloppyFIFOPtrW=0;
									floppyState[disk] |= 0b0100000;		// passo a read
//									}

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}

							else if(FloppyFIFOPtrR) {		// NO! glabios lo fa, PCXTbios no: pensavo ci passasse pi� volte dopo la fine preparazione, ma non � cos� quindi si pu� togliere if
								//FloppyContrRegR[0]=FloppyFIFO[FloppyFIFOPtrR++];togliere
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write

//								FloppyContrRegR[0]=0b10000000) | (floppyState[disk] & 0b01000000));

									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x02:			// read track
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=1 /*FloppyFIFO[4]*/;
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
									encodeDiscSector(disk);

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & 0b00000001)) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegR[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & 0b00001100) {		// DMA mode
												case 0b00000000:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case 0b00001000:			// write
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case 0b00000100:			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & 0b00001100 == 0b00000100;		// Read
										i8237Mode[2] & 0b11000000 == 0b01000000;		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto
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
     						i8237RegR[8] |= 0b00000100;		// TC 2
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x0c:			// read deleted
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
									encodeDiscSector(disk);


									// SIMULO DMA qua...
									if(!(i8237RegW[8] & 0b00000001)) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegR[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & 0b00001100) {		// DMA mode
												case 0b00000000:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case 0b00001000:			// write
													memcpy(sectorData,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
													break;
												case 0b00000100:			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & 0b00001100 == 0b00000100;		// Read
										i8237Mode[2] & 0b11000000 == 0b01000000;		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

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
     						i8237RegR[8] |= 0b00000100;		// TC 2
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x0a:			// read ID
							if(FloppyFIFOPtrW==2 && !FloppyFIFOPtrR) {
								FloppyFIFO[6] = FloppyFIFO[5];
								FloppyFIFO[5] = FloppyFIFO[4];
								FloppyFIFO[4] = FloppyFIFO[3];
								FloppyFIFO[3] = FloppyFIFO[2];
								FloppyFIFO[0] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[1] = 0;		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[2] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyContrRegR[0]=0b11000000;
								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read
								FloppyContrRegR[0]=0b11010000;
								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x05:			// write data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*512;

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & 0b00000001)) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegR[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & 0b00001100) {		// DMA mode
												case 0b00000000:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case 0b00001000:			// write
//													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
                          // ovviamente serve SDcard
													break;
												case 0b00000100:			// read
//													memcpy(msdosDisk,sectorData,0x80 << FloppyFIFO[5]);
                          // ovviamente serve SDcard
													break;
												}
											}
										i8237Mode[2] & 0b00001100 == 0b00001000;		// Write
										i8237Mode[2] & 0b11000000 == 0b01000000;		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

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
     						i8237RegR[8] |= 0b00000100;		// TC 2
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x0d:			// format track
							if(FloppyFIFOPtrW==6) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								FloppyFIFO[2];		// bytes per sector	ossia 1=256, 2=512, 3=1024 se MFM, met� se FM
								FloppyFIFO[3];		// sector per track
								FloppyFIFO[4];		// gap3
								FloppyFIFO[5];		// filler byte
								floppySector[disk]=1;
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*512;

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & 0b00000001)) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegR[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & 0b00001100) {		// DMA mode
												case 0b00000000:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case 0b00001000:			// write
// bah non � chiaro, ma SET funziona!													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[2]);
//													memset(msdosDisk,FloppyFIFO[5],0x80 << FloppyFIFO[2]);
                          // serve SDcard!
													break;
												case 0b00000100:			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & 0b00001100 == 0b00001000;		// Write
										i8237Mode[2] & 0b11000000 == 0b01000000;		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[2])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									addr+=0x80 << FloppyFIFO[2];
									// current address... serve registro specchio?? i8237DMAAddr[2]+=0x80 << FloppyFIFO[5];
									} while(floppySector[disk]<=FloppyFIFO[3]);

								i8237DMACurAddr[2]+=i8237DMACurLen[2]+1;
								i8237DMACurLen[2]=0;
     						i8237RegR[8] |= 0b00000100;		// TC 2
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x09:			// write deleted data
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM
								addr=i8237DMACurAddr[2]=i8237DMAAddr[2];
								i8237DMACurLen[2]=i8237DMALen[2];
								n=i8237DMALen[2]   +1;		// 1ff ...
								do {
								  msdosDisk=((uint8_t*)MSDOS_DISK[disk])+getDiscSector(disk)*512;

									// SIMULO DMA qua...
									if(!(i8237RegW[8] & 0b00000001)) {			// controller enable
										i8237RegR[8] |= ((1 << 2) << 4);  // request??...
										if(!(i8237RegR[15] & (1 << 2)))	{	// mask...
											switch(i8237Mode[2] & 0b00001100) {		// DMA mode
												case 0b00000000:			// verify
													memcmp(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												case 0b00001000:			// write
//													memcpy(msdosDisk,&ram_seg[MAKELONG(addr,DMApage[2])],0x80 << FloppyFIFO[5]);
                          // ovviamente qua non va :) fare con SDcard...
													break;
												case 0b00000100:			// read
													memcpy(&ram_seg[MAKELONG(addr,DMApage[2])],sectorData,0x80 << FloppyFIFO[5]);
													break;
												}
											}
										i8237Mode[2] & 0b00001100 == 0b00001000;		// Write
										i8237Mode[2] & 0b11000000 == 0b01000000;		// Single mode
										i8237RegR[8] |= 1 << 2;  // fatto
										i8237RegR[13]=ram_seg[(0x80 << FloppyFIFO[5])-1]; // ultimo byte trasferito :)
										}

									floppySector[disk]++;
									if(floppySector[disk]>sectorsPerTrack[disk]) {		// ma NON dovrebbe accadere!
										floppySector[disk]=1;

										if(FloppyContrRegW[1] & 0b10000000) {		// solo se MT?

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
     						i8237RegR[8] |= 0b00000100;		// TC 2
								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x11:			// scan equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x19:			// scan low or equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x1d:			// scan high or equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector==sectorsPerTrack ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x07:			// recalibrate
							if(FloppyFIFOPtrW==2) {
								// in FIFO[1] : x x x x x 0 US1 US0									00h
								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=0;

								// delay...

								FloppyContrRegR[0]=0b00000000 | disk;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								floppyState[disk] |= 0b01000000;		// passo a read
								}
							else if(FloppyFIFOPtrR) {
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x08:			// sense interrupt status	
							if(!FloppyFIFOPtrR) {
								FloppyFIFO[1] = 0b11000000 /*| disk*/;		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								// OVVIAMENTE � strano, ma ESIGE 1100xxxx ... boh
								// forse sono sfalsati di 1 ossia ne serve uno in pi�
								FloppyFIFO[2] = floppyTrack[disk];
								FloppyFIFO[3] = floppyTrack[disk];
								FloppyFIFOPtrR=1;
								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b00000000 | disk;

								if(FloppyContrRegW[0] & 0b00001000)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 2) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							break;
						case 0x03:			// specify
							if(FloppyFIFOPtrW==3 && !FloppyFIFOPtrR) {
								// in FIFO[1] : StepRateTime[7..4] | HeadUnloadTime[3..0]				20h
								// in FIFO[2] : HeadLoadTime[7..1] | NonDMAMode[0]							07h
								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=0;
								floppyState[disk] |= 0b01000000;		// passo a read cmq

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x04:			// sense drive status
							if(FloppyFIFOPtrW==2) {
								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??
  							FloppyFIFO[0] = 0b00100000 | (!floppyTrack ? 0b00010000 : 0 | 
									(FloppyFIFO[1] & 0b00000111));		//ST3: B7 Fault, B6 Write Protected, B5 Ready, B4 Track 0, B3 Two Side, B2 Head (side select), B1:0 Disk Select (US1:0)
								FloppyFIFOPtrR=1;
								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read
								FloppyContrRegR[0]=0b11010000;
								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {		// NO! glabios lo fa, PCXTbios no: pensavo ci passasse pi� volte dopo la fine preparazione, ma non � cos� quindi si pu� togliere if
								if(FloppyFIFOPtrR > 1) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x1b:			// scan high or equal
							if(FloppyFIFOPtrW==9) {

								FloppyContrRegR[0]=0b11010000;		// Busy :) (anche 1=A??

								floppyTrack[disk]=FloppyFIFO[2];
								floppyHead[disk]=FloppyFIFO[3];
								floppySector[disk]=FloppyFIFO[4];
								FloppyFIFO[5];		// N ossia 1=256, 2=512, 3=1024 se MFM, met� se FM

								FloppyFIFO[7] = FloppyFIFO[5];		//N
								FloppyFIFO[6] = floppySector[disk]; //FloppyFIFO[4];		//R
								FloppyFIFO[5] = floppyHead[disk]; //FloppyFIFO[3];		//H
								FloppyFIFO[4] = floppyTrack[disk]; //FloppyFIFO[2];		//C
								FloppyFIFO[1] = 0 | (FloppyFIFO[1] & 0b00000111);		//ST0: B7:B6 error code, B5=1 dopo seek, B4=FAULT, B3=Not Ready, B2=Head, B1:0 US1:0 (drive #)
								FloppyFIFO[2] = 0 | (floppySector[disk]==sectorsPerTrack[disk] ? 0b10000000 : 0);		//ST1: B7 End of Cyl/Track, B5 CRC Error, B4 Overrun, B2 No Data (can't find sector), B1 Write Protect, B0 Missing Address Mark
								FloppyFIFO[3] = 0;		//ST2: B6 Control Mark (Deleted Data), B5 Data Error, B4 Wrong Cyl/Track, B3 Scan Equal, B2 Scan not satisfied, B1 Bad Cyl, B0 Missing Address Mark
								FloppyFIFOPtrR=1;

								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read

								FloppyContrRegR[0]=0b11010000;

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
									i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else if(FloppyFIFOPtrR) {
								if(FloppyFIFOPtrR > 7) {
									floppyState[disk] &= ~0b01000000;	// passo a write
									FloppyContrRegR[0]=0b10000000;
									}
								else
									FloppyContrRegR[0]=0b11010000;					// busy
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x0f:			// seek (goto cylinder
							if(FloppyFIFOPtrW==3 && !FloppyFIFOPtrR) {
								// in FIFO[1] : x x x x x Head US1 US0						00h
								// in FIFO[2] : New Cylinder Number								00h
								if(FloppyFIFO[2] < totalTracks) {
									FloppyContrRegR[0]=0b00010000 | (1 << disk);					// busy
									floppyTrack[disk]=FloppyFIFO[2];
								// __delay_ms(12*tracce)
									FloppyContrRegR[0]=0b10000000;					// finito :)
									}
								else {
									}
								FloppyFIFOPtrW=0;
								FloppyFIFOPtrR=0;
								floppyState[disk] |= 0b01000000;		// passo a read cmq

								if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
										i8259IRR |= 0b01000000;		// simulo IRQ...
								}
							else {
								FloppyContrRegR[0]=FloppyFIFOPtrW>0 ? 0b10010000 : 0b10000000;		// busy
								}
							break;
						case 0x00:			// (all'inizio comandi...
							FloppyContrRegR[0]=0b10000000	|
								floppyState[disk] & 0b01000000;
								// b7=1 ready!;	b6 � direction (0=write da CPU a FDD, 1=read); b4 � busy/ready=0

							floppyState[disk] |= 0b01000000;		// passo a read
							FloppyFIFOPtrW=0;
							break;
						default:		// invalid, 
							FloppyFIFO[0];
//								FloppyContrRegR[4] |= 0b10000000);		// 
							if(!FloppyFIFOPtrR) {
								FloppyFIFO[0] = 0x80;		//ST0
								}
							FloppyContrRegR[0]=FloppyFIFO[FloppyFIFOPtrR++];
							if(FloppyFIFOPtrR==1) {
								FloppyFIFOPtrW=0;
								floppyState[disk] |= 0b01000000;		// passo a read
								}
							break;
						}
					i=FloppyContrRegR[0];
          break;
        case 5:   // Read/Write: FDC command/data register
					disk=FloppyContrRegW[0] & 3;
					switch(FloppyContrRegW[1] & 0b00011111) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
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
							FloppyContrRegR[1]=FloppyFIFO[FloppyFIFOPtrR++];
							break;
						case 0x03:
						case 0x07:
						case 0x0f:
							FloppyContrRegR[1]=0b11000000;
							FloppyContrRegR[1]=0b11000000;
							break;
						default:
							FloppyContrRegR[1]=0b10000000;
							break;
						}
					i=FloppyContrRegR[1];
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
						i=COMDataEmuFifo[COMDataEmuFifoCnt++];
#if MOUSE_TYPE==1
						COMDataEmuFifoCnt %= 3;		// 3 byte per mouse microsoft seriale
#elif MOUSE_TYPE==2
						COMDataEmuFifoCnt %= 5;		// 5 byte per mouse mouse system seriale
#endif
						if(!COMDataEmuFifoCnt) {
				      mouseState = 255;  // marker 
							}
						else {
							     // ri-simulo per altre 2/4 volte!
      				i8250Reg[2] &= ~0b00000001;
    //					if(i8250Reg[1] & 0b00000001)			// se IRQ attivo ??
								i8259IRR |= 0x10;
							}
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
						if(mouseState != 255)
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
              i8250Reg[5] |= 0b00100000;
            else
              i8250Reg[5] &= ~0b00100000;
            if(!U1STAbits.UTXBF)    // TX
              i8250Reg[5] |= 0b01000000;
            else
              i8250Reg[5] &= ~0b01000000;*/
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
			break;
    }
  
	return i;
	}

uint16_t InShortValue(uint16_t t) {
	register uint16_t i;

return MAKEWORD(InValue(t),InValue(t+1));			// per ora, v. cmq GLABios
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
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		t-=ROM_START2;
		i=MAKEWORD(bios_rom_opt[t],bios_rom_opt[t+1]);
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE) {
		t-=ROM_HD_BASE;
		i=MAKEWORD(HDbios[t],HDbios[t+1]);
		}
#endif
#ifdef CGA_BASE
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		i=MAKEWORD(CGAram[t],CGAram[t+1]);
    }
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		t-=VGA_BIOS_BASE;
		i=MAKEWORD(VGABios[t],VGABios[t+1]);
    }
#endif
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
	else
		i=UNIMPLEMENTED_MEMORY_VALUE;
  
	return i;
	}


#ifdef EXT_80386
uint32_t GetIntValue(uint32_t t) {
	register uint32_t i;

	if(t >= (ROM_END-ROM_SIZE)) {
		t-=ROM_END-ROM_SIZE;
		i=MAKELONG(MAKEWORD(bios_rom[t],bios_rom[t+1]),MAKEWORD(bios_rom[t+2],bios_rom[t+3]));
		}
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		t-=ROM_START2;
		i=MAKELONG(MAKEWORD(bios_rom_opt[t],bios_rom_opt[t+1]),MAKEWORD(bios_rom_opt[t+2],bios_rom_opt[t+3]));
		}
#endif
#ifdef CGA_BASE
	else if(t >= CGA_BASE && t < CGA_BASE+CGA_SIZE) {
		t-=CGA_BASE;
		i=MAKELONG(MAKEWORD(CGAram[t],CGAram[t+1]),MAKEWORD(CGAram[t+2],CGAram[t+3]));
    }
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		t-=VGA_BIOS_BASE;
		i=MAKELONG(MAKEWORD(VGABios[t],VGABios[t+1]),MAKEWORD(VGABios[t+2],VGABios[t+3]));
    }
#endif
	else if(t < RAM_SIZE) {
		i=MAKELONG(MAKEWORD(ram_seg[t],ram_seg[t+1]),MAKEWORD(ram_seg[t+2],ram_seg[t+3]));
		}
	else
		i=UNIMPLEMENTED_MEMORY_VALUE;
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
	else if(t >= ROM_HD_BASE) {
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
#ifdef ROM_SIZE2
	else if(t >= ROM_START2 && t<(ROM_START2+ROM_SIZE2)) {
		t-=(ROM_START2-4);
		Pipe2.bd[3]=bios_rom_opt[t++];
		Pipe2.bd[4]=bios_rom_opt[t++];
		Pipe2.bd[5]=bios_rom_opt[t];
		}
#endif
#ifdef ROM_HD_BASE
	else if(t >= ROM_HD_BASE) {
		t-=(ROM_HD_BASE -4);
		Pipe2.bd[3]=HDbios[t++];
		Pipe2.bd[4]=HDbios[t++];
		Pipe2.bd[5]=HDbios[t];
		}
#endif
#ifdef VGA
	else if(t >= VGA_BIOS_BASE && t < VGA_BIOS_BASE+32768) {
		t-=VGA_BIOS_BASE;
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
#ifdef CGA_BASE
	if(t>=CGA_BASE && t < (CGA_BASE+CGA_SIZE)) {
		CGAram[t-CGA_BASE]=t1;
    LCDdirty=TRUE;
    }
	else 
#endif
		if(t < RAM_SIZE) {
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
#ifdef CGA_BASE
	if(t>=CGA_BASE && t<(CGA_BASE+CGA_SIZE)) {
		t-=CGA_BASE;
		CGAram[t++]=LOBYTE(t1);
		CGAram[t]=HIBYTE(t1);
    LCDdirty=TRUE;
    }
	else 
#endif
		if(t < RAM_SIZE) {
	  ram_seg[t++]=LOBYTE(t1);
	  ram_seg[t]=HIBYTE(t1);
		}
// else _f.Trap=1?? v. anche eccezione PIC

	}


#ifdef EXT_80386
void PutIntValue(uint32_t t,uint32_t t1) {
	register uint16_t i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

#ifdef CGA_BASE
	if(t>=CGA_BASE && t<(CGA_BASE+CGA_SIZE)) {
		t-=CGA_BASE;
		CGAram[t++]=LOBYTE(LOWORD(t1));
		CGAram[t++]=HIBYTE(LOWORD(t1));
		CGAram[t++]=LOBYTE(HIWORD(t1));
		CGAram[t]=HIBYTE(HIWORD(t1));
    }
	else 
#endif
		if(t < RAM_SIZE) {
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
    case 0:        // 00-1f DMA 8237 controller (usa il 2 per il floppy
    case 1:				// solo PS2 dice!
      t &= 0xf;
      i8237RegR[t]=i8237RegW[t]=t1;
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
					i8237FF++;
					i8237FF &= 1;
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
					i8237FF++;
					i8237FF &= 1;
					break;
				case 0x8:			// Command (reg.8
					break;
				case 0xa:			// set/clear Mask
					if(t1 & 0b00000100)
						i8237RegW[15] |= (1 << (t1 & 0b00000011));
					else
						i8237RegW[15] &= ~(1 << (t1 & 0b00000011));
					i8237RegR[15]=i8237RegW[15];
					break;
				case 0xb:			// Mode
					i8237Mode[t1 & 0b00000011]= t1 & 0b11111100;
					i8237DMACurAddr[t1 & 0b00000011]=i8237DMAAddr[t1 & 0b00000011];			// bah credo, qua
					i8237DMACurLen[t1 & 0b00000011]=i8237DMALen[t1 & 0b00000011];
					break;
				case 0xc:			// clear Byte Pointer Flip-Flop
					i8237FF=0;
					break;
				case 0xd:			// Reset
					memset(i8237RegR,0,sizeof(i8237RegR));
					memset(i8237RegW,0,sizeof(i8237RegW));
					i8237RegR[15]=i8237RegW[15] = 0b00001111;
					break;
				case 0xf:			//  Masks
					i8237RegR[15]=i8237RegW[15] = (t1 & 0b00001111);
					break;
				}

      break;

    case 2:         // 20-21 PIC 8259 controller
//https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
      t &= 0x1;
      switch(t) {
        case 0:     // 0x20 per EOI interrupt, 0xb per "chiedere" quale... subito dopo c'� lettura a 0x20
          i8259RegW[0]=t1;      // 
					if(t1 & 0b00010000) {// se D4=1 qua (0x10) � una sequenza di Init,
						i8259RegR[0]=0;
						// boh i8259IMR=0xff; disattiva tutti
						i8259ICW[0]=t1;
						i8259ICWSel=2;		// >=1 indica che siamo in scrittura ICW
						}
          else {
						// questa sarebbe OCW2  https://k.lse.epita.fr/internals/8259a_controller.html
						if(t1==0x20) {			// questo pulisce IRQ con priorit� + alta in essere (https://rajivbhandari.wordpress.com/wp-content/uploads/2014/12/unitii_8259.pdf

//						i8259ISR=0;		// MIGLIORARE! pulire solo il primo
							for(i=1; i; i<<=1) {
								if(i8259ISR & i) {
									i8259ISR &= ~i;
									break;
									}
								}
//				      CPUPins &= ~DoIRQ;		// SEMPRE! non serve + v. di l�
							}
						else if(t1==0x60)			// 0x60 dovrebbe indicare uno specifico IRQ, nei 3bit bassi 0..7
							i8259ISR &= ~(1 << (t1 & 7));
														
          // (pulire QUA i flag TMRIRQ ecc??  //(o toglierli del tutto e usare i bit di 8259
						}
          break;
        case 1:
          i8259RegR[1]=i8259RegW[1]=t1;      //  0xbc su bios3.1 (kbd, tod, floppy)
          // (cmq FINIRE! ci sono pi� comandi a seconda dei bit... all inizio manda 0x13 a 20 e 0x8 0x9 0xff a 21; poi per pulire un IRQ si manda il valore letto da 0x20 qua
					switch(i8259ICWSel) {
						case 2:
							i8259ICW[1]=t1;
							i8259ICWSel++;
							break;
						case 3:
							i8259ICWSel++;
							if(i8259ICW[0] & 0b00000010) {		// single?
								goto icw4;
								}
							else {
								i8259ICW[2]=t1;
								}
							break;
						case 4:
icw4:
							if(i8259ICW[0] & 0b00000001) {		// ICW4 needed?
								i8259ICW[3]=t1;
								}
							i8259ICWSel=0;
							break;
						default:
							i8259IMR=t1;
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
      switch(t) {
        case 0:
          switch((i8253Mode[0] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 2:
              i8253TimerW[0]=MAKEWORD(LOBYTE(i8253TimerW[0]),t1);
							i8253Mode[0] |= 0b01000000;          // reloaded OCCHIO FORSE SOLO QUANDO BYTE ALTO, verificare
              i8253ModeSel = 0;
              break;
            case 1:
              i8253TimerW[0]=MAKEWORD(t1,HIBYTE(i8253TimerW[0]));
							i8253Mode[0] |= 0b01000000;          // reloaded
              i8253ModeSel = 0;
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[0]=MAKEWORD(LOBYTE(i8253TimerW[0]),t1);
              else
                i8253TimerW[0]=MAKEWORD(t1,HIBYTE(i8253TimerW[0]));
							i8253Mode[0] |= 0b01000000;          // reloaded
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
							i8253Mode[1] |= 0b01000000;          // reloaded
              i8253ModeSel = 0;
              break;
            case 1:
              i8253TimerW[1]=MAKEWORD(t1,HIBYTE(i8253TimerW[1]));
							i8253Mode[1] |= 0b01000000;          // reloaded
              i8253ModeSel = 0;
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[1]=MAKEWORD(LOBYTE(i8253TimerW[1]),t1);
              else
                i8253TimerW[1]=MAKEWORD(t1,HIBYTE(i8253TimerW[1]));
							i8253Mode[1] |= 0b01000000;          // reloaded
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          i8253RegW[1]=t1;
          PR5=i8253TimerW[1] ? i8253TimerW[1] : 65535;
          break;
        case 2:
          switch((i8253Mode[2] & 0b00110000) >> 4) {
            case 0:
              // latching operation...
              break;
            case 2:
              i8253TimerW[2]=MAKEWORD(LOBYTE(i8253TimerW[2]),t1);  
							i8253Mode[2] |= 0b01000000;          // reloaded
              i8253ModeSel = 0;
              break;
            case 1:
              i8253TimerW[2]=MAKEWORD(t1,HIBYTE(i8253TimerW[2]));
							i8253Mode[2] |= 0b01000000;          // reloaded
              i8253ModeSel = 0;
              break;
            case 3:
              if(i8253ModeSel)
                i8253TimerW[2]=MAKEWORD(LOBYTE(i8253TimerW[2]),t1);
              else
                i8253TimerW[2]=MAKEWORD(t1,HIBYTE(i8253TimerW[2]));
							i8253Mode[2] |= 0b01000000;          // reloaded
              i8253ModeSel++;
              i8253ModeSel &= 1;
              break;
            }
          i8253RegW[2]=t1;
          PR2=i8253TimerW[2] ? i8253TimerW[2] : 65535;
          break;
        case 3:
          if((t1 >> 6) < 3)
            i8253Mode[t1 >> 6]=t1 & 0x3f;
          i8253RegR[3]=i8253RegW[3]=t1;
          i8253ModeSel=0;
          break;
        }
      break;

    case 6:        // 60-6F 8042/8255 keyboard; speaker; config byte
      t &= 0xf;
      switch(t) {
        case 0:     // (solo bit7? controllare
/*out 64, aa
in 60
out 64, ab
in 60
out 64, ae
in 60
out 64, 60
out 60, 45
*/
          if(i8042RegW[1]==0xAA) {
            Keyboard[0]=0;
            }
          else if(i8042RegW[1]==0x60) {
            KBCommand=t1;
						KBStatus &= ~0b00010100;
						KBStatus |= KBCommand & 0b00010100;
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
            Keyboard[0]=i8042RegW[1];
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
            }
          i8042RegW[0]=t1;     // 
          i8042RegW[1]=0;     // direi, cos� pulisco
					i8255RegR[0]=i8255RegW[0]=t1;
          break;
        case 1:     // brutalmente, per buzzer!  e machine flags... (da V20)
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
//          MachineFlags=t1;
					// A5 non-turbo E070 (pcxtbios
					// B5 per pulire parity error E191
					// 85 enable parity E194
					// AD memory size nibble E1AA
					// 08 read low switches... E1C9
					// 04 toggle speed (letto da 0x61 e XOR 0c) E289
					//  set b4-5 per disabilitare, poi b6-b7 per pulire errore e quindi reset b4-5 per riattivare (NMI handler in pcxtbios
					// AD KB hold low+disable, NMI on, spkr off (glabios E0AE
					i8255RegR[1]=i8255RegW[1]=t1;


					if(!(KBCommand & 1) && !(t1 & 0b10000000) && (t1 & 0b01000000)) {
						KBCommand |= 1;		// patch glabios/5160
// no! ...						goto kb_setirq;
            i8042RegR[0]=0xAA;
						i8259IRR |= 2;
						}
					if(t1 & 0b10000000) {		// ack kbd		patch glabios/5160
						KBStatus &= ~0b00000001;			// output done
						}

          break;
        case 2:     // machine settings... (da V20), NMI
          // scrive 0x08 per "leggere switch"... ma non si capisce cos'altro cmq... anche AD
          Nop();
					i8255RegR[2]=i8255RegW[2]=t1;
					i8255RegR[2] &= ~0b11000000;		// bah direi! questi sarebbero parity error ecc, da settare in HW
          break;
        case 3:     // PIA 8255, port A & C dice, a E06C...  per� sarebbe anche a 0x61 per machineflags
					i8255RegR[3]=i8255RegW[3]=t1;		// glabios mette 0x99 ossia PA=input, PC=tutta input, PB=output
          break;
        case 4:     // 
          i8042RegW[1]=t1;
          KBStatus |= 0b00000010;
          if(i8042RegW[1]==0xAA) {					// self test
            KBStatus |= 0b00000010;
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xAB) {			// test lines 
            KBStatus |= 0b00000010;
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xAE) {			// enable
            KBStatus |= 0b00000010;
						KBStatus |= 0b00001000;		// command
            KBCommand &= ~0b00010000;
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0x20 || i8042RegW[1]==0x60) { // r/w control
						KBStatus |= 0b00001000;		// command

						goto kb_setirq;
            }
          else if(i8042RegW[1]>0x60 && i8042RegW[1]<0x80) {
            //KBRAM[i8042RegW[1] & 0x1f]
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xC0) {
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xD0) {
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xD1) {
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xD2) {
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xED) {     //led 
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]==0xEE) {     //echo
						KBStatus |= 0b00001000;		// command
						goto kb_setirq;
            }
          else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
						KBStatus |= 0b00001000;		// command
kb_setirq:
						if(KBCommand & 0b00000001) {     //..e se interrupt attivi...
						//    KBDIRQ=1;
							i8259IRR |= 2;
							}
	          }
//        KBStatus &= ~0b00000010;    // vabbe' :)
        break;
        }
      break;

    case 7:        //   70-7F CMOS RAM/RTC (Real Time Clock  MC146818)
      // SECONDO PCXTBIOS, il clock � a 240 o 2c0 o 340 ed � mappato con tutti i registri... pare!
			// https://web.archive.org/web/20160314084340/https://www.bioscentral.com/misc/cmosmap.htm
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
              // in mezzo c'� Alarm...
            case 2:
              currentTime.min=t1;
              break;
            case 4:
              currentTime.hour=t1;
              break;
            case 6:
            // 6 � day of week... si pu� scrivere??
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
      switch(t) {
				case 0:
					//if(t==0x80 /*t>=0x108 && t<=0x10f*/) {      // diag display, uso per led qua!
    // https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html
					t &= 0x7;   //[per ora tolti xch� usati per debug! 
        LED1 = t1 & 1;
        LED2 = t1 & 2 ? 1 : 0;
        LED3 = t1 & 4 ? 1 : 0;
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
				}
      break;

    case 0xa:        // PIC 8259 controller #2 su AT; machine settings su XT
#ifdef PCAT
      t &= 0x1;
      switch(t) {
        case 0:     // 0x20 per EOI interrupt
          i8259Reg2W[0]=t1;
          if(t1==0x20)
            i8259Reg2R[0]=0;
          // pulire QUA i flag KBDIRQ ecc??   //				o toglierli del tutto e usare i bit di 8259

          break;
        case 1:
          i8259Reg2W[1]=t1;
          break;
        }
#else
			i=t;		//DEBUG
					// questo BLOCCA NMI inoltre! (pcxtbios
#endif

      break;

		case 0xc:		// che c'� qua?? ci scrive ibmbio.sys...      DMA 2	(second Direct Memory Access controller 8237)  https://wiki.preterhuman.net/XT,_AT_and_PS/2_I/O_port_addresses

			break;

    case 0xf:  // F0 = coprocessor
      break;

  // 108 diag display opp. 80
  // 278-378 printer
  // 300 = POST diag
  // 2f8-3f8 serial

#ifdef PCAT
    case 0x1f:      // 1f0-1ff   // hard disc 0 su AT		http://www.techhelpmanual.com/897-at_hard_disk_ports.html   http://www.techhelpmanual.com/892-i_o_port_map.html
      break;
#endif
    
    case 0x20:    //200-207   joystick
      break;
      
    case 0x22:    //220-223   Sound Blaster; -22F se Pro
      break;
      
    case 0x24:    // RTC SECONDO PCXTBIOS, il clock � a 240 o 2c0 o 340 ed � mappato con tutti i registri... pare!
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

    case 0x32:    // 320- (hard disk su XT  http://www.techhelpmanual.com/898-xt_hard_disk_ports.html   https://forum.vcfed.org/index.php?threads/ibm-xt-i-o-addresses.1237247/
      t &= 3;
      switch(t) {
        case 0:			// WRITE DATA
					if(!HDFIFOPtrW) {
						HDContrRegW[0]=t1;
						switch(t1 & 0b11100000) {		// i 3 bit alti sono class, gli altri il comando
							case 0b00000000:
								switch(t1 & 0b00011111) {		// i 3 bit alti sono class, gli altri il comando
									case 0x00:			// test ready
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x01:			// recalibrate
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x02:			// reserved
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x03:			// request sense
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x04:			// format drive
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x05:			// ready verify
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x06:			// format track
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x07:			// format bad track
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x08:			// read
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x09:			// reserved
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x0a:			// write
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x0b:			// seek
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x0c:			// initialize
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x0d:			// read ECC burst
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x0e:			// read data from sector buffer
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x0f:			// write data to sector buffer
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									default:		// invalid, 
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										HDFIFO[0];
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
								switch(t1 & 0b00011111) {		// i 3 bit alti sono class, gli altri il comando
									case 0x00:			// RAM diagnostic
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x01:			// reserved
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x02:			// reserved
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x03:			// drive diagnostic
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x04:			// controller internal diagnostic
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x05:			// read long
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									case 0x07:			// write long
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										break;
									default:		// invalid, 
										HDFIFOPtrW=1; HDFIFOPtrR=0;
										HDFIFO[0];
										break;
									}
								break;
							}
						}
					else {
						HDFIFO[HDFIFOPtrW++] = t1;
						HDFIFOPtrW &= 7;		// safety! errore ev.
						}

					if(hdControl & 0b00000010)
						i8259IRR |= 0x20;		// simulo IRQ...
//						hdTimer=100;
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
          break;
        case 2:			// SELECT
					/*Writing to port 322 selects the WD 1002S-WX2,
						sets the Busy bit in the Status Register and prepares
						it to receive a command. When writing to port
						322, the data byte is ignored.*/
					hdState |= 0b00001000;			// BSY
					HDContrRegW[2]=t1;
          break;
        case 3:			// WRITE DMA AND INTERRUPT MASK REGISTER
					hdControl=HDContrRegW[3]=t1;
          break;
				}

			break;

    case 0x37:    // 370-377 (secondary floppy, AT http://www.techhelpmanual.com/896-diskette_controller_ports.html); 378-37A  LPT1
      t &= 3;
      switch(t) {
        case 0:
#ifndef ILI9341
          LATE = (LATE & 0xff00) | t1;
          LATGbits.LATG6 = t1 & 0b00010000) ? 1 : 0;   // perch� LATE4 � led...
#endif
          break;
        case 2:    // qua c'� lo strobe
          LATGbits.LATG7 = t1 & 0b00000001 ? 1 : 0;   // strobe, FARE gli altri :)
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
          MDAreg[5]=i6845RegR[MDAreg[4] % 18]=i6845RegW[MDAreg[4] % 18]=t1;		// cmq dice che � write-only
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

#ifdef CGA
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
#endif
    
    case 0x3f:    // 3f8-3ff  COM1; 3f2-5 NEC floppy (http://www.techhelpmanual.com/896-diskette_controller_ports.html)
      t &= 0xf;
      switch(t) {
				uint8_t disk;
        case 2:   // Write: digital output register  MOTD MOTC MOTB MOTA DMA ENABLE SEL1 SEL0
					// ma secondo ASM 8 � Interrupt on e 4 � reset !
					disk=t1 & 3;
					/*FloppyContrRegR[0]=*/FloppyContrRegW[0]=t1;
					FloppyContrRegW[1]=0;
					if(!(FloppyContrRegW[0] & 0b00000100)) {		// cos� � RESET!
						FloppyFIFOPtrW=FloppyFIFOPtrR=0;
						floppyHead[disk]=0;
						floppyTrack[disk]=0;
						floppySector[disk]=0,
						floppyTimer=0;

						if(disk <= ((MachineSettings & 0b11000000) >> 6))		// (2 presenti!
							floppyState[disk]=0x80; /* b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent*/
						else
							floppyState[disk]=0x00; /* b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent*/

						}
else  /*					boh */{
//						FloppyFIFOPtrW=FloppyFIFOPtrR=0;
						floppyState[disk] &= ~0b01000000;	// passo a write
						if(FloppyContrRegW[0] & 0b00010000)	
							floppyState[0] |= 0b00000100;		// motor
						else
							floppyState[0] &= ~0b00000100;
						if(FloppyContrRegW[0] & 0b00100000)
							floppyState[1] |= 0b00000100;
						else
							floppyState[1] &= ~0b00000100;
						if(FloppyContrRegW[0] & 0b01000000)
							floppyState[2] |= 0b00000100;
						else
							floppyState[2] &= ~0b00000100;
						if(FloppyContrRegW[0] & 0b10000000)
							floppyState[3] |= 0b00000100;
						else
							floppyState[3] &= ~0b00000100;
						if(FloppyContrRegW[0] & 0b00001000 /*in asm dice che questo � "Interrupt ON"*/)
							i8259IRR |= 0b01000000;		// simulo IRQ...
	//						floppyTimer=100;
						}
          break;
        case 4:   // Read-only: main status register  REQ DIR DMA BUSY D C B A
          break;
        case 5:   // Read/Write: FDC command/data register
					if(!FloppyFIFOPtrW) {
						FloppyContrRegW[1]=t1;
						switch(t1 & 0b00011111) {		// i 3 bit alti sono MT (multitrack), MF (MFM=1 o FM=0), SK (Skip deleted)
							case 0x06:			// read data
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x02:			// read track
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x0c:			// read deleted
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x0a:			// read ID
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x05:			// write data
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x0b:			// format track
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x09:			// write deleted data
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x11:			// scan equal
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x19:			// scan low or equal
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x07:			// recalibrate
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x08:			// sense interrupt status
//								FloppyFIFOPtrW=0; FloppyFIFOPtrR=0;
								break;
							case 0x03:			// specify
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x04:			// sense drive status
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x1b:			// scan high or equal
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							case 0x0f:			// seek (goto cylinder
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								break;
							default:		// invalid, 
								FloppyFIFOPtrW=1; FloppyFIFOPtrR=0;
								FloppyFIFO[0];
	//								FloppyContrRegR[4] |= 0b10000000);		// 
								break;
							}
						}
					else {
						FloppyFIFO[FloppyFIFOPtrW++] = t1;
						FloppyFIFOPtrW &= 15;		// safety! errore ev.
						}
//            i8259IRR |= 0b01000000;		// simulo IRQ...
//					if(FloppyContrRegW[0] & 0b10000000))		// RESET si autopulisce
//						FloppyContrRegW[0] &= ~0b10000000);		// 
//					FloppyContrRegR[0] &= ~0b00010000);		// non BUSY :)
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
            i8250Regalt[0]=t1;
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
					COMDataEmuFifo[0]=COMDataEmuFifo[1]=COMDataEmuFifo[2]='M';  COMDataEmuFifoCnt=0;		// Line, RTS CTS...
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

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);
//	i=1;

	OutValue(t,LOBYTE(t1));			// per ora (usato da GLAbios
	OutValue(t+1,HIBYTE(t1));			// 
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

uint8_t *encodeHDiscSector(BYTE disk) {
  int i;
  uint8_t c;

//  msdosHDisk=((uint8_t*)MSDOS_HDISK)+getHDiscSector(disk)*512;

//	memcpy(sectorData,msdosHDisk,512);
 
  return sectorData;
  }




void initHW(void) {
	int i;
  
	MachineSettings=0b00101101;			//1 floppy
#ifdef MDA 
	MachineSettings=0b01111101;			//2 floppy
#endif
#ifdef VGA
	MachineSettings=0b00001101;
#endif
		// b0=floppy (0=s�); b1=FPU; bits 2,3 memory size (64K bytes); b5:4 tipo video: 11=Mono, 10=CGA 80x25, 01=CGA 40x25; b7:6 #floppies-1
		// ma su IBM5160 b0=1 SEMPRE dice!
	MachineFlags=0b00000000;
		// b0-1=Speaker on; b2-3=TURBO PC (o select); b4-5=Parity/I/O errors su TINYBIOS; b66= 0:KC clk low; b7= 0 enable KB    https://wiki.osdev.org/Non_Maskable_Interrupt
  memset(CGAreg,0,sizeof(CGAreg));
  CGAreg[0]=0;              // disattivo video, default cmq
	memset(&MDAreg,0,sizeof(MDAreg));  // 
  i6845RegR[10]=i6845RegW[10]=0x20;     //disattivo cursore!
  
  memset(i8237RegR,0,sizeof(i8237RegR));
  memset(i8237RegW,0,sizeof(i8237RegW));
	i8237RegR[15]=i8237RegW[15] = 0b00001111;
  memset(i8237Reg2R,0,sizeof(i8237Reg2R));
  memset(i8237Reg2W,0,sizeof(i8237Reg2W));
	i8237Reg2R[15]=i8237Reg2W[15] = 0b00001111;
	memset(DMApage,0,sizeof(DMApage));
  i8259RegR[0]=i8259RegW[0]=0x00; i8259RegR[1]=i8259RegW[1]=0xff; i8259IMR=0xff; i8259ISR=i8259IRR=0;
	memset(i8259ICW,0,sizeof(i8259ICW));
#ifdef PCAT
  i8259Reg2R[0]=i8259Reg2W[0]=0x00; i8259Reg2R[1]=i8259Reg2W[1]=0xff; i8259IMR2=0xff; i8259ISR2=i8259IRR2=0;
	memset(i8259ICW2,0,sizeof(i8259ICW2));
	i8259ICWSel=i8259ICW2Sel=0;
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
  memset(i146818RAM,0,sizeof(i146818RAM));
  memset(i8042RegR,0,sizeof(i8042RegR));
  memset(i8042RegW,0,sizeof(i8042RegW));
  memset(i8255RegR,0,sizeof(i8255RegR));
  memset(i8255RegW,0,sizeof(i8255RegW));
	memset(&Keyboard,0,sizeof(Keyboard));
	memset(&KBRAM,0,sizeof(KBRAM));
  KBCommand=0b01010000 /*XT,disabled*/; KBStatus=0x00;

  memset(FloppyContrRegR,0,sizeof(FloppyContrRegR));
  memset(FloppyContrRegW,0,sizeof(FloppyContrRegW));
	memset(FloppyFIFO,0,sizeof(FloppyFIFO));
	FloppyFIFOPtrW=FloppyFIFOPtrR=0;
	for(i=0; i<4; i++) {
		floppyState[i]=0x80; /* b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent*/
		floppyHead[i]=0;
		floppyTrack[i]=0;
		floppySector[i]=0;
		}
	floppyState[1]=0x80; /* b0=stepdir,b1=disk-switch,b2=motor,b6=DIR,b7=diskpresent*/
	floppyState[2]=0;
	floppyState[3]=0;
	
	memset(HDContrRegR,0,sizeof(HDContrRegR));
	memset(HDContrRegW,0,sizeof(HDContrRegW));
	HDFIFOPtrR=HDFIFOPtrW=0;
	hdHead=0;
	hdCylinder=0;
	hdSector=0;
	hdState=hdControl=0;
	mouseState=255;
  
	i146818RAM[13] = 0b10000000;		// VRT RTC batt ok!

	if(!(i146818RAM[11] & 0b00000100)) {			// sarebbe b2 del registro 11 ma al boot non c'�...
/*    currentTime.hour=to_bcd(currentTime.hour);
    currentTime.min=to_bcd(currentTime.min);
    currentTime.sec=to_bcd(currentTime.sec);
    i146818RAM[0x32] = to_bcd((((WORD)currentDate.year)+1900) / 100);		// secolo
    currentDate.year=to_bcd(currentDate.year % 100);			// anno, e v. secolo in RAM[32], GLATick
    currentDate.mon=to_bcd(currentDate.mon  +1);
    currentDate.mday=to_bcd(currentDate.mday);
 * qua forse sono gi� BCD... veriricare */
    }

  
	ColdReset=0;
	ExtIRQNum=0;
  
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
