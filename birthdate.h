/*
HappyDays - A Birthday displayer for the PalmPilot
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

#ifndef _BIRTHDATE_H_
#define _BIRTHDATE_H_

#include <PalmOS.h>

#include "happydays.h"
#include "section.h"

// custom field used Birthdate in AddressBook
//
extern Int16 gHappyDaysField;

void PackHappyDays(HappyDays *birthdate, void* recordP);
void UnpackHappyDays(HappyDays *birthdate,
                     const PackedHappyDays *packedHappyDays) ;

UInt16 AddrGetHappyDays(DmOpenRef dbP, 
					UInt16 AddrCategory, DateType start) ;
Boolean AnalysisHappyDays(const char* birthdate,
                          HappyDaysFlag *flag,
                          Int16* year, Int16* month, Int16* day) ;

Boolean UpdateHappyDaysDB(FormPtr frm) ;
void  SetReadAddressDB() ;

Boolean FindHappyDaysField() ;
Boolean IsChangedAddressDB() ;

#endif
