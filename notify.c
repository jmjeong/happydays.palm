/*
HappyDays - A Birthday displayer for the PalmPilot
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

#include <PalmOS.h>
#include <HandEra/Vga.h>

#include "happydays.h"
#include "happydaysRsc.h"
#include "datebook.h"
#include "address.h"
#include "todo.h"
#include "util.h"

////////////////////////////////////////////////////////////////////
// 	Global variable
////////////////////////////////////////////////////////////////////

extern DmOpenRef DatebookDB;
extern DmOpenRef ToDoDB;
extern DmOpenRef MemoDB;

extern Boolean gbVgaExists;
extern struct sPrefsR gPrefsR;
extern TimeFormatType gPreftfmts;  	// happydas.h
extern UInt32 gMmcdate, gMmmdate;   // Memo create/modify time
extern Int16 gMainTableTotals;      // Main Form table total rows;
extern Int16 gMainTableHandleRow;   // The row of birthdate view to display
extern DateType gStartDate;			// staring date of birthday listing

UInt16 gToDoCategory;               // todo category
Char gDateBk3Icon[52][8];      	    // dateBk3 icon string
Boolean gDateBk3IconLoaded = false; // datebk3 icon loaded


////////////////////////////////////////////////////////////////////
// function declaration 
////////////////////////////////////////////////////////////////////

static Boolean IsHappyDaysRecord(Char* notefield);
static void LoadTDNotifyPrefsFields(void);
static void UnloadTDNotifyPrefsFields();
static void NotifyAction(UInt32 whatAlert,
                         void (*func)(int mainDBIndex, DateType when,
							 	      Int8 age,	Int16 *created, 
									  Int16 *touched));
static void NotifyToDo(int mainDBIndex, DateType when, Int8 age,
                       Int16 *created, Int16 *touched);

static void LoadDBNotifyPrefsFields(void);
static void UnloadDBNotifyPrefsFields();
static void NotifyDatebook(int mainDBIndex, DateType when, Int8 age,
                           Int16 *created, Int16 *touched);
static void TimeToAsciiLocal(TimeType when, TimeFormatType timeFormat,
                             Char * pString);
static Boolean LoadDBNotifyPrefsFieldsMore(void);
static void UnloadDBNotifyPrefsFieldsMore();
static void ShowIconStuff();
static void HideIconStuff();
static Int16 CheckDatebookRecord(DateType when, HappyDays birth);
static Char* DateBk3IconString();
static void ChkNMakePrivateRecord(DmOpenRef db, Int16 index);
static Int16 CheckToDoRecord(DateType when, HappyDays birth);
static void LoadCommonPrefsFields(FormPtr frm);
static void UnloadCommonNotifyPrefs(FormPtr frm);
static Boolean IsSameRecord(Char* notefield, HappyDays birth);

// temporary return value;
Char* EventTypeString(HappyDays r)
{
	static char tmpString[4];
    if (!r.custom[0]) {
        tmpString[0] = gPrefsR.Prefs.custom[0];
    }
    else tmpString[0] = r.custom[0];
    tmpString[1] = 0;

    StrToLower(&tmpString[3], tmpString);     // use temporary
    tmpString[3] = tmpString[3] - 'a' + 'A';  // make upper

    return &tmpString[3];
}

void PerformExport(Char * memo, int mainDBIndex, DateType when)
{
    MemHandle recordH = 0;
    PackedHappyDays* rp;
    HappyDays r;
    
    if ((recordH = DmQueryRecord(MainDB, mainDBIndex))) {
        rp = (PackedHappyDays *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackHappyDays(&r, rp);

        StrNCat(memo, "\"", 4096);
        StrNCat(memo, r.name1, 4096);
        if (r.name2 && r.name2[0]) {
            if (r.name1 && r.name1[0]) StrNCat(memo, ", ", 4096);
            StrNCat(memo, r.name2, 4096);
        }
        StrNCat(memo, "\",", 4096);

        if (r.flag.bits.lunar) {
            StrNCat(memo, "-)", 4096);
        }
        else if (r.flag.bits.lunar_leap) {
            StrNCat(memo, "#)", 4096);
        }

        if (r.flag.bits.year) {
            StrNCat(memo, DateToAsciiLong(r.date.month, r.date.day,
                                          r.date.year + 1904, gPrefdfmts,
                                          gAppErrStr), 4096);
        }
        else {
            StrNCat(memo, DateToAsciiLong(r.date.month, r.date.day,
                                          -1, gPrefdfmts,
                                          gAppErrStr), 4096);
        }
        StrNCat(memo, ",", 4096);
        
        // print converted date
        //
        if (when.year) {
            StrNCat(memo, DateToAsciiLong(when.month, when.day,
                                          when.year + 1904, gPrefdfmts,
                                          gAppErrStr), 4096);
        }
        else {
           StrNCat(memo, DateToAsciiLong(when.month, when.day,
                                          -1, gPrefdfmts,
                                          gAppErrStr), 4096);
        }

        StrNCat(memo, ",", 4096);
        StrNCat(memo, EventTypeString(r), 4096);
        
        MemHandleUnlock(recordH);
    }
}

int CleanupFromTD(DmOpenRef db)
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
        note = GetToDoNotePtr(toDoRec);
        
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

int CleanupFromDB(DmOpenRef db)
{
    UInt16 currIndex = 0;
    MemHandle recordH;
    ApptDBRecordType apptRecord;
    int ret = 0;
    
    while (1) {
        recordH = DmQueryNextInCategory(db, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;
        ApptGetRecord(db, currIndex, &apptRecord, &recordH);
        if (IsHappyDaysRecord(apptRecord.note)) {
            // if it is happydays record?
            //
            ret++;
            // remove the record
            DmDeleteRecord(db, currIndex);
            // deleted records are stored at the end of the database
            //
            DmMoveRecord(db, currIndex, DmNumRecords(DatebookDB));
        }
        else {
            MemHandleUnlock(recordH);
            currIndex++;
        }
    }
    return ret;
}

Boolean ToDoFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(ToDoNotifyForm);

    switch (e->eType) {
    case frmOpenEvent:
        if(gbVgaExists)
       		VgaFormModify(frm, vgaFormModify160To240);

        LoadTDNotifyPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case ToDoNotifyFormOk: 
        {
            UnloadTDNotifyPrefsFields();

            NotifyAction(ToDoNotifyForm, NotifyToDo);
            FrmReturnToForm(0);

            handled = true;
            break;
        }
        
        case ToDoNotifyFormCancel:
            UnloadTDNotifyPrefsFields();

            FrmReturnToForm(0);

            handled = true;
            break;

        case ToDoPopupTrigger: 
        {
            SelectCategoryPopup(ToDoDB, &gToDoCategory,
                                ToDoNotifyCategory, ToDoPopupTrigger,
                                gPrefsR.TDNotifyPrefs.todoCategory);
            handled = true;
            break;
        }
            
        default:
            break;
        }
    case menuEvent:
        MenuEraseStatus(NULL);
        handled = true;
        break;

    default:
        break;
    }

    return handled;
}

Boolean DBNotifyFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(DateBookNotifyForm);

    switch (e->eType) {
    case frmOpenEvent:
        if(gbVgaExists)
       		VgaFormModify(frm, vgaFormModify160To240);

        LoadDBNotifyPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case DateBookNotifyFormOk: 
        {
            UnloadDBNotifyPrefsFields();

            NotifyAction(DateBookNotifyForm, NotifyDatebook);
            FrmReturnToForm(0);

            handled = true;
            break;
        }
        
        case DateBookNotifyFormCancel:
            UnloadDBNotifyPrefsFields();

            FrmReturnToForm(0);

            handled = true;
            break;

		case DateBookNotifyFormMore:
            FrmPopupForm(DateBookNotifyMoreForm);

            handled = true;
            break;

        case DateBookNotifyFormAlarm:
            if (CtlGetValue(GetObjectPointer(frm,
                                             DateBookNotifyFormAlarm))) {
                FrmShowObject(frm,
                              FrmGetObjectIndex(frm, DateBookNotifyFormBefore));

				SysCopyStringResource(gAppErrStr, DateBookNotifyDaysString);
				FldDrawField(SetFieldTextFromStr(DateBookNotifyFormLabelDays, 
                                                 gAppErrStr));
            }
            else {
                FrmHideObject(frm, 
                              FrmGetObjectIndex(frm, DateBookNotifyFormBefore));
				FldDrawField(ClearFieldText(DateBookNotifyFormLabelDays));
            }
            
            handled = true;
            break;

        case DateBookNotifyFormTime:
        {
            TimeType startTime, endTime, lastTime;
            Boolean untimed = false;

            startTime = endTime = lastTime = gPrefsR.DBNotifyPrefs.when;
            if (TimeToInt(startTime) == noTime) {
                untimed = true;
                // set dummy time
                startTime.hours = endTime.hours  = 8;
                startTime.minutes = endTime.minutes = 0;
            }
            
            SysCopyStringResource(gAppErrStr, SelectTimeString);
            if (SelectTimeV33(&startTime, &endTime, untimed, gAppErrStr,8)) {
                gPrefsR.DBNotifyPrefs.when = startTime;
                TimeToAsciiLocal(gPrefsR.DBNotifyPrefs.when, gPreftfmts,
                                 gAppErrStr);
            }
			else {
				TimeToAsciiLocal(lastTime, gPreftfmts, gAppErrStr);
			}
            
			CtlSetLabel(GetObjectPointer(frm, DateBookNotifyFormTime),
						gAppErrStr);

            handled = true;
            break;
        }
            
        default:
            break;
        }
    case menuEvent:
        MenuEraseStatus(NULL);
        handled = true;
        break;

    default:
        break;
    }

    return handled;
}

Boolean DBNotifyFormMoreHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetActiveForm();
    Boolean loadDateBk3Icon;

    switch (e->eType) {
    case frmOpenEvent: 
    {
        TablePtr tableP;
        Int16 numColumns;

        if(gbVgaExists) {
       		VgaFormModify(frm, vgaFormModify160To240);
        }

        loadDateBk3Icon = LoadDBNotifyPrefsFieldsMore();
        FrmDrawForm(frm);

        if (loadDateBk3Icon) {
            tableP = GetObjectPointer(frm, DateBookNotifyBk3Table);
            numColumns = 13;
            TblSelectItem(tableP, gPrefsR.DBNotifyPrefs.datebk3icon/numColumns,
                          gPrefsR.DBNotifyPrefs.datebk3icon % numColumns);
        }

        handled = true;
        break;
    }

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case DateBookNotifyMoreFormOk: 
        {
            UnloadDBNotifyPrefsFieldsMore();

            FrmReturnToForm(0);
            handled = true;
            break;
        }
        case DateBookNotifyFormIcon:
        {
            if (CtlGetValue(GetObjectPointer(frm, DateBookNotifyFormIcon)))
            {
                ShowIconStuff();
            }
            else {
                HideIconStuff();
            }
            
            handled = true;
            break;
        }
      
        default:
            break;
        }
    case tblSelectEvent: 
    {
        if (e->data.tblSelect.tableID == DateBookNotifyBk3Table) {
            gPrefsR.DBNotifyPrefs.datebk3icon =
                e->data.tblSelect.column + e->data.tblSelect.row * 13;

            handled = true;
            break;
        }
    }

    case menuEvent:
        MenuEraseStatus(NULL);
        handled = true;
        break;

    default:
        break;
    }

    return handled;
}

static Boolean IsHappyDaysRecord(Char* notefield)
{
    if (notefield && StrStr(notefield, gPrefsR.Prefs.notifywith)) {
        return true; 
    }
    return false;
}

static Char* gNotifyFormatString[5] =
{ "[+L] +e +y",     // [Jeong, JaeMok] B 1970
  "[+F] +e +y",     // [JaeMok Jeong] B 1970
  "+E - +L +y",     // Birthday - Jeong, JaeMok 1970
  "+E - +F +y",     // Birthday - JaeMok Jeong 1970
  "+F +y",        	// JaeMok Jeong 1970
};

//
// Memory is allocated, after calling this routine, user must free the memory
//
static Char* NotifyDescString(DateType when, HappyDays birth, 
							  Int8 age,Boolean todo)
{
    Char* description, *pDesc;
    Char* pfmtString;
    
    // make the description
    description = pDesc = MemPtrNew(1024);
    SysCopyStringResource(gAppErrStr, NotEnoughMemoryString);
    ErrFatalDisplayIf(!description, gAppErrStr);

    // boundary check must be inserted
    //
    pfmtString = gNotifyFormatString[(int)gPrefsR.Prefs.notifyformat];
    while (*pfmtString) {
        if (*pfmtString == '+') {
            switch (*(pfmtString+1)) {
            case 'L':       // LastName, FirstName
                StrCopy(pDesc, birth.name1);
                if (birth.name2 && birth.name2[0]) {
                    if (birth.name1 && birth.name1[0])
                        StrCat(pDesc, ", ");
                    StrCat(pDesc, birth.name2);
                }

                pDesc += StrLen(pDesc);
                break;

            case 'F':       // FirstName LastName
                StrCopy(pDesc, birth.name2);
                
                if (birth.name1 && birth.name1[0]) {
                    if (birth.name2 && birth.name2[0])
                        StrCat(pDesc, " ");
                    StrCat(pDesc, birth.name1);
                }

                pDesc += StrLen(pDesc);
                break;
                
            case 'e':       // Event Type (like 'B')
                // set event type
                *pDesc++ = EventTypeString(birth)[0];
                
                break;

            case 'E':       // Event Type (like 'Birthday')
                if (!birth.custom[0]) {
                    StrCopy(pDesc, gPrefsR.Prefs.custom);
                }
                else {
                    StrCopy(pDesc, birth.custom);
                }
                pDesc += StrLen(pDesc);
                break;

            case 'y':       // Year and age
                if (birth.flag.bits.lunar || birth.flag.bits.lunar_leap) {
                    if (birth.flag.bits.lunar) {
                        StrCopy(pDesc, "-)");
                    }
                    else if (birth.flag.bits.lunar_leap) {
                        StrCopy(pDesc, "#)");
                    }

                    if (birth.flag.bits.year) {
                        DateToAscii(birth.date.month, birth.date.day,
                                    birth.date.year + 1904,
                                    gPrefdfmts, gAppErrStr);
                    }
                    else {
                        DateToAsciiLong(birth.date.month, birth.date.day, -1, gPrefdfmts,
                                        gAppErrStr);
                    }
    
                    StrCat(pDesc, gAppErrStr);
                    pDesc += StrLen(pDesc);
                }
                if (birth.flag.bits.year) {
                    if (!birth.flag.bits.solar 
                        || gPrefsR.DBNotifyPrefs.duration ==1 || todo) {
                        if (age >= 0) {
                            StrPrintF(gAppErrStr, " (%d)", age);
                            StrCopy(pDesc, gAppErrStr);
                        }
                    }
                    else if (birth.flag.bits.solar 
                             && gPrefsR.DBNotifyPrefs.duration != 1) {
                        StrPrintF(gAppErrStr, "%d", birth.date.year + 1904 );
                        StrCopy(pDesc, gAppErrStr);
                    }
                    pDesc += StrLen(pDesc);
                }
                break;

            default:
            }

            pfmtString += 2;    // advance +char;
        }
        else {
            *pDesc++ = *pfmtString++;
        }
    }
    *pDesc = 0;
    
    return description;
}
    
static Int16 PerformNotifyDB(HappyDays birth, DateType when, Int8 age,
                             RepeatInfoType* repeatInfoPtr,
                             Int16 *created, Int16 *touched)
{
    ApptDBRecordType datebook;
    ApptDateTimeType dbwhen;
    AlarmInfoType dbalarm;
    Char* description = 0;
    Int16 existIndex;
    ApptDBRecordFlags changedFields;
    Char noteField[256];        // (datebk3: 10, AN:14), HD id: 5

    // for the performance, check this first 
    if ( ((existIndex = CheckDatebookRecord(when, birth)) >= 0)
         && !gPrefsR.existing ) {    // exist and keep the record
        (*touched)++;
        
        return 0;
    }

    /* Zero the memory */
    MemSet(&datebook, sizeof(ApptDBRecordType), 0);
    MemSet(&dbwhen, sizeof(ApptDateTimeType), 0);
    MemSet(&dbalarm, sizeof(AlarmInfoType), 0);
    datebook.when = &dbwhen;
    datebook.alarm = &dbalarm;

    // set the date and time
    //
    dbwhen.date = when;
    dbwhen.startTime = gPrefsR.DBNotifyPrefs.when;
    dbwhen.endTime = gPrefsR.DBNotifyPrefs.when;

    // if alarm checkbox is set, set alarm
    //
    if (gPrefsR.DBNotifyPrefs.alarm &&
        gPrefsR.DBNotifyPrefs.notifybefore >= 0) {
        dbalarm.advanceUnit = aauDays;
        dbalarm.advance = gPrefsR.DBNotifyPrefs.notifybefore;
        datebook.alarm = &dbalarm;
    }
    else {
        dbalarm.advance = apptNoAlarm;
        datebook.alarm = NULL;
    }

    // set the repeat
    //
    datebook.repeat = repeatInfoPtr;

    // forget the exceptions 
    //
    datebook.exceptions = NULL;

	// if datebook is datebk3 or AN, set icon
	//
    if (gPrefsR.DBNotifyPrefs.icon == 1) {     // AN
        StrCopy(noteField, "ICON: ");
        StrCat(noteField, gPrefsR.DBNotifyPrefs.an_icon);
        StrCat(noteField, "\n#AN\n");
    }
	else if (gPrefsR.DBNotifyPrefs.icon == 2) {
        StrCopy(noteField, DateBk3IconString());
	}
    else noteField[0] = 0;

    StrCat(noteField, gPrefsR.Prefs.notifywith);
    StrPrintF(gAppErrStr, "%ld", Hash(birth.name1, birth.name2));
    StrCat(noteField, gAppErrStr);
    datebook.note = noteField;

    // make the description
        
    description = NotifyDescString(when, birth, age, false);	// todo = false
    datebook.description = description;

    if (existIndex < 0) {            // there is no same record
        //
        // if not exists
        // write the new record (be sure to fill index)
        //
        ApptNewRecord(DatebookDB, &datebook, &existIndex);
        // if private is set, make the record private
        //
        ChkNMakePrivateRecord(DatebookDB, existIndex);
        (*created)++;
    }
    else {                                      // if exists
        if (gPrefsR.existing == 0) {
            // keep the record
        }
        else {
            // modify
            changedFields.when = 1;
            changedFields.description = 1;
            changedFields.repeat = 1;
            changedFields.alarm = 1;
			changedFields.note = 1;

            ApptChangeRecord(DatebookDB, &existIndex, &datebook,
                             changedFields);
            // if private is set, make the record private
            //
            ChkNMakePrivateRecord(DatebookDB, existIndex);
        }
        (*touched)++;
    }

    if (description) MemPtrFree(description);
    
    return 0;
}

static Int16 PerformNotifyTD(HappyDays birth, DateType when, Int8 age,
                             Int16 *created, Int16 *touched)
{
    Char noteField[256];      // (datebk3: 10, AN:14), HD id: 5
    ToDoItemType todo;
    Char* description = 0;
    
    Int16 existIndex;

    // for the performance, check this first 
    if ( ((existIndex = CheckToDoRecord(when, birth)) >= 0)
         && !gPrefsR.existing ) {    // exists and keep the record
        (*touched)++;
        
        return 0;
    }

    /* Zero the memory */
    MemSet(&todo, sizeof(ToDoItemType), 0);

    // set the date 
    //
    todo.dueDate = when;
    todo.priority = gPrefsR.TDNotifyPrefs.priority;

    StrCopy(noteField, gPrefsR.Prefs.notifywith);
    StrPrintF(gAppErrStr, "%ld", Hash(birth.name1, birth.name2));
    StrCat(noteField, gAppErrStr);
    todo.note = noteField;

    // make the description
        
    description = NotifyDescString(when, birth, age, true);		// todo = true
    todo.description = description;

    // category adjust
    if (gToDoCategory == dmAllCategories) gToDoCategory = dmUnfiledCategory;
    
    if (existIndex < 0) {            // there is no same record
        //
        // if not exists
        // write the new record (be sure to fill index)
        //
        ToDoNewRecord(ToDoDB, &todo, gToDoCategory, &existIndex);
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
            ToDoNewRecord(ToDoDB, &todo, gToDoCategory, &existIndex);
            // if private is set, make the record private
            //
            ChkNMakePrivateRecord(ToDoDB, existIndex);
        }
        (*touched)++;
    }

    if (description) MemPtrFree(description);
    
    return 0;
}


static void AdjustDesktopCompatible(DateType *when)
{
    int maxtry = 4;
    // get current date
    //
    
    if (when->year < 1990 - 1904) when->year = 1990 - 1904;
                
    while (DaysInMonth(when->month, when->year) < when->day && maxtry-- >0) {
        when->year++;
    }
}

static void NotifyDatebook(int mainDBIndex, DateType when, Int8 age,
                           Int16 *created, Int16 *touched)
{
    MemHandle recordH = 0;
    PackedHappyDays* rp;
    HappyDays r;
    RepeatInfoType repeatInfo;
    Int16 i;
    
    // if duration is equal to 0, no notify record is created
    //
    if (gPrefsR.DBNotifyPrefs.duration == 0) return;

    if ((recordH = DmQueryRecord(MainDB, mainDBIndex))) {
        rp = (PackedHappyDays *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackHappyDays(&r, rp);

        if (r.flag.bits.solar) {
            repeatInfo.repeatType = repeatYearly;
            repeatInfo.repeatFrequency = 1;
            repeatInfo.repeatOn = 0;
            repeatInfo.repeatStartOfWeek = 0;

            if (gPrefsR.DBNotifyPrefs.duration > 1) {
                UInt16 end_year;

                repeatInfo.repeatEndDate = when;
                // if end year is more than 2031, adjust it.
                //
                //  'when' is the current year. 
                //
                end_year = when.year + gPrefsR.DBNotifyPrefs.duration -1;

                if (end_year > 2031 - 1904) {
                    repeatInfo.repeatEndDate.year = 2031-1904;
                }
				else {
					repeatInfo.repeatEndDate.year = end_year;
				}
                // if duration > 1, 'when' is the birthdate;
                //     change when.year into original birthday.
                if (r.flag.bits.year) when = r.date;
				// Because Palm Desktop doesn't support events before 1970.
				//
                AdjustDesktopCompatible(&when);
                
                PerformNotifyDB(r, when, age, &repeatInfo, created, touched);
            }
            else if (gPrefsR.DBNotifyPrefs.duration == -1) {
                // if duration > 1, 'when' is the birthdate;
                if (r.flag.bits.year) when = r.date;

				// Because Palm Desktop doesn't support events before 1970.
				//
                AdjustDesktopCompatible(&when);

                DateToInt(repeatInfo.repeatEndDate) = -1;
                PerformNotifyDB(r, when, age, &repeatInfo, created, touched);
            }
            else PerformNotifyDB(r, when, age, NULL, created, touched);
        }
        else if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {
            // if lunar date, make each entry
            //
            int duration = gPrefsR.DBNotifyPrefs.duration;
            DateType current, converted;
            
            // for the convenience, -1 means 5 year in lunar calendar.
            //
            if (duration == -1) duration = DEFAULT_DURATION;

            // now use the same routine in lunar an lunar_leap
            //
            // DateSecondsToDate(TimGetSeconds(), &current);
			current = gStartDate;

            for (i=0; i < duration; i++) {
                converted = r.date;
                if (!FindNearLunar(&converted, current,
                                   r.flag.bits.lunar_leap)) break;

                PerformNotifyDB(r, converted, age, NULL, created, touched);

                // process next target day
                //
                current = converted;
                DateAdjust(&current, 1);
            }
        }
        
        MemHandleUnlock(recordH);
    }
}

static void NotifyToDo(int mainDBIndex, DateType when, Int8 age,
                       Int16 *created, Int16 *touched)
{
    MemHandle recordH = 0;
    PackedHappyDays* rp;
    HappyDays r;
    
    if ((recordH = DmQueryRecord(MainDB, mainDBIndex))) {
        rp = (PackedHappyDays *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackHappyDays(&r, rp);

        if (r.flag.bits.solar) {
            PerformNotifyTD(r, when, age, created, touched);
        }
        else if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {
            // if lunar date, make each entry
            //
            DateType current, converted;
            
            // now use the same routine in lunar an lunar_leap
            //
            // DateSecondsToDate(TimGetSeconds(), &current);
			current = gStartDate;

            converted = r.date;
            if (FindNearLunar(&converted, current,
                              r.flag.bits.lunar_leap)) {
                PerformNotifyTD(r, converted, age, created, touched);
            }
        }
        
        MemHandleUnlock(recordH);
    }
}

static void NotifyAction(UInt32 whatAlert,
                         void (*func)(int mainDBIndex, DateType when,
							 	      Int8 age,	Int16 *created, 
									  Int16 *touched))
{
    Int16 created = 0, touched = 0;
    LineItemPtr ptr;
    FormPtr formPtr;
    Int16 i;
    Char* info = 0;
    Char tmp[25];
    Char temp[255];

    if (gMainTableTotals >0) {
        ptr = MemHandleLock(gTableRowHandle);
        if (gPrefsR.records == 0) {  // all
            if (whatAlert == DateBookNotifyForm) {
                SysCopyStringResource(gAppErrStr, DateBookString);
            }
            else {
                SysCopyStringResource(gAppErrStr, ToDoString);
            }

            StrPrintF(tmp, "%d", gMainTableTotals);
            if (FrmCustomAlert(NotifyWarning, gAppErrStr, tmp, " ")
                == 0) {             // user select OK button

                formPtr = FrmInitForm(ProgressForm);
        		if(gbVgaExists)
                    VgaFormModify(formPtr, vgaFormModify160To240);

                FrmDrawForm(formPtr);
                FrmSetActiveForm(formPtr);
                
                for (i=0; i < gMainTableTotals; i++) {
                    if (ptr[i].date.year != INVALID_CONV_DATE) {
                        (*func)(ptr[i].birthRecordNum,
                                ptr[i].date, 
								ptr[i].age,
								&created,
                                &touched);
                    }

                    if ((i+1) % 20 == 0) {
                        // Reset the auto-off timer to make sure we don't
                        // fall asleep in the
                        //  middle of the update
                        EvtResetAutoOffTimer ();
                    }

                    StrPrintF(gAppErrStr, "%d/%d", i+1, gMainTableTotals);
                    FldDrawField(SetFieldTextFromStr(ProgressFormField, gAppErrStr));
                }

                FrmReturnToForm(0);
            }
            else {
                MemPtrUnlock(ptr);
                goto Exit_Notify_All;
            }
                    
        }
        else {                                      // selected
            if (ptr[gMainTableHandleRow].date.year
                != INVALID_CONV_DATE) {
                (*func)(ptr[gMainTableHandleRow].birthRecordNum,
                        ptr[gMainTableHandleRow].date,
                        ptr[gMainTableHandleRow].age,
                        &created, &touched);
            }
        }
        MemPtrUnlock(ptr);

        info = MemPtrNew(255);
        SysCopyStringResource(gAppErrStr, NotEnoughMemoryString);
        ErrFatalDisplayIf(!info, gAppErrStr);

        SysCopyStringResource(gAppErrStr, NotifyCreateString);
        StrPrintF(info, gAppErrStr, created);

        SysCopyStringResource(temp, ExistingEntryString);
        StrPrintF(gAppErrStr, temp, touched);
        StrNCat(info, gAppErrStr, 255);

        if (gPrefsR.existing == 0) {  // keep
            SysCopyStringResource(temp, UntouchedString);
            StrNCat(info, temp, 255);
        }
        else {                                      // modify
            SysCopyStringResource(temp, ModifiedString);
            StrNCat(info, temp, 255);
        }

        // notify the number of the created and touched record
        //
        FrmCustomAlert(ErrorAlert, info, " ", " ");

        MemPtrFree(info);
    }
 Exit_Notify_All:                

    return;
}

static Char* DateBk3IconString()
{
    static Char bk3Icon[11];

    StrCopy(bk3Icon, "##@@@@@@@\n");
    bk3Icon[5] = gPrefsR.DBNotifyPrefs.datebk3icon + 'A';

    return bk3Icon;
}

static int dec(char hex)
{
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    }
    else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 10;
    }
    else if (hex >= 'a' && hex <= 'f') {
        return hex - 'a' + 10;
    }
    else return 0;  
}

static Int8 convertWord(char first, char second)
{
    return dec(first) * 16 + dec(second);
}

static void DateBk3CustomDrawTable(MemPtr tableP, Int16 row, Int16 column, 
                                   RectanglePtr bounds)
{
    Int16 x, y;
    Int8 drawItem = row * 13 + column;
    Int8 drawFixel;
    Int8 i, j;
    
    x = bounds->topLeft.x + (bounds->extent.x - 8) / 2 -1;
    y = bounds->topLeft.y + (bounds->extent.y - 8) / 2 -1; 

    for (i = 0; i < 8; i++) {
        drawFixel = gDateBk3Icon[drawItem][i];
        if (drawFixel) {        // if not 0
            for (j=0; j < 8; j++) {
                if (drawFixel & 1) {
                    WinDrawLine(x+8-j, y+i+1, x+8-j, y+i+1);
                }
                drawFixel >>= 1;
            }
        }
    }
}

static Boolean loadDatebk3Icon()
{
    UInt16 currIndex = 0;
    MemHandle recordH = 0;
    Char* rp;       // memoPad record
    Boolean found = false;
    Char IconString[16];
    Char *p=0;

    if ((MemCmp((char*) &gMmcdate, gPrefsR.memcdate, 4) == 0) &&
        (MemCmp((char*) &gMmmdate, gPrefsR.memmdate, 4) == 0)) {
        // if matched, use the original datebk3 icon set
        //
        return gDateBk3IconLoaded;
    }

    while (1) {
        recordH = DmQueryNextInCategory(MemoDB, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;

        rp = (Char*)MemHandleLock(recordH);
        if (StrNCompare(rp, DATEBK3_MEMO_STRING,
                        StrLen(DATEBK3_MEMO_STRING)) == 0) {
            p = StrChr(rp, '\n');
            found = true;
            break;
        }
            
        MemHandleUnlock(recordH);
        currIndex++;
    }
    if (found) {
        Int8 i=0, j;
        
        while ((p = StrChr(p+1, '=')) && i < 52) {
            MemMove(IconString, p+1, 16);

            for (j = 0; j < 8; j++) {
                gDateBk3Icon[i][j] =
                    convertWord(IconString[j*2], IconString[j*2+1]);
            }
            i++;
        }

        for (; i < 52; i++) {
            MemSet(gDateBk3Icon[i], 8, 0);
        }
        MemHandleUnlock(recordH);
    }
    
    return (gDateBk3IconLoaded = found);
}

static Boolean DateBk3IconLoadTable(FormPtr frm, Boolean redraw)
{
    TablePtr tableP;
    Int8 row, column, numRows, numColumns;
    RectangleType rect;

    if (!loadDatebk3Icon()) return false;

    tableP = GetObjectPointer(frm, DateBookNotifyBk3Table);
    if (redraw) {
        TblEraseTable(tableP);
    }

    numRows = TblGetNumberOfRows(tableP);
    numColumns = 13;
    TblGetBounds(tableP, &rect);
    
    for (row=0; row < numRows; row++) {
        for (column=0; column < numColumns; column++) {
            TblSetItemStyle(tableP, row, column, customTableItem);
        }
        TblSetRowSelectable(tableP, row, true);

        TblSetRowHeight(tableP, row, rect.extent.y / 4 +1);
        TblSetRowUsable(tableP, row, true);
    }

    for (column=0; column < numColumns; column++) {
        TblSetColumnUsable(tableP, column, true);
        TblSetCustomDrawProcedure(tableP, column, DateBk3CustomDrawTable);
    }

    if (redraw) {
        TblDrawTable(tableP);
    }
    return true;
}

static void HideIconStuff()
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(DateBookNotifyMoreForm)) == 0) return;

    FrmHideObject(frm, FrmGetObjectIndex(frm, DateBookNotifyFormBk3));
    FrmHideObject(frm, FrmGetObjectIndex(frm, DateBookNotifyFormAN));
}

static void ShowIconStuff()
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(DateBookNotifyMoreForm)) == 0) return;

    FrmShowObject(frm, FrmGetObjectIndex(frm, DateBookNotifyFormBk3));
    FrmShowObject(frm, FrmGetObjectIndex(frm, DateBookNotifyFormAN));
}

static Boolean LoadDBNotifyPrefsFieldsMore(void)
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(DateBookNotifyMoreForm)) == 0) return false;

    if (gPrefsR.DBNotifyPrefs.icon == 0) {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormIcon), 0);
        HideIconStuff();
    }
    else {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormIcon), 1);
    }
    
    if (gPrefsR.DBNotifyPrefs.icon == 1) {     // AN selected
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormAN), 1);
    }
    else {                                      // Datebk3 Icon selected 
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormBk3), 1);
    }

    SetFieldTextFromStr(DateBookNotifyANInput,
                        gPrefsR.DBNotifyPrefs.an_icon);

    return DateBk3IconLoadTable(frm, false);

}

static void UnloadDBNotifyPrefsFieldsMore()
{
    FormPtr frm;
    ControlPtr ptr;
    
    if ((frm = FrmGetFormPtr(DateBookNotifyMoreForm)) == 0) return;
    
    ptr = GetObjectPointer(frm, DateBookNotifyFormIcon);
    if (!CtlGetValue(ptr)) gPrefsR.DBNotifyPrefs.icon = 0;
    else {
        ptr = GetObjectPointer(frm, DateBookNotifyFormAN);
        if (CtlGetValue(ptr)) gPrefsR.DBNotifyPrefs.icon = 1;
        else gPrefsR.DBNotifyPrefs.icon = 2;
    }
    
    if (FldDirty(GetObjectPointer(frm, DateBookNotifyANInput))) {
        if (FldGetTextPtr(GetObjectPointer(frm, DateBookNotifyANInput))) {
            StrNCopy(gPrefsR.DBNotifyPrefs.an_icon,
                     FldGetTextPtr(GetObjectPointer(frm, DateBookNotifyANInput)), 9);
        }
    }
}

static void LoadDBNotifyPrefsFields(void)
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(DateBookNotifyForm)) == 0) return;

    LoadCommonPrefsFields(frm);  // all/selected, keep/modify, private
    
    if (gPrefsR.DBNotifyPrefs.alarm == 1) {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormAlarm), 1);

        FrmShowObject(frm, 
                      FrmGetObjectIndex(frm, DateBookNotifyFormBefore));

		SysCopyStringResource(gAppErrStr, DateBookNotifyDaysString);
		SetFieldTextFromStr(DateBookNotifyFormLabelDays, gAppErrStr);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormAlarm), 0);
        FrmHideObject(frm,
                      FrmGetObjectIndex(frm, DateBookNotifyFormBefore));
		ClearFieldText(DateBookNotifyFormLabelDays);
    }
    
    SetFieldTextFromStr(DateBookNotifyFormBefore,
                        StrIToA(gAppErrStr, gPrefsR.DBNotifyPrefs.notifybefore));
    SetFieldTextFromStr(DateBookNotifyFormDuration,
                        StrIToA(gAppErrStr, gPrefsR.DBNotifyPrefs.duration));
    
    TimeToAsciiLocal(gPrefsR.DBNotifyPrefs.when, gPreftfmts, gAppErrStr);
    CtlSetLabel(GetObjectPointer(frm, DateBookNotifyFormTime), gAppErrStr);
}

static void UnloadDBNotifyPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    
    if ((frm = FrmGetFormPtr(DateBookNotifyForm)) == 0) return;

    UnloadCommonNotifyPrefs(frm);       // all/selected, keey/modify, private

    ptr = GetObjectPointer(frm, DateBookNotifyFormAlarm);
    gPrefsR.DBNotifyPrefs.alarm = CtlGetValue(ptr);

    if (FldDirty(GetObjectPointer(frm, DateBookNotifyFormBefore))) {
        gPrefsR.DBNotifyPrefs.notifybefore =
            (int) StrAToI(FldGetTextPtr(
                GetObjectPointer(frm, DateBookNotifyFormBefore)));
    }
    // notification duration
    if (FldDirty(GetObjectPointer(frm, DateBookNotifyFormDuration))) {
        gPrefsR.DBNotifyPrefs.duration =
            (int) StrAToI(FldGetTextPtr(
                GetObjectPointer(frm, DateBookNotifyFormDuration)));
    }
    
    // DateBookNotifyFormTime is set by handler
}


static void LoadTDNotifyPrefsFields(void)
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(ToDoNotifyForm)) == 0) return;

    LoadCommonPrefsFields(frm);  // all/selected, keep/modify, private

    switch (gPrefsR.TDNotifyPrefs.priority) {
    case 1:
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri1), 1);
        break;
    case 2:
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri2), 1);
        break;
    case 3:
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri3), 1);
        break;
    case 4:
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri4), 1);
        break;
    case 5:
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri5), 1);
        break;
    default:
    }
    DisplayCategory(ToDoPopupTrigger, gPrefsR.TDNotifyPrefs.todoCategory, false);

    // set gToDoCategory information
    //
    if ((gToDoCategory = CategoryFind(ToDoDB, gPrefsR.TDNotifyPrefs.todoCategory))
        == dmAllCategories) {
        MemSet(gPrefsR.TDNotifyPrefs.todoCategory,
               sizeof(gPrefsR.TDNotifyPrefs.todoCategory), 0);
        CategoryGetName(ToDoDB, gToDoCategory, gPrefsR.TDNotifyPrefs.todoCategory);
    }
}

static void UnloadTDNotifyPrefsFields()
{
    FormPtr frm;
    
    if ((frm = FrmGetFormPtr(ToDoNotifyForm)) == 0) return;
    UnloadCommonNotifyPrefs(frm);       // all/selected, keey/modify, private

    if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri1)) )
        gPrefsR.TDNotifyPrefs.priority = 1;
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri2)) )
        gPrefsR.TDNotifyPrefs.priority = 2;
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri3)) )
        gPrefsR.TDNotifyPrefs.priority = 3;
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri4)) )
        gPrefsR.TDNotifyPrefs.priority = 4;
    else
        gPrefsR.TDNotifyPrefs.priority = 5;
    
    // ToDo Category is set by handler
}


static void TimeToAsciiLocal(TimeType when, TimeFormatType timeFormat,
                             Char * pString)
{
    if (TimeToInt(when) != noTime) {
        TimeToAscii(when.hours, when.minutes, timeFormat, pString);
    }
    else {
        SysCopyStringResource(gAppErrStr, NoTimeString);
    }
}

static void LoadCommonPrefsFields(FormPtr frm)
{
    if (gPrefsR.records == 0) {
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordAll), 1);
    }
    else if (gPrefsR.records == 1) {
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordSlct), 1);
    }
    else {          //  '3' means that All is selected and slct is unusable
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordAll), 1);
        CtlSetUsable(GetObjectPointer(frm, NotifyFormRecordSlct), false);
    }
    if (gPrefsR.existing == 0) {
        CtlSetValue(GetObjectPointer(frm, NotifyFormEntryKeep), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifyFormEntryModify), 1);
    }

    if (gPrefsR.private == 1) {
        CtlSetValue(GetObjectPointer(frm, NotifyFormPrivate), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifyFormPrivate), 0);
    }

}

static void UnloadCommonNotifyPrefs(FormPtr frm)
{
    ControlPtr ptr;
    
    ptr = GetObjectPointer(frm, NotifyFormRecordAll);
    gPrefsR.records = !CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, NotifyFormEntryKeep);
    gPrefsR.existing = !CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, NotifyFormPrivate);
    gPrefsR.private = CtlGetValue(ptr);

    return;
}


static Int16 CheckDatebookRecord(DateType when, HappyDays birth)
{
    UInt16 numAppoints = 0;
    MemHandle apptListH;
    ApptDBRecordType dbRecord;
    ApptInfoPtr apptList;
    UInt16 recordNum;
    MemHandle recordH;
    Int16 i;

    // check the exisiting records
    //
    apptListH = ApptGetAppointments(DatebookDB, when, &numAppoints);
    if (apptListH) {
        apptList = MemHandleLock(apptListH);
        for (i = 0; i < numAppoints; i++) {
            ApptGetRecord(DatebookDB, apptList[i].recordNum,
                          &dbRecord, &recordH);
            // if matched one is exists, return recordNum;
            //
            if (recordH) {
                if (IsSameRecord(dbRecord.note, birth)) {
                    recordNum = apptList[i].recordNum;
                    
                    MemHandleUnlock(recordH);
                    MemHandleUnlock(apptListH);

                    return recordNum;
                }
                else MemHandleUnlock(recordH);
            }
        }
        MemHandleUnlock(apptListH);
    }
    return -1;
}

static Int16 CheckToDoRecord(DateType when, HappyDays birth)
{
    UInt16 currIndex = 0;
    MemHandle recordH;
    ToDoDBRecordPtr toDoRec;
    Char *note;
    
    while (1) {
        recordH = DmQueryNextInCategory(ToDoDB, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;

        toDoRec = MemHandleLock(recordH);
        note = GetToDoNotePtr(toDoRec);
        
        if ((DateToInt(when) == DateToInt(toDoRec->dueDate))
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

// check global notify preference, and make datebookDB entry private
//
static void ChkNMakePrivateRecord(DmOpenRef db, Int16 index)
{
    Int16 attributes;
    // if private is set, make the record private
    //
    if (gPrefsR.private == 1) {
        DmRecordInfo(db, index, &attributes,
                     NULL,NULL);
        attributes |= dmRecAttrSecret;
        DmSetRecordInfo(db, index, &attributes, NULL);
    }
}

// check if description has the information about name1 and name2
//
static Boolean IsSameRecord(Char* notefield, HappyDays birth)
{
    Char *p;
    
    if (notefield && (p = StrStr(notefield,gPrefsR.Prefs.notifywith))) {
        p += StrLen(gPrefsR.Prefs.notifywith);

        StrPrintF(gAppErrStr, "%ld", Hash(birth.name1,birth.name2));
        
        if (StrCompare(gAppErrStr, p) ==0) return true;
    }
    return false;
}



