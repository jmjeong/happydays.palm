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

#ifndef _BIRTHDATE_H_
#define _BIRTHDATE_H_

#include <PalmOS.h>

#include "happydays.h"

// custom field used Birthdate in AddressBook
//
extern Int16 gBirthDateField;

void PackBirthdate(BirthDate *birthdate, void* recordP);
void UnpackBirthdate(BirthDate *birthdate,
                     const PackedBirthDate *packedBirthdate);

UInt16 AddrGetBirthdate(DmOpenRef dbP, UInt16 AddrCategory);
Boolean AnalysisBirthDate(const char* birthdate,
                          BirthdateFlag *flag,
                          Int16* year, Int16* month, Int16* day);

Int16 UpdateBirthdateDB(DmOpenRef dbP, FormPtr frm);



#endif
