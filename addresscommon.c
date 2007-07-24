/*
HappyDays - A Birthday displayer for the Palm
Copyright (C) 1999-2005 JaeMok Jeong

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
#include "addresscommon.h"
#include "util.h"

char *UnnamedRecordStringPtr="";

// extern Int16 GotoAddress(Int16 index);
// called from Find function(must be in main section)

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
	'AAlb', 		// Address Album
    'CiAe',         // AddressBkR
	'addp', 		// Address Plus(+)
    'SNms',         // Super Names
    'TlPh',         // Teal Phone
	'HnAD'			// HanAD
};

Int16 GotoAddress(Int16 index)
{
    GoToParamsPtr theGotoPointer;
    DmSearchStateType searchInfo;
    UInt16 cardNo;
    LocalID dbID;
    UInt32 addrID = AddrGotoCreatorId[(int)gPrefsR.addrapp];

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
			/*
			 * return from address? (fatal?)
            SysAppLaunch(cardNo, dbID, sysAppLaunchFlagNewThread,
                           sysAppLaunchCmdGoTo,
                           (MemPtr) theGotoPointer, &ret);
			*/
            return 0;
        }
    }

	MemPtrFree(theGotoPointer);
    return -1;
}

void CleanupHappyDaysCache(DmOpenRef dbP)
{
    UInt16 currindex = 0;     
    MemHandle recordH = 0;

    if (dbP) {
        while (1) {
            recordH = DmQueryNextInCategory(dbP, &currindex,
                                            dmAllCategories);
            if (!recordH) break;
            
            DmRemoveRecord(dbP, currindex);     // remove all traces
        }
    }
}
