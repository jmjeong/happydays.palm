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

#include <Pilot.h>
#include <System/DataPrv.h>
#include "util.h"
#include "calendar.h"

//
// GetObjectPointer - return an object pointer given a form and objID
//
void * GetObjectPointer(FormPtr frm, Word objID)
{
     return FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, objID));
}


//
//  SetFieldTextFromHandle - set txtH into field object text
//
FieldPtr SetFieldTextFromHandle(Word fieldID, Handle txtH)
{
   Handle      oldTxtH;
   FormPtr     frm = FrmGetActiveForm();
   FieldPtr    fldP;

   // get the field and the field's current text handle.
   fldP     = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldID));
   ErrNonFatalDisplayIf(!fldP, "missing field");
   oldTxtH  = FldGetTextHandle(fldP);
   
   // set the field's text to the new text.
   FldSetTextHandle(fldP, txtH);

   // free the handle AFTER we call FldSetTextHandle().
   if (oldTxtH) 
      MemHandleFree((VoidHand)oldTxtH);
   
   return fldP;
}

// Allocates new handle and copies incoming string
FieldPtr SetFieldTextFromStr(Word fieldID, CharPtr strP)
{
   VoidHand    txtH;
   
   // get some space in which to stash the string.
   txtH  = MemHandleNew(StrLen(strP) + 1);
   if (!txtH)
      return NULL;

   // copy the string to the locked handle.
   StrCopy((CharPtr)MemHandleLock(txtH), strP);

   // unlock the string handle.
   MemHandleUnlock(txtH);
   
   // set the field to the handle
   return SetFieldTextFromHandle(fieldID, (Handle)txtH);
}

FieldPtr ClearFieldText(Word fieldID)
{
    return SetFieldTextFromHandle(fieldID, NULL);
}

Boolean FindNearLunar(DateType *dt, DateType base, Boolean leapyes)
{
    int lunyear, lunmonth, lunday;
    DateTimeType rtVal;
    Int i;

    lunmonth = dt->month; lunday = dt->day;

    for (i = -1; i < 5; i++) {
        lunyear = base.year + 1904 + i;

        if  (lun2sol(lunyear, lunmonth, lunday, leapyes, &rtVal)) continue;
        
        dt->year = rtVal.year - 1904;
        dt->month = rtVal.month; dt->day = rtVal.day;

        if (DateCompare(base, *dt) <= 0) return true;
    }
    return false;
}

// if year is -1, ignore year field
//
Char* DateToAsciiLong(Byte months, Byte days, Int year,
                            DateFormatType dateFormat, CharPtr pstring)
{
    if (year >= 0) {
        switch (dateFormat) {
        case dfDMYWithSlashes:
            StrPrintF(pstring, "%d/%d/%d", days, months, year);
            break;
        case dfDMYWithDots:
            StrPrintF(pstring, "%d.%d.%d", days, months, year);
            break;
        case dfDMYWithDashes:
            StrPrintF(pstring, "%d-%d-%d", days, months, year);
            break;
        case dfYMDWithSlashes:
            StrPrintF(pstring, "%d/%d/%d", year, months, days);
            break;
        case dfYMDWithDots:
            StrPrintF(pstring, "%d.%d.%d", year, months, days);
            break;
        case dfYMDWithDashes:
            StrPrintF(pstring, "%d-%d-%d", year, months, days);
            break;
        case dfMDYWithSlashes:
        default:
            StrPrintF(pstring, "%d/%d/%d", months, days, year);
            break;
        }
    }
    else {      // no year
        switch (dateFormat) {
        case dfDMYWithSlashes:
            StrPrintF(pstring, "%d/%d", days, months);
            break;
        case dfDMYWithDots:
            StrPrintF(pstring, "%d.%d.", days, months);
            break;
        case dfDMYWithDashes:
            StrPrintF(pstring, "%d-%d", days, months);
            break;
        case dfYMDWithSlashes:
            StrPrintF(pstring, "%d/%d", months, days);
            break;
        case dfYMDWithDots:
            StrPrintF(pstring, "%d.%d", months, days);
            break;
        case dfYMDWithDashes:
            StrPrintF(pstring, "%d-%d", months, days);
            break;
        case dfMDYWithSlashes:
        default:
            StrPrintF(pstring, "%d/%d", months, days);
            break;
        }
    }
    return pstring;
}

/************************************************************
 *
 *  FUNCTION: AppInfoGetPtr
 *  DESCRIPTION: Return a locked pointer to the AppInfo or NULL
 *  PARAMETERS: dbP - open database pointer
 *  RETURNS: locked ptr to the AppInfo or NULL
 *
 *************************************************************/
VoidPtr AppInfoGetPtr(DmOpenRef dbP)
{
    UInt       cardNo;
    LocalID    dbID;
    LocalID    appInfoID;
   
    if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
        return NULL;
    if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL,
                       NULL, &appInfoID, NULL, NULL, NULL))
        return NULL;

    if (appInfoID == NULL)
        return NULL;
    else
        return MemLocalIDToLockedPtr(appInfoID, cardNo);
}   

