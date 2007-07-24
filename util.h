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

extern UInt16    lunarRefNum;

Err EnableSilkResize(UInt16 silkRefNum, UInt16 state);
Err ResizeSilk(UInt16 silkRefNum, UInt16 size);

void * GetObjectPointer(FormPtr frm, UInt16 objID);
void PrvMoveObject(FormPtr frmP, UInt16 objIndex, 
                   Coord y_diff, Boolean draw);
void PrvResizeObject(FormPtr frmP, UInt16 objIndex, 
                     Coord y_diff, Boolean draw);
Char* DateToAsciiLong(UInt8 months, UInt8 days, Int16 year,
                      DateFormatType dateFormat, Char * pstring);
Boolean SelectCategoryPopup(DmOpenRef dbP, UInt16* selected,
                            UInt32 list, UInt32 trigger, Char *string);
FieldPtr SetFieldTextFromStr(UInt16 fieldID, Char * strP);
void DisplayCategory(UInt32 trigger, Char* string, Boolean redraw);
UInt16 GetSilkPos(UInt16 silkRefNum);
MemPtr AppInfoGetPtr(DmOpenRef dbP);
Boolean TextMenuHandleEvent(UInt16 menuID, UInt16 objectID);
FieldPtr ClearFieldText(UInt16 fieldID);
Boolean FindNearLunar(DateType *dt, DateType base, Boolean leapyes);
UInt32 Hash(Char* name1, Char* name2);

#endif
