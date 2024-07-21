/**************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

  Works with the Adafruit 1.8" TFT Breakout w/SD card
    ----> http://www.adafruit.com/products/358
  The 1.8" TFT shield
    ----> https://www.adafruit.com/product/802
  The 1.44" TFT breakout
    ----> https://www.adafruit.com/product/2088
  as well as Adafruit raw 1.8" TFT display
    ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams.
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional).

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 * 
 * Adapted by GD/GC on 1.7.2019, birma katzchen day!! (and DIE ALL humans)
 * 
 **************************************************************************/

#ifndef _ADAFRUIT_ST77XXH_
#define _ADAFRUIT_ST77XXH_

//#include "typedefs.h"
//#include "lcd.h"

#define PROGMEM
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#define pgm_read_pointer(addr) (*(const unsigned long *)(addr))
typedef unsigned char prog_uchar;


typedef struct HardwareSPI SPIClass; ///< SPI is a bit odd on WICED

#define SPI_DEFAULT_FREQ 16000000L  ///< Default SPI data clock frequency

// Possible values for Adafruit_SPITFT.connection:
#define TFT_HARD_SPI 0  ///< Display interface = hardware SPI
#define TFT_SOFT_SPI 1  ///< Display interface = software SPI
#define TFT_PARALLEL 2  ///< Display interface = 8- or 16-bit parallel


#include <xc.h>
#include <stdint.h>

//#include "typedefs.h"
#include "8086_PIC.h"

//#include "Print.h"
//#include <libpic30.h>
#include "Adafruit_GFX.h"
//#include <Adafruit_SPITFT.h>
//#include <Adafruit_SPITFT_Macros.h>

#define ST7735_TFTWIDTH_128   128 // for 1.44 and mini
#define ST7735_TFTWIDTH_80     80 // for mini
#define ST7735_TFTHEIGHT_128  128 // for 1.44" display
#define ST7735_TFTHEIGHT_160  160 // for 1.8" and mini display

#define ST_CMD_DELAY      0x80    // special signifier for command lists

#define ST77XX_NOP        0x00
#define ST77XX_SWRESET    0x01
#define ST77XX_RDDID      0x04
#define ST77XX_RDDST      0x09

#define ST77XX_SLPIN      0x10
#define ST77XX_SLPOUT     0x11
#define ST77XX_PTLON      0x12
#define ST77XX_NORON      0x13

#define ST77XX_INVOFF     0x20
#define ST77XX_INVON      0x21
#define ST77XX_DISPOFF    0x28
#define ST77XX_DISPON     0x29
#define ST77XX_CASET      0x2A
#define ST77XX_RASET      0x2B
#define ST77XX_RAMWR      0x2C
#define ST77XX_RAMRD      0x2E

#define ST77XX_PTLAR      0x30
#define ST77XX_COLMOD     0x3A
#define ST77XX_MADCTL     0x36

#define ST77XX_MADCTL_MY  0x80
#define ST77XX_MADCTL_MX  0x40
#define ST77XX_MADCTL_MV  0x20
#define ST77XX_MADCTL_ML  0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1      0xDA
#define ST77XX_RDID2      0xDB
#define ST77XX_RDID3      0xDC
#define ST77XX_RDID4      0xDD

// Some ready-made 16-bit ('565') color settings:
#define	ST77XX_BLACK      0x0000
#define ST77XX_WHITE      0xFFFF
#define	ST77XX_RED        0xF800
#define	ST77XX_GREEN      0x07E0
#define	ST77XX_BLUE       0x001F
#define ST77XX_CYAN       0x07FF
#define ST77XX_MAGENTA    0xF81F
#define ST77XX_YELLOW     0xFFE0
#define	ST77XX_ORANGE     0xFC00

#define SPI_MODE0 0         // tanto per...

void Adafruit_SPITFT_1(uint16_t w, uint16_t h,
  int8_t cs, int8_t dc, int8_t mosi, int8_t sck, int8_t rst, int8_t miso);
void Adafruit_SPITFT_2(uint16_t w, uint16_t h, int8_t cs, int8_t dc, int8_t rst);


/// Subclass of SPITFT for ST77xx displays (lots in common!)
    int Adafruit_ST77xx_1(uint16_t w, uint16_t h, int8_t _CS, int8_t _mDC,
      int8_t _MOSI, int8_t _SCLK, int8_t _RST, int8_t _MISO);
    int Adafruit_ST77xx_2(uint16_t w, uint16_t h, int8_t CS, int8_t RS,
      int8_t RST);
/*#if !defined(ESP8266)
    int Adafruit_ST77xx_SPI(uint16_t w, uint16_t h, SPIClass *spiClass,
      int8_t CS, int8_t RS, int8_t RST);
#endif // end !ESP8266
 * */

    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void setRotation(uint8_t r);
    void setCursor(UGRAPH_COORD_T x,UGRAPH_COORD_T y);	//char addressing
    void enableDisplay(BOOL enable);

    extern uint8_t _colstart, ///< Some displays need this changed to offset
            _rowstart, ///< Some displays need this changed to offset
            spiMode; ///< Certain display needs MODE3 instead

    void    Adafruit_ST77xx_begin(uint32_t freq);
    void    commonInit(const uint8_t *cmdList);
    void    displayInit(const uint8_t *addr);
    void    setColRowStart(int8_t col, int8_t row);

    
extern WORD invertOnCommand,invertOffCommand;
extern WORD _xstart,_ystart;
extern uint8_t rotation;
extern uint32_t _freq;
extern uint8_t _initError;



void writeCommand(uint8_t);
void sendCommand(uint8_t, uint8_t *, uint8_t);


uint8_t spiRead(void);
void spiWrite(uint8_t c);
void SPI_WRITE16(uint16_t);
void SPI_WRITE32(uint32_t);
// se ne sbatte dell'inline, così faccio macro!
void __attribute__((always_inline)) SPI_BEGIN_TRANSACTION(void);
void __attribute__((always_inline)) SPI_END_TRANSACTION(void);
void __attribute__((always_inline)) startWrite(void);
void __attribute__((always_inline)) endWrite(void);
#ifdef ST7735
#define START_WRITE() SPI_CS_LOW()
#define END_WRITE() SPI_CS_HIGH()


#define SPI_CS_LOW() m_SPICSBit=0
#define SPI_DC_LOW() m_LCDDCBit=0
#define SPI_DC_HIGH() m_LCDDCBit=1
#define SPI_CS_HIGH() m_SPICSBit=1
#endif
  


#define MAKEWORD(a, b)   ((uint16_t) (((uint8_t) (a)) | ((uint16_t) ((uint8_t) (b))) << 8)) 
#define MAKELONG(a, b)   ((unsigned long) (((uint16_t) (a)) | ((uint32_t) ((uint16_t) (b))) << 16)) 
#define HIBYTE(w)   ((uint8_t) ((((uint16_t) (w)) >> 8) /* & 0xFF*/)) 
//#define HIBYTE(w)   ((uint8_t) (*((char *)&w+1)))		// molto meglio :)
#define HIWORD(l)   ((uint16_t) (((uint32_t) (l) >> 16) & 0xFFFF)) 
#define LOBYTE(w)   ((uint8_t) (w)) 
#define LOWORD(l)   ((uint16_t) (l)) 


#endif // _ADAFRUIT_ST77XXH_
