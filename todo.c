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
 * Just so you know some of these routines are pulled from the Todo
 * source code as provided by Palm.  At least that is what I started from,
 * I may have touched a few lines to get it to compile with gcc without
 * warnings.
 */

#include <PalmOS.h>
#include "todo.h"
#include "util.h"

#include "happydays.h"
#include "happydaysRsc.h"

/***********************************************************************
 *
 * FUNCTION:    GetToDoNotePtr
 *
 * DESCRIPTION: This routine returns a pointer to the note field in a to 
 *              do record.
 *
 * PARAMETERS:  recordP - pointer to a ToDo record
 *
 * RETURNED:    pointer to a null-terminated note
 *
 * REVISION HISTORY:
 *          Name    Date        Description
 *          ----    ----        -----------
 *          art 4/15/95 Initial Revision
 *
 ***********************************************************************/
Char* GetToDoNotePtr (ToDoDBRecordPtr recordP)
{
    return (&recordP->description + StrLen (&recordP->description) + 1);
}

/************************************************************
 *
 *  FUNCTION: DateTypeCmp
 *
 *  DESCRIPTION: Compare two dates
 *
 *  PARAMETERS: 
 *
 *  RETURNS: 
 *
 *  CREATED: 1/20/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static Int16 DateTypeCmp(DateType d1, DateType d2)
{
    Int16 result;
    
    result = d1.year - d2.year;
    if (result != 0)
    {
        if (DateToInt(d1) == 0xffff)
            return 1;
        if (DateToInt(d2) == 0xffff)
            return -1;
        return result;
    }
    
    result = d1.month - d2.month;
    if (result != 0)
        return result;
    
    result = d1.day - d2.day;

    return result;

}

/************************************************************
 *
 *  FUNCTION: ToDoGetSortOrder
 *
 *  DESCRIPTION: This routine gets the sort order value from the 
 *               'to do' application info block.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS:    true - if the 'to do' record are sorted by priority, 
 *              false - if the records are sorted by due date.
 *
 * REVISION HISTORY:
 *          Name    Date        Description
 *          ----    ----        -----------
 *          art 5/12/95 Initial Revision
 *       art    3/22/96 Rename routine and added more sort orders
 *
 *************************************************************/
UInt8 ToDoGetSortOrder (DmOpenRef dbP)
{
    UInt8 sortOrder;
    ToDoAppInfoPtr appInfoP;
            
    appInfoP = MemHandleLock (ToDoGetAppInfo (dbP));
    sortOrder = appInfoP->sortOrder;
    MemPtrUnlock (appInfoP);    

    return (sortOrder);
}

/************************************************************
 *
 *  FUNCTION: ToDoGetAppInfo
 *
 *  DESCRIPTION: Get the app info chunk 
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: MemHandle to the to do application info block (ToDoAppInfoType)
 *
 *  CREATED: 5/12/95 
 *
 *  BY: Art Lamb
 *
 *************************************************************/
MemHandle ToDoGetAppInfo (DmOpenRef dbP)
{
    Err error;
    UInt16 cardNo;
    LocalID dbID;
    LocalID appInfoID;
    
    error = DmOpenDatabaseInfo (dbP, &dbID, NULL, NULL, &cardNo, NULL);
    ErrFatalDisplayIf (error,  "Get getting to do app info block");

    error = DmDatabaseInfo (cardNo, dbID, NULL, NULL, NULL, NULL, NULL, 
                            NULL, NULL, &appInfoID, NULL, NULL, NULL);
    ErrFatalDisplayIf (error,  "Get getting to do app info block");

    return ((MemHandle) MemLocalIDToGlobal (appInfoID, cardNo));
}


/************************************************************
 *
 *  FUNCTION: CategoryCompare
 *
 *  DESCRIPTION: Compare two categories
 *
 *  PARAMETERS:  attr1, attr2 - record attributes, which contain 
 *                        the category. 
 *                    appInfoH - MemHandle of the applications category info
 *
 *  RETURNS: 0 if they match, non-zero if not
 *               + if s1 > s2
 *               - if s1 < s2
 *
 * REVISION HISTORY:
 *          Name    Date        Description
 *          ----    ----        -----------
 *          art 8/5/96  Initial Revision
 *
 *************************************************************/
static Int16 CategoryCompare (UInt8 attr1, UInt8 attr2, MemHandle appInfoH)
{
    Int16 result;
    UInt8 category1;
    UInt8 category2;
    ToDoAppInfoPtr appInfoP;
    
    
    category1 = attr1 & dmRecAttrCategoryMask;
    category2 = attr2 & dmRecAttrCategoryMask;

    result = category1 - category2;
    if (result != 0)
    {
        if (category1 == dmUnfiledCategory)
            return (1);
        else if (category2 == dmUnfiledCategory)
            return (-1);
        
        appInfoP = MemHandleLock (appInfoH);
        result = StrCompare (appInfoP->categoryLabels[category1],
                             appInfoP->categoryLabels[category2]);

        MemPtrUnlock (appInfoP);
        return result;
    }
    
    return result;
}

/************************************************************
 *
 *  FUNCTION: ToDoCompareRecords
 *
 *  DESCRIPTION: Compare two records.
 *
 *  PARAMETERS: database record 1
 *              database record 2
 *
 *  RETURNS: -n if record one is less (n != 0)
 *            n if record two is less
 *
 *  CREATED: 1/23/95 
 *
 *  BY: Roger Flores
 *
 *  COMMENTS:   Compare the two records key by key until
 *  there is a difference.  Return -n if r1 is less or n if r2
 *  is less.  A zero is never returned because if two records
 *  seem identical then their unique IDs are compared!
 *
 * This function accepts record data chunk pointers to avoid
 * requiring that the record be within the database.  This is
 * important when adding records to a database.  This prevents
 * determining if a record is a deleted record (which are kept
 * at the end of the database and should be considered "greater").
 * The caller should test for deleted records before calling this
 * function!
 *
 *************************************************************/
 
static Int16 ToDoCompareRecords(ToDoDBRecordPtr r1, 
                                ToDoDBRecordPtr r2, Int16 sortOrder, 
                                SortRecordInfoPtr info1, SortRecordInfoPtr info2,
                                MemHandle appInfoH)
{
    Int16 result = 0;

    // Sort by priority, due date, and category.
    if (sortOrder == soPriorityDueDate)
    {
        result = (r1->priority & priorityOnly) - (r2->priority & priorityOnly);
        if (result == 0)
        {
            result = DateTypeCmp (r1->dueDate, r2->dueDate);
            if (result == 0)
            {
                result = CategoryCompare (info1->attributes, info2->attributes, appInfoH);
            }
        }
    }

    // Sort by due date, priority, and category.
    else if (sortOrder == soDueDatePriority)
    {
        result = DateTypeCmp(r1->dueDate, r2->dueDate);
        if (result == 0)
        {
            result = (r1->priority & priorityOnly) - (r2->priority & priorityOnly);
            if (result == 0)
            {
                result = CategoryCompare (info1->attributes, info2->attributes, appInfoH);
            }
        }
    }
    
    // Sort by category, priority, due date
    else if (sortOrder == soCategoryPriority)
    {
        result = CategoryCompare (info1->attributes, info2->attributes, appInfoH);
        if (result == 0)
        {
            result = (r1->priority & priorityOnly) - (r2->priority & priorityOnly);
            if (result == 0)
            {
                result = DateTypeCmp (r1->dueDate, r2->dueDate);
            }
        }
    }

    // Sort by category, due date, priority
    else if (sortOrder == soCategoryDueDate)
    {
        result = CategoryCompare (info1->attributes, info2->attributes, appInfoH);
        if (result == 0)
        {
            result = DateTypeCmp (r1->dueDate, r2->dueDate);
            if (result == 0)
            {
                result = (r1->priority & priorityOnly) - (r2->priority & priorityOnly);
            }
        }
    }
    
    return result;
}

/************************************************************
 *
 *  FUNCTION: ToDoFindSortPosition
 *
 *  DESCRIPTION: Return where a record is or should be.
 *      Useful to find a record or find where to insert a record.
 *
 *  PARAMETERS: database record (not deleted!)
 *
 *  RETURNS: the size in bytes
 *
 *  CREATED: 1/11/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static UInt16 ToDoFindSortPosition(DmOpenRef dbP, ToDoDBRecord *newRecord, 
                                   SortRecordInfoPtr newRecordInfo)
{
    int sortOrder;
    
    sortOrder = ToDoGetSortOrder (dbP);
        
    return (DmFindSortPosition (dbP, newRecord, newRecordInfo, 
                                (DmComparF *)ToDoCompareRecords, sortOrder));
}

/************************************************************
 *
 *  FUNCTION: ToDoNewRecord
 *
 *  DESCRIPTION: Create a new record in sorted position
 *
 *  PARAMETERS: database pointer
 *              database record
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *
 *  CREATED: 1/10/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Err ToDoNewRecord(DmOpenRef dbP, ToDoItemPtr item, UInt16 category, UInt16 *index)
{
    Err                         err;
    UInt16                  size;
    Char                        zero=0;
    UInt16                  attr;
    UInt16                  newIndex;
    UInt32                  offset;
    ToDoDBRecordPtr     nilP=0;
    ToDoDBRecordPtr     recordP;
    MemHandle               recordH;
    SortRecordInfoType  sortInfo;

    // Compute the size of the new to do record.
    size = sizeDBRecord;
    if (item->description)
        size += StrLen (item->description);
    if (item->note)
        size += StrLen (item->note);
        

    //  Allocate a chunk in the database for the new record.
    recordH = (MemHandle)DmNewHandle(dbP, size);
    if (recordH == NULL)
        return dmErrMemError;

    // Pack the the data into the new record.
    recordP = MemHandleLock (recordH);
    DmWrite(recordP, (UInt32)&nilP->dueDate, &item->dueDate, sizeof(nilP->dueDate));
    DmWrite(recordP, (UInt32)&nilP->priority, &item->priority, sizeof(nilP->priority));

    offset = (UInt32)&nilP->description;
    if (item->description)
    {
        DmStrCopy(recordP, offset, item->description);
        offset += StrLen (item->description) + 1;
    }
    else
    {
        DmWrite(recordP, offset, &zero, 1);
        offset++;
    }
    
    if (item->note) 
        DmStrCopy(recordP, offset, item->note);
    else
        DmWrite(recordP, offset, &zero, 1);
    
    
    sortInfo.attributes = category;
    sortInfo.uniqueID[0] = 0;
    sortInfo.uniqueID[1] = 0;
    sortInfo.uniqueID[2] = 0;
    
    // Determine the sort position of the new record.
    newIndex = ToDoFindSortPosition (dbP, recordP, &sortInfo);

    MemPtrUnlock (recordP);

    // Insert the record.
    err = DmAttachRecord(dbP, &newIndex, recordH, 0);
    if (err) 
        MemHandleFree(recordH);
    else
    {
        *index = newIndex;

        // Set the category.
        DmRecordInfo (dbP, newIndex, &attr, NULL, NULL);
        attr &= ~dmRecAttrCategoryMask;
        attr |= category;
        DmSetRecordInfo (dbP, newIndex, &attr, NULL);
    }

    return err;
}

