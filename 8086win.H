//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
//#define STRICT

#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#ifndef WIN32
#include <shell.h>
#endif

#include "afxres.h"
#include "stdint.h"
#include "resource.h"


//#define EXT_80186 1
//#define EXT_NECV20
//#define EXT_80286 1
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
#define EXT_80186 1
#endif
#if !defined(EXT_80186) && !defined(EXT_NECV20)
#define UNDOCUMENTED_8086			//GLORIOUSCOW	 per cose non standard ma verificate sul campo da lui :D
#endif



//#define CGA 1
//#define MDA 1
#define VGA 1

//#define PC_IBM5150
//#define PC_IBM5160
#ifdef EXT_80286
#define PCAT 1
#endif

//#define DEBUG_TESTSUITE 1

//#define DEBUG_MEMORY 1
//#define DEBUG_8237 1
#define DEBUG_8253 1
//#define DEBUG_8259 1
//#define DEBUG_RTC 1
//#define DEBUG_KB 1
//#define DEBUG_FD 1
//#define DEBUG_HD 1
//#define DEBUG_VGA 1


//#pragma message("SPOSTARE questi "segmenti" >> 4 e togliere da GetShortValue ecc! ma OCCHIO")
// (e mettere controllo su REPecc che se segue istruzione non-stringa SALTA

#ifndef PCAT		// non serve, ma v. CMOS
#define ROM_HD_BASE 0xc8000			 //PRIMA di option/ROM_START2 !  nb causa un mucchio di riletture status floppy al boot, su glabios...
#endif
//#define RAM_DOS 1

#define MS_DOS_VERSION 3			// (occhio INT_1E nei bios! per #settori, anche se non cambia nulla...
// sembra che windows 2 voglia dos 3 (minimo) ... INT 2F multiplex

#define MOUSE_TYPE 1			// 1=Microsoft, 2=Mouse System  (ora va anche su windows 2... solo dopo un po' si blocca (credo come Atari, problema di semaforo


#ifdef DEBUG_TESTSUITE
#define RAM_SIZE 1048576L	/*per test*/		//0xa0000L
#define RAM_SIZE_EXTENDED 0
#define CPU_CLOCK_DIVIDER 1000000L		// 
#else
#if !defined(EXT_80286)
#define CPU_CLOCK_DIVIDER 1150000L		// circa 5x 4.77MHz con MIPS speed test, 28/7/25 ... non va ancora bene, anche se aumento non quadra (il risultante è 2 e non si può fare molto altro...
// ma OCCHIO clock timer e DMA!
			// (500000L			// così va più o meno come il PIC, su PC greggiod
#define RAM_SIZE 0xA0000  //1048576L  //0xa0000L prova per windows 3 VGA
#define RAM_SIZE_EXTENDED 0
#else
#define CPU_CLOCK_DIVIDER 1600000L		// circa 5x 4.77MHz con MIPS speed test, 28/7/25 ... non va ancora bene, anche se aumento non quadra (il risultante è 2 e non si può fare molto altro...
// ma OCCHIO clock timer e DMA!
#ifdef _DEBUG
#define RAM_SIZE 0x70000L			//0x1000000L  //16MB
#define RAM_SIZE_EXTENDED 0x10000L		//160000
#else
#define RAM_SIZE 0xa0000L			//0x1000000L  //16MB
#define RAM_SIZE_EXTENDED 0x60000L		//160000
#endif
#endif
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
#define VGA_BIOS_BASE 0xC0000L					// https://flint.cs.yale.edu/feng/cos/resources/BIOS/mem.htm
#ifndef PCAT		// non ha senso glatick su PCAT!
#define ROM_SIZE2 2048			// BIOS opt
#define ROM_START2 0xD0000			// (preferred, dice https://github.com/640-KB/GLaTICK/wiki/Option-ROM-How-to
#endif
#define ROM_END 0x100000L

#if ROM_SIZE<=8192
#define ROM_BASIC 1    //
#endif


#ifdef CGA
#define MIN_RASTER 0        // 
#define HORIZ_SIZE 640      // 
#define VERT_SIZE 200
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define MAX_RASTER (VERT_SIZE-1)
#endif
#ifdef MDA
#define MIN_RASTER 0        // 
#define HORIZ_SIZE 720      // 
#define VERT_SIZE 350			// 348?? in graphics forse
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define MAX_RASTER (VERT_SIZE-1)
#endif
#ifdef VGA
#define MIN_RASTER 0        // 
#define HORIZ_SIZE 640      // 
#define VERT_SIZE 400  //200
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define MAX_RASTER (VERT_SIZE-1)
#endif


typedef unsigned short int SWORD;

#pragma pack( push, before_structs )
#pragma pack(1)
union PIPE {
	uint32_t d;
	uint8_t bd[8];
	uint16_t wd[4];
	struct {
		uint16_t l;
		uint16_t h;
		} x;
	struct {
		uint8_t dummy;
		uint16_t w;
		} xm;
	struct {
		uint8_t l;
		uint8_t h;
		uint8_t u;		 // oppure spostare la pipe quando ci sono le istruzioni lunghe 4+...
//		uint8_t u2;
//		uint8_t u3;
//		uint8_t u4;
		} b;
  struct {
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
	struct {
		uint16_t l;
		uint16_t h;
		} x;
	uint32_t d;
#else
  uint16_t x;
#endif
  struct { 
    uint8_t l;
    uint8_t h;
#ifdef EXT_80386
    uint8_t u;
    uint8_t u2;
#endif
    } b;
  };
union OPERAND {
  uint8_t *reg8;
  uint16_t *reg16;
#ifdef EXT_80386
  uint32_t *reg32;
  uint32_t mem;
#else
  uint16_t mem;
#endif
  };
union RESULT {
  uint8_t b;
  uint16_t x;
  uint32_t d;    // 
#ifdef EXT_80386
  QWORD dd;
#endif
  };
#ifndef EXT_80386
	union REGISTERS {
		BYTE b[16];
	  union REG_8086 r[8];
		};
#else
	union REGISTERS {
		BYTE b[32];
	  union REG_8086 r[8];
		};
#endif
#ifndef EXT_80386
	union REGISTERS16 {
	  union REG_8086 r[4];
		};
#else
	union REGISTERS32 {
	  union REG_8086 r[6];
		};
#endif
#define ID_CARRY 0x1
#define ID_PARITY 0x4
#define ID_AUX 0x10
#define ID_ZERO 0x40
#define ID_SIGN 0x80
#define ID_TRAP 0x100
#define ID_IF  0x200
#define ID_DIR 0x400
#define ID_OVF 0x800
#define ID_IOPL 0x3000
#define ID_NESTEDTASK 0x4000
	union REGISTRO_F {
#ifdef EXT_80386
    uint32_t x;
#else
    uint16_t x;
#endif
		struct {
			unsigned char Carry: 1;
			unsigned char unused: 1;			// 1 fisso
			unsigned char Parity: 1;
			unsigned char unused2: 1;
			unsigned char Aux: 1;
			unsigned char unused3: 1;
			unsigned char Zero: 1;
			unsigned char Sign: 1;
			unsigned char Trap: 1;
			unsigned char IF: 1;
			unsigned char Dir: 1;
			unsigned char Ovf: 1;
#ifdef EXT_80286
			unsigned char IOPL: 2;
			unsigned char NestedTask: 1;
#else
			unsigned char unused3_: 3;
#endif
#ifdef EXT_NECV20
			unsigned char MD: 1;
#else
			unsigned char unused4: 1;
#endif
#ifdef EXT_80386				// https://blog.naver.com/krquddnr37/220271675934?viewType=pc
			unsigned char ResumeFlag: 1;
			unsigned char VirtualMode: 1;
			unsigned char AlignmentCheck: 1;
			unsigned char VIF: 1;
			unsigned char VID: 1;
			unsigned char ID: 1;
			unsigned short unused5: 10;
#endif
      };
		};
#ifdef EXT_80286
	union SEGMENT_SELECTOR {		// 
    uint16_t x;			//real mode compat
		struct {
			unsigned short int RPL: 2;			// SERVE SHORT o va a 4 byte! e non char che si incasina Index... 
			unsigned short int TI: 1;
			unsigned short int Index: 13;
			};
		};
	union MACHINE_STATUS_WORD {		// 
    uint16_t x;
		struct {
			unsigned char PE: 1;
			unsigned char MP: 1;
			unsigned char EM: 1;
			unsigned char TS: 1;
			unsigned short unused: 12;
			};
		};
	struct SEGMENT_DESCRIPTOR {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union {
			uint8_t b;
			struct {
				unsigned char A: 1;
				unsigned char Type: 3;
				unsigned char System: 1;		// 1=segm descr; 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct SEGMENT_DESCRIPTOR_SYSTEM {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union {
			uint8_t b;
			struct {
				unsigned char Type: 4;
				unsigned char System: 1;		// 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct SEGMENT_DESCRIPTOR_GATE {
		unsigned short Offset: 16;
		unsigned short Selector: 16;
		unsigned char WordCount: 5;
		unsigned char unused: 3;
		union {
			uint8_t b;
			struct {
				unsigned char Type: 4;
				unsigned char System: 1;		// 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct SEGMENT_DESCRIPTOR_INTERRUPT_TRAP {
		unsigned short Offset: 16;
		unsigned short Selector: 16;
		unsigned char unused: 8;
		union {
			uint8_t b;
			struct {
				unsigned char Type: 5;		// 6=interrupt, 7 =gate
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct SEGMENT_DESCRIPTOR_TASK_GATE {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union {
			uint8_t b;
			struct {
				unsigned char A: 1;
				unsigned char Type: 3;			// 001
				unsigned char System: 1;		// 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct SEGMENT_DESCRIPTOR_TSS {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union {
			uint8_t b;
			struct {
				union {
					unsigned char Type:4;		// non va... si spostano in avanti i bit a seguire...
					struct {
						unsigned char Id0:1;		// fisso 1
						unsigned char Busy:1;
						unsigned char Id1:1;		// fisso 1
						unsigned char Id2:1;		// fisso 0
						};
					};
				unsigned char System: 1;		// 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct REGISTERS_SEG {
		union SEGMENT_SELECTOR s;
		struct SEGMENT_DESCRIPTOR d;
		};
	union _DTR {
    uint8_t b[6];
		struct {
			unsigned short Limit: 16;
//			unsigned long Base: 24;		// non ci piace, si sposta in avanti £$%@#  RIPROVARE ev. con short ecc
			unsigned short Base: 16;
			unsigned char BaseH: 8;
			unsigned char unused: 8;
			};
		};
	union INTERRUPT_WORD {
    uint16_t x;
		struct {
			unsigned char Type: 1;
			unsigned char Type2: 1;			// boh dice 1 fisso
			unsigned char unused0: 1;		// dice 0
			unsigned char Vector: 8;
			unsigned char unused: 5;
			};
		};
	struct TASK_STATE_REGISTER {
		uint16_t PTSS;			// previous TSS
		uint16_t SS_CPL0;		//(never altered) sp for CPL 0
		uint16_t SP_SPL0;		//(never altered) ss for CPL 0
		uint16_t SS_CPL1;		//
		uint16_t SP_SPL1;		//
		uint16_t SS_CPL2;		//
		uint16_t SP_SPL2;		//
		uint16_t EntryPoint;	//ip (entry point)
		union REGISTRO_F Flags;			//flag: (I/O privilege level 
		uint16_t Ax,Bx,Cx,Dx;
		uint16_t Sp,Bp,Si,Di;
		uint16_t Es,Cs,Ss,Ds;
		uint16_t Ldt;				// task ldt selector
		};

#else
	struct REGISTERS_SEG {
		union {
			uint16_t x;		// real mode 
			} s;
		};
	union INTERRUPT_WORD {
    uint16_t x;
		struct {
			unsigned char Vector: 8;
			unsigned char unused: 8;
			};
		};
#endif
union SEGMENTS_DESCRIPTOR {
	struct REGISTERS_SEG r[4];
	};
#ifdef EXT_80386
	union CR0_MACHINE_STATUS_WORD {		// 
    uint16_t x;
		struct {
			unsigned char PE: 1;
			unsigned char MP: 1;
			unsigned char EM: 1;
			unsigned char TS: 1;
			unsigned char ET: 1;		// Bit 4 (ET) : ExtensionType Flag. This tells us what type of coprocessor is installed.
//    0 - 80287 is installed
//    1 - 80387 is installed.
			unsigned char unused2: 1;
			unsigned char PG: 1;		// Bit 6 (PG) : Enables Memory Paging.
			unsigned short unused: 9;
			};
		};
	union SEGMENT_DESCRIPTOR {		// https://stackoverflow.com/questions/30253678/x86-segment-descriptor-layout-why-is-it-weird
    uint64_t x;
		struct {
			unsigned char Limit: 16;
			unsigned char Address: 16;
			unsigned char AddressH: 8;
			unsigned char Type: 4;
			unsigned char System: 1;
			unsigned char DPL: 2;
			unsigned char P: 1;
			unsigned char LimitH: 4;
			unsigned char A: 1;
			unsigned char unused: 1;
			unsigned char DB: 1;
			unsigned char G: 1;
			unsigned char AddressU: 8;
			};
		};
	union _DTR {
    uint8_t b[8];
		struct {
			unsigned short Limit: 16;
//			unsigned long Base: 24;		// non ci piace, si sposta in avanti £$%@#
			unsigned short Base: 16;
			unsigned char BaseH: 8;
			unsigned char LimitH: 4;
			unsigned char Flags: 4;
			unsigned char BaseU: 8;
			};
		};
#endif
#ifdef EXT_80286
	enum EXCEPTIONS {
		EXCEPTION_DIVIDE0=0,			// divide by 0
		EXCEPTION_TRACE=1,			// 
		EXCEPTION_BPOINT=3,			// "breakpoint" non ci piace... 
		EXCEPTION_OVERFLOW=4,
		EXCEPTION_BOUND=5,
		EXCEPTION_UD=6,			// invalid opcode
		EXCEPTION_NOMATH=7,			// 
		EXCEPTION_DOUBLEFAULT=8,			// double fault
		EXCEPTION_MATHPROTECTION=9,			// MATH			no! segment overrun
		EXCEPTION_INVALIDTASK=10,		// invalid task
		EXCEPTION_NP=11,			// not present
		EXCEPTION_STACK=12,		// stack fault
		EXCEPTION_GP=13,			// protection
		EXCEPTION_MATH=16,			//
		};
struct _EXCEPTION {
#ifdef EXT_80386
  DWORD addr;
#else
  WORD addr;
#endif
  WORD addrH;
	DWORD parm;
  union {
    uint16_t w;
    struct {
      unsigned char ud:8;
      unsigned char hascode:1;
      unsigned char in:1;
      unsigned char rw:1;
      };
    } descr;
  BYTE active;
  };
#endif

#pragma pack( pop, before_structs )


enum {
  DoReset=1,
  DoNMI=2,
  DoIRQ=4,
  DoHalt=8
  };



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
          theDs=_ds; op2.mem=_bx+_si; /*unsigned*/ \
          break;\
        case 1:\
          theDs=_ds; op2.mem=_bx+_di;\
          break;\
        case 2:\
          theDs=_ss; op2.mem=_bp+_si;\
          break;\
        case 3:\
          theDs=_ss; op2.mem=_bp+_di;\
          break;\
        case 4:\
          theDs=_ds; op2.mem=_si;\
          break;\
        case 5:\
          theDs=_ds; op2.mem=_di;\
          break;\
        case 6:\
          if(Pipe2.mod) {\
            theDs=_ss;\
            op2.mem=_bp;\
            }\
          else {\
            theDs=_ds;\
            op2.mem=Pipe2.xm.w;\
            _ip+=2;\
            immofs+=2;\
            }\
          break;\
        case 7:\
          theDs=_ds; op2.mem=_bx;\
          break;\
        }\
      if(segOverride) {\
        theDs=&segs.r[segOverride-1];\
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


#ifdef EXT_80286
//#define PUSH_STACK(a) 	{ _sp-=2;	ASSIGN_SEGMENT(_ss,_ss->s.x); PutShortValue(_ss,_sp,(uint16_t)a); }
#define PUSH_STACK(a) 	{ _sp-=2;	PutShortValue(_ss,_sp,(uint16_t)a); }
#else
#define PUSH_STACK(a) 	{ _sp-=2;	PutShortValue(_ss,_sp,(uint16_t)a); }
#endif
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


extern HWND ghWnd,hStatusWnd;
extern HINSTANCE g_hinst;
extern HANDLE hAccelTable;
extern volatile struct tm currentDateTime;


ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Dump0(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Dump1(HWND, UINT, WPARAM, LPARAM);
int decodeKBD(int, long, BOOL);
int UpdateScreen(HDC,SWORD,SWORD);
extern const uint8_t CGAfont[];
extern const uint8_t MDAfont[];
extern const uint8_t VGAfont_8[],VGAfont_14[],VGAfont_16[];
#ifdef EXT_80386
uint8_t _fastcall GetPipe(struct REGISTERS_SEG *,uint32_t);
uint8_t _fastcall GetMorePipe(struct REGISTERS_SEG *,uint32_t);
uint8_t _fastcall GetValue(struct REGISTERS_SEG *,uint32_t);
uint16_t _fastcall GetShortValue(struct REGISTERS_SEG *,uint32_t);
uint32_t _fastcall GetIntValue(struct REGISTERS_SEG *,uint32_t);
uint8_t _fastcall InValue(uint16_t);
uint16_t _fastcall InShortValue(uint16_t);
uint16_t _fastcall InIntValue(uint16_t);
void _fastcall PutValue(struct REGISTERS_SEG *,uint32_t, uint8_t);
void _fastcall PutShortValue(struct REGISTERS_SEG *,uint32_t, uint16_t);
void _fastcall PutIntValue(struct REGISTERS_SEG *,uint32_t, uint32_t);
void _fastcall OutValue(uint16_t, uint8_t);
void _fastcall OutShortValue(uint16_t, uint16_t);
void _fastcall OutIntValue(uint32_t, uint16_t);
#else
uint8_t _fastcall GetPipe(struct REGISTERS_SEG *,uint16_t);
uint8_t _fastcall GetMorePipe(struct REGISTERS_SEG *,uint16_t);
uint8_t _fastcall GetValue(struct REGISTERS_SEG *,uint16_t);
uint16_t _fastcall GetShortValue(struct REGISTERS_SEG *,uint16_t);
uint8_t _fastcall InValue(uint16_t);
uint16_t _fastcall InShortValue(uint16_t);
void _fastcall PutValue(struct REGISTERS_SEG *,uint16_t, uint8_t);
void _fastcall PutShortValue(struct REGISTERS_SEG *,uint16_t, uint16_t);
void _fastcall OutValue(uint16_t, uint8_t);
void _fastcall OutShortValue(uint16_t, uint16_t);
#endif
int Emulate(int);
void initHW(void);
BOOL CenterWindow(HWND, HWND);
int WritePrivateProfileInt(char *,char *,int,char *);
BOOL PlayResource(LPSTR lpName,BOOL bStop);
int Disassemble(const BYTE *src,HFILE f,char *dest,DWORD len,WORD pcaddr,struct REGISTERS_SEG  *pcaddrH,BYTE opzioni);
BYTE to_bcd(BYTE n);
BYTE from_bcd(BYTE n);
char *strcpyhl(char *dest,const char *src);





/* turn a numeric literal into a hex constant
(avoids problems with leading zeroes)
8-bit constants max value 0b11111111, always fits in unsigned long
Tom Torfs
https://post.bytes.com/forum/topic/c/176274-binary-constant-macros
*/
#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */ /* OCCHIO a volte servono extra parentesi, specie in if() */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
| ((x&0x000000F0LU)?2:0) \
| ((x&0x00000F00LU)?4:0) \
| ((x&0x0000F000LU)?8:0) \
| ((x&0x000F0000LU)?16:0) \
| ((x&0x00F00000LU)?32:0) \
| ((x&0x0F000000LU)?64:0) \
| ((x&0xF0000000LU)?128:0)

/* *** user macros *** /

/* for upto 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))

/* for upto 16-bit binary constants, MSB first */
#define B16(dmsb,dlsb) (((unsigned short)B8(dmsb)<<8) \
| B8(dlsb))

/* for upto 32-bit binary constants, MSB first */
#define B32(dmsb,db2,db3,dlsb) (((unsigned long)B8(dmsb)<< 24) \
| ((unsigned long)B8(db2)<<16) \
| ((unsigned long)B8(db3)<<8) \
| B8(dlsb))

