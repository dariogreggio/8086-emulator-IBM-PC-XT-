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
	VERNUMH+'0','.',VERNUML/10+'0',(VERNUML % 10)+'0',' ','-',' ', '2','1','/','0','7','/','2','4', 0 };

const char Copyr1[]="(C) Dario's Automation 2019-2024 - G.Dar\xd\xa\x0";


// Global Variables:
extern BOOL fExit,debug;
extern BYTE DoIRQ,DoNMI,DoHalt,DoReset,ColdReset;
extern BYTE ram_seg[];
extern BYTE CGAram[];
extern BYTE CGAreg[16];
extern BYTE i8237RegR[16],i8237RegW[16],i8237Reg2R[16],i8237Reg2W[16];
extern BYTE i8259RegR[2],i8259RegW[2],i8259Reg2R[2],i8259Reg2W[2];
extern BYTE i8253RegR[4],i8253RegW[4],i8253Mode[3],i8253ModeSel;
extern WORD i8253TimerR[3],i8253TimerW[3];
extern BYTE i8250Reg[8],i8250Regalt[3];
extern BYTE i6845RegR[18],i6845RegW[18];
extern BYTE i146818RegR[2],i146818RegW[2],i146818RAM[64];
extern BYTE i8042RegR[2],i8042RegW[2],KBRAM[32],KBControl,KBStatus;
extern uint8_t FloppyContrRegR[8],FloppyContrRegW[8],FloppyFIFO[16],FloppyFIFOPtr;
extern volatile BYTE Keyboard[];
extern volatile BYTE VIDIRQ /*TIMIRQ,KBDIRQ,SERIRQ,RTCIRQ,FDCIRQ*/;
uint32_t /*uint16_t*/ VICRaster;

volatile PIC32_RTCC_DATE currentDate={1,1,0};
volatile PIC32_RTCC_TIME currentTime={0,0,0};
const BYTE dayOfMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};


const WORD cgaColors[3][4]={ {BLACK,CYAN,MAGENTA,WHITE},
  {BLACK,RED,GREEN,YELLOW},
  {BLACK,RED,CYAN,WHITE} };
// cga_colors[4] = {0 /* Black */, 0x1F1F /* Cyan */, 0xE3E3 /* Magenta */, 0xFFFF /* White */
const WORD extCGAColors[16]={BLACK,GREEN,BLUE,CYAN,BURLYWOOD /*CRIMSON*/,DARKGRAY,MAGENTA,BRIGHTMAGENTA /*VIOLET*/,
	GRAY128,LIGHTGREEN,LIGHTGRAY,BRIGHTCYAN,LIGHTRED,YELLOW,PINK,WHITE};
const WORD textColors[16]={BLACK,BLUE,GREEN,CYAN, RED,MAGENTA,YELLOW,LIGHTGRAY,
	GRAY128,LIGHTBLUE,LIGHTGREEN,BRIGHTCYAN, LIGHTRED,BRIGHTMAGENTA,LIGHTYELLOW,WHITE};


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
      setAddrWindow(HORIZ_OFFSCREEN+0,rowIni/2 /*+VERT_OFFSCREEN*/,_width-0,(rowFin-rowIni)/2 +VERT_OFFSCREEN+VERT_OFFSCREEN);
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
      BYTE cy1,cy2;
      switch((i6845RegW[10] & 0x60)) {
        case 0:
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
  
  if(rowIni==0)
    CGAreg[0xa] &= ~0b1000; // CGA not in retrace V
  else if(rowIni==VERT_SIZE-8)
    CGAreg[0xa] |= 0b1000; // CGA in retrace V
  
  if(CGAreg[8] & 8) {     // enable video
    if(CGAreg[8] & 2) {     // graphic mode
      if(!(CGAreg[8] & 16)) {     // lores graph (320x200x4)
        if(CGAreg[9] & 16)      // bright foreground
          ;
        START_WRITE();
        if(rowIni==0) {
          setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<160; px++) {         // 320 pixel
              writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
              }
            }
          }
        setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
        p1=CGAram + (rowIni*80);
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
        if(CGAreg[9] & 16)      // disable color ovvero palette #3
          color=2;     // quale palette
        else
          color=CGAreg[9] & 32 ? 1 : 0;     // quale palette
        for(py=rowIni/8; py<rowFin/8; py++) {    // 
          for(px=0; px<160; px++) {         // 320 pixel (80byte) 
            ch=*p1++;
            writedata16(cgaColors[color][ch >> 6]);
            writedata16(cgaColors[color][(ch >> 2) & 3]);
            }
          ClrWdt();
          }
        if(rowIni==VERT_SIZE-8) {
          color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
          setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
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
        if(CGAreg[8] & 4) {     // modo 160x200 16 colori   SICURO??? 2024
          START_WRITE();
          if(rowIni==0) {
            setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
              for(px=0; px<160; px++) {         // 320 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
                }
              }
            }
          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
          p1=CGAram + (rowIni*80);
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
          for(py=rowIni/8; py<rowFin/8; py++) {    // 
            for(px=0; px<80; px++) {         // 160 pixel (80byte) 
              ch=*p1++;
              writedata16x2(textColors[ch >> 4],textColors[ch & 0xf]);
              writedata16x2(textColors[ch >> 4],textColors[ch & 0xf]);    // 160 -> 320
              }
            ClrWdt();
            }
          if(rowIni==VERT_SIZE-8) {
            color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
            setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
              for(px=0; px<160; px++) {         // 320 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
                }
              }
            }
          END_WRITE();
          }
        else {
          START_WRITE();
          p1=CGAram + (rowIni*80);
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
          if(CGAreg[9] & 4)      // disable color ovvero palette #3 QUA NON SI CAPISCE COSA DOVREBBE FARE!
            color=15;
          else
            color=CGAreg[9] & 15;
          if(rowIni==0) {
            setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
              for(px=0; px<160; px++) {         // 320 pixel
                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
                }
              }
            }
          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
          for(py=rowIni/8; py<rowFin/8; py++) {    // 
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
          if(rowIni==VERT_SIZE-8) {
            color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
            setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
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
    else {                // text mode
      START_WRITE();
      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
      if(rowIni==0) {
        setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
          for(px=0; px<160; px++) {         // 320 pixel
            writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
            }
          }
        }
      setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
      for(py=rowIni/8; py<rowFin/8; py++) {    // 
        if(CGAreg[8] & 1) {     // 80x25
          for(i=0; i<8; i++) {         // 8 righe 
            p1=CGAram + (py*80*2);    // char/colore
#warning            era 80?? 2024 ah ma tanto rowini è 0...
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
            p1=CGAram + (py*40*2);    // char/colore
            for(px=0; px<40; px++) {         // 40 char (40byte) diventano 320 pixel
              ch=*p1++;
              p=(BYTE *)&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x80)   // 8 pixel
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x40)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x20)
                writedata16(textColors[color & 0xf]);
              else
                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
              if(ch & 0x10)
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
        ClrWdt();
        }

      if(rowIni==VERT_SIZE-8) {
        color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
        setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
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
        switch((i6845RegW[10] & 0x60)) {
          case 0:
plot_cursor:
  // test          i6845RegW[10]=5;i6845RegW[11]=7;
            cy2= (i6845RegW[10] & /*0x1f*/ 7);    // 0..32 ma in effetti sono "reali" per cui di solito 6..7!
            cy1= (i6845RegW[11] & /*0x1f*/ 7);
            if(cy2==cy1) 
              ;
            else {
              if(CGAreg[8] & 1) {     // 80x25
                color=7;    // fisso :)
                START_WRITE();
                setAddrWindow((i % 80)*4,1+ /* ?? boh*/ cy1+ (i/80)*8 +VERT_OFFSCREEN,4,cy2-cy1);   // altezza e posizione fissa, gestire da i6845RegW[10,11]
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                END_WRITE();
                }
              else {
                color=7;    // fisso :)
                START_WRITE();
                setAddrWindow((i % 40)*8,1+ /* ?? boh*/ cy1+ (i/40)*8 +VERT_OFFSCREEN,8,cy2-cy1);
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                writedata16x2(textColors[color],textColors[color]);
                END_WRITE();
                }
              }
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
	TRISD=0b0000000000001100;			// 2 pulsanti
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
#ifdef USING_SIMULATOR
  UART_Init(1000000);
#else
  UART_Init(/*230400L*/ 115200L);
#endif

  myINTEnableSystemMultiVectoredInt();
#ifndef __DEBUG  // cazzo di merda cagata male di simulatore
  __delay_ms(50); 
#endif

  
  DoReset=1;
#ifdef USING_SIMULATOR
  puts("boot emulator");
   	ColdReset=0;    Emulate(0);
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

	ColdReset=0;

  Emulate(0);

  }


void initHW(void) {
  
  memset(CGAreg,0,sizeof(CGAreg));
  CGAreg[0]=0;              // disattivo video, default cmq
  i6845RegR[10]=i6845RegW[10]=0x20;     //disattivo cursore!
  
  memset(i8237RegR,0,sizeof(i8237RegR));
  memset(i8237RegW,0,sizeof(i8237RegW));
  memset(i8237Reg2R,0,sizeof(i8237Reg2R));
  memset(i8237Reg2W,0,sizeof(i8237Reg2W));
  i8259RegR[0]=i8259RegW[0]=0x00; i8259RegR[1]=i8259RegW[1]=0xff;
  i8259Reg2R[0]=i8259Reg2W[0]=0x00; i8259Reg2R[1]=i8259Reg2W[1]=0xff;
  memset(i8253RegR,0,sizeof(i8253RegR));
  memset(i8253RegW,0,sizeof(i8253RegW));
  memset(i8253Mode,0,sizeof(i8253Mode));
  i8253ModeSel=0;
  memset(i8253TimerR,0,sizeof(i8253TimerR));
  memset(i8253TimerW,0,sizeof(i8253TimerR));
  memset(i8250Reg,0,sizeof(i8250Reg));
  memset(i8250Regalt,0,sizeof(i8250Regalt));
  memset(i6845RegR,0,sizeof(i6845RegR));
  memset(i6845RegW,0,sizeof(i6845RegW));
  memset(i146818RegR,0,sizeof(i146818RegR));
  memset(i146818RegW,0,sizeof(i146818RegW));
  memset(i146818RAM,0,sizeof(i146818RAM));
  memset(i8042RegR,0,sizeof(i8042RegR));
  memset(i8042RegW,0,sizeof(i8042RegW));
  memset(FloppyContrRegR,0,sizeof(FloppyContrRegR));
  memset(FloppyContrRegW,0,sizeof(FloppyContrRegW));
	memset(FloppyFIFO,0,sizeof(FloppyFIFO));
	FloppyFIFOPtr=0;
	
  KBControl=0x00; KBStatus=0x00;
  
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

  CFGCONbits.OCACLK=0;      // sceglie timer per PWM
  
#ifdef ST7735
  OC1CON = 0x0006;      // TimerX ossia Timer2; PWM mode no fault; Timer 16bit, TimerX
//  OC1R    = 500;		 // su PIC32 è read-only!
//  OC1RS   = 1000;   // 50%, relativo a PR2 del Timer2
  OC1R    = 32768;		 // su PIC32 è read-only!
  OC1RS   = 0;        // per ora faccio solo onda quadra, v. SID reg. 0-1
  OC1CONbits.ON = 1;   // on
#endif
#ifdef ILI9341
  OC7CON = 0x0006;      // TimerX ossia Timer2; PWM mode no fault; Timer 16bit, TimerX
//  OC7R    = 500;		 // su PIC32 è read-only!
//  OC7RS   = 1000;   // 50%, relativo a PR2 del Timer2
  OC7R    = 32768;		 // su PIC32 è read-only!
  OC7RS   = 0;        // per ora faccio solo onda quadra, v. SID reg. 0-1
  OC7CONbits.ON = 1;   // on
#endif
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


int emulateKBD(BYTE ch) {
  int i;

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
      i=0x2;
			break;
		case '1':
      i=0x3;
			break;
		case '2':
      i=0x4;
			break;
		case '3':
      i=0x5;
			break;
		case '4':
      i=0x6;
			break;
		case '5':
      i=0x7;
			break;
		case '6':
      i=0x8;
			break;
		case '7':
      i=0x9;
			break;
		case '8':
      i=0xa;
			break;
		case '9':
      i=0xb;
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
		case '?':
      i=0x35;     // + shift
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
      goto no_irq;
      break;
      
// 	cmp	al,0x36 ; Shift?
//	cmp	al,0x38 ; Alt?
//	cmp	al,0x1d ; Ctrl?

		}
  
  if(!i)
    Keyboard[0] |= 0x80;
  else
    Keyboard[0]=i;
  KBStatus |= 0b00000001;
  
  if((KBStatus & 0b00010000)) {   // se attiva...
    if((KBControl & 0b00000001) && !(i8259RegW[1] & 2)) {     //..e se interrupt attivi...
  //    KBDIRQ=1;
      i8259RegR[0] |= 2;
      }
    }
  
no_irq:
    ;
  }

BYTE whichKeysFeed=0;
char keysFeed[32]={0};
volatile BYTE keysFeedPtr=255;
const char *keysFeed1="  A\r";     // 
//const char *keysFeed1="  A\x81\xa1\xa2:\r";     // space per BASIC su Bios "nuovo"
const char *keysFeed2="\x1b";    //RIMETTERE shift \x81

void __attribute__((no_fpu)) __ISR(_TIMER_3_VECTOR,ipl4SRS) TMR3_ISR(void) {   //100Hz 2024
// https://www.microchip.com/forums/m842396.aspx per IRQ priority ecc
  static BYTE dividerTim,dividerVICpatch;
  static WORD dividerEmulKbd;
  static BYTE keysFeedPhase=0;
  int i;

//  LED2 ^= 1;      // check timing: 1600Hz, 9/11/19 (fuck berlin day))
  
  dividerTim++;
  if(dividerTim>=100) {   // 1Hz RTC

    // vedere registro 0A, che ha i divisori...
    // i146818RAM[10] & 15
    dividerTim=0;
    if(!(i146818RAM[11] & 0x80)) {    // SET
#warning VERIFICARE se il bios ABILITA RTC!! e occhio PCXTbios che usa un diverso RTC...
      i146818RAM[10] |= 0x80;
      currentTime.sec++;
      if(currentTime.sec >= 60) {
        currentTime.sec=0;
        currentTime.min++;
        if(currentTime.min >= 60) {
          currentTime.min=0;
          currentTime.hour++;
          if( ((i146818RAM[11] & 2) && currentTime.hour >= 24) || 
            (!(i146818RAM[11] & 2) && currentTime.hour >= 12) ) {
            currentTime.hour=0;
            currentDate.mday++;
            i=dayOfMonth[currentDate.mon-1];
            if((i==28) && !(currentDate.year % 4))
              i++;
            if(currentDate.mday > i) {		// (rimangono i secoli...)
              currentDate.mday=0;
              currentDate.mon++;
              if(currentDate.mon > 12) {		// 
                currentDate.mon=1;
                currentDate.year++;
                }
              }
            }
          }
        } 
      i146818RAM[12] |= 0x90;
      i146818RAM[10] &= ~0x80;
      } 
    else
      i146818RAM[10] &= ~0x80;
    // inserire Alarm... :)
    i146818RAM[12] |= 0x40;     // in effetti dice che deve fare a 1024Hz! o forse è l'altro flag, bit3 ecc
    if(i146818RAM[12] & 0x40 && i146818RAM[11] & 0x40 ||
       i146818RAM[12] & 0x20 && i146818RAM[11] & 0x20 ||
       i146818RAM[12] & 0x10 && i146818RAM[11] & 0x10)     
      i146818RAM[12] |= 0x80;
    if(i146818RAM[12] & 0x80)     
      if(!(i8259Reg2W[1] & 1)) {
//        RTCIRQ=1;
        i8259Reg2R[0] |= 1;
        }

		} 

  dividerVICpatch++;
  if(dividerVICpatch>=3) {    // 25mS per fascia [per ora faccio una passata unica, quindi + lento] (sono ~30 tutto compreso, 21/7/24
    dividerVICpatch=0;
    VIDIRQ=1;       // refresh screen in 200/8=25 passate, 50 volte al secondo FINIRE QUA!
    }
// v.  CIA1RegR[0xe];

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
      }
		whichKeysFeed++;
		if(whichKeysFeed>=2)
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

