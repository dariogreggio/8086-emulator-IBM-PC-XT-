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



#undef EXT_80286
#undef EXT_80386
#undef EXT_80486
//#define EXT_80x87
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

//#define PCAT 1

#define ROM_BASIC 1    //
//#define ROM_HD_BASE 0xc8000			 PRIMA di option/ROM_START2 !
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


#define CPU_CLOCK_DIVIDER 500000L			// così va più o meno come il PIC, su PC greggiod
#define RAM_SIZE 0x74000L
#define RAM_START 0L
#define ROM_SIZE 8192				// BIOS
#define ROM_SIZE2 2048			// BIOS opt
#define ROM_START2 0xD0000			// (preferred, dice https://github.com/640-KB/GLaTICK/wiki/Option-ROM-How-to
#define ROM_END 0x100000L

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
    
#define VERNUML 2
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
union DWORD_BYTES {
	uint32_t d;
	struct {
		uint16_t l;
		uint16_t h;
		} w;
	uint8_t bd[4];
	struct {
		uint8_t l;
		uint8_t h;
		uint8_t u;
		uint8_t u2;
		} b;
	};
union REG_8086 {
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
#ifndef EXT_80386
	union __attribute__((__packed__)) REGISTERS {
		BYTE b[16];
	  union __attribute__((__packed__)) REG_8086 r[8];
		};
#else
	union __attribute__((__packed__)) REGISTERS {
		BYTE b[32];
	  union __attribute__((__packed__)) REG_8086 r[8];
		};
#endif
#ifndef EXT_80386
	union __attribute__((__packed__)) REGISTERS16 {
	  union __attribute__((__packed__)) REG_8086 r[4];
		};
#else
	union __attribute__((__packed__)) REGISTERS32 {
	  union __attribute__((__packed__)) REG_8086 r[6];
		};
#endif
#ifdef EXT_80286
	union SEGMENT_SELECTOR {		// 
    uint16_t x;
		struct {
			unsigned int RPL: 1;
			unsigned int TI: 2;
			unsigned int Index: 13;
			};
		};
	union MACHINE_STATUS_WORD {		// 
    uint16_t x;
		struct {
			unsigned int PE: 1;
			unsigned int MP: 1;
			unsigned int EM: 1;
			unsigned int TS: 1;
			unsigned int unused: 12;
			};
		};
	union SEGMENT_DESCRIPTOR {
    uint64_t x;
		struct {
			unsigned int Limit: 16;
			unsigned int Base: 16;
			unsigned int BaseH: 8;
			unsigned int A: 1;
			unsigned int Type: 3;
			unsigned int System: 1;		// 1=segm descr; 0=system/control segm descr
			unsigned int DPL: 2;
			unsigned int P: 1;
			unsigned int reserved: 16;
			};
		};
#endif
#ifdef EXT_80386
	union SEGMENT_DESCRIPTOR {		// https://stackoverflow.com/questions/30253678/x86-segment-descriptor-layout-why-is-it-weird
    uint64_t x;
		struct {
			unsigned int Limit: 16;
			unsigned int Address: 16;
			unsigned int AddressH: 8;
			unsigned int Type: 4;
			unsigned int System: 1;
			unsigned int DPL: 2;
			unsigned int P: 1;
			unsigned int LimitH: 4;
			unsigned int A: 1;
			unsigned int unused: 1;
			unsigned int DB: 1;
			unsigned int G: 1;
			unsigned int AddressU: 8;
			};
		};
#endif

	union __attribute__((__packed__)) REGISTRO_F {
    uint16_t x;
		struct __attribute__((__packed__)) {
			unsigned int Carry: 1;
			unsigned int unused: 1;		// 1 fisso
			unsigned int Parity: 1;
			unsigned int unused2: 1;
			unsigned int Aux: 1;
			unsigned int unused3: 1;
			unsigned int Zero: 1;
			unsigned int Sign: 1;
			unsigned int Trap: 1;
			unsigned int IF: 1;
			unsigned int Dir: 1;
			unsigned int Ovf: 1;
#ifdef EXT_80286
			unsigned int IOPL: 2;
			unsigned int NestedTask: 1;
#else
			unsigned int unused3_: 3;
#endif
#ifdef EXT_NECV20
			unsigned int MD: 1;
#else
			unsigned int unused4: 1;
#endif
#ifdef EXT_80386				// https://blog.naver.com/krquddnr37/220271675934?viewType=pc
			unsigned int ResumeFlag: 1;
			unsigned int VirtualMode: 1;
			unsigned int AlignmentCheck: 1;
			unsigned int VIF: 1;
			unsigned int VIP: 1;
			unsigned int ID: 1;
			unsigned int unused5: 10;
#endif
      };
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
  
  
  
enum __attribute__((__packed__)) {
  DoReset=1,
  DoNMI=2,
  DoIRQ=4,
  DoHalt=8
  };
  
  
volatile BYTE TIMIRQ,VIDIRQ,KBDIRQ,SERIRQ,RTCIRQ;
extern volatile BYTE keysFeedPtr;




#define COMPUTE_RM_OFS \
	_ip++;\
  switch(Pipe2.mod) {\
	/*bisognerebbe gestire il GetMorePipe() qua, dinamicamente*/\
    case 2:\
      _ip++;\
    case 1:\
      _ip++;\
    case 0:\
      immofs=1;\
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
            immofs+=2;\
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

#define GET_MEM_OPER \
  switch(Pipe2.mod) {\
		case 0:\
			break;\
		case 1:\
			op2.mem+=(int16_t)(int8_t)Pipe2.b.h;\
			immofs+=1;\
			break;\
		case 2:\
			op2.mem+=Pipe2.xm.w;\
			immofs+=2;\
			break;\
		case 3:/*gestito a parte*/\
			break;\
		}

#define COMPUTE_RM \
	GET_REGISTER_8_16\
	COMPUTE_RM_OFS\
		GET_MEM_OPER

#define GET_REGISTER_8_16 \
	if(Pipe1 & 1)\
		op1.reg16= &regs.r[Pipe2.reg].x;\
	else\
		op1.reg8= Pipe2.reg & 0x4 ? &regs.r[Pipe2.reg & 0x3].b.h : &regs.r[Pipe2.reg & 0x3].b.l;

#define GET_REGISTER_8_16_2 \
	if(Pipe1 & 1)\
		op2.reg16= &regs.r[Pipe2.rm].x;\
	else\
	  op2.reg8= Pipe2.rm & 0x4 ? &regs.r[Pipe2.rm & 0x3].b.h : &regs.r[Pipe2.rm & 0x3].b.l;


#define PUSH_STACK(a) 	{ _sp-=2;	PutShortValue(_ss,_sp,(uint16_t)a); }
#define POP_STACK(a) 	{ a=GetShortValue(_ss,_sp);	_sp+=2; }

#define ROTATE_SHIFT8 \
	switch(Pipe2.reg) {\
    	union REGISTRO_F _f1;\
		case 0:       /* ROL */\
			while(res2.b--) {\
				_f.Carry=res3.b & 0x80 ? 1 : 0;\
				res3.b <<= 1;\
				res3.b |= _f.Carry;\
				}\
			if(i==1)/*gloriouscow lo fa comunque*/\
				_f.Ovf=(!!_f.Carry != !!(res3.b & 0x80)) ? 1 : 0;		/* Ovf= Carry uscente XOR MSB uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 1:       /* ROR*/\
			while(res2.b--) {\
				_f.Carry=res3.b & 1;\
				res3.b >>= 1;\
				if(_f.Carry)\
					res3.b |= 0x80;\
				}\
			if(i==1)\
				_f.Ovf=(!!(res3.b & 0x80) != !!(res3.b & 0x40)) ? 1 : 0;		/* Ovf= B7 XOR B6 uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 2:       /* RCL*/\
			while(res2.b--) {\
				_f1.Carry=_f.Carry;\
				_f.Carry=res3.b & 0x80 ? 1 : 0;\
				res3.b <<= 1;\
				res3.b |= _f1.Carry;\
				}\
			if(i==1)\
				_f.Ovf=(!!_f.Carry != !!(res3.b & 0x80)) ? 1 : 0;		/* Ovf= Carry uscente XOR MSB uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 3:       /* RCR*/\
			while(res2.b--) {\
				_f1.Carry=_f.Carry;\
				_f.Carry=res3.b & 1;\
				res3.b >>= 1;\
				if(_f1.Carry)\
					res3.b |= 0x80;\
				}\
			if(i==1)\
				_f.Ovf=(!!(res3.b & 0x80) != !!(res3.b & 0x40)) ? 1 : 0;		/* Ovf= B7 XOR B6 uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 6:       /* SAL/SHL ... bah mi sembrano identiche! ma sarebbe SETMO :D secondo unofficial */\
		case 4:       /* SHL/SAL*/\
			while(res2.b--) {\
				_f.Carry=res3.b & 0x80 ? 1 : 0;\
				res3.b <<= 1;\
				}\
			if(i==1)\
				_f.Ovf=(res3.b ^ res1.b) & 0x80 ? 1 : 0;		/* Ovf=0 se MSB uscente è = MSB entrante / il segno è preservato (https://c9x.me/x86/html/file_module_x86_id_285.html*/\
			break;\
		case 5:       /* SHR*/\
			while(res2.b--) {\
				_f.Carry=res3.b & 1;\
				res3.b = ((uint8_t)res3.b) >> 1;\
				}\
			if(i==1)\
				_f.Ovf=res1.b & 0x80 ? 1 : 0;		/* Ovf= MSB entrante (https://c9x.me/x86/html/file_module_x86_id_285.html*/\
			break;\
		case 7:       /* SAR*/\
			while(res2.b--) {\
				_f.Carry=res3.b & 1;\
				res3.b = ((int8_t)res3.b) >> 1;\
				}\
			if(i==1)\
				_f.Ovf=0;		/* Ovf=0 (https://c9x.me/x86/html/file_module_x86_id_285.html*/\
			break;\
		}
// _f.Aux=0;			// pare... da gloriouscow...


#define ROTATE_SHIFT16 \
	switch(Pipe2.reg) {	\
    	union REGISTRO_F _f1;\
		case 0:       /* ROL*/\
			while(res2.b--) {\
				_f.Carry=res3.x & 0x8000 ? 1 : 0;\
				res3.x <<= 1;\
				res3.x |= _f.Carry;\
				}\
			if(i==1)/*gloriouscow lo fa comunque*/\
				_f.Ovf=(!!_f.Carry != !!(res3.x & 0x8000)) ? 1 : 0;		/* Ovf= Carry uscente XOR MSB uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 1:       /* ROR*/\
			while(res2.b--) {\
				_f.Carry=res3.x & 1;\
				res3.x >>= 1;\
				if(_f.Carry)\
					res3.x |= 0x8000;\
				}\
			if(i==1)\
				_f.Ovf=(!!(res3.x & 0x8000) != !!(res3.x & 0x4000)) ? 1 : 0;		/* Ovf= B7 XOR B6 uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 2:       /* RCL*/\
			while(res2.b--) {\
				_f1.Carry=_f.Carry;\
				_f.Carry=res3.x & 0x8000 ? 1 : 0;\
				res3.x <<= 1;\
				res3.x |= _f1.Carry;\
				}\
			if(i==1)\
				_f.Ovf=(!!_f.Carry != !!(res3.x & 0x8000)) ? 1 : 0;		/* Ovf= Carry uscente XOR MSB uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 3:       /* RCR*/\
			while(res2.b--) {\
				_f1.Carry=_f.Carry;\
				_f.Carry=res3.x & 1;\
				res3.x >>= 1;\
				if(_f1.Carry)\
					res3.x |= 0x8000;\
				}\
			if(i==1)\
				_f.Ovf=(!!(res3.x & 0x8000) != !!(res3.x & 0x4000)) ? 1 : 0;		/* Ovf= B7 XOR B6 uscente (https://c9x.me/x86/html/file_module_x86_id_273.html*/\
			break;\
		case 6:       /* SAL/SHL ... bah mi sembrano identiche! dice che su 8088 sono undocumented SETMO SETMOC */\
		case 4:       /* SHL/SAL*/\
			while(res2.b--) {\
				_f.Carry=res3.x & 0x8000 ? 1 : 0;\
				res3.x <<= 1;\
				}\
			if(i==1)\
				_f.Ovf=(res3.x ^ res1.x) & 0x8000 ? 1 : 0;		/* Ovf=0 se MSB uscente è = MSB entrante / il segno è preservato (https://c9x.me/x86/html/file_module_x86_id_285.html*/\
			break;\
		case 5:       /* SHR*/\
			while(res2.b--) {\
				_f.Carry=res3.x & 1;\
				res3.x = ((uint16_t)res3.x) >> 1;\
				}\
			if(i==1)\
				_f.Ovf=res1.x & 0x8000 ? 1 : 0;		/* Ovf= MSB entrante (https://c9x.me/x86/html/file_module_x86_id_285.html*/\
			break;\
		case 7:       /* SAR*/\
			while(res2.b--) {\
				_f.Carry=res3.x & 1;\
				res3.x = ((int16_t)res3.x) >> 1;\
				}\
			if(i==1)\
				_f.Ovf=0;		/* Ovf=0 (https://c9x.me/x86/html/file_module_x86_id_285.html*/\
			break;\
		}
// _f.Aux=0;			// pare... da gloriouscow...



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
uint8_t  GetPipe(uint16_t,uint32_t);
uint8_t  GetMorePipe(uint16_t,uint32_t);
uint8_t  GetValue(uint16_t,uint32_t);
uint16_t  GetShortValue(uint16_t,uint32_t);
uint32_t  GetIntValue(uint16_t,uint32_t);
uint8_t  InValue(uint16_t);
uint16_t  InShortValue(uint16_t);
uint16_t  InIntValue(uint16_t);
void  PutValue(uint16_t,uint32_t, uint8_t);
void  PutShortValue(uint16_t,uint32_t, uint16_t);
void  PutIntValue(uint16_t,uint32_t, uint32_t);
void  OutValue(uint16_t, uint8_t);
void  OutShortValue(uint16_t, uint16_t);
void  OutIntValue(uint32_t, uint16_t);
#else
uint8_t  GetPipe(uint16_t,uint16_t);
uint8_t  GetMorePipe(uint16_t,uint16_t);
uint8_t  GetValue(uint16_t,uint16_t);
uint16_t  GetShortValue(uint16_t,uint16_t);
uint8_t  InValue(uint16_t);
uint16_t  InShortValue(uint16_t);
void  PutValue(uint16_t,uint16_t, uint8_t);
void  PutShortValue(uint16_t,uint16_t, uint16_t);
void  OutValue(uint16_t, uint8_t);
void  OutShortValue(uint16_t, uint16_t);
#endif

int Emulate(int);
void initHW(void);
uint8_t to_bcd(uint8_t n);
uint8_t from_bcd(uint8_t n);

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

#define m_TouchX1   LATBbits.LATB3  //AN3     // https://www.hackster.io/calogerus/arduino-uno-2-4-tft-lcd-display-shield-touch-panel-ili9341-576b1b
//#define m_TouchX2   PORTEbits.RE6  //AN16 ev.
#define m_TouchX2   LATEbits.LATE6  //
#define m_TouchY1   LATBbits.LATB2  //AN2
//#define m_TouchY2   PORTEbits.RE7  //AN15 ev.
#define m_TouchY2   LATEbits.LATE7  //

#endif

#endif
