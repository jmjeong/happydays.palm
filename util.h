/*
HappyDays - A Birthdate displayer for the PalmPilot
Copyright (C) 1999-2001 JaeMok Jeong

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef UTIL_H__
#define UTIL_H__

#include <PalmOS.h>
#include "section.h"

extern void * GetObjectPointer(FormPtr frm, UInt16 objID);
extern FieldPtr SetFieldTextFromHandle(UInt16 fieldID, MemHandle txtH);
extern FieldPtr SetFieldTextFromStr(UInt16 fieldID, Char * strP);
extern FieldPtr ClearFieldText(UInt16 fieldID);
extern Char * DateToAsciiLong(UInt8 months, UInt8 days, Int16 year,
                            DateFormatType dateFormat, Char * pstring);
extern UInt32 Hash(Char* name1, Char* name2);
extern Boolean SelectCategoryPopup(DmOpenRef dbP, UInt16* selected,
                                   UInt32 list, UInt32 trigger, Char *string);
extern void DisplayCategory(UInt32 trigger, Char* string, Boolean redraw);

extern MemPtr AppInfoGetPtr(DmOpenRef dbP);

extern Boolean FindNearLunar(DateType *dt, DateType base, Boolean leapyes);
extern Int16 DateCompare (DateType d1, DateType d2); // defined in datebook.c

extern void PrvMoveObject(FormPtr frmP, UInt16 objIndex, Coord y_diff, Boolean draw) SECT1;
extern void PrvResizeObject(FormPtr frmP, UInt16 objIndex, Coord y_diff, Boolean draw) SECT1;

Boolean TextMenuHandleEvent(UInt16 menuID, UInt16 objectID);

Err ResizeSilk(UInt16 silkRefNum, UInt16 size) SECT1;
Err EnableSilkResize(UInt16 silkRefNum, UInt16 state) SECT1;
UInt16 GetSilkPos(UInt16 silkRefNum) SECT1;



// Extract the bit at position index from bitfield.  0 is the high bit.
#define BitAtPosition(pos)                ((UInt32)1 << (pos))
#define GetBitMacro(bitfield, index)      ((bitfield) & BitAtPosition(index))
#define SetBitMacro(bitfield, index)      ((bitfield) |= BitAtPosition(index))
#define RemoveBitMacro(bitfield, index)   ((bitfield) &= ~BitAtPosition(index))

#endif
