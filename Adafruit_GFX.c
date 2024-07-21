/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

	GD adapted to Microchip 24/2/2016 (and 2014 too) - porcodio
	(inizialmente pareva che quel display fosse come il 5110 invece no, ... vabbe' è migliore!)
*/

#include "8086_PIC.h"
//#include "typedefs.h"

#include "Adafruit_GFX.h"
#include "glcdfont.c"

#include "Adafruit_ST7735.h"

#include <string.h>
#include <stdint.h>
//#include <sramalloc.h>



/*#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef abs
#define abs(a) (((a) >= (0)) ? (a) : (-a))
#endif*/



const UGRAPH_COORD_T WIDTH=_TFTWIDTH, HEIGHT=_TFTHEIGHT;   // This is the 'raw' display w/h - never changes
UGRAPH_COORD_T _width, _height, // Display w/h as modified by current rotation
  cursor_x, cursor_y;
WORD textcolor, textbgcolor;
BYTE textsize;
BOOL wrap,_cp437; // If set, 'wrap' text at right edge of display
#ifdef USE_CUSTOM_FONTS 
GFXfont *gfxFont;
#endif

#if defined(SSD1309)
uint8_t lcd_dirty=0;
#endif


//ADAFRUIT_GFX Adafruit_GFX_this;

#ifdef USE_GFX_UI

ADAFRUIT_GFX _gfx;
GRAPH_COORD_T _x, _y;
UGRAPH_COORD_T _w, _h;
uint8_t _textsize;
UINT16 _outlinecolor, _fillcolor, _textcolor;
char _label[10];

BOOL currstate, laststate;
#endif


void Adafruit_GFX(UGRAPH_COORD_T w, UGRAPH_COORD_T h) {

  _width    = w;
  _height   = h;

  cursor_y  = cursor_x    = 0;
  textsize  = 1;
  textcolor = WHITE;
  textcolor = BLACK;
  wrap      = TRUE;
  _cp437    = FALSE;
#ifdef USE_CUSTOM_FONTS 
  gfxFont   = NULL;
#endif
	}

// Draw a circle outline
void drawCircle(UGRAPH_COORD_T x0, UGRAPH_COORD_T y0, UGRAPH_COORD_T r, UINT16 color) {
  GRAPH_COORD_T f = 1 - r;
  GRAPH_COORD_T ddF_x = 1;
  GRAPH_COORD_T ddF_y = -2 * r;
  GRAPH_COORD_T x = 0;
  GRAPH_COORD_T y = r;

  drawPixel(x0  , y0+r, color);
  drawPixel(x0  , y0-r, color);
  drawPixel(x0+r, y0  , color);
  drawPixel(x0-r, y0  , color);

  while(x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
		}
#if defined(SSD1309)
  lcd_dirty=1;
#endif
	}

void drawCircleHelper(UGRAPH_COORD_T x0, UGRAPH_COORD_T y0, UGRAPH_COORD_T r, uint8_t cornername, UINT16 color) {
  GRAPH_COORD_T f     = 1 - r;
  GRAPH_COORD_T ddF_x = 1;
  GRAPH_COORD_T ddF_y = -2 * r;
  GRAPH_COORD_T x     = 0;
  GRAPH_COORD_T y     = r;

  while(x<y) {
    if(f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if(cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    	}
    if(cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    	}
    if(cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    	}
    if(cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
  	  }
		}
	}

void fillCircle(UGRAPH_COORD_T x0, UGRAPH_COORD_T y0, UGRAPH_COORD_T r, UINT16 color) {

  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
#if defined(SSD1309)
  lcd_dirty=1;
#endif
	}

// Used to do circles and roundrects
void fillCircleHelper(UGRAPH_COORD_T x0, UGRAPH_COORD_T y0, UGRAPH_COORD_T r, uint8_t cornername, GRAPH_COORD_T delta, UINT16 color) {
  GRAPH_COORD_T f     = 1 - r;
  GRAPH_COORD_T ddF_x = 1;
  GRAPH_COORD_T ddF_y = -2 * r;
  GRAPH_COORD_T x     = 0;
  GRAPH_COORD_T y     = r;

  while(x<y) {
    if(f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    	}
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if(cornername & 0x1) {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    	}
    if(cornername & 0x2) {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    	}
  	}
	}


// Draw a rounded rectangle
void drawRoundRect(UGRAPH_COORD_T x, UGRAPH_COORD_T y, UGRAPH_COORD_T w, UGRAPH_COORD_T h, UGRAPH_COORD_T r, UINT16 color) {

  // smarter version
  drawFastHLine(x+r  , y    , w-2*r, color); // Top
  drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x    , y+r  , h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
  
#if defined(SSD1309)
  lcd_dirty=1;
#endif

	}

// Fill a rounded rectangle
void fillRoundRect(UGRAPH_COORD_T x, UGRAPH_COORD_T y, UGRAPH_COORD_T w, UGRAPH_COORD_T h, UGRAPH_COORD_T r, UINT16 color) {

  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
	}

// Draw a triangle
void drawTriangle(UGRAPH_COORD_T x0, UGRAPH_COORD_T y0, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2, UINT16 color) {

  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
	}

// Fill a triangle
void fillTriangle(UGRAPH_COORD_T x0, UGRAPH_COORD_T y0, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2, UINT16 color) {
  GRAPH_COORD_T a, b, y, last;
  GRAPH_COORD_T dx01, dy01, dx02, dy02, dx12, dy12;
  INT32 sa, sb;


  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if(y0 > y1) {
    _swap(&y0,&y1); _swap(&x0,&x1);
 		}
  if(y1 > y2) {
    _swap(&y2,&y1); _swap(&x2,&x1);
  	}
  if(y0 > y1) {
    _swap(&y0,&y1); _swap(&x0,&x1);
	  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      
			a = x1;
    else 
			if(x1 > b) 
				b = x1;
    if(x2 < a)      
			a = x2;
    else 
			if(x2 > b) 
				b = x2;
    drawFastHLine(a, y0, b-a+1, color);
    return;
  	}

    dx01 = x1 - x0;
    dy01 = y1 - y0;
    dx02 = x2 - x0;
    dy02 = y2 - y0;
    dx12 = x2 - x1;
    dy12 = y2 - y1;
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) 
		last = y1;   // Include y1 scanline
  else         
		last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) 
			_swap(&a,&b);
    drawFastHLine(a, y, b-a+1, color);
  	}

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) 
			_swap(&a,&b);
    drawFastHLine(a, y, b-a+1, color);
  	}
  
#if defined(SSD1309)
  lcd_dirty=1;
#endif
	}

// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer (must be PROGMEM memory) using the specified
// foreground color (unset bits are transparent).
void drawBitmap(UGRAPH_COORD_T x, UGRAPH_COORD_T y, const uint8_t *bitmap, UGRAPH_COORD_T w, UGRAPH_COORD_T h, UINT16 color) {
  GRAPH_COORD_T i, j, byteWidth = (w+7) / 8;
  uint8_t byte=0;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      if(i & 7) 
				byte <<= 1;
      else      
				byte = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      if(byte & 0x80) 
				drawPixel(x+i, y+j, color);
  	  }
  	}
	}

// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer (must be PROGMEM memory) using the specified
// foreground (for set bits) and background (for clear bits) colors.
void drawBitmapBG(UGRAPH_COORD_T x, UGRAPH_COORD_T y, const uint8_t *bitmap, UGRAPH_COORD_T w, UGRAPH_COORD_T h, UINT16 color, UINT16 bg) {
  GRAPH_COORD_T i, j, byteWidth = (w + 7) / 8;
  uint8_t byte=0;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) 
				byte <<= 1;
      else      
				byte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      if(byte & 0x80) 
				drawPixel(x+i, y+j, color);
      else            
				drawPixel(x+i, y+j, bg);
    	}
  	}
	}


void drawBitmap4(UGRAPH_COORD_T x, UGRAPH_COORD_T y, const uint8_t *bitmap) {
  UGRAPH_COORD_T i,j;
  UGRAPH_COORD_T h,w;
  uint8_t b,*p,c1,c2,c3;
  uint32_t l;
  uint16_t /*GFX_COLOR*/ c;
  
  w=MAKEWORD(bitmap[4],bitmap[5]);
  h=MAKEWORD(bitmap[8],bitmap[9]);

  if(MAKEWORD(bitmap[14],bitmap[15]) != 4)    // bpp
    return;

  p=bitmap+40+4*16;   // RGBQUAD ovvero 4 byte per palette entry!
  for(j=0; j<h; j++) {
    for(i=0; i<w; i+=2) {
      b=*p >> 4;
      c1=bitmap[40+b*4];
      c2=bitmap[40+1+b*4];
      c3=bitmap[40+2+b*4];
      c=Color565(c3,c2,c1);
			drawPixel(x+i, y+j, c);
      b=*p++ & 0x0f;
      c1=bitmap[40+b*4];
      c2=bitmap[40+1+b*4];
      c3=bitmap[40+2+b*4];
      c=Color565(c3,c2,c1);
			drawPixel(x+i+1, y+j, c);
  	  }
  	}
	}


#ifdef RAM_BITMAP
// drawBitmap() variant for RAM-resident (not PROGMEM) bitmaps.
void drawBitmap3(UGRAPH_COORD_T x, UGRAPH_COORD_T y, uint8_t *bitmap, UGRAPH_COORD_T w, UGRAPH_COORD_T h, UINT16 color) {
  GRAPH_COORD_T i, j, byteWidth = (w + 7) / 8;
  uint8_t byte;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) 
				byte <<= 1;
      else      
				byte   = bitmap[j * byteWidth + i / 8];
      if(byte & 0x80) 
				drawPixel(x+i, y+j, color);
    	}
  	}
	}

// drawBitmap() variant w/background for RAM-resident (not PROGMEM) bitmaps.
void drawBitmap4(UGRAPH_COORD_T x, UGRAPH_COORD_T y, uint8_t *bitmap, UGRAPH_COORD_T w, UGRAPH_COORD_T h, UINT16 color, UINT16 bg) {
  GRAPH_COORD_T i, j, byteWidth = (w + 7) / 8;
  uint8_t byte;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) 
				byte <<= 1;
      else      
				byte   = bitmap[j * byteWidth + i / 8];
      if(byte & 0x80) 
				drawPixel(x+i, y+j, color);
      else            
				drawPixel(x+i, y+j, bg);
    	}
  	}
	}

//Draw XBitMap Files (*.xbm), exported from GIMP,
//Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//C Array can be directly used with this function
void drawXBitmap(UGRAPH_COORD_T x, UGRAPH_COORD_T y,
  const uint8_t *bitmap, UGRAPH_COORD_T w, UGRAPH_COORD_T h, UINT16 color) {

  GRAPH_COORD_T i, j, byteWidth = (w + 7) / 8;
  uint8_t byte;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(i & 7) 
				byte >>= 1;
      else      
				byte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      if(byte & 0x01) 
				drawPixel(x+i, y+j, color);
    	}
  	}
	}
#endif


size_t cwrite(uint8_t c) {

#ifdef USE_CUSTOM_FONTS 
  if(!gfxFont) { // 'Classic' built-in font
#endif

    if(c == '\n') {
      cursor_y += textsize*8;
      cursor_x  = 0;
    	} 
		else if(c == '\r') {
      // skip em
    	} 
		else {
      if(wrap && ((cursor_x + textsize * 6) >= _width)) { 	// Heading off edge?
        cursor_x  = 0;            // Reset x to zero
        cursor_y += textsize * 8; // Advance y one line
      	}
      drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
      cursor_x += textsize * 6;
    	}

#ifdef USE_CUSTOM_FONTS 
  	} 
	else { // Custom font

    if(c == '\n') {
      cursor_x  = 0;
      cursor_y += (GRAPH_COORD_T)textsize *
                  (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
  	  } 
		else if(c != '\r') {
      uint8_t first = pgm_read_byte(&gfxFont->first);
      if((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
        uint8_t   c2    = c - pgm_read_byte(&gfxFont->first);
        GFXglyph *glyph = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c2]);
        uint8_t   w     = pgm_read_byte(&glyph->width),
                  h     = pgm_read_byte(&glyph->height);
        if((w > 0) && (h > 0)) { // Is there an associated bitmap?
          GRAPH_COORD_T xo = (INT8)pgm_read_byte(&glyph->xOffset); // sic
          if(wrap && ((cursor_x + textsize * (xo + w)) >= _width)) {
            // Drawing character would go off right edge; wrap to new line
            cursor_x  = 0;
            cursor_y += (GRAPH_COORD_T)textsize *
                        (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
          	}
          drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
        	}
        cursor_x += pgm_read_byte(&glyph->xAdvance) * (GRAPH_COORD_T)textsize;
      	}
    	}

  	}
#endif

  return 1;
	}

// Draw a character
void drawChar(UGRAPH_COORD_T x, UGRAPH_COORD_T y, unsigned char c, UINT16 color, UINT16 bg, uint8_t size) {
	INT8 i,j;
	BYTE *fontPtr;

#ifdef USE_CUSTOM_FONTS 
  if(!gfxFont) { // 'Classic' built-in font
#endif

    if((x >= _width)            || // Clip right
       (y >= _height)           || // Clip bottom
       ((x + 6 * size - 1) < 0) || // Clip left
       ((y + 8 * size - 1) < 0))   // Clip top
      return;

    if(!_cp437 && (c >= 176)) 
			c++; // Handle 'classic' charset behavior
#ifndef USE_256_CHAR_FONT 
		if(c>=128)
			return;
#endif

		fontPtr=font+((UINT16)c)*5;
    for(i=0; i<6; i++) {
      uint8_t line;
      if(i<5) 
				line = pgm_read_byte(fontPtr+i);
      else  
		    line = 0x0;
      for(j=0; j<8; j++, line >>= 1) {
        if(line & 0x1) {
          if(size == 1) 
						drawPixel(x+i, y+j, color);
          else
		        fillRect(x+(i*size), y+(j*size), size, size, color);
        	} 
				else if(bg != color) {
          if(size == 1) 
						drawPixel(x+i, y+j, bg);
          else          
						fillRect(x+(i*size), y+(j*size), size, size, bg);
      	  }
    	  }
  	  }

#ifdef USE_CUSTOM_FONTS 
	  } 
	else { // Custom font
    GFXglyph *glyph;
    uint8_t  *bitmap;
    UINT16 bo;
    uint8_t  w, h, xa;
    INT8   xo, yo;
    uint8_t  xx, yy, bits, bit = 0;
    GRAPH_COORD_T  xo16, yo16;

    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling drawChar()
    // directly with 'bad' characters of font may cause mayhem!

    c -= pgm_read_byte(&gfxFont->first);
    glyph  = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
    bitmap = (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);

    bo = pgm_read_word(&glyph->bitmapOffset);
    w  = pgm_read_byte(&glyph->width),
             h  = pgm_read_byte(&glyph->height),
             xa = pgm_read_byte(&glyph->xAdvance);
    xo = pgm_read_byte(&glyph->xOffset),
             yo = pgm_read_byte(&glyph->yOffset);
    xx, yy, bits, bit = 0;
    xo16, yo16;

    if(size > 1) {
      xo16 = xo;
      yo16 = yo;
   		}

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL infortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.

    for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 7)) {
          bits = pgm_read_byte(&bitmap[bo++]);
        	}
        if(bits & 0x80) {
          if(size == 1) {
            drawPixel(x+xo+xx, y+yo+yy, color);
          	} 
					else {
            fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          	}
        	}
        bits <<= 1;
      	}
    	}

  	} // End classic vs custom font
#endif
  
#if defined(SSD1309)
  lcd_dirty=1;
#endif

	}


void setTextSize(uint8_t s) {

  textsize = (s > 0) ? s : 1;
	}

void setTextColor(UINT16 c) {

  // For 'transparent' background, we'll set the bg
  //   to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
	}

void setTextColorBG(WORD c, WORD b) {
  
	textcolor   = c;
  textbgcolor = b; 
	}

void setTextWrap(BOOL w) {

  wrap = w;
	}

uint8_t getRotation(void) {

  return rotation;
	}


// Enable (or disable) Code Page 437-compatible charset.
// There was an error in glcdfont.c for the longest time -- one character
// (#176, the 'light shade' block) was missing -- this threw off the index
// of every character that followed it.  But a TON of code has been written
// with the erroneous character indices.  By default, the library uses the
// original 'wrong' behavior and old sketches will still work.  Pass 'true'
// to this function to use correct CP437 character values in your code.
void cp437(BOOL x) {
  
	_cp437 = x;
	}

#ifdef USE_CUSTOM_FONTS 
void setFont(const GFXfont *f) {

  if(f) {          // Font struct pointer passed in?
    if(!gfxFont) { // And no current font struct?
      // Switching from classic to new font behavior.
      // Move cursor pos down 6 pixels so it's on baseline.
      cursor_y += 6;
    	}
  	} 
	else if(gfxFont) { // NULL passed.  Current font struct defined?
    // Switching from new to classic font behavior.
    // Move cursor pos up 6 pixels so it's at top-left of char.
    cursor_y -= 6;
  	}
  gfxFont = (GFXfont *)f;
	}
#endif

// Pass string and a cursor position, returns UL corner and W,H.
void getTextBounds(char *str, UGRAPH_COORD_T x, UGRAPH_COORD_T y, UGRAPH_COORD_T *x1, UGRAPH_COORD_T *y1, UGRAPH_COORD_T *w, UGRAPH_COORD_T *h) {
  uint8_t c; // Current character

  *x1 = x;
  *y1 = y;
  *w  = *h = 0;

#ifdef USE_CUSTOM_FONTS
  if(gfxFont) {

    GFXglyph *glyph;
    uint8_t   first = pgm_read_byte(&gfxFont->first),
              last  = pgm_read_byte(&gfxFont->last),
              gw, gh, xa;
    INT8    xo, yo;
    GRAPH_COORD_T   minx = _width, miny = _height, maxx = -1, maxy = -1,
              gx1, gy1, gx2, gy2, ts = (GRAPH_COORD_T)textsize,
              ya = ts * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);

    while(1) {
			if(str) {
				c=*str++;
				}
			else if(strRom) {
				c=*strRom++;
				}
			if(!c)
				break;
      if(c != '\n') { // Not a newline
        if(c != '\r') { // Not a carriage return, is normal char
          if((c >= first) && (c <= last)) { // Char present in current font
            c    -= first;
            glyph = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
            gw    = pgm_read_byte(&glyph->width);
            gh    = pgm_read_byte(&glyph->height);
            xa    = pgm_read_byte(&glyph->xAdvance);
            xo    = pgm_read_byte(&glyph->xOffset);
            yo    = pgm_read_byte(&glyph->yOffset);
            if(wrap && ((x + (((GRAPH_COORD_T)xo + gw) * ts)) >= _width)) {
              // Line wrap
              x  = 0;  // Reset x to 0
              y += ya; // Advance y by 1 line
            }
            gx1 = x   + xo * ts;
            gy1 = y   + yo * ts;
            gx2 = gx1 + gw * ts - 1;
            gy2 = gy1 + gh * ts - 1;
            if(gx1 < minx) 
							minx = gx1;
            if(gy1 < miny) 
							miny = gy1;
            if(gx2 > maxx) 
							maxx = gx2;
            if(gy2 > maxy) 
							maxy = gy2;
            x += xa * ts;
          	}
        	} // Carriage return = do nothing
      	} 
			else { // Newline
        x  = 0;  // Reset x
        y += ya; // Advance y by 1 line
      	}
    	}
    // End of string
    *x1 = minx;
    *y1 = miny;
    if(maxx >= minx) 
			*w  = maxx - minx + 1;
    if(maxy >= miny) 
			*h  = maxy - miny + 1;

  	} 
	else { // Default font
#endif
		{
    UGRAPH_COORD_T lineWidth = 0, maxWidth = 0; // Width of current, all lines

    while(1) {
			if(str) {
				c=*str++;
				}
			if(!c)
				break;
      if(c != '\n') { // Not a newline
        if(c != '\r') { // Not a carriage return, is normal char
          if(wrap && ((x + textsize * 6) >= _width)) {
            x  = 0;            // Reset x to 0
            y += textsize * 8; // Advance y by 1 line
            if(lineWidth > maxWidth) maxWidth = lineWidth; // Save widest line
            lineWidth  = textsize * 6; // First char on new line
          	} 
					else { // No line wrap, just keep incrementing X
            lineWidth += textsize * 6; // Includes interchar x gap
          	}
        	} // Carriage return = do nothing
      	} 
			else { // Newline
        x  = 0;            // Reset x to 0
        y += textsize * 8; // Advance y by 1 line
        if(lineWidth > maxWidth) maxWidth = lineWidth; // Save widest line
        lineWidth = 0;     // Reset lineWidth for new line
      	}
    	}
    // End of string
    if(lineWidth) 
			y += textsize * 8; // Add height of last (or only) line
    *w = maxWidth - 1;               // Don't include last interchar x gap
    *h = y - *y1;
		}

#ifdef USE_CUSTOM_FONTS
  	} // End classic vs custom font
#endif
	}


// Return the size of the display (per current rotation)
UGRAPH_COORD_T width(void) {
  
	return _width;
	}

UGRAPH_COORD_T height(void) {
  
	return _height;
	}


/***************************************************************************/
// code for the GFX button UI element

#ifdef USE_GFX_UI
void Adafruit_GFX_Button(void) {
  
	_gfx = 0;
	}

void initButton(ADAFRUIT_GFX gfx, UGRAPH_COORD_T x, UGRAPH_COORD_T y, uint8_t w, uint8_t h,
  UINT16 outline, UINT16 fill, UINT16 textcolor,
  char *label, uint8_t textsize) {

  _x            = x;
  _y            = y;
  _w            = w;
  _h            = h;
  _outlinecolor = outline;
  _fillcolor    = fill;
  _textcolor    = textcolor;
  _textsize     = textsize;
  _gfx          = gfx;
  strncpy(_label, label, 9);
  _label[9] = 0;
	}

void drawButton(BOOL inverted) {
  UINT16 fill, outline, text;

  if(!inverted) {
    fill    = _fillcolor;
    outline = _outlinecolor;
    text    = _textcolor;
  	} 
	else {
    fill    = _textcolor;
    outline = _outlinecolor;
    text    = _fillcolor;
  }

  fillRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, fill);
  drawRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, outline);

  setCursor(_x - strlen(_label)*3*_textsize, _y-4*_textsize);
  setTextColor(text);
  setTextSize(_textsize);
  gfx_print(_label);
	}

BOOL contains(UGRAPH_COORD_T x, UGRAPH_COORD_T y) {

  if((x < (_x - _w/2)) || (x > (_x + _w/2))) 
		return FALSE;
  if((y < (_y - _h/2)) || (y > (_y + _h/2))) 
		return FALSE;
  return TRUE;
	}

void press(BOOL p) {

  laststate = currstate;
  currstate = p;
	}

BOOL Adafruit_GFX_Button_isPressed() { return currstate; }
BOOL Adafruit_GFX_Button_justPressed() { return (currstate && !laststate); }
BOOL Adafruit_GFX_Button_justReleased() { return (!currstate && laststate); }

#endif


void gfx_print(char *s) {

	while(*s) 
		cwrite(*s++);
	}



