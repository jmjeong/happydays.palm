#define __TODOMGR_PRIVATE__

#include <PalmOS.h>
#include "happydays.h"
#include "newtodo.h"
#include "util.h"
#include "section.h"

#define TODO_SORT_OTHER_MAKE( sort, filter, subfilter )	((((sort) & 0x0F) << 8) | (((filter) & 0x0F) << 4) |(((subfilter) & 0x0F)))
#define TODO_SORT_OTHER_GET_SORT_ORDER( other )	(((other) >> 8) & 0x0F)
#define TODO_SORT_OTHER_GET_FILTER( other )		(((other) >> 4) & 0x0F)
#define TODO_SORT_OTHER_GET_SUBFILTER( other )	(((other) & 0x0F))

Int16 NewActualPerformNotifyTD(HappyDays birth, DateType when, Int8 age,
                               Int16 *created, Int16 *touched, Int16 existIndex) SECT2;
UInt8 NewToDoGetSortOrder (DmOpenRef dbP) SECT2;
Err ToDoDBRecordSetDueDate( DmOpenRef dbP, UInt16 index, DateType *dueDateP ) SECT2;
Err ToDoDBRecordClearDueDate( DmOpenRef dbP, UInt16 index ) SECT2;

extern Int16 FindEventNoteInfo(HappyDays birth);
extern void LoadEventNoteString(char *p, Int16 idx, Int16 maxString);
extern Char* NotifyDescString(DateType when, HappyDays birth, 
							  Int8 age, Boolean todo);
extern void ChkNMakePrivateRecord(DmOpenRef db, Int16 index);
extern Boolean IsSameRecord(Char* notefield, HappyDays birth);
extern Boolean IsHappyDaysRecord(Char* notefield);


static Char *ToDoDBRecordGetFieldPointer( ToDoDBRecordPtr recordP,
		ToDoRecordFieldType field ) SECT2;
static Err ToDoDBRecordSetOptionalField( DmOpenRef dbP, UInt16 index,
                                         ToDoRecordFieldType field, void *fieldDataP, UInt32 fieldDataSize ) SECT2;
static MemHandle NewToDoGetAppInfo (DmOpenRef dbP) SECT2;
static Int16 DateTypeCmp(DateType d1, DateType d2) SECT2;
static Int16 CategoryCompare (UInt8 attr1, UInt8 attr2, MemHandle appInfoH) SECT2;
static Int16 ToDoCompareRecords( ToDoDBRecordPtr r1,  ToDoDBRecordPtr r2,
                                 Int16 sortOther, SortRecordInfoPtr info1, SortRecordInfoPtr info2,
                                 MemHandle appInfoH ) SECT2;
static UInt16 ToDoFindSortPosition(DmOpenRef dbP, ToDoDBRecord *newRecord, 
                                   SortRecordInfoPtr newRecordInfo, UInt16 filter, UInt16 subFilter ) SECT2;


#define maxDescLen	256
#define maxNoteLen	maxFieldTextLen             //was 4096, now is 32,767

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
 *  FUNCTION: CategoryCompare
 *
 *  DESCRIPTION: Compare two categories
 *
 *  PARAMETERS:  attr1, attr2 - record attributes, which contain 
 *						  the category. 
 *					  appInfoH - MemHandle of the applications category info
 *
 *  RETURNS: 0 if they match, non-zero if not
 *				 + if s1 > s2
 *				 - if s1 < s2
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/5/96	Initial Revision
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
 *				database record 2
 *
 *  RETURNS: -n if record one is less (n != 0)
 *			  n if record two is less
 *
 *  CREATED: 1/23/95 
 *
 *  BY: Roger Flores
 *
 *	COMMENTS:	Compare the two records key by key until
 *	there is a difference.  Return -n if r1 is less or n if r2
 *	is less.  A zero is never returned because if two records
 *	seem identical then their unique IDs are compared!
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
 
static Int16 ToDoCompareRecords( ToDoDBRecordPtr r1,  ToDoDBRecordPtr r2,
	Int16 sortOther, SortRecordInfoPtr info1, SortRecordInfoPtr info2,
	MemHandle appInfoH )
{
	DateType	*dateP;
	DateType	r1DueDate, r2DueDate;
	Int16		sortOrder, filter, subFilter;
	Int16		result = 0;

	sortOrder	= TODO_SORT_OTHER_GET_SORT_ORDER( sortOther );
	filter		= TODO_SORT_OTHER_GET_FILTER( sortOther );
	subFilter	= TODO_SORT_OTHER_GET_SUBFILTER( sortOther );

#if 1
	dateP = ( DatePtr ) ToDoDBRecordGetFieldPointer( r1, toDoRecordFieldDueDate );
	if ( dateP )
		{
		r1DueDate = *dateP;
		}
	else
		{
		dateP = ( DatePtr ) ToDoDBRecordGetFieldPointer( r1,
			toDoRecordFieldCompletionDate );
		if ( dateP )
			{
			r1DueDate = *dateP;
			}
		else
			{
			DateToInt( r1DueDate ) = toDoNoDueDate;
			}
		}

	dateP = ( DatePtr ) ToDoDBRecordGetFieldPointer( r2, toDoRecordFieldDueDate );
	if ( dateP )
		{
		r2DueDate = *dateP;
		}
	else
		{
		dateP = ( DatePtr ) ToDoDBRecordGetFieldPointer( r2,
			toDoRecordFieldCompletionDate );
		if ( dateP )
			{
			r2DueDate = *dateP;
			}
		else
			{
			DateToInt( r2DueDate ) = toDoNoDueDate;
			}
		}
#else
	DateSecondsToDate( TimGetSeconds(), &today );

	if ( errNone != ToDoDBRecordGetDueDate( r1, filter, subFilter,
			today, &r1DueDate ) )
		{
		DateToInt( r1DueDate ) = toDoNoDueDate;
		}

	if ( errNone != ToDoDBRecordGetDisplayedDueDate( r2, filter, subFilter,
			today, &r2DueDate ) )
		{
		DateToInt( r2DueDate ) = toDoNoDueDate;
		}
#endif

	// Sort by priority, due date, and category.
	if (sortOrder == soPriorityDueDate)
		{
		result = (r1->priority) - (r2->priority);
		if (result == 0)
			{
			result = DateTypeCmp( r1DueDate, r2DueDate );
			if (result == 0)
				{
				result = CategoryCompare (info1->attributes, info2->attributes, appInfoH);
				}
			}
		}

	// Sort by due date, priority, and category.
	else if (sortOrder == soDueDatePriority)
		{
		result = DateTypeCmp( r1DueDate, r2DueDate );
		if (result == 0)
			{
			result = (r1->priority) - (r2->priority);
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
			result = (r1->priority) - (r2->priority);
			if (result == 0)
				{
				result = DateTypeCmp( r1DueDate, r2DueDate );
				}
			}
		}

	// Sort by category, due date, priority
	else if (sortOrder == soCategoryDueDate)
		{
		result = CategoryCompare (info1->attributes, info2->attributes, appInfoH);
		if (result == 0)
			{
			result = DateTypeCmp( r1DueDate, r2DueDate );
			if (result == 0)
				{
				result = (r1->priority) - (r2->priority);
				}
			}
		}
	
	return result;
}

/************************************************************
 *
 *  FUNCTION: NewToDoGetSortOrder
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
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/12/95	Initial Revision
 *       art	3/22/96	Rename routine and added more sort orders
 *
 *************************************************************/
UInt8 NewToDoGetSortOrder (DmOpenRef dbP)
{
	UInt8 sortOrder;
	ToDoAppInfoPtr appInfoP;
			
	appInfoP = MemHandleLock (NewToDoGetAppInfo (dbP));
	sortOrder = appInfoP->sortOrder;
	MemPtrUnlock (appInfoP);	

	return (sortOrder);
}

/************************************************************
 *
 *  FUNCTION: ToDoFindSortPosition
 *
 *  DESCRIPTION: Return where a record is or should be.
 *		Useful to find a record or find where to insert a record.
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
		SortRecordInfoPtr newRecordInfo, UInt16 filter, UInt16 subFilter )
{
	Int16	sortOther;
	UInt8	sortOrder;
	
	sortOrder = NewToDoGetSortOrder (dbP);
	
	sortOther = TODO_SORT_OTHER_MAKE( sortOrder, filter, subFilter );
		
	return (DmFindSortPosition (dbP, newRecord, newRecordInfo, 
		(DmComparF *)ToDoCompareRecords, sortOther));
}

/******************************************************************************
 *
 * FUNCTION:	ToDoDBRecordSetDueDate
 *
 * DESCRIPTION:	Set or add due date on the specified record
 *
 * PARAMETERS:	dbP -			database pointer
 *				index -			record index
 *				dueDate -		due date
 *
 * RETURNED:	errNone if successful, error code otherwise
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	CS2		03/13/03	Initial Revision
 *
 *****************************************************************************/
Err ToDoDBRecordSetDueDate( DmOpenRef dbP, UInt16 index, DateType *dueDateP )
{
	return ToDoDBRecordSetOptionalField( dbP, index, toDoRecordFieldDueDate,
		dueDateP, sizeof( DateType ) );
}

static Err ToDoDBRecordSetOptionalField( DmOpenRef dbP, UInt16 index,
		ToDoRecordFieldType field, void *fieldDataP, UInt32 fieldDataSize )
{
	MemHandle	recordH;
	Err			ret;

	recordH = DmQueryRecord( dbP, index );
	if ( recordH )
	{
		ToDoDBRecordPtr	recordP;
		Char			*src, *dest;
		UInt32			offset, numBytes;
		
		recordP = ( ToDoDBRecordPtr ) MemHandleLock( recordH );
		
		// if the record doesn't already have the field, make room first
		if ( ((toDoRecordFieldDueDate == field) &&
			  !recordP->dataFlags.dueDate) ||
			 ((toDoRecordFieldCompletionDate == field) &&
			  !recordP->dataFlags.completionDate) ||
			 ((toDoRecordFieldAlarm == field) &&
			  !recordP->dataFlags.alarm) ||
			 ((toDoRecordFieldRepeat == field) &&
			  !recordP->dataFlags.repeat) )
		{
			MemHandleUnlock( recordH );
			
			numBytes = MemHandleSize( recordH ) + fieldDataSize;

			ret = MemHandleResize( recordH, numBytes );
			
			if ( errNone == ret )
			{
				ToDoDBDataFlags	newFlags;

				recordP = ( ToDoDBRecordPtr ) MemHandleLock( recordH );
				
				// set new field's flag
				newFlags = recordP->dataFlags;

				if ( toDoRecordFieldDueDate == field )
				{
					newFlags.dueDate = 1;
				}
				else if ( toDoRecordFieldCompletionDate == field )
				{
					newFlags.completionDate = 1;
				}
				else if ( toDoRecordFieldAlarm == field )
				{
					newFlags.alarm = 1;
				}
				else if ( toDoRecordFieldRepeat == field )
				{
					newFlags.repeat = 1;
				}

				DmWrite( recordP, OffsetOf( ToDoDBRecord, dataFlags ),
					&newFlags, sizeof( ToDoDBDataFlags ) );
				
				// move the rest of the data down
				
				// it will move FROM where the field will now be
				src = ToDoDBRecordGetFieldPointer( recordP, field );

				// it will move TO just after the new field
				dest = src + fieldDataSize;

				// how many bytes to move?
				// numBytes is new size of record.
				// to calculate the number of bytes, we must get back to
				// the old size of the record
				numBytes -= fieldDataSize;
				// and then we must subtract off the part that won't move
				numBytes -= (src - (( Char * ) recordP ));

				offset = dest - (( Char * ) recordP );
				DmWrite( recordP, offset, src, numBytes );
			}
		}
		else
		{
			ret = errNone;
		}
		
		if ( errNone == ret )
		{
			dest = ToDoDBRecordGetFieldPointer( recordP, field );
			
			// write new field data into record
			offset = dest - (( Char * ) recordP);
			DmWrite( recordP, offset, fieldDataP, fieldDataSize );
		}
		
		MemHandleUnlock( recordH );
	}
	else
	{
		ret = dmErrNotValidRecord;
	}

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	ECToDoDBValidate( dbP );
#endif

	return ret;
}

/************************************************************
 *
 *  FUNCTION: NewToDoGetAppInfo
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
MemHandle NewToDoGetAppInfo (DmOpenRef dbP)
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

/***********************************************************************
 *
 * FUNCTION:    GetToDoDescriptionPtr
 *
 * DESCRIPTION: This routine returns a pointer to the description field in a to 
 *              do record.
 *
 * PARAMETERS:  recordP - pointer to a ToDo record
 *
 * RETURNED:    pointer to a null-terminated description
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	CS2		01/15/03	Initial Revision
 *
 ***********************************************************************/
Char* GetToDoDescriptionPtr (ToDoDBRecordPtr recordP)
{
	return ToDoDBRecordGetFieldPointer( recordP, toDoRecordFieldDescription );
}


/***********************************************************************
 *
 * FUNCTION:    NewGetToDoNotePtr
 *
 * DESCRIPTION: This routine returns a pointer to the note field in a to 
 *              do record.
 *
 * PARAMETERS:  recordP - pointer to a ToDo record
 *
 * RETURNED:    pointer to a null-terminated note
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	art		4/15/95		Initial Revision
 *	CS2		01/23/03	Revised for new structs
 *
 ***********************************************************************/
Char* NewGetToDoNotePtr (ToDoDBRecordPtr recordP)
{
	return ToDoDBRecordGetFieldPointer( recordP, toDoRecordFieldNote );
}


/******************************************************************************
 *
 * FUNCTION:	ToDoDBRecordGetFieldPointer
 *
 * DESCRIPTION:	Get a pointer to the data for a specified field
 *
 * PARAMETERS:	recordP -		record pointer
 *				field -			which field
 *
 * RETURNED:	pointer to data if present, NULL otherwise
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	CS2		02/06/03	Initial Revision
 *
 *****************************************************************************/
static Char *ToDoDBRecordGetFieldPointer( ToDoDBRecordPtr recordP,
		ToDoRecordFieldType field )
{
	Char	*ret;

	if ( toDoRecordFieldDataFlags == field )
	{
		return ( Char * ) &recordP->dataFlags;
	}
	
	if ( toDoRecordFieldRecordFlags == field )
	{
		return ( Char * ) &recordP->recordFlags;
	}

	if ( toDoRecordFieldPriority == field )
	{
		return ( Char * ) &recordP->priority;
	}
	
	ret = ( Char * ) &recordP->optionalData;
	
	if ( toDoRecordFieldDueDate == field )
	{
		if ( recordP->dataFlags.dueDate )
		{
			return ret;
		}
		else
		{
			return NULL;
		}
	}
	
	if ( recordP->dataFlags.dueDate )
	{
		ret += sizeof( DateType );
	}
	
	if ( toDoRecordFieldCompletionDate == field )
	{
		if ( recordP->dataFlags.completionDate )
		{
			return ret;
		}
		else
		{
			return NULL;
		}
	}

	if ( recordP->dataFlags.completionDate )
	{
		ret += sizeof( DateType );
	}

	if ( toDoRecordFieldAlarm == field )
	{
		if ( recordP->dataFlags.alarm )
		{
			return ret;
		}
		else
		{
			return NULL;
		}
	}

	if ( recordP->dataFlags.alarm )
	{
		ret += sizeof( ToDoAlarmInfoType );
	}

	if ( toDoRecordFieldRepeat == field )
	{
		if ( recordP->dataFlags.repeat )
		{
			return ret;
		}
		else
		{
			return NULL;
		}
	}

	if ( recordP->dataFlags.repeat )
	{
		ret += sizeof( ToDoRepeatInfoType );
	}

	if ( toDoRecordFieldDescription == field )
	{
		return ret;
	}

	ret += (StrLen( ret ) + 1);
	
	if ( toDoRecordFieldNote == field )
	{
		return ret;
	}
	
	ret += (StrLen( ret ) + 1);
	
	if ( toDoRecordFieldEndOfRecord == field )
	{
		return ret;
	}

	return NULL;
}

/******************************************************************************
 *
 * FUNCTION:    NewToDoNewRecord
 *
 * DESCRIPTION: Create a new record in sorted position
 *
 * PARAMETERS:  dbP - database pointer
 *				item - item pointer
 *				category - new record's category
 *				index - on return, new record's index
 *                		
 * RETURNED:    errNone if successful, error code if not
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	Roger	01/10/95	Initial Revision
 *	CS2		01/15/03	Revised for Mullet database structure
 *
 *****************************************************************************/
static Err NewToDoNewRecord( DmOpenRef dbP, ToDoItemPtr item, UInt16 category,
		UInt16 filter, UInt16 subFilter, UInt16 *index )
{
	Err 				err;
	UInt16				size, dataSize;
	UInt16				zero = 0;
	UInt16				attr;
	UInt16 				newIndex;
	UInt32				offset;
	ToDoDBRecordPtr		recordP;
	MemHandle 			recordH;
	SortRecordInfoType	sortInfo;

	// Compute the size of the new to do record.
	size = sizeDBRecord;

	if ( item->dataFlags.dueDate )
	{
		size += sizeof( DateType );
	}
	
	if ( item->dataFlags.completionDate )
	{
		size += sizeof( DateType );
	}

	if ( item->dataFlags.alarm )
	{
		size += sizeof( ToDoAlarmInfoType );
	}
	
	if ( item->dataFlags.repeat )
	{
		size += sizeof( ToDoRepeatInfoType );
	}

	if ( item->dataFlags.description )
	{
		size += StrLen( item->descriptionP );
	}

	if ( item->dataFlags.note )
	{
		size += StrLen( item->noteP );
	}

	//  Allocate a chunk in the database for the new record.
	recordH = ( MemHandle ) DmNewHandle( dbP, size );
	if ( recordH == NULL )
	{
		return dmErrMemError;
	}

	// Pack the the data into the new record.
	recordP = MemHandleLock( recordH );
	
	DmWrite( recordP, OffsetOf( ToDoDBRecord, dataFlags ),
		&item->dataFlags, sizeof( ToDoDBDataFlags ) );

	DmWrite( recordP, OffsetOf( ToDoDBRecord, recordFlags ),
		&item->recordFlags, sizeof( UInt16 ) );

	DmWrite( recordP, OffsetOf( ToDoDBRecord, priority ),
		&item->priority, sizeof( UInt16 ) );
	
	offset = OffsetOf( ToDoDBRecord, optionalData );

	if ( item->dataFlags.dueDate )
	{
		dataSize = sizeof( DateType );
		DmWrite( recordP, offset, &item->dueDate, dataSize );
		offset += dataSize;
	}
	
	if ( item->dataFlags.completionDate )
	{
		dataSize = sizeof( DateType );
		DmWrite( recordP, offset, &item->completionDate, dataSize );
		offset += dataSize;
	}

	if ( item->dataFlags.alarm )
	{
		dataSize = sizeof( ToDoAlarmInfoType );
		DmWrite( recordP, offset, &item->alarmInfo, dataSize );
		offset += dataSize;
	}

	if ( item->dataFlags.repeat )
	{
		dataSize = sizeof( ToDoRepeatInfoType );
		DmWrite( recordP, offset, &item->repeatInfo, dataSize );
		offset += dataSize;
	}

	if ( item->dataFlags.description )
	{
		DmStrCopy( recordP, offset, item->descriptionP );
		offset += StrLen( item->descriptionP ) + 1;
	}
	else
	{
		DmWrite( recordP, offset, &zero, 1 );
		offset += 1;
	}

	if ( item->dataFlags.note )
	{
		DmStrCopy( recordP, offset, item->noteP );
	}
	else
	{
		DmWrite( recordP, offset, &zero, 1 );
	}

	sortInfo.attributes = category;
	sortInfo.uniqueID[0] = 0;
	sortInfo.uniqueID[1] = 0;
	sortInfo.uniqueID[2] = 0;
	
	// Determine the sort position of the new record.
	newIndex = ToDoFindSortPosition (dbP, recordP, &sortInfo, filter, subFilter);

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

Int16 NewActualPerformNotifyTD(HappyDays birth, DateType when, Int8 age,
                               Int16 *created, Int16 *touched, Int16 existIndex)
{
    Char noteField[255+1];  // (datebk3: 10, AN:14), HD id: 5
    ToDoItemType todo;
    Char* description = 0;
    Int16 idx;  

    /* Zero the memory */
    MemSet(&todo, sizeof(ToDoItemType), 0);

    todo.dataFlags.dueDate = 1;
    todo.dataFlags.description = 1;
    todo.dataFlags.note = 1;
    
    // set the date 
    //
    todo.dueDate = when;
    todo.priority = gPrefsR.priority;

    // General note field edit
	//
    if (gPrefsR.tusenote) {
        StrCopy(noteField, gPrefsR.tnote);
        StrCat(noteField, "\n");
    }
    else if ((idx = FindEventNoteInfo(birth)) >= 0) {
        LoadEventNoteString(noteField, idx, 256);
    }
    else noteField[0] = 0;

    StrCat(noteField, gPrefsR.notifywith);
    StrPrintF(gAppErrStr, "%ld", Hash(birth.name1, birth.name2));
    StrCat(noteField, gAppErrStr);
    todo.noteP = noteField;

    // make the description
        
    description = NotifyDescString(when, birth, age, true);		// todo = true
    todo.descriptionP = description;

    if (existIndex < 0) {            // there is no same record
        //
        // if not exists
        // write the new record (be sure to fill index)
        //
        NewToDoNewRecord(ToDoDB, &todo, gPrefsR.todoCategory, 0, 0, (UInt16*)&existIndex);
        // if private is set, make the record private
        //
        ChkNMakePrivateRecord(ToDoDB, existIndex);
        (*created)++;
    }
    else {                                      // if exists
        if (gPrefsR.existing == 0) {
            // keep the record
        }
        else {
            // modify

            // remove the record
            DmDeleteRecord(ToDoDB, existIndex);
            // deleted records are stored at the end of the database
            //
            DmMoveRecord(ToDoDB, existIndex, DmNumRecords(ToDoDB));
            
            // make new record
            NewToDoNewRecord(ToDoDB, &todo, gPrefsR.todoCategory, 0, 0, (UInt16*)&existIndex);
            // if private is set, make the record private
            //
            ChkNMakePrivateRecord(ToDoDB, existIndex);
            (*touched)++;
        }
    }

    if (description) MemPtrFree(description);
    return 0;
}

Int16 NewCheckToDoRecord(DateType when, HappyDays birth)
{
    UInt16 currIndex = 0;
    MemHandle recordH;
    
    ToDoDBRecordPtr toDoRec;
    DateType dueDate;
    Char *note;
    
    while (1) {
        recordH = DmQueryNextInCategory(ToDoDB, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;

        toDoRec = MemHandleLock(recordH);
        note = NewGetToDoNotePtr(toDoRec);

        if (toDoRec->dataFlags.dueDate) {
            dueDate = *(DateType*)ToDoDBRecordGetFieldPointer(toDoRec, toDoRecordFieldDueDate);
        }
        else {
            DateToInt(dueDate) = 0;
        }
        
        if ((DateToInt(when) == DateToInt(dueDate))
            && IsSameRecord(note, birth)) {
            MemHandleUnlock(recordH);

            return currIndex;
        }
        else {
            MemHandleUnlock(recordH);
            currIndex++;
        }
    }
    return -1;
}

int NewCleanupFromTD(DmOpenRef db)
{
    UInt16 currIndex = 0;
    MemHandle recordH;
    ToDoDBRecordPtr toDoRec;
    Char *note;
    int ret = 0;
    
    while (1) {
        recordH = DmQueryNextInCategory(db, &currIndex, dmAllCategories);
        if (!recordH) break;

        toDoRec = MemHandleLock(recordH);
        note = NewGetToDoNotePtr(toDoRec);
        
        if (IsHappyDaysRecord(note)) {
            // if it is happydays record?
            //
            ret++;
            // remove the record
            DmDeleteRecord(db, currIndex);
            // deleted records are stored at the end of the database
            //
            DmMoveRecord(db, currIndex, DmNumRecords(ToDoDB));
        }
        else {
            MemHandleUnlock(recordH);
            currIndex++;
        }
    }
    return ret;
}
