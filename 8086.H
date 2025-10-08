//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#ifndef _8086_H_INCLUDED
#define _8086_H_INCLUDED

#include <stdint.h>
#include "8086_PIC.h"

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
union __attribute__((__packed__))  DWORD_BYTES {
	uint32_t d;
	struct __attribute__((__packed__)) {
		uint16_t l;
		uint16_t h;
		} w;
	uint8_t bd[4];
	struct __attribute__((__packed__)) {
		uint8_t l;
		uint8_t h;
		uint8_t u;
		uint8_t u2;
		} b;
	};
union __attribute__((__packed__))  REG_8086 {
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
union __attribute__((__packed__))  OPERAND {
  uint8_t *reg8;
  uint16_t *reg16;
#ifdef EXT_80386
  uint32_t *reg32;
  uint32_t mem;
#else
  uint16_t mem;
#endif
  };
union __attribute__((__packed__))  RESULT {
  uint8_t b;
  uint16_t x;
  uint32_t d;    // 
#ifdef EXT_80386
  uint16_t dd;
#endif
  };
#ifndef EXT_80386
	union __attribute__((__packed__))  REGISTERS {
		uint8_t b[16];
	  union __attribute__((__packed__))  REG_8086 r[8];
		};
#else
	union __attribute__((__packed__))  REGISTERS {
		uint8_t b[32];
	  union __attribute__((__packed__))  REG_8086 r[8];
		};
#endif
#ifndef EXT_80386
	union __attribute__((__packed__))  REGISTERS16 {
	  union __attribute__((__packed__))  REG_8086 r[4];
		};
#else
	union __attribute__((__packed__))  REGISTERS32 {
	  union __attribute__((__packed__))  REG_8086 r[6];
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
	union __attribute__((__packed__))  REGISTRO_F {
#ifdef EXT_80386
    uint32_t x;
#else
    uint16_t x;
#endif
		struct __attribute__((__packed__)) {
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
#define _CPL _cs->s.RPL
// RPL sono i bit RPL del segmento destinazione e DPL sono i bit nel descrittore (ACCESS) di codesto

	union __attribute__((__packed__))  SEGMENT_SELECTOR {		// 
    uint16_t x;			//real mode compat
		struct __attribute__((__packed__)) {
			unsigned short int RPL: 2;			// SERVE SHORT o va a 4 byte! e non char che si incasina Index... 
			unsigned short int TI: 1;
			unsigned short int Index: 13;
			};
		};
	union __attribute__((__packed__))  MACHINE_STATUS_WORD {		// 
    uint16_t x;
		struct __attribute__((__packed__)) {
			unsigned char PE: 1;
			unsigned char MP: 1;
			unsigned char EM: 1;
			unsigned char TS: 1;
			unsigned short unused: 12;
			};
		};
	struct __attribute__((__packed__)) SEGMENT_DESCRIPTOR {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union __attribute__((__packed__))  {
			uint8_t b;
			struct __attribute__((__packed__)) {
				unsigned char A: 1;
				unsigned char Type: 3;
				unsigned char System: 1;		// 1=segm descr; 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct __attribute__((__packed__)) SEGMENT_DESCRIPTOR_SYSTEM {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union __attribute__((__packed__))  {
			uint8_t b;
			struct __attribute__((__packed__)) {
				unsigned char Type: 4;
				unsigned char System: 1;		// 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct __attribute__((__packed__)) SEGMENT_DESCRIPTOR_GATE {
		unsigned short Offset: 16;
		unsigned short Selector: 16;
		unsigned char WordCount: 5;
		unsigned char unused: 3;
		union __attribute__((__packed__))  {
			uint8_t b;
			struct __attribute__((__packed__)) {
				unsigned char Type: 4;
				unsigned char System: 1;		// 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct __attribute__((__packed__)) SEGMENT_DESCRIPTOR_INTERRUPT_TRAP {
		unsigned short Offset: 16;
		unsigned short Selector: 16;
		unsigned char unused: 8;
		union __attribute__((__packed__))  {
			uint8_t b;
			struct __attribute__((__packed__)) {
				unsigned char Type: 5;		// 6=interrupt, 7 =gate
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct __attribute__((__packed__)) SEGMENT_DESCRIPTOR_TASK_GATE {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union __attribute__((__packed__))  {
			uint8_t b;
			struct __attribute__((__packed__)) {
				unsigned char A: 1;
				unsigned char Type: 3;			// 001
				unsigned char System: 1;		// 0=system/control segm descr
				unsigned char DPL: 2;
				unsigned char P: 1;
				};
			} Access;
		unsigned int reserved: 16;
		};
	struct __attribute__((__packed__)) SEGMENT_DESCRIPTOR_TSS {
		unsigned short Limit: 16;
		unsigned short Base: 16;
		unsigned char BaseH: 8;
		union __attribute__((__packed__))  {
			uint8_t b;
			struct __attribute__((__packed__)) {
				union __attribute__((__packed__))  {
					unsigned char Type:4;		// non va... si spostano in avanti i bit a seguire...
					struct __attribute__((__packed__)) {
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
	struct __attribute__((__packed__)) REGISTERS_SEG {
		union __attribute__((__packed__))  SEGMENT_SELECTOR s;
		struct __attribute__((__packed__)) SEGMENT_DESCRIPTOR d;
		};
	union __attribute__((__packed__))  _DTR {
    uint8_t b[6];
		struct __attribute__((__packed__)) {
			unsigned short Limit: 16;
//			unsigned long Base: 24;		// non ci piace, si sposta in avanti £$%@#  RIPROVARE ev. con short ecc
			unsigned short Base: 16;
			unsigned char BaseH: 8;
			unsigned char unused: 8;
			};
		};
	union __attribute__((__packed__))  INTERRUPT_WORD {
    uint16_t x;
		struct __attribute__((__packed__)) {
			unsigned char Type: 1;
			unsigned char Type2: 1;			// boh dice 1 fisso
			unsigned char unused0: 1;		// dice 0
			unsigned char Vector: 8;
			unsigned char unused: 5;
			};
		};
	struct __attribute__((__packed__)) TASK_STATE_REGISTER {
		uint16_t PTSS;			// previous TSS
		uint32_t SS_SP_CPL[3];		//(never altered) sp for CPL 0 (uso array per comodità di assegnazione
		uint16_t EntryPoint;	//ip (entry point)
		union __attribute__((__packed__))  REGISTRO_F Flags;			//flag: (I/O privilege level 
		uint16_t Ax,Bx,Cx,Dx;
		uint16_t Sp,Bp,Si,Di;
		uint16_t Es,Cs,Ss,Ds;
		uint16_t Ldt;				// task ldt selector
		};

#else
	struct __attribute__((__packed__)) REGISTERS_SEG {
		union __attribute__((__packed__))  {
			uint16_t x;		// real mode 
			} s;
		};
	union __attribute__((__packed__))  INTERRUPT_uint16_t {
    uint16_t x;
		struct __attribute__((__packed__)) {
			unsigned char Vector: 8;
			unsigned char unused: 8;
			};
		};
#endif
union __attribute__((__packed__))  SEGMENTS_DESCRIPTOR {
	struct __attribute__((__packed__)) REGISTERS_SEG r[4];
	};
#ifdef EXT_80386
	union __attribute__((__packed__))  CR0_MACHINE_STATUS_uint16_t {		// 
    uint16_t x;
		struct __attribute__((__packed__)) {
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
	union __attribute__((__packed__))  SEGMENT_DESCRIPTOR {		// https://stackoverflow.com/questions/30253678/x86-segment-descriptor-layout-why-is-it-weird
    uint64_t x;
		struct __attribute__((__packed__)) {
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
	union __attribute__((__packed__))  _DTR {
    uint8_t b[8];
		struct __attribute__((__packed__)) {
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
struct __attribute__((__packed__)) _EXCEPTION {
#ifdef EXT_80386
  uint32_t addr;
#else
  uint16_t addr;
#endif
  uint16_t addrH;
	uint32_t parm;
  union __attribute__((__packed__))  {
    uint16_t w;
    struct __attribute__((__packed__)) {
      unsigned char ud:8;
      unsigned char hascode:1;
      unsigned char in:1;
      unsigned char rw:1;
      };
    } descr;
  uint8_t active;
  };
#endif


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
		union __attribute__((__packed__))  REGISTRO_F _f1;\
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
		union __attribute__((__packed__))  REGISTRO_F _f1;\
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

// https://rafaeljin.gitlab.io/2021/02/03/segmentation/
// https://manybutfinite.com/post/cpu-rings-privilege-and-protection/
// https://xem.github.io/minix86/manual/intel-x86-and-64-manual-vol3/o_fe12b1e2a880e0ce-160.html
#if defined(EXT_80386)
#define ASSIGN_SEGMENT_CODE(seg,v) {\
	}
#define ASSIGN_SEGMENT_DATA(seg,v) {\
	}
#define ASSIGN_SEGMENT_STACK(seg,v) {\
	}
#elif defined(EXT_80286)
#define ASSIGN_SEGMENT_CODE(seg,v) {\
	union SEGMENT_SELECTOR __ss;\
	__ss.x=v;\
	if(_msw.PE) {\
		struct SEGMENT_DESCRIPTOR *sd;\
		if(__ss.x > GDTR.Limit) {/*+7*/\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		sd=(struct SEGMENT_DESCRIPTOR*)&ram_seg[\
			((!__ss.TI) ? MAKELONG(GDTR.Base,GDTR.BaseH) : \
			MAKELONG(LDTR->Base,LDTR->BaseH)) +\
			(__ss.Index << 3)];\
	/*se segmento dati NON accettare selector di tipo call interrupt ecc*/\
	/*mettere controlli CPL<DPL se dati, CPL==DPL se code (anche stack direi */\
	/*un selettore nullo è accettato e va a copiare i primi byte di GDTR che sono 0 quindi ok!*/\
		if(__ss.RPL > sd->Access.DPL   /*sd->Access.Type & 4  conforming gestire */)\
			goto exception286priv;\
		sd->Access.P=1;/*valid*/\
		(seg)->s.x=__ss.x;\
		memcpy(((uint8_t*)&(seg)->d),sd,sizeof(struct SEGMENT_DESCRIPTOR)-sizeof(uint16_t));\
		}\
	else\
		(seg)->s.x=__ss.x;\
	}		// sempre RAM?? o GetValue?
#define ASSIGN_SEGMENT_DATA(seg,v) {\
	union SEGMENT_SELECTOR __ss;\
	__ss.x=v;\
	if(_msw.PE) {\
		struct SEGMENT_DESCRIPTOR *sd;\
		if(__ss.x > GDTR.Limit) {/*+7*/\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		sd=(struct SEGMENT_DESCRIPTOR*)&ram_seg[\
			((!__ss.TI) ? MAKELONG(GDTR.Base,GDTR.BaseH) : \
			MAKELONG(LDTR->Base,LDTR->BaseH)) +\
			(__ss.Index << 3)];\
	/*se segmento dati NON accettare selector di tipo call interrupt ecc*/\
	/*mettere controlli CPL<DPL se dati, CPL==DPL se code (anche stack direi */\
	/*un selettore nullo è accettato e va a copiare i primi byte di GDTR che sono 0 quindi ok!*/\
		if(_CPL > sd->Access.DPL)\
			goto exception286priv;\
		sd->Access.P=1;/*valid*/\
		(seg)->s.x=__ss.x;\
		memcpy(((uint8_t*)&(seg)->d),sd,sizeof(struct SEGMENT_DESCRIPTOR)-sizeof(uint16_t));\
		}\
	else\
		(seg)->s.x=__ss.x;\
	}		// sempre RAM?? o GetValue?
#define ASSIGN_SEGMENT_STACK(seg,v) {\
	union SEGMENT_SELECTOR __ss;\
	__ss.x=v;\
	if(_msw.PE) {\
		struct SEGMENT_DESCRIPTOR *sd;\
		if(__ss.x > GDTR.Limit) {/*+7*/\
			Exception86.descr.ud=EXCEPTION_GP;\
			goto exception286;\
			}\
		sd=(struct SEGMENT_DESCRIPTOR*)&ram_seg[\
			((!__ss.TI) ? MAKELONG(GDTR.Base,GDTR.BaseH) : \
			MAKELONG(LDTR->Base,LDTR->BaseH)) +\
			(__ss.Index << 3)];\
	/*se segmento dati NON accettare selector di tipo call interrupt ecc*/\
	/*mettere controlli CPL<DPL se dati, CPL==DPL se code (anche stack direi */\
	/*un selettore nullo è accettato e va a copiare i primi byte di GDTR che sono 0 quindi ok!*/\
		if(_CPL > sd->Access.DPL)\
			goto exception286priv;\
		sd->Access.P=1;/*valid*/\
		(seg)->s.x=__ss.x;\
		memcpy(((uint8_t*)&(seg)->d),sd,sizeof(struct SEGMENT_DESCRIPTOR)-sizeof(uint16_t));\
		}\
	else\
		(seg)->s.x=__ss.x;\
	}		// sempre RAM?? o GetValue?
#else
#define ASSIGN_SEGMENT_CODE(seg,v) { (seg)->s.x=v; }
#define ASSIGN_SEGMENT_DATA(seg,v) { (seg)->s.x=v; }
#define ASSIGN_SEGMENT_STACK(seg,v) { (seg)->s.x=v; }
#endif

#define GET_GADDRESS_FROM_INDEX(v) (&ram_seg[MAKELONG(GDTR.Base,GDTR.BaseH)+(v)])
#define GET_LADDRESS_FROM_INDEX(v) (&ram_seg[MAKELONG(LDTR->Base,LDTR->BaseH)+(v)])

#define OVERRIDE_SEG(seg) {\
  if(segOverride) {\
    theDs=&segs.r[segOverride-1];\
    segOverride=0;\
    }\
  else\
    theDs=seg;\
	}
#define OVERRIDE_SEG_REP(seg) {\
  if(segOverride) {\
    theDs=&segs.r[segOverride-1];\
    if(!inRep)\
    	segOverride=0;\
    }\
  else\
    theDs=seg;\
	}

  
#endif

