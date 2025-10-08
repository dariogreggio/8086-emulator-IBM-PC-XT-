#ifndef _8086_IO_H_INCLUDED
#define _8086_IO_H_INCLUDED


#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
//#include "8086win.h"


#ifdef EXT_80286
extern struct _EXCEPTION Exception86;
extern union _DTR GDTR;
extern union _DTR *LDTR;
extern union MACHINE_STATUS_WORD _msw;
extern union REGISTRO_F _f;
#endif

enum DMA_CONTROL {		// 
	DMA_DISABLED		= 0x01,
	DMA_UPDOWN			= 0x20,
	DMA_AUTORELOAD	= 0x10,

//masks
	DMA_READ				= 0x04,
	DMA_TYPE				= 0x0C,
	DMA_SINGLE			= 0x40,
	DMA_MODE				= 0xC0,

	DMA_MODEVERIFY	= 0x00,
	DMA_MODEWRITE		= 0x01,
	DMA_MODEREAD		= 0x02,

	DMA_MODEDEMAND	= 0x00,
	DMA_MODESINGLE	= 0x01,
	DMA_MODEBLOCK		= 0x02,
	DMA_MODECASCADE	= 0x03,

	DMA_MASK0				= 0x01,
	DMA_MASK1				= 0x02,
	DMA_MASK2				= 0x04,
	DMA_MASK3				= 0x08,

	DMA_TC0					= 0x01,
	DMA_TC1					= 0x02,
	DMA_TC2					= 0x04,
	DMA_TC3					= 0x08,
	DMA_REQUEST0		= 0x10,
	DMA_REQUEST1		= 0x20,
	DMA_REQUEST2		= 0x40,
	DMA_REQUEST3		= 0x80,
	};

enum PIT_MODE {		// 
	PIT_MODE_INTONTERMINAL		= 0,
	PIT_MODE_ONESHOT					= 1,
	PIT_MODE_FREECOUNTER			= 2,
	PIT_MODE_SQUAREWAVEGEN		= 3,
	PIT_MODE_SWTRIGGERED			= 4,
	PIT_MODE_HWTRIGGERED			= 5,
	};

enum PIT_OPERATION {		// 
	PIT_LATCH								= 0,
	PIT_LOADLOW							= 1,
	PIT_LOADHIGH						= 2,
	PIT_LOADBOTH						= 3,
	};

enum PIT_FLAGS {		// 
	PIT_OUTPUT						= 0x80,
	PIT_ACTIVE						= 0x40,
	PIT_LATCHED						= 0x20,
	PIT_LOADED						= 0x10,
	PIT_LOHIBYTE					= 0x08,

	PIT_BCD								= 0x01,
	};

enum PIC_OPERATION {		// 
	PIC_AUTOROTATE						= 0,
	PIC_CLEARNONSPEC					= 1,
	PIC_NOOPERATION						= 2,
	PIC_CLEARSPEC							= 3,
	PIC_ROTATEAUTO						= 4,
	PIC_ROTATENONSPEC					= 5,
	PIC_SETPRIORITY						= 6,
	PIC_ROTATESPEC						= 7,
	};

enum PIC_FLAGS {		// 
	PIC_ISREQUEST							= 0x08,
	PIC_ISINIT								= 0x10,
	PIC_WANTSIRR							= 0x2,
	PIC_WANTSISR							= 0x3,
	};

#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define CASCADE_IRQ 2


enum KB_CONTROL {		// 
	KB_IRQENABLE			= 0x01,
	KB_TRANSLATE			= 0x40,
	};
enum KB_STATUS {		// 
	KB_OUTPUTFULL			= 0x01,
	KB_INPUTFULL			= 0x02,
	KB_POWERON				= 0x04,
	KB_COMMAND				= 0x08,
	KB_ENABLED				= 0x10,
	};

enum FLOPPY_COMMANDS {		// https://wiki.osdev.org/Floppy_Disk_Controller  (da granite
	FLOPPY_READ_TRACK =                 2,	  // generates IRQ6 tutti, piciu ;)
	FLOPPY_SPECIFY =                    3,      // set drive parameters
	FLOPPY_SENSE_DRIVE_STATUS =         4,
	FLOPPY_WRITE_DATA =                 5,      // write to the disk
	FLOPPY_READ_DATA =                  6,      // read from the disk
	FLOPPY_RECALIBRATE =                7,      // seek to cylinder 0
	FLOPPY_SENSE_INTERRUPT =            8,      // ack IRQ6, get status of last command
	FLOPPY_WRITE_DELETED_DATA =         9,
	FLOPPY_READ_ID =                    10,	// generates IRQ6
	FLOPPY_READ_DELETED_DATA =          12,
	FLOPPY_FORMAT_TRACK =               13,     // 
	FLOPPY_DUMPREG =                    14,
	FLOPPY_SEEK =                       15,     // seek both heads to cylinder X
	FLOPPY_VERSION =                    16,	// used during initialization, once
	FLOPPY_SCAN_EQUAL =                 17,
	FLOPPY_PERPENDICULAR_MODE =         18,	// used during initialization, once, maybe
	FLOPPY_CONFIGURE =                  19,     // set controller parameters
	FLOPPY_LOCK =                       20,     // protect controller params from a reset
	FLOPPY_VERIFY =                     22,
	FLOPPY_SCAN_LOW_OR_EQUAL =          25,
	FLOPPY_SCAN_HIGH_OR_EQUAL =         29,
    
	FLOPPY_COMMAND_MASK =         0x1f
  };
enum FLOPPY_STATE {		// 
	FLOPPY_STATE_DISKPRESENT =        0x80,
	FLOPPY_STATE_DDR =				        0x40,
	FLOPPY_STATE_XFER =								0x20,		// usato per handshake
	FLOPPY_STATE_MOTORON =						0x04,
	FLOPPY_STATE_DISKCHANGED =        0x02,
	FLOPPY_STATE_STEPDIR =		        0x01,
	};
enum FLOPPY_STATUS {		// 
	FLOPPY_STATUS_REQ =								0x80,
	FLOPPY_STATUS_DDR =								0x40,		// 1 floppy->CPU, 0 CPU->floppy
	FLOPPY_STATUS_NONDMA =						0x20,
	FLOPPY_STATUS_BUSY =			        0x10,
	FLOPPY_STATUS_DISKS =							0x0F,
	};
enum FLOPPY_CONTROL {		// 
	FLOPPY_CONTROL_MT =								0x80,
	FLOPPY_CONTROL_MF =								0x40,
	FLOPPY_CONTROL_SK =								0x20,

	FLOPPY_CONTROL_IRQ =			        0x08,
	};
enum FLOPPY_ERROR0 {		// 
	FLOPPY_ERROR_INVALID =		        0x80,
	FLOPPY_ERROR_ABNORMAL =		        0x40,
	FLOPPY_ERROR_SEEKEND =		        0x20,
	FLOPPY_ERROR_CHECK =							0x10,
	FLOPPY_ERROR_NOTREADY =		        0x08,
	};
enum FLOPPY_ERROR1 {		// 
	FLOPPY_ERROR_ENDCYLINDER =		    0x80,
	FLOPPY_ERROR_CRC =								0x20,
	FLOPPY_ERROR_OVERRUN =						0x10,
	FLOPPY_ERROR_READONLY =						0x02,
	FLOPPY_ERROR_NOMARK =							0x01,
	};
enum FLOPPY_ERROR2 {		// 
	FLOPPY_ERROR_DATACRC =						0x20,
	FLOPPY_ERROR_WRONGCYLINDER =			0x10,
	FLOPPY_ERROR_BADCYLINDER =				0x02,
	FLOPPY_ERROR_NOMARKDATA =					0x01,
	};

enum HD_COMMANDS_XT {
	HD_XT_TEST_READY =                 0,
	HD_XT_RECALIBRATE =                1,      // seek to cylinder 0
	HD_XT_READ_STATUS =				         3,
	HD_XT_FORMAT_DRIVE =               4,     // 
	HD_XT_VERIFY =                     5,
	HD_XT_FORMAT_TRACK =               6,     // 
	HD_XT_FORMAT_BAD_TRACK =           7,     // 
	HD_XT_READ_DATA =                  8,      // read from the disk
	HD_XT_WRITE_DATA =                 10,      // write to the disk
	HD_XT_SEEK =                       11,     // seek both heads to cylinder X
	HD_XT_INITIALIZE =                 12,     // set controller parameters
	HD_XT_READ_ECC_BURST_ERROR_LENGTH= 13,	  // 
	HD_XT_READ_SECTOR_BUFFER =         14,      // read from the disk
	HD_XT_WRITE_SECTOR_BUFFER =        15,      // write to the disk
	HD_XT_EXECUTE_SECTOR_DIAGNOSTIC =  0xe0,   // 
	HD_XT_EXECUTE_DRIVE_DIAGNOSTIC =   0xe3,   // 
	HD_XT_EXECUTE_CONTROLLER_DIAGNOSTIC = 0xe4,   // 
	HD_XT_READ_LONG =                  0xe5,    // read from the disk
	HD_XT_WRITE_LONG =                 0xe6,    // write to the disk
    
	HD_XT_COMMAND_MASK =         0x1f
  };
enum HD_STATUS_XT {		// 
	HD_XT_STATUS_XFER =							0x01,		// handshake
	HD_XT_STATUS_DDR =							0x02,
	HD_XT_STATUS_CD =								0x04,
	HD_XT_STATUS_BUSY =							0x08,
	HD_XT_STATUS_DRQ =							0x10,
	HD_XT_STATUS_IRQ =							0x20,
	};
enum HD_COMMANDS_AT {		// FINIRE :) 
	HD_AT_RESTORE =                 0x10,  // generates IRQ14 su AT 
	HD_AT_SEEK =                    0x70,     // seek both heads to cylinder X
	HD_AT_READ_DATA =               0x20,      // read from the disk
	HD_AT_WRITE_DATA =              0x30,      // write to the disk
	HD_AT_FORMAT =									0x50,     // 
	HD_AT_READ_VERIFY =             0x40,
	HD_AT_DIAGNOSE =                0x90,      // 
	HD_AT_SET_DRIVE_PARAMETERS =    0x91,      // set drive parameters
	HD_AT_DEVICE_CONTROL =					0xb0,   // 
	HD_AT_READ_MULTIPLE =           0xc0,      // read from the disk
	HD_AT_WRITE_MULTIPLE =          0xc0,      // write to the disk
	HD_AT_IDENTIFY =								0xec,   // 
	HD_AT_FLUSHCACHE =							0xe7,   // anche ef??
  };
enum HD_STATUS_AT {		// 
	HD_AT_STATUS_ERROR =						0x01,
	HD_AT_STATUS_DRQ =							0x08,
	HD_AT_STATUS_READYTOSEEK =			0x10,
	HD_AT_STATUS_READY =						0x40,
	HD_AT_STATUS_BUSY =							0x80
	};

#endif
