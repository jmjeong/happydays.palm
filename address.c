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

/*
 * Just so you know some of these routines are pulled from the Address
 * source code as provided by Palm.  At least that is what I started from,
 * I may have touched a few lines to get it to compile with gcc without
 * warnings.
 */

#include <PalmOS.h>

#include "happydaysRsc.h"
#include "happydays.h"
#include "address.h"
#include "util.h"

char *UnnamedRecordStringPtr="";

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
Boolean
DetermineRecordName(AddrDBRecordPtr recordP, 
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

/***********************************************************************
 *
 * FUNCTION:    DrawRecordName
 *
 * DESCRIPTION: Draws an address book record name.  It is used
 * for the list view and note view.
 *
 * PARAMETERS:  name1, name2 - first and seconds names to draw
 *              nameExtent - the space the names must be drawn in
 *              *x, y - where the names are drawn
 *
 * RETURNED:    x is set after the last char drawn
 *
 * REVISION HISTORY:
 *            Name        Date      Description
 *            ----        ----      -----------
 *            roger      6/20/95   Initial Revision
 *            frigino    970813    Rewritten. Now includes a variable ratio for
 *                             name1/name2 width allocation, a prioritization
 *                             parameter, and a word break search to allow
 *                             reclaiming of space from the low priority
 *                             name.
 *            jmjeong    99/12/22  Abridged version. make the num of
 *                                     parameters small
 *
 ***********************************************************************/

void DrawRecordName(
    char* name1, char* name2,
    UInt16 nameExtent, Int16 *x, Int16 y,
    Boolean center, Boolean priorityIsName1)
{
    Int16 name1MaxWidth;
    Int16 name2MaxWidth;
    Boolean ignored;
    Int16 totalWidth;
    Char * lowPriName;
    Int16 highPriNameWidth;
    Int16 lowPriNameWidth;
    Int16 highPriMaxWidth;
    Int16 lowPriMaxWidth;
    Char * spaceP;
    UInt16 shortenedFieldWidth;
    UInt16 fieldSeparatorWidth;
    Int16 name1Length, name1Width;
    Int16 name2Length, name2Width;

    shortenedFieldWidth =  FntCharsWidth(shortenedFieldString, 
                                         shortenedFieldLength);
    fieldSeparatorWidth = FntCharsWidth(fieldSeparatorString, 
                                        fieldSeparatorLength);

    if (*name1) {
        /* Only show text from the first line in the field */
        name1Length = nameExtent; /* longer than possible */
        name1Width = nameExtent;  /* wider than possible */
        FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
    } else {
        /* Set the name to the unnamed string */
        name1 = UnnamedRecordStringPtr;
        name1Length = StrLen(UnnamedRecordStringPtr);
        name1Width = FntCharsWidth(UnnamedRecordStringPtr,
                                   name1Length);
    }
      
    if (*name2) {
        /* Only show text from the first line in the field */
        name2Length = nameExtent; /* longer than possible */
        name2Width = nameExtent;  /* wider than possible */
        FntCharsInWidth(name2, &name2Width, &name2Length, &ignored);
    } else {
        name2Length = 0;
        name2Width = 0;
    }

    /* Check if both names fit */
    totalWidth = name1Width + (*name2 ? fieldSeparatorWidth : 0) + name2Width;

    /*
     * If we are supposed to center the names then move in the x position
     * by the amount that centers the text
     */
    if (center && (nameExtent > totalWidth)) {
        *x += (nameExtent - totalWidth) / 2;
    }

    /* Special case if only name1 is given */
    if (!*name2) {
        /*
         * For some reason, OS3 changed the code so that it doesn't show
         * ellipses if there is only name1.  I liked it the old way!
         */
        /* Does name1 fit in its maximum width? */
        if (name1Width > nameExtent) {
            /* No. Draw it to max width minus the ellipsis */
            name1Width = nameExtent-shortenedFieldWidth;
            FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
            WinDrawChars(name1, name1Length, *x, y);
            *x += name1Width;
            /* Draw ellipsis */
            WinDrawChars(shortenedFieldString, shortenedFieldLength, *x, y);
            *x += shortenedFieldWidth;
        } else {
            /* Yes. Draw name1 within its width */
            FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
            WinDrawChars(name1, name1Length, *x, y);
            *x += name1Width;
        }
        return;
    }

    /* Remove name separator width */
    nameExtent -= fieldSeparatorWidth;

    /* Test if both names fit */
    if ((name1Width + name2Width) <= nameExtent) {
        name1MaxWidth = name1Width;
        name2MaxWidth = name2Width;
    } else {
        /*
         * They dont fit. One or both needs truncation
         * Establish name priorities and their allowed widths
         * Change this to alter the ratio of the low and high
         * priority name spaces
         */
        highPriMaxWidth = (nameExtent << 1)/3; /* 1/3 to low and 2/3 to high */
        lowPriMaxWidth = nameExtent-highPriMaxWidth;

        /* Save working copies of names and widths based on priority */
        if (priorityIsName1) {
            /* Priority is name1 */
            highPriNameWidth = name1Width;
            lowPriName = name2;
            lowPriNameWidth = name2Width;
        } else {
            /* Priority is name2 */
            highPriNameWidth = name2Width;
            lowPriName = name1;
            lowPriNameWidth = name1Width;
        }

        /* Does high priority name fit in high priority max width? */
        if (highPriNameWidth > highPriMaxWidth) {
            /* No. Look for word break in low priority name */
            spaceP = StrChr(lowPriName, spaceChr);
            if (spaceP != NULL) {
                /* Found break. Set low priority name width to break width */
                lowPriNameWidth = FntCharsWidth(lowPriName, spaceP-lowPriName);
                /*
                 * Reclaim width from low pri name width to low pri max width,
                 * if smaller.
                 */
                if (lowPriNameWidth < lowPriMaxWidth) {
                    lowPriMaxWidth = lowPriNameWidth;
                    /* Set new high pri max width */
                    highPriMaxWidth = nameExtent-lowPriMaxWidth;
                }
            }
        } else {
            /* Yes. Adjust maximum widths */
            highPriMaxWidth = highPriNameWidth;
            lowPriMaxWidth = nameExtent-highPriMaxWidth;
        }

        /* Convert priority widths back to name widths */
        if (priorityIsName1) {
            /* Priority is name1 */
            name1Width = highPriNameWidth;
            name2Width = lowPriNameWidth;
            name1MaxWidth = highPriMaxWidth;
            name2MaxWidth = lowPriMaxWidth;
        } else {
            /* Priority is name2 */
            name1Width = lowPriNameWidth;
            name2Width = highPriNameWidth;
            name1MaxWidth = lowPriMaxWidth;
            name2MaxWidth = highPriMaxWidth;
        }
    }

    /* Does name1 fit in its maximum width? */
    if (name1Width > name1MaxWidth) {
        /* No. Draw it to max width minus the ellipsis */
        name1Width = name1MaxWidth-shortenedFieldWidth;
        FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
        WinDrawChars(name1, name1Length, *x, y);
        *x += name1Width;

        /* Draw ellipsis */
        WinDrawChars(shortenedFieldString, shortenedFieldLength, *x, y);
        *x += shortenedFieldWidth;
    } else {
        /* Yes. Draw name1 within its width */
        FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
        WinDrawChars(name1, name1Length, *x, y);
        *x += name1Width;
    }

    if (*name1 && *name2) {
        /* Draw name separator */
        WinDrawChars(fieldSeparatorString, fieldSeparatorLength, *x, y);
        *x += fieldSeparatorWidth;
    }
    
    /* Draw name2 within its maximum width */
    FntCharsInWidth(name2, &name2MaxWidth, &name2Length, &ignored);
    WinDrawChars(name2, name2Length, *x, y);
    *x += name2MaxWidth;
}

// address creator id
// the order must be same as 'happydays.rcp' file.
//
UInt32 AddrGotoCreatorId[] = {
    'addr',         // Built-in Addressbook
    'adKr',         // Address Kr
	'AAlb', 		// Address Album
    'CiAe',         // AddressBkR
	'addp', 		// Address Plus(+)
    'SNms',         // Super Names
    'TlPh'          // Teal Phone
};

Int16 GotoAddress(Int16 index)
{
    GoToParamsPtr theGotoPointer;
    DmSearchStateType searchInfo;
    UInt16 cardNo;
    LocalID dbID;
    UInt32 addrID = AddrGotoCreatorId[(int)gPrefsR.BirthPrefs.addrapp];

    theGotoPointer = MemPtrNew(sizeof(GoToParamsType));
    if (!theGotoPointer) return -1;
    /* Set the owner of the pointer to be the
       system. This is required because all memory
       allocated by our application is freed when
       the application quits. Our application will
       quit the next time through our event
       handler.
    */
    
    
    if ((MemPtrSetOwner(theGotoPointer, 0) == 0) &&
        (DmGetNextDatabaseByTypeCreator(true, &searchInfo, 0,
                                        addrID, true, &cardNo, &dbID)
         == 0)) {

        // copy all the goto information into the
        // GotoParamsPtr structure
        theGotoPointer->searchStrLen = 0;
        theGotoPointer->dbCardNo = cardNo;
        theGotoPointer->dbID = dbID;
        theGotoPointer->recordNum = index;
        theGotoPointer->matchPos = 0;
        theGotoPointer->matchFieldNum = 0;
        theGotoPointer->matchCustom = 0;

        if ((DmGetNextDatabaseByTypeCreator
             (true, &searchInfo,
            sysFileTApplication, addrID, true, &cardNo, &dbID) == 0)) {
            SysUIAppSwitch(cardNo, dbID,
                           sysAppLaunchCmdGoTo,
                           (MemPtr) theGotoPointer);
            return 0;
        }
    }

	MemPtrFree(theGotoPointer);
    return -1;
}
