/*
HappyDays - A Birthdate displayer for the PalmPilot
Copyright (C) 1999-2000 JaeMok Jeong

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

#include <Pilot.h>
#include <System/DataPrv.h>

extern void * GetObjectPointer(FormPtr frm, Word objID);
extern FieldPtr SetFieldTextFromHandle(Word fieldID, Handle txtH);
extern FieldPtr SetFieldTextFromStr(Word fieldID, CharPtr strP);
extern FieldPtr ClearFieldText(Word fieldID);
extern CharPtr DateToAsciiLong(Byte months, Byte days, Int year,
                            DateFormatType dateFormat, CharPtr pstring);

extern VoidPtr AppInfoGetPtr(DmOpenRef dbP);

Boolean FindNearLunar(DateType *dt, DateType base, Boolean leapyes);
Int DateCompare (DateType d1, DateType d2);     // defined in datebook.c

// Extract the bit at position index from bitfield.  0 is the high bit.
#define BitAtPosition(pos)                ((ULong)1 << (pos))
#define GetBitMacro(bitfield, index)      ((bitfield) & BitAtPosition(index))
#define SetBitMacro(bitfield, index)      ((bitfield) |= BitAtPosition(index))
#define RemoveBitMacro(bitfield, index)   ((bitfield) &= ~BitAtPosition(index))

#endif
