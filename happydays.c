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

#include <PalmOS.h>
#include "address.h"
#include "datebook.h"
#include "memodb.h"
#include "todo.h"

#include "happydays.h"
#include "happydaysRsc.h"
#include "calendar.h"
#include "s2lconvert.h"
#include "util.h"

#define frmRescanUpdateCode (frmRedrawUpdateCode + 1)
#define frmReloadUpdateCode (frmRedrawUpdateCode + 2)

// global variable
//
MemHandle gTableRowHandle;

Int16 gBirthDateField;
Char gAppErrStr[AppErrStrLen];
Char gDateBk3Icon[52][16];      // dateBk3 icon string
Boolean gPrefsRdirty, gPrefsWeredirty;
UInt32 gAdcdate, gAdmdate;       //  AddressBook create/modify time
Boolean gSortByCompany=true;    // sort by company is set in AddressBook?
UInt16 gAddrCategory;           // address book category
UInt16 gToDoCategory;           // todo category
Int16 gMainTableStart;        // Main Form table start point
Int16 gMainTableRows;         // Main Form table num of rows(default 11) constant
Int16 gMainTableTotals;       // Main Form table total rows;
Int16 gMainTableHandleRow;    // The row of birthdate view to display
DateFormatType gPrefdfmts;  // global date format for Birthday field
DateFormatType gDispdfmts;  // global date format for display
TimeFormatType gPreftfmts;  // global time format

Boolean gProgramExit = false;    // Program exit control(set by Startform)

// for scroll bar characters
static char *pageupchr="\001", *pagedownchr="\002";
static char *pageupgreychr="\003", *pagedowngreychr="\004";

// Prefs stuff 
MemHandle PrefsRecHandle, PrefsRHandle;
UInt16 PrefsRecIndex;

struct sPrefsR *gPrefsR;
struct sPrefsR DefaultPrefsR = {
    '0', '0', '0',              // all/selected, keep/modified, private
    {   '1', '0', 9, 3, 1, {-1, -1}, "145" },
#ifdef GERMAN
    {  '1', '0', "Alle" },
    {  "Geburtstag", "*HD:", '1', '1', 0, 0, dfDMYWithDots },
#else
    {  '1', '0', "All" },
    {  "Birthday", "*HD:", '1', '1', 0, 0, dfMDYWithSlashes },
#endif
    0,
#ifdef GERMAN
    "Alle", "\0\0\0", "\0\0\0"         /* addr create/modify date */
#else
    "All", "\0\0\0", "\0\0\0"          /* addr create/modify date */
#endif
};

// function declaration 
static void WritePrefsRec(void);
static Boolean SelectCategoryPopup(DmOpenRef dbP, UInt16* selected,
                                   UInt32 list, UInt32 trigger, Char *string);
static void MainFormLoadTable(FormPtr frm, Boolean redraw);

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

static void RereadBirthdateDB()
{
    gMainTableTotals = AddrGetBirthdate(MainDB, gAddrCategory);
}

static void HighlightMatchRow(DateTimeType inputDate)
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
        FormPtr frm = FrmGetActiveForm();
        TablePtr tableP = GetObjectPointer(frm, MainFormTable);

        if (gMainTableStart > selected
            || selected >= (gMainTableStart + gMainTableRows) ) {
            // if not exist in table view, redraw table
            gMainTableStart = MAX(0, selected-5);
            MainFormLoadTable(frm, true);
        }
        // highlight the selection
        TblUnhighlightSelection(tableP);
        TblSelectItem(tableP, (selected-gMainTableStart), 0);
    }
}

static void DoDateSelect(Boolean popup)
{
    Char titleStr[32];
    Char dteStr[15];
    DateTimeType dt;
    Boolean selected = false;

    TimSecondsToDateTime(TimGetSeconds(), &dt);

    /* Pop up the form and get the date */
    if (popup) {
        SysCopyStringResource(titleStr, SelectDateString);
        selected = SelectDay(selectDayByDay, &(dt.month), &(dt.day),
                             &(dt.year), titleStr);
    }
    if (!popup || selected) {
        DateToAsciiLong(dt.month, dt.day, -1, gDispdfmts, dteStr);
        FldDrawField(SetFieldTextFromStr(MainFormField, dteStr));
    
        SndPlaySystemSound(sndClick);

        // highlight table row matching the date
        //
        HighlightMatchRow(dt);
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

        StrPrintF(gAppErrStr, "%ld-%ld",
                  birth.addrRecordNum, Hash(birth.name1,birth.name2));
        
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
                                          r.date.year + 1904, gDispdfmts,
                                          gAppErrStr), 4096);
        }
        else {
            StrNCat(memo, DateToAsciiLong(r.date.month, r.date.day,
                                          -1, gDispdfmts,
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

    MenuEraseStatus(NULL);

    if (FrmGetActiveFormID() == MainForm) {
        // unhighlight table row selection
        //      and delete the main form field
        TablePtr tableP = GetObjectPointer(frm, MainFormTable);

        TblUnhighlightSelection(tableP);
        FldDrawField(ClearFieldText(MainFormField));
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
        gPrefsR->records = '3';
        
        FrmPopupForm(DateBookNotifyForm);

        handled = true;
        break;

    case MainFormMenuCleanupDatebook: 
    {
        Char tmp[25];

        SysCopyStringResource(tmp, DateBookString);

        SysCopyStringResource(gAppErrStr, RemoveConfirmString);
        if (FrmCustomAlert(CleanupAlert, tmp, gAppErrStr, " ") == 0) {
            int ret;
            
            // do clean up processing
            ret = CleanupFromDB(DatebookDB);
            StrPrintF(gAppErrStr, "%d", ret);

            FrmCustomAlert(CleanupDone, tmp, gAppErrStr, " ");
        }
        handled = true;
        break;
    }
    case MainFormMenuNotifyTodo:
        // Select Records to all and select is invalid
        //
        gPrefsR->records = '3';
        
        FrmPopupForm(ToDoNotifyForm);

        handled = true;
        break;
        
    case MainFormMenuCleanupTodo: 
    {
        Char tmp[25];

        SysCopyStringResource(tmp, ToDoString);

        SysCopyStringResource(gAppErrStr, RemoveConfirmString);
        if (FrmCustomAlert(CleanupAlert, tmp, gAppErrStr, " ") == 0) {
            int ret;
            
            // do clean up processing
            ret = CleanupFromTD(ToDoDB);
            StrPrintF(gAppErrStr, "%d", ret);

            FrmCustomAlert(CleanupDone, tmp, gAppErrStr, " ");
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
            handled = 1;
			break;
        }
        else if (e->data.frmUpdate.updateCode == frmRedrawUpdateCode) {
            // invoked by PrefFrom --> ignored
			//
            handled = 1;
            break;
        }
        // frmReloadUpdateCode invoked by SpecialKeyDown
        //
		// else execute frmOpenEvent

    case frmOpenEvent:
        // Main Form table row settting
        //
        gMainTableRows = TblGetNumberOfRows(tableP);

        gMainTableStart = 0;
        DisplayCategory(MainFormPopupTrigger, gPrefsR->addrCategory, false);

        // category information of MainDB is same as AddressDB
        //
        if ((gAddrCategory = CategoryFind(AddressDB, gPrefsR->addrCategory)) ==
            dmAllCategories) {
            MemSet(gPrefsR->addrCategory, sizeof(gPrefsR->addrCategory), 0);
            CategoryGetName(AddressDB, gAddrCategory, gPrefsR->addrCategory);
        }

        RereadBirthdateDB();
		if (e->eType == frmOpenEvent) {
			MainFormLoadTable(frm, false);
			FrmDrawForm(frm);
			ShowScrollArrows(frm, gMainTableStart, gMainTableTotals);
		}
		else {
			MainFormLoadTable(frm, true);
		}
        handled = true;
        break;

    case menuEvent: 
        handled = MenuHandler(frm, e);
        break;
            
    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case MainFormLookup:
            // popup select day window
            DoDateSelect(true);
            handled = true;
            break;
        case MainFormToday:
            // doesn't popup select day window
            DoDateSelect(false);
            handled = true;
            break;
        case MainFormPopupTrigger: 
        {
            if (SelectCategoryPopup(AddressDB, &gAddrCategory,
                                    MainFormAddrCategories, MainFormPopupTrigger,
                                    gPrefsR->addrCategory) ) {
                // changed
                gMainTableStart = 0;
                RereadBirthdateDB();
                MainFormLoadTable(frm, true);
                FldDrawField(ClearFieldText(MainFormField));
            }
        

            handled = true;
            
            break;
        }

        case MainFormPageUp:
            if (gMainTableStart) {
                gMainTableStart -= MIN(gMainTableStart, gMainTableRows);
                MainFormLoadTable(frm, true);
                FldDrawField(ClearFieldText(MainFormField));
            }
            
            break;
        case MainFormPageDown:
            if ((gMainTableStart + gMainTableRows) < gMainTableTotals) {
                gMainTableStart += gMainTableRows;
                MainFormLoadTable(frm, true);
                FldDrawField(ClearFieldText(MainFormField));
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
        gMainTableHandleRow = gMainTableStart + e->data.tblEnter.row;

        FrmPopupForm(BirthdateForm);
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

    if (gPrefsR->BirthPrefs.emphasize == '1') {
        CtlSetValue(GetObjectPointer(frm, PrefFormEmphasize), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, PrefFormEmphasize), 0);
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
        if (gPrefsR->BirthPrefs.sysdateover)
        {
            needrescan = 1;
            gPrefdfmts = gDispdfmts;    // revert to system date format
        }
    }
    gPrefsR->BirthPrefs.sysdateover = newoverride;
    gPrefsR->BirthPrefs.dateformat = newdateformat;

    // notify string
    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    gPrefsR->BirthPrefs.notifyformat = LstGetSelection(lstnotify);
        
    ptr = GetObjectPointer(frm, PrefFormEmphasize);
    if (CtlGetValue(ptr)) gPrefsR->BirthPrefs.emphasize = '1';
    else gPrefsR->BirthPrefs.emphasize = '0';

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
            
            // clear the memory
            ClearFieldText(PrefFormCustomField);

            // may be bug gcc or emulator?
            // it redraw twice.. so I delete updateform
            // FrmUpdateForm(MainForm, frmRedrawUpdateCode);

            FrmReturnToForm(0);
            if (rescan)
                FrmUpdateForm(MainForm, frmRescanUpdateCode);
			else 
			    FrmUpdateForm(MainForm, frmRedrawUpdateCode);

            handled = true;
            break;
        case PrefFormCancel:
            
            FrmUpdateForm(StartForm, 0);
            FrmReturnToForm(0);

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

            MemSet(string, sizeof(string), 0);
            MemMove(string, name, StrLen(name));

            gPrefsRdirty = true;
            DisplayCategory(trigger, string, true);
            ret = true;
        }
    }
    CategoryFreeList(dbP, lst, false, false);
    return ret;
}

static Int16 CalculateAge(DateType converted, DateType birthdate,
                          BirthdateFlag flag)
{
    DateTimeType rtVal;
    Int16 age;
    int dummy = 0;
    int ret;
    
    if (flag.bits.lunar_leap || flag.bits.lunar) {
        // lunar birthdate
        //          change to lunar date
        ret = sol2lun(converted.year + 1904, converted.month, converted.day,
                      &rtVal, &dummy);
        if (ret) return -1;     // error

        converted.year = rtVal.year - 1904;
        converted.month = rtVal.month;
        converted.day = rtVal.day;
    }
    
    return converted.year - birthdate.year;
    return age;
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
		Int16 age;
        Char* eventType;

        rp = (PackedBirthDate *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackBirthdate(&r, rp);

		// category display
		//
		categoryWidth = FntCharWidth('W');
		width = bounds->extent.x - MAINTABLEAGEFIELD;

		if (drawRecord.date.year != INVALID_CONV_DATE) {
			DateToAscii(drawRecord.date.month, drawRecord.date.day,
						drawRecord.date.year+ 1904, gDispdfmts, gAppErrStr);
		}
		else {
			StrCopy(gAppErrStr, "-");
		}

		length = StrLen(gAppErrStr);
		FntCharsInWidth(gAppErrStr, &width,
						&length, &ignored);

		nameExtent = bounds->extent.x - width - MAINTABLEAGEFIELD - 3; 

		DrawRecordName(r.name1, r.name2, nameExtent, &x, y,
					   false,
					   r.flag.bits.priority_name1 || !gSortByCompany);

		WinDrawChars(gAppErrStr, length,
					 bounds->topLeft.x + bounds->extent.x -
					 MAINTABLEAGEFIELD - width, y);

        if (gPrefsR->BirthPrefs.emphasize == '1'
            && drawRecord.date.year != INVALID_CONV_DATE
            && (r.flag.bits.lunar || r.flag.bits.lunar_leap)) {
            // draw gray line
            WinDrawGrayLine(bounds->topLeft.x,
                            bounds->topLeft.y + bounds->extent.y-1,
                            bounds->topLeft.x + bounds->extent.x -
                            MAINTABLEAGEFIELD,
                            bounds->topLeft.y + bounds->extent.y-1);
        }
        
		if (drawRecord.date.year != INVALID_CONV_DATE 
			&& r.flag.bits.year 
			&& (age = CalculateAge(drawRecord.date, r.date, r.flag)) >= 0) {
			// calculate age if year exists
			//
			StrPrintF(gAppErrStr, "%d", age);

		}
		else {
			StrCopy(gAppErrStr, "-");
		}
		width = MAINTABLEAGEFIELD - categoryWidth - 1;
		length = StrLen(gAppErrStr);

		FntCharsInWidth(gAppErrStr, &width, &length, &ignored);

		WinDrawChars(gAppErrStr, length,
                     bounds->topLeft.x + bounds->extent.x - categoryWidth
                     - width - 1, y);

        // EventTypeString use  'gAppErrStr'
        //
        eventType = EventTypeString(r);
        
        width = categoryWidth;
        length = 1;
        FntCharsInWidth(eventType, &width, &length, &ignored);
		WinDrawChars(eventType, 1, bounds->topLeft.x +
                     bounds->extent.x - width - (categoryWidth - width)/2, y);

        MemHandleUnlock(recordH);
    }

    MemPtrUnlock(ptr);

    /* Restore the font */
    FntSetFont (currFont);
}

// IN : index - mainDB index
//
static void SetBirthdateViewForm(Int16 index, DateType converted)
{
    FontID currFont;
    MemHandle recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    FormPtr frm;
    Char displayStr[255];
    UInt16 addrattr;
    Int16 age = 0;

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
                            converted.year+ 1904, gDispdfmts,
                            displayStr);

            if (r.flag.bits.year            // year exist 
                && (age = CalculateAge(converted, r.date, r.flag)) >= 0) {
                // calculate age if year exists
                //
                StrPrintF(gAppErrStr, "   [%d -> %d]", age-1, age);
                StrCat(displayStr, gAppErrStr);
            }
        }
        else {
            SysCopyStringResource(gAppErrStr, ViewNotExistString);
            StrNCopy(displayStr, gAppErrStr, 255);
        }
        SetFieldTextFromStr(BirthdateDate, displayStr);

        StrCopy(displayStr, " <==  ");

        if (r.flag.bits.lunar) {
            StrCat(displayStr, "-)");
        }
        else if (r.flag.bits.lunar_leap) {
            StrCat(displayStr, "#)");
        }

        if (r.flag.bits.year) {
            DateToAsciiLong(r.date.month, r.date.day, r.date.year + 1904,
                            gDispdfmts, gAppErrStr);
        }
        else {
            DateToAsciiLong(r.date.month, r.date.day, -1, gDispdfmts,
                            gAppErrStr);
        }
            
        StrCat(displayStr, gAppErrStr);
        SetFieldTextFromStr(BirthdateOrigin, displayStr);

        // read category information
        //
        DmRecordInfo(MainDB, index, &addrattr, NULL, NULL);
        addrattr &= dmRecAttrCategoryMask;      // get category info
        // Category information is the same as AddressDB
        //  and MainDB doesn't have the category information
        //  so read the AddressDB information
        //
        //  INFO: gAppErrStr's length is 40(must be larger than
        //  dmCategoryLength)
        //
        //
        CategoryGetName(AddressDB, addrattr, gAppErrStr);
        FrmSetCategoryLabel(frm, FrmGetObjectIndex(frm, BirthdateCategory),
                            gAppErrStr);
        
        MemHandleUnlock(recordH);
    }
    
    /* Restore the font */
    FntSetFont (currFont);
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
    if (gPrefsR->records == '0') {
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordAll), 1);
    }
    else if (gPrefsR->records == '1') {
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordSlct), 1);
    }
    else {          //  '3' means that All is selected and slct is unusable
        CtlSetValue(GetObjectPointer(frm, NotifyFormRecordAll), 1);
        CtlSetUsable(GetObjectPointer(frm, NotifyFormRecordSlct), false);
    }
    if (gPrefsR->existing == '0') {
        CtlSetValue(GetObjectPointer(frm, NotifyFormEntryKeep), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifyFormEntryModify), 1);
    }

    if (gPrefsR->private == '1') {
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
    if (CtlGetValue(ptr)) gPrefsR->records = '0';
    else gPrefsR->records = '1';

    ptr = GetObjectPointer(frm, NotifyFormEntryKeep);
    if (CtlGetValue(ptr)) gPrefsR->existing = '0';
    else gPrefsR->existing = '1';

    ptr = GetObjectPointer(frm, NotifyFormPrivate);
    if ( CtlGetValue(ptr) ) gPrefsR->private = '1';
    else gPrefsR->private = '0';

    return;
}

static void LoadDBNotifyPrefsFields(void)
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(DateBookNotifyForm)) == 0) return;

    LoadCommonPrefsFields(frm);  // all/selected, keep/modify, private
    
    if (gPrefsR->DBNotifyPrefs.alarm == '1') {
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
    if ( CtlGetValue(ptr) ) gPrefsR->DBNotifyPrefs.alarm = '1';
    else gPrefsR->DBNotifyPrefs.alarm = '0';

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
    case '1':
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri1), 1);
        break;
    case '2':
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri2), 1);
        break;
    case '3':
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri3), 1);
        break;
    case '4':
        CtlSetValue(GetObjectPointer(frm, ToDoNotifyPri4), 1);
        break;
    case '5':
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
        gPrefsR->TDNotifyPrefs.priority = '1';
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri2)) )
        gPrefsR->TDNotifyPrefs.priority = '2';
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri3)) )
        gPrefsR->TDNotifyPrefs.priority = '3';
    else if ( CtlGetValue(GetObjectPointer(frm, ToDoNotifyPri4)) )
        gPrefsR->TDNotifyPrefs.priority = '4';
    else
        gPrefsR->TDNotifyPrefs.priority = '5';
    
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
        
        if ((DateToInt(when) = DateToInt(toDoRec->dueDate))
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
    if (gPrefsR->private == '1') {
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
  "* +F +y",        // * JaeMok Jeong 1970
};

//
// Memory is alloced, after calling this routine, user must free the memory
//
static Char* NotifyDescString(DateType when, BirthDate birth)
{
    Char* description, *pDesc;
    Char* pfmtString;
    Int16 age = 0;
    
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
                        DateToAsciiLong(birth.date.month, birth.date.day,
                                        birth.date.year + 1904,
                                        gDispdfmts, gAppErrStr);
                    }
                    else {
                        DateToAsciiLong(birth.date.month, birth.date.day, -1, gDispdfmts,
                                        gAppErrStr);
                    }
    
                    StrCat(pDesc, gAppErrStr);
                    StrCat(pDesc, " ");
                    pDesc += StrLen(pDesc);
                }
                if (birth.flag.bits.year) {
                    if (!birth.flag.bits.solar || gPrefsR->DBNotifyPrefs.duration ==1) {
                        age = CalculateAge(when, birth.date, birth.flag);
                        if (age >= 0) {
                            StrPrintF(gAppErrStr, " (%d)", age);
                            StrCopy(pDesc, gAppErrStr);
                        }
                    }
                    else if (birth.flag.bits.solar && gPrefsR->DBNotifyPrefs.duration != 1) {
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
    
static Int16 PerformNotifyDB(BirthDate birth, DateType when,
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
         && gPrefsR->existing == '0') {    // exist and keep the record
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
    if (gPrefsR->DBNotifyPrefs.alarm == '1' &&
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
    if (gPrefsR->DBNotifyPrefs.icon == '1') {     // AN
        StrCopy(noteField, "ICON: ");
        StrCat(noteField, gPrefsR->DBNotifyPrefs.an_icon);
        StrCat(noteField, "\n#AN\n");
    }
	else if (gPrefsR->DBNotifyPrefs.icon == '2') {
        StrCopy(noteField, DateBk3IconString());
	}
    else noteField[0] = 0;

    StrCat(noteField, gPrefsR->BirthPrefs.notifywith);
    StrPrintF(gAppErrStr, "%ld-%ld", birth.addrRecordNum,
              Hash(birth.name1, birth.name2));
    StrCat(noteField, gAppErrStr);
    datebook.note = noteField;

    // make the description
        
    description = NotifyDescString(when, birth);
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
        if (gPrefsR->existing == '0') {
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

static Int16 PerformNotifyTD(BirthDate birth, DateType when,
                             Int16 *created, Int16 *touched)
{
    Char noteField[256];      // (datebk3: 10, AN:14), HD id: 5
    ToDoItemType todo;
    Char* description = 0;
    
    Int16 existIndex;

    // for the performance, check this first 
    if ( ((existIndex = CheckToDoRecord(when, birth)) >= 0)
         && gPrefsR->existing == '0') {    // exist and keep the record
        (*touched)++;
        
        return 0;
    }

    /* Zero the memory */
    MemSet(&todo, sizeof(ToDoItemType), 0);

    // set the date 
    //
    todo.dueDate = when;
    todo.priority = gPrefsR->TDNotifyPrefs.priority - '0';

    StrCopy(noteField, gPrefsR->BirthPrefs.notifywith);
    StrPrintF(gAppErrStr, "%ld-%ld", birth.addrRecordNum,
              Hash(birth.name1, birth.name2));
    StrCat(noteField, gAppErrStr);
    todo.note = noteField;

    // make the description
        
    description = NotifyDescString(when, birth);
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
        if (gPrefsR->existing == '0') {
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

static void NotifyDatebook(int mainDBIndex, DateType when,
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
				if (when.year < 1970 - 1904) when.year = 1970 - 1904;
                
                PerformNotifyDB(r, when, &repeatInfo, created, touched);
            }
            else if (gPrefsR->DBNotifyPrefs.duration == -1) {
                // if duration > 1, 'when' is the birthdate;
                if (r.flag.bits.year) when = r.date;

				// Because Palm Desktop doesn't support events before 1970.
				//   
				if (when.year < 1970 - 1904) when.year = 1970 - 1904;

                DateToInt(repeatInfo.repeatEndDate) = -1;
                PerformNotifyDB(r, when, &repeatInfo, created, touched);
            }
            else PerformNotifyDB(r, when, NULL, created, touched);
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

                PerformNotifyDB(r, converted, NULL, created, touched);

                // process next target day
                //
                current = converted;
                DateAdjust(&current, 1);
            }
        }
        
        MemHandleUnlock(recordH);
    }
}

static void NotifyToDo(int mainDBIndex, DateType when,
                       Int16 *created, Int16 *touched)
{
    MemHandle recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    
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
            PerformNotifyTD(r, when, created, touched);
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
                PerformNotifyTD(r, converted, created, touched);
            }
        }
        
        MemHandleUnlock(recordH);
    }
}

static void NotifyAction(UInt32 whatAlert,
                         void (*func)(int mainDBIndex, DateType when,
                                      Int16 *created, Int16 *touched))
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
        if (gPrefsR->records == '0') {  // all
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
                                       ptr[i].date, &created,
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

        if (gPrefsR->existing == '0') {  // keep
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
            
            // clear memory
            //
            ClearFieldText(DateBookNotifyFormBefore);

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
            TimeType startTime, endTime;
            Boolean untimed = false;

            startTime = endTime = gPrefsR->DBNotifyPrefs.when;
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
                
                CtlSetLabel(GetObjectPointer(frm, DateBookNotifyFormTime),
                            gAppErrStr);
            }
            
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

static Int16 convertWord(char first, char second)
{
    return dec(first) * 16 + dec(second);
}

static void DateBk3CustomDrawTable(MemPtr tableP, Int16 row, Int16 column, 
                                   RectanglePtr bounds)
{
    Int16 x, y;
    Int8 drawItem = row * 13 + column;
    Int16 drawFixel;
    Int8 i, j;
    
    x = bounds->topLeft.x;
    y = bounds->topLeft.y;

    for (i = 0; i < 8; i++) {
        drawFixel = convertWord(gDateBk3Icon[drawItem][i*2],
                                gDateBk3Icon[drawItem][i*2+1]);
        for (j=0; j < 8; j++) {
            if (drawFixel & 1) {
                WinDrawLine(x+8-j, y+i+1, x+8-j, y+i+1);
            }
            drawFixel >>= 1;
        }
    }
}

static Boolean loadDatebk3Icon()
{
    UInt16 currIndex = 0;
    MemHandle recordH = 0;
    Char* rp;       // memoPad record
    Boolean found = false;
    Char *p=0, *q;

    while (1) {
        recordH = DmQueryNextInCategory(MemoDB, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;

        rp = (Char*)MemHandleLock(recordH);
        if (StrNCompare(rp, DATEBK3_MEMO_STRING,
                        StrLen(DATEBK3_MEMO_STRING)) == 0) {
            p = StrChr(rp, '\n') + 1;
            found = true;
            break;
        }
            
        MemHandleUnlock(recordH);
        currIndex++;
    }
    if (found) {
        Int8 i=0;
        while ((q = StrChr(p, '\n'))) {
            MemMove(gDateBk3Icon[i++], StrChr(p, '=')+1, 16);
            p = q+1;
        }
        MemMove(gDateBk3Icon[i++], StrChr(p, '=')+1, 16);

        for (; i < 52; i++) {
            MemSet(gDateBk3Icon[i], 16, '0');
        }
        MemHandleUnlock(recordH);
    }
    
    return found;
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

/*    if (gPrefsR->DBNotifyPrefs.hide_id == '1') {
      CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormHide), 1);
      }
      else {
      CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormHide), 0);
      } */

    if (gPrefsR->DBNotifyPrefs.icon == '0') {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormIcon), 0);
        HideIconStuff();
    }
    else {
        CtlSetValue(GetObjectPointer(frm, DateBookNotifyFormIcon), 1);
    }
    
    if (gPrefsR->DBNotifyPrefs.icon == '1') {     // AN selected
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
    if (!CtlGetValue(ptr)) gPrefsR->DBNotifyPrefs.icon = '0';
    else {
        ptr = GetObjectPointer(frm, DateBookNotifyFormAN);
        if (CtlGetValue(ptr)) gPrefsR->DBNotifyPrefs.icon = '1';
        else gPrefsR->DBNotifyPrefs.icon = '2';
    }
    // ptr = GetObjectPointer(frm, DateBookNotifyFormHide);

    /*
      if (CtlGetValue(ptr)) gPrefsR->DBNotifyPrefs.hide_id = '1';
      else gPrefsR->DBNotifyPrefs.hide_id = '0';
    */
    
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

static Boolean BirthdateFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(BirthdateForm);

    switch (e->eType) {
    case frmOpenEvent: 
    {
        // set appropriate information
        //
        LineItemPtr ptr = MemHandleLock(gTableRowHandle);
        SetBirthdateViewForm(ptr[gMainTableHandleRow].birthRecordNum,
                             ptr[gMainTableHandleRow].date);
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

            handled = true;
            break;
        case BirthdateNotifyDB:
            //  Notify event into the datebook entry
            //
            gPrefsR->records = '1';
            FrmPopupForm(DateBookNotifyForm);

            handled = true;
            break;

        case BirthdateNotifyTD:
            //  Notify event into the datebook entry
            //
            gPrefsR->records = '1';
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
        else FrmGotoForm(MainForm);
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
        {
            FrmSetEventHandler(frm, MainFormHandleEvent);
            break;
        }
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

    gDispdfmts = gPrefdfmts = sysPrefs.dateFormat;  // save global date format
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
            ToDoDB = DmOpenDatabase(cardNo, dbID, mode | dmModeReadWrite);
        }
        else if ((creator == MemoAppID)
                 && (StrCaselessCompare(dbname, MemoDBName) == 0)) {
            MemoDB = DmOpenDatabase(cardNo, dbID, mode | dmModeReadWrite);
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
        DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
                       NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        attributes |= dmHdrAttrBackup;
        DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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
    if ((PrefsRecHandle = DmQueryRecord(PrefsDB, PrefsRecIndex)) == 0) {
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

    FrmGotoForm(StartForm);     // read the address db and so
    
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
    // DateTimeType dt;

    if (e->eType != keyDownEvent) return false;

	// softkey only work on main screen
	//
	if (FrmGetActiveFormID() != MainForm) return false;
		
	// chr = e->data.keyDown.chr;
    if (e->data.keyDown.modifiers & commandKeyMask) {
    	chr = e->data.keyDown.chr;

        if (e->data.keyDown.modifiers & autoRepeatKeyMask) return false;
        if (e->data.keyDown.modifiers & poweredOnKeyMask) return false;

        if (keyboardAlphaChr == chr) {
            gPrefsR->BirthPrefs.sort = '0';     // sort by name
			SndPlaySystemSound(sndClick);
            FrmUpdateForm(MainForm, frmReloadUpdateCode);
            WritePrefsRec();
            return true;
        }
        else if (keyboardNumericChr == chr) {
            gPrefsR->BirthPrefs.sort = '1';     // sort by date
			SndPlaySystemSound(sndClick);
            FrmUpdateForm(MainForm, frmReloadUpdateCode);
            WritePrefsRec();
            return true;
        }
    }
	/*
      else if (chr >= '0' && chr <= '9') {
      TimSecondsToDateTime(TimGetSeconds(), &dt);
			
      dt.month = chr - '0';
      dt.day = 1;

      HighlightMatchRow(dt);

      return true;
      }
	*/
	
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

/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    UInt16 err;

    if (cmd == sysAppLaunchCmdNormalLaunch) {
        err = StartApplication();
        if (err) return err;

        EventLoop();
        StopApplication();

    } else {
        return sysErrParamErr;
    }

    return 0;
}
