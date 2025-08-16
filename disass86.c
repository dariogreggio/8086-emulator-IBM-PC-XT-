#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include "8086win.h"



HFILE spoolFile;

char commento[256];
BYTE Opzioni,CPUType;
BYTE sizeOverride=0,sizeOverrideA=0;

static const char *getRegister(BYTE Pipe1,BYTE reg) {
	const char *r;

	if(Pipe1 & 1) {
		if(CPUType>=3 || sizeOverride) {
			switch(reg & 7) {
				case 0:
					r="EAX";
					break;
				case 1:
					r="ECX";
					break;
				case 2:
					r="EDX";
					break;
				case 3:
					r="EBX";
					break;
				case 4:
					r="ESP";
					break;
				case 5:
					r="EBP";
					break;
				case 6:
					r="ESI";
					break;
				case 7:
					r="EDI";
					break;
				}
			}
		else {
			switch(reg & 7) {
				case 0:
					r="AX";
					break;
				case 1:
					r="CX";
					break;
				case 2:
					r="DX";
					break;
				case 3:
					r="BX";
					break;
				case 4:
					r="SP";
					break;
				case 5:
					r="BP";
					break;
				case 6:
					r="SI";
					break;
				case 7:
					r="DI";
					break;
				}
			}
		}
	else {
		switch(reg & 7) {
			case 0:
				r="AL";
				break;
			case 1:
				r="CL";
				break;
			case 2:
				r="DL";
				break;
			case 3:
				r="BL";
				break;
			case 4:
				r="AH";
				break;
			case 5:
				r="CH";
				break;
			case 6:
				r="DH";
				break;
			case 7:
				r="BH";
				break;
			}
		}

	return r;
	}

static const char *getSRegister(BYTE s) {
	const char *r;

	switch(s) {
		case 0:
			r="ES";
			break;
		case 1:
			r="CS";
			break;
		case 2:
			r="SS";
			break;
		case 3:
			r="DS";
			break;
		case 4:
			if(CPUType>=3) {
				r="FS";
				}
			else 
				r="unknown";
			break;
		case 5:
			if(CPUType>=3) {
				r="GS";
				}
			else 
				r="unknown";
			break;
		default:
			r="unknown";
			break;
		}

	return r;
	}

static const char *getRegister2(BYTE Pipe1,BYTE rm) {			// unire!!

	return getRegister(Pipe1,rm);
	}

static const char *getAddressing(union PIPE Pipe2,const BYTE **ip,BYTE *immofs) {
	const char *r;
	static    char myBuf[32];		// migliorare
	char myBuf2[32];	

	// è il caso di mettere BYTE PTR e WORD PTR qua?? togliendolo in giro

	myBuf[0]='[';  myBuf[1]=0;
  switch(Pipe2.rm) {
    case 0:
			r="BX+SI";
      break;
    case 1:
			r="BX+DI";
      break;
    case 2:
			r="BP+SI";
      break;
    case 3:
			r="BP+DI";
      break;
    case 4:
			r="SI";
      break;
    case 5:
			r="DI";
      break;
    case 6:
      if(Pipe2.mod) {
				r="BP";
        }
      else {
				wsprintf(myBuf+1,"%04Xh",Pipe2.xm.w);
				if(Opzioni & 4) {
					wsprintf(myBuf2,"%d",Pipe2.xm.w);
					if(*commento)
						strcat(commento," ");
					strcat(commento,myBuf2);
					}
        *ip+=2;
        *immofs+=2;
				r="";
        }
      break;
    case 7:
			r="BX";
      break;
    }
	strcat(myBuf,r);
  switch(Pipe2.mod) {
		case 0:
			break;
		case 1:		// mettere solo se !=0?
			if((int8_t)Pipe2.b.h<0) {
				wsprintf(myBuf2,"-%04Xh",-(int16_t)(int8_t)Pipe2.b.h);		// gestire segno
				}
			else {
				wsprintf(myBuf2,"+%04Xh",(int16_t)(int8_t)Pipe2.b.h);		// 
				}
			strcat(myBuf,myBuf2);
			if(Opzioni & 4) {
				if((int8_t)Pipe2.b.h<0) {
					wsprintf(myBuf2,"%d",-(int16_t)(int8_t)Pipe2.b.h);
					}
				else {
					wsprintf(myBuf2,"%d",(int16_t)(int8_t)Pipe2.b.h);
					}
				if(*commento)
					strcat(commento," ");
				strcat(commento,myBuf2);
				}
			*immofs+=1;
			break;
		case 2:		// mettere solo se !=0?
			if((int16_t)Pipe2.xm.w<0) {
				wsprintf(myBuf2,"-%04Xh",-(int16_t)Pipe2.xm.w);	
				}
			else {
				wsprintf(myBuf2,"+%04Xh",(int16_t)Pipe2.xm.w);	
				}
			strcat(myBuf,myBuf2);
			if(Opzioni & 4) {
				if((int16_t)Pipe2.xm.w<0) {
					wsprintf(myBuf2,"%d",(int16_t)Pipe2.xm.w);
					}
				else {
					wsprintf(myBuf2,"%d",(int16_t)Pipe2.xm.w);
					}
				if(*commento)
					strcat(commento," ");
				strcat(commento,myBuf2);
				}
			*immofs+=2;
			break;
		case 3:/*gestito a parte*/
			break;
		}

	strcat(myBuf,"]");
	r=myBuf;

	return r;
	}

static char *outputText(char *d,const char *s) {

	strcat(d,s);
	return d;
	}
	
static char *outputPtr(char *d,BYTE t) {

	switch(t) {
		case 1:
			strcat(d,"BYTE PTR ");
			break;
		case 2:
			strcat(d,"WORD PTR ");
			break;
		case 4:
			strcat(d,"DWORD PTR ");
			break;
		case 8:
			strcat(d,"QWORD PTR ");
			break;
		default:
			strcat(d,"UNKNOWN PTR");
			break;
		}
	return d;
	}
	
static char *outputByte(char *d,BYTE n) {
	char myBuf[64];

	wsprintf(myBuf,"%02Xh",n);
	strcat(d,myBuf);
	if(Opzioni & 4) {
		wsprintf(myBuf,"%d",n);
		if(*commento)
			strcat(commento," ");
		strcat(commento,myBuf);
		}

	return d;
	}

static char *outputWord(char *d,WORD n) {
	char myBuf[64];

	wsprintf(myBuf,"%04Xh",n);
	strcat(d,myBuf);
	if(Opzioni & 4) {
		wsprintf(myBuf,"%d",n);
		if(*commento)
			strcat(commento," ");
		strcat(commento,myBuf);
		}
	return d;
	}

static char *outputDWord(char *d,DWORD n) {
	char myBuf[64];

	wsprintf(myBuf,"%08X",n);
	strcat(d,myBuf);
	if(Opzioni & 4) {
		wsprintf(myBuf,"%d",n);
		if(*commento)
			strcat(commento," ");
		strcat(commento,myBuf);
		}
	return d;
	}

static char *outputAddress(char *d,WORD n) {
	char myBuf[64];

	wsprintf(myBuf,"%04Xh",n);
	strcat(d,myBuf);
	return d;
	}

static char *outputFarAddress(char *d,DWORD n) {
	char myBuf[64];

	wsprintf(myBuf,"%04X:%04Xh",HIWORD(n),LOWORD(n));
	strcat(d,myBuf);
	return d;
	}

static char *outputUnknown(char *d,BYTE n) {

	outputText(d,"DB ");
	outputByte(d,n);
	return d;
	}

int Disassemble(const BYTE *src,char *dest,BYTE len,WORD pcaddr,BYTE opzioni) {
	// opzioni: b0 addbytes, b1 addPC, b2 numeri dec in commenti
  uint8_t inRep=0;
	uint8_t inRepStep=0;
	BYTE segOverride,segOverrideIRQ;
	BYTE status8087,control8087;
  register union OPERAND op1,op2;
  BYTE immofs;		// usato per dato #imm che segue indirizzamento 0..2 (NON registro)
		// è collegato a GetMorePipe, inoltre
	BYTE Pipe1;
	union PIPE Pipe2;
	const BYTE *oldsrc;
	const char *r,*r2;
  register uint16_t i;
	BYTE fExit=0;


	Opzioni=opzioni;
	CPUType=0;

	*dest=0;



	do {

		oldsrc=src;

		*commento=0;

// http://www.mlsite.net/8086/  per hex-codes
		Pipe1=*src++;
		Pipe2.bd[0]=*src; Pipe2.bd[1]=*(src+1);		// mah vedere se farlo di volta in volta nelle istruzioni lunghe...
		Pipe2.bd[2]=*(src+2); Pipe2.bd[3]=*(src+3);
		Pipe2.bd[4]=*(src+4); Pipe2.bd[5]=*(src+5);

		immofs=0;
		segOverride=0; segOverrideIRQ=0;
		sizeOverride=0; sizeOverrideA=0;
		status8087=0; control8087=0;

		switch(Pipe1) {

			case 0:     // ADD registro a r/m
			case 1:
			case 2:     // ADD r/m a registro
			case 3:
				outputText(dest,"ADD ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
            switch(Pipe1 & 0x3) {
              case 0:
								outputText(dest,r);
                break;
              case 1:
								if(CPUType>=3) {
									sizeOverrideA;
									}
								outputText(dest,r);
                break;
              case 2:
								outputText(dest,r);
                break;
              case 3:
								if(CPUType>=3) {

									sizeOverrideA;
								}
								outputText(dest,r);
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
            switch(Pipe1 & 0x3) {
              case 0:
								outputText(dest,r);
                break;
              case 1:
								if(CPUType>=3) {
									sizeOverrideA;
								}
								outputText(dest,r);
                break;
              case 2:
								outputText(dest,r);
                break;
              case 3:
								if(CPUType>=3) {
									sizeOverrideA;
								}
								outputText(dest,r);
                break;
              }
            break;
          }
				src++;
				break;
        
			case 4:       // ADDB immediate
				outputText(dest,"ADDB AL,");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 5:       // ADDW immediate
				outputText(dest,"ADDW AX,");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;
        
			case 6:       // PUSH segment
			case 0xe:
			case 0x16:
			case 0x1e:
				outputText(dest,"PUSH ");
				outputText(dest,getSRegister((Pipe1 >> 3) & 3));
				break;
        
			case 0xf:       // non esiste POP CS (abbastanza logico); istruzioni MMX/386/(V20) qua...
				outputUnknown(dest,Pipe2.b.l);

				src++;
        
        switch(Pipe2.b.l) {

#ifndef EXT_NECV20
          case 0xff:      // TRAP
						outputText(dest,"TRAP/NEC ");
            break;
#endif
            

#ifdef EXT_NECV20
					case 0x10:						// TEST1, CLR1, SET1, NOT1 , cl
					case 0x11:// (OCCHIO pare che SET1 possa andare anche su Carry o su DIR... verificare, anche le altre!
					case 0x12:// ah no che frociata, sono le altre istruzioni, STC STD porcamadonna gli umani col cancro!!
					case 0x13:
					case 0x14:
					case 0x15:
					case 0x16:
					case 0x17:
						// servirà //        GetMorePipe(_cs,_ip-1));
				r=getRegister(Pipe1,Pipe2);
								if(!(Pipe1 & 1)) {
									res1.b=GetValue(*theDs,op2.mem);
									}
								else {
									res1.x=GetShortValue(*theDs,op2.mem);
									}
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (_cl & 0x7));
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (_cl & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (_cl & 0xf));
											}
										break;
									case 2:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (_cl & 0xf));
											}
										break;
									case 3:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (_cl & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
									PutValue(*theDs,op2.mem,res3.b);
									}
								else {
									PutShortValue(*theDs,op2.mem,res3.x);
									}
								break;
							case 3:
								r=getRegister2(Pipe1,Pipe2);
								res1.b=*op2.reg8;
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (_cl & 0x7));
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (_cl & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (_cl & 0xf));
											}
										break;
									case 2:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (_cl & 0xf));
											}
										break;
									case 3:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (_cl & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (_cl & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
	                *op2.reg8=res3.b;
									}
								else {
	                *op2.reg16=res3.x;
									}
								break;
							}
            break;
					case 0x18:						// TEST1, CLR1, SET1, NOT1 , #imm
					case 0x19:
					case 0x1a:
					case 0x1b:
					case 0x1c:
					case 0x1d:
					case 0x1e:
					case 0x1f:
						// servirà //        GetMorePipe(_cs,_ip-1));

				r=getRegister(Pipe1,Pipe2);
								if(!(Pipe1 & 1)) {
									res1.b=GetValue(*theDs,op2.mem);
									}
								else {
									res1.x=GetShortValue(*theDs,op2.mem);
									}
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										outputText(dest,"TEST1 ");
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (Pipe2.b.u & 0x7));			// VERIFICARE DOV'è IMM8!!
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (Pipe2.b.u & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										outputText(dest,"CLR1 ");
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 2:
										outputText(dest,"SET1 ");
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 3:
										outputText(dest,"NOT1 ");
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (Pipe2.b.u & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
									PutValue(*theDs,op2.mem,res3.b);
									}
								else {
									PutShortValue(*theDs,op2.mem,res3.x);
									}
								break;
							case 3:
								r=getRegister2(Pipe1,Pipe2);
								res1.b=*op2.reg8;
								switch((Pipe1 >> 1) & 0x3) {
									case 0:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & (1 << (Pipe2.b.u & 0x7));
											goto aggFlagBZC;
											}
										else {
											res3.x= res1.x & (1 << (Pipe2.b.u & 0xf));
											goto aggFlagWZC;
											}
										break;
									case 1:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b & ~(1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x & ~(1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 2:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b | (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x | (1 << (Pipe2.b.u & 0xf));
											}
										break;
									case 3:
										if(!(Pipe1 & 1)) {
											res3.b= res1.b ^ (1 << (Pipe2.b.u & 0x7));
											}
										else {
											res3.x= res1.x ^ (1 << (Pipe2.b.u & 0xf));
											}
										break;
									}
								if(!(Pipe1 & 1)) {
	                *op2.reg8=res3.b;
									}
								else {
	                *op2.reg16=res3.x;
									}
								break;
							}
            break;
					case 0x20:					// ADD4S
						outputText(dest,"ADD4S ");
						{BYTE j=0;
						uint16_t di1=_di,si1=_si;
						i=(((uint16_t)_cl)+1) >> 1;
						_f.Zero=1;
						while(i--) {
							res1.b=GetValue(*theDs,si1);
							res2.b=GetValue(_es,di1);
              res1.b = (res1.b >> 4)*10 + (res1.b & 0xf);
              res2.b = (res2.b >> 4)*10 + (res2.b & 0xf);
              res3.b = res1.b + res2.b + j;
              if(res3.b>99) 
								j=1; 
							else 
								j=0;
              res3.b = res3.b % 100;
              res3.b = ((res3.b/10) << 4) | (res3.b % 10);

							if(j)
								_f.Carry=1;
							if(res3.b)
								_f.Zero=0;

							PutValue(_es,_di,res3.b);
							si1++;
							di1++;
							}
						}
            break;
					case 0x22:					// SUB4S
						outputText(dest,"SUB4S ");
						{BYTE j=0;
						uint16_t di1=_di,si1=_si;
						i=(((uint16_t)_cl)+1) >> 1;
						_f.Zero=1;
						while(i--) {
							res1.b=GetValue(*theDs,si1);
							res2.b=GetValue(_es,di1);
              res1.b = (res1.b >> 4)*10 + (res1.b & 0xf);
              res2.b = (res2.b >> 4)*10 + (res2.b & 0xf);
              if(res1.b < (res2.b+j)) {
                res1.b = res1.b + 100;
                res3.b = res1.b - (res2.b+j);
                i= 1;
	              }
              else  {
                res3.b = res1.b - (res2.b+j);
                i= 0;
								}
              res3.b = ((res3.b/10)<<4) | (res3.b % 10);

							if(j)
								_f.Carry=1;
							if(res3.b)
								_f.Zero=0;

							PutValue(_es,_di,res3.b);
							si1++;
							di1++;
							}
						}
            break;
					case 0x26:					// CMP4S
						outputText(dest,"CMP4S ");
						{BYTE j=0;
						uint16_t di1=_di,si1=_si;
						i=(((uint16_t)_cl)+1) >> 1;
						_f.Zero=1;
						while(i--) {
							res1.b=GetValue(*theDs,si1);
							res2.b=GetValue(_es,di1);
              res1.b = (res1.b >> 4)*10 + (res1.b & 0xf);
              res2.b = (res2.b >> 4)*10 + (res2.b & 0xf);
              if(res1.b < (res2.b+j)) {
                res1.b = res1.b + 100;
                res3.b = res1.b - (res2.b+j);
                i= 1;
	              }
              else  {
                res3.b = res1.b - (res2.b+j);
                i= 0;
								}
              res3.b = ((res3.b/10)<<4) | (res3.b % 10);

							if(j)
								_f.Carry=1;
							if(res3.b)
								_f.Zero=0;

							si1++;
							di1++;
							}
						}
            break;
					case 0x28:					// ROL4
					case 0x2a:					// ROR4
  					_ip++;
				r=getRegister(Pipe1,Pipe2);
								res1.b=GetValue(*theDs,op2.mem);
								res2.b=*op1.reg8;
								if(Pipe2.b.l == 0x28) {
                  res2.b = (_al << 4) | ((res1.b >> 4) & 0x000F) ;
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=res2.b;
									}
								else {
                  res2.b = (res1.b >> 4) | ((_al << 4) & 0x00F0);
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=(_al & 0xFF00) | (res2.b & 0x00FF);
									}
								PutValue(*theDs,op2.mem,res3.b);
							break;
						case 3:
							r=getRegister2(Pipe1,Pipe2);
								res1.b=*op2.reg8;
								res2.b=*op1.reg8;
								if(Pipe2.b.l == 0x28) {
                  res2.b = (_al << 4) | ((res1.b >> 4) & 0x000F);
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=res2.b;
									}
								else {
                  res2.b = (_al << 4) | ((res1.b >> 4) & 0x000F);
                  res3.b=( (res1.b << 4) | (_al & 0x000F) ); 
                  _al=res2.b;
									}
                *op2.reg8=res3.b;
								break;
							}
            break;
					case 0x31:					// INS
						outputText(dest,"INS ");
						{BYTE j1,j2;
						if(!theDs)
							theDs=&_es;
						res1.b=GetValue(*theDs,_di);
						res3.x=_ax;

						PutShortValue(*theDs,_di,res3.x);
						}
            break;
					case 0x39:					// INS
						outputText(dest,"INS ");
						{BYTE j1,j2;
						if(!theDs)
							theDs=&_es;
						j2=Pipe2.b.u & 0xf;			// VERIFICARE DOV'è IMM8!!
						res3.x=_ax;

						PutShortValue(*theDs,_di,res3.x);
						}
            break;
					case 0x33:					// EXT
						outputText(dest,"EXT ");
						{BYTE j1,j2;
						res1.x=GetShortValue(*theDs,_si);

						_ax=res3.x;
						}
            break;
					case 0x3b:					// EXT
						outputText(dest,"EXT ");
						{BYTE j1,j2;
						j2=Pipe2.b.u & 0xf;			// VERIFICARE DOV'è IMM8!!
						res1.x=GetShortValue(*theDs,_si);

						_ax=res3.x;
						}
            break;
					case 0xff:					// BRKMM
						outputText(dest,"BRKMM ");
						// entra in modalità 8080!!
						_f.MD=0;
            break;
#endif

#ifdef EXT_80286
					if(CPUType>=2) {
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
					if(CPUType>=3) {
          case 0x5:      // SYSCALL
            break;
          case 0x31:      // RDTSC; si potrebbe mettere anche in 8086??!
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
            _eax= GenuineIntel  GDIntel2025
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
      case 0x0f:				//LSS
        /* deve seguire B2.. verificare
#define WORKING_REG regs.r[(Pipe2.reg].x      // CONTROLLARE
				i=GetShortValue(Pipe2.xm.x);
				WORKING_REG=GetShortValue(i);
        i+=2;
				_ss=GetShortValue(i);
        _ip+=2;
        */
					if(CPUType>=3) {
        			// e se segue b4 b5 è fs o gs!
#define WORKING_REG regs.r[(Pipe2.reg].x      // CONTROLLARE
				i=GetShortValue(Pipe2.xm.w);
				WORKING_REG=GetShortValue(i);
        i+=2;
				_fs=GetShortValue(i);
        _ip+=2;
					}
#endif
#ifdef EXT_80486
					if(CPUType>=4) {
          case 0x1:      // INVLPG
            break;
          case 0x8:      // INVD
            break;
          case 0x9:      // WBINVD
            break;
          case 0x1f:      // NOP...
            break;
          case 0x34:      // SYSENTER
            break;
          case 0xa6:      // CMPXCHG/CMPXCHG8B
          case 0xa7:
          case 0xb0:
          case 0xb1:
          case 0xc7:
            break;
          case 0xc0:      // XADD
          case 0xc1:      // XADD
            break;
          case 0x80:      // Jcc near
          case 0x81:
          case 0x82:
          case 0x83:
          case 0x84:
          case 0x85:
          case 0x86:
          case 0x87:
          case 0x88:
          case 0x89:
          case 0x8a:
          case 0x8b:
          case 0x8c:
          case 0x8d:
          case 0x8e:
          case 0x8f:
            break;
          case 0x90:      // SETcc
          case 0x91:
          case 0x92:
          case 0x93:
          case 0x94:
          case 0x95:
          case 0x96:
          case 0x97:
          case 0x98:
          case 0x99:
          case 0x9a:
          case 0x9b:
          case 0x9c:
          case 0x9d:
          case 0x9e:
          case 0x9f:
            break;
#endif
          }
				break;
        
			case 7:         // POP segment
			case 0x17:
			case 0x1f:
				outputText(dest,"POP ");
				outputText(dest,getSRegister((Pipe1 >> 3) & 3));
				break;

			case 0x08:      // OR
			case 0x09:
			case 0x0a:
			case 0x0b:
				outputText(dest,"OR ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          }
				src++;
				break;

			case 0x0c:      // OR
				outputText(dest,"OR AL,");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0x0d:      // OR
				outputText(dest,"OR AX,");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;

			case 0x10:     // ADC registro a r/m
			case 0x11:
			case 0x12:     // ADC r/m a registro
			case 0x13:
				outputText(dest,"ADC ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          }
				src++;
				break;

			case 0x14:        // ADC
				outputText(dest,"ADC ");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0x15:        // ADC
				outputText(dest,"ADC ");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;

			case 0x18:        // SBB
			case 0x19:
			case 0x1a:
			case 0x1b:
				outputText(dest,"SBB ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          }
				src++;
				break;

			case 0x1c:        // SBB
				outputText(dest,"SBB ");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0x1d:        // SBB
				outputText(dest,"SBB ");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;

			case 0x20:        // AND
			case 0x21:
			case 0x22:
			case 0x23:
				outputText(dest,"AND ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          }
				src++;
				break;

			case 0x24:        // AND
				outputText(dest,"AND ");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0x25:        // AND
				outputText(dest,"AND ");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;

			case 0x26:
				outputText(dest,"ES");
				segOverride=0+1;			// ES
				break;

			case 0x27:				// DAA
				outputText(dest,"DAA");
        break;
        
			case 0x28:      // SUB
			case 0x29:
			case 0x2a:
			case 0x2b:
				outputText(dest,"SUB ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          }
				src++;
				break;

			case 0x2c:      // SUBB
				outputText(dest,"SUBB ");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0x2d:			// SUBW
				outputText(dest,"SUBW ");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;

			case 0x2e:
				outputText(dest,"CS");
				segOverride=1+1;			// CS
				break;

			case 0x2f:				// DAS
				outputText(dest,"DAS");
				break;
        
			case 0x30:        // XOR
			case 0x31:
			case 0x32:
			case 0x33:
				outputText(dest,"XOR ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          }
				src++;
        break;
        
			case 0x34:        // XOR
				outputText(dest,"XOR ");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0x35:        // XOR
				outputText(dest,"XOR ");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;

			case 0x36:
				outputText(dest,"SS");
        segOverride=2+1;			// SS
				break;

			case 0x37:				// AAA
				outputText(dest,"AAA");
				break;

			case 0x38:      // CMP
			case 0x39:
			case 0x3a:
			case 0x3b:
				outputText(dest,"CMP ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            switch(Pipe1 & 0x3) {
              case 0:
                break;
              case 1:
                break;
              case 2:
                break;
              case 3:
                break;
              }
            break;
          }
				src++;
				break;
        
			case 0x3c:      // CMPB
				outputText(dest,"CMPB ");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0x3d:			// CMPW
				outputText(dest,"CMPW ");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;

			case 0x3e:
				outputText(dest,"DS");
        segOverride=3+1;			// DS
				break;

 			case 0x3f:      // AAS
				outputText(dest,"AAS");
				break;
        
			case 0x40:      // INC
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
				outputText(dest,"INC ");
				r=getRegister(1,Pipe1 & 7);
				outputText(dest,r);
				break;

			case 0x48:      // DEC
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4e:
			case 0x4f:
				outputText(dest,"DEC ");
				r=getRegister(1,Pipe1 & 7);
				outputText(dest,r);
				break;

			case 0x50:		// PUSH
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x55:
			case 0x56:
			case 0x57:
			case 0x54:
				outputText(dest,"PUSH ");
				r=getRegister(1,Pipe1);
				outputText(dest,r);
				break;

			case 0x58:		// POP
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x5d:
			case 0x5e:
			case 0x5f:
			case 0x5c:
				outputText(dest,"POP ");
				r=getRegister(1,Pipe1);
				outputText(dest,r);
				break;

#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0x60:      // PUSHA
				outputText(dest,"PUSHA");
				break;
			case 0x61:      // POPA
				outputText(dest,"POPA");
				break;
			case 0x62:        // BOUND
				outputText(dest,"BOUND ");
#define WORKING_REG regs.r[Pipe2.reg].x
//        GetMorePipe(_cs,_ip-1));
        src+=3;
				outputWord(dest,Pipe2.x.l);
				outputText(dest,",");
				outputWord(dest,Pipe2.x.h);
				break;
#endif
#ifdef EXT_80286
			case 0x63:        // ARPL
				outputText(dest,"ARPL ");
				break;
#endif
#ifdef EXT_NECV20
			case 0x63:        // undefined, dice, su V20...
				r=getRegister(Pipe1,Pipe2);
				break;
#endif
        
			case 0x64:
					if(CPUType>=3) {
				outputText(dest,"FS");
				segOverride=4+1;			// FS
					}
					else {      // JE, JZ
#ifdef UNDOCUMENTED_8086
				outputText(dest,"JZ ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
#endif
					}
				break;

			case 0x65:
					if(CPUType>=3) {
				outputText(dest,"GS");
				segOverride=5+1;			// GS
				}
					else {      // JNE, JNZ
#ifdef UNDOCUMENTED_8086
				outputText(dest,"JNZ ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
#endif
					}
				break;
#ifdef EXT_NECV20
			case 0x64:
				outputText(dest,"REPNC");
				inRep=0x15;					// REPNC
				break;
        
			case 0x65:
				outputText(dest,"REPC");
				inRep=0x16;					// REPC
				break;
#endif
        
			case 0x66:
					if(CPUType>=3) {
				//invert operand size next instr
				outputText(dest,"/16");		// finire!
        sizeOverride=2;
        
     		switch(*src) {
          case 0x99:    // anche CDQ
//            _edx= _eax & 0x80000000 ? 0xffffffff : 0;
    				break;
          case 0x98:    // anche CWDE
//            _eax |= _ax & 0x8000 ? 0xffff0000 : 0;
          	break;
          }
					}
					else {    // JBE, JNA
#ifdef UNDOCUMENTED_8086
				outputText(dest,"JNA ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
#endif
					}
        
				break;
			case 0x67:
					if(CPUType>=3) {
				outputText(dest,"/32");
        sizeOverrideA=2;
        
     		switch(*src) {
          case 0x63:      // JCXZ
            break;
          }
				//invert address size next instr
					}
					else {      // JA, JNBE
#ifdef UNDOCUMENTED_8086
				outputText(dest,"JA ");
				src++;
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
#endif
					}
				break;

#ifdef EXT_NECV20
			case 0x66:		// altre ESC per coprocessore NEC...
			case 0x67:
				outputText(dest,"ESC_NEC ");
				r=getRegister(Pipe1,Pipe2);
				break;
#endif
        
#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0x68:
					if(CPUType>=1) {
				outputText(dest,"PUSH ");
				PUSH_STACK(Pipe2.x.l);
        src+=2;
					}
				break;
        
			case 0x69:			//IMUL16
					if(CPUType>=1) {
			  union OPERAND op3;

				outputText(dest,"IMUL16 ");
        op1.reg16 = &regs.r[(Pipe2.b.l >> 3) & 0x7].x;		// 3 o 7??

				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
						GetPipe(_cs,_ip);
            op3.mem=Pipe2.x.l;
						outputWord(Pipe2.x.l);
            res1.x=GetShortValue(*theDs,op2.mem);
            res2.x=op3.mem;
            res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;   // (sicuro 32? per i flag
#ifdef EXT_80386
            *op1.reg32=res3.d;		// finire!
#else
            *op1.reg16=res3.x;
#endif
            break;
          case 3:
						r=getRegister2(Pipe2);
						//GET_REGISTER_8_16_2
						outputText(dest,r);
            op2.reg16= &regs.r[Pipe2.b.l & 0x7].x;			// 3 o 7 ??
						GetMorePipe(_cs,_ip); 
            op3.mem=Pipe2.xm.w;
						outputWord(Pipe2.xm.w);
            res1.x=*op2.reg16;
            res2.x=op3.mem;
            res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;   // (sicuro 32? per i flag
					if(CPUType>=3) {
            *op1.reg32=res3.d;		// finire!
					}
					else {
            *op1.reg16=res3.x;
					}
            break;
          }
 				_ip+=2;      // imm16
				}
				
				break;

			case 0x6a:        // PUSH imm8
				outputText(dest,"PUSH ");
				PUSH_STACK((int16_t)(int8_t)Pipe2.b.l);
        _ip++;
        break;

			case 0x6b:			//IMUL8
				{
			  union OPERAND op3;

				outputText(dest,"IMUL8 ");
        op1.reg16 = &regs.r[(Pipe2.b.l >> 3) & 0x7].x;		// 3 o 7??
       
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
						GetPipe(_cs,_ip);
            op3.mem=(int16_t)(int8_t)Pipe2.b.l;
						outputByte(Pipe2.b.l);
            res1.x=GetShortValue(*theDs,op2.mem);
            res2.x=op3.mem;
						res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;
					if(CPUType>=3) {
            *op1.reg32=res3.d;		// finire!
					}
						else {
            *op1.reg16=res3.x;
						}
            break;
          case 3:
						r=getRegister2(Pipe2);
						outputText(dest,r);
						//GET_REGISTER_8_16_2
            op2.reg16= &regs.r[Pipe2.b.l & 0x7].x;			// 3 o 7 ??
						GetMorePipe(_cs,_ip); 
            op3.mem=(int16_t)(int8_t)Pipe2.b.h;
						outputByte(Pipe2.b.h);
            res1.x=*op2.reg16;
            res2.x=op3.mem;
						res3.d=(int32_t)((int16_t)res1.x)*(int16_t)res2.x;
					if(CPUType>=3) {
            *op1.reg32=res3.d;		// finire!
					}
					else {
            *op1.reg16=res3.x;
					}
            break;
          }
    		_ip++;      // imm8
				}
				
				break;

			case 0x6c:        // INSB; NO OVERRIDE qua  https://www.felixcloutier.com/x86/ins:insb:insw:insd
				outputText(dest,"INSB ");
        if(inRep) {   // .. ma prefisso viene accettato cmq...
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
				PutValue(_es,_di,InValue(_dx));
        if(_f.Dir)
          _di--;
        else
          _di++;
				break;

			case 0x6d:        // INSW
				outputText(dest,"INSW ");
        if(inRep) {   // .. ma prefisso viene accettato cmq...
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
					if(CPUType>=3) {
					}
				PutShortValue(_es,_di,InShortValue(_dx));
        if(_f.Dir) {
          _di-=2;   // anche 32bit??
          }
        else {
          _di+=2;
          }
				break;

			case 0x6e:        // OUTSB
				outputText(dest,"OUTSB ");
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
				OutValue(_dx,GetValue(_es,_di));
				break;

			case 0x6f:        // OUTSW
				outputText(dest,"OUTSW ");
        if(inRep) {
          inRep=3;
          inRepStep=segOverride ? 2 : 1;
          }
					if(CPUType>=3) {
					}
				OutShortValue(_dx,GetShortValue(_es,_di));
				break;
#endif			// 186, V20

#ifdef UNDOCUMENTED_8086				// su 8086 questi sono alias per 0x70..7f  (!)
			case 0x60:    // JO
				outputText(dest,"JO ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x61:    // JNO
				outputText(dest,"JNO ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x62:    // JB, JC, JNAE
				outputText(dest,"JC ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x63:    // JAE, JNB, JNC
				outputText(dest,"JNC ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x68:          // JS
				outputText(dest,"JS ");
				src++;
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				break;

			case 0x69:          // JNS
				outputText(dest,"JNS ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

      case 0x6a:          // JP, JPE
				outputText(dest,"JP ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;
        
			case 0x6b:          // JNP, JPO
				outputText(dest,"JNP ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x6c:          // JL, JNGE
				outputText(dest,"JL ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x6d:
				outputText(dest,"JGE ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x6e:      // JLE, JNG
				outputText(dest,"JLE ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x6f:      // JG, JNLE
				outputText(dest,"JG ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;
#endif

			case 0x70:    // JO
				outputText(dest,"JO ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x71:    // JNO
				outputText(dest,"JNO ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x72:    // JB, JC, JNAE
				outputText(dest,"JC ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x73:    // JAE, JNB, JNC
				outputText(dest,"JNC ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x74:      // JE, JZ
				outputText(dest,"JZ ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x75:      // JNE, JNZ
				outputText(dest,"JNZ ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x76:    // JBE, JNA
				outputText(dest,"JNA ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x77:      // JA, JNBE
				outputText(dest,"JA ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x78:          // JS
				outputText(dest,"JS ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x79:          // JNS
				outputText(dest,"JNS ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

      case 0x7a:          // JP, JPE
				outputText(dest,"JP ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;
        
			case 0x7b:          // JNP, JPO
				outputText(dest,"JNP ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x7c:          // JL, JNGE
				outputText(dest,"JL ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x7d:
				outputText(dest,"JGE ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x7e:      // JLE, JNG
				outputText(dest,"JLE ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x7f:      // JG, JNLE
				outputText(dest,"JG ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;

			case 0x80:				// ADD ecc rm8, immediate8
			case 0x81:				// ADD ecc rm16, immediate16
			case 0x82:				// undefined/nop... (ADD ecc rm8, immediate8  con sign-extend   LO USA EDLIN!@#£$%
			case 0x83:				// ADD ecc rm16, immediate8 con sign-extend
				if(!(Pipe1 & 2)) { 			// vuol dire che l'operando è 8bit ma esteso a 16
          src++;
					}
				if(Pipe1 == 0x83) {
//					immofs++;
					}
        
				switch(Pipe2.mod) {
					case 2:
						/*Pipe2.b.h=**/src++;
					case 1:
						/*Pipe2.b.u=**/src++;
					case 0:
						immofs++;
						r=getAddressing(Pipe2,&src,&immofs);
					if(!(Pipe1 & 1)) {
            op1.mem=Pipe2.bd[immofs];
						switch(Pipe2.reg) {
			        case 0:       // ADD
								outputText(dest,"ADD ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							case 1:       // OR
								outputText(dest,"OR ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							case 2:       // ADC
								outputText(dest,"ADC ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							case 3:       // SBB
								outputText(dest,"SBB ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							case 4:       // AND
								outputText(dest,"AND ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							case 5:       // SUB
								outputText(dest,"SUB ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							case 6:       // XOR
								outputText(dest,"XOR ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							case 7:       // CMP
								outputText(dest,"CMP ");
								outputPtr(dest,1);
								outputText(dest,r);
								break;
							}
						outputText(dest,",");
						outputByte(dest,(BYTE)op1.mem);
						}
					else {
            src++;
						if(Pipe1 & 2) 			// sign extend
              op1.mem=(int16_t)(int8_t)Pipe2.bd[immofs];
						else
              op1.mem=MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
						switch(Pipe2.reg) {
			        case 0:       // ADD
								outputText(dest,"ADD ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
							case 1:       // OR
								outputText(dest,"OR ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
			        case 2:       // ADC
								outputText(dest,"ADC ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
			        case 3:       // SBB
								outputText(dest,"SBB ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
			        case 4:       // AND
								outputText(dest,"AND ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
			        case 5:       // SUB
								outputText(dest,"SUB ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
			        case 6:       // XOR
								outputText(dest,"XOR ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
			        case 7:       // CMP
								outputText(dest,"CMP ");
								outputPtr(dest,2);
								outputText(dest,r);
								break;
							}
						outputText(dest,",");
						outputWord(dest,op1.mem);
            }
          break;
        case 3:
					r=getRegister(Pipe1,Pipe2.rm);
					if(!(Pipe1 & 1)) {
						switch(Pipe2.reg) {
			        case 0:       // ADD
								outputText(dest,"ADD ");
								break;
							case 1:       // OR
								outputText(dest,"OR ");
								break;
							case 2:       // ADC
								outputText(dest,"ADC ");
								break;
							case 3:       // SBB
								outputText(dest,"SBB ");
								break;
							case 4:       // AND
								outputText(dest,"AND ");
								break;
							case 5:       // SUB
								outputText(dest,"SUB ");
								break;
							case 6:       // XOR
								outputText(dest,"XOR ");
								break;
							case 7:       // CMP
								outputText(dest,"CMP ");
								break;
							}
						outputText(dest,r);
						outputText(dest,",");
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
						} 
					else {
      			src++;      // imm16...
						switch(Pipe2.reg) {
			        case 0:       // ADD
								outputText(dest,"ADD ");
								break;
							case 1:       // OR
								outputText(dest,"OR ");
								break;
			        case 2:       // ADC
								outputText(dest,"ADC ");
								break;
			        case 3:       // SBB
								outputText(dest,"SBB ");
								break;
			        case 4:       // AND
								outputText(dest,"AND ");
								break;
			        case 5:       // SUB
								outputText(dest,"SUB ");
								break;
			        case 6:       // XOR
								outputText(dest,"XOR ");
								break;
			        case 7:       // CMP
								outputText(dest,"CMP ");
								break;
							}
						outputText(dest,r);
						outputText(dest,",");
						if(Pipe1 & 2) 			// sign extend  BOH verificare come va sotto... 2024
							outputWord(dest,(int16_t)(int8_t)Pipe2.bd[immofs+1]);
						else
							outputWord(dest,(int16_t)Pipe2.xm.w);
						}
          break;
          }
				src++;
        break;
        
			case 0x84:        // TESTB
				outputText(dest,"TESTB ");
				goto istest;
			case 0x85:        // TESTW
				outputText(dest,"TESTW ");
istest:
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
            if(!(Pipe1 & 1)) {
							}
						else {
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
            if(!(Pipe1 & 1)) {
							}
						else {
              }
            break;
          }
				outputText(dest,r);
				outputText(dest,",");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				src++;
				break;

			case 0x86:				// XCHG
			case 0x87:
				outputText(dest,"XCHG ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            break;
          }
				break;
        
			case 0x88:				//MOV8
			case 0x89:				//MOV16
			case 0x8a:
			case 0x8b:
				outputText(dest,"MOV ");
				r=getRegister(Pipe1,Pipe2.reg);
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r2=getAddressing(Pipe2,&src,&immofs);
						//verificare!!
            switch(Pipe1 & 0x3) {
              case 0:
								outputPtr(dest,1);
								outputText(dest,r2);
								outputText(dest,",");
								outputText(dest,r);
                break;
              case 1:
								outputPtr(dest,2);
								outputText(dest,r2);
								outputText(dest,",");
								outputText(dest,r);
                break;
              case 2:
								outputText(dest,r);
								outputText(dest,",");
								outputPtr(dest,1);
								outputText(dest,r2);
                break;
              case 3:
								outputText(dest,r);
								outputText(dest,",");
								outputPtr(dest,2);
								outputText(dest,r2);
                break;
              }
            break;
          case 3:
						r2=getRegister2(Pipe1,Pipe2.rm);
            switch(Pipe1 & 0x3) {
              case 0:
								outputText(dest,r2);
								outputText(dest,",");
								outputText(dest,r);
                break;
              case 1:
								outputText(dest,r2);
								outputText(dest,",");
								outputText(dest,r);
                break;
              case 2:
								outputText(dest,r);
								outputText(dest,",");
								outputText(dest,r2);
                break;
              case 3:
								outputText(dest,r);
								outputText(dest,",");
								outputText(dest,r2);
                break;
              }
            break;
          }
				src++;
				break;

			case 0x8c:        // MOV rm16,SREG
				outputText(dest,"MOV ");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
						outputText(dest,",");
#ifdef EXT_80286
						r=getSRegister(Pipe2.reg);
#else
						r=getSRegister(Pipe2.reg & 0x3);
#endif
						outputText(dest,r);
            break;
          case 3:
						r=getRegister(1,Pipe2.rm);
						outputText(dest,r);
						outputText(dest,",");
						r=getSRegister(Pipe2.reg & 3);
						outputText(dest,r);
            break;
          }
				src++;
				break;
        
			case 0x8d:        // LEA
				outputText(dest,"LEA ");
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            break;
          }
				src++;
				break;
        
			case 0x8e:        // MOV SREG,rm16
				outputText(dest,"MOV ");
#ifdef EXT_80286
				r=getSRegister(Pipe2.reg);
#else
				r=getSRegister(Pipe2.reg & 0x3);
#endif
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            break;
          case 3:
						r=getRegister2(1,Pipe2.rm);
						outputText(dest,r);
            break;
          }
				src++;
        break;
        
			case 0x8f:      // POP imm
				outputText(dest,"POP ");
        // i 3 bit "reg" qua sempre 0! 
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						outputPtr(dest,2);
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            break;
          case 3:			// 
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
            break;
          }
				src++;
				break;

			case 0x90:    // NOP, v. anche single byte instructions http://xxeo.com/single-byte-or-small-x86-opcodes
      case 0x91:		// XCHG AX
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
				outputText(dest,"XCHG AX,");
				r=getRegister2(Pipe1,Pipe1 & 7);
				outputText(dest,r);
        break;
        
			case 0x98:		// CBW
				outputText(dest,"CBW");
        break;
        
			case 0x99:		// CWD
				outputText(dest,"CWD");
        break;
        
			case 0x9a:      // CALLF
				outputText(dest,"CALLF ");
				Pipe2.x.h=*((WORD*)(src+2));
				outputFarAddress(dest,MAKELONG(Pipe2.x.h,Pipe2.x.l));
        src+=4;
				break;

   		case 0x9b:
				outputText(dest,"WAIT ");
				// WAIT Wait for BUSY pin (usually FPU 
#ifdef EXT_80x87
          status8087,control8087;
            
#endif
				break;
        
   		case 0x9c:        // PUSHF
				outputText(dest,"PUSHF");
				break;
        
   		case 0x9d:        // POPF
				outputText(dest,"POPF");
				break;

			case 0x9e:      // SAHF
				outputText(dest,"SAHF");
				break;
        
			case 0x9f:      // LAHF
				outputText(dest,"LAHF");
				break;

 			case 0xa0:      // MOV al,[ofs]
 			case 0xa1:      // MOV ,al
 			case 0xa2:      // MOV ax,
 			case 0xa3:      // MOV ,ax
				if(!(Pipe1 & 2)) {
					if(Pipe1 & 1) {
						outputText(dest,"MOV AX,[");
						outputWord(dest,Pipe2.x.l);
						}
					else {
						outputText(dest,"MOV AL,[");
						outputWord(dest,Pipe2.x.l);
						}
					outputText(dest,"]");
					}
				else {        
					outputText(dest,"MOV [");
					if(Pipe1 & 1) {
						outputWord(dest,Pipe2.x.l);
						outputText(dest,"],AX");
						}
					else {
						outputWord(dest,Pipe2.x.l);
						outputText(dest,"],AL");
						}
					}
        src+=2;
				break;
        
			case 0xa4:      // MOVSB
				outputText(dest,"MOVSB ");
				break;

			case 0xa5:      // MOVSW
				outputText(dest,"MOVSW ");
					if(CPUType>=3) {
					}
				break;
        
			case 0xa6:      // CMPSB
				outputText(dest,"CMPSB ");
				break;

			case 0xa7:      // CMPSW
				outputText(dest,"CMPSW ");
				break;

			case 0xa8:        // TESTB
				outputText(dest,"TESTB AL,");
				outputByte(dest,Pipe2.b.l);
        src++;
				break;

			case 0xa9:        // TESTW
				outputText(dest,"TESTW AX,");
				outputWord(dest,Pipe2.x.l);
        src+=2;
				break;
        
			case 0xaa:      // STOSB
				outputText(dest,"STOSB ");
				break;

			case 0xab:      // STOSW
				outputText(dest,"STOSW ");
				break;

			case 0xac:      // LODSB; [was NO OVERRIDE qua! ... v. bug 8088
				outputText(dest,"LODSB ");
				break;

			case 0xad:      // LODSW
				outputText(dest,"LODSW ");
				break;

			case 0xae:      // SCASB; NO OVERRIDE qua!  http://www.nacad.ufrj.br/online/intel/vtune/users_guide/mergedProjects/analyzer_ec/mergedProjects/reference_olh/mergedProjects/instructions/instruct32_hh/vc285.htm
				outputText(dest,"SCASB ");
				break;

			case 0xaf:      // SCASW
				outputText(dest,"SCASW ");
				break;

      case 0xb0:      // altre MOVB
      case 0xb1:
      case 0xb2:
      case 0xb3:
      case 0xb4:
      case 0xb5:
      case 0xb6:
      case 0xb7:
				outputText(dest,"MOVB ");
				r=getRegister(0,Pipe1 & 7);
				outputText(dest,r);
				outputText(dest,",");
				outputByte(dest,Pipe2.b.l);
        src++;
				break;
        
      case 0xb8:      // altre MOVW
      case 0xb9:
      case 0xba:
      case 0xbb:
      case 0xbc:
      case 0xbd:
      case 0xbe:
      case 0xbf:
				outputText(dest,"MOVW ");
				r=getRegister(1,Pipe1 & 7);
				outputText(dest,r);
				outputText(dest,",");
				outputWord(dest,Pipe2.x.l);
        src+=2;
				break;
        
#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0xc0:      // RCL, RCR, SHL ecc
				switch(Pipe2.reg) {
					case 0:       /* ROL */
						outputText(dest,"ROL ");
						break;
					case 1:       /* ROR*/
						outputText(dest,"ROR ");
						break;
					case 2:       /* RCL*/
						outputText(dest,"RCL ");
						break;
					case 3:       /* RCR*/
						outputText(dest,"RCR ");
						break;
					case 6:       /* SAL/SHL ... bah mi sembrano identiche! ma sarebbe SETMO :D secondo unofficial */
						outputText(dest,"SAL/SETMO ");
						break;
					case 4:       /* SHL/SAL*/
						outputText(dest,"SHL ");
						break;
					case 5:       /* SHR*/
						outputText(dest,"SHR ");
						break;
					case 7:       /* SAR*/
						outputText(dest,"SAR ");
						break;
					}
			  if(Pipe2.mod<3)
					GetMorePipe(_cs,_ip-1);   // 
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
            op1.mem=Pipe2.bd[immofs];
						i=res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
						res2.b &= 31;		// non va dentro la macro...
#endif
#endif
						res3.b=res1.b=GetValue(*theDs,op2.mem);
						i=0;
						ROTATE_SHIFT8
						PutValue(*theDs,op2.mem,res3.b);
            break;
          case 3:
            op2.reg8= Pipe2.rm & 0x4 ? &regs.r[Pipe2.rm & 0x3].b.h : &regs.r[Pipe2.rm & 0x3].b.l;
						op1.mem=Pipe2.b.h;
						i=res2.b=(uint8_t)op1.mem;
#ifndef EXT_NECV20
#ifdef EXT_80286
						res2.b &= 31;		// non va dentro la macro...
#endif
#endif
    				_ip++;      // imm8
						res3.b=res1.b=*op2.reg8;
						i=0;
						ROTATE_SHIFT8
						*op2.reg8=res3.b;
						break;
					}
				src++;
				break;

			case 0xc1:
				switch(Pipe2.reg) {
					case 0:       /* ROL */
						outputText(dest,"ROL ");
						break;
					case 1:       /* ROR*/
						outputText(dest,"ROR ");
						break;
					case 2:       /* RCL*/
						outputText(dest,"RCL ");
						break;
					case 3:       /* RCR*/
						outputText(dest,"RCR ");
						break;
					case 6:       /* SAL/SHL ... bah mi sembrano identiche! ma sarebbe SETMO :D secondo unofficial */
						outputText(dest,"SAL/SETMO ");
						break;
					case 4:       /* SHL/SAL*/
						outputText(dest,"SHL ");
						break;
					case 5:       /* SHR*/
						outputText(dest,"SHR ");
						break;
					case 7:       /* SAR*/
						outputText(dest,"SAR ");
						break;
					}
			  if(Pipe2.mod<3)
					GetMorePipe(_cs,_ip-1);   // 
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
            op1.mem=Pipe2.bd[immofs];
						i=res2.b=(uint8_t)op1.mem;
						res3.x=res1.x=GetShortValue(*theDs,op2.mem);
						i=0;
						ROTATE_SHIFT16
						PutShortValue(*theDs,op2.mem,res3.x);
            break;
          case 3:
            op2.reg16= &regs.r[Pipe2.b.l & 0x7].x;
						op1.mem=Pipe2.b.h;
						i=res2.b=(uint8_t)op1.mem;
    				_ip++;      // imm8
						res3.x=res1.x=*op2.reg16;
						i=0;
						ROTATE_SHIFT16
						*op2.reg16=res3.x;
						break;
					}
				src++;
				break;
#endif
        
#if !defined(EXT_80186) && !defined(EXT_NECV20)
			case 0xc0:
#endif

			case 0xc2:      // RETN
				outputText(dest,"RETN ");
				outputWord(dest,Pipe2.x.l);
				src+=2;
				break;
        
#if !defined(EXT_80186) && !defined(EXT_NECV20)
			case 0xc1:
#endif

			case 0xc3:      // RET
				outputText(dest,"RET");
				break;

			case 0xc4:				//LES
				outputText(dest,"LES ");
				goto isLes;
			case 0xc5:				//LDS
				outputText(dest,"LDS ");
isLes:
				r=getRegister(Pipe1,Pipe2.reg);
				outputText(dest,r);
				outputText(dest,",");
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            break;
          case 3:		// 
            break;
          }
				src++;
				break;

 			case 0xc6:				//MOV imm8...
 			case 0xc7:				//MOV imm16
				outputText(dest,"MOV ");
        // i 3 bit "reg" qua sempre 0! 
				switch(Pipe2.mod) {
					case 2:
						/*Pipe2.bd[3]=*/*src++;
					case 1:
						/*Pipe2.bd[4]=*/*src++;
					case 0:
						outputPtr(dest,2);
						immofs=1;
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
						outputText(dest,",");
					  if(!(Pipe1 & 1)) {
              src++;
              op1.mem=Pipe2.bd[immofs];
							outputByte(dest,(uint8_t)op1.mem);
							}
						else {
              src+=2;
              op1.mem=MAKEWORD(Pipe2.bd[immofs],Pipe2.bd[immofs+1]);
							outputWord(dest,op1.mem);
							}
            break;
          case 3:		// 
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
						outputText(dest,",");
            if(!(Pipe1 & 1)) {
							src++;
              op1.mem=Pipe2.b.h;
							outputByte(dest,(uint8_t)op1.mem);
							}
						else {
							src+=2;
              op1.mem=Pipe2.xm.w;
							outputWord(dest,op1.mem);
              }
            break;
          }
				src++;
				break;


#if defined(EXT_80186) || defined(EXT_NECV20)
			case 0xc8:        //ENTER
				outputText(dest,"ENTER ");
				outputWord(dest,(uint8_t)Pipe2.b.u);
        src+=3;
        break;
#endif
        
#ifdef EXT_80186
			case 0xc9:        // LEAVE
				outputText(dest,"LEAVE");
        _sp=_bp;
				POP_STACK(_bp);
        break;
#endif
        
#if !defined(EXT_80186) && !defined(EXT_NECV20)     // bah, così dicono..
			case 0xc8:
#endif
			case 0xca:      // RETF
				outputText(dest,"RETF ");
				outputWord(dest,(uint16_t)Pipe2.x.l);
				src+=2;
        break;
        
#ifndef EXT_80186     // bah, così dicono..
			case 0xc9:
#endif
			case 0xcb:
				outputText(dest,"RETF");
				break;

			case 0xcc:
				outputText(dest,"INT3");
				break;

			case 0xcd:
				outputText(dest,"INT ");

#ifdef EXT_80286
#endif
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0xce:
				outputText(dest,"INTO");
				break;

			case 0xcf:        // IRET
				outputText(dest,"IRET");
				break;

			case 0xd0:      // RCL, RCR ecc 8
				i=1;
				goto do_rcl8;
			case 0xd2:
				i=0;

do_rcl8:
				switch(Pipe2.reg) {
					case 0:       /* ROL */
						outputText(dest,"ROL ");
						break;
					case 1:       /* ROR*/
						outputText(dest,"ROR ");
						break;
					case 2:       /* RCL*/
						outputText(dest,"RCL ");
						break;
					case 3:       /* RCR*/
						outputText(dest,"RCR ");
						break;
					case 6:       /* SAL/SHL ... bah mi sembrano identiche! ma sarebbe SETMO :D secondo unofficial */
						outputText(dest,"SAL/SETMO ");
						break;
					case 4:       /* SHL/SAL*/
						outputText(dest,"SHL ");
						break;
					case 5:       /* SHR*/
						outputText(dest,"SHR ");
						break;
					case 7:       /* SAR*/
						outputText(dest,"SAR ");
						break;
					}
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
						break;
					}
				outputText(dest,i ? "" : ",CL");
				src++;
				break;

			case 0xd1:      // RCL, RCR ecc 16
				i=1;
				goto do_rcl16;
			case 0xd3:
				i=0;

do_rcl16:
				switch(Pipe2.reg) {
					case 0:       /* ROL */
						outputText(dest,"ROL ");
						break;
					case 1:       /* ROR*/
						outputText(dest,"ROR ");
						break;
					case 2:       /* RCL*/
						outputText(dest,"RCL ");
						break;
					case 3:       /* RCR*/
						outputText(dest,"RCR ");
						break;
					case 6:       /* SAL/SHL ... bah mi sembrano identiche! ma sarebbe SETMO :D secondo unofficial */
						outputText(dest,"SAL/SETMO ");
						break;
					case 4:       /* SHL/SAL*/
						outputText(dest,"SHL ");
						break;
					case 5:       /* SHR*/
						outputText(dest,"SHR ");
						break;
					case 7:       /* SAR*/
						outputText(dest,"SAR ");
						break;
					}
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						outputText(dest,r);
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						outputText(dest,r);
						break;
					}
				outputText(dest,i ? "" : ",CL");
				src++;
				break;

			case 0xd4:      // AAM
				outputText(dest,"AAM");
				break;
        
			case 0xd5:      // AAD
				outputText(dest,"AAD");
				break;
        
			case 0xd6:      // dice che non è documentata (su 8088)...
				outputText(dest,"SALC");
#ifndef EXT_NECV20
				break;
#endif								// altrimenti è come D7!

			case 0xd7:      // XLAT
				outputText(dest,"XLATB");
        break;
        
#ifdef EXT_80x87      // https://www.ic.unicamp.br/~celio/mc404/opcodes.html#Co
			case 0xd8:
			case 0xd9:
			case 0xda:
			case 0xdb:
			case 0xdc:
			case 0xdd:
			case 0xde:
			case 0xdf:
				outputText(dest,"[FPU] ");
				i=2;

				switch(Pipe1 & 7) {
					case 0:
							status8087,control8087;
						break;
					case 1: 
						switch(Pipe2.b.l) {
							case 0xd0:    // FNOP
								outputText(dest,"FNOP ");
								break;
							case 0x3c:      // FNSTCW
								outputText(dest,"FNSTCW ");
								break;
							case 0xE0:    // FCHS
								outputText(dest,"FCHS ");
								break;
							case 0xE1:    // FABS
								outputText(dest,"FABS ");
								break;
							case 0xE5:    // FXAM
								outputText(dest,"FXAM ");
								break;
							case 0xEE:    // FLDZ
								outputText(dest,"FLDZ ");
								break;
							case 0xE8:    // FLD1
								outputText(dest,"FLD1 ");
								break;
							case 0xE9:    // FLDL2T
								outputText(dest,"FLDL2T ");
								break;
							case 0xEB:    // FLDPI
								outputText(dest,"FLDPI ");
								break;
							case 0xEC:    // FLDLG2
								outputText(dest,"FLDLG2 ");
								break;
							case 0xED:    // FLDLN2
								outputText(dest,"FLDLN2 ");
								break;
							case 0xF0:    // F2XM1
								outputText(dest,"F2XM1 ");
								break;
							case 0xF1:    // FYL2X
								outputText(dest,"FYL2X ");
								break;
							case 0xF2:    // FPTAN
								outputText(dest,"FPTAN ");
								break;
							case 0xF3:    // FPATAN
								outputText(dest,"FPATAN ");
								break;
							case 0xF4:    // FXTRACT
								outputText(dest,"FXTRACT ");
								break;
							case 0xF6:    // FDECSTP
								outputText(dest,"FDECSTP ");
								break;
							case 0xF7:    // FINCSTP
								outputText(dest,"FINCSTP ");
								break;
							case 0xF8:    // FPREM
								outputText(dest,"FPREM ");
								break;
							case 0xF9:    // FYL2XP1
								outputText(dest,"FYL2XP1 ");
								break;
							case 0xFA:    // FSQRT
								outputText(dest,"FSQRT ");
								break;
							case 0xFC:    // FRNDINT
								outputText(dest,"FRNDINT ");
								break;
							case 0xFD:    // FSCALE
								outputText(dest,"FSCALE ");
								break;
							default:      // FNSTCW
								outputText(dest,"FNSTCW ");
								break;
							}
							status8087,control8087;
						break;
					case 2:
							status8087,control8087;
						break;
					case 3:
						switch(Pipe2.b.l) {
							case 0xe0:      // FENI
								outputText(dest,"FENI ");
								break;
							case 0xe1:      // FDISI
								outputText(dest,"FDISI ");
								break;
							case 0xe2:      // FCLEX
								outputText(dest,"FCLEX ");
								break;
							case 0xe3:      // FNINIT
								outputText(dest,"FNINIT ");
								i=0;
								break;
							}
							status8087,control8087;
						break;
					case 4:
							status8087,control8087;
						break;
					case 5:
						// FSTSW
						outputText(dest,"FNSTSW ");		// FSTSW AX  diceva ma boh
						i=4;
						break;
					case 6:
							status8087,control8087;
						break;
					case 7:
						switch(Pipe2.b.l) {
							case 0xe0:      // FNSTSW AX
								outputText(dest,"FNSTSW AX ");		// 
								i=4;
								break;
							}
						break;
					}
//no				outputText(dest,",");
				if(i) {
					switch(Pipe2.mod) {
						case 2:
							Pipe2.b.h=*src++;
						case 1:
							Pipe2.b.u=*src++;
						case 0:
							r=getAddressing(Pipe2,&src,&immofs);
							outputPtr(dest,i);
							outputText(dest,r);
							break;
						case 3:
							r=getRegister2(Pipe1,Pipe2.rm);
							outputText(dest,r);
							break;
	          }
          }
				src++;
        break;
          
#else            
			case 0xd8:
			case 0xd9:
			case 0xda:
			case 0xdb:
			case 0xdc:
			case 0xdd:
			case 0xde:
			case 0xdf:
//				r=getRegister(Pipe1,Pipe2);
				outputText(dest,"FPU ");

#ifdef EXT_80286
#endif

#ifdef EXT_80186		// boh
#else
#endif
#endif

        break;
        
			case 0xe0:      // LOOPNZ/LOOPNE
				outputText(dest,"LOOPNZ ");
				outputWord(dest,pcaddr+(int8_t)Pipe2.b.l);
				src++;
				break;
        
			case 0xe1:      // LOOPZ/LOOPE
				outputText(dest,"LOOPZ ");
				outputWord(dest,pcaddr+(int8_t)Pipe2.b.l);
				src++;
				break;
        
			case 0xe2:      // LOOP
				outputText(dest,"LOOP ");
				outputWord(dest,pcaddr+(int8_t)Pipe2.b.l);
				src++;
				break;

			case 0xe3:      // JCXZ
				outputText(dest,"JCXZ ");
				outputWord(dest,pcaddr+(int8_t)Pipe2.b.l);
				src++;
				break;

			case 0xe4:        // INB
				outputText(dest,"INB AL,");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0xe5:        // INW
				outputText(dest,"INW AX,");
				outputByte(dest,Pipe2.b.l);
				src++;
				break;

			case 0xe6:      // OUTB
				outputText(dest,"OUTB ");
				outputByte(dest,Pipe2.b.l);
				outputText(dest,",AL");
				src++;
				break;

			case 0xe7:      // OUTW
				outputText(dest,"OUTW ");
				outputByte(dest,Pipe2.b.l);
				outputText(dest,",AX");
				src++;
				break;

			case 0xe8:      // CALL rel16
				outputText(dest,"CALL ");
				outputAddress(dest,pcaddr+(int16_t)Pipe2.x.l+3);
        src+=2;
				break;

			case 0xe9:      // JMP rel16 (32 su 386)
				outputText(dest,"JMP ");
				outputAddress(dest,pcaddr+(int16_t)Pipe2.x.l+3);
        src+=2;
				break;
        
			case 0xea:      // JMP abs  (32 su 386)
				outputText(dest,"JMPF ");		// ljmp
//        GetMorePipe(_cs,_ip-1);
				Pipe2.x.h=*((WORD*)(src+2));
				outputFarAddress(dest,MAKELONG(Pipe2.x.l,Pipe2.x.h));
        src+=4;
				break;

			case 0xeb:      // JMP rel8
				outputText(dest,"JMPS ");
				outputAddress(dest,pcaddr+(int8_t)Pipe2.b.l+2);
				src++;
				break;
			
			case 0xec:        // INB
				outputText(dest,"INB AL,DX");
				break;

			case 0xed:        // INW
				outputText(dest,"INW AX,DX");

#ifdef EXT_NECV20
				// dice che ED FD è RETEM, ritorno da modo 8080...
//			_f.MD=1;
				// e ED ED CALLN
#endif
				break;

			case 0xee:        // OUTB
				outputText(dest,"OUTB DX,AL");
				break;

			case 0xef:        // OUTW
				outputText(dest,"OUTW DX,AX");
				break;

			case 0xf0:				// LOCK (prefisso...
				outputText(dest,"LOCK");
				break;

			case 0xf1:
				outputText(dest,"TRAP");
#ifdef EXT_NECV20
#endif
				break;

			case 0xf2:				//REPNZ/REPNE; entrambi vengono mutati in 3 nelle istruzioni che non usano Z
				outputText(dest,"REPNZ ");
				break;

			case 0xf3:				//REPZ/REPE
				outputText(dest,"REPZ ");
				break;

			case 0xf4:				// HLT
				outputText(dest,"HLT ");
				break;

			case 0xf5:				// CMC
				outputText(dest,"CMC ");
				break;

			case 0xf6:        // altre MUL ecc //	; mul on V20 does not affect the zero flag,   but on an 8088 the zero flag is used
			case 0xf7:
				switch(Pipe2.mod) {
					case 2:
						Pipe2.b.h=*src++;
					case 1:
						Pipe2.b.u=*src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						if(!(Pipe1 & 1)) {
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
									outputText(dest,"TEST ");
									outputPtr(dest,1);
									outputText(dest,r);
                  src++;
									break;
								case 2:			// NOT
									outputText(dest,"NOT ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
								case 3:			// NEG
									outputText(dest,"NEG ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
								case 4:       // MUL8
									outputText(dest,"MUL8 ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
								case 5:       // IMUL8
									outputText(dest,"IMUL8 ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
								case 6:       // DIV8
									outputText(dest,"DIV8 ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
								case 7:       // IDIV8
									outputText(dest,"IDIV8 ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
								}
							src++;
							}
						else {
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
									outputText(dest,"TEST ");
									outputPtr(dest,2);
									outputText(dest,r);
                  src+=2;
									break;
								case 2:			// NOT
									outputText(dest,"NOT ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 3:			// NEG
									outputText(dest,"NEG ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 4:       // MUL16
									outputText(dest,"MUL16 ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 5:       // IMUL16
									outputText(dest,"IMUL16 ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 6:       // DIV16
									outputText(dest,"DIV16 ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 7:       // IDIV16
									outputText(dest,"IDIV16 ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								}
							src+=2;
              }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						if(!(Pipe1 & 1)) {
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
									outputText(dest,"TEST ");
                  src++;
									outputText(dest,r);
									break;
								case 2:			// NOT
									outputText(dest,"NOT ");
									outputText(dest,r);
									break;
								case 3:			// NEG
									outputText(dest,"NEG ");
									outputText(dest,r);
									break;
								case 4:       // MUL8
									outputText(dest,"MUL8 ");
									outputText(dest,r);
									break;
								case 5:       // IMUL8
									outputText(dest,"IMUL8 ");
									outputText(dest,r);
									break;
								case 6:       // DIV8
									outputText(dest,"DIV8 ");
									outputText(dest,r);
									break;
								case 7:       // IDIV8
									outputText(dest,"IDIV8 ");
									outputText(dest,r);
									break;
								}
							} 
						else {
							switch(Pipe2.reg) {
								case 0:       // TEST
								case 1:       // TEST forse....
									outputText(dest,"TEST ");
									outputText(dest,r);
                  src+=2;
									break;
								case 2:			// NOT
									outputText(dest,"NOT ");
									outputText(dest,r);
									break;
								case 3:			// NEG
									outputText(dest,"NEG ");
									outputText(dest,r);
									break;
								case 4:       // MUL16
									outputText(dest,"MUL16 ");
									outputText(dest,r);
									break;
								case 5:       // IMUL16
									outputText(dest,"IMUL16 ");
									outputText(dest,r);
									break;
								case 6:       // DIV16
									outputText(dest,"DIV16 ");
									outputText(dest,r);
									break;
								case 7:       // IDIV16
									outputText(dest,"IDIV16 ");
									outputText(dest,r);
									break;
								}
              }
            break;
          }
				break;
        
			case 0xf8:				// CLC
				outputText(dest,"CLC");
				break;

			case 0xf9:				// STC
				outputText(dest,"STC");
				break;
        
			case 0xfa:        // CLI
				outputText(dest,"CLI");
				break;

			case 0xfb:        // STI
				outputText(dest,"STI");
				break;
        
			case 0xfc:				// CLD
				outputText(dest,"CLD");
				break;

			case 0xfd:				// STD
				outputText(dest,"STD");
				break;
        
			case 0xfe:    // altre INC DEC 8bit
			case 0xff:    // 
				switch(Pipe2.mod) {
					case 2:
						/*Pipe2.b.h=**/src++;
					case 1:
						/*Pipe2.b.u=**/src++;
					case 0:
						r=getAddressing(Pipe2,&src,&immofs);
						if(!(Pipe1 & 1)) {
							switch(Pipe2.reg) {
								case 0:       // INC
									outputText(dest,"INC ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
								case 1:       // DEC
									outputText(dest,"DEC ");
									outputPtr(dest,1);
									outputText(dest,r);
									break;
#ifdef EXT_NECV20
								case 3:
								case 5:			// sono "nonsense" a 8 bit, dice!
									outputUnknown(dest,Pipe2.b.l);
//								  CPUPins |= DoHalt;
									break;
#endif
								default:
									outputUnknown(dest,Pipe2.b.l);
									break;
								}
							src++;
							}
						else {
							switch(Pipe2.reg) {
								case 0:       // INC
									outputText(dest,"INC ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 1:       // DEC
									outputText(dest,"DEC ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 2:			// CALL (D)WORD PTR
									outputText(dest,"CALL ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 3:			// LCALL (FAR) (D)WORD PTR
									outputText(dest,"CALLF ");		// LCALL
									outputPtr(dest,4);
									outputText(dest,r);
//									outputText(dest,":");
//									outputText(dest,r);
// finire!									_cs=GetShortValue(*theDs,op2.mem+2);
									break;
								case 4:       // JMP DWORD PTR jmp [100]
									outputText(dest,"JMP ");		// 
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 5:       // JMP FAR DWORD PTR
									outputText(dest,"JMPF ");		// LJMP
									outputPtr(dest,4);
									outputText(dest,r);
									outputText(dest,":");
									outputText(dest,r);
//									_ip=res1.x;
//									_cs=GetShortValue(*theDs,op2.mem+2);
									break;
								case 6:       // PUSH 
#ifdef UNDOCUMENTED_8086
								case 7:
#endif
									outputText(dest,"PUSH ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								default:
									outputUnknown(dest,Pipe2.b.l);
									break;
								}
	 						src++;
		          }
            break;
          case 3:
						r=getRegister2(Pipe1,Pipe2.rm);
						if(!(Pipe1 & 1)) {
							switch(Pipe2.reg) {
								case 0:       // INC
									outputText(dest,"INC ");
									outputText(dest,r);
									break;
								case 1:       // DEC
									outputText(dest,"DEC ");
									outputText(dest,r);
									break;
								default:
									outputUnknown(dest,Pipe2.b.l);
									break;
								}
							} 
						else {
							switch(Pipe2.reg) {
								case 0:       // INC
									outputText(dest,"INC ");
									outputText(dest,r);
									break;
								case 1:       // DEC
									outputText(dest,"DEC ");
									outputText(dest,r);
									break;
								case 2:			// CALL (D)WORD PTR 
									outputText(dest,"CALL ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 3:			// CALL FAR DWORD32 PTR
									outputText(dest,"CALLF ");	// LCALL
									outputPtr(dest,4);
									outputText(dest,r);
//									_ip=res1.x;			// VERIFICARE COME SI FA! o forse non c'è
//									_cs=*op2.reg16+2;
									break;
								case 4:       // JMP DWORD PTR jmp [100]
									outputText(dest,"JMP ");
									outputPtr(dest,2);
									outputText(dest,r);
									break;
								case 5:       // JMP FAR DWORD PTR
									outputText(dest,"JMPF ");		// LJMP
									outputPtr(dest,4);
// finire									_ip=res1.x;			// VERIFICARE COME SI FA! o forse non c'è
//									_cs=*op2.reg16+2;
									outputText(dest,r);
									outputText(dest,":");
									outputText(dest,r);
									break;
								case 6:       // PUSH 
#ifdef UNDOCUMENTED_8086
								case 7:
#endif
					if(CPUType>=2) {
					}
					else {
					}
									outputText(dest,"PUSH ");
									break;
								default:
									outputUnknown(dest,Pipe2.b.l);
									break;
								}
              }
            break;
          }
				break;
        
			default:
				outputUnknown(dest,Pipe2.b.l);
				break;
			}
    
		pcaddr+=(src-oldsrc);
		if(opzioni & 1) {
			char myBuf[256],myBuf2[32];

			strcpy(myBuf,dest);
			*dest=0;
			if(opzioni & 2) {
				wsprintf(dest,"%04X: ",pcaddr-(src-oldsrc));
				}
			while(oldsrc<src) {
				wsprintf(myBuf2,"%02X ",*oldsrc);
				strcat(dest,myBuf2);
				oldsrc++;
				}
			strcat(dest,"\t");
			strcat(dest,myBuf);

			}
		if(*commento) {
			strcat(dest,"\t; ");
			strcat(dest,commento);
			}
		strcat(dest,"\r\n");

		if(!len)
			fExit=1;

		} while(!fExit);
	}
