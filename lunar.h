/*
 * lunar.h
 *
 * public header for shared library
 *
 * This wizard-generated code is based on code adapted from the
 * SampleLib project distributed as part of the Palm OS SDK 4.0,
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */

#ifndef LUNAR_H_
#define LUNAR_H_

/* Palm OS common definitions */
#include <SystemMgr.h>

/* If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2. */
#ifdef BUILDING_LUNAR
	#define LUNAR_LIB_TRAP(trapNum)
#else
	#define LUNAR_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif

/*********************************************************************
 * Type and creator of Sample Library database
 *********************************************************************/

#define		lunarCreatorID	'sLun'
#define		lunarTypeID		sysFileTLibrary

/*********************************************************************
 * Internal library name which can be passed to SysLibFind()
 *********************************************************************/

#define		lunarName		"LunarLibrary"

/*********************************************************************
 * LunarLibrary result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *********************************************************************/

/* invalid parameter */
#define lunarErrParam		(appErrorClass | 1)		

/* library is not open */
#define lunarErrNotOpen		(appErrorClass | 2)		

/* returned from lunarClose() if the library is still open */
#define lunarErrStillOpen	(appErrorClass | 3)		

/*********************************************************************
 * API Prototypes
 *********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Standard library open, close, sleep and wake functions */

extern Err lunarOpen(UInt16 refNum, UInt32 * clientContextP)
	LUNAR_LIB_TRAP(sysLibTrapOpen);
				
extern Err lunarClose(UInt16 refNum, UInt32 clientContext)
	LUNAR_LIB_TRAP(sysLibTrapClose);

extern Err lunarSleep(UInt16 refNum)
	LUNAR_LIB_TRAP(sysLibTrapSleep);

extern Err lunarWake(UInt16 refNum)
	LUNAR_LIB_TRAP(sysLibTrapWake);

/* Custom library API functions */

extern Err lunarS2L(UInt16 refNum, int syear, int smonth, int sday, int* plyear, int* plmonth, int *plday, int *leapyes)
	LUNAR_LIB_TRAP(sysLibTrapBase + 5);

extern Err lunarL2S(UInt16 refNum, int lyear, int lmonth, int lday, int leapyes, int* psyear, int* psmonth, int* psday)
	LUNAR_LIB_TRAP(sysLibTrapBase + 6);

#ifdef __cplusplus
}
#endif

Err lunar_OpenLibrary(UInt16 *refNumP, UInt32 * clientContextP);
Err lunar_CloseLibrary(UInt16 refNum, UInt32 clientContext);

#endif /* LUNAR_H_ */
