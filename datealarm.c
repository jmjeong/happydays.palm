#include <PalmOS.h>
#include <AlarmMgr.h>
#include <FeatureMgr.h>
#include <PalmUtils.h>

#include "datebook.h"
#include "happydaysRsc.h"
#include "util.h"
#include "section.h"

#define apptNoAlarm                    -1
#define apptMaxDisplayableAlarms        10

#define datebookUnsavedPrefsVersionNum  1
#define datebookUnsavedPrefID           0x00

#define apptEndOfTime                   0xffffffff

Boolean NextRepeat (ApptDBRecordPtr apptRec, DatePtr dateP) 
{
	Int16  i;
	UInt16 day;
	UInt16 freq;
	UInt16 year;
	UInt16 adjust;
	UInt16 weeksDiff;
	UInt16 monthsDiff;
	UInt16 daysInMonth;
	UInt16 dayOfWeek;
	UInt16 apptWeekDay;
	UInt16 firstDayOfWeek;
	UInt16 daysTilNext;
	UInt16 monthsTilNext;
	UInt32 dateInDays;
	UInt32 startInDays;
	DateType start;
	DateType date;
	DateType next;

	date = *dateP;

	// Is the date passed after the end date of the appointment?
	if (DateCompare (date, apptRec->repeat->repeatEndDate) > 0)
		return (false);
	
	// Is the date passed before the start date of the appointment?
	if (DateCompare (date, apptRec->when->date) < 0)
		date = apptRec->when->date;

	// Get the frequency on occurrecne (ex: every 2nd day, every 3rd month, etc).  
	freq = apptRec->repeat->repeatFrequency;
	
	// Get the date of the first occurrecne of the appointment.
	start = apptRec->when->date;
	

	switch (apptRec->repeat->repeatType)
    {
		// Daily repeating appointment.
    case repeatDaily:
        dateInDays = DateToDays (date);
        startInDays = DateToDays (start);
        daysTilNext = (dateInDays - startInDays + freq - 1) / freq * freq;
        if (startInDays + daysTilNext > (UInt32) maxDays)
            return (false);
        DateDaysToDate (startInDays + daysTilNext, &next);
        break;
			


		// Weekly repeating appointment (ex: every Monday and Friday). 
		// Yes, weekly repeating appointment can occur more then once a
		// week.
    case repeatWeekly:
        dateInDays = DateToDays (date);
        startInDays = DateToDays (start);

        firstDayOfWeek = (DayOfWeek (1, 1, firstYear) - 
                          apptRec->repeat->repeatStartOfWeek + daysInWeek) % daysInWeek;

        dayOfWeek = DayOfWeek (date.month, date.day, date.year+firstYear);
        apptWeekDay = (dayOfWeek - apptRec->repeat->repeatStartOfWeek +
                       daysInWeek) % daysInWeek;

        // Are we in a week in which the appointment occurrs, if not 
        // move to that start of the next week in which the appointment
        // does occur.
        weeksDiff = (((dateInDays + firstDayOfWeek) / daysInWeek) - 
                     ((startInDays + firstDayOfWeek) / daysInWeek)) %freq;
        if (weeksDiff)
        {
            adjust = ((freq - weeksDiff) * daysInWeek)- apptWeekDay;
            apptWeekDay = 0;
            dayOfWeek = (dayOfWeek + adjust) % daysInWeek;
        }
        else
            adjust = 0;
			
        // Find the next day on which the appointment repeats.
        for (i = 0; i < daysInWeek; i++)
        {
            if (apptRec->repeat->repeatOn & (1 << dayOfWeek)) break;
            adjust++;
            if (++dayOfWeek == daysInWeek)
                dayOfWeek = 0;
            if (++apptWeekDay == daysInWeek)
                adjust += (freq - 1) * daysInWeek;
        }

        if (dateInDays + adjust > (UInt32) maxDays)
            return (false);
        DateDaysToDate (dateInDays + adjust, &next);
//			next = date;
//			DateAdjust (&next, adjust);
        break;



		// Monthly-by-day repeating appointment (ex: the 3rd Friday of every
		// month).
    case repeatMonthlyByDay:
        // Compute the number of month until the appointment repeats again.
        monthsTilNext = (date.month - start.month);

        monthsTilNext = ((((date.year - start.year) * monthsInYear) + 
                          (date.month - start.month)) + freq - 1) /
            freq * freq;

        while (true)
        {
            year = start.year + 
                (start.month - 1 + monthsTilNext) / monthsInYear;
            if (year >= numberOfYears)
                return (false);

            next.year = year;
            next.month = (start.month - 1 + monthsTilNext) % monthsInYear + 1;
	
            dayOfWeek = DayOfWeek (next.month, 1, next.year+firstYear);
            if ((apptRec->repeat->repeatOn % daysInWeek) >= dayOfWeek)
                day = apptRec->repeat->repeatOn - dayOfWeek + 1;
            else
                day = apptRec->repeat->repeatOn + daysInWeek - dayOfWeek + 1;
	
            // If repeat-on day is between the last sunday and the last
            // saturday, make sure we're not passed the end of the month.
            if ( (apptRec->repeat->repeatOn >= domLastSun) &&
                 (day > DaysInMonth (next.month, next.year+firstYear)))
            {
                day -= daysInWeek;
            }
            next.day = day;

            // Its posible that "next date" calculated above is 
            // before the date passed.  If so, move forward
            // by the length of the repeat freguency and preform
            // the calculation again.
            if ( DateToInt(date) > DateToInt (next))
                monthsTilNext += freq;
            else
                break;
        }
        break;						



		// Monthly-by-date repeating appointment (ex: the 15th of every
		// month).
    case repeatMonthlyByDate:
        // Compute the number of month until the appointment repeats again.
        monthsDiff = ((date.year - start.year) * monthsInYear) + 
            (date.month - start.month);
        monthsTilNext = (monthsDiff + freq - 1) / freq * freq;

        if ((date.day > start.day) && (!(monthsDiff % freq)))
            monthsTilNext += freq;
				
        year = start.year + 
            (start.month - 1 + monthsTilNext) / monthsInYear;
        if (year >= numberOfYears)
            return (false);

        next.year = year;
        next.month = (start.month - 1 + monthsTilNext) % monthsInYear + 1;
        next.day = start.day;

        // Make sure we're not passed the last day of the month.
        daysInMonth = DaysInMonth (next.month, next.year+firstYear);
        if (next.day > daysInMonth)
            next.day = daysInMonth;
        break;



		// Yearly repeating appointment.
    case repeatYearly:
        next.day = start.day;
        next.month = start.month;

        year = start.year + 
            ((date.year - start.year + freq - 1) / freq * freq);
			
        if ((date.month > start.month) ||
            ((date.month == start.month) && (date.day > start.day)))
            year += freq;

        // Specal leap day processing.
        if ( (next.month == february) && (next.day == 29) &&
             (next.day > DaysInMonth (next.month, year+firstYear)))
        {
            next.day = DaysInMonth (next.month, year+firstYear);
        }				      
        if (year >= numberOfYears)
            return (false);

        next.year = year;	
        break;
    default:
        break;
    }

	// Is the next occurrence after the end date of the appointment?
	if (DateCompare (next, apptRec->repeat->repeatEndDate) > 0)
		return (false);

	ErrFatalDisplayIf ((DateToInt (next) < DateToInt (*dateP)),
                       "Calculation error");

	*dateP = next;
	return (true);
}

static Boolean IsException (ApptDBRecordPtr apptRec, DateType date) 
{
	int i;
	DatePtr exceptions;

	if (apptRec->exceptions)
    {
		exceptions = &apptRec->exceptions->exception;
		for (i = 0; i < apptRec->exceptions->numExceptions; i++)
        {
			if (DateCompare (date, exceptions[i]) == 0)
                return (true);
        }
    }
	return (false);
}

Boolean ApptNextRepeat (ApptDBRecordPtr apptRec, DatePtr dateP) 
{
	DateType date;
	
	date = *dateP;
	
	while (true)
    {
		// Compute the next time the appointment repeats.
		if (! NextRepeat (apptRec, &date))
			return (false);

		// Check if the date computed is in the exceptions list.
		if (! IsException (apptRec, date))
        {
			*dateP = date;
			return (true);
        }
			
		DateAdjust (&date, 1);
    }		
}

UInt32 ApptGetAlarmTime (ApptDBRecordPtr apptRec, UInt32 currentTime) 
{
	UInt32					advance = 0;
	UInt32					alarmTime;
	DateType				repeatDate;
	DateTimeType		curDateTime;
	DateTimeType		apptDateTime;

	if (!apptRec->alarm)
    {
		return apptNoTime;
    }

	// Non-repeating appointment?
	if (! apptRec->repeat)
    {
		// An alarm on an untimed event triggers at midnight.
		if (TimeToInt (apptRec->when->startTime) == apptNoTime)
        {
			apptDateTime.minute = 0;
			apptDateTime.hour = 0;
        }
		else
        {
			apptDateTime.minute = apptRec->when->startTime.minutes;
			apptDateTime.hour = apptRec->when->startTime.hours;
        }
		apptDateTime.second = 0;
		apptDateTime.day = apptRec->when->date.day;
		apptDateTime.month = apptRec->when->date.month;
		apptDateTime.year = apptRec->when->date.year + firstYear;



		// Compute the time of the alarm by adjusting the date and time 
		// of the appointment by the length of the advance notice.
		advance = apptRec->alarm->advance;
		switch (apptRec->alarm->advanceUnit)
        {
        case aauMinutes:
            advance *= minutesInSeconds;
            break;
        case aauHours:
            advance *= hoursInSeconds;
            break;
        case aauDays:
            advance *= daysInSeconds;
            break;
        }

		alarmTime = TimDateTimeToSeconds (&apptDateTime) - advance;
		if (alarmTime >= currentTime)
			return (alarmTime);
		else
			return (0);
    }


	// Repeating appointment.
	TimSecondsToDateTime (currentTime, &curDateTime);
	repeatDate.year = curDateTime.year - firstYear;
	repeatDate.month = curDateTime.month;
	repeatDate.day = curDateTime.day;
	
	while (ApptNextRepeat (apptRec, &repeatDate))
    {
		// An alarm on an untimed event triggers at midnight.
		if (TimeToInt (apptRec->when->startTime) == apptNoTime)
        {
			apptDateTime.minute = 0;
			apptDateTime.hour = 0;
        }
		else
        {
			apptDateTime.minute = apptRec->when->startTime.minutes;
			apptDateTime.hour = apptRec->when->startTime.hours;
        }
		apptDateTime.second = 0;
		apptDateTime.day = repeatDate.day;
		apptDateTime.month = repeatDate.month;
		apptDateTime.year = repeatDate.year + firstYear;

		// Compute the time of the alarm by adjusting the date and time 
		// of the appointment by the length of the advance notice.
		switch (apptRec->alarm->advanceUnit)
        {
        case aauMinutes:
            advance = (UInt32) apptRec->alarm->advance * minutesInSeconds;
            break;
        case aauHours:
            advance = (UInt32) apptRec->alarm->advance * hoursInSeconds;
            break;
        case aauDays:
            advance = (UInt32) apptRec->alarm->advance * daysInSeconds;
            break;
        }

		alarmTime = TimDateTimeToSeconds (&apptDateTime) - advance;
		if (alarmTime >= currentTime)
			return (alarmTime);

		DateAdjust (&repeatDate, 1);
    } 
		
	return (0);
}


UInt32 AlarmGetTrigger (UInt32* refP) 
{
	UInt16 		cardNo;
	LocalID 		dbID;
	UInt32		alarmTime = 0;	
	DmSearchStateType searchInfo;

	DmGetNextDatabaseByTypeCreator (true, &searchInfo, 
                                    sysFileTApplication, sysFileCDatebook, true, &cardNo, &dbID);

	alarmTime = AlmGetAlarm (cardNo, dbID, refP);

	return (alarmTime);
}

void AlarmSetTrigger (UInt32 alarmTime, UInt32 ref) 
{
	UInt16 cardNo;
	LocalID dbID;
	DmSearchStateType searchInfo;

	DmGetNextDatabaseByTypeCreator (true, &searchInfo, 
                                    sysFileTApplication, sysFileCDatebook, true, &cardNo, &dbID);

	AlmSetAlarm (cardNo, dbID, ref, alarmTime, true);
}

static UInt32
GetSnoozeAnchorTime (
	PendingAlarmQueueType*	inAlarmInternalsP ) 
{
	return inAlarmInternalsP->snoozeAnchorTime;
}

/*
  static void
  SetPendingAlarmCount (
  PendingAlarmQueueType*	inAlarmInternalsP,
  UInt16						inCount)
  {
  inAlarmInternalsP->pendingCount = inCount;
  }
*/

UInt16
GetDismissedAlarmCount (
	PendingAlarmQueueType*	inAlarmInternalsP ) 
{
	return inAlarmInternalsP->dismissedCount;
}

UInt32 *
GetDismissedAlarmList (
	PendingAlarmQueueType*	inAlarmInternalsP ) 
{
	return inAlarmInternalsP->dismissedAlarm;
}

UInt32
ApptAlarmMunger (
	DmOpenRef					inDbR,
	UInt32							inAlarmStart,
	UInt32							inAlarmStop,
	PendingAlarmQueueType *	inAlarmInternalsP,
	UInt16*						ioCountP,
	DatebookAlarmType *		outAlarmListP,
	Boolean*					outAudibleP ) 
{
	UInt16							alarmListSize;
	UInt16							baseIndex = 0;
	UInt16							numRecords;
	UInt16							numAlarms = 0;
	UInt16							recordNum;
	MemHandle						recordH;
	ApptDBRecordType			    apptRec;
	ApptPackedDBRecordPtr	        r;
	UInt32							alarmTime;
	UInt32							earliestAlarm = 0;
	UInt32							uniqueID;
	Boolean						    skip;
	UInt16							i;
	UInt16							index;
	UInt16							dismissedCount = 0;
	UInt32 *						dismissedListP = 0;
	
	if ( outAudibleP )
    {
		*outAudibleP = false;
    }
	
	if ( ioCountP )
    {
		alarmListSize = *ioCountP;
    }
	else
    {
		alarmListSize = 0;
    }
	
	if (inAlarmInternalsP)
    {
		dismissedCount = GetDismissedAlarmCount (inAlarmInternalsP);
		dismissedListP = GetDismissedAlarmList (inAlarmInternalsP);
    }
	
	numRecords = DmNumRecords (inDbR);
	for (recordNum = 0; recordNum < numRecords; recordNum++)
    {
		recordH = DmQueryRecord (inDbR, recordNum);
		DmRecordInfo (inDbR, recordNum, NULL, &uniqueID, NULL);
		
		if ( !recordH )
        {
			break;
        }
		
		r = (ApptPackedDBRecordPtr) MemHandleLock (recordH);
		
		if ( r->flags.alarm )
        {
			ApptUnpack (r, &apptRec);
			
			// Get the first alarm on or after inAlarmStart
			alarmTime = ApptGetAlarmTime (&apptRec, inAlarmStart);
			
			// If in range, add the alarm to the output
			if ( alarmTime && (alarmTime >= inAlarmStart) && (alarmTime <= inAlarmStop) )
            {
				skip = false;
				index = numAlarms;
				
				if (inAlarmInternalsP)
                {
					// If this alarm was snoozed, make room for it at the front of the list
					if (uniqueID == inAlarmInternalsP->snoozeAnchorUniqueID)
                    {
						// If the list is already full, don't bother shifting its contents.
						// The snoozed alarm will overwrite the oldest alarm, which is at
						// the front of the list.
						// If the list is empty, there's nothing to shift.
						if (outAlarmListP && (index != 0) && (index != alarmListSize))
                        {
							MemMove (&outAlarmListP[1], &outAlarmListP[0],
                                     (alarmListSize - 1) * sizeof(*outAlarmListP) );
                        }
						
						// The snoozed alarm is placed at the front of the list
						index = 0;
						
						// Protect the snoozed alarm from being pushed out of a full list
						baseIndex = 1;
                    }

					// Otherwise, skip over any alarms that have already been dismissed
					else if (dismissedCount && (alarmTime == inAlarmStart))
                    {
						for (i = 0; i < dismissedCount; i++)
                        {
							if (dismissedListP [i] == uniqueID)
                            {
								skip = true;
								break;
                            }
                        }
                    }
                }
					
				if (!skip)
                {
					// If collecting alarms, add it to the list
					if (outAlarmListP)
                    {
						// If the alarm list is too large, push the oldest alarm off of the
						// queue to make room for the new one
						if (index >= alarmListSize)
                        {
							// baseIndex is usually 0, but will be set to 1 while snoozing to
							// keep the snoozed alarm from being pushed out of the queue
							MemMove (&outAlarmListP[baseIndex], &outAlarmListP[baseIndex + 1],
                                     (alarmListSize - baseIndex - 1) * sizeof(*outAlarmListP) );
							index = alarmListSize - 1;
                        }
						
						outAlarmListP[index].recordNum = recordNum;
						outAlarmListP[index].alarmTime = alarmTime;
                    }
					
					// If the event is timed, inform the caller to play the alarm sound
					if (outAudibleP && (TimeToInt (apptRec.when->startTime) != apptNoTime) )
                    {
						*outAudibleP = true;
                    }
					
					// Remember the earliest in-range alarm for our return value
					if ( (alarmTime < earliestAlarm) || (earliestAlarm == 0) )
                    {
						earliestAlarm = alarmTime;
                    }
					
					numAlarms++;
                }	// don't skip this alarm
            }	// add alarm to output
        }	// an alarm exists
		
		MemHandleUnlock (recordH);
    }
	
	if (ioCountP)
    {
		*ioCountP = numAlarms;
    }
		
	return earliestAlarm;
}

UInt32
ApptGetTimeOfNextAlarm (
	DmOpenRef					inDbR,			// the open database
	UInt32							inAlarmStart )	// find first alarm on or after this time (in seconds)
{
	UInt32							roundedStart;
	UInt32							nextAlarm;
	
	roundedStart = inAlarmStart - (inAlarmStart % minutesInSeconds);
	if (roundedStart != inAlarmStart)
    {
		roundedStart += minutesInSeconds;
    }
	
	nextAlarm = ApptAlarmMunger (inDbR, roundedStart, apptEndOfTime, NULL, NULL, NULL, NULL);
	
	return nextAlarm;
}
void
ReleaseAlarmInternals (
	PendingAlarmQueueType *	inAlarmInternalsP,
	Boolean						inModified )
{
//#pragma unused (inInterrupt)
//	MemPtrUnlock (inAlarmInternalsP);
	if (inModified)
    {
		PrefSetAppPreferences (sysFileCDatebook, datebookUnsavedPrefID,
                               datebookUnsavedPrefsVersionNum, inAlarmInternalsP,
                               sizeof (PendingAlarmQueueType), false);
    }
}

/***********************************************************************
 *
 * FUNCTION:    ClearPendingAlarms
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb	4/29/99	Initial Revision
 *
 ***********************************************************************/
static void
ClearPendingAlarms (
	PendingAlarmQueueType*	inAlarmInternalsP )
{
	inAlarmInternalsP->pendingCount = 0;
}

/***********************************************************************
 *
 * FUNCTION:    ReserveAlarmInternals
 *
 * DESCRIPTION: Returns the internal struct which stores pending alarms,
 *					 etc. for use by AlarmTrigger and DBDisplayAlarm, et al.
 *					 THIS MAY BE CALLED AT INTERRUPT LEVEL (inInterrupt == true)!
 *					 IF SO, DONT USE GLOBALS!!
 *
 * PARAMETERS:  inInterrupt		If true, use FeatureMgr to retrieve
 *											application globals.
 *
 * RETURNED:    Pointer to the internal struct
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb	4/29/99	Initial Revision
 *
 ***********************************************************************/
void
ReserveAlarmInternals (
	PendingAlarmQueueType*		outAlarmInternalsP )
{
	UInt16 prefsSize = sizeof (PendingAlarmQueueType);
	Int16 prefsVersion;

	prefsVersion = PrefGetAppPreferences
        (sysFileCDatebook, datebookUnsavedPrefID,
         outAlarmInternalsP, &prefsSize, false);
	
	if (prefsVersion == noPreferenceFound)
    {
		MemSet (outAlarmInternalsP, sizeof (PendingAlarmQueueType), 0);
		outAlarmInternalsP->snoozeAnchorTime = apptNoAlarm;
		
		PrefSetAppPreferences (sysFileCDatebook, datebookUnsavedPrefID,
                               datebookUnsavedPrefsVersionNum,
                               outAlarmInternalsP,
                               sizeof (PendingAlarmQueueType), false);
    }
}


/***********************************************************************
 *
 * FUNCTION:    RescheduleAlarms
 *
 * DESCRIPTION: This routine computes the time of the next alarm and 
 *              compares it to the time of the alarm scheduled with
 *              Alarm Manager,  if they are different it reschedules
 *              the next alarm with the Alarm Manager.
 *
 * PARAMETERS:  dbP - the appointment database
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/5/95	Initial Revision
 *			rbb	4/22/99	Snoozing now disables other rescheduling of alarms
 *
 ***********************************************************************/
void RescheduleAlarms (DmOpenRef dbP)
{
	UInt32 ref;
	UInt32 timeInSeconds;
	UInt32 nextAlarmTime;
	UInt32 scheduledAlarmTime;
	PendingAlarmQueueType alarmInternals;
	
	ReserveAlarmInternals (&alarmInternals);
	
	// The pending alarm queue used to be empty, as a rule, any time that
	// RescheduleAlarms could possibly be called. With the addition of the
	// snooze feature, the pending alarm queue may not be empty and, if this
	// is the case, could be invalidated by the changes that prompted the
	// call to RescheduleAlarms. We clear them here and will rebuild the
	// queue in AlarmTriggered().
	ClearPendingAlarms (&alarmInternals);
		
	// only reschedule if we're not snoozing
	if (GetSnoozeAnchorTime (&alarmInternals) == apptNoAlarm)
    {
		scheduledAlarmTime = AlarmGetTrigger (&ref);
		timeInSeconds = TimGetSeconds ();
		if ((timeInSeconds < scheduledAlarmTime) ||
		    (scheduledAlarmTime == 0) ||
		    (ref > 0))
			scheduledAlarmTime = timeInSeconds;
		
		nextAlarmTime = ApptGetTimeOfNextAlarm (dbP, scheduledAlarmTime);
		
		// If the scheduled time of the next alarm is not equal to the
		// calculate time of the next alarm,  reschedule the alarm with 
		// the alarm manager.
		if (scheduledAlarmTime != nextAlarmTime)
        {
			AlarmSetTrigger (nextAlarmTime, 0);
        }
    }
	else
    {
    }
	
	ReleaseAlarmInternals (&alarmInternals, true);
}

