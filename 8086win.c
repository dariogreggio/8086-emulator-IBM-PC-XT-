#define APPNAME "8086win"



// Windows Header Files:
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

// Local Header Files
#include "8086win.h"
#include "resource.h"
#include "cjson.h"

// Makes it easier to determine appropriate code paths:
#if defined (WIN32)
	#define IS_WIN32 TRUE
#else
	#define IS_WIN32 FALSE
#endif
#define IS_NT      IS_WIN32 && (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN32S  IS_WIN32 && (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95 (BOOL)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32

// Global Variables:
HINSTANCE g_hinst;
HANDLE hAccelTable;
char szAppName[] = APPNAME; // The name of this application
char INIFile[] = APPNAME".ini";
char szTitle[]   = APPNAME; // The title bar text
LARGE_INTEGER pcFrequency;
#ifdef CGA
int AppXSize=670,AppYSize=270,AppYSizeR,YXRatio=1;
#endif
#ifdef MDA
int AppXSize=750,AppYSize=400,AppYSizeR,YXRatio=1;
#endif
#ifdef VGA
int AppXSize=700,AppYSize=460,AppYSizeR,YXRatio=1;
extern uint8_t VGABios[];
//extern uint8_t VGAram[];
#endif
BOOL fExit,debug=0,doppiaDim;
HWND ghWnd,hStatusWnd;
HBRUSH hBrush;
HPEN hPen1;
HFONT hFont,hFont2;
#ifdef CGA
const WORD cgaColors[3][4]={ {0/*BLACK*/,11/*CYAN*/,13/*MAGENTA*/,15/*WHITE*/},
  {0/*BLACK*/,12/*RED*/,2/*GREEN*/,14/*YELLOW*/},
  {0/*BLACK*/,12/*RED*/,11/*CYAN*/,15/*WHITE*/} };
// cga_colors[4] = {0 /* Black */, 0x1F1F /* Cyan */, 0xE3E3 /* Magenta */, 0xFFFF /* White */
//const WORD extCGAColors[16]={BLACK,GREEN,BLUE,CYAN,BURLYWOOD /*CRIMSON*/,DARKGRAY,MAGENTA,BRIGHTMAGENTA /*VIOLET*/,
//	GRAY128,LIGHTGREEN,LIGHTGRAY,BRIGHTCYAN,LIGHTRED,YELLOW,PINK,WHITE};
//const WORD textColors[16]={BLACK,BLUE,GREEN,CYAN, RED,MAGENTA,YELLOW,LIGHTGRAY,
//	GRAY128,LIGHTBLUE,LIGHTGREEN,BRIGHTCYAN, LIGHTRED,BRIGHTMAGENTA,LIGHTYELLOW,WHITE};
COLORREF Colori[16]={
	RGB(0,0,0),						 // nero
	RGB(0x00,0x00,0x80),	 // blu 
	RGB(0x00,0x80,0x00),	 // verde 
	RGB(0x00,0x80,0x80),	 // azzurro/cyan
	RGB(0x80,0x00,0x00),	 // rosso
	RGB(0x80,0x00,0x80),	 // porpora/magenta
	RGB(0x80,0x80,0x00),	 // giallo
	RGB(0xc0,0xc0,0xc0),		 // grigio chiaro
	
	RGB(0x80,0x80,0x80),	 // grigio medio
	RGB(0x00,0x00,0xff),	 // blu chiaro
	RGB(0x00,0xff,0x00),	 // verde chiaro
	RGB(0x00,0xff,0xff),	 // azzurro/cyan chiaro
	RGB(0xff,0x00,0x00),	 // rosso chiaro
	RGB(0xff,0x00,0xff),	 // porpora/magenta chiaro
	RGB(0xff,0xff,0x00),	 // giallo
	RGB(0xff,0xff,0xff),	 // bianco
	};
#endif
#ifdef MDA
COLORREF Colori[4]={
	RGB(0,0,0),						 // nero
	RGB(0x00,0x88,0x00),	 // verde scuro
	RGB(0x00,0xc0,0x00),	 // verde 
	RGB(0x00,0xff,0x00),	 // verde chiaro
	};
#endif
#ifdef VGA
const WORD cgaColors[3][4]={{0/*BLACK*/,12/*RED*/,2/*GREEN*/,14/*YELLOW*/},
	{0/*BLACK*/,11/*CYAN*/,13/*MAGENTA*/,15/*WHITE*/},
  {0/*BLACK*/,12/*RED*/,11/*CYAN*/,15/*WHITE*/} };
// cga_colors[4] = {0 /* Black */, 0x1F1F /* Cyan */, 0xE3E3 /* Magenta */, 0xFFFF /* White */
COLORREF Colori[256]={
	RGB(0,0,0),						 // nero
	RGB(0x00,0x00,0x80),	 // blu 
	RGB(0x00,0x80,0x00),	 // verde 
	RGB(0x00,0x80,0x80),	 // azzurro/cyan
	RGB(0x80,0x00,0x00),	 // rosso
	RGB(0x80,0x00,0x80),	 // porpora/magenta
	RGB(0x80,0x80,0x00),	 // giallo
	RGB(0xc0,0xc0,0xc0),		 // grigio chiaro
	
	RGB(0x80,0x80,0x80),	 // grigio medio
	RGB(0x00,0x00,0xff),	 // blu chiaro
	RGB(0x00,0xff,0x00),	 // verde chiaro
	RGB(0x00,0xff,0xff),	 // azzurro/cyan chiaro
	RGB(0xff,0x00,0x00),	 // rosso chiaro
	RGB(0xff,0x00,0xff),	 // porpora/magenta chiaro
	RGB(0xff,0xff,0x00),	 // giallo
	RGB(0xff,0xff,0xff),	 // bianco
	};
#endif
UINT hTimer,hTimer2;
extern BOOL ColdReset;
extern BYTE CPUPins;
extern BYTE CPUDivider;
extern BYTE bios_rom[],ram_seg[],IBMBASIC[],HDbios[],bios_rom_opt[];
extern BYTE CGAram[];		// anche MDA direi 
extern BYTE VGAram[];
extern BYTE CGAreg[],MDAreg[],VGAreg[],VGAactlReg[],VGAcrtcReg[],VGAgraphReg[],VGAseqReg[];
extern BYTE i8259IRR,i8259IMR,i8259IRR2,i8259IMR2;
extern BYTE i8253RegR[],i8253RegW[],i8253Mode[],i8253ModeSel;
extern BYTE i8250Reg[],i8250Regalt[];
extern BYTE i6845RegR[],i6845RegW[];
extern BYTE i146818RegR[],i146818RegW[],i146818RAM[];
extern BYTE i8042RegR[],i8042RegW[],KBRAM[],KBControl,KBStatus;
extern SWORD VICRaster;
#ifdef VGA
BYTE VideoRAM[(HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2)  *3];		// *3 , 24bpp
#else
BYTE VideoRAM[(HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2)  /2];		// :2 , 4bpp
#endif
extern union {
  BYTE b[4];
  DWORD d;
  } RTC;
extern BYTE VIDIRQ;
const BYTE dayOfMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};
extern HFILE spoolFile;
extern unsigned char MSDOS_DISK[2][1440/*360*/*512*2];
extern BYTE sectorsPerTrack[];
extern BYTE totalTracks;
extern BYTE floppyState[];
extern unsigned char MSDOS_HDISK[];
BITMAPINFO *bmI;
extern BYTE mouseState;
POINT mousePos={HORIZ_SIZE/2,VERT_SIZE/2},oldMousePos={-1,-1};		// pos iniziali mouse 
uint8_t oldmouse=255;
extern BYTE COMDataEmuFifo[];
extern BYTE COMDataEmuFifoCnt;

void test_dump_vga_reg();


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MSG msg;

	if(!hPrevInstance) {
		if(!InitApplication(hInstance)) {
			return (FALSE);
		  }
	  }

	if (!InitInstance(hInstance, nCmdShow)) {
		return (FALSE);
  	}

	if(*lpCmdLine) {
		PostMessage(ghWnd,WM_USER+1,0,(LPARAM)lpCmdLine);
		}

	hAccelTable = LoadAccelerators (hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));
	Emulate(0);

  return (msg.wParam);
	}


int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin) {
	int k,y1,y2,x1,x2,row1,row2;
  uint8_t color,color2;
  BYTE ch,ch2;
	register int i,j;
	register BYTE *p,*p1;
	HBITMAP hBitmap,hOldBitmap;
	HDC hCompDC;
	UINT16 px,py;
	BYTE *pVideoRAM;
  static BYTE cursorState=0,cursorDivider=0,blinkState=0;

/*					{
					char myBuf[128];
				wsprintf(myBuf,"updatescreen %u %u: ",rowIni,rowFin);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/

#ifdef CGA
  rowFin=min(rowFin,VERT_SIZE);
  if(rowIni==0)
    CGAreg[0xa] &= ~B8(00001000); // CGA not in retrace V
  else if(rowFin>=VERT_SIZE-8)
    CGAreg[0xa] |= B8(00001000); // CGA in retrace V
  
  if(CGAreg[8] & 8) {     // enable video
    if(CGAreg[8] & 2) {     // graphic mode
      if(!(CGAreg[8] & 16)) {     // lores graph (320x200x4)
        if(CGAreg[9] & 16)      // bright foreground
          ;
        if(rowIni==0) {
		      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
					pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//          setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//              writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
							*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
              }
            }
          }
				pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
//        setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
        // OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!
        if(CGAreg[8] & 4)      // disable color ovvero palette #3
          color=2;     // quale palette
        else
          color=CGAreg[9] & B8(00100000) ? 1 : 0;     // quale palette
        if(CGAreg[9] & 16)      // high intensity foreground color...
					;
				// CGA 4 color https://www.usebox.net/jjm/notes/cga/
        for(py=rowIni; py<rowFin; py++) {    // 
					p1=(BYTE*)&CGAram[0] + (((py / 2))*80) + (py & 1 ? 8192 : 0);
          for(px=0; px<HORIZ_SIZE/8; px++) {         // 320 pixel (80byte) 
            ch=*p1++;
//            writedata16(cgaColors[color][ch >> 6]);
//            writedata16(cgaColors[color][(ch >> 2) & 3]);
						*pVideoRAM++=(cgaColors[color][ch >> 6] << 4) | cgaColors[color][((ch >> 6) & 3)];
						*pVideoRAM++=(cgaColors[color][(ch >> 4) & 3] << 4) | cgaColors[color][((ch >> 4) & 3)];
						*pVideoRAM++=(cgaColors[color][(ch >> 2) & 3] << 4) | cgaColors[color][((ch >> 2)) & 3];
						*pVideoRAM++=(cgaColors[color][(ch >> 0) & 3] << 4) | cgaColors[color][(ch ) & 3];


            }
          }
        if(rowFin>=VERT_SIZE-8) {
          color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
					pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//          setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
          for(py=0; py<VERT_OFFSCREEN; py++) {    // 
            for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
							*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
              }
            }
          }
        }
      else {                  // hires graph (640x200x1 e 160x120x4)
	      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE... VERIFICARE 2024 (non c'era...
//        (CGAreg[9] & 0xf)      // colori per overscan/background/hires color
        if(!(CGAreg[8] & 16)) {     // modo 160x200 16 colori   
          if(rowIni==0) {
					pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//            setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		          for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
                }
              }
            }
					pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
//          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
		      for(py=rowIni; py<rowFin; py++) {    // 
						p1=(BYTE*)&CGAram[0] + (((py / 2))*80) + (py & 1 ? 8192 : 0);
            for(px=0; px<HORIZ_SIZE/8; px++) {         // 160 pixel (80byte) 
              ch=*p1++;
//              writedata16x2(textColors[ch >> 4],textColors[ch & 0xf]);
//              writedata16x2(textColors[ch >> 4],textColors[ch & 0xf]);    // 160 -> 320
							*pVideoRAM++=(ch & 0xf0) | (ch & 0xf);		// :)
							*pVideoRAM++=(ch & 0xf0) | (ch & 0xf);
							*pVideoRAM++=(ch & 0xf0) | (ch & 0xf);
							*pVideoRAM++=(ch & 0xf0) | (ch & 0xf);
              }
            }
          if(rowFin>=VERT_SIZE-8) {
            color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
						pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//            setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		          for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
                }
              }
            }
          }
        else {
          if(CGAreg[9] & 4)      // disable color ovvero palette #3 QUA NON SI CAPISCE COSA DOVREBBE FARE!
            color=15;
          else
            color=CGAreg[9] & 15;
          if(rowIni==0) {
						pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//            setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		          for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
                }
              }
            }
					pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
//          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
	        for(py=rowIni; py<rowFin; py++) {    // 
						p1=(BYTE*)&CGAram[0] + (((py / 2))*(HORIZ_SIZE/8)) + (py & 1 ? 8192 : 0);
            for(px=0; px<HORIZ_SIZE/8; px++) {      // 640 pixel (80byte) 
              ch=*p1++;
              if(ch & 0x80)
//                writedata16(textColors[color]);
								*pVideoRAM=color << 4;
              else
//                writedata16(textColors[0]);
								*pVideoRAM=0 << 4;
              if(ch & 0x40)
//                writedata16(textColors[color]);
								*pVideoRAM++ |= color;
              else
//                writedata16(textColors[0]);
								*pVideoRAM++ = 0;
              if(ch & 0x20)
//                writedata16(textColors[color]);
								*pVideoRAM = color << 4;
              else
//                writedata16(textColors[0]);
								*pVideoRAM = 0 << 4;
              if(ch & 0x10)
//                writedata16(textColors[color]);
								*pVideoRAM++ |= color;
              else
//                writedata16(textColors[0]);
								*pVideoRAM++ |= 0;
              if(ch & 0x8)
								*pVideoRAM=color << 4;
              else
								*pVideoRAM=0 << 4;
              if(ch & 0x4)
								*pVideoRAM++ |= color;
              else
								*pVideoRAM++ = 0;
              if(ch & 0x2)
								*pVideoRAM = color << 4;
              else
								*pVideoRAM = 0 << 4;
              if(ch & 0x1)
								*pVideoRAM++ |= color;
              else
								*pVideoRAM++ |= 0;
              }
            }
          if(rowFin>=VERT_SIZE-8) {
            color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
					pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//            setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
            for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		          for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
                }
              }
            }
          }
        }
      }
    else {                // text mode (da qualche parte potrebbe esserci la Pagina selezionata...
      color=CGAreg[9] & 15;   // questo qua diventa il bordo! USARE...
      if(rowIni==0) {
				pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//        setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
						}
					}
				}
//      setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
			pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
      for(py=rowIni/8; py<rowFin/8; py++) {    // 
        if(CGAreg[8] & 1) {     // 80x25
          for(i=0; i<8; i++) {         // 8 righe 
						p1=(BYTE*)&CGAram[0] + (py*80*2)   + 2*MAKEWORD(i6845RegW[13],i6845RegW[12]) /* display start addr*/;    // char/colore
//#warning            era 80?? 2024 ah ma tanto rowini è 0...
            for(px=0; px<HORIZ_SIZE/8; px++) {         // 80 char (80byte) diventano 640 pixel
              ch=*p1++;
              p=(BYTE *)&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x80)   // (difficile scegliere quali 2 pixel prendere.. !
//                writedata16(textColors[color & 0xf]);
								*pVideoRAM=(color & 0xf) << 4;
              else
//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
								*pVideoRAM=(color & 0xf0);
              if(ch & 0x40)   // 
//                writedata16(textColors[color & 0xf]);
								*pVideoRAM++ |= (color & 0xf);
              else
//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
								*pVideoRAM++ |= (color & 0xf0) >> 4;
              if(ch & 0x20)
//                writedata16(textColors[color & 0xf]);
								*pVideoRAM=(color & 0xf) << 4;
              else
//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
								*pVideoRAM=(color & 0xf0);
              if(ch & 0x10)   // 
//                writedata16(textColors[color & 0xf]);
								*pVideoRAM++ |= (color & 0xf);
              else
//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
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
          for(i=0; i<8; i++) {         // 8 righe 
						p1=(BYTE*)&CGAram[0] + (py*40*2)   + 2*MAKEWORD(i6845RegW[13],i6845RegW[12]) /* display start addr*/;    // char/colore
            for(px=0; px<40; px++) {         // 40 char (40byte) diventano 640 pixel
              ch=*p1++;
              p=(BYTE *)&CGAfont[((WORD)ch)*8+i];
              ch=*p;
              color=*p1++;   // il colore segue il char
              if(ch & 0x80)   // 8 pixel
//                writedata16(textColors[color & 0xf]);
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
								*pVideoRAM++= (color & 0xf0) | (color >> 4);
              if(ch & 0x40)
//                writedata16(textColors[color & 0xf]);
								*pVideoRAM++= ((color & 0xf) << 4) | (color & 0xf);
              else
//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
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
//        setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
            }
          }
				blinkState = !blinkState;		// rallentare!
        }

      i=MAKEWORD(i6845RegW[15],i6845RegW[14] & 0x3f);    // coord cursore, abs
      row1=CGAreg[8] & 1 ? (i/80)*8 : (i/40)*8;
      if(row1>=rowIni && row1<rowFin) {
        BYTE cy1,cy2;
				row1--;			// è la pos della prima riga in alto del char contenente il cursore
        switch((i6845RegW[10] & 0x60)) {
          case 0:
					// secondo GLABios, in CGA il cursore lampeggia sempre indipendentemente da questo valore... me ne frego!!
plot_cursor:
  // test          i6845RegW[10]=5;i6845RegW[11]=7;
            cy1= (i6845RegW[10] & /*0x1f*/ 7);    // 0..32 ma in effetti sono "reali" per cui di solito 6..7!
            cy2= (i6845RegW[11] & /*0x1f*/ 7);
            if(cy2 && cy1<=cy2) do {
              if(CGAreg[8] & 1) {     // 80x25
                color=7;    // fisso :)
								pVideoRAM=(BYTE*)&VideoRAM[0]+((row1)+cy1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+((i % 80)*4)+(HORIZ_OFFSCREEN/2);
					//			pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
//                setAddrWindow((i % 80)*4,1+ /* ?? boh*/ cy1+ (i/80)*8 +VERT_OFFSCREEN,4,cy2-cy1);   // altezza e posizione fissa, gestire da i6845RegW[10,11]
//                writedata16x2(textColors[color],textColors[color]);
//                writedata16x2(textColors[color],textColors[color]);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
                }
              else {
                color=7;    // fisso :)
								pVideoRAM=(BYTE*)&VideoRAM[0]+((row1)+cy1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+((i % 40)*8)+(HORIZ_OFFSCREEN/2);
//                setAddrWindow((i % 40)*8,1+ /* ?? boh*/ cy1+ (i/40)*8 +VERT_OFFSCREEN,8,cy2-cy1);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
								*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
//                writedata16x2(textColors[color],textColors[color]);
//                writedata16x2(textColors[color],textColors[color]);
//                writedata16x2(textColors[color],textColors[color]);
//                writedata16x2(textColors[color],textColors[color]);
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

#ifdef MDA
  rowFin=min(rowFin,VERT_SIZE);
  if(rowIni==0)
    MDAreg[0xa] &= ~B8(00001000); // MDA not in retrace V
  else if(rowFin>=VERT_SIZE-8)
    MDAreg[0xa] |= B8(00001000); // MDA in retrace V
  
  if(MDAreg[8] & 8) {     // enable video
    if(MDAreg[8] & 2) {     // graphic high resolution mode
// qua?		  color=MDAreg[9] & 15;   // questo qua diventa il bordo! USARE... VERIFICARE 2024 (non c'era...
//        (MDAreg[9] & 0xf)      // colori per overscan/background/hires color
    
      color=3;		// finire intensità
			color2=0;


      if(rowIni==0) {
				pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//            setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
            }
          }
        }
			pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
//          setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
      for(py=rowIni; py<rowFin; py++) {    // 
				p1=(BYTE*)&CGAram[MDAreg[8] & 0x80 ? 0x8000 : 0] + ((py & 3) * 0x2000) + ((py/4)*(HORIZ_SIZE/8)) ;
				// qua??   OCCHIO CHE PRIMA CI SON TUTTE LE RIGHE PARI E POI LE DISPARI!

        for(px=0; px<HORIZ_SIZE/8; px++) {      // 720 pixel (90byte) 
          ch=*p1++;
          if(ch & 0x80)
//                writedata16(textColors[color]);
						*pVideoRAM=color << 4;
          else
//                writedata16(textColors[0]);
						*pVideoRAM=color2 << 4;
          if(ch & 0x40)
//                writedata16(textColors[color]);
						*pVideoRAM++ |= color;
          else
//                writedata16(textColors[0]);
						*pVideoRAM++ = color2;
          if(ch & 0x20)
//                writedata16(textColors[color]);
						*pVideoRAM = color << 4;
          else
//                writedata16(textColors[0]);
						*pVideoRAM = color2 << 4;
          if(ch & 0x10)
//                writedata16(textColors[color]);
						*pVideoRAM++ |= color;
          else
//                writedata16(textColors[0]);
						*pVideoRAM++ |= color2;
          if(ch & 0x8)
						*pVideoRAM=color << 4;
          else
						*pVideoRAM=color2 << 4;
          if(ch & 0x4)
						*pVideoRAM++ |= color;
          else
						*pVideoRAM++ = color2;
          if(ch & 0x2)
						*pVideoRAM = color << 4;
          else
						*pVideoRAM = color2 << 4;
          if(ch & 0x1)
						*pVideoRAM++ |= color;
          else
						*pVideoRAM++ |= color2;
          }
        }
      if(rowFin>=VERT_SIZE-8) {
        color=MDAreg[9] & 15;   // questo qua diventa il bordo! USARE...
				pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//            setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
            }
          }
        }
      }
    else {                // text mode
      if(rowIni==0) {
				pVideoRAM=(BYTE*)&VideoRAM[0]+HORIZ_OFFSCREEN/2;
//        setAddrWindow(HORIZ_OFFSCREEN+0,0,_width-0,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
						}
					}
				}
//      setAddrWindow(HORIZ_OFFSCREEN+0,rowIni +VERT_OFFSCREEN,_width-0,(rowFin-rowIni));
			pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
      for(py=rowIni/14; py<rowFin/14; py++) {    // 
        for(i=0; i<14; i++) {         // 14 righe 
						p1=(BYTE*)&CGAram[0] + (py*80*2)   + 2*MAKEWORD(i6845RegW[13],i6845RegW[12]) /* display start addr*/;    // char/colore
	//#warning            era 80?? 2024 ah ma tanto rowini è 0...
						for(px=0; px<80; px++) {         // 80 char (80byte) diventano 720 pixel
							ch=*p1++;
							if(i<8) 
								p=(BYTE *)&MDAfont[((WORD)ch)*8+i];			// i caratteri sono rappresentati metà prima e metà dopo...
							else
								p=(BYTE *)&MDAfont[((WORD)ch)*8+i+2048-8];
							ch=*p;
							ch2=*p1++;
							switch(ch2) {		// il colore/attributi segue il char
								case 0:
								case 8:
								case 0x80:
								case 0x88:		//inverted https://www.seasip.info/VintagePC/mda.html#memmap
									color=0;		// black on black
									color2=0;
									break;
								case 0x70:
									color=0;		// black on green
									color2=2;
									break;
								case 0x78:
									color=1;		// dark green on green
									color2=2;
									break;
								default:
									color2=0;
									if(ch2 & 0x80) {		// blink
										if(blinkState)
											color=0;
										}
									else {
										if(ch2 & 8)
											color=3;
										else
											color=2;		// (finire intensità) underline blink
										if((ch2 & 7) == 1) {			// underline
											if(i==14)
												color=color2;
											}
										}
									break;
								}
							

							if(!(px & 1)) {			// 8 -> 9 pixel width...
								if(ch & 0x80)   // (difficile scegliere quali 2 pixel prendere.. !
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM=color << 4;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM=color2 << 4;
								if(ch & 0x40)   // 
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM++ |= color;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM++ |= color2;
								if(ch & 0x20)
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM=color << 4;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM=color2 << 4;
								if(ch & 0x10)   // 
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM++ |= color;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM++ |= color2;
								if(ch & 0x08)
									*pVideoRAM=color << 4;
								else
									*pVideoRAM=color2 << 4;
								if(ch & 0x4)   // 
									*pVideoRAM++ |= color;
								else
									*pVideoRAM++ |= color2;
								if(ch & 0x2) 
									*pVideoRAM=color << 4;
								else
									*pVideoRAM=color2 << 4;
								if(ch & 0x1)   // 
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM++ |= color;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM++ |= color2;
								*pVideoRAM = color2  << 4;			//For characters 0C0h-0DFh, the ninth pixel column is a duplicate of the eighth; for others, it's blank.
								}
							else {
								if(ch & 0x80)   // (difficile scegliere quali 2 pixel prendere.. !
									*pVideoRAM++ |= color;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM++ |= color2;
								if(ch & 0x40)   // 
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM = color << 4;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM = color2 << 4;
								if(ch & 0x20)
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM++ |= color;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM++ |= color2;
								if(ch & 0x10)   // 
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM = color << 4;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM = color2 << 4;
								if(ch & 0x08)
									*pVideoRAM++ |= color;
								else
									*pVideoRAM++ |= color2;
								if(ch & 0x4)   // 
									*pVideoRAM = color << 4;
								else
									*pVideoRAM = color2 << 4;
								if(ch & 0x2) 
									*pVideoRAM++ |= color;
								else
									*pVideoRAM++ |= color2;
								if(ch & 0x1)   // 
		//                writedata16(textColors[color & 0xf]);
									*pVideoRAM = color << 4;
								else
		//                writedata16(textColors[color >> 4]);    // GESTIRE BLINK!
									*pVideoRAM = color2 << 4;
								*pVideoRAM++ |= color2;		// For characters 0C0h-0DFh, the ninth pixel column is a duplicate of the eighth; for others, it's blank.
								}

							}
					}
        }

      if(rowFin>=VERT_SIZE-8) {
// qua??        color=MDAreg[9] & 15;   // questo qua diventa il bordo! USARE...
				pVideoRAM=(BYTE*)&VideoRAM[0]+(VERT_SIZE+VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
//        setAddrWindow(HORIZ_OFFSCREEN+0,VERT_SIZE+VERT_OFFSCREEN,_width-0,VERT_OFFSCREEN);
        for(py=0; py<VERT_OFFSCREEN; py++) {    // 
		      for(px=0; px<HORIZ_SIZE/4; px++) {         // 320 pixel
//                writedata16x2(textColors[color & 0xf],textColors[color & 0xf]);
						*pVideoRAM++=((color & 0xf) << 4) | (color & 0xf);
            }
          }
				blinkState = !blinkState;		// rallentare!
        }

      i=MAKEWORD(i6845RegW[15],i6845RegW[14] & 0x3f);    // coord cursore, abs
      row1=(i/80)*8*14/8;
      if(row1>=rowIni && row1<rowFin) {
        BYTE cy1,cy2;
				row1--;			// è la pos della prima riga in alto del char contenente il cursore
        switch((i6845RegW[10] & 0x60)) {
          case 0:
plot_cursor:
  // test          i6845RegW[10]=5;i6845RegW[11]=7;
            cy1= (i6845RegW[10] & /*0x1f*/ 15);    // 0..32 ma in effetti sono "reali" per cui di solito 6..7!
            cy2= (i6845RegW[11] & /*0x1f*/ 15);
            if(cy2 && cy1<=cy2) do {
              color=3;    // fisso :)
							pVideoRAM=(BYTE*)&VideoRAM[0]+((row1)+cy1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+(((i % 80)*9)/2)+(HORIZ_OFFSCREEN/2);
				//			pVideoRAM=(BYTE*)&VideoRAM[0]+(rowIni +VERT_OFFSCREEN)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN)))+HORIZ_OFFSCREEN/2;
//                setAddrWindow((i % 80)*4,1+ /* ?? boh*/ cy1+ (i/80)*8 +VERT_OFFSCREEN,4,cy2-cy1);   // altezza e posizione fissa, gestire da i6845RegW[10,11]
//                writedata16x2(textColors[color],textColors[color]);
//                writedata16x2(textColors[color],textColors[color]);
							*pVideoRAM++=((3) << 4) | (3);
							*pVideoRAM++=((3) << 4) | (3);
							*pVideoRAM++=((3) << 4) | (3);
							*pVideoRAM++=((3) << 4) | (3);
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

	i=StretchDIBits(hDC,0,(rowIni*AppYSizeR)/(VERT_SIZE),doppiaDim ? AppXSize*2 : AppXSize,
		doppiaDim ? ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE/2) : ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE),
		0,rowFin,HORIZ_SIZE+HORIZ_OFFSCREEN*2,rowIni-rowFin,
		VideoRAM,bmI,DIB_RGB_COLORS,SRCCOPY);
	}


VOID CALLBACK myTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime) {
	int i;
	BYTE n;
	HDC hDC;
	static BYTE divider;
  static BYTE dividerTim,dividerVICpatch;

  divider++;
//  if(divider>=32) {   // 50 Hz per TOD  qua siamo a 50 Hz :)
//    divider=0;
//    }

  if(divider>=50) {   // ergo 1Hz   ma sembra MOLTO più veloce, 20/7/2025, verificare
    divider=0;
    if(!(i146818RAM[11] & B8(10000000))) {    // SET
//#warning VERIFICARE se il bios ABILITA RTC (sì con GLATICK)!! e occhio PCXTbios che usa un diverso RTC...
      i146818RAM[10] |= B8(10000000);
			if(!(i146818RAM[11] & B8(00000100))) 			// BCD mode
				currentDateTime.tm_sec=from_bcd(currentDateTime.tm_sec);
      currentDateTime.tm_sec++;
			n=currentDateTime.tm_sec;
			if(!(i146818RAM[11] & B8(00000100))) 	
				currentDateTime.tm_sec=to_bcd(currentDateTime.tm_sec);
      if(n >= 60) {
        currentDateTime.tm_sec=0;
				if(!(i146818RAM[11] & B8(00000100))) 			// BCD mode
					currentDateTime.tm_min=from_bcd(currentDateTime.tm_min);
        currentDateTime.tm_min++;
				n=currentDateTime.tm_min;
				if(!(i146818RAM[11] & B8(00000100))) 	
					currentDateTime.tm_min=to_bcd(currentDateTime.tm_min);
        if(n >= 60) {
          currentDateTime.tm_min=0;
					if(!(i146818RAM[11] & B8(00000100))) 			// BCD mode
						currentDateTime.tm_hour=from_bcd(currentDateTime.tm_hour);
          currentDateTime.tm_hour++;
					n=currentDateTime.tm_hour;
					if(!(i146818RAM[11] & B8(00000100)))
						currentDateTime.tm_hour=to_bcd(currentDateTime.tm_hour);
          if( ((i146818RAM[11] & B8(00000010)) && n >= 24) || 
            (!(i146818RAM[11] & B8(00000010)) && n >= 12) ) {
            currentDateTime.tm_hour=0;
						if(!(i146818RAM[11] & B8(00000100))) 			// BCD mode
							currentDateTime.tm_mday=from_bcd(currentDateTime.tm_mday);
            currentDateTime.tm_mday++;
						n=currentDateTime.tm_mday;
						if(!(i146818RAM[11] & B8(00000100)))
							currentDateTime.tm_mday=to_bcd(currentDateTime.tm_mday);
            i=dayOfMonth[currentDateTime.tm_mon-1];
            if((i==28) && !(currentDateTime.tm_year % 4))
              i++;
            if(n > i) {		// (rimangono i secoli... GLATick li mette nella RAM[0x32] del RTC)
              currentDateTime.tm_mday=0;
							if(!(i146818RAM[11] & B8(00000100))) 			// BCD mode
								currentDateTime.tm_mon=from_bcd(currentDateTime.tm_mon);
              currentDateTime.tm_mon++;
							if(!(i146818RAM[11] & B8(00000100)))
								currentDateTime.tm_mon=to_bcd(currentDateTime.tm_mon);
              if(n > 12) {		// 
                currentDateTime.tm_mon=1;
								if(!(i146818RAM[11] & B8(00000100))) 			// BCD mode
									currentDateTime.tm_year=from_bcd(currentDateTime.tm_year);
                currentDateTime.tm_year++;
								n=currentDateTime.tm_year;
								if(!(i146818RAM[11] & B8(00000100)))
									currentDateTime.tm_year=to_bcd(currentDateTime.tm_year);
								if(n>=100) {
	                currentDateTime.tm_year=0;
									i146818RAM[0x32]++; // vabbe' :)
									}
                }
              }
            }
          }
        } 
      i146818RAM[12] |= B8(10010000);
      i146818RAM[10] &= ~B8(10000000);
      } 
    else
      i146818RAM[10] &= ~B8(10000000);
    // inserire Alarm... :)
    i146818RAM[12] |= B8(01000000);     // in effetti dice che deve fare a 1024Hz! o forse è l'altro flag, bit3 ecc
    if(i146818RAM[12] & B8(01000000) && i146818RAM[11] & B8(01000000) ||
       i146818RAM[12] & B8(00100000) && i146818RAM[11] & B8(00100000) ||
       i146818RAM[12] & B8(00010000) && i146818RAM[11] & B8(00010000))     
      i146818RAM[12] |= B8(10000000);
    if(i146818RAM[12] & B8(10000000)) {
#ifdef EXT_80286			// solo PC/AT!
//        RTCIRQ=1;
       i8259IRR2 |= 1; 
	     if(!(i8259IMR & 2)) {		// poi forse andrebbe gestito il Cascaded...  http://www.osdever.net/tutorials/view/irqs
				}
#endif
			}
		}


//  dividerVICpatch++;
//  if(dividerVICpatch>=3) {    // 25mS per fascia [per ora faccio una passata unica, quindi + lento] (sono ~30 tutto compreso, 21/7/24
    dividerVICpatch=0;
    VIDIRQ=1;       // refresh screen in 200/8=25 passate, 50 volte al secondo FINIRE QUA!
//    }
// v.  CIA1RegR[0xe];


	}

VOID CALLBACK myTimerProc2(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime) {
	int i;
	static BYTE divider;

//  if((i8253Mode[0] & 0b00001110) == 0 MA LUI METTE 6 ossia mode 3??! )
    
/*  if(i8259RegW[1] == 0xbc &&      // PATCH finire gestione 8259, se no arrivano irq ad minchiam
      !(i8259RegW[1] & 1)) {
//      TIMIRQ=1;  //
    i8259RegR[0] |= 1;
    }*/
	}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId,wmEvent;
	PAINTSTRUCT ps;
	HDC hDC,hCompDC;
 	POINT pnt;
	HMENU hMenu;
	HRSRC hrsrc;
	HFILE myFile;
 	BOOL bGotHelp;
	int i,j,k,i1,j1,k1;
	long l;
	char myBuf[128];
	char *s,*s1;
	LOGBRUSH br;
	RECT rc;
	SIZE mySize;
	HFONT hOldFont;
	HPEN myPen,hOldPen;
	HBRUSH myBrush,hOldBrush;
	static int TimerCnt;
	HANDLE hROM;

	switch(message) { 
		case WM_COMMAND:
			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch(wmId) {
				case ID_APP_ABOUT:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_ABOUT), hWnd, (DLGPROC)About);
					break;

				case ID_OPZIONI_DUMP:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_DUMP), hWnd, (DLGPROC)Dump0);
					break;

				case ID_OPZIONI_DUMPDISP:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_DUMP1), hWnd, (DLGPROC)Dump1);
					break;

				case ID_OPZIONI_RESET:
   				CPUPins |= DoReset;
					oldMousePos.x=-1; oldMousePos.y=-1;
					InvalidateRect(hWnd,NULL,TRUE);
  				SetWindowText(hStatusWnd,"<reset>");


//			debug=1;



					break;

				case ID_OPZIONI_RESET2:
					PostMessage(hWnd,WM_KEYDOWN,VK_CONTROL,0);
					PostMessage(hWnd,WM_KEYDOWN,VK_MENU,0);
					PostMessage(hWnd,WM_KEYDOWN,VK_DELETE,0);
/*					decodeKBD(VK_CONTROL,0,0);
					decodeKBD(VK_MENU,0,0);
					decodeKBD(VK_DELETE,0,0);
					// ora funzia su parecchio BIOS, non GLABios ma ok IBM
					// Send non va (giustamente
					decodeKBD(VK_DELETE,0,1);
					decodeKBD(VK_MENU,0,1);
					decodeKBD(VK_CONTROL,0,1);*/
//					PostMessage(hWnd,WM_KEYUP,VK_DELETE,0);
//					PostMessage(hWnd,WM_KEYUP,VK_MENU,0);
//					PostMessage(hWnd,WM_KEYUP,VK_CONTROL,0);

					InvalidateRect(hWnd,NULL,TRUE);
					break;

				case ID_EMULATORE_CARTRIDGE:
					break;

				case ID_APP_EXIT:
					PostMessage(hWnd,WM_CLOSE,0,0l);
					break;

				case ID_FILE_NEW:
					break;


				case ID_FILE_OPEN:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Binari\0*.BIN\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if(GetOpenFileName(&ofn) == TRUE) {
						myFile=_lopen(ofn.lpstrFile /*"68kmemory.bin"*/,OF_READ);
						if(myFile != -1) {
							_lread(myFile,ram_seg,RAM_SIZE);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile caricare",
								szAppName,MB_OK|MB_ICONHAND);
						}
					}
					break;

				case ID_EMULATORE_CARICADISCO_A:
				case ID_EMULATORE_CARICADISCO_B:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros
					uint8_t disk=wmId==ID_EMULATORE_CARICADISCO_A ? 0 : 1;

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Image\0*.IMG\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.lpstrTitle = disk ? "Carica disco B" : "Carica disco A";
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if(GetOpenFileName(&ofn) == TRUE) {
						myFile=_lopen(ofn.lpstrFile,OF_READ);
						if(myFile != -1) {
							_lread(myFile,MSDOS_DISK[disk],
								sectorsPerTrack[disk]*totalTracks*512*2);
							_lclose(myFile);
							floppyState[disk] |= B8(10000000);		// present
							floppyState[disk] |= B8(00000010);		// changed, gestire! solo PCAT??
							// QB45 install non vede il disco cambiato...
							}
						else
							MessageBox(hWnd,"Impossibile caricare",
								szAppName,MB_OK|MB_ICONHAND);
						}
					}
					break;

				case ID_EMULATORE_CARICADISCO_C:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Image\0*.IMG\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.lpstrTitle = "Carica hard disc";
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if(GetOpenFileName(&ofn) == TRUE) {
						myFile=_lopen(ofn.lpstrFile,OF_READ);
						if(myFile != -1) {
							_lread(myFile,MSDOS_HDISK,
								10*1048576L);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile caricare",
								szAppName,MB_OK|MB_ICONHAND);
						}
					}
					break;

				case ID_FILE_SAVE_AS:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Binari\0*.BIN\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST;

					if(GetSaveFileName(&ofn) == TRUE) {
							// use ofn.lpstrFile
						myFile=_lcreat(ofn.lpstrFile /*"68kmemory.bin"*/,0);
						if(myFile != -1) {
							_lwrite(myFile,ram_seg,RAM_SIZE);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile salvare",szAppName,MB_OK|MB_ICONHAND);
						}

					}
					break;

				case ID_EMULATORE_SALVADISCO_A:
				case ID_EMULATORE_SALVADISCO_B:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros
					uint8_t disk=wmId==ID_EMULATORE_SALVADISCO_A ? 0 : 1;

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Image\0*.IMG\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.lpstrTitle = disk ? "Salva disco B" : "Salva disco A";
					ofn.Flags = OFN_PATHMUSTEXIST;

					if(GetSaveFileName(&ofn) == TRUE) {
						myFile=_lcreat(ofn.lpstrFile,0);
						if(myFile != -1) {
							_lwrite(myFile,MSDOS_DISK[disk],
								sectorsPerTrack[disk]*totalTracks*512*2);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile salvare",szAppName,MB_OK|MB_ICONHAND);
						}

					}
					break;

				case ID_EMULATORE_SALVADISCO_C:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Image\0*.IMG\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.lpstrTitle = "Salva disco C";
					ofn.Flags = OFN_PATHMUSTEXIST;

					if(GetSaveFileName(&ofn) == TRUE) {
						myFile=_lcreat(ofn.lpstrFile,0);
						if(myFile != -1) {
							_lwrite(myFile,MSDOS_HDISK,
								10*1048576L);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile salvare",szAppName,MB_OK|MB_ICONHAND);
						}

					}
					break;

				case ID_OPZIONI_DEBUG:
					debug=!debug;
					break;

				case ID_OPZIONI_TRACE:
//					_f.Trap =!_f.Trap;
					break;

				case ID_OPZIONI_KEYSTROKE:
					{static BYTE state=0;
					if(!state)
						decodeKBD('1',0,0);
					else
						decodeKBD('1',0,1);
					state ^= 1;
					}
					break;


				case ID_OPZIONI_DIMENSIONEDOPPIA:
					doppiaDim=!doppiaDim;
					break;

				case ID_OPZIONI_AGGIORNA:
					InvalidateRect(hWnd,NULL,TRUE);
					break;

				case ID_OPZIONI_VELOCIT_LENTO:
					CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER/2);
					break;
				case ID_OPZIONI_VELOCIT_NORMALE:
					CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER);
					break;
				case ID_OPZIONI_VELOCIT_VELOCE:
					CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER*2);
					break;

				case ID_EDIT_PASTE:
					break;

        case ID_HELP: // Only called in Windows 95
          bGotHelp = WinHelp(hWnd, APPNAME".HLP", HELP_FINDER,(DWORD)0);
          if(!bGotHelp) {
            MessageBox(GetFocus(),"Unable to activate help",
              szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_INDEX: // Not called in Windows 95
          bGotHelp = WinHelp(hWnd, APPNAME".HLP", HELP_CONTENTS,(DWORD)0);
		      if(!bGotHelp) {
            MessageBox(GetFocus(),"Unable to activate help",
              szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_FINDER: // Not called in Windows 95
          if(!WinHelp(hWnd, APPNAME".HLP", HELP_PARTIALKEY,
				 		(DWORD)(LPSTR)"")) {
						MessageBox(GetFocus(),"Unable to activate help",
							szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_USING: // Not called in Windows 95
					if(!WinHelp(hWnd, (LPSTR)NULL, HELP_HELPONHELP, 0)) {
						MessageBox(GetFocus(),"Unable to activate help",
							szAppName, MB_OK|MB_ICONHAND);
					  }
					break;

				default:
					return (DefWindowProc(hWnd, message, wParam, lParam));
			}
			break;

		case WM_NCRBUTTONUP: // RightClick on windows non-client area...
			if (IS_WIN95 && SendMessage(hWnd, WM_NCHITTEST, 0, lParam) == HTSYSMENU) {
				// The user has clicked the right button on the applications
				// 'System Menu'. Here is where you would alter the default
				// system menu to reflect your application. Notice how the
				// explorer deals with this. For this app, we aren't doing anything
				return (DefWindowProc(hWnd, message, wParam, lParam));
			  }
			else {
				// Nothing we are interested in, allow default handling...
				return (DefWindowProc(hWnd, message, wParam, lParam));
			  }
      break;

		case WM_PAINT:
			hDC=BeginPaint(hWnd,&ps);
			myPen=CreatePen(PS_SOLID,16,Colori[0]);
			br.lbStyle=BS_SOLID;
			br.lbColor=Colori[0];

			br.lbHatch=0;
			myBrush=CreateBrushIndirect(&br);
			SelectObject(hDC,myPen);
			SelectObject(hDC,myBrush);
//			SelectObject(hDC,hFont);
//			Rectangle(hDC,0,0,200,200);
			Rectangle(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);

			UpdateScreen(hDC,VERT_OFFSCREEN,VERT_SIZE+VERT_OFFSCREEN*2);
			DeleteObject(myPen);
			DeleteObject(myBrush);
			EndPaint(hWnd,&ps);
			break;        

		case WM_TIMER:
			TimerCnt++;
			break;

		case WM_SIZE:
			GetClientRect(hWnd,&rc);
			AppXSize=rc.right-rc.left;
			AppYSize=rc.bottom-rc.top;
			AppYSizeR=rc.bottom-rc.top-(GetSystemMetrics(SM_CYSIZEFRAME)*2);
			MoveWindow(hStatusWnd,0,rc.bottom-16,rc.right,16,TRUE);
			break;        

		case WM_KEYDOWN:
			decodeKBD(wParam,lParam,0);
			break;        

		case WM_KEYUP:
			decodeKBD(wParam,lParam,1);
			break;        

		case WM_LBUTTONDOWN:
      mouseState |= B8(00100000);
			goto mouse_click;
			break;        

		case WM_LBUTTONUP:
      mouseState &= ~B8(00100000);  // 
			goto mouse_click;
			break;        

		case WM_RBUTTONDOWN:
      if(GetAsyncKeyState(VK_LMENU) & 0x8000 || GetAsyncKeyState(VK_RMENU) & 0x8000) { // RightClick in windows client area...
        pnt.x = LOWORD(lParam);
        pnt.y = HIWORD(lParam);
        ClientToScreen(hWnd, (LPPOINT)&pnt);
        hMenu = GetSubMenu(GetMenu(hWnd),2);
        if(hMenu) {
          TrackPopupMenu(hMenu, 0, pnt.x, pnt.y, 0, hWnd, NULL);
          }

				}
      else if(GetAsyncKeyState(VK_LCONTROL) & 0x8000) { // RightClick tanto per risistemare cursore!
				RECT rc;

				GetWindowRect(hWnd,&rc);
				SetCursorPos(rc.left+AppXSize/2,rc.top+AppYSize/2+GetSystemMetrics(SM_CYMENUSIZE)+GetSystemMetrics(SM_CYSIZE));
				oldMousePos.x=-1;
				oldMousePos.y=-1;
				mouseState |= B8(10000000);		// e semaforo (bug

				}
			else {
				mouseState |= B8(00010000);
				goto mouse_click;
				}
			break;        

		case WM_RBUTTONUP:
      mouseState &= ~B8(00010000);  // 

mouse_click:
			if(mouseState & B8(10000000)) {		// semaforo
#if MOUSE_TYPE==1
				COMDataEmuFifo[0]=B8(01000000) | (mouseState & B8(00110000));
				COMDataEmuFifo[1]=0;
				COMDataEmuFifo[2]=0;
				COMDataEmuFifoCnt=0;
#elif MOUSE_TYPE==2
				COMDataEmuFifo[0]=B8(10000000) | (mouseState & B8(00100000) ? 0 : B8(00000100))  | 
					(mouseState & B8(00010000) ? 0 : B8(00000001));
				COMDataEmuFifo[1]=0;
				COMDataEmuFifo[2]=0;
				COMDataEmuFifo[3]=0;
				COMDataEmuFifo[4]=0;
				COMDataEmuFifoCnt=0;
#endif

				i8250Reg[2] &= ~B8(00000001);
				if(i8250Reg[1] & B8(00000001))			// se IRQ attivo
					i8259IRR |= 0x10;
        i8250Reg[5] |= 1;			// byte rx in COM cmq

				mouseState &= ~B8(10000000);  // marker per COM
				}
			break;        

		case WM_MOUSEMOVE:
			{int32_t x,y;
			RECT rc;
			POINT mypos;

//			mousePos.x=(int)LOWORD(lParam);
//			mousePos.y=(int)HIWORD(lParam);

			GetCursorPos(&mousePos);
			if(oldMousePos.x != -1 && oldMousePos.y != -1) {
				x=mousePos.x-oldMousePos.x;
				if(x<-127)
					x=-127;
				if(x>127)
					x=127;
				if(mouseState & B8(10000000)) 		// semaforo
					mousePos.x=oldMousePos.x+x;
				y=mousePos.y-oldMousePos.y;
				if(y<-127)
					y=-127;
				if(y>127)
					y=127;
				if(mouseState & B8(10000000)) 		// semaforo
					mousePos.y=oldMousePos.y+y;
	//			GetWindowRect(hWnd,&rc);
				if(x || y)		// evito spostamenti troppo altri; evito loop rientro
					SetCursorPos(mousePos.x,mousePos.y);

				GetClientRect(hWnd,&rc);
				rc.right=GetSystemMetrics(SM_CXSCREEN);
				rc.bottom=GetSystemMetrics(SM_CYSCREEN);
	//			ScreenToClient(hWnd,&mousePos);
//				x=(x*AppXSize)/rc.right;//
//fanculo				y=(y*AppYSize)/rc.bottom;
				if(x) {
					if(abs(x)>1)
						x=x/2;//
					}
				if(y) {
					if(abs(y)>1)
						y=y/2;
					}

				if(mouseState & B8(10000000)) {		// semaforo
#if MOUSE_TYPE==1
					COMDataEmuFifo[0]=B8(01000000) | (mouseState & B8(00110000)) | (((int8_t)x >> 6) & 0x03) | (((int8_t)y >> 4) & 0x0c);
					COMDataEmuFifo[1]=(int8_t)x & 0x3f;
					COMDataEmuFifo[2]=(int8_t)y & 0x3f;
					COMDataEmuFifoCnt=0;
#elif MOUSE_TYPE==2
					COMDataEmuFifo[0]=B8(10000000) | (mouseState & B8(00100000) ? 0 : B8(00000100))  | 
						(mouseState & B8(00010000) ? B8(00000001) : 0);
					COMDataEmuFifo[1]=(int8_t)x;
					COMDataEmuFifo[2]=(int8_t)-y;		// dio che culattoni :D
					COMDataEmuFifo[3]=0;
					COMDataEmuFifo[4]=0;
					COMDataEmuFifoCnt=0;
#endif
					i8250Reg[2] &= ~B8(00000001);
					if(i8250Reg[1] & B8(00000001))			// se IRQ attivo
						i8259IRR |= 0x10;
					mouseState &= ~B8(10000000);  // marker per COM
					}
				}

			oldMousePos=mousePos;
			}
			break;        

		case WM_CREATE:

			QueryPerformanceFrequency(&pcFrequency);		// in KHz => 2800000 su PC
			CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER);	// su PC greggiod con 6 ho un clock 68000 ragionevolmente simile all'originale (7MHz
	

//			bInFront=GetPrivateProfileInt(APPNAME,"SempreInPrimoPiano",0,INIFile);

			bmI=GlobalAlloc(GPTR,sizeof(BITMAPINFO)+(sizeof(COLORREF)*16));
			bmI->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
#ifdef CGA
			bmI->bmiHeader.biBitCount=4;
			bmI->bmiHeader.biClrImportant=bmI->bmiHeader.biClrUsed=16;
#endif
#ifdef MDA
			bmI->bmiHeader.biBitCount=4;		// lascio cmq per intensità :)
			bmI->bmiHeader.biClrImportant=bmI->bmiHeader.biClrUsed=16;
#endif
#ifdef VGA
			bmI->bmiHeader.biBitCount=4;		// più facile
			bmI->bmiHeader.biClrImportant=bmI->bmiHeader.biClrUsed=16;
#endif
			bmI->bmiHeader.biCompression=BI_RGB;
			bmI->bmiHeader.biWidth=HORIZ_SIZE+HORIZ_OFFSCREEN*2;
			bmI->bmiHeader.biHeight=VERT_SIZE+VERT_OFFSCREEN*2;
			bmI->bmiHeader.biPlanes=1;
			bmI->bmiHeader.biXPelsPerMeter=bmI->bmiHeader.biYPelsPerMeter=0;
#if defined(MDA) || defined(CGA) || defined(VGA)
			for(i=0; i<bmI->bmiHeader.biClrUsed; i++) {
				bmI->bmiColors[i].rgbRed=GetRValue(Colori[i]);
				bmI->bmiColors[i].rgbGreen=GetGValue(Colori[i]);
				bmI->bmiColors[i].rgbBlue=GetBValue(Colori[i]);
				}
#endif

			hFont=CreateFont(12,6,0,0,FW_LIGHT,0,0,0,
				ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_MODERN, (LPSTR)"Courier New");
			hFont2=CreateFont(14,7,0,0,FW_LIGHT,0,0,0,
				ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS, (LPSTR)"Arial");
			GetClientRect(hWnd,&rc);
			hStatusWnd = CreateWindow("static","",
				WS_BORDER | SS_LEFT | WS_CHILD,
				0,rc.bottom-16,AppXSize-GetSystemMetrics(SM_CXVSCROLL)-2*GetSystemMetrics(SM_CXSIZEFRAME),16,
				hWnd,(HMENU)1001,g_hinst,NULL);
			ShowWindow(hStatusWnd, SW_SHOW);
			GetClientRect(hWnd,&rc);
			AppYSize=rc.bottom-rc.top;
			AppYSizeR=AppYSize-16;
			SendMessage(hStatusWnd,WM_SETFONT,(WPARAM)hFont2,0);

			GetWindowRect(hWnd,&rc);
			SetCursorPos(rc.left+AppXSize/2,rc.top+AppYSize/2+GetSystemMetrics(SM_CYMENUSIZE)+GetSystemMetrics(SM_CYSIZE));
			// x Windows, ma servirebbe quando il win parte...


			hPen1=CreatePen(PS_SOLID,1,RGB(255,255,255));
			br.lbStyle=BS_SOLID;
			br.lbColor=0x000000;
			br.lbHatch=0;
			hBrush=CreateBrushIndirect(&br);

//			time(&currentDateTime);  in initHW

//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_PCXTBIOS),"BINARY"))) {	// va
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_BIOSXT),"BINARY"))) {		// non va
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_BIOSTINY),"BINARY"))) {		// va, 40 col 80186
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GLABIOS_5160),"BINARY"))) {		// va
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GLABIOS_5160_4),"BINARY"))) {		// ora ok (va con DOS 2 ma non vede i floppy con 1 & 3, non fa il boot
				// occhio ST version :D 26/7

			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GLABIOS_TEST),"BINARY"))) {		// 
#ifdef PC_IBM5150
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_IBM_5150),"BINARY"))) {		// ora va 20/8/25 (vuole MDA... + HLT e23e verificare
#endif
#ifdef PC_IBM5160
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_IBM_5160),"BINARY"))) {		// va! 16/8/25, 301,601 al boot (HLT durante DMA test... OCCHIO 32 (+8) KB!
// dicono che "601 test does a 'recalibrate' (move heads to track 1) followed by a move to track 34.
#endif
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_IBM_5150_2),"BINARY"))) {		// ora va 20/8/25  (non va, beep continui dopo un po'
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GLABIOS_5150_8P),"BINARY"))) {		// (ora floppy non va di nuovo PD 20/8 (va, (con switch per 5150)
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GLABIOS_XT),"BINARY"))) {		// ora va, CGA e MDA (non va, credo machinesettings switch
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GLABIOS_u8088),"BINARY"))) {		// ora va (non va, credo machinesettings switch
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_BIOSAMI),"BINARY"))) {		// ora va! 18/8/25 ,no floppy hd ok (erano problemi 8259 (in MDA va [no boot, errore floppy]! (in CGA 40 col dà interrupt error system halted ANCHE MDA 2025...
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_AWARDBIOS),"BINARY"))) {		// va, errore 301 ma poi va
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_BIOSAMI87),"BINARY"))) {		// ora va! 18/8/25 ,no floppy hd ok (erano problemi 8259  (idem interrupt error system halted 
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_ERSOBIOS),"BINARY"))) {		// (DTK) va, floppy e kb error alla partenza
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_SERGIOAGBIOS),"BINARY"))) {		// (DTC MegaBIOS) va, errore #08 alla partenza (credo tastiera)
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_PHOENIXBIOS),"BINARY"))) {		// va, ma non vede i floppy o non fa il boot

				hROM=LoadResource(NULL,hrsrc);
				memcpy(bios_rom,hROM,ROM_SIZE);

				FreeResource(hrsrc);
#ifdef ROM_SIZE2
				if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GLATICK),"BINARY"))) {		// per RTC (piciu solito :D
					hROM=LoadResource(NULL,hrsrc);
					memcpy(bios_rom_opt,hROM,ROM_SIZE2);
					FreeResource(hrsrc);
					}
#endif
				}
			else
				MessageBox(NULL,"Impossibile caricare ROM BIOS",NULL,MB_OK);
#ifdef ROM_BASIC
#ifdef PC_IBM5160
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_IBM_5160_BASIC),"BINARY"))) {		// bah dicevano che son diversi...
#else
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_IBMBASIC),"BINARY"))) {
#endif
				hROM=LoadResource(NULL,hrsrc);
				memcpy(IBMBASIC,hROM,0x8000);
				FreeResource(hrsrc);
				}
			else
				MessageBox(NULL,"Impossibile caricare ROM BASIC",NULL,MB_OK);
#endif

#ifdef ROM_HD_BASE
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_IDEXTBIOS),"BINARY"))) {
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_HDXEBEC),"BINARY"))) {
				hROM=LoadResource(NULL,hrsrc);
				memcpy(HDbios,hROM,0x2000);
				FreeResource(hrsrc);
				}
			else
				MessageBox(NULL,"Impossibile caricare ROM HD",NULL,MB_OK);
			myFile=_lopen("harddisc.img",OF_READ);
			if(myFile != -1) {
				_lread(myFile,MSDOS_HDISK,10*1048576L);
				_lclose(myFile);
				}
			else
				MessageBox(NULL,"Impossibile caricare HDisk",NULL,MB_OK);
#endif

#ifdef VGA
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_VGABIOS_TRIDENT),"BINARY"))) {
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_VGABIOS_BOCHS),"BINARY"))) {
				hROM=LoadResource(NULL,hrsrc);
				memcpy(VGABios,hROM,0x8000);
				FreeResource(hrsrc);
				}
			else
				MessageBox(NULL,"Impossibile caricare ROM VGA",NULL,MB_OK);
#endif

			// MSDOS sources https://github.com/microsoft/MS-DOS/tree/main/v2.0/source
#if MS_DOS_VERSION==1
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS125),"BINARY"))) {		// va 3/2/25
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS11),"BINARY"))) {		// va 3/2/25
				HANDLE hDisk;
				hDisk=LoadResource(NULL,hrsrc);
				memcpy(MSDOS_DISK[0],hDisk,320*512*2);
				FreeResource(hrsrc);
#elif MS_DOS_VERSION==2
		//	if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS211),"BINARY"))) {	// non va 3/2/25
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS212),"BINARY"))) {	// non va 3/2/25
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS211_EPSON),"BINARY"))) {	// va 3/2/25
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_WIN10_1),"BINARY"))) {	// 
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS211_720),"BINARY"))) {	//	va idem 4/2/25
				HANDLE hDisk;
				hDisk=LoadResource(NULL,hrsrc);
				memcpy(MSDOS_DISK[0],hDisk,720*512*2);
				FreeResource(hrsrc);
#elif MS_DOS_VERSION==3
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS331),"BINARY"))) {		// va 3/2/25
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSDOS331_720),"BINARY"))) {		// va 10/2/25
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_FREEDOS),"BINARY"))) {		// non va 10/2/25, fa alcune cose ma non trova COMMAND.COM
//			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_FREEDOS720),"BINARY"))) {		// va 7/8/25
				HANDLE hDisk;
				hDisk=LoadResource(NULL,hrsrc);
				memcpy(MSDOS_DISK[0],hDisk,720*512*2);
				FreeResource(hrsrc);
#endif
				}
			else
				MessageBox(NULL,"Impossibile caricare Disk",NULL,MB_OK);
//			memset(MSDOS_DISK[0],0x0,1440*512*2);		// TEST boot from C!	 ma non va, si resetta in continuazione, o con F6 rimane piantato
			// l'unica è mettere floppyState[0]:b7 a 0! v.
			memset(MSDOS_DISK[1],0xF6,1440*512*2);

			spoolFile=_lcreat("spoolfile.txt",0);
			if(spoolFile==-1)
				MessageBox(NULL,"Impossibile creare spoolfile",NULL,MB_OK);

			/*{//extern const unsigned char IBMBASIC[32768];
			spoolFile=_lcreat("pcxtbios_gd.bin",0);
			_lwrite(spoolFile,bios_romgd,8192);
_lclose(spoolFile);
			}*/
/*			{
				char *myBuf=malloc(100);
//				memset(myBuf,0,100000);
			Disassemble(bios_rom+0x0000,spoolFile,myBuf,0x2000,0xf000e000,7);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			free(myBuf);
			}*/


#if DEBUG_TESTSUITE
			{char *buf;
			FILE *f;
			struct _stat fs;
			cJSON *theJSON,*j2,*jtop;
			char myBuf[256];
			extern union REGISTERS regs;
			extern union REGISTERS16 segs;
			extern uint16_t _ip;
			extern union REGISTRO_F _f;

			WORD opcode=6; BYTE opcode2;


			for(opcode=0x00; opcode<0x10; opcode++) {
//			for(opcode2=0; opcode2<8; opcode2++) {

//			wsprintf(myBuf,"%02x.%u.json",opcode,opcode2);
			wsprintf(myBuf,"%02x.json",opcode,opcode2);
			if(f=fopen(myBuf,"r")) {
				_fstat(f->_file,&fs);
				buf=malloc(fs.st_size);
				fread(buf,fs.st_size,1,f);
				theJSON=cJSON_Parse(buf);


				jtop=theJSON->child;
				j2=jtop;
				while(j2) {		// ne farà 10000!
					cJSON *j3,*j4;
					j3=j2->child;
					_lwrite(spoolFile,j3->valuestring,strlen(j3->valuestring));		// qua il mnemonico istruzione
					_lwrite(spoolFile,"\r\n",2);

					j3=j3->next;							// qua istr.
					j4=j3->child;			// qua i byte istr.
					while(j4) {

						sprintf(myBuf,"%02X ",(BYTE)j4->valuedouble);
						_lwrite(spoolFile,myBuf,strlen(myBuf));
						j4=j4->next;
						}
					_lwrite(spoolFile,"\r\n",2);

				j4=FindItem(theJSON,NULL,"initial");	// prova
				j4=j4->child;			// qua i valori registri iniziali

					j3=j3->next;							// "initial"
					j4=j3->child->child;			// qua i valori registri iniziali
					while(j4) {

						sprintf(myBuf,"%s: %d %04X\r\n",j4->string,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
						_lwrite(spoolFile,myBuf,strlen(myBuf));
						if(!stricmp(j4->string,"ax"))
							regs.r[0].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"bx"))
							regs.r[3].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"cx"))
							regs.r[1].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"dx"))
							regs.r[2].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"sp"))
							regs.r[4].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"bp"))
							regs.r[5].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"si"))
							regs.r[6].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"di"))
							regs.r[7].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"es"))
							segs.r[0].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"cs"))
							segs.r[1].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"ss"))
							segs.r[2].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"ds"))
							segs.r[3].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"ip"))
							_ip=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"flags"))
							_f.x=(WORD)j4->valuedouble;
						j4=j4->next;
						}

					j4=j3->child->next->child;			// qua i valori ram iniziali
					while(j4) {

						sprintf(myBuf,"%u (%08X): %d (%02X)\r\n",(DWORD)j4->child->valuedouble,(DWORD)j4->child->valuedouble,
							(DWORD)j4->child->next->valuedouble,(DWORD)j4->child->next->valuedouble);
						_lwrite(spoolFile,myBuf,strlen(myBuf));
//						PutValue((DWORD)j4->child->valuedouble,(BYTE)j4->child->next->valuedouble);
						ram_seg[(DWORD)j4->child->valuedouble]=(BYTE)j4->child->next->valuedouble;
						j4=j4->next;
						}

				singleStep();

					j3=j3->next;
					j4=j3->child->child;			// qua i valori registri finali
					while(j4) {

						sprintf(myBuf,"%s: %d %04X\r\n",j4->string,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
						_lwrite(spoolFile,myBuf,strlen(myBuf));
						if(!stricmp(j4->string,"ax")) {
							if(regs.r[0].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[0].x,regs.r[0].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"bx")) {
							if(regs.r[3].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[3].x,regs.r[3].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"cx")) {
							if(regs.r[1].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[1].x,regs.r[1].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"dx")) {
							if(regs.r[2].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[2].x,regs.r[2].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"sp")) {
							if(regs.r[4].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[4].x,regs.r[4].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"bp")) {
							if(regs.r[5].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[5].x,regs.r[5].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"si")) {
							if(regs.r[6].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[6].x,regs.r[6].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"di")) {
							if(regs.r[7].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs.r[7].x,regs.r[7].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"es")) {
							if(segs.r[0].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,segs.r[0].x,segs.r[0].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"cs")) {
							if(segs.r[1].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,segs.r[1].x,segs.r[1].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"ss")) {
							if(segs.r[2].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,segs.r[2].x,segs.r[2].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"ds")) {
							if(segs.r[3].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,segs.r[3].x,segs.r[3].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"ip")) {
							if(_ip != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,_ip,_ip,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"flags")) {
							if(_f.x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFFf%s: %u (%04X) != %u (%04X)\r\n",j4->string,_f.x,_f.x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						j4=j4->next;
						}

					j4=j3->child->next->child;			// qua i valori ram finali
					while(j4) {

						sprintf(myBuf,"%u (%08X): %d (%02X)\r\n",(DWORD)j4->child->valuedouble,(DWORD)j4->child->valuedouble,
							(BYTE)j4->child->next->valuedouble,(BYTE)j4->child->next->valuedouble);
						_lwrite(spoolFile,myBuf,strlen(myBuf));
						if(ram_seg[(DWORD)j4->child->valuedouble] != (BYTE)j4->child->next->valuedouble) {
							sprintf(myBuf,"DIFFRAM %u (%08X): %u (%02X) != %u (%02X)\r\n",(DWORD)j4->child->valuedouble,(DWORD)j4->child->valuedouble,
								ram_seg[(DWORD)j4->child->valuedouble],ram_seg[(DWORD)j4->child->valuedouble],
								(BYTE)j4->child->next->valuedouble,(BYTE)j4->child->next->valuedouble);
							_lwrite(spoolFile,myBuf,strlen(myBuf));
							}
						j4=j4->next;
						}

					_lwrite(spoolFile,"\r\n",2);
					j2=j2->next;
					}


				cJSON_Delete(theJSON);
				free(buf);
				fclose(f);
				}
						PlayResource(MAKEINTRESOURCE(IDR_WAVE_SPEAKER),FALSE);
//				}
				}
			}
#endif


//			hTimer=SetTimer(NULL,0,1000/32,myTimerProc);  // (basato su Raster
			// non usato... fa schifo! era per refresh...
			hTimer=SetTimer(NULL,0,1000/50,myTimerProc);  // 
//			hTimer2=SetTimer(NULL,0,1000/18,myTimerProc2);  // 55mS IRQ
			initHW();
			ColdReset=1;
			break;

		case WM_QUERYENDSESSION:
#ifndef _DEBUG
			if(MessageBox(hWnd,"La chiusura del programma cancellerà la memoria dell'8086. Continuare?",APPNAME,MB_OKCANCEL | MB_ICONSTOP | MB_DEFBUTTON2) == IDOK)
				return 1l;
			else 
				return 0l;
#else
			return 1;
#endif
			break;

		case WM_CLOSE:
esciprg:          
#ifndef _DEBUG
			if(MessageBox(hWnd,"La chiusura del programma cancellerà la memoria dell'8086. Continuare?",APPNAME,MB_OKCANCEL | MB_ICONSTOP | MB_DEFBUTTON2) == IDOK) {
			  DestroyWindow(hWnd);
			  }
#else
		  DestroyWindow(hWnd);
#endif
			return 0l;
			break;

		case WM_DESTROY:
//			WritePrivateProfileInt(APPNAME,"SempreInPrimoPiano",bInFront,INIFile);
			// Tell WinHelp we don't need it any more...
			KillTimer(hWnd,hTimer2);
			KillTimer(hWnd,hTimer);
			_lclose(spoolFile);
	    WinHelp(hWnd,APPNAME".HLP",HELP_QUIT,(DWORD)0);
			GlobalFree(bmI);
			DeleteObject(hBrush);
			DeleteObject(hPen1);
			DeleteObject(hFont);
			DeleteObject(hFont2);
			PostQuitMessage(0);
			break;

   	case WM_INITMENU:
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_DEBUG,MF_BYCOMMAND | (debug ? MF_CHECKED : MF_UNCHECKED));
//   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_TRACE,MF_BYCOMMAND | (_f.Trap ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_DIMENSIONEDOPPIA,MF_BYCOMMAND | (doppiaDim ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_VELOCIT_LENTO,MF_BYCOMMAND | (CPUDivider==pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER/2) ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_VELOCIT_NORMALE,MF_BYCOMMAND | (CPUDivider==pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER) ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_VELOCIT_VELOCE,MF_BYCOMMAND | (CPUDivider==pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER*2) ? MF_CHECKED : MF_UNCHECKED));
#ifdef ROM_BASIC
   	  EnableMenuItem((HMENU)wParam,ID_EMULATORE_CARTRIDGE,MF_BYCOMMAND | MF_ENABLED);
   	  CheckMenuItem((HMENU)wParam,ID_EMULATORE_CARTRIDGE,MF_BYCOMMAND | (IBMBASIC[0] != 0 ? MF_CHECKED : MF_UNCHECKED));
#else
   	  EnableMenuItem((HMENU)wParam,ID_EMULATORE_CARTRIDGE,MF_BYCOMMAND | MF_DISABLED);
#endif
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));
		}
	return (0);
	}



ATOM MyRegisterClass(CONST WNDCLASS *lpwc) {
	HANDLE  hMod;
	FARPROC proc;
	WNDCLASSEX wcex;

	hMod=GetModuleHandle("USER32");
	if(hMod != NULL) {

#if defined (UNICODE)
		proc = GetProcAddress (hMod, "RegisterClassExW");
#else
		proc = GetProcAddress (hMod, "RegisterClassExA");
#endif

		if(proc != NULL) {
			wcex.style         = lpwc->style;
			wcex.lpfnWndProc   = lpwc->lpfnWndProc;
			wcex.cbClsExtra    = lpwc->cbClsExtra;
			wcex.cbWndExtra    = lpwc->cbWndExtra;
			wcex.hInstance     = lpwc->hInstance;
			wcex.hIcon         = lpwc->hIcon;
			wcex.hCursor       = lpwc->hCursor;
			wcex.hbrBackground = lpwc->hbrBackground;
    	wcex.lpszMenuName  = lpwc->lpszMenuName;
			wcex.lpszClassName = lpwc->lpszClassName;

			// Added elements for Windows 95:
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.hIconSm = LoadIcon(wcex.hInstance, "SMALL");
			
			return (*proc)(&wcex);//return RegisterClassEx(&wcex);
		}
	}
return (RegisterClass(lpwc));
}


BOOL InitApplication(HINSTANCE hInstance) {
  WNDCLASS  wc;
  HWND      hwnd;

  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_APP32));
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(GetStockObject(BLACK_BRUSH));

        // Since Windows95 has a slightly different recommended
        // format for the 'Help' menu, lets put this in the alternate menu like this:
  if(IS_WIN95) {
		wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    } else {
	  wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    }
  wc.lpszClassName = szAppName;

  if(IS_WIN95) {
	  if(!MyRegisterClass(&wc))
			return 0;
    }
	else {
	  if(!RegisterClass(&wc))
	  	return 0;
    }


  }

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	int x,y;

#ifndef _DEBUG
		x=CW_USEDEFAULT; y=CW_USEDEFAULT;
#else		// rompe i coglioni in alto a sx :)
		x=50; y=250;
#endif
	
	g_hinst=hInstance;

	ghWnd = CreateWindow(szAppName, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_THICKFRAME,
		x,y,
		AppXSize+GetSystemMetrics(SM_CXSIZEFRAME)*2,AppYSize+GetSystemMetrics(SM_CYSIZEFRAME)*2+GetSystemMetrics(SM_CYMENUSIZE)+GetSystemMetrics(SM_CYSIZE)+18,
		NULL, NULL, hInstance, NULL);

	if(!ghWnd) {
		return (FALSE);
	  }

	ShowWindow(ghWnd, nCmdShow);
	UpdateWindow(ghWnd);

	return (TRUE);
  }

//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
// 		This version allows greater flexibility over the contents of the 'About' box,
// 		by pulling out values from the 'Version' resource.
//
//  MESSAGES:
//
//	WM_INITDIALOG - initialize dialog box
//	WM_COMMAND    - Input received
//
//
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static  HFONT hfontDlg;		// Font for dialog text
	static	HFONT hFinePrint;	// Font for 'fine print' in dialog
	DWORD   dwVerInfoSize;		// Size of version information block
	LPSTR   lpVersion;			// String pointer to 'version' text
	DWORD   dwVerHnd=0;			// An 'ignored' parameter, always '0'
	UINT    uVersionLen;
	WORD    wRootLen;
	BOOL    bRetCode;
	int     i;
	char    szFullPath[256];
	char    szResult[256];
	char    szGetName[256];
	DWORD	dwVersion;
	char	szVersion[40];
	DWORD	dwResult;

	switch (message) {
    case WM_INITDIALOG:
//			ShowWindow(hDlg, SW_HIDE);
			hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				VARIABLE_PITCH | FF_SWISS, "");
			hFinePrint = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				VARIABLE_PITCH | FF_SWISS, "");
//			CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
			GetModuleFileName(g_hinst, szFullPath, sizeof(szFullPath));

			// Now lets dive in and pull out the version information:
			dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
			if(dwVerInfoSize) {
				LPSTR   lpstrVffInfo;
				HANDLE  hMem;
				hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
				lpstrVffInfo  = GlobalLock(hMem);
				GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
				// The below 'hex' value looks a little confusing, but
				// essentially what it is, is the hexidecimal representation
				// of a couple different values that represent the language
				// and character set that we are wanting string values for.
				// 040904E4 is a very common one, because it means:
				//   US English, Windows MultiLingual characterset
				// Or to pull it all apart:
				// 04------        = SUBLANG_ENGLISH_USA
				// --09----        = LANG_ENGLISH
				// ----04E4 = 1252 = Codepage for Windows:Multilingual
				lstrcpy(szGetName, "\\StringFileInfo\\040904E4\\");	 
				wRootLen = lstrlen(szGetName); // Save this position
			
				// Set the title of the dialog:
				lstrcat (szGetName, "ProductName");
				bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
					(LPSTR)szGetName,
					(LPVOID)&lpVersion,
					(UINT *)&uVersionLen);
//				lstrcpy(szResult, "About ");
//				lstrcat(szResult, lpVersion);
//				SetWindowText (hDlg, szResult);

				// Walk through the dialog items that we want to replace:
				for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++) {
					GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
					szGetName[wRootLen] = (char)0;
					lstrcat (szGetName, szResult);
					uVersionLen   = 0;
					lpVersion     = NULL;
					bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
						(LPSTR)szGetName,
						(LPVOID)&lpVersion,
						(UINT *)&uVersionLen);

					if(bRetCode && uVersionLen && lpVersion) {
					// Replace dialog item text with version info
						lstrcpy(szResult, lpVersion);
						SetDlgItemText(hDlg, i, szResult);
					  }
					else {
						dwResult = GetLastError();
						wsprintf (szResult, "Error %lu", dwResult);
						SetDlgItemText (hDlg, i, szResult);
					  }
					SendMessage (GetDlgItem (hDlg, i), WM_SETFONT, 
						(UINT)((i==DLG_VERLAST)?hFinePrint:hfontDlg),TRUE);
				  } // for


				GlobalUnlock(hMem);
				GlobalFree(hMem);

			}
		else {
				// No version information available.
			} // if (dwVerInfoSize)

    SendMessage(GetDlgItem (hDlg, IDC_LABEL), WM_SETFONT,
			(WPARAM)hfontDlg,(LPARAM)TRUE);

			// We are  using GetVersion rather then GetVersionEx
			// because earlier versions of Windows NT and Win32s
			// didn't include GetVersionEx:
			dwVersion = GetVersion();

			if (dwVersion < 0x80000000) {
				// Windows NT
				wsprintf (szVersion, "Microsoft Windows NT %u.%u (Build: %u)",
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
          (DWORD)(HIWORD(dwVersion)) );
				}
			else
				if (LOBYTE(LOWORD(dwVersion))<4) {
					// Win32s
				wsprintf (szVersion, "Microsoft Win32s %u.%u (Build: %u)",
  				(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
					(DWORD)(HIWORD(dwVersion) & ~0x8000) );
				}
			else {
					// Windows 95
				wsprintf(szVersion,"Microsoft Windows 95 %u.%u",
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))) );
				}

			SetWindowText(GetDlgItem(hDlg, IDC_OSVERSION), szVersion);
//			SetWindowPos(hDlg,NULL,GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2,0,0,SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
//			ShowWindow(hDlg, SW_SHOW);
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK || wParam==IDCANCEL) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			else if(wParam==3) {
				MessageBox(hDlg,"Se trovate utile questo programma, mandate un contributo!!\nVia Rivalta 39 - 10141 Torino (Italia)\n[Dario Greggio]","ADPM Synthesis sas",MB_OK);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}


LRESULT CALLBACK Dump0(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	char myBuf[256],myBuf1[32];
	int i,j;

	switch (message) {
    case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),WM_SETFONT,(WPARAM)hFont,0);
			for(i=0; i<256; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",ram_seg[i+j]);
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}

LRESULT CALLBACK Dump1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	char myBuf[256],myBuf1[32];
	int i,j;

	switch (message) {
    case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST2),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST2),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),WM_SETFONT,(WPARAM)hFont,0);

			// sono del C64 :D cambiare...
			for(i=0xd000; i<0xd000+47; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",GetValue(0,i+j));
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			for(i=0xd400; i<=0xd41C; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",GetValue(0,i+j));
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST2),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			i=0xdc00;
			wsprintf(myBuf,"%04X: ",i);
			for(j=0; j<16; j++) {
				wsprintf(myBuf1,"%02X ",GetValue(0,i+j));
				strcat(myBuf,myBuf1);
			  }
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),LB_ADDSTRING,0,(LPARAM)myBuf);
			i=0xdd00;
			wsprintf(myBuf,"%04X: ",i);
			for(j=0; j<16; j++) {
				wsprintf(myBuf1,"%02X ",GetValue(0,i+j));
				strcat(myBuf,myBuf1);
			  }
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),LB_ADDSTRING,0,(LPARAM)myBuf);
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}



int WritePrivateProfileInt(char *s, char *s1, int n, char *f) {
  int i;
  char myBuf[16];
  
  wsprintf(myBuf,"%d",n);
  WritePrivateProfileString(s,s1,myBuf,f);
  }

int ShowMe() {
	int i;
	char buffer[16];

	buffer[0]='A'^ 0x17;
	buffer[1]='D'^ 0x17;
	buffer[2]='P'^ 0x17;
	buffer[3]='M'^ 0x17;
	buffer[4]='-'^ 0x17;
	buffer[5]='G'^ 0x17;
	buffer[6]='.'^ 0x17;
	buffer[7]='D'^ 0x17;
	buffer[8]='a'^ 0x17;
	buffer[9]='r'^ 0x17;
	buffer[10]=' '^ 0x17;
	buffer[11]='1'^ 0x17;
	buffer[12]='9' ^ 0x17;
	buffer[13]=0;
	for(i=0; i<13; i++) buffer[i]^=0x17;
	MessageBox(GetDesktopWindow(),buffer,APPNAME,MB_OK | MB_ICONEXCLAMATION);
	}


#ifdef VGA
void test_dump_vga_reg() {
	static BYTE oldVGAreg[16],oldVGAactlReg[32],oldVGAcrtcReg[32],oldVGAgraphReg[16],oldVGAseqReg[8];
	int i;
	char myBuf[256],myBuf2[8];

	if(memcmp(oldVGAreg,VGAreg,sizeof(oldVGAreg))) {
		wsprintf(myBuf,"%u VGAreg: ",timeGetTime());
		for(i=0; i<sizeof(oldVGAreg); i++) {
			wsprintf(myBuf2,"%02X ",VGAreg[i]);
			strcat(myBuf,myBuf2);
			}
		strcat(myBuf,"\r\n");
		_lwrite(spoolFile,myBuf,strlen(myBuf));
		memcpy(oldVGAreg,VGAreg,sizeof(oldVGAreg));

		}
	if(memcmp(oldVGAactlReg,VGAactlReg,sizeof(oldVGAactlReg))) {
		wsprintf(myBuf,"%u VGAACTLreg: ",timeGetTime());
		for(i=0; i<22 /*sizeof(oldVGAactlReg)*/; i++) {			// 0x14 registri
			wsprintf(myBuf2,"%02X ",VGAactlReg[i]);
			strcat(myBuf,myBuf2);
			}
		strcat(myBuf,"\r\n");
		_lwrite(spoolFile,myBuf,strlen(myBuf));
		memcpy(oldVGAactlReg,VGAactlReg,sizeof(oldVGAactlReg));

		}
	if(memcmp(oldVGAcrtcReg,VGAcrtcReg,14 /*sizeof(oldVGAcrtcReg)*/)) {			// poi c'è il cursore e rompe
		wsprintf(myBuf,"%u VGACRTCreg: ",timeGetTime());
		for(i=0; i<25 /*sizeof(oldVGAcrtcReg)*/; i++) {			// dice 24 ma trovo a FF al 25
			wsprintf(myBuf2,"%02X ",VGAcrtcReg[i]);
			strcat(myBuf,myBuf2);
			}
		strcat(myBuf,"\r\n");
		_lwrite(spoolFile,myBuf,strlen(myBuf));
		memcpy(oldVGAcrtcReg,VGAcrtcReg,sizeof(oldVGAcrtcReg));

		}
	if(memcmp(oldVGAgraphReg,VGAgraphReg,sizeof(oldVGAgraphReg))) {
		wsprintf(myBuf,"%u VGAGRAPHreg: ",timeGetTime());
		for(i=0; i<9 /*sizeof(oldVGAgraphReg)*/; i++) {		// 9 registri
			wsprintf(myBuf2,"%02X ",VGAgraphReg[i]);
			strcat(myBuf,myBuf2);
			}
		strcat(myBuf,"\r\n");
		_lwrite(spoolFile,myBuf,strlen(myBuf));
		memcpy(oldVGAgraphReg,VGAgraphReg,sizeof(oldVGAgraphReg));

		}
	if(memcmp(oldVGAseqReg,VGAseqReg,sizeof(oldVGAseqReg))) {
		wsprintf(myBuf,"%u VGASEQreg: ",timeGetTime());
		for(i=0; i<5 /*sizeof(oldVGAseqReg)*/; i++) {		// 5 registri
			wsprintf(myBuf2,"%02X ",VGAseqReg[i]);
			strcat(myBuf,myBuf2);
			}
		strcat(myBuf,"\r\n");
		_lwrite(spoolFile,myBuf,strlen(myBuf));
		memcpy(oldVGAseqReg,VGAseqReg,sizeof(oldVGAseqReg));

		}
	}
#endif
