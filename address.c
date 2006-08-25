/*
HappyDays - A Birthday/Event displayer for the Palm
Copyright (C) 1999-2006 Jaemok Jeong

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


#include <PalmOS.h>

#include "happydaysRsc.h"
#include "happydays.h"
#include "address.h"
#include "util.h"

#define INDICATE_NUM        8

// Extract the bit at position index from bitfield.  0 is the high bit.
#define BitAtPosition(pos)                ((UInt32)1 << (pos))
#define GetBitMacro(bitfield, index)      ((bitfield) & BitAtPosition(index))
#define SetBitMacro(bitfield, index)      ((bitfield) |= BitAtPosition(index))
#define RemoveBitMacro(bitfield, index)   ((bitfield) &= ~BitAtPosition(index))

extern void initIndicate();
extern void displayNextIndicate( int index );
extern Int16 AnalizeOneRecord(UInt16 addrattr, Char* src, HappyDays* hd, Boolean *ignore);
extern void CleanupHappyDaysCache(DmOpenRef dbP);

void AddrUnpack(AddrPackedDBRecord *src, AddrDBRecordPtr dest) SECT2;
Boolean FindHappyDaysField() SECT2;
Boolean UpdateHappyDaysDB(FormPtr frm) SECT2;

static Boolean DetermineRecordName(AddrDBRecordPtr recordP, 
                                   Boolean sortByCompany,
                                   Char **name1,
                                   Char **name2) SECT2;


/************************************************************
 *
 *  FUNCTION: AddrUnpack
 *
 *  DESCRIPTION: Fills in the AddrDBRecord structure
 *
 *  PARAMETERS: address record to unpack
 *                the address record to unpack into
 *
 *  RETURNS: the record unpacked
 *
 *  CREATED: 1/14/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
void AddrUnpack(AddrPackedDBRecord *src, AddrDBRecordPtr dest)
{
    Int16 index;
    UInt32 flags;
    char *p;

    dest->options = src->options;
    flags = src->flags.allBits;
    p = &src->firstField;
         
    for (index=firstAddressField; index < addressFieldsCount; index++) {
        /* If the flag is set point to the string else NULL */
        if (GetBitMacro(flags, index) != 0) {
            dest->fields[index] = p;
            p += StrLen(p) + 1;
        } else dest->fields[index] = NULL;
    }
}

/***********************************************************************
 *
 * FUNCTION:    DetermineRecordName
 *
 * DESCRIPTION: Determines an address book record's name.  The name
 * varies based on which fields exist and what the sort order is.
 *
 * PARAMETERS:  recordP - address record
 *              sortByCompany - sort by company field
 *              
 * RETURNED:    name1, name2 - first and seconds names to draw
 *              Boolean - name1/name2 priority based on sortByCompany
 *
 * REVISION HISTORY:
 *            Name        Date        Description
 *            ----        ----        -----------
 *            roger        6/20/95    Initial Revision
 *            frigino      970813     Added priority return value
 *
 ***********************************************************************/
static Boolean DetermineRecordName(AddrDBRecordPtr recordP, 
                                   Boolean sortByCompany,
                                   Char **name1,
                                   Char **name2)
{
    UInt16 fieldNameChoiceList[4];
    UInt16 fieldNameChoice;
    Boolean name1HasPriority;

    *name1 = NULL;
    *name2 = NULL;

    if (sortByCompany) {
        /* When sorting by company, always treat name2 as priority. */
        name1HasPriority = false;
        fieldNameChoiceList[3] = addressFieldsCount;
        fieldNameChoiceList[2] = firstName;
        fieldNameChoiceList[1] = name;
        fieldNameChoiceList[0] = company;
        fieldNameChoice = 0;

        while ((*name1 == NULL) &&
               (fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)) {
            *name1 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
        }

        /*
         * When sorting by company, treat name2 as priority if we
         * succeed in getting the company name as the name1
         * Did we get the company name?
         */
        if (fieldNameChoice > 1) {
            /*
             * No. We got a last name, first name, or nothing.
             * Priority switches to name1
             */
            name1HasPriority = true;
        }

        while ((*name2 == NULL) &&
               (fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)) {
            *name2 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
        }
    } else {
        /* When not sorting by company, always treat name1 as priority. */
        name1HasPriority = true;

        fieldNameChoiceList[3] = addressFieldsCount;
        fieldNameChoiceList[2] = addressFieldsCount;
        fieldNameChoiceList[1] = firstName;
        fieldNameChoiceList[0] = name;
        fieldNameChoice = 0;

        while ((*name1 == NULL) &&
               (fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)) {
            *name1 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
        }

        if (*name1 == NULL) {
            *name1 = recordP->fields[company];
            *name2 = NULL;
        } else {
            while ((*name2 == NULL) &&
                   (fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)) {
                *name2 =
                    recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
            }
        }         
    }
    /* Return priority status */
    return name1HasPriority;
}

Boolean FindHappyDaysField()
{
    AddrAppInfoPtr addrInfoPtr;
    Int16 i;

    gHappyDaysField = -1;
    
    if ((addrInfoPtr = (AddrAppInfoPtr)AppInfoGetPtr(AddressDB))) {
        for (i= firstRenameableLabel; i <= lastRenameableLabel; i++) {
            if (!StrCaselessCompare(addrInfoPtr->fieldLabels[i],
                                    gPrefsR.custom)) {
                gHappyDaysField = i;
                break;
            }
        }
        MemPtrUnlock(addrInfoPtr);
    }
    if (gHappyDaysField < 0) {
        // Custom field에 일치하는 게 없는 경우에는 note field를 조사
        return true;
    }
    return true;
}


Boolean UpdateHappyDaysDB(FormPtr frm)
{
    UInt16 currIndex = 0;
    AddrPackedDBRecord *rp;
    AddrDBRecordType r;
    MemHandle recordH = 0;
    UInt16 recordNum;
    int i = 0, indicateNext;
    int step;

    // create the happydays cache db
    HappyDays   hd;
    Boolean     ignore = false;         // ignore error record
    Char*       hdField;
    UInt16      addrattr;
    Char        *p, *q, *end;

    // display collecting information
    //
    FrmDrawForm(frm);

    // clean up old database
    //
    CleanupHappyDaysCache(MainDB);

    recordNum = DmNumRecords(AddressDB);
    indicateNext = step = recordNum / INDICATE_NUM;

    if (recordNum > 50) initIndicate();
            
    while (1) {
        char *name1, *name2;
        Int8 whichField;        // birthday field or note field?
            
        recordH = DmQueryNextInCategory(AddressDB, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;
        if (i++ == indicateNext) {
            if (recordNum > 50) displayNextIndicate( (i-1) / step);
            indicateNext += step;
        }

        DmRecordInfo(AddressDB, currIndex, &addrattr, NULL, NULL);
        addrattr &= dmRecAttrCategoryMask;      // get category info
                
        rp = (AddrPackedDBRecord*)MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        AddrUnpack(rp, &r);

        if ((gHappyDaysField <= 0 || !r.fields[gHappyDaysField])
            && !(gPrefsR.scannote && r.fields[note]
                 && StrStr(r.fields[note], gPrefsR.notifywith) ) ) {
            // not exist in happydays field or note field
            //
            MemHandleUnlock(recordH);
            currIndex++;
            continue;
        }
			
        MemSet(&hd, sizeof(hd), 0);
        hd.addrRecordNum = currIndex;
        if (DetermineRecordName(&r, gSortByCompany,
                                &hd.name1,
                                &hd.name2)) {
            // name 1 has priority;
            hd.flag.bits.priority_name1 = 1;
        }

        // save the temporary name
        name1 = hd.name1;
        name2 = hd.name2;

        if (gHappyDaysField >= 0 && r.fields[gHappyDaysField])
            whichField = gHappyDaysField;
        else whichField = note;

        while (whichField >= 0) {

            if (whichField == note) {
                p = StrStr(r.fields[note], gPrefsR.notifywith)
                    + StrLen(gPrefsR.notifywith) + 1;

                if ( StrLen(r.fields[note]) < (p - r.fields[note]) ) break;
            }
            else {
                p = r.fields[whichField];
            }
                
            hdField =
                MemPtrNew(StrLen(r.fields[whichField]) - (p - r.fields[whichField])+1);

            SysCopyStringResource(gAppErrStr, NotEnoughMemoryString);
            ErrFatalDisplayIf(!hdField, gAppErrStr);
            
            p = StrCopy(hdField, p);

            if (whichField == note && 
                (end = StrStr(p, gPrefsR.notifywith))) {
                // end delimeter
                //
                *end = 0;
            }
            
            while ((q = StrChr(p, '\n'))) {
                // multiple event
                //

                *q = 0;
                if (AnalizeOneRecord(addrattr, p, &hd, &ignore)) 
					goto Update_ErrHandler;
                p = q+1;

                // restore the saved name
                hd.name1 = name1;
                hd.name2 = name2;
                
                // reset multiple flag
                hd.flag.bits.multiple_event = 0;

                while (*p == ' ' || *p == '\t' || *p == '\n')
                    p++;     // skip white space
            }
            // last record
            if (*p) {
                // check the null '\n'
                if (AnalizeOneRecord(addrattr, p, &hd, &ignore)) 
					goto Update_ErrHandler;
            }
                
            if (whichField == gHappyDaysField  // next is note field
                && (gPrefsR.scannote     // scanNote & exists
                    && r.fields[note]       
                    && StrStr(r.fields[note], gPrefsR.notifywith)) ) {
                whichField = note;
            }
            else whichField = -1;

            MemPtrFree(hdField);
        }
        MemHandleUnlock(recordH);
        currIndex++;
    }
    if (recordNum > 50) displayNextIndicate( INDICATE_NUM -1);
    
	return true;

Update_ErrHandler:
	MemPtrFree(hdField);
	MemHandleUnlock(recordH);
	return false;
}
