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
 * Just so you know these routines are pulled from the Datebook source code
 * as provided by Palm.  At least that is what I started from, I may have
 * touched a few lines to get it to compile with gcc without warnings.
 */

#include <PalmOS.h>
#include "address.h"
#include "datebook.h"

#include "happydays.h"
#include "happydaysRsc.h"

/***********************************************************************
 *  
 *  Internal Structutes
 *  
 ***********************************************************************/
    
// The following structure doesn't really exist.  The first field
// varies depending on the data present.  However, it is convient
// (and less error prone) to use when accessing the other information.
typedef struct {
    ApptDateTimeType    when;
    ApptDBRecordFlags   flags;  // A flag set for each  datum present
    char                firstField;
} ApptPackedDBRecordType;

typedef ApptPackedDBRecordType * ApptPackedDBRecordPtr;
 
typedef Int16 comparF (const void *, const void *, Int16 other);
 
/***********************************************************************
 *
 *  Internal Routines
 *
 ***********************************************************************/

static Int16 TimeCompare (TimeType t1, TimeType t2);

/***********************************************************************
 *
 * FUNCTION:    DateCompare
 *
 * DESCRIPTION: This routine compares two dates.
 *
 * PARAMETERS:  d1 - a date 
 *              d2 - a date 
 *
 * RETURNED:    if d1 > d2  returns a positive int
 *              if d1 < d2  returns a negative int
 *              if d1 = d2  returns zero
 *
 * NOTE: This routine treats the DateType structure like an unsigned int,
 *       it depends on the fact the the members of the structure are ordered
 *       year, month, day form high bit to low low bit.
 *
 ***********************************************************************/
Int16 DateCompare(DateType d1, DateType d2)
{
    UInt16 int1, int2;
    
    int1 = DateToInt(d1);
    int2 = DateToInt(d2);
    
    if (int1 > int2) return (1);
    else if (int1 < int2) return (-1);
    return 0;
}

/***********************************************************************
 *
 * FUNCTION:    TimeCompare
 *
 * DESCRIPTION: This routine compares two times.  "No time" is represented
 *              by minus one, and is considered less than all times.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    if t1 > t2  returns a positive int
 *              if t1 < t2  returns a negative int
 *              if t1 = t2  returns zero
 *
 * REVISION HISTORY:
 *          Name    Date        Description
 *          ----    ----        -----------
 *          art 6/12/95     Initial Revision
 *
 ***********************************************************************/
static Int16 TimeCompare (TimeType t1, TimeType t2) {
    Int16 int1, int2;

    int1 = TimeToInt(t1);
    int2 = TimeToInt(t2);

    if (int1 > int2)
        return (1);
    else if (int1 < int2)
        return (-1);
    return 0;

}

/************************************************************
 *
 *  FUNCTION:    ApptComparePackedRecords
 *
 *  DESCRIPTION: Compare two packed records.
 *
 *  PARAMETERS:  r1    - database record 1
 *               r2    - database record 2
 *               extra - extra data, not used in the function
 *
 *  RETURNS:    -1 if record one is less
 *                 1 if record two is less
 *
 *  CREATED: 1/14/95 
 *
 *  BY: Roger Flores
 *
 *  COMMENTS:   Compare the two records key by key until
 *  there is a difference.  Return -1 if r1 is less or 1 if r2
 *  is less.  A zero is never returned because if two records
 *  seem identical then their unique IDs are compared!
 *
 *************************************************************/
static Int16
ApptComparePackedRecords (ApptPackedDBRecordPtr r1,
                          ApptPackedDBRecordPtr r2, Int16 extra,
                          SortRecordInfoPtr info1,
                          SortRecordInfoPtr info2, MemHandle appInfoH)
{
    Int16 result;

    if ((r1->flags.repeat) || (r2->flags.repeat)) {
        if ((r1->flags.repeat) && (r2->flags.repeat))
            result = 0;
        else if (r1->flags.repeat)
            result = -1;
        else
            result = 1;
    }
    else {
        result = DateCompare (r1->when.date, r2->when.date);
        if (result == 0) {
            result = TimeCompare (r1->when.startTime, r2->when.startTime);
        }
    }
    return result;
}

/************************************************************
 *
 *  FUNCTION: ApptPackedSize
 *
 *  DESCRIPTION: Return the packed size of an ApptDBRecordType 
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: the size in bytes
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static UInt16 ApptPackedSize (ApptDBRecordPtr r) {
    UInt16 size;


    size = sizeof (ApptDateTimeType) + sizeof (ApptDBRecordFlags);

    if (r->alarm != NULL)
        size += sizeof (AlarmInfoType);

    if (r->repeat != NULL)
        size += sizeof (RepeatInfoType);

    if (r->exceptions != NULL)
        size += sizeof (UInt16) +
            (r->exceptions->numExceptions * sizeof (DateType));

    if (r->description != NULL)
        size += StrLen(r->description) + 1;

    if (r->note != NULL)
        size += StrLen(r->note) + 1;


    return size;
}


/************************************************************
 *
 *  FUNCTION: ApptPack
 *
 *  DESCRIPTION: Pack an ApptDBRecordType
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: the ApptPackedDBRecord is packed
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static void ApptPack(ApptDBRecordPtr s, ApptPackedDBRecordPtr d) {
    ApptDBRecordFlags   flags;
    UInt16    size;
    UInt32    offset = 0;


    *(unsigned char *)&flags = 0;           // clear the flags


    // copy the ApptDateTimeType
    //c = (char *) d;
    offset = 0;
    DmWrite(d, offset, s->when, sizeof(ApptDateTimeType));
    offset += sizeof (ApptDateTimeType) + sizeof (ApptDBRecordFlags);


    if (s->alarm != NULL) {
        DmWrite(d, offset, s->alarm, sizeof(AlarmInfoType));
        offset += sizeof (AlarmInfoType);
        flags.alarm = 1;
    }


    if (s->repeat != NULL) {
        DmWrite(d, offset, s->repeat, sizeof(RepeatInfoType));
        offset += sizeof (RepeatInfoType);
        flags.repeat = 1;
    }


    if (s->exceptions != NULL) {
        size = sizeof (UInt16) +
            (s->exceptions->numExceptions * sizeof (DateType));
        DmWrite(d, offset, s->exceptions, size);
        offset += size;
        flags.exceptions = 1;
    }


    if (s->description != NULL) {
        size = StrLen(s->description) + 1;
        DmWrite(d, offset, s->description, size);
        offset += size;
        flags.description = 1;
    }



    if (s->note != NULL) {
        size = StrLen(s->note) + 1;
        DmWrite(d, offset, s->note, size);
        offset += size;
        flags.note = 1;
    }

    DmWrite(d, sizeof(ApptDateTimeType), &flags, sizeof(flags));
}

/************************************************************
 *
 *  FUNCTION: ApptUnpack
 *
 *  DESCRIPTION: Fills in the ApptDBRecord structure
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: the record unpacked
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static void ApptUnpack(ApptPackedDBRecordPtr src, ApptDBRecordPtr dest) {
    ApptDBRecordFlags   flags;
    char *p;


    flags = src->flags;
    p = &src->firstField;
    dest->when = (ApptDateTimeType *) src;

    if (flags.alarm) {
        dest->alarm = (AlarmInfoType *) p;
        p += sizeof (AlarmInfoType);
    } else
        dest->alarm = NULL;


    if (flags.repeat) {
        dest->repeat = (RepeatInfoType *) p;
        p += sizeof (RepeatInfoType);
    } else
        dest->repeat = NULL;


    if (flags.exceptions) {
        dest->exceptions = (ExceptionsListType *) p;
        p += sizeof (UInt16) +
            (((ExceptionsListType *) p)->numExceptions * sizeof (DateType));
    } else
        dest->exceptions = NULL;


    if (flags.description) {
        dest->description = p;
        p += StrLen(p) + 1;
    } else
        dest->description = NULL;


    if (flags.note) {
        dest->note = p;
    } else
        dest->note = NULL;

}

/************************************************************
 *
 *  FUNCTION: ApptFindSortPosition
 *
 *  DESCRIPTION: Return where a record is or should be
 *      Useful to find or find where to insert a record.
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: position where a record should be
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static UInt16 ApptFindSortPosition(DmOpenRef dbP, ApptPackedDBRecordPtr
                                 newRecord)
{
    return (DmFindSortPosition (dbP, newRecord, NULL,
                                (DmComparF *)ApptComparePackedRecords, 0));
}

/***********************************************************************
 *
 * FUNCTION:    ApptFindFirst
 *
 * DESCRIPTION: This routine finds the first appointment on the specified
 *              day.
 *
 * PARAMETERS:  dbP    - pointer to the database
 *              date   - date to search for
 *              indexP - pointer to the index of the first record on the 
 *                       specified day (returned value)
 *
 * RETURNED:    true if a record has found
 *
 * REVISION HISTORY:
 *          Name    Date        Description
 *          ----    ----        -----------
 *          art 6/15/95     Initial Revision
 *
 ***********************************************************************/
Boolean ApptFindFirst (DmOpenRef dbP, DateType date, UInt16 * indexP) {
    Err err;
    Int16 numOfRecords;
    Int16 kmin, probe, i;     // all positions in the database.
    Int16 result = 0;         // result of comparing two records
    UInt16 index;
    MemHandle recordH;
    Boolean found = false;
    ApptPackedDBRecordPtr r;

    kmin = probe = 0;
    numOfRecords = DmNumRecords(dbP);


    while (numOfRecords > 0) {
        i = numOfRecords >> 1;
        probe = kmin + i;

        index = probe;
        recordH = DmQueryNextInCategory (dbP, &index, dmAllCategories);
        if (recordH) {
            r = (ApptPackedDBRecordPtr) MemHandleLock (recordH);
            if (r->flags.repeat)
                result = 1;
            else
                result = DateCompare (date, r->when.date);
            MemHandleUnlock (recordH);
        }

        // If no handle, assume the record is deleted, deleted records
        // are greater.
        else
            result = -1;


        // If the date passed is less than the probe's date, keep searching.
        if (result < 0)
            numOfRecords = i;

        // If the date passed is greater than the probe's date, keep searching.
        else if (result > 0) {
            kmin = probe + 1;
            numOfRecords = numOfRecords - i - 1;
        }

        // If the records are equal find the first record on the day.
        else {
            found = true;
            *indexP = index;
            while (true) {
                err = DmSeekRecordInCategory (dbP, &index, 1, dmSeekBackward,
                                              dmAllCategories);
                if (err == dmErrSeekFailed) break;

                recordH = DmQueryRecord(dbP, index);
                r = (ApptPackedDBRecordPtr) MemHandleLock (recordH);
                if (r->flags.repeat)
                    result = 1;
                else
                    result = DateCompare (date, r->when.date);
                MemHandleUnlock (recordH);
                if (result != 0) break;
                *indexP = index;
            }

            break;
        }
    }


    // If that were no appointments on the specified day, return the
    // index of the next appointment (on a future day).
    if (! found) {
        if (result < 0)
            *indexP = probe;
        else
            *indexP = probe + 1;
    }

    return (found);
}

/************************************************************
 *
 *  FUNCTION: ApptGetRecord
 *
 *  DESCRIPTION: Get a record from a Appointment Database
 *
 *  PARAMETERS: database pointer
 *                  database index
 *                  database record
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Err ApptGetRecord (DmOpenRef dbP, UInt16 index, ApptDBRecordPtr r,
                   MemHandle * handleP) {
    MemHandle handle;
    ApptPackedDBRecordPtr src;

    handle = DmQueryRecord(dbP, index);
    ErrFatalDisplayIf(DmGetLastErr(), "Error Querying record");

    src = (ApptPackedDBRecordPtr) MemHandleLock (handle);

    if (DmGetLastErr()) {
        *handleP = 0;
        return DmGetLastErr();
    }

    ApptUnpack(src, r);

    *handleP = handle;
    return 0;
}

/************************************************************
 *
 *  FUNCTION: ApptChangeRecord
 *
 *  DESCRIPTION: Change a record in the Appointment Database
 *
 *  PARAMETERS: database pointer
 *                   database index
 *                   database record
 *                   changed fields
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *  COMMENTS:   Records are not stored with extra padding - they
 *  are always resized to their exact storage space.  This avoids
 *  a database compression issue.  The code works as follows:
 *  
 *  1)  get the size of the new record
 *  2)  make the new record
 *  3)  pack the packed record plus the changes into the new record
 *  4)  if the sort position is changes move to the new position
 *  5)  attach in position
 *
 *************************************************************/
Err ApptChangeRecord(DmOpenRef dbP, UInt16 *index, ApptDBRecordPtr r,
                     ApptDBRecordFlags changedFields) {
    Err result;
    Int16 newIndex;
    UInt16 attributes;
    Boolean dontMove;
    MemHandle oldH;
    MemHandle srcH;
    MemHandle dstH;
    ApptDBRecordType src;
    ApptPackedDBRecordPtr dst = 0;
    ApptPackedDBRecordPtr cmp;

    // We do not assume that r is completely valid so we get a valid
    // ApptDBRecordPtr...
    if ((result = ApptGetRecord(dbP, *index, &src, &srcH)) != 0)
        return result;

    // and we apply the changes to it.
    if (changedFields.when)
        src.when = r->when;

    if (changedFields.alarm)
        src.alarm = r->alarm;

    if (changedFields.repeat)
        src.repeat = r->repeat;

    if (changedFields.exceptions)
        src.exceptions = r->exceptions;

    if (changedFields.description)
        src.description = r->description;

    if (changedFields.note)
        src.note = r->note;

    // Allocate a new chunk with the correct size and pack the data from
    // the unpacked record into it.
    dstH = DmNewHandle(dbP, (UInt32) ApptPackedSize(&src));
    if (dstH) {
        dst = MemHandleLock (dstH);
        ApptPack (&src, dst);
    }

    MemHandleUnlock (srcH);
    if (dstH == NULL)
        return dmErrMemError;

    // If the sort position is changed move to the new position.
    // Check if any of the key fields have changed.
    if ((!changedFields.when) && (! changedFields.repeat))
        goto attachRecord;      // repeating events aren't in sorted order

    // Make sure *index-1 < *index < *index+1, if so it's in sorted
    // order.  Leave it there.
    if (*index > 0) {
        // This record wasn't deleted and deleted records are at the end of the
        // database so the prior record may not be deleted!
        cmp = MemHandleLock (DmQueryRecord(dbP, *index-1));
        dontMove = (ApptComparePackedRecords (cmp, dst, 0, NULL, NULL, 0)
                    <= 0);
        MemPtrUnlock (cmp);
    } else
        dontMove = true;


    if (dontMove && (*index+1 < DmNumRecords (dbP))) {
        DmRecordInfo(dbP, *index+1, &attributes, NULL, NULL);
        if ( ! (attributes & dmRecAttrDelete) ) {
            cmp = MemHandleLock (DmQueryRecord(dbP, *index+1));
            dontMove &= (ApptComparePackedRecords (dst, cmp, 0, NULL, NULL,
                                                   0) <= 0);
            MemPtrUnlock (cmp);
        }
    }

    if (dontMove)
        goto attachRecord;

    // The record isn't in the right position.  Move it.
    newIndex = ApptFindSortPosition (dbP, dst);
    DmMoveRecord (dbP, *index, newIndex);
    if (newIndex > *index) newIndex--;
    *index = newIndex;                      // return new position

 attachRecord:
    // Attach the new record to the old index,  the preserves the
    // category and record id.
    result = DmAttachRecord (dbP, index, dstH, &oldH);

    MemPtrUnlock(dst);

    if (result) return result;

    MemHandleFree(oldH);


    return 0;
}
/***********************************************************************
 *
 * FUNCTION:    ApptRepeatsOnDate
 *
 * DESCRIPTION: This routine returns true if a repeating appointment
 *              occurrs on the specified date.
 *
 * PARAMETERS:  apptRec - a pointer to an appointment record
 *              date    - date to check              
 *
 * RETURNED:    true if the appointment occurs on the date specified
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/14/95		Initial Revision
 *
 ***********************************************************************/
Boolean ApptRepeatsOnDate (ApptDBRecordPtr apptRec, DateType date)
{
	Int16  i;
	UInt16 freq;
	UInt16 weeksDiff;
	UInt16 dayInMonth;
	UInt16 dayOfWeek;
	UInt16 dayOfMonth;
	UInt16 firstDayOfWeek;
	long dateInDays;
	long startInDays;
	Boolean onDate = false;
	DatePtr exceptions;
	DateType startDate;

	// Is the date passed before the start date of the appointment?
	if (DateCompare (date, apptRec->when->date) < 0)
		return (false);

	// Is the date passed after the end date of the appointment?
	if (DateCompare (date, apptRec->repeat->repeatEndDate) > 0)
		return (false);
	

	// Get the frequency of occurrecne
    //    (ex: every 2nd day, every 3rd month, etc.).  
	freq = apptRec->repeat->repeatFrequency;
	
	// Get the date of the first occurrecne of the appointment.
	startDate = apptRec->when->date;

	switch (apptRec->repeat->repeatType)
    {
		// Daily repeating appointment.
    case repeatDaily:
        dateInDays = DateToDays (date);
        startInDays = DateToDays (startDate);
        onDate = ((dateInDays - startInDays) % freq) == 0;
        break;
			

		// Weekly repeating appointment (ex: every Monday and Friday). 
		// Yes, weekly repeating appointment can occur more then once a
		// week.
    case repeatWeekly:
        // Are we on a day of the week that the appointment repeats on.
        dayOfWeek = DayOfWeek (date.month, date.day, date.year+firstYear);
        onDate = ((1 << dayOfWeek) & apptRec->repeat->repeatOn);
        if (! onDate) break;

        // Are we in a week in which the appointment occurrs, if not 
        // move to that start of the next week in which the appointment
        // does occur.
        dateInDays = DateToDays (date);
        startInDays = DateToDays (startDate);

        firstDayOfWeek = (DayOfWeek (1, 1, firstYear) - 
                          apptRec->repeat->repeatStartOfWeek + daysInWeek) % daysInWeek;

        weeksDiff = (((dateInDays + firstDayOfWeek) / daysInWeek) - 
                     ((startInDays + firstDayOfWeek) / daysInWeek)) %freq;
        onDate = (weeksDiff == 0);
        break;


//			// Compute the first occurrence of the appointment that occurs
//			// on the same day of the week as the date passed.
//			startDayOfWeek = DayOfWeek (startDate.month, startDate.day, 
//				startDate.year+firstYear);
//			startInDays = DateToDays (startDate);
//			if (startDayOfWeek < dayOfWeek)
//				startInDays += dayOfWeek - startDayOfWeek;
//			else if (startDayOfWeek > dayOfWeek)
//				startInDays += dayOfWeek+ (daysInWeek *freq) - startDayOfWeek;
//			
//			// Are we in a week in which the appointment repeats.
//			dateInDays = DateToDays (date);
//			onDate = (((dateInDays - startInDays) / daysInWeek) % freq) == 0;
//			break;


		// Monthly-by-day repeating appointment (ex: the 3rd Friday of every
		// month).
    case repeatMonthlyByDay:
        // Are we in a month in which the appointment repeats.
        onDate = ((((date.year - startDate.year) * monthsInYear) + 
                   (date.month - startDate.month)) % freq) == 0;
        if (! onDate) break;

        // Do the days of the month match (ex: 3rd Friday)
        dayOfMonth = DayOfMonth (date.month, date.day, date.year+firstYear);
        onDate = (dayOfMonth == apptRec->repeat->repeatOn);
        if (onDate) break;
			
        // If the appointment repeats on one of the last days of the month,
        // check if the date passed is also one of the last days of the 
        // month.  By last days of the month we mean: last sunday, 
        // last monday, etc.
        if ((apptRec->repeat->repeatOn >= domLastSun) &&
            (dayOfMonth >= dom4thSun))
        {
            dayOfWeek = DayOfWeek (date.month, date.day, date.year+firstYear);
            dayInMonth = DaysInMonth (date.month, date.year+firstYear);
            onDate = (((date.day + daysInWeek) > dayInMonth) &&
                      (dayOfWeek == (apptRec->repeat->repeatOn % daysInWeek)));
        }
        break;						


		// Monthly-by-date repeating appointment (ex: the 15th of every
		// month).
    case repeatMonthlyByDate:
        // Are we in a month in which the appointment repeats.
        onDate = ((((date.year - startDate.year) * monthsInYear) + 
                   (date.month - startDate.month)) % freq) == 0;
        if (! onDate) break;
			
        // Are we on the same day of the month as the start date.
        onDate = (date.day == startDate.day);
        if (onDate) break;

        // If the staring day of the appointment is greater then the 
        // number of day in the month passed, and the day passed is the 
        // last day of the month, then the appointment repeats on the day.
        dayInMonth = DaysInMonth (date.month, date.year+firstYear);
        onDate = ((startDate.day > dayInMonth) && (date.day == dayInMonth));
        break;


		// Yearly repeating appointment.
    case repeatYearly:
        // Are we in a year in which the appointment repeats.
        onDate = ((date.year - startDate.year) % freq) == 0;
        if (! onDate) break;
			
        // Are we on the month and day that the appointment repeats.
        onDate = (date.month == startDate.month) &&
            (date.day == startDate.day);
        if (onDate) break;
			
        // Specal leap day processing.
        if ( (startDate.month == february) && 
             (startDate.day == 29) &&
             (date.month == february) && 
             (date.day == DaysInMonth (date.month, date.year+firstYear)))
        {
            onDate = true;
        }				      
        break;
    default:
        break;
    }

	// Check for an exception.
	if ((onDate) && (apptRec->exceptions))
    {
		exceptions = &apptRec->exceptions->exception;
		for (i = 0; i < apptRec->exceptions->numExceptions; i++)
        {
			if (DateCompare (date, exceptions[i]) == 0)
            {
				onDate = false;
				break;
            }
        }
    }

	return (onDate);
}

/***********************************************************************
 *
 * FUNCTION:    ApptGetAppointments
 *
 * DESCRIPTION: This routine returns a list of appointments that are on 
 *              the date specified
 *
 * PARAMETERS:  dbP    - pointer to the database
 *              date   - date to search for
 *              countP - number of appointments on the specified 
 *                       day (returned value)
 *
 * RETURNED:    handle of the appointment list (ApptInfoType)
 *
 * REVISION HISTORY:
 *          Name    Date        Description
 *          ----    ----        -----------
 *          art     6/15/95     Initial Revision
 *
 ***********************************************************************/
MemHandle ApptGetAppointments (DmOpenRef dbP, DateType date, UInt16 * countP) {
    Err error;
    Int16 result;
    Int16 count = 0;
    UInt16    recordNum;
    Boolean repeats;
    MemHandle recordH;
    MemHandle apptListH;
    ApptInfoPtr apptList;
    ApptDBRecordType apptRec;
    ApptPackedDBRecordPtr r;

    // Allocated a block to hold the appointment list.
    apptListH = MemHandleNew (sizeof (ApptInfoType) * apptMaxPerDay);
    ErrFatalDisplayIf(!apptListH, "Out of memory");
    if (! apptListH) return (0);

    apptList = MemHandleLock (apptListH);

    // Find the first non-repeating appointment of the day.
    if (ApptFindFirst (dbP, date, &recordNum)) {
        while (count < apptMaxPerDay) {
            // Check if the appointment is on the date passed, if it is
            // add it to the appointment list.
            recordH = DmQueryRecord (dbP, recordNum);
            r = MemHandleLock (recordH);
            result = DateCompare (r->when.date, date);

            if (result == 0) {
                // Add the record to the appoitment list.
                apptList[count].startTime = r->when.startTime;
                apptList[count].endTime = r->when.endTime;
                apptList[count].recordNum = recordNum;
                count++;
            }
            MemHandleUnlock (recordH);
            if (result != 0) break;

            // Get the next record.
            error = DmSeekRecordInCategory (dbP, &recordNum, 1,
                                            dmSeekForward,
                                            dmAllCategories);
            if (error == dmErrSeekFailed) break;
        }
    }

    // Add the repeating appointments to the list.  Repeating appointments
    // are stored at the beginning of the database.
    recordNum = 0;
    while (count < apptMaxPerDay) {
        recordH = DmQueryNextInCategory (dbP, &recordNum, dmAllCategories);
        if (! recordH) break;
        
        r = (ApptPackedDBRecordPtr) MemHandleLock (recordH);
        
        repeats = (r->flags.repeat != 0);
        if (repeats) {
            ApptUnpack (r, &apptRec);
            if (ApptRepeatsOnDate (&apptRec, date)) {
                // Add the record to the appoitment list.
                apptList[count].startTime = r->when.startTime;              
                apptList[count].endTime = r->when.endTime;              
                apptList[count].recordNum = recordNum;  
                count++;
            }
        }
        MemHandleUnlock (recordH);

        // If the record has no repeating info we've reached the end of the 
        // repeating appointments.
        if (! repeats) break;
        
        recordNum++;
    }

    // Sort the list by start time.
    // SysInsertionSort (apptList, count, sizeof (ApptInfoType),
    //          ApptListCompare, 0L);

    // If there are no appointments on the specified day, free the appointment
    // list.
    if (count == 0) {
        MemPtrFree (apptList);
        apptListH = 0;
    }
    // Resize the appointment list block to release any unused space.
    else {
        MemHandleUnlock (apptListH);
        MemHandleResize (apptListH, count * sizeof (ApptInfoType));
    }

    *countP = count;
    return (apptListH);
}

/************************************************************
 *
 *  FUNCTION: ApptNewRecord
 *
 *  DESCRIPTION: Create a new packed record in sorted position
 *
 *  PARAMETERS: database pointer
 *              database record
 *
 *  RETURNS: ##0 if successful, error code if not
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Err ApptNewRecord(DmOpenRef dbP, ApptDBRecordPtr r, UInt16 *index) {
    MemHandle recordH;
    ApptPackedDBRecordPtr recordP;
    UInt16                    newIndex;
    Err err;


    // Make a new chunk with the correct size.
    recordH = DmNewHandle (dbP, (UInt32) ApptPackedSize(r));
    if (recordH == NULL)
        return dmErrMemError;

    recordP = MemHandleLock (recordH);

    // Copy the data from the unpacked record to the packed one.
    ApptPack (r, recordP);

    newIndex = ApptFindSortPosition(dbP, recordP);

    MemPtrUnlock (recordP);

    // 4) attach in place
    err = DmAttachRecord(dbP, &newIndex, recordH, 0);
    if (err)
        MemHandleFree(recordH);
    else
        *index = newIndex;

    return err;
}

