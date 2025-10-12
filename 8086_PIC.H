//---------------------------------------------------------------------------
//
#ifndef _8086_PIC_INCLUDED
#define _8086_PIC_INCLUDED

#include <stdint.h>

//---------------------------------------------------------------------------

/* check if build is for a real debug tool */
#if defined(__DEBUG) && !defined(__MPLAB_ICD2_) && !defined(__MPLAB_ICD3_) && \
   !defined(__MPLAB_PICKIT2__) && !defined(__MPLAB_PICKIT3__) && \
   !defined(__MPLAB_REALICE__) && \
   !defined(__MPLAB_DEBUGGER_REAL_ICE) && \
   !defined(__MPLAB_DEBUGGER_ICD3) && \
   !defined(__MPLAB_DEBUGGER_PK3) && \
   !defined(__MPLAB_DEBUGGER_PICKIT2) && \
   !defined(__MPLAB_DEBUGGER_PIC32MXSK)
    #warning Debug with broken MPLAB simulator
    #define USING_SIMULATOR
#endif

#ifdef ST7735
#define _TFTWIDTH  		160     //the REAL W resolution of the TFT
#define _TFTHEIGHT 		128     //the REAL H resolution of the TFT
//#define REAL_SIZE    
#define MIN_RASTER 0        // noi visualizziamo da 48 a 248
#define MAX_RASTER 199
#define HORIZ_SIZE 320      // 504 pixel compresi bordi, dice...
#define VERT_SIZE 200
#define HORIZ_OFFSCREEN 0
#ifdef REAL_SIZE    
#define VERT_OFFSCREEN 14      // 
#else
#define VERT_OFFSCREEN 14       // sarebbe ok 28 (128-100) ma preferisco leggere meglio su display piccolo! 25*5+3
#endif
#endif
#ifdef ILI9341
#define _TFTWIDTH  		320     //the REAL W resolution of the TFT
#define _TFTHEIGHT 		240     //the REAL H resolution of the TFT
//#define REAL_SIZE    
#define MIN_RASTER 0        // noi visualizziamo da 48 a 248
#define MAX_RASTER 199
#define HORIZ_SIZE 320      // 504 pixel compresi bordi, dice...
#define VERT_SIZE 200
#define HORIZ_OFFSCREEN 0
#ifdef REAL_SIZE    
#define VERT_OFFSCREEN 20      // 
#else
#define VERT_OFFSCREEN 20       // sarebbe ok 28 (128-100) ma preferisco leggere meglio su display piccolo! 25*5+3
#endif
#endif



//#define EXT_80186 1
#define EXT_80286 1
//#define EXT_80386 1
//#define EXT_80486 1
//#define EXT_80x87 1
#if defined(EXT_80486)
#define EXT_80386
#endif
#if defined(EXT_80386)
#define EXT_80286
#endif
#if defined(EXT_80286)
#define EXT_80186
#endif
//#define EXT_80186
//#define EXT_NECV20
#if !defined(EXT_80186) && !defined(EXT_NECV20)
#define UNDOCUMENTED_8086			//GLORIOUSCOW	 per cose non standard ma verificate sul campo da lui :D
#endif


#define CGA 1
//#define MDA 1
//#define VGA 1

#ifdef EXT_80286
#define PCAT 1
#endif


#ifndef PCAT		// non serve, ma v. CMOS
#define ROM_HD_BASE 0xc8000			 //PRIMA di option/ROM_START2 !  nb causa un mucchio di riletture status floppy al boot, su glabios...
#endif
//#define PC_IBM5150 1
//#define PC_IBM5160 1

//#define RAM_DOS 1
#define MS_DOS_VERSION 3			// occhio INT_1E nei bios! per #settori, anche non cambia nulla...

#define MOUSE_TYPE 1			// 1=Microsoft, 2=Mouse System  (per qualche motivo microsoft NON va su windows 2... non viene visto al boot
// ora sembra andare, ma a volte disturba la tastiera... credo sia troppo sensibile a VCC/USB e a posizione touch
//#warning ##################  verificare tastiera che salta dopo mouse (o timer troppo veloci??
// oppure se togli mouse il floppy non va! pd
// con 4 va, anche se i timer sono ovviamente troppo lenti, e i settaggi per delay IRQ floppy non fanno nulla
// AH NO, succede con alimentazione USB! con il PICKit no... e cmq non prende il click del touch, quindi ci deve essere qualche altro errore


#if !defined(EXT_80286)
#define CPU_CLOCK_DIVIDER 500000L			// così va più o meno come il PIC, su PC greggiod
#define RAM_SIZE 0x74000L
#define RAM_SIZE_EXTENDED 0
#else
#define CPU_CLOCK_DIVIDER 100000L			// provare
#define RAM_SIZE 0x74000L			//0x1000000L  //16MB
#define RAM_SIZE_EXTENDED 0x4000L		//160000
#endif
#define RAM_START 0L
#ifdef PCAT  //EXT_80286
#define ROM_SIZE 65536				// BIOS
#else
#ifdef PC_IBM5160
#define ROM_SIZE (32768/*+8192*/)				// BIOS
#else
#define ROM_SIZE 8192				// BIOS
#endif
#endif
#ifndef PCAT		// non ha senso glatick su PCAT!
#define ROM_SIZE2 2048			// BIOS opt
#define ROM_START2 0xD0000			// (preferred, dice https://github.com/640-KB/GLaTICK/wiki/Option-ROM-How-to
#endif
#define ROM_END 0x100000L

#if ROM_SIZE<=8192
#define ROM_BASIC 1    //
#endif

#ifdef VGA
#define VGA_BIOS_BASE 0xC0000L					// https://flint.cs.yale.edu/feng/cos/resources/BIOS/mem.htm
#endif

#include "8086.h"

extern const uint8_t bios_rom[],bios_rom_opt[];
extern const uint8_t HDbios[];
//extern const uint8_t MBR[512];
extern const uint8_t IBMBASIC[];
//extern const uint8_t msdossys[];
//extern const uint8_t iosys[];
//extern const uint8_t commandcom[];
    
#define FCY 205000000ul    //Oscillator frequency; ricontrollato con baud rate, pare giusto così!
#define FOSC 8000000ul

#define CPU_CLOCK_HZ             (FCY)    // CPU Clock Speed in Hz
#define CPU_CT_HZ            (CPU_CLOCK_HZ/2)    // CPU CoreTimer   in Hz
#define PERIPHERAL_CLOCK_HZ      (FCY/2 /*100000000UL*/)    // Peripheral Bus  in Hz
#define GetSystemClock()         (FCY)    // CPU Clock Speed in Hz
#define GetPeripheralClock()     (PERIPHERAL_CLOCK_HZ)    // Peripheral Bus  in Hz

#define US_TO_CT_TICKS  (CPU_CT_HZ/1000000UL)    // uS to CoreTimer Ticks
    
#define VERNUML 31
#define VERNUMH 1


typedef char BOOL;
typedef unsigned char UINT8;
typedef unsigned char BYTE;
typedef signed char INT8;
typedef unsigned short int WORD;
typedef unsigned long UINT32;
typedef signed long INT32;
typedef unsigned short int UINT16;
typedef signed int INT16;

typedef uint32_t COLORREF;
typedef uint16_t GFX_COLOR;

#define RGB(r,g,b)      ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((uint32_t)(BYTE)(b))<<16)))


#define TRUE 1
#define FALSE 0

  
volatile BYTE TIMIRQ,VIDIRQ,KBDIRQ,SERIRQ,RTCIRQ;
extern volatile BYTE keysFeedPtr;


#ifdef ST7735
typedef uint8_t GRAPH_COORD_T;
typedef uint8_t UGRAPH_COORD_T;
#endif
#ifdef ILI9341
typedef uint16_t GRAPH_COORD_T;
typedef uint16_t UGRAPH_COORD_T;
#endif

void mySYSTEMConfigPerformance(void);
void myINTEnableSystemMultiVectoredInt(void);

#define ReadCoreTimer()          _CP0_GET_COUNT()           // Read the MIPS Core Timer

void __attribute__((used)) __delay_us(unsigned int usec);
void __attribute__((used)) __delay_ms(unsigned int ms);

#define ClrWdt() { WDTCONbits.WDTCLRKEY=0x5743; }


// PIC32 RTCC Structure
typedef union {
  struct {
    uint8_t   weekday;    // BCD codification for day of the week, 00-06
    uint8_t   mday;       // BCD codification for day of the month, 01-31
    uint8_t   mon;        // BCD codification for month, 01-12
    uint8_t   year;       // BCD codification for years, 00-99
  	};                              // field access	
  uint8_t       b[4];       // byte access
  uint16_t      w[2];       // 16 bits access
  uint32_t       l;          // 32 bits access
	} PIC32_RTCC_DATE;

// PIC32 RTCC Structure
typedef union {
  struct {
    uint8_t   reserved;   // reserved for future use. should be 0
    uint8_t   sec;        // BCD codification for seconds, 00-59
    uint8_t   min;        // BCD codification for minutes, 00-59
    uint8_t   hour;       // BCD codification for hours, 00-24
  	};                              // field access
  uint8_t       b[4];       // byte access
  uint16_t      w[2];       // 16 bits access
  uint32_t       l;          // 32 bits access
	} PIC32_RTCC_TIME;
extern volatile PIC32_RTCC_DATE currentDate;
extern volatile PIC32_RTCC_TIME currentTime;



void Timer_Init(void);
void PWM_Init(void);
void ADC_Init(void);
void UART_Init(uint32_t);
void putsUART1(unsigned int *buffer);

int decodeKBD(int, long, BOOL);
#ifdef EXT_80386
uint8_t GetPipe(struct REGISTERS_SEG *,uint32_t);
uint8_t GetMorePipe(struct REGISTERS_SEG *,uint32_t);
uint8_t GetValue(struct REGISTERS_SEG *,uint32_t);
uint16_t GetShortValue(struct REGISTERS_SEG *,uint32_t);
uint32_t GetIntValue(struct REGISTERS_SEG *,uint32_t);
uint8_t InValue(uint16_t);
uint16_t InShortValue(uint16_t);
uint16_t InIntValue(uint16_t);
void PutValue(struct REGISTERS_SEG *,uint32_t, uint8_t);
void PutShortValue(struct REGISTERS_SEG *,uint32_t, uint16_t);
void PutIntValue(struct REGISTERS_SEG *,uint32_t, uint32_t);
void OutValue(uint16_t, uint8_t);
void OutShortValue(uint16_t, uint16_t);
void OutIntValue(uint32_t, uint16_t);
#else
uint8_t GetPipe(struct REGISTERS_SEG *,uint16_t);
uint8_t GetMorePipe(struct REGISTERS_SEG *,uint16_t);
uint8_t GetValue(struct REGISTERS_SEG *,uint16_t);
uint16_t GetShortValue(struct REGISTERS_SEG *,uint16_t);
uint8_t InValue(uint16_t);
uint16_t InShortValue(uint16_t);
void PutValue(struct REGISTERS_SEG *,uint16_t, uint8_t);
void PutShortValue(struct REGISTERS_SEG *,uint16_t, uint16_t);
void OutValue(uint16_t, uint8_t);
void OutShortValue(uint16_t, uint16_t);
#endif

int Emulate(int);
void initHW(void);
uint8_t to_bcd(uint8_t n);
uint8_t from_bcd(uint8_t n);
char *strcpyhl(char *,const char *);

int UpdateScreen(int /*uint16_t*/,int /*uint16_t*/);
extern const uint8_t CGAfont[8192];

void LCDXY(uint8_t, uint8_t);
void LCDCls();
void LCDWrite(const char *);



#ifdef ST7735           // ST7735 160x128 su Arduino (?? 2024)
// pcb SDRradio 2019
#define LED1 LATEbits.LATE2
#define LED2 LATEbits.LATE3
#define LED3 LATEbits.LATE4
#define SW1  PORTDbits.RD2
#define SW2  PORTDbits.RD3

#define	SPISDITris 0		// niente qua
#define	SPISDOTris TRISGbits.TRISG8				// SDO
#define	SPISCKTris TRISGbits.TRISG6				// SCK
#define	SPICSTris  TRISGbits.TRISG7				// CS
#define	LCDDCTris  TRISEbits.TRISE7				// DC che su questo LCD è "A0" per motivi ignoti
//#define	LCDRSTTris TRISBbits.TRISB7
	
#define	m_SPISCKBit LATGbits.LATG6		// pin 
#define	m_SPISDOBit LATGbits.LATG8		// pin 
#define	m_SPISDIBit 0
#define	m_SPICSBit  LATGbits.LATG7		// pin 
#define	m_LCDDCBit  LATEbits.LATE7 		// pin 
//#define	m_LCDRSTBit LATBbits.LATB7 //FARE
//#define	m_LCDBLBit  LATBbits.LATB12
#endif

#ifdef ILI9341      // su arduino

#define LED1 LATEbits.LATE4
#define LED2 LATDbits.LATD0
#define LED3 LATDbits.LATD11
#define SW2  PORTFbits.RF0
#define SW1  PORTBbits.RB0          // bah uso AREF tanto per...          

#define	LCDDCTris  TRISBbits.TRISB3				// http://attach01.oss-us-west-1.aliyuncs.com/IC/Datasheet/11009.zip?spm=a2g0o.detail.1000023.9.70352ae94rI9S1&file=11009.zip
#define	LCDRSTTris TRISBbits.TRISB10

#define	LCDRDTris  TRISBbits.TRISB5          // 
#define	LCDWRTris  TRISBbits.TRISB4          // WR per LCD parallelo
#define	LCDSTRTris  TRISBbits.TRISB4         // Strobe per LCD parallelo A3_TRIS (in pratica Write...)

#define	LCDCSTris  TRISBbits.TRISB2

#define	m_LCDDCBit  LATBbits.LATB3 		// 
#define	m_LCDRSTBit LATBbits.LATB10
//#define	m_LCDBLBit  LATBbits.LATB12

#define	m_LCDRDBit  LATBbits.LATB5 		// 
#define	m_LCDWRBit  LATBbits.LATB4 		// per LCD parallelo ILI
#define	m_LCDSTRBit LATBbits.LATB4        // non è chiaro... m_A3_out; in pratica è WRITE

#define	m_LCDCSBit  LATBbits.LATB2      // anche touch

#define m_TouchY1   LATBbits.LATB4  //AN1     // https://forum.arduino.cc/t/ili9341-shield-touchscreen-initial-setup/1171638  o https://forum.arduino.cc/t/ili9488-touchscreen-via-mcufriend_kbv-h/695844
                                                //(NO PUTTANOGESU https://www.hackster.io/calogerus/arduino-uno-2-4-tft-lcd-display-shield-touch-panel-ili9341-576b1b
#define TouchY1Tris TRISBbits.TRISB4
#define m_TouchX1   LATBbits.LATB3  //AN2
#define TouchX1Tris TRISBbits.TRISB3
//#define m_TouchX2   PORTEbits.RE6  //AN16 ev.
#define m_TouchY2   LATEbits.LATE5  // IO7
#define TouchY2Tris TRISEbits.TRISE5
//#define m_TouchY2   PORTEbits.RE7  //AN15 ev.
#define m_TouchX2   LATFbits.LATF5  // IO6
#define TouchX2Tris TRISFbits.TRISF5

#endif

#endif
