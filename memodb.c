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

// Set this to get to private database defines
#define __MEMOMGR_PRIVATE__

#include <Pilot.h>
#include "memodb.h"



Err MemoNewRecord (DmOpenRef dbP, MemoItemPtr item, UInt *index)
{
    Err 					result;
    ULong					size = 0;
    VoidHand				recordH;
    MemoDBRecordPtr	recordP;

    // Compute the size of the new memo record.
    size = StrLen (item->note);

    //  Allocate a chunk in the database for the new record.
    recordH = (VoidHand)DmNewHandle(dbP, size+1);
    if (recordH == NULL)
        return dmErrMemError;

    // Pack the the data into the new record.
    recordP = MemHandleLock (recordH);
    DmStrCopy(recordP, 0, item->note);

    MemPtrUnlock (recordP);

    // Insert the record.
    *index = dmMaxRecordIndex;
    result = DmAttachRecord(dbP, index, (Handle)recordH, 0);
    if (result)
        MemHandleFree(recordH);

    return result;
}

