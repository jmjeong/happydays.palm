/*
HappyDays - A Birthday displayer for the PalmPilot
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

#include <PalmOS.h>
#include "address.h"
#include "datebook.h"
#include "memodb.h"
#include "todo.h"

#include "birthdate.h"
#include "happydays.h"
#include "happydaysRsc.h"
#include "calendar.h"
#include "s2lconvert.h"
#include "util.h"

#define frmRescanUpdateCode (frmRedrawUpdateCode + 1)

// global variable
//
MemHandle gTableRowHandle;

Int16 gBirthDateField;
Char gAppErrStr[AppErrStrLen];
Char gDateBk3Icon[52][8];      // dateBk3 icon string
Boolean gDateBk3IconLoaded = false;     // datebk3 icon loaded
Boolean gPrefsRdirty, gPrefsWeredirty;
UInt32 gAdcdate, gAdmdate;      // AddressBook create/modify time
UInt32 gMmcdate, gMmmdate;      // Memo create/modify time
Boolean gSortByCompany=true;    // sort by company is set in AddressBook?
UInt16 gAddrCategory;           // address book category
UInt16 gToDoCategory;           // todo category
Int16 gMainTableStart = 0;      // Main Form table start point
Int16 gMainTableRows;         // Main Form table num of rows(default 11) constant
Int16 gMainTableTotals;       // Main Form table total rows;
Int16 gMainTableHandleRow;    // The row of birthdate view to display
DateFormatType gPrefdfmts;  // global date format for Birthday field
DateFormatType gSystemdfmts;  // global system date format 
TimeFormatType gPreftfmts;  // global time format
DateType gStartDate;		// staring date of birthday listing

Boolean gProgramExit = false;    // Program exit control(set by Startform)

// for scroll bar characters
static char *pageupchr="\001", *pagedownchr="\002";
static char *pageupgreychr="\003", *pagedowngreychr="\004";

// Prefs stuff 
MemHandle PrefsRecHandle, PrefsRHandle;
UInt16 PrefsRecIndex;

struct sPrefsR *gPrefsR;
struct sPrefsR DefaultPrefsR = {
    PREFSVERSION,
    0, 0, 0,              // all/selected, keep/modified, private
    {   1, 0, 9, 3, 1, {-1, -1}, "145" },
#ifdef GERMAN
    {  1, "Alle" },
    {  "Geburtstag", "*HD:", 1, 1, 0, 0, 0, dfDMYWithDots },
#else
    {  1, "All" },
    {  "Birthday", "*HD:", 1, 1, 0, 0, 0, dfMDYWithSlashes },
#endif
    0,
#ifdef GERMAN
    "Alle", 
#else
    "All", 
#endif
    "\0\0\0", "\0\0\0",         /* addr create/modify date */
    "\0\0\0", "\0\0\0"          /* memo create/modify date */
};

// function declaration 
static void WritePrefsRec(void);
static Boolean SelectCategoryPopup(DmOpenRef dbP, UInt16* selected,
                                   UInt32 list, UInt32 trigger, Char *string);
static void MainFormLoadTable(FormPtr frm, Boolean redraw);
static void MainFormLoadHeader(FormPtr frm, Boolean redraw);

/* private database */
DmOpenRef MainDB;
DmOpenRef PrefsDB;


//  accessing the built-in DatebookDB
//
DmOpenRef DatebookDB;

//  accessing the built-in ToDoDB
//
DmOpenRef ToDoDB;

// These are for accessing the built-in AddressDB
//
DmOpenRef AddressDB;

// These are for accessing the built-in MemoDB
//
DmOpenRef MemoDB;

static void DisplayCategory(UInt32 trigger, Char* string, Boolean redraw)
{
    ControlPtr ctl;
    FontID currFont;
    UInt16 categoryLen;
    FormPtr frm = FrmGetActiveForm();

    currFont = FntSetFont(stdFont);

    categoryLen = StrLen(string);
    
    ctl = GetObjectPointer(frm, trigger);
    if ( redraw ) CtlEraseControl(ctl);

    CtlSetLabel(ctl, string);

    if (redraw) CtlDrawControl(ctl);
    
    FntSetFont (currFont);
}

static void RereadBirthdateDB(DateType start)
{
    gMainTableTotals = AddrGetBirthdate(MainDB, gAddrCategory, start);
}

static void HighlightAction(int selected, Boolean sound)
{
    FormPtr frm = FrmGetActiveForm();
    TablePtr tableP = GetObjectPointer(frm, MainFormTable);

	if (sound) SndPlaySystemSound(sndClick);

    if (gMainTableStart > selected
        || selected >= (gMainTableStart + gMainTableRows) ) {
        // if not exist in table view, redraw table
        gMainTableStart = MAX(0, selected-4);
        MainFormLoadTable(frm, true);
    }
    // highlight the selection
    TblUnhighlightSelection(tableP);
    TblSelectItem(tableP, (selected-gMainTableStart), 0);
}

static void HighlightMatchRowDate(DateTimeType inputDate)
{
    LineItemPtr ptr;
    DateType dt;
    Int16 selected = -1;
    UInt32 maxDiff = 365*2;  // date is displayed within 1 year(upper bound)
    Int16 i;

    if (gMainTableTotals <= 0) return;

    dt.year = inputDate.year - 1904;
    dt.month = inputDate.month;
    dt.day = inputDate.day;

    if ((ptr = MemHandleLock(gTableRowHandle))) {
        for (i = 0; i < gMainTableTotals; i++) {
            UInt32 diff = DateToDays(ptr[i].date) - DateToDays(dt);
            if (diff >= 0 && diff < maxDiff) {
                maxDiff = diff;
                selected = i;
            }
        }
        MemPtrUnlock(ptr);
    }
    if (selected >= 0) {        // matched one exists
        HighlightAction(selected, true);
    }
}

static void HighlightMatchRowName(Char first)
{
    LineItemPtr ptr;
    Int16 selected = -1;
    Int16 i;
    MemHandle recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    Char p;

    if (gMainTableTotals <= 0) return;

    if ((ptr = MemHandleLock(gTableRowHandle))) {
        for (i = 0; i < gMainTableTotals; i++) {
            if ((recordH = DmQueryRecord(MainDB, ptr[i].birthRecordNum))) {
                rp = (PackedBirthDate*) MemHandleLock(recordH);
                UnpackBirthdate(&r,rp);

                p = (r.name1[0]) ? r.name1[0] : r.name2[0];
                // make capital letter
                //
                if (p >= 'a' && p <= 'z') {
                    p -= 'a' - 'A';
                }

                if (p == first ) {
                    selected = i;

                    // trick.. exit the loop
                    i = gMainTableTotals;
                }
                MemHandleUnlock(recordH);
            }
        }
        MemPtrUnlock(ptr);
    }
    if (selected >= 0) {        // matched one exists
        HighlightAction(selected, true);
    }
}

static void DoDateSelect()
{
    Char titleStr[32];
    DateTimeType dt;
    Boolean selected = false;

	dt.year = gStartDate.year + 1904;
	dt.month = gStartDate.month;
	dt.day = gStartDate.day;

    /* Pop up the form and get the date */
	SysCopyStringResource(titleStr, SelectDateString);
	selected = SelectDay(selectDayByDay, &(dt.month), &(dt.day),
						 &(dt.year), titleStr);

    if (selected) {
		// make the starting date to user selected date
		gStartDate.year = dt.year - 1904; 
		gStartDate.month = dt.month;
		gStartDate.day = dt.day;

		gMainTableStart = 0;
		FrmUpdateForm(MainForm, frmRedrawUpdateCode);
    }
}

static Boolean IsHappyDaysRecord(Char* notefield)
{
    if (notefield && StrStr(notefield, gPrefsR->BirthPrefs.notifywith)) {
        return true; 
    }
    return false;
}

// check if description has the information about name1 and name2
//
static Boolean IsSameRecord(Char* notefield, BirthDate birth)
{
    Char *p;
    
    if (notefield && (p = StrStr(notefield,gPrefsR->BirthPrefs.notifywith))) {
        p += StrLen(gPrefsR->BirthPrefs.notifywith);

        StrPrintF(gAppErrStr, "%ld", Hash(birth.name1,birth.name2));
        
        if (StrCompare(gAppErrStr, p) ==0) return true;
    }
    return false;
}

static int CleanupFromDB(DmOpenRef db)
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

static int CleanupFromTD(DmOpenRef db)
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

static void ShowScrollArrows(FormPtr frm, Int16 start, Int16 total)
{
    ControlPtr ptrup, ptrdown;
    Boolean enable;

    ptrup = GetObjectPointer(frm, MainFormPageUp);
    ptrdown = GetObjectPointer(frm, MainFormPageDown);

    if ((total > gMainTableRows) || start) {
        if (start) {
            CtlSetLabel(ptrup, pageupchr);
            enable = true;
        }
        else {
            CtlSetLabel(ptrup, pageupgreychr);
            enable = false;
        }
        CtlShowControl(ptrup);
        CtlSetEnabled(ptrup, enable);
        if ((start+gMainTableRows) < total) {
            CtlSetLabel(ptrdown, pagedownchr);
            enable = true;
        }
        else {
            CtlSetLabel(ptrdown, pagedowngreychr);
            enable = false;
        }
        CtlShowControl(ptrdown);
        CtlSetEnabled(ptrdown, enable);
    }
    else {
        CtlHideControl(ptrup);
        CtlHideControl(ptrdown);
    }
}

// temporary return value;
static Char* EventTypeString(BirthDate r)
{
    if (!r.custom[0]) {
        gAppErrStr[0] = gPrefsR->BirthPrefs.custom[0];
    }
    else gAppErrStr[0] = r.custom[0];
    gAppErrStr[1] = 0;

    StrToLower(&gAppErrStr[3], gAppErrStr);     // use temporary
    gAppErrStr[3] = gAppErrStr[3] - 'a' + 'A';  // make upper

    return &gAppErrStr[3];
}

static void PerformExport(Char * memo, int mainDBIndex, DateType when)
{
    MemHandle recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    
    if ((recordH = DmQueryRecord(MainDB, mainDBIndex))) {
        rp = (PackedBirthDate *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackBirthdate(&r, rp);

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
        StrNCat(memo, EventTypeString(r), 4096);
        
        MemHandleUnlock(recordH);
    }
}

static Boolean MenuHandler(FormPtr frm, EventPtr e)
{
    Boolean handled = false;
	static Char tmpString[25];

    MenuEraseStatus(NULL);

    if (FrmGetActiveFormID() == MainForm) {
        // unhighlight table row selection
        //      and delete the main form field
        TablePtr tableP = GetObjectPointer(frm, MainFormTable);

        TblUnhighlightSelection(tableP);
    }
    
    switch(e->data.menu.itemID) {
    case MainFormMenuAbout: 
        frm = FrmInitForm(AboutForm);

        switch (FrmDoDialog(frm)) {
        case AboutFormOk:
            break;
        case AboutFormHelp:
            FrmHelp(AboutHelpString);
            break;
        }

        FrmDeleteForm(frm);
            
        handled = true;
        break;
    case MainFormMenuPref:
        FrmPopupForm(PrefForm);
            
        handled = true;
        break;

    case MainFormMenuNotifyDatebook:

        // Select Records to all and select is invalid
        //
        gPrefsR->records = 3;
        
        FrmPopupForm(DateBookNotifyForm);

        handled = true;
        break;

    case MainFormMenuCleanupDatebook: 
    {
        SysCopyStringResource(tmpString, DateBookString);

        SysCopyStringResource(gAppErrStr, RemoveConfirmString);
        if (FrmCustomAlert(CleanupAlert, tmpString, gAppErrStr, " ") == 0) {
            int ret;
            
            // do clean up processing
            ret = CleanupFromDB(DatebookDB);
            StrPrintF(gAppErrStr, "%d", ret);

            FrmCustomAlert(CleanupDone, tmpString, gAppErrStr, " ");
        }
        handled = true;
        break;
    }
    case MainFormMenuNotifyTodo:
        // Select Records to all and select is invalid
        //
        gPrefsR->records = 3;
        
        FrmPopupForm(ToDoNotifyForm);

        handled = true;
        break;
        
    case MainFormMenuCleanupTodo: 
    {
        SysCopyStringResource(tmpString, ToDoString);

        SysCopyStringResource(gAppErrStr, RemoveConfirmString);
        if (FrmCustomAlert(CleanupAlert, tmpString, gAppErrStr, " ") == 0) {
            int ret;
            
            // do clean up processing
            ret = CleanupFromTD(ToDoDB);
            StrPrintF(gAppErrStr, "%d", ret);

            FrmCustomAlert(CleanupDone, tmpString, gAppErrStr, " ");
        }
        
        handled = true;
        break;
    }
    
    case MainFormMenuExport:
    {
        LineItemPtr ptr;
        Int16 i;
        UInt16 index;
        MemoItemType memo;

        if (gMainTableTotals >0
            && (FrmCustomAlert(ExportAlert, " ", " ", " ") == 0)) {
            memo.note = MemPtrNew(4096);
            SysCopyStringResource(gAppErrStr, NotEnoughMemoryString);
            ErrFatalDisplayIf(!memo.note, gAppErrStr);
            ptr = MemHandleLock(gTableRowHandle);

            SysCopyStringResource(gAppErrStr, ExportHeaderString);
            StrCopy(memo.note, gAppErrStr);

            for (i=0; i < gMainTableTotals; i++) {
                if (ptr[i].date.year != INVALID_CONV_DATE) {
                    StrNCat(memo.note, "\n", 4096);
                    PerformExport(memo.note, ptr[i].birthRecordNum,
                                  ptr[i].date);
                }
            }
            MemPtrUnlock(ptr);
            
            MemoNewRecord(MemoDB, &memo, &index);
            if (memo.note) MemPtrFree(memo.note);

            SysCopyStringResource(gAppErrStr, ExportDoneString);
            FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " ");
        }
        
        handled = true;
        break;
    }
    
    case MainFormMenuSl2Ln:
        FrmPopupForm(Sl2LnForm);
                
        handled = true;
        break;
    case MainFormMenuLn2Sl:
        FrmPopupForm(Ln2SlForm);   

        handled = true;
        break;
    }

    return handled;
}

static void MainFormReadDB()
{
	// category information of MainDB is same as AddressDB
	//
	if ((gAddrCategory = CategoryFind(AddressDB, gPrefsR->addrCategory)) ==
		dmAllCategories) {
		MemSet(gPrefsR->addrCategory, sizeof(gPrefsR->addrCategory), 0);
		CategoryGetName(AddressDB, gAddrCategory, gPrefsR->addrCategory);
	}
	RereadBirthdateDB(gStartDate);
}

static Boolean MainFormHandleEvent (EventPtr e)
{
    Boolean handled = false;

    FormPtr frm = FrmGetFormPtr(MainForm);
    TablePtr tableP = GetObjectPointer(frm, MainFormTable);

    switch (e->eType) {
    case frmUpdateEvent:
        if (e->data.frmUpdate.updateCode == frmRescanUpdateCode)
        {
            MemSet(&gPrefsR->adrmdate, 4, 0);  // force a re-scan
            FrmGotoForm(StartForm);
            handled = true;
			break;
        }
		MainFormReadDB();
        // frmRedrawUpdateCode invoked by SpecialKeyDown
        //
		// else execute frmOpenEvent

    case frmOpenEvent:
		// Main Form table row settting
		//
		gMainTableRows = TblGetNumberOfRows(tableP); 

        DisplayCategory(MainFormPopupTrigger, gPrefsR->addrCategory, false);

		if (e->eType == frmOpenEvent) {
			MainFormLoadHeader(frm, false);
			MainFormLoadTable(frm, false);
			FrmDrawForm(frm);
			ShowScrollArrows(frm, gMainTableStart, gMainTableTotals);
		}
		else {
			// MainFormLoadHeader(frm, false);
			MainFormLoadTable(frm, true);
		}
		// display starting date
        DateToAsciiLong(gStartDate.month, gStartDate.day, 
						gStartDate.year+1904, gPrefdfmts, gAppErrStr);
		CtlSetLabel(GetObjectPointer(frm, MainFormStart), gAppErrStr);

        handled = true;
        break;

    case menuEvent: 
        handled = MenuHandler(frm, e);
        break;
            
    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case MainFormStart:
			// display starting date
			DateToAsciiLong(gStartDate.month, gStartDate.day, 
							gStartDate.year+1904, gPrefdfmts, gAppErrStr);
			CtlSetLabel(GetObjectPointer(frm, MainFormStart), gAppErrStr);

            // popup select day window
            DoDateSelect();
            handled = true;
            break;

        case MainFormPopupTrigger: 
        {
            if (SelectCategoryPopup(AddressDB, &gAddrCategory,
                                    MainFormAddrCategories, MainFormPopupTrigger,
                                    gPrefsR->addrCategory) ) {
                // changed
                gMainTableStart = 0;
                RereadBirthdateDB(gStartDate);
                MainFormLoadTable(frm, true);
            }
        

            handled = true;
            
            break;
        }
        case MainFormPageUp:
            if (gMainTableStart) {
                gMainTableStart -= MIN(gMainTableStart, gMainTableRows-1);
                MainFormLoadTable(frm, true);
            }
            
            break;
        case MainFormPageDown:
            if ((gMainTableStart + gMainTableRows) < gMainTableTotals) {
                gMainTableStart += gMainTableRows-1;
                MainFormLoadTable(frm, true);
            }
            break;

        default:
            break;
                
        }
        break;

    case keyDownEvent: 
    {
        ControlPtr ptr;

        if (e->data.keyDown.chr == pageUpChr) {
            ptr = GetObjectPointer(frm, MainFormPageUp);
            CtlHitControl(ptr);
        }
        else if (e->data.keyDown.chr == pageDownChr) {
            ptr = GetObjectPointer(frm, MainFormPageDown);
            CtlHitControl(ptr);
        }
        break;
    }

    case tblSelectEvent: 
    {
        switch(e->data.tblSelect.tableID) {
			case MainFormTable:
				gMainTableHandleRow = gMainTableStart + e->data.tblEnter.row;

				FrmPopupForm(BirthdateForm);
				handled = true;
				break;
			case MainFormHeader: {
				FormPtr frm = FrmGetActiveForm();
				TablePtr tableP = GetObjectPointer(frm, MainFormHeader);
				Boolean change = true;

				switch (e->data.tblEnter.column) {
					case 0:			// Name
						if (gPrefsR->BirthPrefs.sort == 0) change = false;
						else gPrefsR->BirthPrefs.sort = 0;	// sort by name

						break;
					case 1:			// Date
						if (gPrefsR->BirthPrefs.sort == 1) change = false;
						else gPrefsR->BirthPrefs.sort = 1; 	// sort by date
						break;
					case 2:			// Age
						if (gPrefsR->BirthPrefs.sort == 2) 
							gPrefsR->BirthPrefs.sort = 3;  	// sort by age(re)
						else gPrefsR->BirthPrefs.sort = 2; 	// sort by age
						break;
				}
				if (change) {
					gMainTableStart = 0;
					SndPlaySystemSound(sndClick);

					FrmUpdateForm(MainForm, frmRedrawUpdateCode);
					WritePrefsRec();
					handled = true;
				}
				TblUnhighlightSelection(tableP);
				break;
			}
		}
		break;
    }
	case frmGotoEvent: {
		// recordNum has the selected position
		//
		HighlightAction(e->data.frmGoto.recordNum, false);

		handled = true;
		break;
	}
    
    default:
        break;
    }

    return handled;
}


static void LoadPrefsFields()
{
    FormPtr frm;
    ListPtr lstdate, lstnotify;

    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return;
    
    CtlSetValue(GetObjectPointer(frm, PrefFormOverrideSystemDate),
                gPrefsR->BirthPrefs.sysdateover);
    lstdate = GetObjectPointer(frm, PrefFormDateFmts);
    LstSetSelection(lstdate, gPrefsR->BirthPrefs.dateformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormDateTrigger),
                LstGetSelectionText(lstdate, gPrefsR->BirthPrefs.dateformat));

    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    LstSetSelection(lstnotify, gPrefsR->BirthPrefs.notifyformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormNotifyTrigger),
                LstGetSelectionText(lstnotify, gPrefsR->BirthPrefs.notifyformat));
                    
    SetFieldTextFromStr(PrefFormCustomField, gPrefsR->BirthPrefs.custom);
    SetFieldTextFromStr(PrefFormNotifyWith, gPrefsR->BirthPrefs.notifywith);

    if (gPrefsR->BirthPrefs.emphasize == 1) {
        CtlSetValue(GetObjectPointer(frm, PrefFormEmphasize), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, PrefFormEmphasize), 0);
    }
    if (gPrefsR->BirthPrefs.scannote == 1) {
        CtlSetValue(GetObjectPointer(frm, PrefFormScanNote), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, PrefFormScanNote), 0);
    }

}

static Boolean UnloadPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    ListPtr lstdate, lstnotify;
    Boolean newoverride;
    DateFormatType newdateformat;
    Boolean needrescan = 0;
    
    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return needrescan;
    if (FldDirty(GetObjectPointer(frm, PrefFormCustomField))) {
        needrescan = 1;
        if (FldGetTextPtr(GetObjectPointer(frm, PrefFormCustomField))) {
            StrNCopy(gPrefsR->BirthPrefs.custom,
                     FldGetTextPtr(GetObjectPointer(frm,
                                                    PrefFormCustomField)), 12);
        }
    }
    if (FldDirty(GetObjectPointer(frm, PrefFormNotifyWith))) {
        if (FldGetTextPtr(GetObjectPointer(frm, PrefFormNotifyWith))) {
            StrNCopy(gPrefsR->BirthPrefs.notifywith,
                     FldGetTextPtr(GetObjectPointer(frm,
                                                    PrefFormNotifyWith)), 5);
        }
    }

    // If effective date format has changed, we need to signal for a re-scan
    // of the address book database. This occurs if
    ptr = GetObjectPointer(frm, PrefFormOverrideSystemDate);
    newoverride = CtlGetValue(ptr);
    lstdate = GetObjectPointer(frm, PrefFormDateFmts);
    newdateformat = LstGetSelection(lstdate);
    if (newoverride)
    {
        if (newdateformat != gPrefdfmts)
        {
            needrescan = 1;
            gPrefdfmts = newdateformat;
        }
    }
    else
    {
        if (gPrefsR->BirthPrefs.sysdateover && gPrefdfmts != gSystemdfmts)
        {
            needrescan = 1;
            gPrefdfmts = gSystemdfmts;    // revert to system date format
        }
    }
    gPrefsR->BirthPrefs.sysdateover = newoverride;
    gPrefsR->BirthPrefs.dateformat = newdateformat;

    // notify string
    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    gPrefsR->BirthPrefs.notifyformat = LstGetSelection(lstnotify);
        
    ptr = GetObjectPointer(frm, PrefFormEmphasize);
    gPrefsR->BirthPrefs.emphasize = CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, PrefFormScanNote);
    if (CtlGetValue(ptr) != gPrefsR->BirthPrefs.scannote ) {
        needrescan = 1;
    }
    gPrefsR->BirthPrefs.scannote = CtlGetValue(ptr);

    return needrescan;
}

static Boolean PrefFormHandleEvent(EventPtr e)
{
    Boolean rescan;
    Boolean handled = false;
    FormPtr frm;
    
    switch (e->eType) {
    case frmOpenEvent:
        frm = FrmGetFormPtr(PrefForm);

        LoadPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

        
    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case PrefFormOk:
            rescan = UnloadPrefsFields();
            WritePrefsRec();
            
            FrmReturnToForm(0);
            if (rescan)
                FrmUpdateForm(MainForm, frmRescanUpdateCode);
			else 
			    FrmUpdateForm(MainForm, frmRedrawUpdateCode);

            handled = true;
            break;
        case PrefFormCancel:

            FrmReturnToForm(0);
			// need for 'preference-cancel' work
            FrmUpdateForm(StartForm, frmRedrawUpdateCode);

            handled = true;
            break;

        default:
            break;
        }
        break;

    case menuEvent:
        MenuEraseStatus(NULL);
        break;
        
    default:
        break;
    }


    return handled;
}

Boolean SelectCategoryPopup(DmOpenRef dbP, UInt16* selected,
                            UInt32 list, UInt32 trigger, Char *string)
{
    FormPtr frm;
    ListPtr lst;
    Int16 curSelection, newSelection;
    Char * name;
    Boolean ret = false;

    frm = FrmGetActiveForm();

    lst = GetObjectPointer(frm, list);

    // create a list of categories 
    CategoryCreateList(dbP, lst, *selected, true, true, 1, 0, true);

    // display the category list
    curSelection = LstGetSelection(lst);
    newSelection = LstPopupList(lst);

    // was a new category selected? 
    if ((newSelection != curSelection) && (newSelection != -1)) {
        if ((name = LstGetSelectionText(lst, newSelection))) {
            *selected = CategoryFind(dbP, name);

            MemSet(string, dmCategoryLength, 0);
            MemMove(string, name, StrLen(name));

            gPrefsRdirty = true;
            DisplayCategory(trigger, string, true);
            ret = true;
        }
    }
    CategoryFreeList(dbP, lst, false, false);
    return ret;
}

static void MainFormDrawRecord(MemPtr tableP, Int16 row, Int16 column, 
                               RectanglePtr bounds)
{
    FontID currFont;
    MemHandle recordH = 0;
    Int16 x, y;
    UInt16 nameExtent;
    PackedBirthDate* rp;
    BirthDate r;
    LineItemPtr ptr;
    LineItemType drawRecord;
	Boolean ignored = true;
	Int16 ageFieldWidth = AGE_FIELD_WIDTH;		// age + categoryWidth 
	Int16 dateFieldWidth = DATE_FIELD_WIDTH;	// date width
    
    /*
     * Set the standard font.  Save the current font.
     * It is a Pilot convention to not destroy the current font
     * but to save and restore it.
     */
    currFont = FntSetFont(stdFont);

    x = bounds->topLeft.x;
    y = bounds->topLeft.y;

    ptr = MemHandleLock(gTableRowHandle);
    drawRecord = ptr[row+gMainTableStart];
    
    if ((recordH =
         DmQueryRecord(MainDB, drawRecord.birthRecordNum))) {
		short categoryWidth;
		Int16 width, length;
        Char* eventType;

        rp = (PackedBirthDate *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackBirthdate(&r, rp);

		// Display date(converted date)
		// 
		if (drawRecord.date.year != INVALID_CONV_DATE) {
			DateToAscii(drawRecord.date.month, drawRecord.date.day,
						drawRecord.date.year+ 1904, gPrefdfmts, gAppErrStr);
		}
		else {
			StrCopy(gAppErrStr, "-");
		}
		width = dateFieldWidth;
		length = StrLen(gAppErrStr);
		FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
		WinDrawChars(gAppErrStr, length,
					 bounds->topLeft.x + bounds->extent.x -
					 ageFieldWidth - width - 3, y);

		// Display name
		// 
		nameExtent = bounds->extent.x - dateFieldWidth - ageFieldWidth;
		DrawRecordName(r.name1, r.name2, nameExtent, &x, y,
					   false,
					   r.flag.bits.priority_name1 || !gSortByCompany);

		// Display age
		if (drawRecord.age >= 0) {
			StrPrintF(gAppErrStr, "%d", drawRecord.age);
		}
		else {
			StrCopy(gAppErrStr, "-");
		}
		categoryWidth = FntCharWidth('W');
		width = ageFieldWidth - categoryWidth - 1;
		length = StrLen(gAppErrStr);

		FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
		WinDrawChars(gAppErrStr, length,
                     bounds->topLeft.x + bounds->extent.x - categoryWidth
                     - width - 1, y);

        // EventTypeString use  'gAppErrStr'
        //
        eventType = EventTypeString(r);
        
		// category display
		//
        width = categoryWidth;
        length = 1;
        FntCharsInWidth(eventType, &width, &length, &ignored);
		WinDrawChars(eventType, 1, bounds->topLeft.x +
                     bounds->extent.x - width - (categoryWidth - width)/2, y);

        if (gPrefsR->BirthPrefs.emphasize 
            && drawRecord.date.year != INVALID_CONV_DATE
            && (r.flag.bits.lunar || r.flag.bits.lunar_leap)) {
            // draw gray line
            WinDrawGrayLine(bounds->topLeft.x,
                            bounds->topLeft.y + bounds->extent.y-1,
                            bounds->topLeft.x + bounds->extent.x -
                            ageFieldWidth,
                            bounds->topLeft.y + bounds->extent.y-1);
        }
        
        MemHandleUnlock(recordH);
    }

    MemPtrUnlock(ptr);

    /* Restore the font */
    FntSetFont (currFont);
}

// IN : index - mainDB index
//
static void SetBirthdateViewForm(Int16 index, DateType converted, Int8 age)
{
    FontID currFont;
    MemHandle recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    FormPtr frm;
    Char displayStr[255];
    UInt16 addrattr;
	Int16 dateDiff;
	DateType current;
            
    if ((frm = FrmGetFormPtr(BirthdateForm)) == 0) return;
    /*
     * Set the standard font.  Save the current font.
     * It is a Pilot convention to not destroy the current font
     * but to save and restore it.
     */
    currFont = FntSetFont(stdFont);

    if ((recordH = DmQueryRecord(MainDB, index))) {
        rp = (PackedBirthDate *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackBirthdate(&r, rp);

        if (r.name2 && r.name2[0]) {
            SetFieldTextFromStr(BirthdateFirst, r.name2);
            SetFieldTextFromStr(BirthdateSecond, r.name1);
        }
        else {
            SetFieldTextFromStr(BirthdateFirst, r.name1);
            ClearFieldText(BirthdateSecond);
        }

        if (!r.custom[0]) {         // custom does not exist
            SetFieldTextFromStr(BirthdateCustom,
                                gPrefsR->BirthPrefs.custom);
        }
        else {
            SetFieldTextFromStr(BirthdateCustom, r.custom);
        }

        if (converted.year != INVALID_CONV_DATE) {
            DateToAsciiLong(converted.month, converted.day,
                            converted.year+ 1904, gPrefdfmts,
                            displayStr);

			if (age>=0) {
                StrPrintF(gAppErrStr, "   [%d -> %d]", age-1, age);
                StrCat(displayStr, gAppErrStr);
            }
        }
        else {
            SysCopyStringResource(gAppErrStr, ViewNotExistString);
            StrNCopy(displayStr, gAppErrStr, 255);
        }
        SetFieldTextFromStr(BirthdateDate, displayStr);

        DateSecondsToDate(TimGetSeconds(), &current);
        StrCopy(displayStr, " <==  ");

        if (r.flag.bits.lunar) {
            StrCat(displayStr, "-)");
        }
        else if (r.flag.bits.lunar_leap) {
            StrCat(displayStr, "#)");
        }

        if (r.flag.bits.year) {
            DateToAsciiLong(r.date.month, r.date.day, r.date.year + 1904,
                            gPrefdfmts, gAppErrStr);
        }
        else {
            DateToAsciiLong(r.date.month, r.date.day, -1, gPrefdfmts,
                            gAppErrStr);
        }
            
        StrCat(displayStr, gAppErrStr);

        if (r.flag.bits.year) {
            DateType solBirth;
            DateTimeType rtVal;
            UInt8 ret;
            
            if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {
                ret = !lun2sol(r.date.year+1904,
                               r.date.month,
                               r.date.day,
                               r.flag.bits.lunar_leap, &rtVal);
                if (ret) {
                    solBirth.year = rtVal.year - 1904;
                    solBirth.month = rtVal.month;
                    solBirth.day = rtVal.day;
                }
            }
            else solBirth = r.date;
            
            dateDiff = (Int16)(DateToDays(current) - DateToDays(solBirth));
			if (dateDiff > (Int16)0) {
				StrPrintF(gAppErrStr, " (%d)", dateDiff);
				StrCat(displayStr,gAppErrStr);
			}
        }
        SetFieldTextFromStr(BirthdateOrigin, displayStr);

		DateSecondsToDate(TimGetSeconds(), &current);
		dateDiff = DateToDays(converted) - DateToDays(current);

		if (dateDiff >= (Int16)0) {
			SysCopyStringResource(gAppErrStr, BirthLeftString);
			StrPrintF(displayStr, gAppErrStr, dateDiff);
		}
		else {
			SysCopyStringResource(gAppErrStr, BirthPassedString);
			StrPrintF(displayStr, gAppErrStr, -1 * dateDiff);
		}
		SetFieldTextFromStr(BirthdateLeft, displayStr);

        // read category information
        //
        DmRecordInfo(MainDB, index, &addrattr, NULL, NULL);
        addrattr &= dmRecAttrCategoryMask;      // get category info
        // Category information is the same as AddressDB
        //  and MainDB doesn't have the category information
        //  so read the AddressDB information
        //
        //  INFO: gAppErrStr's length is 255(must be larger than
        //  dmCategoryLength)
        //
        //

        
        // FrmSetCategoryLabel(frm, FrmGetObjectIndex(frm, BirthdateCategory),
        //                    gAppErrStr);
        // error? (suggested by Mike McCollister)

        CategoryGetName(AddressDB, addrattr, gAppErrStr);
        SetFieldTextFromStr(BirthdateCategory, gAppErrStr);
        
        MemHandleUnlock(recordH);
    }
    
    /* Restore the font */
    FntSetFont (currFont);
}

static void MainFormHeaderDrawTable(MemPtr tableP, Int16 row, Int16 column, 
                               RectanglePtr bounds)
{
    FontID currFont;
    Int16 x, y;
	RectangleType rect;
	Int16 length, width;
	Boolean ignored = true;

    /*
     * Set the standard font.  Save the current font.
     * It is a Pilot convention to not destroy the current font
     * but to save and restore it.
     */
    currFont = FntSetFont(stdFont);

	rect.topLeft.x = x = bounds->topLeft.x;
    rect.topLeft.y = y = bounds->topLeft.y;

	rect.extent.x = bounds->extent.x;
	rect.extent.y = bounds->extent.y;

	// draw header rectangle
	WinDrawRectangle(&rect, 0);

	switch(column) {
		case 0: SysCopyStringResource(gAppErrStr, MainHdrNameStr);
				break;
		case 1: SysCopyStringResource(gAppErrStr, MainHdrDateStr);
				break;
		case 2: SysCopyStringResource(gAppErrStr, MainHdrAgeStr);
				break;
		default:

	}

	width = bounds->extent.x;
	length = StrLen(gAppErrStr);

	FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
	WinDrawInvertedChars(gAppErrStr, length, 
				bounds->topLeft.x + (bounds->extent.x - width) / 2, y);

    /* Restore the font */
    FntSetFont (currFont);
}

static void MainFormLoadHeader(FormPtr frm, Boolean redraw)
{
    TablePtr tableP;
    Int8 column, numColumns;

    tableP = GetObjectPointer(frm, MainFormHeader);
    if (redraw) {
        TblEraseTable(tableP);
    }

    numColumns = 3;
    
	for (column=0; column < numColumns; column++) {
		TblSetItemStyle(tableP, 0, column, customTableItem);
	}
	TblSetRowSelectable(tableP, 0, true);

	TblSetRowHeight(tableP, 0, 11);
	TblSetRowUsable(tableP, 0, true);

    for (column=0; column < numColumns; column++) {
        TblSetColumnUsable(tableP, column, true);
        TblSetCustomDrawProcedure(tableP, column, MainFormHeaderDrawTable);
    }

    if (redraw) {
        TblDrawTable(tableP);
    }
}

static void MainFormLoadTable(FormPtr frm, Boolean redraw)
{
    TablePtr tableP;
    int row;

    tableP = GetObjectPointer(frm, MainFormTable);

    if (redraw) {
        TblEraseTable(tableP);
    }
        
    for (row=0; row < gMainTableRows; row++) {
        if ((gMainTableStart + row) < gMainTableTotals) { 
            TblSetItemStyle(tableP, row, 0, customTableItem);
            TblSetItemInt(tableP, row, 0, row); 

            TblSetRowHeight(tableP, row, FntCharHeight());
            TblSetRowUsable(tableP, row, true);
            TblSetRowSelectable(tableP, row, true);
        }
        else {
            TblSetRowUsable(tableP, row, false);
        }
    }
    TblSetColumnUsable(tableP, 0, true);

    TblSetCustomDrawProcedure(tableP, 0, MainFormDrawRecord);

    if (redraw) {
        TblDrawTable(tableP);
		ShowScrollArrows(frm, gMainTableStart, gMainTableTotals);
    }
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
    if (gPrefsR->records == 0) {
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordAll), 1);
    }
    else if (gPrefsR->records == 1) {
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordSlct), 1);
    }
    else {          //  '3' means that All is selected and slct is unusable
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordAll), 1);
        CtlSetUsable(GetObjectPointer(frm, NotifyFormRecordSlct), false);
    }
    if (gPrefsR->existing == 0) {
        CtlSetValue(GetObjectPointer(frm, NotifyFormEntryKeep), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifyFormEntryModify), 1);
    }

    if (gPrefsR->private == 1) {
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
    gPrefsR->records = !CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, NotifyFormEntryKeep);
    gPrefsR->existing = !CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, NotifyFormPrivate);
    gPrefsR->private = CtlGetValue(ptr);

    return;
}

static void LoadDBNotifyPrefsFields(void)
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(DateBookNotifyForm)) == 0) return;

    LoadCommonPrefsFields(frm);  // all/selected, keep/modify, private
    
    if (gPrefsR->DBNotifyPrefs.alarm == 1) {
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
                        StrIToA(gAppErrStr, gPrefsR->DBNotifyPrefs.notifybefore));
    SetFieldTextFromStr(DateBookNotifyFormDuration,
                        StrIToA(gAppErrStr, gPrefsR->DBNotifyPrefs.duration));
    
    TimeToAsciiLocal(gPrefsR->DBNotifyPrefs.when, gPreftfmts, gAppErrStr);
    CtlSetLabel(GetObjectPointer(frm, DateBookNotifyFormTime), gAppErrStr);
}

static void UnloadDBNotifyPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    
    if ((frm = FrmGetFormPtr(DateBookNotifyForm)) == 0) return;

    UnloadCommonNotifyPrefs(frm);       // all/selected, keey/modify, private

    ptr = GetObjectPointer(frm, DateBookNotifyFormAlarm);
    gPrefsR->DBNotifyPrefs.alarm = CtlGetValue(ptr);

    if (FldDirty(GetObjectPointer(frm, DateBookNotifyFormBefore))) {
        gPrefsR->DBNotifyPrefs.notifybefore =
            (int) StrAToI(FldGetTextPtr(
                GetObjectPointer(frm, DateBookNotifyFormBefore)));
    }
    // notification duration
    if (FldDirty(GetObjectPointer(frm, DateBookNotifyFormDuration))) {
        gPrefsR->DBNotifyPrefs.duration =
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

    switch (gPrefsR->TDNotifyPrefs.priority) {
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
    DisplayCategory(ToDoPopupTrigger, gPrefsR->TDNotifyPrefs.todoCategory, false);

    // set gToDoCategory information
    //
    if ((gToDoCategory = CategoryFind(ToDoDB, gPrefsR->TDNotifyPrefs.todoCategory))
        == dmAllCategories) {
        MemSet(gPrefsR->TDNotifyPrefs.todoCategory,
               sizeof(gPrefsR->TDNotifyPrefs.todoCategory), 0);
        CategoryGetName(ToDoDB, gToDoCategory, gPrefsR->TDNotifyPrefs.todoCategory);
    }
}

static void UnloadTDNotifyPrefsFields()
{
    FormPtr frm;
    
    if ((frm = FrmGetFormPtr(ToDoNotifyForm)) == 0) return;
    UnloadCommonNotifyPrefs(frm);       // all/selected, keey/modify, private

    if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri1)) )
        gPrefsR->TDNotifyPrefs.priority = 1;
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri2)) )
        gPrefsR->TDNotifyPrefs.priority = 2;
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri3)) )
        gPrefsR->TDNotifyPrefs.priority = 3;
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri4)) )
        gPrefsR->TDNotifyPrefs.priority = 4;
    else
        gPrefsR->TDNotifyPrefs.priority = 5;
    
    // ToDo Category is set by handler
}


static Int16 CheckDatebookRecord(DateType when, BirthDate birth)
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

static Int16 CheckToDoRecord(DateType when, BirthDate birth)
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
    if (gPrefsR->private == 1) {
        DmRecordInfo(db, index, &attributes,
                     NULL,NULL);
        attributes |= dmRecAttrSecret;
        DmSetRecordInfo(db, index, &attributes, NULL);
    }
}

static Char* DateBk3IconString()
{
    static Char bk3Icon[11];

    StrCopy(bk3Icon, "##@@@@@@@\n");
    bk3Icon[5] = gPrefsR->DBNotifyPrefs.datebk3icon + 'A';

    return bk3Icon;
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
static Char* NotifyDescString(DateType when, BirthDate birth, 
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
    pfmtString = gNotifyFormatString[(int)gPrefsR->BirthPrefs.notifyformat];
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
                    StrCopy(pDesc, gPrefsR->BirthPrefs.custom);
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
                        || gPrefsR->DBNotifyPrefs.duration ==1 || todo) {
                        if (age >= 0) {
                            StrPrintF(gAppErrStr, " (%d)", age);
                            StrCopy(pDesc, gAppErrStr);
                        }
                    }
                    else if (birth.flag.bits.solar 
                             && gPrefsR->DBNotifyPrefs.duration != 1) {
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
    
static Int16 PerformNotifyDB(BirthDate birth, DateType when, Int8 age,
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
         && !gPrefsR->existing ) {    // exist and keep the record
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
    dbwhen.startTime = gPrefsR->DBNotifyPrefs.when;
    dbwhen.endTime = gPrefsR->DBNotifyPrefs.when;

    // if alarm checkbox is set, set alarm
    //
    if (gPrefsR->DBNotifyPrefs.alarm &&
        gPrefsR->DBNotifyPrefs.notifybefore >= 0) {
        dbalarm.advanceUnit = aauDays;
        dbalarm.advance = gPrefsR->DBNotifyPrefs.notifybefore;
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
    if (gPrefsR->DBNotifyPrefs.icon == 1) {     // AN
        StrCopy(noteField, "ICON: ");
        StrCat(noteField, gPrefsR->DBNotifyPrefs.an_icon);
        StrCat(noteField, "\n#AN\n");
    }
	else if (gPrefsR->DBNotifyPrefs.icon == 2) {
        StrCopy(noteField, DateBk3IconString());
	}
    else noteField[0] = 0;

    StrCat(noteField, gPrefsR->BirthPrefs.notifywith);
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
        if (gPrefsR->existing == 0) {
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

static Int16 PerformNotifyTD(BirthDate birth, DateType when, Int8 age,
                             Int16 *created, Int16 *touched)
{
    Char noteField[256];      // (datebk3: 10, AN:14), HD id: 5
    ToDoItemType todo;
    Char* description = 0;
    
    Int16 existIndex;

    // for the performance, check this first 
    if ( ((existIndex = CheckToDoRecord(when, birth)) >= 0)
         && !gPrefsR->existing ) {    // exists and keep the record
        (*touched)++;
        
        return 0;
    }

    /* Zero the memory */
    MemSet(&todo, sizeof(ToDoItemType), 0);

    // set the date 
    //
    todo.dueDate = when;
    todo.priority = gPrefsR->TDNotifyPrefs.priority;

    StrCopy(noteField, gPrefsR->BirthPrefs.notifywith);
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
        if (gPrefsR->existing == 0) {
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
    
    if (when->year < 1970 - 1904) when->year = 1970 - 1904;
                
    while (DaysInMonth(when->month, when->year) < when->day && maxtry-- >0) {
        when->year++;
    }
}

static void NotifyDatebook(int mainDBIndex, DateType when, Int8 age,
                           Int16 *created, Int16 *touched)
{
    MemHandle recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    RepeatInfoType repeatInfo;
    Int16 i;
    
    // if duration is equal to 0, no notify record is created
    //
    if (gPrefsR->DBNotifyPrefs.duration == 0) return;

    if ((recordH = DmQueryRecord(MainDB, mainDBIndex))) {
        rp = (PackedBirthDate *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackBirthdate(&r, rp);

        if (r.flag.bits.solar) {
            repeatInfo.repeatType = repeatYearly;
            repeatInfo.repeatFrequency = 1;
            repeatInfo.repeatOn = 0;
            repeatInfo.repeatStartOfWeek = 0;

            if (gPrefsR->DBNotifyPrefs.duration > 1) {
                // if solar date, make entry repeat
                //
                repeatInfo.repeatEndDate = when;
                repeatInfo.repeatEndDate.year +=
                    gPrefsR->DBNotifyPrefs.duration -1;

                // if end year is more than 2301, set for ever
                if (repeatInfo.repeatEndDate.year > lastYear) {
                    DateToInt(repeatInfo.repeatEndDate) = -1;
                }

                // if duration > 1, 'when' is the birthdate;
                if (r.flag.bits.year) when = r.date;

				// Because Palm Desktop doesn't support events before 1970.
				//
                AdjustDesktopCompatible(&when);
                PerformNotifyDB(r, when, age, &repeatInfo, created, touched);
            }
            else if (gPrefsR->DBNotifyPrefs.duration == -1) {
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
            int duration = gPrefsR->DBNotifyPrefs.duration;
            DateType current, converted;
            
            // for the convenience, -1 means 5 year in lunar calendar.
            //
            if (duration == -1) duration = DEFAULT_DURATION;

            // now use the same routine in lunar an lunar_leap
            //
            DateSecondsToDate(TimGetSeconds(), &current);

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
    PackedBirthDate* rp;
    BirthDate r;
    
    if ((recordH = DmQueryRecord(MainDB, mainDBIndex))) {
        rp = (PackedBirthDate *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackBirthdate(&r, rp);

        if (r.flag.bits.solar) {
            PerformNotifyTD(r, when, age, created, touched);
        }
        else if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {
            // if lunar date, make each entry
            //
            DateType current, converted;
            
            // now use the same routine in lunar an lunar_leap
            //
            DateSecondsToDate(TimGetSeconds(), &current);

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
        if (gPrefsR->records == 0) {  // all
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

        if (gPrefsR->existing == 0) {  // keep
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


static Boolean DBNotifyFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(DateBookNotifyForm);

    switch (e->eType) {
    case frmOpenEvent:

        LoadDBNotifyPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case DateBookNotifyFormOk: 
        {
            UnloadDBNotifyPrefsFields();
            WritePrefsRec();

            NotifyAction(DateBookNotifyForm, NotifyDatebook);
            FrmReturnToForm(0);

            handled = true;
            break;
        }
        
        case DateBookNotifyFormCancel:
            UnloadDBNotifyPrefsFields();
            WritePrefsRec();

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

            startTime = endTime = lastTime = gPrefsR->DBNotifyPrefs.when;
            if (TimeToInt(startTime) == noTime) {
                untimed = true;
                // set dummy time
                startTime.hours = endTime.hours  = 8;
                startTime.minutes = endTime.minutes = 0;
            }
            
            SysCopyStringResource(gAppErrStr, SelectTimeString);
            if (SelectTimeV33(&startTime, &endTime, untimed, gAppErrStr,8)) {
                gPrefsR->DBNotifyPrefs.when = startTime;
                TimeToAsciiLocal(gPrefsR->DBNotifyPrefs.when, gPreftfmts,
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

static Boolean ToDoFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(ToDoNotifyForm);

    switch (e->eType) {
    case frmOpenEvent:

        LoadTDNotifyPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case ToDoNotifyFormOk: 
        {
            UnloadTDNotifyPrefsFields();
            WritePrefsRec();

            NotifyAction(ToDoNotifyForm, NotifyToDo);
            FrmReturnToForm(0);

            handled = true;
            break;
        }
        
        case ToDoNotifyFormCancel:
            UnloadTDNotifyPrefsFields();
            WritePrefsRec();

            FrmReturnToForm(0);

            handled = true;
            break;

        case ToDoPopupTrigger: 
        {
            SelectCategoryPopup(ToDoDB, &gToDoCategory,
                                ToDoNotifyCategory, ToDoPopupTrigger,
                                gPrefsR->TDNotifyPrefs.todoCategory);
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
    
    x = bounds->topLeft.x;
    y = bounds->topLeft.y;

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

    if ((MemCmp((char*) &gMmcdate, gPrefsR->memcdate, 4) == 0) &&
        (MemCmp((char*) &gMmmdate, gPrefsR->memmdate, 4) == 0)) {
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

    if (!loadDatebk3Icon()) return false;

    tableP = GetObjectPointer(frm, DateBookNotifyBk3Table);
    if (redraw) {
        TblEraseTable(tableP);
    }

    numRows = TblGetNumberOfRows(tableP);
    numColumns = 13;
    
    for (row=0; row < numRows; row++) {
        for (column=0; column < numColumns; column++) {
            TblSetItemStyle(tableP, row, column, customTableItem);
        }
        TblSetRowSelectable(tableP, row, true);

        TblSetRowHeight(tableP, row, 10);
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

    if (gPrefsR->DBNotifyPrefs.icon == 0) {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormIcon), 0);
        HideIconStuff();
    }
    else {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormIcon), 1);
    }
    
    if (gPrefsR->DBNotifyPrefs.icon == 1) {     // AN selected
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormAN), 1);
    }
    else {                                      // Datebk3 Icon selected 
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormBk3), 1);
    }

    SetFieldTextFromStr(DateBookNotifyANInput,
                        gPrefsR->DBNotifyPrefs.an_icon);

    return DateBk3IconLoadTable(frm, false);

}

static void UnloadDBNotifyPrefsFieldsMore()
{
    FormPtr frm;
    ControlPtr ptr;
    
    if ((frm = FrmGetFormPtr(DateBookNotifyMoreForm)) == 0) return;
    
    ptr = GetObjectPointer(frm, DateBookNotifyFormIcon);
    if (!CtlGetValue(ptr)) gPrefsR->DBNotifyPrefs.icon = 0;
    else {
        ptr = GetObjectPointer(frm, DateBookNotifyFormAN);
        if (CtlGetValue(ptr)) gPrefsR->DBNotifyPrefs.icon = 1;
        else gPrefsR->DBNotifyPrefs.icon = 2;
    }
    
    if (FldDirty(GetObjectPointer(frm, DateBookNotifyANInput))) {
        if (FldGetTextPtr(GetObjectPointer(frm, DateBookNotifyANInput))) {
            StrNCopy(gPrefsR->DBNotifyPrefs.an_icon,
                     FldGetTextPtr(GetObjectPointer(frm, DateBookNotifyANInput)), 9);
        }
    }
}

static Boolean DBNotifyFormMoreHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetActiveForm();
    Boolean loadDateBk3Icon;

    switch (e->eType) {
    case frmOpenEvent: 
    {
        TablePtr tableP;
        Int16 numColumns;

        loadDateBk3Icon = LoadDBNotifyPrefsFieldsMore();
        FrmDrawForm(frm);

        if (loadDateBk3Icon) {
            tableP = GetObjectPointer(frm, DateBookNotifyBk3Table);
            numColumns = 13;
            TblSelectItem(tableP, gPrefsR->DBNotifyPrefs.datebk3icon/numColumns,
                          gPrefsR->DBNotifyPrefs.datebk3icon % numColumns);
        }

        handled = true;
        break;
    }

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case DateBookNotifyMoreFormOk: 
        {
            UnloadDBNotifyPrefsFieldsMore();
            // WritePrefsRec();

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
            gPrefsR->DBNotifyPrefs.datebk3icon =
                e->data.tblEnter.column + e->data.tblEnter.row * 13;

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

static void RecordViewScroll(Int16 chr)
{
	LineItemPtr ptr;
    
    // Before processing the scroll, be sure that the command bar has
    // been closed.
    MenuEraseStatus (0);
    
    switch (chr) {
    case vchrPageDown:
        if (gMainTableHandleRow < gMainTableTotals-1) {
            gMainTableHandleRow++;
            ptr = MemHandleLock(gTableRowHandle);
            SetBirthdateViewForm(ptr[gMainTableHandleRow].birthRecordNum,
                                 ptr[gMainTableHandleRow].date,
                                 ptr[gMainTableHandleRow].age);
            MemPtrUnlock(ptr);
        
            SndPlaySystemSound(sndInfo);
            FrmDrawForm(FrmGetFormPtr(BirthdateForm));
        }
        break;
    case vchrPageUp:
        if (gMainTableHandleRow > 0 ) {
            gMainTableHandleRow--;

            ptr = MemHandleLock(gTableRowHandle);
            SetBirthdateViewForm(ptr[gMainTableHandleRow].birthRecordNum,
                                 ptr[gMainTableHandleRow].date,
                                 ptr[gMainTableHandleRow].age);
            MemPtrUnlock(ptr);
        
            SndPlaySystemSound(sndInfo);
            FrmDrawForm(FrmGetFormPtr(BirthdateForm));
        }
        break;
    }
}

static Boolean BirthdateFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(BirthdateForm);

    switch (e->eType) {
	case keyDownEvent:
	{
		if (EvtKeydownIsVirtual(e)) {
			RecordViewScroll(e->data.keyDown.chr);
			handled = true;
		}
    }
    
    case frmOpenEvent: 
    {
        // set appropriate information
        //
        LineItemPtr ptr = MemHandleLock(gTableRowHandle);
        SetBirthdateViewForm(ptr[gMainTableHandleRow].birthRecordNum,
                             ptr[gMainTableHandleRow].date,
                             ptr[gMainTableHandleRow].age);
        MemPtrUnlock(ptr);
        
        FrmDrawForm(frm);
        handled = true;
        break;
    }
    
    case menuEvent:
        handled = MenuHandler(frm, e);
        break;
        
    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case BirthdateDone:
            FrmReturnToForm(0);
            HighlightAction(gMainTableHandleRow, false);
                
            handled = true;
            break;
        case BirthdateNotifyDB:
            //  Notify event into the datebook entry
            //
            gPrefsR->records = 1;
            FrmPopupForm(DateBookNotifyForm);

            handled = true;
            break;

        case BirthdateNotifyTD:
            //  Notify event into the datebook entry
            //
            gPrefsR->records = 1;
            FrmPopupForm(ToDoNotifyForm);

            handled = true;
            break;
            
        case BirthdateGoto:
        {
            MemHandle recordH = 0;
            PackedBirthDate* rp;
            UInt32 gotoIndex;
            
            LineItemPtr ptr = MemHandleLock(gTableRowHandle);
            
            if ((recordH =
                 DmQueryRecord(MainDB,
                               ptr[gMainTableHandleRow].birthRecordNum))) {
                rp = (PackedBirthDate *) MemHandleLock(recordH);
                gotoIndex = rp->addrRecordNum;
                MemHandleUnlock(recordH);

                if (!GotoAddress(gotoIndex)) {
                    handled = true;
                }
            }
            MemPtrUnlock(ptr);
            break;
        }
        default:
            break;
        }
        
    default:
        break;
    }

    return handled;
}

static Boolean StartFormHandlerEvent(EventPtr e)
{
    Int16 err;
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(StartForm);

    // no event procedure
    //
    switch (e->eType) {
	case frmUpdateEvent:
    case frmOpenEvent:
        err = UpdateBirthdateDB(MainDB, frm);
        if (err == ENONBIRTHDATEFIELD) {
            char ErrStr[200];

            SysCopyStringResource(ErrStr, CantFindAddrString);
            StrPrintF(gAppErrStr, ErrStr, gPrefsR->BirthPrefs.custom);

            switch (FrmCustomAlert(CustomFieldAlert,
                                   gAppErrStr, " ", " ")) {
            case 0:             // OK
                gProgramExit = true;
                break;
            case 1:             // Help
                FrmHelp(CustomErrorHelpString);
                FrmUpdateForm(StartForm, 0);    // to redisplay startform
                
                break;
            case 2:             // Preferences
                FrmPopupForm(PrefForm);
                break;
            }
        }
        else {
			MainFormReadDB();
			FrmGotoForm(MainForm);
		}
        handled = true;
        break;
    default:
        break;
    }
    
    return handled;
}

static Boolean ApplicationHandleEvent(EventPtr e)
{
    FormPtr frm;
    Boolean handled = false;
	UInt16 formID;

    if (e->eType == frmLoadEvent) {
        formID = e->data.frmLoad.formID;
        frm = FrmInitForm(formID);
        FrmSetActiveForm(frm);

        switch(formID) {
        case StartForm:
            FrmSetEventHandler(frm, StartFormHandlerEvent);
            break;
        case MainForm: 
            FrmSetEventHandler(frm, MainFormHandleEvent);
            break;
        case PrefForm:
            FrmSetEventHandler(frm, PrefFormHandleEvent);
            break;
        case DateBookNotifyForm:
            FrmSetEventHandler(frm, DBNotifyFormHandleEvent);
            break;
		case DateBookNotifyMoreForm:
			FrmSetEventHandler(frm, DBNotifyFormMoreHandleEvent);
			break;
        case ToDoNotifyForm:
            FrmSetEventHandler(frm, ToDoFormHandleEvent);
            break;
        case Sl2LnForm:
            FrmSetEventHandler(frm, Sl2LnFormHandleEvent);
            break;
        case Ln2SlForm:
            FrmSetEventHandler(frm, Ln2SlFormHandleEvent);
            break;
        case BirthdateForm:
            FrmSetEventHandler(frm, BirthdateFormHandleEvent);
            break;
        }
        handled = true;
    }

    return handled;
}

/*
 * These are called in StartApplication and StopApplication because they
 * are required memory allocations that exist from start to finish of the
 * application run.
 */
static void freememories(void)
{
    if (PrefsRHandle) {
        MemHandleFree(PrefsRHandle);
        PrefsRHandle = 0;
    }
    if (gTableRowHandle) {
        MemHandleFree(gTableRowHandle);
        gTableRowHandle = 0;
    }
}

static void ReadPrefsRec(void)
{
    MemPtr p;

    if (PrefsDB) {
        p = MemHandleLock(PrefsRecHandle);
        MemMove(gPrefsR, p, sizeof(struct sPrefsR));
        MemPtrUnlock(p);
        gPrefsRdirty = false;
        gPrefsWeredirty = false;
        if (gPrefsR->BirthPrefs.sysdateover)
            gPrefdfmts = gPrefsR->BirthPrefs.dateformat;
    }
}

static void WritePrefsRec(void)
{
    MemPtr p;
    Int8 err;
    
    if (PrefsDB) {
        p = MemHandleLock(PrefsRecHandle);
        err = DmWrite(p, 0, gPrefsR, sizeof(struct sPrefsR));
        SysCopyStringResource(gAppErrStr, CantWritePrefsString);
        ErrFatalDisplayIf(err, gAppErrStr);
        MemPtrUnlock(p);
        /*
         * This is set until we exit the app at which time the ReleaseRecord()
         * is called.
         */
        gPrefsWeredirty = true;
        gPrefsRdirty = false;
    }
}
    
static Int16 OpenDatabases(void)
{
    SystemPreferencesType sysPrefs;
    UInt16 mode = 0;
    UInt16 cardNo;
    LocalID dbID, appInfoID;
    char dbname[dmDBNameLength];
    Boolean newSearch = true;
    UInt32 creator;
    UInt16 attributes;
    MemPtr p, p2;
    Int8 err;
    DmSearchStateType searchInfo;
    UInt32 cdate, mdate;
    AddrAppInfoPtr appInfoPtr;

    /*
     * Set these to zero in case any of the opens fail.
     */
    PrefsDB = 0;
    MainDB = 0;
    DatebookDB = 0;
    AddressDB = 0;
    MemoDB = 0;
    PrefsRecHandle = 0;

    /* Determine if secret records should be shown. */
    PrefGetPreferences(&sysPrefs);
    if (!sysPrefs.hideSecretRecords) mode |= dmModeShowSecret;

    gSystemdfmts = gPrefdfmts = sysPrefs.dateFormat;// save global date format
    gPreftfmts = sysPrefs.timeFormat;               // save global time format
    
    /*
     * Quick way to open all needed databases.  Thanks to Danny Epstein.
     */
    while ((DmGetNextDatabaseByTypeCreator(newSearch, &searchInfo, 'DATA',
                                           NULL, false, &cardNo, &dbID) == 0) &&
           (!DatebookDB || !ToDoDB || !AddressDB
            || !MemoDB || !MainDB || !PrefsDB)) {
        if (DmDatabaseInfo(cardNo, dbID, dbname, NULL, NULL, &cdate, &mdate,
                           NULL, NULL, &appInfoID, NULL, NULL, &creator))
            break;
        if ((creator == DatebookAppID) &&
            (StrCaselessCompare(dbname, DatebookDBName) == 0)) {
            DatebookDB = DmOpenDatabase(cardNo, dbID, mode |
                                        dmModeReadWrite);
        }
        else if ((creator == ToDoAppID) &&
                 (StrCaselessCompare(dbname, ToDoDBName) == 0)) {
            gMmcdate = cdate;
            gMmmdate = mdate;
            ToDoDB = DmOpenDatabase(cardNo, dbID, mode | dmModeReadWrite);
        }
        else if ((creator == MemoAppID)
                 && (StrCaselessCompare(dbname, MemoDBName) == 0)) {
            MemoDB = DmOpenDatabase(cardNo, dbID, 
						mode | dmModeReadWrite | dmModeShowSecret);
        }
        else if ((creator == AddressAppID) &&
                 (StrCaselessCompare(dbname, AddressDBName) == 0)) {
            gAdcdate = cdate;
            gAdmdate = mdate;
            AddressDB = DmOpenDatabase(cardNo, dbID, mode | dmModeReadOnly);
            /*
             * We want to get the sort order for the address book.  This
             * controls how to display the birthday list.
             */
            if (appInfoID) {
                if ((appInfoPtr = (AddrAppInfoPtr)
                     MemLocalIDToLockedPtr(appInfoID, cardNo))) {
                    gSortByCompany = appInfoPtr->misc.sortByCompany;
                    MemPtrUnlock(appInfoPtr);
                }
            }
        } else if ((creator == MainAppID) &&
                   (StrCaselessCompare(dbname, MainDBName) == 0)) {
            MainDB = DmOpenDatabase(cardNo, dbID, dmModeReadWrite);
        } else if ((creator == MainAppID) &&
                   (StrCaselessCompare(dbname, PrefsDBName) == 0)) {
            PrefsDB = DmOpenDatabase(cardNo, dbID, dmModeReadWrite);
        }
        newSearch = false;
    }

    if (!MainDB){
        //
        // Create our database if it doesn't exist yet
        //
        if (DmCreateDatabase(0, MainDBName, MainAppID, MainDBType, false))
            return EDBCREATE;
        MainDB = DmOpenDatabaseByTypeCreator(MainDBType, MainAppID,
                                             dmModeReadWrite);
        if (!MainDB) return EDBCREATE;
        /* Set the backup bit.  This is to aid syncs with non Palm software. */
        DmOpenDatabaseInfo(MainDB, &dbID, NULL, NULL, &cardNo, NULL);
		/*	turn-off

            DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            attributes |= dmHdrAttrBackup;
            DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		*/
    }
    if (!PrefsDB) {
        //
        // Create our database if it doesn't exist yet
        //
        if (DmCreateDatabase(0, PrefsDBName, MainAppID, PrefsDBType, false))
            return EDBCREATE;
        PrefsDB = DmOpenDatabase(0, DmFindDatabase(0, PrefsDBName),
                                 dmModeReadWrite);
        if (!PrefsDB) return EDBCREATE;
        /* Set the backup bit.  This is to aid syncs with non Palm software. */
        DmOpenDatabaseInfo(PrefsDB, &dbID, NULL, NULL, &cardNo, NULL);
        DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
                       NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        attributes |= dmHdrAttrBackup;
        DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    
    PrefsRecIndex = 0;
    if ((PrefsRecHandle = DmQueryRecord(PrefsDB, PrefsRecIndex)) != 0) {
        // if prefs record already exist, so we will do a sanity check
        //
        if (MemHandleSize(PrefsRecHandle) >= sizeof(gPrefsR->version)) {
            p = MemHandleLock(PrefsRecHandle);
            if (*((char*)p) != (char) PREFSVERSION) {
                MemPtrUnlock(p);
                err = DmRemoveRecord(PrefsDB,PrefsRecIndex);
                ErrFatalDisplayIf(err, SysErrString(err, gAppErrStr, AppErrStrLen));
                PrefsRecHandle = 0;
            }
            else MemPtrUnlock(p);
        }
        else {
            err = DmRemoveRecord(PrefsDB, PrefsRecIndex);
            ErrFatalDisplayIf(err, SysErrString(err, gAppErrStr, AppErrStrLen));
            PrefsRecHandle = 0;
        }
    }
    if (PrefsRecHandle == 0) {
        /*
         * Create the preferences record if it doesn't exist and
         * preload it with default values.
         */
        PrefsRecHandle = DmNewRecord(PrefsDB, &PrefsRecIndex,
                                     sizeof(struct sPrefsR));
        ErrFatalDisplayIf(!PrefsRecHandle,
                          "Could not create prefs record.");
        p = MemHandleLock(PrefsRecHandle);
        p2 = (MemPtr) &DefaultPrefsR;
        err = DmWrite(p, 0, p2, sizeof(struct sPrefsR));
        ErrFatalDisplayIf(err, "Could not write default prefs.");
        MemPtrUnlock(p);
    }
    /*
     * Read in the preferences from the private database.  This also loads
     * some global variables with values for convenience.
     */
    ReadPrefsRec();

    // if hideSecret setting is changed
    if (gPrefsR->gHideSecretRecord != sysPrefs.hideSecretRecords) {
        // reset the address book timber
        MemSet(gPrefsR->adrcdate, sizeof(gPrefsR->adrcdate), 0);
        MemSet(gPrefsR->adrmdate, sizeof(gPrefsR->adrmdate), 0);
        
        gPrefsR->gHideSecretRecord = sysPrefs.hideSecretRecords;
        WritePrefsRec();
    }

    /*
     * Check if the other databases opened correctly.  Abort if not.
     */
    if (!DatebookDB || !ToDoDB || !AddressDB || !MemoDB) return EDBCREATE;

    return 0;
}

void CloseDatabases(void)
{
    if (DatebookDB) DmCloseDatabase(DatebookDB);
    if (AddressDB) DmCloseDatabase(AddressDB);
    if (ToDoDB) DmCloseDatabase(ToDoDB);
    if (MemoDB) DmCloseDatabase(MemoDB);
    if (MainDB) DmCloseDatabase(MainDB);

    if (PrefsDB) {
        if (PrefsRecHandle) {
            if (gPrefsRdirty) WritePrefsRec();
            DmReleaseRecord(PrefsDB, PrefsRecIndex, gPrefsWeredirty);
            /*
             * Unlock the PrefsR structure
             */
            MemPtrUnlock(gPrefsR);
        }
        DmCloseDatabase(PrefsDB);
    }
}

    
/* Get preferences, open (or create) app database */
static UInt16 StartApplication(void)
{
    Int8 err;
//    UInt32 romVersion;

    PrefsRHandle = 0;
    gTableRowHandle = 0;
    
    if ((PrefsRHandle = MemHandleNew(sizeof (struct sPrefsR))) == 0) {
        freememories();
        return 1;
    }
    gPrefsR = MemHandleLock(PrefsRHandle);

//    romVersion = 0;
//    err = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
//    if (romVersion >= 0x03003000) {
//        gIsOS3_0 = true;
//    }
//    else gIsOS3_0 = false;
    
    if ((err = OpenDatabases()) < 0) {
        if (!DatebookDB) {
            /*
             * BTW make sure there is " " (single space) in unused ^1 ^2 ^3
             * entries or PalmOS <= 2 will blow up.
             */

            SysCopyStringResource(gAppErrStr, DateBookFirstAlertString);
            FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " ");
        }

        freememories();
        return 1;
    }
	// set current date to  the starting date
	DateSecondsToDate(TimGetSeconds(), &gStartDate);

    
    return 0;
}

/* Save preferences, close forms, close app database */
static void StopApplication(void)
{
    FrmSaveAllForms();
    FrmCloseAllForms();
    CloseDatabases();
    freememories();
}

static Boolean SpecialKeyDown(EventPtr e) 
{
    Int16 chr;
    DateTimeType dt;
    static Int16 push_chr = '0';
    Int8 month;

    if (e->eType != keyDownEvent) return false;

	// softkey only work on main screen
	//
	if (FrmGetActiveFormID() != MainForm) return false;

    if (e->data.keyDown.modifiers & autoRepeatKeyMask) return false;
    if (e->data.keyDown.modifiers & poweredOnKeyMask) return false;
    if (e->data.keyDown.modifiers & commandKeyMask) return false;

	chr = e->data.keyDown.chr;
/*
    if (e->data.keyDown.modifiers & commandKeyMask) {
        if (keyboardAlphaChr == chr) {
            gPrefsR->BirthPrefs.sort = 0;     // sort by name
			SndPlaySystemSound(sndClick);

            gMainTableStart = 0;
            FrmUpdateForm(MainForm, frmRedrawUpdateCode);
            WritePrefsRec();
            return true;
        }
        else if (keyboardNumericChr == chr) {
            gPrefsR->BirthPrefs.sort = 1;     // sort by date
			SndPlaySystemSound(sndClick);

            gMainTableStart = 0;
            FrmUpdateForm(MainForm, frmRedrawUpdateCode);
            WritePrefsRec();
            return true;
        }
    }
    else 
*/
	if (chr >= '0' && chr <= '9') {
        // numeric character
        //
        if ( (push_chr == '1') && (chr >= '0' && chr <= '2')) {
            month = 10 + (chr - '0');
        }
        else {
            month = chr - '0';
        }
        if (month == 0) {
            push_chr = chr;
            return false;
        }

        TimSecondsToDateTime(TimGetSeconds(), &dt);

        if ( month < dt.month ) {
            dt.year++;
        }
        dt.month = month;
        dt.day = 1;

        // save the original
        push_chr = chr;
        
        HighlightMatchRowDate(dt);
        return false;
    }
    else if ((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z')) {
        // is alpha?
        //

        if (chr >= 'a') chr -=  'a' - 'A';      // make capital
        HighlightMatchRowName(chr);

		return false;
    }
    // else if ( chr == 0xA4 || (chr >= 0xA1 && chr <= 0xFE) ) {
    // is korean letter?
    //
    //}
	
    return false;
}

/* The main event loop */
static void EventLoop(void)
{
    UInt16 err;
    EventType e;

    do {
        EvtGetEvent(&e, evtWaitForever);

        if (SpecialKeyDown(&e)) continue;
        
        if (SysHandleEvent(&e)) continue;
        if (MenuHandleEvent (NULL, &e, &err)) continue;
        
        if (! ApplicationHandleEvent (&e))
            FrmDispatchEvent (&e);
    } while (e.eType != appStopEvent && !gProgramExit);
}

/***********************************************************************
 *
 * FUNCTION:    GoToItem
 *
 * DESCRIPTION: This routine is a entry point of this application.
 *              It is generally call as the result of hiting of 
 *              "Go to" button in the text search dialog.
 *
 * PARAMETERS:    recordNum - 
 ***********************************************************************/
static void GoToItem (GoToParamsPtr goToParams, Boolean launchingApp)
{
   	UInt16 recordNum;
   	EventType event;
	LineItemPtr ptr;
	Int16 selected = -1;
	Int16 i;

   	recordNum = goToParams->recordNum;

  	// If the application is already running, close all the open forms.  If
   	// the current record is blank, then it will be deleted, so we'll 
   	// the record's unique id to find the record index again, after all 
   	// the forms are closed.
   	if (! launchingApp) {
		UInt32 uniqueID;
		DmRecordInfo (MainDB, recordNum, NULL, &uniqueID, NULL);
      	FrmCloseAllForms ();
      	DmFindRecordByID (MainDB, uniqueID, &recordNum);
   	}

	// initialize category
	//
	StrCopy(gPrefsR->addrCategory, "All");

	MainFormReadDB();


	// find the inital gMainTableStart 
	// 
	if (gMainTableTotals <= 0) return;
	if ((ptr = MemHandleLock(gTableRowHandle))) {
		for (i = 0; i < gMainTableTotals; i++) {
			if (ptr[i].birthRecordNum == recordNum) {
				selected = i;
				break;
			}
		}
		MemPtrUnlock(ptr);
	}

	gMainTableStart = MAX(0, selected-4);
	FrmGotoForm(MainForm);
   	MemSet (&event, sizeof(EventType), 0);

   	// Send an event to goto a form and select the matching text.
   	event.eType = frmGotoEvent;
   	event.data.frmGoto.formID = MainForm;
	// trick selected number is returned by recordNum
   	event.data.frmGoto.recordNum = selected;
   	event.data.frmGoto.matchPos = goToParams->matchPos;
   	event.data.frmGoto.matchLen = goToParams->searchStrLen;
   	event.data.frmGoto.matchFieldNum = goToParams->matchFieldNum;
   	EvtAddEventToQueue (&event);
}

static void DrawSearchLine(BirthDate birthdate, RectangleType r)
{
	DrawRecordName(birthdate.name1, birthdate.name2, 
			r.extent.x, &r.topLeft.x, 
			r.topLeft.y, false,
		   	birthdate.flag.bits.priority_name1);
	return;

	/*
	 *  display original birthday field
	 *  (bug!!! must handle gPrefdfmts variable, 
	 *  Find function must not use the global variable)
	 *
		Int16 width, length;
		Boolean ignored = true;
		// Char displayStr[255];
		Int16 x = r.topLeft.x;

		if (birthdate.flag.bits.lunar) {
			StrCopy(displayStr, "-)");
		}
		else if (birthdate.flag.bits.lunar_leap) {
			StrCopy(displayStr, "#)");
		}
		else displayStr[0] = 0;

		if (birthdate.flag.bits.year) {
			DateToAsciiLong(birthdate.date.month, birthdate.date.day, 
							birthdate.date.year + 1904,
							gPrefdfmts, displayStr + StrLen(displayStr));
		}
		else {
			DateToAsciiLong(birthdate.date.month, birthdate.date.day, -1, 
							gPrefdfmts, displayStr + StrLen(displayStr));
		}

		width = 60;
		length = StrLen(displayStr);
		FntCharsInWidth(displayStr, &width, &length, &ignored);
		WinDrawChars(displayStr, length,
					 r.topLeft.x + r.extent.x - width, 
					 r.topLeft.y);
	*/
}

static void Search(FindParamsPtr findParams)
{
   	UInt16         	pos;
   	UInt16         	fieldNum;
   	UInt16         	cardNo = 0;
   	UInt16          recordNum;
   	Char*        	header;
   	Boolean        	done;
   	MemHandle       recordH;
   	MemHandle       headerH;
   	LocalID        	dbID;
   	DmOpenRef       dbP;
   	RectangleType   r;
   
   	// unless told otherwise, there are no more items to be found
   	findParams->more = false;  

	// refind this code(**************)
	//
   	// Open the happydays database.
   	// dbP = DmOpenDatabase(cardNo, dbID, findParams->dbAccesMode);
	// 
	// refind this code(**************)
	dbID = DmFindDatabase(cardNo, MainDBName);
   	dbP = DmOpenDatabase(cardNo, dbID, findParams->dbAccesMode);
   	if (! dbP) return;

   	// Display the heading line.
   	headerH = DmGetResource(strRsc, FindHeaderString);
   	header = MemHandleLock(headerH);
   	done = FindDrawHeader(findParams, header);
   	MemHandleUnlock(headerH);
   	if (done) {
      	findParams->more = true;
   	}
   	else {
      	// Search all the fields; start from the last record searched.
      	recordNum = findParams->recordNum;
      	for(;;) {
         	Boolean match = false;
         	BirthDate       birthdate;
         
         	// Because applications can take a long time to finish a find 
         	// users like to be able to stop the find.  Stop the find 
         	// if an event is pending. This stops if the user does 
         	// something with the device.  Because this call slows down 
         	// the search we perform it every so many records instead of 
         	// every record.  The response time should still be short 
         	// without introducing much extra work to the search.
         
         	// Note that in the implementation below, if the next 16th 
         	// record is secret the check doesn't happen.  Generally 
         	// this shouldn't be a problem since if most of the records 
         	// are secret then the search won't take long anyway!
         	if ((recordNum & 0x000f) == 0 &&       // every 16th record
            	EvtSysEventAvail(true)) {
            	// Stop the search process.
            	findParams->more = true;
            	break;
         	}
         
         	recordH = DmQueryNextInCategory(dbP, &recordNum, dmAllCategories);
         	// Have we run out of records?
         	if (! recordH) break;
         
         	// Search each of the fields of the birthdate
      
         	UnpackBirthdate(&birthdate, MemHandleLock(recordH));
         
         	if ((match = FindStrInStr((Char*) birthdate.name1, 
						findParams->strToFind, &pos)) != false)
            	fieldNum = 1;
         	else if ((match = FindStrInStr((Char*) birthdate.name2, 
						findParams->strToFind, &pos)) != false)
            	fieldNum = 2;
            
         	if (match) {
           	 	done = FindSaveMatch(findParams, recordNum, pos, fieldNum, 0, cardNo, dbID);
            	if (done) break;
   
            	// Get the bounds of the region where we will draw the results.
            	FindGetLineBounds(findParams, &r);
            
            	// Display the title of the description.
				DrawSearchLine(birthdate, r);

            	findParams->lineNumber++;
         	}
         	MemHandleUnlock(recordH);

         	if (done) break;
         	recordNum++;
      	}
   	}  
   	DmCloseDatabase(dbP);   
} 

/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    UInt16 err;

    if (cmd == sysAppLaunchCmdNormalLaunch) {
        err = StartApplication();
        if (err) return err;

		FrmGotoForm(StartForm);     // read the address db and so
        EventLoop();
        StopApplication();
	}
	// Launch code sent to running app before sysAppLaunchCmdFind
	// or other action codes that will cause data searches or manipulation.                      
	else if (cmd == sysAppLaunchCmdSaveData) {
   		FrmSaveAllForms();
   	}
   	else if (cmd == sysAppLaunchCmdFind) {
      	Search((FindParamsPtr)cmdPBP);
    } 
	// This launch code might be sent to the app when it's already running
   	else if (cmd == sysAppLaunchCmdGoTo) {
      	Boolean  launched;
      	launched = launchFlags & sysAppLaunchFlagNewGlobals;
      
      	if (launched) {
         	err = StartApplication();
         	if (!err) {
            	GoToItem((GoToParamsPtr) cmdPBP, launched);
            	EventLoop();
            	StopApplication();   
         	}
      	} else {
         	GoToItem((GoToParamsPtr) cmdPBP, launched);
      	}
   	}
	else {
        return sysErrParamErr;
    }

    return 0;
}
