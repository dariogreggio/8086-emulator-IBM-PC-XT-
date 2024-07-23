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

//#define ROM_BASIC 1    //non va, verificare versione ecc 21/7/24 // https://github.com/jeffpar/pcjs/tree/gh-pages/machines/pcx86/ibm/5150/rom/basic/
#define RAM_DOS 1


extern const uint8_t bios_rom[];
extern const uint8_t MBR[512];
extern const uint8_t IBMBASIC[];
extern const uint8_t msdossys[];
extern const uint8_t iosys[];
extern const uint8_t commandcom[];
    
#define FCY 205000000ul    //Oscillator frequency; ricontrollato con baud rate, pare giusto così!
#define FOSC 8000000ul

#define CPU_CLOCK_HZ             (FCY)    // CPU Clock Speed in Hz
#define CPU_CT_HZ            (CPU_CLOCK_HZ/2)    // CPU CoreTimer   in Hz
#define PERIPHERAL_CLOCK_HZ      (FCY/2 /*100000000UL*/)    // Peripheral Bus  in Hz
#define GetSystemClock()         (FCY)    // CPU Clock Speed in Hz
#define GetPeripheralClock()     (PERIPHERAL_CLOCK_HZ)    // Peripheral Bus  in Hz

#define US_TO_CT_TICKS  (CPU_CT_HZ/1000000UL)    // uS to CoreTimer Ticks
    
#define VERNUML 91
#define VERNUMH 0


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


enum {
  DoReset=1,
  DoNMI=2,
  DoIRQ=4,
  DoHalt=8
  };

union __attribute__((__packed__)) PIPE {
	uint32_t d;
	uint8_t bd[8];
	uint16_t wd[4];
	struct __attribute__((__packed__)) {
		uint16_t l;
		uint16_t h;
		} x;
	struct __attribute__((__packed__)) {
		uint8_t dummy;
		uint16_t w;
		} xm;
	struct __attribute__((__packed__)) {
		uint8_t l;
		uint8_t h;
		uint8_t u;		 // oppure spostare la pipe quando ci sono le istruzioni lunghe 4+...
//		uint8_t u2;
//		uint8_t u3;
//		uint8_t u4;
		} b;
    struct __attribute__((__packed__)) {
        unsigned int rm:3;
        unsigned int reg:3;
        unsigned int mod:2;
        };
	};
union __attribute__((__packed__)) REG_8086 {
#ifdef EXT_80386
	struct __attribute__((__packed__)) {
		uint16_t l;
		uint16_t h;
		} x;
	uint32_t d;
#else
  uint16_t x;
#endif
  struct __attribute__((__packed__)) { 
    uint8_t l;
    uint8_t h;
#ifdef EXT_80386
    uint8_t u;
    uint8_t u2;
#endif
    } b;
  };
union __attribute__((__packed__)) OPERAND {
  uint8_t *reg8;
  uint16_t *reg16;
#ifdef EXT_80386
  uint32_t *reg32;
  uint32_t mem;
#else
  uint16_t mem;
#endif
  };
union __attribute__((__packed__)) RESULT {
  uint8_t b;
  uint16_t x;
  uint32_t d;    // serve cmq per gli overflow/carry
#ifdef EXT_80386
  QWORD dd;
#endif
  };
  
volatile BYTE TIMIRQ,VIDIRQ,KBDIRQ,SERIRQ,RTCIRQ;
extern volatile BYTE keysFeedPtr;


#define MAKE20BITS(a,b) (((uint32_t)b) + (((uint32_t)a) << 4))



#define COMPUTE_RM_OFS \
  switch(Pipe2.mod) {\
    case 2:\
      _ip++;\
    case 1:\
      _ip++;\
    case 0:\
      switch(Pipe2.rm) {\
        case 0:\
          theDs=&_ds; op2.mem=_bx+_si; /*unsigned*/ \
          break;\
        case 1:\
          theDs=&_ds; op2.mem=_bx+_di;\
          break;\
        case 2:\
          theDs=&_ss; op2.mem=_bp+_si;\
          break;\
        case 3:\
          theDs=&_ss; op2.mem=_bp+_di;\
          break;\
        case 4:\
          theDs=&_ds; op2.mem=_si;\
          break;\
        case 5:\
          theDs=&_ds; op2.mem=_di;\
          break;\
        case 6:\
          if(Pipe2.mod) {\
            theDs=&_ss;\
            op2.mem=_bp;\
            }\
          else {\
            theDs=&_ds;\
            op2.mem=Pipe2.xm.w;\
            _ip+=2;\
            }\
          break;\
        case 7:\
          theDs=&_ds; op2.mem=_bx;\
          break;\
        }\
        if(segOverride) {\
            theDs=&segs.r[segOverride-1].x;\
            segOverride=0;\
            }

#define COMPUTE_RM \
	_ip++;\
	if(Pipe1 & 1)\
		op1.reg16= &regs.r[Pipe2.reg].x;\
	else\
		op1.reg8= Pipe2.b.l & 0x20 ? &regs.r[Pipe2.reg & 3].b.h : &regs.r[Pipe2.reg & 0x3].b.l;\
	COMPUTE_RM_OFS\
      if(Pipe2.mod == 1)\
        op2.mem+=(int8_t)Pipe2.b.h;\
      else if(Pipe2.mod == 2)\
        op2.mem+=Pipe2.xm.w;


#define PUSH_STACK(a) 	{ _sp-=2;	PutShortValue(MAKE20BITS(_ss,_sp),a); }
#define POP_STACK(a) 	{ a=GetShortValue(MAKE20BITS(_ss,_sp));	_sp+=2; }


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
void UART_Init(uint32_t);
void putsUART1(unsigned int *buffer);

int decodeKBD(int, long, BOOL);
uint8_t GetValue(uint32_t);
uint8_t InValue(uint16_t);
uint16_t InIntValue(uint16_t);
uint16_t GetShortValue(uint32_t);
uint32_t GetIntValue(uint32_t);
void PutValue(uint32_t, uint8_t);
void PutShortValue(uint32_t, uint16_t);
void PutIntValue(uint32_t, uint32_t);
void OutValue(uint16_t, uint8_t);
void OutIntValue(uint16_t, uint16_t);
int Emulate(int);
void initHW(void);

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

#define	m_LCDCSBit  LATBbits.LATB2

#endif

#endif
