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
 * Just so you know these routines are pulled from the Memo source code
 * as provided by Palm.  At least that is what I started from, I may have
 * touched a few lines to get it to compile with gcc without warnings.
 */

// Set this to get to private database defines
#define __MEMOMGR_PRIVATE__

#include <PalmOS.h>
#include "memodb.h"



Err MemoNewRecord (DmOpenRef dbP, MemoItemPtr item, UInt16 *index)
{
    Err 					result;
    UInt32					size = 0;
    MemHandle				recordH;
    MemoDBRecordPtr	recordP;

    // Compute the size of the new memo record.
    size = StrLen (item->note);

    //  Allocate a chunk in the database for the new record.
    recordH = (MemHandle)DmNewHandle(dbP, size+1);
    if (recordH == NULL)
        return dmErrMemError;

    // Pack the the data into the new record.
    recordP = MemHandleLock (recordH);
    DmStrCopy(recordP, 0, item->note);

    MemPtrUnlock (recordP);

    // Insert the record.
    *index = dmMaxRecordIndex;
    result = DmAttachRecord(dbP, index, recordH, 0);
    if (result)
        MemHandleFree(recordH);

    return result;
}

