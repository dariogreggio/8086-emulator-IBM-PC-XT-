// Local Header Files
#include <stdlib.h>
#include <string.h>
#include <xc.h>
#include "8086_pic.h"

#include <sys/attribs.h>
#include <sys/kmem.h>

#ifdef ST7735
#include "Adafruit_ST77xx.h"
#include "Adafruit_ST7735.h"
#include "adafruit_gfx.h"
#endif
#ifdef ILI9341
#include "Adafruit_ILI9341.h"
#include "adafruit_gfx.h"
#endif



// PIC32MZ1024EFE064 Configuration Bit Settings

// 'C' source line config statements

// DEVCFG3
// USERID = No Setting
#pragma config FMIIEN = OFF             // Ethernet RMII/MII Enable (RMII Enabled)
#pragma config FETHIO = OFF             // Ethernet I/O Pin Select (Alternate Ethernet I/O)
#pragma config PGL1WAY = ON             // Permission Group Lock One Way Configuration (Allow only one reconfiguration)
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = ON            // USB USBID Selection (Controlled by the USB Module)

// DEVCFG2
/* Default SYSCLK = 200 MHz (8MHz FRC / FPLLIDIV * FPLLMUL / FPLLODIV) */
//#pragma config FPLLIDIV = DIV_1, FPLLMULT = MUL_50, FPLLODIV = DIV_2
#pragma config FPLLIDIV = DIV_1         // System PLL Input Divider (1x Divider)
#pragma config FPLLRNG = RANGE_5_10_MHZ// System PLL Input Range (5-10 MHz Input)
#pragma config FPLLICLK = PLL_FRC       // System PLL Input Clock Selection (FRC is input to the System PLL)
#pragma config FPLLMULT = MUL_51       // System PLL Multiplier (PLL Multiply by 50)
#pragma config FPLLODIV = DIV_2        // System PLL Output Clock Divider (2x Divider)
#pragma config UPLLFSEL = FREQ_24MHZ    // USB PLL Input Frequency Selection (USB PLL input is 24 MHz)

// DEVCFG1
#pragma config FNOSC = FRCDIV           // Oscillator Selection Bits (Fast RC Osc w/Div-by-N (FRCDIV))
#pragma config DMTINTV = WIN_127_128    // DMT Count Window Interval (Window/Interval value is 127/128 counter value)
#pragma config FSOSCEN = ON             // Secondary Oscillator Enable (Enable SOSC)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FCKSM = CSECME           // Clock Switching and Monitor Selection (Clock Switch Enabled, FSCM Enabled)
#pragma config WDTPS = PS8192          // Watchdog Timer Postscaler (1:8192)
  // circa 6-7 secondi, 24.7.19
#pragma config WDTSPGM = STOP           // Watchdog Timer Stop During Flash Programming (WDT stops during Flash programming)
#pragma config WINDIS = NORMAL          // Watchdog Timer Window Mode (Watchdog Timer is in non-Window mode)
#pragma config FWDTEN = ON             // Watchdog Timer Enable (WDT Enabled)
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window size is 25%)
#pragma config DMTCNT = DMT31           // Deadman Timer Count Selection (2^31 (2147483648))
#pragma config FDMTEN = OFF             // Deadman Timer Enable (Deadman Timer is disabled)

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Disabled)
#ifdef ILI9341      // board arduino
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#else
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#endif
#pragma config TRCEN = OFF              // Trace Enable (Trace features in the CPU are disabled)
#pragma config BOOTISA = MIPS32         // Boot ISA Selection (Boot code and Exception code is MIPS32)
#pragma config FECCCON = OFF_UNLOCKED   // Dynamic Flash ECC Configuration (ECC and Dynamic ECC are disabled (ECCCON bits are writable))
#pragma config FSLEEP = OFF             // Flash Sleep Mode (Flash is powered down when the device is in Sleep mode)
#pragma config DBGPER = PG_ALL          // Debug Mode CPU Access Permission (Allow CPU access to all permission regions)
#pragma config SMCLR = MCLR_NORM        // Soft Master Clear Enable bit (MCLR pin generates a normal system Reset)
#pragma config SOSCGAIN = GAIN_2X       // Secondary Oscillator Gain Control bits (2x gain setting)
#pragma config SOSCBOOST = ON           // Secondary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config POSCGAIN = GAIN_2X       // Primary Oscillator Gain Control bits (2x gain setting)
#pragma config POSCBOOST = ON           // Primary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config EJTAGBEN = NORMAL        // EJTAG Boot (Normal EJTAG functionality)

// DEVCP0
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// SEQ3

// DEVADC0

// DEVADC1

// DEVADC2

// DEVADC3

// DEVADC4

// DEVADC7



const char CopyrightString[]= {'P','C','/','X','T',' ','E','m','u','l','a','t','o','r',' ','v',
	VERNUMH+'0','.',VERNUML/10+'0',(VERNUML % 10)+'0',' ','-',' ', '0','2','/','0','9','/','2','5', 0 };

const char Copyr1[]="(C) Dario's Automation 2019-2025 - G.Dar\xd\xa\x0";


// Global Variables:
extern BOOL fExit,debug;
extern BYTE CPUPins;
extern BYTE ColdReset;
extern BYTE ram_seg[];
extern BYTE CGAram[],VGAram[];
extern BYTE CGAreg[16];
extern BYTE i8237RegR[16],i8237RegW[16],i8237Reg2R[16],i8237Reg2W[16];
extern BYTE i8259RegR[2],i8259RegW[2],i8259Reg2R[2],i8259Reg2W[2];
extern BYTE i8253RegR[4],i8253RegW[4],i8253Mode[3],i8253ModeSel;
extern WORD i8253TimerR[3],i8253TimerW[3];
extern BYTE i8250Reg[8],i8250Regalt[3];
extern BYTE i6845RegR[18],i6845RegW[18];
extern BYTE i146818RegR[2],i146818RegW[2],i146818RAM[64];
extern BYTE i8042RegR[2],i8042RegW[2],KBRAM[32],KBCommand,KBStatus;
extern BYTE i8259IRR,i8259IMR;
extern uint8_t i8255RegW[];
extern uint8_t FloppyContrRegR[8],FloppyContrRegW[8],FloppyFIFO[16],FloppyFIFOPtr;
extern volatile BYTE Keyboard[];
extern BYTE mouseState;
int8_t mouseX,mouseY;
extern BYTE COMDataEmuFifo[3];
extern BYTE COMDataEmuFifoCnt;
extern volatile BYTE VIDIRQ /*TIMIRQ,KBDIRQ,SERIRQ,RTCIRQ,FDCIRQ*/;
uint16_t VICRaster;

volatile PIC32_RTCC_DATE currentDate={1 /*mon*/,1,1,0x90};    // BCD!
volatile PIC32_RTCC_TIME currentTime={0,0,0};   // BCD
const BYTE dayOfMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};


const WORD cgaColors[3][4]={ {BLACK,CYAN,MAGENTA,WHITE},
  {BLACK,RED,GREEN,YELLOW},
  {BLACK,RED,CYAN,WHITE} };
// cga_colors[4] = {0 /* Black */, 0x1F1F /* Cyan */, 0xE3E3 /* Magenta */, 0xFFFF /* White */
const WORD extCGAColors[16]={BLACK,GREEN,BLUE,CYAN,BURLYWOOD /*CRIMSON*/,DARKGRAY,MAGENTA,BRIGHTMAGENTA /*VIOLET*/,
	GRAY128,LIGHTGREEN,LIGHTGRAY,BRIGHTCYAN,LIGHTRED,YELLOW,PINK,WHITE};
const WORD textColors[16]={BLACK,BLUE,GREEN,CYAN, RED,MAGENTA,YELLOW,LIGHTGRAY,
	GRAY128,BRIGHTBLUE,BRIGHTGREEN,BRIGHTCYAN, BRIGHTRED,BRIGHTMAGENTA,BRIGHTYELLOW,WHITE};


#ifdef ST7735
//#define REAL_SIZE 1
sistemare come in ili9341! 2024
int UpdateScreen(uint16_t rowIni, uint16_t rowFin) {
	int k,y1,y2,x1,x2,row1,row2;
  uint16_t color;
  BYTE ch;
	register int i,j;
	register BYTE *p,*p1;
 	UINT16 px,py;
  static BYTE cursorState=0,cursorDivider=0;

  if(rowIni==0)
    CGAreg[0xa] &= ~0b1000; // CGA not in retrace V
  else if(rowIni==VERT_SIZE-8)
    CGAreg[0xa] |= 0b1000; // CGA in retrace V
  
  if(CGAreg[1] & 8) {     // enable video
    if(CGAreg[1] & 2) {     // graphic mode
      row1=rowIni;
      row2=rowFin;
      if(CGAreg[1] & 16) {     // lores graph (320x200x4)
        if(CGAreg[9] & 16)      // bright foreground
          ;
        START_WRITE();
/* NON C'ERA QUA!! v. windows... mettere,,,
        if(rowIni==0) {
		      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
					pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//          setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//              writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
							*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
              }
            }
          }*/
        setAddrWindow(0+HORIZ_OFFSCREEN,rowIni/2+VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,(rowFin-rowIni)/2);
        p1=CGAram + (rowIni*80);
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
        if(CGAreg[1] & 4)      // disable color ovvero palette #3
          color=2;     // quale palette
        else
          color=CGAreg[9] & 32 ? 1 : 0;     // quale palette
        for(py=rowIni; py<rowFin /*_height*/; py+=2) {    // 200 linee diventa 100 O MEGLIO 125
          for(px=0; px<80 /*_width*/; px++) {         // 320 pixel (80byte) diventano 160
            ch=*p1++;
            writedata16(cgaColors[color][ch >> 6]);
            writedata16(cgaColors[color][(ch >> 2) & 3]);
            }
    #ifdef USA_SPI_HW
          ClrWdt();
    #endif
          p1+=80;    // salto 1 riga
          }
        END_WRITE();
      //	writecommand(CMD_NOP);
        
        }
      else {                  // hires graph (640x200x1)
	      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE... VERIFICARE 2024 (non c'era...
//        (CGAreg[9] & 0xf)      // colori per overscan/background/hires color
        if(CGAreg[1] & 4) {     // modo 160x200 16 colori
          START_WRITE();
          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni/2 /*+VERT_OFFSCREEN*/,_width-HORIZ_OFFSCREEN,(rowFin-rowIni)/2 +VERT_OFFSCREEN);
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<80; px++) {         // 160 pixel
              writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              }
            }
          p1=CGAram + (rowIni*80);
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
          for(py=rowIni; py<rowFin /*_height*/; py+=2) {    // 200 linee diventa 100 O MEGLIO 125
            for(px=0; px<80 /*_width*/; px++) {         // 160 pixel (80byte) 
              ch=*p1++;
              writedata16x2(textColors[ch >> 4],textColors[ch & 0xf]);
              }
      #ifdef USA_SPI_HW
            ClrWdt();
      #endif
            p1+=80;    // salto 1 riga
            }
          END_WRITE();
        //	writecommand(CMD_NOP);
          }
        else {
          START_WRITE();
          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni/2 /*+VERT_OFFSCREEN*/,_width-HORIZ_OFFSCREEN,(rowFin-rowIni)/2 +VERT_OFFSCREEN);
          p1=CGAram + (rowIni*80);
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
          if(CGAreg[1] & 4)      // disable color ovvero palette #3 QUA NON SI CAPISCE COSA DOVREBBE FARE!
            color=15;
          else
            color=CGAreg[9] & 15;
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<80; px++) {         // 160 pixel
              writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              }
            }
          for(py=rowIni; py<rowFin /*_height*/; py+=2) {    // 200 linee diventa 100 O MEGLIO 125
            for(px=0; px<80 /*_width*/; px++) {         // 640 pixel (80byte) diventano 160
              ch=*p1++;
              if(ch & 0x80)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              if(ch & 0x20)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              if(ch & 0x8)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              if(ch & 0x2)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              }
      #ifdef USA_SPI_HW
            ClrWdt();
      #endif
            p1+=80;    // salto 1 riga
            }
          END_WRITE();
        //	writecommand(CMD_NOP);
          }
        }
      }
    else {                // text mode
      START_WRITE();
      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
      setAddrWindow(HORIZ_OFFSCREEN+0,rowIni/2 /*+VERT_OFFSCREEN*/,_width-HORIZ_OFFSCREEN,(rowFin-rowIni)/2 +VERT_OFFSCREEN+VERT_OFFSCREEN);
      for(py=0; py<VERT_OFFSCREEN; py++) {    // 
        for(px=0; px<80; px++) {         // 160 pixel
          writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
          }
        }
//#define REAL_SIZE 1
#ifdef REAL_SIZE
      for(py=0; py<12; py++) {    // 
        if(CGAreg[1] & 1) {     // 80x25
          for(i=0; i<8; i++) {         // 
            p1=CGAram + (rowIni*80/8) + (py*80*2);    // char/colore
            for(px=0; px<20; px++) {         // 20 char diventano 160 pixel
              ch=*p1++;
              p=&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x80)   // 8 pixel
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x40)   // 8 pixel
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x20)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x10)   // 
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x8)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x4)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x2)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x1)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              }
            }
          }
        else {     // 40x25
          for(i=0; i<8; i++) {         // 
            p1=CGAram + (rowIni*80/8) + (py*40*2);    // char/colore
            for(px=0; px<20; px++) {         // 20 char diventano 160 pixel
              ch=*p1++;
              p=&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x80)   // 8 pixel
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x40)   // 8 pixel
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x20)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x10)   // 
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x8)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x4)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x2)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x1)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              }
            }
          }
  #ifdef USA_SPI_HW
        ClrWdt();
  #endif
        }
#else      
      for(py=0; py<25; py++) {    // 
        if(CGAreg[1] & 1) {     // 80x25
          for(i=0; i<8; i+=2) {         // 8 righe diventano 4
            p1=CGAram + (rowIni*80/8) + (py*80*2);    // char/colore
#warning            non dovrebbe essere 160?? 2024
            for(px=0; px<80; px++) {         // 80 char (80byte) diventano 160 pixel
              ch=*p1++;
              p=&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x40)   // difficile scegliere quali 2 pixel prendere.. !
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x4)   // difficile scegliere quali 2 pixel prendere.. !
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              }
            }
          }
        else {     // 40x25
          for(i=0; i<8; i+=2) {         // 8 righe diventano 4
            p1=CGAram + (rowIni*80/8) + (py*40*2);    // char/colore
            for(px=0; px<40; px++) {         // 40 char (40byte) diventano 160 pixel
              ch=*p1++;
              p=&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x80)   // 4 pixel
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x20)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x8)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x2)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              }
            }
          }
  #ifdef USA_SPI_HW
        ClrWdt();
  #endif
        }
#endif
      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
      for(py=0; py<VERT_OFFSCREEN; py++) {    // 
        for(px=0; px<80; px++) {         // 160 pixel
          writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
          }
        }
      END_WRITE();

      i=MAKEWORD(i6845RegW[15],i6845RegW[14]);    // coord cursore, abs
      row1=CGAreg[8] & 1 ? (i/80)*8 : (i/40)*8;
      BYTE cy1,cy2;
      switch((i6845RegW[10] & 0x60)) {
        case 0:
					// secondo GLABios, in CGA il cursore lampeggia sempre indipendentemente da questo valore... me ne frego!!
plot_cursor:
// test          i6845RegW[10]=5;i6845RegW[11]=7;
          cy1=(i6845RegW[10] & /*0x1f*/ 7)/2;    // 0..32 ma in effetti sono "reali" per cui di solito 6..7!
          cy2=(i6845RegW[11] & /*0x1f*/ 7)/2;
          if(cy2==cy1) cy2++;
          i=MAKEWORD(i6845RegW[15],i6845RegW[14]);    // coord cursore, abs
          if(CGAreg[1] & 1) {     // 80x25
            color=7;    // fisso :)
            START_WRITE();
            setAddrWindow((i*2) % 80,1+ /* ?? boh*/ cy1+ (i/80)*4 +VERT_OFFSCREEN,2,cy2-cy1);   // altezza e posizione fissa, gestire da i6845RegW[10,11]
            writedata16x2(textColors[color],textColors[color]);
//            writedata16x2(textColors[color],textColors[color]);
            END_WRITE();
            }
          else {
            color=7;    // fisso :)
            START_WRITE();
            setAddrWindow((i*4) % 40,1+ /* ?? boh*/ cy1+ (i/40)*4 +VERT_OFFSCREEN,4,cy2-cy1);
            writedata16x2(textColors[color],textColors[color]);
            writedata16x2(textColors[color],textColors[color]);
//            writedata16x2(textColors[color],textColors[color]);
//            writedata16x2(textColors[color],textColors[color]);
            END_WRITE();
            }
          break;
        case 0x20:
          break;
        case 0x40:
          cursorDivider++;
          if(cursorDivider>=4) {
            cursorDivider=0;
            cursorState=!cursorState;
            }
          if(cursorState) {
            goto plot_cursor;
            }
          break;
        case 0x60:
          cursorDivider++;
          if(cursorDivider>=2) {
            cursorDivider=0;
            cursorState=!cursorState;
            }
          if(cursorState) {
            goto plot_cursor;
            }
          break;

        }
    //	writecommand(CMD_NOP);
      }
    
    }

	}
#endif
#ifdef ILI9341
//#define REAL_SIZE 1
int UpdateScreen(int rowIni, int rowFin) {
	int k,y1,y2,x1,x2,row1,row2;
  uint8_t color;
  BYTE ch;
	register int i,j;
	register BYTE *p,*p1;
 	int px,py;
  static BYTE cursorState=0,cursorDivider=0;

#ifdef CGA  
  if(rowIni==0)
    CGAreg[0xa] &= ~0b1000; // CGA not in retrace V
  else if(rowFin>=VERT_SIZE-8)
    CGAreg[0xa] |= 0b1000; // CGA in retrace V
  
  if(CGAreg[8] & 8) {     // enable video
    if(CGAreg[8] & 2) {     // graphic mode
      if(!(CGAreg[8] & 16)) {     // lores graph (320x200x4)
        if(CGAreg[9] & 16)      // bright foreground
          ;
        START_WRITE();
        if(rowIni==0) {
		      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
          setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<160; px++) {         // 320 pixel
              writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              }
            }
          }
        setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,(rowFin-rowIni));
        p1=CGAram + (rowIni*80);
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
        if(CGAreg[8] & 4)      // disable color ovvero palette #3
          color=2;     // quale palette
        else
          color=CGAreg[9] & 32 ? 1 : 0;     // quale palette
        if(CGAreg[9] & 16)      // high intensity foreground color...
					;
        for(py=rowIni; py<rowFin; py++) {    // 
					p1=(BYTE*)&CGAram[0] + (((py / 2))*80) + (py & 1 ? 8192 : 0);
          for(px=0; px<80; px++) {         // 320 pixel (80byte) 
            ch=*p1++;
            writedata16(cgaColors[color][ch >> 6]);
            writedata16(cgaColors[color][(ch >> 4) & 3]);
            writedata16(cgaColors[color][(ch >> 2) & 3]);
            writedata16(cgaColors[color][(ch >> 0) & 3]);
            }
          ClrWdt();
          }
        if(rowIni>=VERT_SIZE-8) {
          color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
          setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<160; px++) {         // 320 pixel
              writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              }
            }
          }
        END_WRITE();
        }
      else {                  // hires graph (640x200x1)
	      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE... VERIFICARE 2024 (non c'era...
//        (CGAreg[9] & 0xf)      // colori per overscan/background/hires color
        if(!(CGAreg[8] & 16)) {     // modo 160x200 16 colori   
          START_WRITE();
          if(rowIni==0) {
            setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
              for(px=0; px<160; px++) {         // 320 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
                }
              }
            }
          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,(rowFin-rowIni));
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
          for(py=rowIni; py<rowFin; py++) {    // 
						p1=(BYTE*)&CGAram[0] + (((py / 2))*80) + (py & 1 ? 8192 : 0);
            for(px=0; px<80; px++) {         // 160 pixel (80byte) 
              ch=*p1++;
              writedata16x2(textColors[ch >> 4],textColors[ch >> 4]);
              writedata16x2(textColors[ch & 0xf],textColors[ch & 0xf]);    // 160 -> 320
              }
            ClrWdt();
            }
          if(rowIni>=VERT_SIZE-8) {
            color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
            setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
              for(px=0; px<160; px++) {         // 320 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
                }
              }
            }
          END_WRITE();
          }
        else {
          if(CGAreg[9] & 4)      // disable color ovvero palette #3 QUA NON SI CAPISCE COSA DOVREBBE FARE!
            color=15;
          else
            color=CGAreg[9] & 15;
          START_WRITE();
          if(rowIni==0) {
            setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
              for(px=0; px<160; px++) {         // 320 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
                }
              }
            }
          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,(rowFin-rowIni));
          for(py=rowIni; py<rowFin; py++) {    // 
						p1=(BYTE*)&CGAram[0] + (((py / 2))*(HORIZ_SIZE/8)) + (py & 1 ? 8192 : 0);
            for(px=0; px<80; px++) {         // 640 pixel (80byte) diventano 320
              ch=*p1++;
              if(ch & 0x80)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              if(ch & 0x20)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              if(ch & 0x8)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              if(ch & 0x2)
                writedata16(textColors[color]);
              else
                writedata16(textColors[0]);
              }
            ClrWdt();
            }
          if(rowIni>=VERT_SIZE-8) {
            color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
            setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
              for(px=0; px<160; px++) {         // 320 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
                }
              }
            }
          END_WRITE();
          }
        }
      }
    else {                // text mode (da qualche parte potrebbe esserci la Pagina selezionata...
      START_WRITE();
      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
      if(rowIni==0) {
        setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
          for(px=0; px<160; px++) {         // 320 pixel
            writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
            }
          }
        }
      setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,(rowFin-rowIni));
      for(py=rowIni/8; py<rowFin/8; py++) {    // 
        if(CGAreg[8] & 1) {     // 80x25
          for(i=0; i<8; i++) {         // 8 righe 
						p1=(BYTE*)&CGAram[0] + (py*80*2)   + 2*MAKEWORD(i6845RegW[13],i6845RegW[12]) /* display start addr*/;    // char/colore
//#warning            era 80?? 2024 ah ma tanto rowini è 0...
            for(px=0; px<80; px++) {         // 80 char (80byte) diventano 320 pixel
              ch=*p1++;
              p=(BYTE *)&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x40)   // difficile scegliere quali 2 pixel prendere.. !
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x10)   // difficile scegliere quali 2 pixel prendere.. !
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x4)   // difficile scegliere quali 2 pixel prendere.. !
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x1)   // difficile scegliere quali 2 pixel prendere.. !
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              }
            }
          }
        else {     // 40x25
          for(i=0; i<8; i++) {         // 8 righe 
  					p1=(BYTE*)&CGAram[0] + (py*40*2)   + 2*MAKEWORD(i6845RegW[13],i6845RegW[12]) /* display start addr*/;    // char/colore
            for(px=0; px<40; px++) {         // 40 char (40byte) diventano 320 pixel
              ch=*p1++;
              p=(BYTE *)&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x80)   // 8 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x40)
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);    // 
              if(ch & 0x20)
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);
              if(ch & 0x10)
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);
              if(ch & 0x8)
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);
              if(ch & 0x4)
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);
              if(ch & 0x2)
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);
              if(ch & 0x1)
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              else
                writedata16x2(textColors[color >> 4],textColors[color >> 4]);
              }
            }
          }
        ClrWdt();
        }

      if(rowIni==VERT_SIZE-8) {
        color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
        setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-HORIZ_OFFSCREEN,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
          for(px=0; px<160; px++) {         // 320 pixel
            writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
            }
          }
        }
      END_WRITE();

      i=MAKEWORD(i6845RegW[15],i6845RegW[14]);    // coord cursore, abs
      row1=CGAreg[8] & 1 ? (i/80)*8 : (i/40)*8;
      if(row1>=rowIni && row1<rowFin) {
        BYTE cy1,cy2;
				row1--;			// è la pos della prima riga in alto del char contenente il cursore
        switch(i6845RegW[10] & 0x60) {
          case 0:
					// secondo GLABios, in CGA il cursore lampeggia sempre indipendentemente da questo valore... me ne frego!!
plot_cursor:
  // test          i6845RegW[10]=5;i6845RegW[11]=7;
            cy1= (i6845RegW[10] & /*0x1f*/ 7);    // 0..32 ma in effetti sono "reali" per cui di solito 6..7!
            cy2= (i6845RegW[11] & /*0x1f*/ 7);
            if(cy2 && cy1<=cy2) do {
              if(CGAreg[8] & 1) {     // 80x25
                color=7;    // fisso :)
                START_WRITE();
                setAddrWindow((i % 80)*4+(HORIZ_OFFSCREEN/2),1+ /* ?? boh*/ cy1+ row1 +VERT_OFFSCREEN,4,cy2-cy1);   // altezza e posizione fissa, gestire da i6845RegW[10,11]
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                END_WRITE();
                }
              else {
                color=7;    // fisso :)
                START_WRITE();
                setAddrWindow((i % 40)*8+(HORIZ_OFFSCREEN/2),1+ /* ?? boh*/ cy1+ row1 +VERT_OFFSCREEN,8,cy2-cy1);
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                END_WRITE();
                }
							cy1++;
							} while(cy1<=cy2);
            break;
          case 0x20:    // no cursor!
            break;
          case 0x40:
            cursorDivider++;
            if(cursorDivider>=2) {
              cursorDivider=0;
              cursorState=!cursorState;
              }
            if(cursorState) {
              goto plot_cursor;
              }
            break;
          case 0x60:
            cursorDivider++;
            if(cursorDivider>=1) {
              cursorDivider=0;
              cursorState=!cursorState;
              }
            if(cursorState) {
              goto plot_cursor;
              }
            break;
          }
        }

      }
    }
#endif
  
#ifdef VGA
	uint8_t *pVGARam;
	uint8_t mask;
	uint32_t offset;

  rowFin=min(rowFin,VERT_SIZE);
  if(rowIni==0) {
    CGAreg[0xa] &= ~B8(00001000); // CGA not in retrace V
//		VGAstatus0 |= B8(10000000);			
//		VGAstatus1 &= ~B8(00001000);
		}
  else if(rowFin>=VERT_SIZE-8) {
    CGAreg[0xa] |= B8(00001000); // CGA in retrace V
//		VGAstatus0 &= ~B8(10000000);			
//		VGAstatus1 |= B8(00001000);
		}
  

//	i=VGAcrtcReg[8];
//	VGAseqReg[0] b0 e b1 sono reset sequencer

//	vmode = VGAcrtcReg[0x5];			// https://www.artikel33.com/informatik/1/vgaregister.php
//	vmode = CGAreg[8] /* 8 | 1*/;		// http://www.antonis.de/qbebooks/gwbasman/screens.html per test modi screen gwbasic

//	VGAcrtcReg[0x17] CRTC Mode passa da a3h text a e3h graph
	// gli altri fanno un po' come cazzo ci pare
//  VGAgraphReg[5] GDC Mode passa da 10h a 68h (256 color mode, B6 si alza; con Mode 13h diventa 40h
//  VGAgraphReg[4] Read plane select passa da 00h a 68h
//  VGAgraphReg[6]   passa da 0Eh a 05h (graph mode B1 si alza, memory mode è 01 ossia 64KB partendo da A0000
//	VGAseqReg[1] Dot 8/9 passa da 0 a 1 
//	VGAseqReg[2] Map 0 Enable passa da 03 a 0f (da 2 map a 4, pare
//	VGAseqReg[4]   passa da 03 a 06


	test_dump_vga_reg();


  if(!(VGAseqReg[1] & 0x20)) {     // enable video
    if(VGAgraphReg[6] & 1) {     // graphic mode
			switch(VGAgraphReg[6] & 0x0c) {
				case 0x00:
				case 0x04:
					pVGARam=&VGAram[0];
					offset=0x10000;
					break;
				case 0x08:
					pVGARam=&VGAram[0]+0x10000L;
					offset=0x2000;
					break;
				case 0x0c:
					pVGARam=&VGAram[0]+0x18000L;
					offset=0x2000;
					break;
				}
//    anche  if(VGAactlReg[0x10] & 1) {     // graphic mode
			if(VGAgraphReg[5] & 0x40 /* usare altro ma per ora è unico a 256color!*/) {			// modo 13h, 320*200*256 su 256K
//    anche  if(VGAactlReg[0x10] & 0x40) {     // graphic mode
				pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((640)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
	//          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
				for(py=rowIni; py<rowFin; py++) {    // modo 13h
					for(i=0; i<2; i++) {
						p1=(BYTE*)pVGARam + (((py ))*320) ;		// 
						for(px=0; px<320; px++) {         // 320 pixel (320byte) 
							ch=*p1++;
							*pVideoRAM=(ch << 4);
	//						ch=*p1++;
							*pVideoRAM++ |= (ch & 0xf);
							}
						}
					}
				}
			else {			// 
//				if(!(VGAreg[2] & 0x80)) {		// questo  è 1 per i modi 2 colori e 0 a 16, ma non lo trovo nel doc... forse sbaglio qualcosa
						// e3 mode 12h, 6ah
						// a3 mode 10h
						// 63 mode 0dh e 06h
						// ma cmq non va su Mode 6, 640x200x2
//				if(!(VGAgraphReg[6] & 0x8)) {		//questo va basso su modo 6 ma ANCHE su modo 11h... ovvio,è memory size
//			if(VGAgraphReg[5] & 0x20 // questo va a 1 per i modi CGA, even/odd memory maps
				switch(VGAseqReg[2] & 0xf) {		// uso questo, bit planes, anche se non mi fa impazzire come idea
					case 15:
						// 0Dh, 320x200x16, 
						i=VGAcrtcReg[9] & 0x80 ? 640 : 320;
						pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((i)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
						switch(VGAcrtcReg[1]) {
							case 99:		// 6Ah; horiz display end, 99 per 800/600
								pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((320)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
								// finire!!
								for(py=rowIni; py<rowFin; py++) {    // 
									p1=(BYTE*)pVGARam + (py*100) ;		// 
									for(px=0; px<80; px++) {         // 640 pixel PER ORA taglio
										for(mask=0x80; mask; mask>>=1) {
											if(mask & 0x55) {
												ch=*(p1+0);
												*pVideoRAM |= ch & mask ? 1 : 0;
												ch=*(p1+offset);
												*pVideoRAM |= ch & mask ? 2 : 0;
												ch=*(p1+offset*2);
												*pVideoRAM |= ch & mask ? 4 : 0;
												ch=*(p1+offset*3);
												*pVideoRAM++ |= ch & mask ? 8 : 0;
												}
											else {
												ch=*(p1+0);
												*pVideoRAM = ch & mask ? 0x10 : 0;
												ch=*(p1+offset);
												*pVideoRAM |= ch & mask ? 0x20 : 0;
												ch=*(p1+offset*2);
												*pVideoRAM |= ch & mask ? 0x40 : 0;
												ch=*(p1+offset*3);
												*pVideoRAM |= ch & mask ? 0x80 : 0;
												}
											}
										p1++;
										}
// non va									if(!(py % 5))
//										pVideoRAM-=640;
									}
								break;
							case 79:		// 11h; horiz display end, 79 o 39 per 640/320; sarebbero 480 ma ok
								for(py=rowIni; py<rowFin; py++) {    // mode 11h
									p1=(BYTE*)pVGARam + (py*80) ;		// 
									for(px=0; px<80; px++) {         // 640 pixel 
										for(mask=0x80; mask; mask>>=1) {
											if(mask & 0x55) {
												ch=*(p1+0);
												*pVideoRAM |= ch & mask ? 1 : 0;
												ch=*(p1+offset);
												*pVideoRAM |= ch & mask ? 2 : 0;
												ch=*(p1+offset*2);
												*pVideoRAM |= ch & mask ? 4 : 0;
												ch=*(p1+offset*3);
												*pVideoRAM++ |= ch & mask ? 8 : 0;
												}
											else {
												ch=*(p1+0);
												*pVideoRAM = ch & mask ? 0x10 : 0;
												ch=*(p1+offset);
												*pVideoRAM |= ch & mask ? 0x20 : 0;
												ch=*(p1+offset*2);
												*pVideoRAM |= ch & mask ? 0x40 : 0;
												ch=*(p1+offset*3);
												*pVideoRAM |= ch & mask ? 0x80 : 0;
												}
											}
										p1++;
										}
									if(VGAcrtcReg[9] & 0x80) {		// raddoppio se 200 linee (+ veloce così
										memcpy(pVideoRAM,pVideoRAM-320,320);
										pVideoRAM+=320;
										}
									}
								break;
							case 39:
								for(py=rowIni; py<rowFin; py++) {    // 
									p1=(BYTE*)pVGARam + (py*40) ;		// 
									for(px=0; px<40; px++) {         // 640 pixel 
										for(mask=0x80; mask; mask>>=1) {
											ch=*(p1+0);
											*pVideoRAM = ch & mask ? 1 : 0;
											ch=*(p1+offset);
											*pVideoRAM |= ch & mask ? 2 : 0;
											ch=*(p1+offset*2);
											*pVideoRAM |= ch & mask ? 4 : 0;
											ch=*(p1+offset*3);
											*pVideoRAM |= ch & mask ? 8 : 0;
											*pVideoRAM++ |= *pVideoRAM << 4;
											}
										p1++;
										}
									if(VGAcrtcReg[9] & 0x80) {		// raddoppio se 200 linee (+ veloce così
										memcpy(pVideoRAM,pVideoRAM-320,320);
										pVideoRAM+=320;
										}
									}
								break;
							}
						break;
					case 3:			// 4 colori, mode 4,5
/* gloriouscow dice che qua non esiste        if(CGAreg[8] & 4)      // disable color ovvero palette #3
          color=2;     // quale palette
        else*/
						color=CGAreg[9] & B8(00100000) ? 1 : 0;
						pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((640)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
						switch(VGAcrtcReg[1]) {
							case 39:		// mode 4,5 : 320x200x4
									/*{int myFile;
								myFile=_lcreat("vgaram.bin",0);
								if(myFile != -1) {
									_lwrite(myFile,VGAram,262144);
									_lclose(myFile);
									}
									}*/
									// v. CGA 4 color https://www.usebox.net/jjm/notes/cga/
								for(py=rowIni; py<rowFin; py++) {    // 
									p1=(BYTE*)pVGARam + ((py/2)*40) +(py & 1 ? 8192/*CGA emu*//2 : 0);// bit planes 0 and 1 in mode 6 are used to store even and odd bytes of the data, non è proprio così, verificare
									for(px=0; px<40; px++) {         // 320 pixel 
										BYTE c;

										ch=*p1;
										*pVideoRAM++=(cgaColors[color][ch >> 6] << 4) | cgaColors[color][((ch >> 6) & 3)];
										*pVideoRAM++=(cgaColors[color][(ch >> 4) & 3] << 4) | cgaColors[color][((ch >> 4) & 3)];
										*pVideoRAM++=(cgaColors[color][(ch >> 2) & 3] << 4) | cgaColors[color][((ch >> 2)) & 3];
										*pVideoRAM++=(cgaColors[color][(ch >> 0) & 3] << 4) | cgaColors[color][(ch ) & 3];
										ch=*(p1+offset);
										*pVideoRAM++=(cgaColors[color][ch >> 6] << 4) | cgaColors[color][((ch >> 6) & 3)];
										*pVideoRAM++=(cgaColors[color][(ch >> 4) & 3] << 4) | cgaColors[color][((ch >> 4) & 3)];
										*pVideoRAM++=(cgaColors[color][(ch >> 2) & 3] << 4) | cgaColors[color][((ch >> 2)) & 3];
										*pVideoRAM++=(cgaColors[color][(ch >> 0) & 3] << 4) | cgaColors[color][(ch ) & 3];
										
										p1++;
										}
									memcpy(pVideoRAM,pVideoRAM-320,320);
									pVideoRAM+=320;
									}
																/*	{int myFile;
								myFile=_lcreat("VideoRAM.bin",0);
								if(myFile != -1) {
									_lwrite(myFile,VideoRAM,320*480);
									_lclose(myFile);
									}
									}*/
								break;
							}
						break;
					case 1:
						color=0x0f;			// black / white
	//				if(!(VGAseqReg[1] & 0x08)) {		//  questo è dot clock =1 se 320 horiz...
						switch(VGAcrtcReg[1]) {
							case 79:		// 11h NO! esce sopra; horiz display end, 79 o 39 per 640/320; anche 0F che sarebbe 350 linee ma ok; anche 06h
						// usare VGAcrtcReg[9] & 0x80 per 200/400 linee (1=200
								i=VGAcrtcReg[9] & 0x80 ? 640 : 320;
								pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((i)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
								// mah è ok (400 invece di 480 ma ok) 
								for(py=rowIni; py<rowFin; py++) {    // 
									p1=(BYTE*)pVGARam + ((py/2)*80) +(py & 1 ? offset : 0);		// bit planes 0 and 1 in mode 6 are used to store even and odd bytes of the data, non è proprio così, verificare
									for(px=0; px<80; px++) {         // 640 pixel
										ch=*p1++;
										if(ch & 0x80)   // 
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x40)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										if(ch & 0x20)
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x10)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										if(ch & 0x08)
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x4)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										if(ch & 0x2) 
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x1)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										}
									if(VGAcrtcReg[9] & 0x80) {		// raddoppio se 200 linee (+ veloce così
										memcpy(pVideoRAM,pVideoRAM-320,320);
										pVideoRAM+=320;
										}
									}
								break;
							case 39:		// modo 4 opp 5, mah questo non esiste così, c'è 320x200x16 e non b/n
								i=VGAcrtcReg[9] & 0x80 ? 640 : 320;
								pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((i)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
								// mah è ok (200 invece di 240 ma ok) 
								for(py=rowIni; py<rowFin; py++) {    // 
									p1=(BYTE*)pVGARam + (py*40) ;		// 
									for(px=0; px<40; px++) {         // 320 pixel (40byte) 
										ch=*p1++;
										if(ch & 0x80)   // 
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x40)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										if(ch & 0x20)
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x10)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										if(ch & 0x08)
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x4)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										if(ch & 0x2) 
											*pVideoRAM=(color & 0xf) << 4;
										else
											*pVideoRAM=(color & 0xf0);
										if(ch & 0x1)   // 
											*pVideoRAM++ |= (color & 0xf);
										else
											*pVideoRAM++ |= (color & 0xf0) >> 4;
										}
									if(VGAcrtcReg[9] & 0x80) {		// raddoppio se 200 linee (+ veloce così
										memcpy(pVideoRAM,pVideoRAM-320,320);
										pVideoRAM+=320;
										}
									}
								break;
							}
						break;
					}
	      }
      }
    else {                // text mode (da qualche parte potrebbe esserci la Pagina selezionata...
			switch(VGAgraphReg[6] & 0x0c) {
				case 0x00:
				case 0x04:		// beh non capita, direi
					pVGARam=&VGAram[0];
					offset=0x10000;
					break;
				case 0x08:
					pVGARam=&VGAram[0]+0x10000L;
					offset=0x2000;
					break;
				case 0x0c:
					pVGARam=&VGAram[0]+0x18000L;
					offset=0x2000;
					break;
				}
      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
			//    anche  VGAactlReg[0x11] ma non palette

      if(rowIni==0) {
				pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
						}
					}
				}
			pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
      for(py=rowIni/16; py<rowFin/16; py++) {    // 
//    vedere VGAactlReg[0x10] b3 e b2 per intensity e 9th bit
        if(VGAcrtcReg[0x2] == 80) {     // 80x25
          for(i=0; i<16; i++) {         // 16 righe 
						p1=(BYTE*)pVGARam + (py*80)   ;    // char/colore
//#warning            era 80?? 2024 ah ma tanto rowini è 0...
            for(px=0; px<HORIZ_SIZE/8; px++) {         // 80 char (80byte) diventano 640 pixel
              ch=*p1;
//              p=(BYTE *)&VGAfont_16[((WORD)ch)*16+i];  // TROVARLI IN qualche registro! sono nella ROM BIOS VGA
//              p=(BYTE *)&VGABios[0x2526 + ((WORD)ch)*16+i];  // TROVARLI IN qualche registro! 
              p=(BYTE *)&VGAram[0x20000 + 2*((WORD)ch)*16+i];  // When an alphanumeric mode is selected, the character font patterns are transferred from the ROM to map 2, OCCHIO SONO INTERLEAVATI con qualcosa ;)
							/*{int myFile;
						myFile=_lcreat("vgaram.bin",0);
						if(myFile != -1) {
							_lwrite(myFile,VGAram,262144);
							_lclose(myFile);
							}
							}*/
              ch=*p;
              color=*(p1+offset);   // il colore si trova nel planes seguente
							p1++;
              if(ch & 0x80)
								*pVideoRAM=(color & 0xf) << 4;
              else
								*pVideoRAM=(color & 0xf0);
              if(ch & 0x40)   // 
								*pVideoRAM++ |= (color & 0xf);
              else
								*pVideoRAM++ |= (color & 0xf0) >> 4;
              if(ch & 0x20)
								*pVideoRAM=(color & 0xf) << 4;
              else
								*pVideoRAM=(color & 0xf0);
              if(ch & 0x10)   // 
								*pVideoRAM++ |= (color & 0xf);
              else
								*pVideoRAM++ |= (color & 0xf0) >> 4;
              if(ch & 0x08)
								*pVideoRAM=(color & 0xf) << 4;
              else
								*pVideoRAM=(color & 0xf0);
              if(ch & 0x4)   // 
								*pVideoRAM++ |= (color & 0xf);
              else
								*pVideoRAM++ |= (color & 0xf0) >> 4;
              if(ch & 0x2) 
								*pVideoRAM=(color & 0xf) << 4;
              else
								*pVideoRAM=(color & 0xf0);
              if(ch & 0x1)   // 
//                writedata16(textColors[color & 0xf]);
								*pVideoRAM++ |= (color & 0xf);
              else
//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
								*pVideoRAM++ |= (color & 0xf0) >> 4;
              }
            }
          }
        else {     // 40x25
          for(i=0; i<16; i++) {         // 16 righe 
						p1=(BYTE*)pVGARam + (py*40);    // char/colore
            for(px=0; px<40; px++) {         // 40 char (40byte) diventano 640 pixel
              ch=*p1;
//              p=(BYTE *)&VGAfont_16[((WORD)ch)*16+i];  // TROVARLI IN qualche registro! sono nella ROM BIOS VGA
              p=(BYTE *)&VGAram[0x20000 + 2*((WORD)ch)*16+i];  // When an alphanumeric mode is selected, the character font patterns are transferred from the ROM to map 2, OCCHIO SONO INTERLEAVATI con qualcosa ;)
              ch=*p;
              color=*(p1+offset);   // il colore si trova nel planes seguente
							p1++;
              if(ch & 0x80)   // 8 pixel
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x40)
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x20)
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x10)
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x8)
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x4)
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x2)
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x1)
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              }
            }
          }
        }

      if(rowFin>=VERT_SIZE-8) {
        color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
				pVideoRAM=(BYTE*)&VideoRAM[0]+(VERT_SIZE+VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
            }
          }
				blinkState = !blinkState;		// rallentare!
        }

      i=MAKEWORD(VGAcrtcReg[15],VGAcrtcReg[14] & 0x3f);    // coord cursore, abs
      row1=(VGAcrtcReg[0x2] == 80) ? (i/80)*16 : (i/40)*16;
      if(row1>=rowIni && row1<rowFin) {
        BYTE cy1,cy2;
				row1--;			// è la pos della prima riga in alto del char contenente il cursore
        switch((VGAcrtcReg[10] & 0x20)) {		// questo è solo enable! lampeggio??
          case 0:

plot_cursor:
  // test          i6845RegW[10]=5;i6845RegW[11]=7;
            cy1= (VGAcrtcReg[10] & 31);    // 0..31
            cy2= (VGAcrtcReg[11] & 31);
            if(cy2 && cy1<=cy2) do {
              if(VGAcrtcReg[0x2] == 80) {     // 80x25
                color=7;    // fisso :)
								pVideoRAM=(BYTE*)&VideoRAM[0]+((row1)+cy1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+((i % 80)*4)+(HORIZ_OFFSCREEN/2);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
                }
              else {
                color=7;    // fisso :)
								pVideoRAM=(BYTE*)&VideoRAM[0]+((row1)+cy1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+((i % 40)*8)+(HORIZ_OFFSCREEN/2);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
                }
							cy1++;
							} while(cy1<=cy2);
            break;
          case 0x20:    // no cursor!
            break;
          case 0x40:
            cursorDivider++;
            if(cursorDivider>=2) {
              cursorDivider=0;
              cursorState=!cursorState;
              }
            if(cursorState) {
              goto plot_cursor;
              }
            break;
          case 0x60:
            cursorDivider++;
            if(cursorDivider>=1) {
              cursorDivider=0;
              cursorState=!cursorState;
              }
            if(cursorState) {
              goto plot_cursor;
              }
            break;
          }
        }

      }
		}

#endif
  
	}
#endif


int main(void) {

  // disable JTAG port
//  DDPCONbits.JTAGEN = 0;
  
  SYSKEY = 0x00000000;
  SYSKEY = 0xAA996655;    //qua non dovrebbe servire essendo 1° giro (v. IOLWAY
  SYSKEY = 0x556699AA;
  CFGCONbits.IOLOCK = 0;      // PPS Unlock
  SYSKEY = 0x00000000;
  
  RPB9Rbits.RPB9R = 1;        // Assign RPB9 as U3TX, pin 22 (ok arduino)
// NON SI PUO'... sistemare se serve  U3RXRbits.U3RXR = 2;      // Assign RPB8 as U3RX, pin 21 (ok arduino)
#ifdef ST7735
#ifdef USA_SPI_HW
  RPG8Rbits.RPG8R = 6;        // Assign RPG8 as SDO2, pin 6
//  SDI2Rbits.SDI2R = 1;        // Assign RPG7 as SDI2, pin 5
#endif
  RPD5Rbits.RPD5R = 12;        // Assign RPD5 as OC1, pin 53; anche vaga uscita audio :)
  RPD1Rbits.RPD1R = 12;        // Assign RPD1 as OC1, pin 49; buzzer
#endif
#ifdef ILI9341
  RPB1Rbits.RPB1R = 12;        // Assign RPB1 as OC7, pin 15; buzzer
#endif

//#define DEBUG_TESTREFCLK  
#ifdef DEBUG_TESTREFCLK
// test REFCLK
#ifdef ST7735
  PPSOutput(4,RPC4,REFCLKO2);   // RefClk su pin 1 (RG15, buzzer)
	REFOCONbits.ROSSLP=1;
	REFOCONbits.ROSEL=1;
	REFOCONbits.RODIV=0;
	REFOCONbits.ROON=1;
	TRISFbits.TRISF3=1;
#endif
#ifdef ILI9341
//  RPD9Rbits.RPD9R = 15;        // Assign RPD9 (SDA) as RefClk3, pin 43
  RPB8Rbits.RPB8R = 15;        // Assign RPB8 as RefClk3, pin 21 + comodo
	REFO3CONbits.SIDL=0;
	REFO3CONbits.RSLP=1;
	REFO3CONbits.ROSEL=1;   // PBclk
	REFO3CONbits.RODIV=1;   // :2
	REFO3CONbits.OE=1;
	REFO3CONbits.ON=1;
//	TRISDbits.TRISD9=1;
	TRISBbits.TRISB8=1;
#endif
#endif

  SYSKEY = 0x00000000;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;
  CFGCONbits.IOLOCK = 1;      // PPS Lock
  SYSKEY = 0x00000000;

   // Disable all Interrupts
  __builtin_disable_interrupts();
  
//  SPLLCONbits.PLLMULT=10;
  
  OSCTUN=0;
  OSCCONbits.FRCDIV=0;
  
  // Switch to FRCDIV, SYSCLK=8MHz
  SYSKEY=0xAA996655;
  SYSKEY=0x556699AA;
  OSCCONbits.NOSC=0x00; // FRC
  OSCCONbits.OSWEN=1;
  SYSKEY=0x33333333;
  while(OSCCONbits.OSWEN) {
    Nop();
    }
    // At this point, SYSCLK is ~8MHz derived directly from FRC
 //http://www.microchip.com/forums/m840347.aspx
  // Switch back to FRCPLL, SYSCLK=200MHz
  SYSKEY=0xAA996655;
  SYSKEY=0x556699AA;
  OSCCONbits.NOSC=0x01; // SPLL
  OSCCONbits.OSWEN=1;
  SYSKEY=0x33333333;
  while(OSCCONbits.OSWEN) {
    Nop();
    }
  // At this point, SYSCLK is ~200MHz derived from FRC+PLL
//***
  mySYSTEMConfigPerformance();
  //myINTEnableSystemMultiVectoredInt(();

    
#ifdef ST7735
	TRISB=0b0000000000110000;			// AN4,5 (rb4..5)
	TRISC=0b0000000000000000;
	TRISD=0b0000000000001100;			// 2 pulsanti; buzzer
	TRISE=0b0000000000000000;			// 3 led
	TRISF=0b0000000000000000;			// 
	TRISG=0b0000000000000000;			// SPI2 (rg6..8)

  ANSELB=0;
  ANSELE=0;
  ANSELG=0;

  CNPUDbits.CNPUD2=1;   // switch/pulsanti
  CNPUDbits.CNPUD3=1;
  CNPUGbits.CNPUG6=1;   // I2C tanto per
  CNPUGbits.CNPUG8=1;  
#endif

#ifdef ILI9341
	TRISB=0b0000000000000001;			// pulsante; [ AN ?? ]
	TRISC=0b0000000000000000;
	TRISD=0b0000000000000000;			// 2led
	TRISE=0b0000000000000000;			// led
	TRISF=0b0000000000000001;			// pulsante
	TRISG=0b0000000000000000;			// SPI2 (rg6..8)

  ANSELB=0;
  ANSELE=0;
  ANSELG=0;

  CNPUFbits.CNPUF0=1;   // switch/pulsanti
  CNPUBbits.CNPUB0=1;
  CNPUDbits.CNPUD9=1;   // I2C tanto per
  CNPUDbits.CNPUD10=1;  
#endif
      
  
  Timer_Init();
  PWM_Init();
  ADC_Init();
#ifdef USING_SIMULATOR
  UART_Init(1000000);
#else
  UART_Init(/*230400L*/ 115200L);
#endif

  myINTEnableSystemMultiVectoredInt();
#ifndef __DEBUG  // cazzo di merda cagata male di simulatore
  __delay_ms(50); 
#endif

  
  CPUPins |= DoReset;
#ifdef USING_SIMULATOR
  puts("boot emulator");
   	ColdReset=1;    Emulate(0);
#endif

//#ifndef __DEBUG
#ifdef ST7735
  Adafruit_ST7735_1(0,0,0,0,-1);
  Adafruit_ST7735_initR(INITR_BLACKTAB);
#endif
#ifdef ILI9341
  Adafruit_ILI9341_8(8, 9, 10, 11, 12, 13, 14);
	begin(0);
  __delay_ms(200);
#endif
  
//  displayInit(NULL);
  
#ifdef m_LCDBLBit
  m_LCDBLBit=1;
#endif
  
//	begin();
	clearScreen();

// init done
	setTextWrap(1);
//	setTextColor2(WHITE, BLACK);

	drawBG();
  
  __delay_ms(200);

//#endif

//  if(!SW2)      //
//    PLAReg[0]=0;    // cartridge a 8000-bfff, kernel al suo posto
//    memcpy(&ram_seg[0x8000],&C64cartridge,0x4000);
  

	initHW();

	ColdReset=1;

  Emulate(0);

  }



enum CACHE_MODE {
  UNCACHED=0x02,
  WB_WA=0x03,
  WT_WA=0x01,
  WT_NWA=0x00,
/* Cache Coherency Attributes */
//#define _CACHE_WRITEBACK_WRITEALLOCATE      3
//#define _CACHE_WRITETHROUGH_WRITEALLOCATE   1
//#define _CACHE_WRITETHROUGH_NOWRITEALLOCATE 0
//#define _CACHE_DISABLE                      2
  };
void mySYSTEMConfigPerformance(void) {
  unsigned int PLLIDIV;
  unsigned int PLLMUL;
  unsigned int PLLODIV;
  float CLK2USEC;
  unsigned int SYSCLK;
  char PLLODIVVAL[]={
    2,2,4,8,16,32,32,32
    };
  unsigned int cp0;

  PLLIDIV=SPLLCONbits.PLLIDIV+1;
  PLLMUL=SPLLCONbits.PLLMULT+1;
  PLLODIV=PLLODIVVAL[SPLLCONbits.PLLODIV];

  SYSCLK=(FOSC*PLLMUL)/(PLLIDIV*PLLODIV);
  CLK2USEC=SYSCLK/1000000.0f;

  SYSKEY = 0x0;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;

  if(SYSCLK<=60000000)
    PRECONbits.PFMWS=0;
  else if(SYSCLK<=120000000)
    PRECONbits.PFMWS=1;
  else if(SYSCLK<=210000000)		// per overclock :)
    PRECONbits.PFMWS=2;
  else if(SYSCLK<=252000000)
    PRECONbits.PFMWS=4;
  else
    PRECONbits.PFMWS=7;

  PRECONbits.PFMSECEN=0;    // non c'è nella versione "2019" ...
  PRECONbits.PREFEN=0b10;

  SYSKEY = 0x0;

	  // Set up caching
  cp0 = _mfc0(16, 0);
  cp0 &= ~0x07;
  cp0 |= WB_WA /*0b011*/; // K0 = Cacheable, non-coherent, write-back, write allocate
  _mtc0(16, 0, cp0);  

  }

void myINTEnableSystemMultiVectoredInt(void) {

  PRISS = 0x76543210;
  INTCONSET = _INTCON_MVEC_MASK /*0x1000*/;    //MVEC
  asm volatile ("ei");
  //__builtin_enable_interrupts();
  }

/* CP0.Count counts at half the CPU rate */
#define TICK_HZ (CPU_HZ / 2)

/* wait at least usec microseconds */
#if 0
void delay_usec(unsigned long usec) {
unsigned long start, stop;

  /* get start ticks */
  start = readCP0Count();

  /* calculate number of ticks for the given number of microseconds */
  stop = (usec * 1000000) / TICK_HZ;

  /* add start value */
  stop += start;

  /* wait till Count reaches the stop value */
  while (readCP0Count() < stop)
    ;
  }
#endif


void __delay_us(unsigned int usec) {
  unsigned int tWait, tStart;

  tWait=(GetSystemClock()/2000000)*usec;
  tStart=_mfc0(9,0);
  while((_mfc0(9,0)-tStart)<tWait)
    ClrWdt();        // wait for the time to pass
  }

void __delay_ms(unsigned int ms) {
  
  for(;ms;ms--)
    __delay_us(1000);
  }


void Timer_Init(void) {

  // TIMER 2 INITIALIZATION (TIMER IS USED for buzzer, v. 8253 reg.2
  T2CON=0;
  T2CONbits.TCS = 0;                  // clock from peripheral clock
  T2CONbits.TCKPS = 0b110;            // 1:64 prescaler (pwm clock=1KHz circa se buzzer viene impostato a 0x528=~1300)
  T2CONbits.T32 = 0;                  // 16bit
//  PR2 = 2000;                         // rollover every n clocks; 2000 = 50KHz
  PR2 = 65535;                         // 
  T2CONbits.TON = 1;                  // (start timer per PWM) 
  
  // TIMER 3 INITIALIZATION (TIMER IS USED for RTC, 1Hz).
  T3CON=0;
  T3CONbits.TCS = 0;                  // clock from peripheral clock
  T3CONbits.TCKPS = 0b110;            // 1:64 prescaler => ~1MHz
  PR3 = 15625;                        // rollover every n clocks; v. sw
  T3CONbits.TON = 1;                  // (start timer ) 

  IPC3bits.T3IP=4;            // set IPL 4, sub-priority 2??
  IPC3bits.T3IS=0;
  IEC0bits.T3IE=0;             // enable Timer 3 interrupt (se si vuole) in initHW, meglio

  // TIMER 4 INITIALIZATION (TIMER IS USED for 55mS timer/counter, and CGA redraw emulation
  T4CON=0;
  T4CONbits.TCS = 0;                  // clock from peripheral clock
  T4CONbits.TCKPS = 0b110;             // 1:64 prescaler => 100/64 = ~1MHz
  PR4 = 65535;                         // rollover every n clocks; 
  T4CONbits.TON = 1;                  // (start timer ) 

  IPC4bits.T4IP=4;            // set IPL 4, sub-priority 2??
  IPC4bits.T4IS=0;
  IEC0bits.T4IE=0;             // enable Timer 4 interrupt (se si vuole) in initHW, meglio
  
  // TIMER 5 INITIALIZATION (TIMER IS USED for DMA controller (fake :)
  T5CON=0;
  T5CONbits.TCS = 0;                  // clock from peripheral clock
  T5CONbits.TCKPS = 0b110;             // 1:64 prescaler => 100/64 = ~1MHz
  PR5 = 65535;                         // rollover every n clocks; 
  T5CONbits.TON = 1;                  // (start timer ) 

  IPC6bits.T5IP=4;            // set IPL 4, sub-priority 2??
  IPC6bits.T5IS=0;
  IEC0bits.T5IE=0;             // enable Timer 5 interrupt (se si vuole) in initHW, meglio
	}

void PWM_Init(void) {

  SYSKEY = 0x00000000;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;
  CFGCONbits.OCACLK=0;      // sceglie timer per PWM
  SYSKEY = 0x00000000;
  
#ifdef ST7735
  OC1CON = 0x0006;      // TimerX ossia Timer2; PWM mode no fault; Timer 16bit, TimerX
//  OC1R    = 500;		 // su PIC32 è read-only!
//  OC1RS   = 1000;   // 50%, relativo a PR2 del Timer2
  OC1R    = 32768;		 // su PIC32 è read-only!
  OC1RS   = 0;        // solo onda quadra
  OC1CONbits.ON = 1;   // on
#endif
#ifdef ILI9341
  OC7CON = 0x0006;      // TimerX ossia Timer2; PWM mode no fault; Timer 16bit, TimerX
//  OC7R    = 500;		 // su PIC32 è read-only!
//  OC7RS   = 1000;   // 50%, relativo a PR2 del Timer2
  OC7R    = 32768;		 // su PIC32 è read-only!
  OC7RS   = 0;        // solo onda quadra, 
  OC7CONbits.ON = 1;   // on
#endif
  }

void ADC_Init(void) {   // v. LCDcontroller e PC_PIC_audio

  ADCCON1=0;    // AICPMPEN=0, siamo sopra 2.5V
  CFGCONbits.IOANCPEN=0;    // idem; questo credo voglia SYSLOCK
  ADCCON2=0;
  ADCCON3=0;
  
  //Configure Analog Ports
  ADCCON3bits.VREFSEL = 0; //Set Vref to VREF+/-

  ADCCMPEN1=0x00000000;
  ADCCMPEN2=0x00000000;
  ADCCMPEN3=0x00000000;
  ADCCMPEN4=0x00000000;
  ADCCMPEN5=0x00000000;
  ADCCMPEN6=0x00000000;
  ADCFLTR1=0x00000000;
  ADCFLTR2=0x00000000;
  ADCFLTR3=0x00000000;
  ADCFLTR4=0x00000000;
  ADCFLTR5=0x00000000;
  ADCFLTR6=0x00000000;
  
  ADCFSTAT=0;
  
  ADCTRGMODE=0;
  // no SH ALT qua
  ADCTRGSNS=0;

  ADCTRG1=0;
  ADCTRG2=0;
  ADCTRG3=0;
  ADCTRG1bits.TRGSRC3 = 0b00011; // Set AN3 to trigger from scan trigger
  
  // I PRIMI 12 POSSONO OVVERO DEVONO USARE gli ADC dedicati! e anche se si usano
  // poi gli SCAN, per quelli >12, bisogna usarli entrambi (e quindi TRGSRC passa a STRIG ossia "common")

  ADCIMCON1bits.DIFF3 = 0; // single ended, unsigned
  ADCIMCON1bits.SIGN3 = 0; // 
  ADCIMCON2bits.DIFF16 = 0; // 
  ADCIMCON2bits.SIGN16 = 0; // 
  ADCCON1bits.SELRES = 0b10; // ADC7 resolution is 10 bits
  ADCCON1bits.STRGSRC = 1; // Select scan trigger.

  // Initialize warm up time register
  ADCANCON = 0;
  ADCANCONbits.WKUPCLKCNT = 5; // Wakeup exponent = 32 * TADx

  ADCEIEN1 = 0;
    
  ADCCON2bits.ADCDIV = 64; // per SHARED: 2 TQ * (ADCDIV<6:0>) = 64 * TQ = TAD
  ADCCON2bits.SAMC = 10;
    
  ADCCON3bits.ADCSEL = 0;   //0=periph clock 3; 1=SYSCLK
  ADCCON3bits.CONCLKDIV = 4; // 25MHz, sotto è poi diviso 2 per il canale, = max 50MHz come da doc

  ADC3TIMEbits.SELRES=0b10;        // 10 bits
  ADC3TIMEbits.ADCDIV=4;       // 
  ADC3TIMEbits.SAMC=10;        //   
  
  ADCCSS1 = 0; // Clear all bits
  ADCCSS2 = 0;
  ADCCSS1bits.CSS3 = 1; // AN3 (Class 1) set for scan
  ADCCSS1bits.CSS16 = 1; // AN16 (Class 2) set for scan

  ADC0CFG=DEVADC0;
  ADC1CFG=DEVADC1;
  ADC2CFG=DEVADC2;
  ADC3CFG=DEVADC3;
  ADC4CFG=DEVADC4;
  ADC7CFG=DEVADC7;

  ADCCON1bits.ON = 1;   //Enable AD
  ClrWdt();
  
  // Wait for voltage reference to be stable 
#ifndef USING_SIMULATOR
  while(!ADCCON2bits.BGVRRDY); // Wait until the reference voltage is ready
  //while(ADCCON2bits.REFFLT); // Wait if there is a fault with the reference voltage
#endif

  // Enable clock to the module.
  ADCANCONbits.ANEN7 = 1;
  ADCCON3bits.DIGEN7 = 1;
  ADCANCONbits.ANEN3 = 1;
  ADCCON3bits.DIGEN3 = 1;
  
#ifndef USING_SIMULATOR
  while(!ADCANCONbits.WKRDY3); // Wait until ADC is ready
  while(!ADCANCONbits.WKRDY7); // 
#endif
  
  // ADCGIRQEN1bits.AGIEN7=1;     // IRQ (anche ev. per DMA))

	}

BYTE readADC(BYTE n) { // http://ww1.microchip.com/downloads/en/DeviceDoc/70005213f.pdf
  WORD retval;
  

	__delay_us(10);
  
  ADCCON3bits.GSWTRG = 1; // Start software trigger

  switch(n) {
    case 1:
      while(!ADCDSTAT1bits.ARDY3)    // Wait for the conversion to complete
        ClrWdt();
      ADCDATA16;
      retval=ADCDATA3;
      break;
    case 0:
      while(!ADCDSTAT1bits.ARDY16)    // Wait for the conversion to complete
        ClrWdt();
      ADCDATA3;
      retval=ADCDATA16;
    // PARE che quando mandi il trigger, lui converte TUTTI i canali abilitati,
    // per cui se non pulisco "l'altro" mi becco un RDY e una lettura precedente...
    // forse COSI' funzionerebbe, PROVARE      while(ADCCON2bits.EOSRDY == 0) // Wait until the measurement run
      break;
    default:
      break;
    }
  
 
//    IFS0bits.AD1IF=0;
    
  return retval >> 2;   // 12->10 bit
  }

void UART_Init(uint32_t baudRate) {
  
  U3MODE=0b0000000000001000;    // BRGH=1
  U3STA= 0b0000010000000000;    // TXEN
  uint32_t baudRateDivider = ((GetPeripheralClock()/(4*baudRate))-1);
  U3BRG=baudRateDivider;
  U3MODEbits.ON=1;
  
#if 0
  ANSELDCLR = 0xFFFF;
  CFGCONbits.IOLOCK = 0;      // PPS Unlock
  RPD11Rbits.RPD11R = 3;        // Assign RPD11 as U1TX
  U1RXRbits.U1RXR = 3;      // Assign RPD10 as U1RX
  CFGCONbits.IOLOCK = 1;      // PPS Lock

  // Baud related stuffs.
  U1MODEbits.BRGH = 1;      // Setup High baud rates.
  unsigned long int baudRateDivider = ((GetSystemClock()/(4*baudRate))-1);
  U1BRG = baudRateDivider;  // set BRG

  // UART Configuration
  U1MODEbits.ON = 1;    // UART1 module is Enabled
  U1STAbits.UTXEN = 1;  // TX is enabled
  U1STAbits.URXEN = 1;  // RX is enabled

  // UART Rx interrupt configuration.
  IFS1bits.U1RXIF = 0;  // Clear the interrupt flag
  IFS1bits.U1TXIF = 0;  // Clear the interrupt flag

  INTCONbits.MVEC = 1;  // Multi vector interrupts.

  IEC1bits.U1RXIE = 1;  // Rx interrupt enable
  IEC1bits.U1EIE = 1;
  IPC7bits.U1IP = 7;    // Rx Interrupt priority level
  IPC7bits.U1IS = 3;    // Rx Interrupt sub priority level
#endif
  }

char BusyUART1(void) {
  
  return(!U3STAbits.TRMT);
  }

char DataRdyUART1(void) {
  
  return(!U3STAbits.URXDA);
  }

void putsUART1(unsigned int *buffer) {
  char *temp_ptr = (char *)buffer;

    // transmit till NULL character is encountered 

  if(U3MODEbits.PDSEL == 3)        /* check if TX is 8bits or 9bits */
    {
        while(*buffer) {
            while(U3STAbits.UTXBF); /* wait if the buffer is full */
            U3TXREG = *buffer++;    /* transfer data word to TX reg */
        }
    }
  else {
        while(*temp_ptr) {
            while(U3STAbits.UTXBF);  /* wait if the buffer is full */
            U3TXREG = *temp_ptr++;   /* transfer data byte to TX reg */
        }
    }
  }

unsigned int ReadUART1(void) {
  
  if(U3MODEbits.PDSEL == 3)
    return (U3RXREG);
  else
    return (U3RXREG & 0xFF);
  }

void WriteUART1(unsigned int data) {
  
  if(U3MODEbits.PDSEL == 3)
    U3TXREG = data;
  else
    U3TXREG = data & 0xFF;
  }

void __attribute__((no_fpu)) __ISR(_UART3_RX_VECTOR) UART3_ISR(void) {
  
#ifndef ILI9341     // piazzare...
  LATDbits.LATD4 ^= 1;    // LED to indicate the ISR.
#endif
  char curChar = U3RXREG;
  if(i8250Reg[1] & 1)
    if(!(i8259RegW[1] & 0x10))
      i8259RegR[0] |= 0x10;
    //    SERIRQ=1;
  IFS4bits.U3RXIF = 0;  // Clear the interrupt flag!
  }


int xlat_key(BYTE ch) {
	int i;

	if(KBCommand & 0b01000000) {//  XT opp AT translation, 1=XT
    switch(ch) {
      case 0:
        break;
      case ' ':
        i=0x39;
        break;
      case 'A':
        i=0x1e;
        break;
      case 'B':
        i=0x30;
        break;
      case 'C':
        i=0x2e;
        break;
      case 'D':
        i=0x20;
        break;
      case 'E':
        i=0x12;
        break;
      case 'F':
        i=0x21;
        break;
      case 'G':
        i=0x22;
        break;
      case 'H':
        i=0x23;
        break;
      case 'I':
        i=0x17;
        break;
      case 'J':
        i=0x24;
        break;
      case 'K':
        i=0x25;
        break;
      case 'L':
        i=0x26;
        break;
      case 'M':
        i=0x32;
        break;
      case 'N':
        i=0x31;
        break;
      case 'O':
        i=0x17;
        break;
      case 'P':
        i=0x19;
        break;
      case 'Q':
        i=0x10;
        break;
      case 'R':
        i=0x13;
        break;
      case 'S':
        i=0x1f;
        break;
      case 'T':
        i=0x14;
        break;
      case 'U':
        i=0x16;
        break;
      case 'V':
        i=0x2f;
        break;
      case 'W':
        i=0x11;
        break;
      case 'X':
        i=0x2d;
        break;
      case 'Y':
        i=0x15;
        break;
      case 'Z':
        i=0x2c;
        break;
      case '0':
        i=0xb;
        break;
      case '1':
        i=0x2;
        break;
      case '2':
        i=0x3;
        break;
      case '3':
        i=0x4;
        break;
      case '4':
        i=0x5;
        break;
      case '5':
        i=0x6;
        break;
      case '6':
        i=0x7;
        break;
      case '7':
        i=0x8;
        break;
      case '8':
        i=0x9;
        break;
      case '9':
        i=0xa;
        break;
      case '.':
        i=0x34;
        break;
      case ':':     // + shift
        i=0x34;
        break;
      case ',':
        i=0x33;
        break;
      case '£':
        break;
      case '/':
        i=0x35;
        break;
      case '?':
        i=0x35;     // + shift
        break;
			case '+':
				i = 0xd;
				break;
			case '-':
				i = 0xc;
				break;
			case '*':
				i = 0x1a;
				break;
			case '\x8':
				i = 0xe;
				break;
      case '\r':
        i=0x1c;
        break;
      case '\n':
        break;
      case '\x1b':
        i=0x1;
        break;

      case 0xa1:    //F1
        i=0x3b;
        break;
      case 0xa2:
        i=0x3c;
        break;
      case 0xa3:
        i=0x3d;
        break;
      case 0xa4:
        i=0x3e;
        break;
      case 0xa5:
        i=0x3f;
        break;

      case 129:     // shift...
        i=0x36;
        break;
      case 130:     // alt
        i=0x38;
        break;
      case 131:     // ctrl
        i=0x1d;
        break;

      default:
        break;

  // 	cmp	al,0x36 ; Shift?
  //	cmp	al,0x38 ; Alt?
  //	cmp	al,0x1d ; Ctrl?

      }
		}
	else {
		switch(ch) {
			case 0:
				break;
			case ' ':
				i=0x39;
				break;
			case 'A':
				i=0x1e;
				break;
			}
		}
  
	return i;
  }

int emulateKBD(BYTE ch) {
  int i;

  
	KBCommand |= 0b01000001;		// per glabios
//per glabios
//	if(!(i8255RegW[1] & 0b01000000))			// CLOCK low per disabilitare...
//fare			ANCHE b7!
//		goto skipKB;
  
  if(!ch) {
		i=xlat_key(ch);
		if(i) {
#ifndef PCAT
			Keyboard[0]=i | 0x80;
  
			if(!(KBCommand & 0b10000000) && (KBCommand & 0b01000000)) {     //..e se enabled e non reset...
				KBStatus |= 0b00000001;			// output available
			//    KBDIRQ=1;
				i8259IRR |= 0b00000010;
				}
			}
#else
			if(KBCommand & 0b01000000) {//  XT opp AT translation, 1=XT  MA NON è CHIARO, forse non su XT ma solo su AT con 8042!
				Keyboard[0]=i | 0x80;
				}
			else {
				Keyboard[0] = 0xe0;
				Keyboard[1] = i;
				}
			KBStatus |= 0b00000001;			// output available
			KBStatus &= ~0b00001000;		// data
  
	//#ifndef _DEBUG
			if(!(KBStatus & 0b00010000)) {   // se attiva...
				if((KBCommand & 0b00000001)) {     //..e se interrupt attivi...
			//    KBDIRQ=1;
					i8259IRR |= 0b00000010;
					}
				}
			}
//#endif
#endif
    }
  else {
		i=xlat_key(ch);
		if(i) {
#ifndef PCAT
			Keyboard[0]=i;
  
			if(!(KBCommand & 0b10000000) && (KBCommand & 0b01000000)) {     //..e se enabled e non reset...
				KBStatus |= 0b00000001;			// output available
			//    KBDIRQ=1;
				i8259IRR |= 0b00000010;
				}
#else
			if(KBCommand & 0b01000000) {//  XT opp AT translation, 1=XT  MA NON è CHIARO, forse non su XT ma solo su AT con 8042!
				Keyboard[0]=i;
				}
			else {
				Keyboard[0]=i;
				}
			KBStatus |= 0b00000001;			// output available
			KBStatus &= ~0b00001000;		// data
  
	//#ifndef _DEBUG
			if(!(KBStatus & 0b00010000)) {   // se attiva...
				if((KBCommand & 0b00000001)) {     //..e se interrupt attivi...
			//    KBDIRQ=1;
					i8259IRR |= 0b00000010;
					}
				}
#endif

			}
    }

  KBStatus |= 0b00000001;
  
  if((KBStatus & 0b00010000)) {   // se attiva...
		if(KBCommand & 0b00000001) {     //..e se interrupt attivi...
			//    KBDIRQ=1;
			i8259IRR |= 2;
			}
    }
  
no_irq:
    ;
  }

BYTE whichKeysFeed=0;
char keysFeed[32]={0};
volatile BYTE keysFeedPtr=255;
const char *keysFeed1=" \r";     // per BASIC!
const char *keysFeed2="8/14/2025\r\r";     // 
const char *keysFeed3="DIR\r";     // 
//const char *keysFeed1="  A\r";     // 
//const char *keysFeed1="  A\x81\xa1\xa2:\r";     // space per BASIC su Bios "nuovo"
const char *keysFeed4="B:\r";
const char *keysFeed5="A \x1b";    //RIMETTERE shift \x81
const char *keysFeed6="A:\r";
const char *keysFeed7="CHKDSK \r";
const char *keysFeed8="SCREEN 1\r";
const char *keysFeed9="LIST\r";
const char *keysFeed10="SPEED\r";

void __attribute__((no_fpu)) __ISR(_TIMER_3_VECTOR,ipl4SRS) TMR3_ISR(void) {   //100Hz 2024
// https://www.microchip.com/forums/m842396.aspx per IRQ priority ecc
  static BYTE dividerTim,dividerVICpatch;
  static WORD dividerEmulKbd;
  static BYTE keysFeedPhase=0;
  int i;
  BYTE n;

//  LED2 ^= 1;      // check timing: 100Hz, 18/2/25 (die humans 4ever)
  
  dividerTim++;
  if(dividerTim>=100) {   // 1Hz RTC

#warning usare anche se GLATick!!
#ifdef PCAT
    // vedere registro 0A, che ha i divisori...
    // i146818RAM[10] & 15
    dividerTim=0;
    if(!(i146818RAM[11] & 0b10000000)) {    // SET
#warning VERIFICARE se il bios ABILITA RTC!! e occhio PCXTbios che usa un diverso RTC...
      i146818RAM[10] |= 0b10000000;
			if(!(i146818RAM[11] & 0b00000100)) 			// BCD mode
				currentTime.sec=from_bcd(currentTime.sec);
      currentTime.sec++;
			n=currentTime.sec;
			if(!(i146818RAM[11] & 0b00000100)) 	
				currentTime.sec=to_bcd(currentTime.sec);
      if(n >= 60) {
        currentTime.sec=0;
				if(!(i146818RAM[11] & 0b00000100)) 			// BCD mode
					currentTime.min=from_bcd(currentTime.min);
        currentTime.min++;
				n=currentTime.min;
				if(!(i146818RAM[11] & 0b00000100)) 	
					currentTime.min=to_bcd(currentTime.min);
        if(n >= 60) {
          currentTime.min=0;
					if(!(i146818RAM[11] & 0b00000100)) 			// BCD mode
						currentTime.hour=from_bcd(currentTime.hour);
          currentTime.hour++;
					n=currentTime.hour;
					if(!(i146818RAM[11] & 0b00000100))
						currentTime.hour=to_bcd(currentTime.hour);
          if( ((i146818RAM[11] & 0b00000010) && n >= 24) || 
            (!(i146818RAM[11] & 0b00000010) && n >= 12) ) {
            currentTime.hour=0;
						if(!(i146818RAM[11] & 0b00000100)) 			// BCD mode
							currentDate.mday=from_bcd(currentDate.mday);
            currentDate.mday++;
						n=currentDate.mday;
						if(!(i146818RAM[11] & 0b00000100))
							currentDate.mday=to_bcd(currentDate.mday);
            i=dayOfMonth[currentDate.mon-1];
            if((i==28) && !(currentDate.year % 4))
              i++;
            if(n > i) {		// (rimangono i secoli... GLATick li mette nella RAM[0x32] del RTC)
              currentDate.mday=0;
							if(!(i146818RAM[11] & 0b00000100)) 			// BCD mode
								currentDate.mon=from_bcd(currentDate.mon);
              currentDate.mon++;
							if(!(i146818RAM[11] & 0b00000100))
								currentDate.mon=to_bcd(currentDate.mon);
              if(n > 12) {		// 
                currentDate.mon=1;
								if(!(i146818RAM[11] & 0b00000100)) 			// BCD mode
									currentDate.year=from_bcd(currentDate.year);
                currentDate.year++;
								n=currentDate.year;
								if(!(i146818RAM[11] & 0b00000100))
									currentDate.year=to_bcd(currentDate.year);
								if(n>=100) {
	                currentDate.year=0;
									i146818RAM[0x32]++; // vabbe' :)
									}
                }
              }
            }
          }
        } 
      i146818RAM[12] |= 0b10010000;
      i146818RAM[10] &= ~0b10000000;
      } 
    else
      i146818RAM[10] &= ~0b10000000;
    // inserire Alarm... :)
    i146818RAM[12] |= 0b01000000;     // in effetti dice che deve fare a 1024Hz! o forse è l'altro flag, bit3 ecc
    if(i146818RAM[12] & 0b01000000 && i146818RAM[11] & 0b01000000 ||
       i146818RAM[12] & 0b00100000 && i146818RAM[11] & 0b00100000 ||
       i146818RAM[12] & 0b00010000 && i146818RAM[11] & 0b00010000)
      i146818RAM[12] |= 0b10000000;
    if(i146818RAM[12] & 0b10000000) {
#ifdef EXT_80286			// solo PC/AT!
//        RTCIRQ=1;
        i8259IRR2 |= 1;
	      if(!(i8259IMR & 2)) {		// poi forse andrebbe gestito il Cascaded...  http://www.osdever.net/tutorials/view/irqs
        }
#endif
      }
#endif
		} 

//  dividerVICpatch++;
//  if(dividerVICpatch>=3) {    // 80mS per fascia (sono 25 in CGA ossia ~750 tutto compreso, 18/2/25
//    dividerVICpatch=0;
    VIDIRQ=1;       // refresh screen in 200/8=25 passate, 50 volte al secondo FINIRE QUA!
//    }

  if(keysFeedPtr==255)      // EOL
    goto fine;
  if(keysFeedPtr==254) {    // NEW string
    keysFeedPtr=0;
    keysFeedPhase=0;
		switch(whichKeysFeed) {
			case 0:
				strcpy(keysFeed,keysFeed1);
				break;
			case 1:
				strcpy(keysFeed,keysFeed2);
				break;
			case 2:
				strcpy(keysFeed,keysFeed3);
				break;
			case 3:
				strcpy(keysFeed,keysFeed4);
				break;
			case 4:
				strcpy(keysFeed,keysFeed5);
				break;
			case 5:
				strcpy(keysFeed,keysFeed6);
				break;
			case 6:
				strcpy(keysFeed,keysFeed7);
				break;
			case 7:
				strcpy(keysFeed,keysFeed8);
				break;
			case 8:
				strcpy(keysFeed,keysFeed9);
				break;
			case 9:
				strcpy(keysFeed,keysFeed10);
				break;
      }
		whichKeysFeed++;
		if(whichKeysFeed>=10)
			whichKeysFeed=0;
//    goto fine;
		}
    
  if(keysFeed[keysFeedPtr]) {
    dividerEmulKbd++;
    if(dividerEmulKbd>=30) {   // ~.3Hz per emulazione tastiera! (più veloce di tot non va...)
      dividerEmulKbd=0;
      if(!keysFeedPhase) {
        keysFeedPhase=1;
        emulateKBD(keysFeed[keysFeedPtr]);
        }
      else {
        keysFeedPhase=0;
        emulateKBD(0);
        keysFeedPtr++;
        }
      }
    }
  else
    keysFeedPtr=255;
    
  
fine:
  IFS0CLR = _IFS0_T3IF_MASK;
  }

void __attribute__((no_fpu)) __ISR(_TIMER_4_VECTOR,ipl4SRS) TMR4_ISR(void) {
// https://www.microchip.com/forums/m842396.aspx per IRQ priority ecc

//  LED2 ^= 1;      // check timing: 1600Hz, 9/11/19 (fuck berlin day))
  
//  if((i8253Mode[0] & 0b00001110) == 0 MA LUI METTE 6 ossia mode 3??! )
    
  if(i8259RegW[1] == 0xbc &&      // PATCH finire gestione 8259, se no arrivano irq ad minchiam
      !(i8259RegW[1] & 1)) {
//      TIMIRQ=1;  //
    i8259RegR[0] |= 1;
    }
    
fine:
  IFS0CLR = _IFS0_T4IF_MASK;
  }

void __attribute__((no_fpu)) __ISR(_TIMER_5_VECTOR,ipl4SRS) TMR5_ISR(void) {
  
fine:
  IFS0CLR = _IFS0_T5IF_MASK;
  }


#ifdef ILI9341
#define RPLATE 300
#define NUMSAMPLES 2
#define SAMPLE_NOISE 6
BYTE manageTouchScreen(void /*UGRAPH_COORD_T *x,UGRAPH_COORD_T *y,uint16_t *z*/) { // v. breakthrough
  uint16_t x,y,z;
  static uint16_t oldx=HORIZ_SIZE/2,oldy=VERT_SIZE/2;
  int samples[NUMSAMPLES];
  uint8_t i, valid;

  valid = 1;

//  ANSELBbits.ANSB2 = 1;
  ANSELBbits.ANSB3 = 1;
  ANSELEbits.ANSE6 = 1;
//  ANSELEbits.ANSE7 = 1;
  
  TRISBbits.TRISB3=1;    //(_yp, INPUT);
  TRISEbits.TRISE7=1;    //(_ym, INPUT);
  TRISBbits.TRISB2=0;    //(_xp, OUTPUT);
  TRISEbits.TRISE6=0;    //(_xm, OUTPUT);

  m_TouchX1=1;      //(_xp, HIGH);
  m_TouchX2=0;      //(_xm, LOW);

  __delay_us(30); // Fast ARM chips need to allow voltages to settle

  for(i=0; i<NUMSAMPLES; i++)
    samples[i] = readADC(1 /*_yp*/);

  TRISBbits.TRISB3=0;    //(_yp, OUTPUT);
  TRISEbits.TRISE7=0;    //(_ym, OUTPUT);
  TRISBbits.TRISB2=1;    //(_xp, INPUT);
  TRISEbits.TRISE6=1;    //(_xm, INPUT);

  m_TouchY2=1;      //(_ym, LOW);
  m_TouchY1=0;      //(_yp, HIGH);
  
  
#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
   // Allow small amount of measurement noise, because capacitive
   // coupling to a TFT display's signals can induce some noise.
  if(((samples[0] - samples[1]) < -SAMPLE_NOISE) || ((samples[0] - samples[1]) > SAMPLE_NOISE) || 
          samples[0]==0 || samples[0]==1023) {
    valid = 0;
    } 
  else {
    samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples
    }
#endif

  y = 1023-(1023-samples[NUMSAMPLES/2]);
// qui li inverto... x con y e poi la direzione di y (credo dipenda dalla Rotazione)


  __delay_us(30); // Fast ARM chips need to allow voltages to settle

  for(i=0; i<NUMSAMPLES; i++)
    samples[i] = readADC(0 /*_xm*/);

   // Set X+ to ground
   // Set Y- to VCC
   // Hi-Z X- and Y+
  TRISBbits.TRISB3=1;    //(_yp, INPUT);
  TRISEbits.TRISE7=0;    //(_ym, OUTPUT);
  TRISBbits.TRISB2=0;    //(_xp, OUTPUT);
  TRISEbits.TRISE6=1;    //(_xm, INPUT);

  m_TouchY2=1;      //(_ym, HIGH); 
  m_TouchX1=0;      //(_xp, LOW);
  
  
#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
   // Allow small amount of measurement noise, because capacitive
   // coupling to a TFT display's signals can induce some noise.
  if(((samples[0] - samples[1]) < -SAMPLE_NOISE) || ((samples[0] - samples[1]) > SAMPLE_NOISE) || 
          samples[0]==0 || samples[0]==1023) {
    valid = 0;
    } 
  else {
    samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples
    }
#endif

  x = (1023-samples[NUMSAMPLES/2]);

  __delay_us(30); // Fast ARM chips need to allow voltages to settle
  int z1 = readADC(0 /*_xm*/); 
  int z2 = readADC(1 /*_yp*/);

  if(RPLATE != 0) {
    // now read the x 
    float rtouch;
    rtouch = z2;
    rtouch /= z1;
    rtouch -= 1;
    rtouch *= x;
    rtouch *= RPLATE;
    rtouch /= 1024;
     
    z = rtouch;
    // abbiamo da 200 a 150 (< se + pressione) con stilo, 300..200 con cotton fioc :) 250..50 con dito
    } 
  else {
    z = (1023-(z2-z1));
    }

  if(!valid) {
    z = 0;
    }

  TRISBbits.TRISB2=0;
  TRISBbits.TRISB3=0;
  TRISEbits.TRISE6=0;
  TRISEbits.TRISE7=0;
//  ANSELBbits.ANSB2 = 0;
  ANSELBbits.ANSB3 = 0;
  ANSELEbits.ANSE6 = 0;
//  ANSELEbits.ANSE7 = 0;


  if(valid) {
    mouseX=x-oldx;
    mouseY=y-oldy;
    
    if(z > 200) {
      }
    
    if(mouseState & 0b10000000) {		// semaforo
      mouseState |= 0b00100000;     // left click
#if MOUSE_TYPE==1
      COMDataEmuFifo[0]=0b01000000 | (mouseState & 0b00110000) | (((int8_t)mouseX >> 6) & 0x03) | (((int8_t)mouseY >> 4) & 0x0c);
      COMDataEmuFifo[1]=(int8_t)mouseX & 0x3f;
      COMDataEmuFifo[2]=(int8_t)mouseY & 0x3f;
      COMDataEmuFifoCnt=0;
#elif MOUSE_TYPE==2
      COMDataEmuFifo[0]=0b10000000) | (mouseState & 0b00100000) ? 0 : 0b00000100))  | 
        (mouseState & 0b00010000 ? 0b00000001 : 0);
      COMDataEmuFifo[1]=(int8_t)x;
      COMDataEmuFifo[2]=(int8_t)-y;		// dio che culattoni :D
      COMDataEmuFifo[3]=0;
      COMDataEmuFifo[4]=0;
      COMDataEmuFifoCnt=0;
#endif
			i8250Reg[2] &= ~0b00000001;
			if(i8250Reg[1] & 0b00000001)			// se IRQ attivo
        i8259IRR |= 0x10;

      i8250Reg[5] |= 1;			// byte rx in COM cmq

      }
		mouseState &= ~0b10000000;  // marker per COM

    oldx=mouseX; oldy=mouseY; 
    }

  return valid;
  }
#endif


// ---------------------------------------------------------------------------------------
// declared static in case exception condition would prevent
// auto variable being created
static enum {
	EXCEP_IRQ = 0,			// interrupt
	EXCEP_AdEL = 4,			// address error exception (load or ifetch)
	EXCEP_AdES,				// address error exception (store)
	EXCEP_IBE,				// bus error (ifetch)
	EXCEP_DBE,				// bus error (load/store)
	EXCEP_Sys,				// syscall
	EXCEP_Bp,				// breakpoint
	EXCEP_RI,				// reserved instruction
	EXCEP_CpU,				// coprocessor unusable
	EXCEP_Overflow,			// arithmetic overflow
	EXCEP_Trap,				// trap (possible divide by zero)
	EXCEP_IS1 = 16,			// implementation specfic 1
	EXCEP_CEU,				// CorExtend Unuseable
	EXCEP_C2E				// coprocessor 2
  } _excep_code;

static unsigned int _epc_code;
static unsigned int _excep_addr;

void /*__attribute__((weak))*/ _general_exception_handler(uint32_t __attribute__((unused)) code, uint32_t __attribute__((unused)) address) {
//  _f.Trap=1;
  }

void __attribute__((nomips16,used)) _general_exception_handler_entry(void) {
  
	asm volatile("mfc0 %0,$13" : "=r" (_epc_code));
	asm volatile("mfc0 %0,$14" : "=r" (_excep_addr));

	_excep_code = (_epc_code & 0x0000007C) >> 2;

  _general_exception_handler(_excep_code, _excep_addr);

  switch(_excep_code) {
    case EXCEP_Trap:
      // causare INT 0
      break;
    case EXCEP_RI:
      // causare INT ??
      break;
    case EXCEP_AdEL:
    case EXCEP_AdES:
      // causare INT ?? o _f.Trap=1 ??
      break;
    }
	while (1)	{
		// Examine _excep_code to identify the type of exception
		// Examine _excep_addr to find the address that caused the exception
    }
  }

#ifdef USING_SIMULATOR
void _mon_putc(unsigned char data) {
  
    while(!U3STAbits.TRMT)          // wait until the transmitter is ready
         ClrWdt();
    U3TXREG=data;                     // send one character
  }
#endif

