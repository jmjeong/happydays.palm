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

#include <Pilot.h>
#include <System/DataPrv.h>
#include "callback.h"
#include "address.h"
#include "datebook.h"
#include "memodb.h"

#include "happydays.h"
#include "happydaysRsc.h"
#include "calendar.h"
#include "util.h"

// global variable
//
VoidHand gTableRowHandle;

Int gBirthDateField;
Char gAppErrStr[AppErrStrLen];
Boolean gPrefsRdirty, gPrefsWeredirty;
DWord gAdcdate, gAdmdate;       //  AddressBook create/modify time
Boolean gSortByCompany=true;    // sort by company is set in AddressBook?
Word gAddrCategory;
Int gMainTableStart;        // Main Form table start point
Int gMainTableRows;         // Main Form table num of rows(default 11) constant
Int gMainTableTotals;       // Main Form table total rows;
Int gMainTableHandleRow;    // The row of birthdate view to display
DateFormatType gPrefdfmts;  // global date format
TimeFormatType gPreftfmts;  // global time format

Boolean gProgramExit = false;    // Program exit control(set by Startform)

// for scroll bar characters
static char *pageupchr="\001", *pagedownchr="\002";
static char *pageupgreychr="\003", *pagedowngreychr="\004";

// Prefs stuff 
VoidHand PrefsRecHandle, PrefsRHandle;
UInt PrefsRecIndex;
struct sPrefsR *gPrefsR;
struct sPrefsR DefaultPrefsR = {
    {  '0', '0', '0', '0', '1', 3, 1, {8, 0} },
#ifdef GERMAN
    {  "Birthday", "HD:", '1', '0' },
#else
    {  "Geburtstag", "HD:", '1', '0' },
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
static void SelectCategoryPopup(DmOpenRef dbP);
static void MainFormLoadTable(FormPtr frm, Boolean redraw);

/* private database */
DmOpenRef MainDB;
DmOpenRef PrefsDB;


//  accessing the built-in DatebookDB
//
DmOpenRef DatebookDB;

// These are for accessing the built-in AddressDB
//
DmOpenRef AddressDB;

// These are for accessing the built-in MemoDB
//
DmOpenRef MemoDB;

static void MainFormDisplayCategory(Boolean redraw)
{
    ControlPtr ctl;
    FontID currFont;
    UInt categoryLen;
    FormPtr frm = FrmGetActiveForm();

    currFont = FntSetFont(stdFont);

    categoryLen = StrLen(gPrefsR->addrCategory);
    
    ctl = GetObjectPointer(frm, MainFormPopupTrigger);
    if ( redraw ) CtlEraseControl(ctl);

    CtlSetLabel(ctl, gPrefsR->addrCategory);

    if (redraw) CtlDrawControl(ctl);
    
    FntSetFont (currFont);

}

static void RereadBirthdateDB()
{
    // category information of MainDB is same as AddressDB
    //
    if ((gAddrCategory = CategoryFind(AddressDB, gPrefsR->addrCategory)) ==
        dmAllCategories) {
        MemSet(gPrefsR->addrCategory, sizeof(gPrefsR->addrCategory), 0);
        CategoryGetName(AddressDB, gAddrCategory, gPrefsR->addrCategory);
    }

    gMainTableTotals = AddrGetBirthdate(MainDB, gAddrCategory);
}

static void HighlightMatchRow(DateTimeType inputDate)
{
    LineItemPtr ptr;
    DateType dt;
    Int selected = -1;
    ULong maxDiff = 365*2;  // date is displayed within 1 year(upper bound)
    Int i;

    if (gMainTableTotals <= 0) return;

    dt.year = inputDate.year - 1904;
    dt.month = inputDate.month;
    dt.day = inputDate.day;

    if ((ptr = MemHandleLock(gTableRowHandle))) {
        for (i = 0; i < gMainTableTotals; i++) {
            ULong diff = DateToDays(ptr[i].date) - DateToDays(dt);
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
        DateToAsciiLong(dt.month, dt.day, -1, gPrefdfmts, dteStr);
        FldDrawField(SetFieldTextFromStr(MainFormField, dteStr));
    
        SndPlaySystemSound(sndClick);

        // highlight table row matching the date
        //
        HighlightMatchRow(dt);
    }
}

static Boolean IsHappyDateRecord(Char* description)
{
    CharPtr p;

    if ((p = StrChr(description, '[')) && (p = StrChr(p, ']'))) {
        p++; p++;       // skip "] "

        // if the record has the Notifywith phrases?
        //
        if (StrNCaselessCompare(p , gPrefsR->BirthPrefs.notifywith,
                                StrLen(gPrefsR->BirthPrefs.notifywith))==0)
            return true;
    }
    return false;
}

static int CleanupDatebook()
{
    UInt currIndex = 0;
    VoidHand recordH;
    ApptDBRecordType apptRecord;
    int ret = 0;
    
    while (1) {
        recordH = DmQueryNextInCategory(DatebookDB, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;
        ApptGetRecord(DatebookDB, currIndex, &apptRecord, &recordH);
        if (IsHappyDateRecord(apptRecord.description)) {
            // if it is happydays record?
            //
            ret++;
            // remove the record
            DmDeleteRecord(DatebookDB, currIndex);
            // deleted records are stored at the end of the database
            //
            DmMoveRecord(DatebookDB, currIndex, DmNumRecords(DatebookDB));
        }
        else {
            MemHandleUnlock(recordH);
            currIndex++;
        }
    }
    return ret;
}

static void ShowScrollArrows(FormPtr frm, Int start, Int total)
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

static void PerformExport(CharPtr memo, int mainDBIndex, DateType when)
{
    VoidHand recordH = 0;
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

    case MainFormMenuNotify:
        // if (FrmGetActiveFormID() == MainForm) {
        //    gPrefsR->NotifyPrefs.records = '3';
        // }
        // else {
        //    gPrefsR->NotifyPrefs.records = '1';
        // }

        // Select Records to all and select is invalid
        //
        gPrefsR->NotifyPrefs.records = '3';
        
        FrmPopupForm(NotifySettingForm);

        handled = true;
        break;

    case MainFormMenuCleanup:
        SysCopyStringResource(gAppErrStr, RemoveConfirmString);
        if (FrmCustomAlert(CleanupAlert, gAppErrStr, " ", " ") == 0) {
            int ret;
            // do clean up processing
            ret = CleanupDatebook();
            StrPrintF(gAppErrStr, "%d", ret);
            FrmCustomAlert(CleanupDone, gAppErrStr, " ", " ");
        }
        handled = true;
        break;
    case MainFormMenuExport:
    {
        LineItemPtr ptr;
        Int i;
        UInt index;
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
    FormPtr frm = FrmGetActiveForm();
    TablePtr tableP = GetObjectPointer(frm, MainFormTable);

    CALLBACK_PROLOGUE;
    
    switch (e->eType) {
    case frmOpenEvent:
    case frmUpdateEvent:
        // Main Form table row settting
        //
        gMainTableRows = TblGetNumberOfRows(tableP);

        gMainTableStart = 0;
        MainFormDisplayCategory(false);

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
            SelectCategoryPopup(AddressDB);
            handled = true;
            
            break;

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

    CALLBACK_EPILOGUE;
        

    return handled;
}


static void LoadPrefsFields()
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return;
    
    SetFieldTextFromStr(PrefFormCustomField, gPrefsR->BirthPrefs.custom);
    SetFieldTextFromStr(PrefFormNotifyWith, gPrefsR->BirthPrefs.notifywith);

    if (gPrefsR->BirthPrefs.internal == '1') {
        CtlSetValue(GetObjectPointer(frm, PrefFormInternal), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, PrefFormInternal), 0);
    }

    if (gPrefsR->BirthPrefs.sort == '0') {
        CtlSetValue(GetObjectPointer(frm, PrefFormSortName), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, PrefFormSortDate), 1);
    }
}

static void UnloadPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    
    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return;
    if (FldDirty(GetObjectPointer(frm, PrefFormCustomField))) {
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

    ptr = GetObjectPointer(frm, PrefFormSortDate);
    if (CtlGetValue(ptr)) gPrefsR->BirthPrefs.sort = '1';
    else gPrefsR->BirthPrefs.sort = '0';

    ptr = GetObjectPointer(frm, PrefFormInternal);
    if (CtlGetValue(ptr)) gPrefsR->BirthPrefs.internal = '1';
    else gPrefsR->BirthPrefs.internal = '0';
}

static Boolean PrefFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm;
    
    CALLBACK_PROLOGUE;
    
    switch (e->eType) {
    case frmOpenEvent:
        frm = FrmGetActiveForm();

        LoadPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

        
    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case PrefFormOk:
            UnloadPrefsFields();
            WritePrefsRec();
            
            // clear the memory
            ClearFieldText(PrefFormCustomField);

            FrmUpdateForm(StartForm, frmRedrawUpdateCode);
            // may be bug gcc or emulator?
            // it redraw twice.. so I delete updateform
            // FrmUpdateForm(MainForm, frmRedrawUpdateCode);

            FrmReturnToForm(0);
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

    CALLBACK_EPILOGUE;
        

    return handled;
}

static Boolean TextMenuHandleEvent(Word menuID, Word objectID)
{
    FormPtr   form = FrmGetActiveForm();
    FieldPtr  field = GetObjectPointer(form, objectID);

    if (!field) return false;

    switch (menuID) {
    case TextMenuUndo:
        FldUndo(field);
        return true;
    case TextMenuCut:
        FldCut(field);
        return true;
    case TextMenuCopy:
        FldCopy(field);
        return true;
    case TextMenuPaste:
        FldPaste(field);
        return true;
    case TextMenuSAll:
        FldSetSelection(field, 0, FldGetTextLength (field));
        return true;
    case TextMenuKBoard:
        SysKeyboardDialog(kbdDefault);
        return true;

    case TextMenuGHelp:
        SysGraffitiReferenceDialog(referenceDefault);
        return true;
    }

    return false;
}

static Boolean Sl2LnFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    DateType dt = {0, 0};
    FormPtr frm = FrmGetActiveForm();

    CALLBACK_PROLOGUE;
    
    switch (e->eType) {
    case frmOpenEvent:
        DateSecondsToDate(TimGetSeconds(), &dt);

        DateToAsciiLong(dt.month, dt.day, dt.year + 1904,
                        gPrefdfmts, gAppErrStr);

        SetFieldTextFromStr(Sl2LnFormInput, gAppErrStr);

        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case Sl2LnFormConvert:
        {
            DateTimeType rtVal;
            BirthdateFlag dummy;
            Int year, month, day;
            Char* input;
            int ret = false;

            input = FldGetTextPtr(GetObjectPointer(frm, Sl2LnFormInput));
            if ((ret = AnalysisBirthDate(input, &dummy,
                                         &year, &month, &day))) {
                int leapyes = 0;

                ret = !sol2lun(year, month, day, &rtVal,
                               &leapyes);
                if (ret) {
                    if (leapyes) {
                        StrCopy(gAppErrStr, "#)");
                    }
                    else {
                        StrCopy(gAppErrStr, "-)");
                    }
                    DateToAsciiLong(rtVal.month, rtVal.day, rtVal.year,
                                    gPrefdfmts, &gAppErrStr[2]);
                          
                    FldDrawField(SetFieldTextFromStr(Sl2LnFormResult,
                                                     gAppErrStr));
                }

            }
            if (!ret) {
                FldDrawField(ClearFieldText(Sl2LnFormResult));
                SysCopyStringResource(gAppErrStr, InvalidDateString);
                FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " ");
            }

            handled = true;
            break;
        }

        case Sl2LnFormOk:
            FrmReturnToForm(0);

            handled = true;
            break;
            
        default:
            break;
                
        }
        break;

    case menuEvent:
        handled = TextMenuHandleEvent(e->data.menu.itemID, Sl2LnFormInput);
        break;

    default:
        break;
    }

    CALLBACK_EPILOGUE;

    return handled;
}

static Boolean Ln2SlFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    DateType dt = {0, 0};
    FormPtr frm = FrmGetActiveForm();

    CALLBACK_PROLOGUE;
    
    switch (e->eType) {
    case frmOpenEvent:
        DateSecondsToDate(TimGetSeconds(), &dt);
        DateToAsciiLong(dt.month, dt.day, dt.year + 1904,
                        gPrefdfmts, gAppErrStr);
        SetFieldTextFromStr(Ln2SlFormInput, gAppErrStr);

        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case Ln2SlFormOk:
            FrmReturnToForm(0);

            handled = true;
            break;

        case Ln2SlFormConvert:
        {
            DateTimeType rtVal;
            BirthdateFlag dummy;
            Int year, month, day;
            Char* input;
            int ret;
            
            input = FldGetTextPtr(GetObjectPointer(frm, Ln2SlFormInput));

            if ((ret = AnalysisBirthDate(input, &dummy,
                                         &year, &month, &day))) {
                int leapyes
                    = CtlGetValue(GetObjectPointer(frm, Ln2SlFormInputLeap));

                ret = !lun2sol(year, month, day, leapyes, &rtVal);
                if (ret) {
                    Char temp[15];
                    SysCopyStringResource(temp,
                                          DayOfWeek(rtVal.month, rtVal.day,
                                                    rtVal.year) + SunString);

                    DateToAsciiLong(rtVal.month, rtVal.day, rtVal.year,
                                    gPrefdfmts, gAppErrStr);
                    StrNCat(gAppErrStr, " [", AppErrStrLen);
                    StrNCat(gAppErrStr, temp, AppErrStrLen);
                    StrNCat(gAppErrStr, "]", AppErrStrLen);

                    FldDrawField(SetFieldTextFromStr(Ln2SlFormResult,
                                                     gAppErrStr));
                }
            }
            
            if (!ret) {
                FldDrawField(ClearFieldText(Ln2SlFormResult));
                SysCopyStringResource(gAppErrStr, InvalidDateString);
                FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " ");
            }
            
            handled = true;
            break;
        }
        
        default:
            break;
                
        }
        break;

    case menuEvent:
        handled = TextMenuHandleEvent(e->data.menu.itemID, Ln2SlFormInput);
        break;

    default:
        break;
    }

    CALLBACK_EPILOGUE;

    return handled;
}

void SelectCategoryPopup(DmOpenRef dbP)
{
    FormPtr frm;
    ListPtr lst;
    Int curSelection, newSelection;
    CharPtr name;

    frm = FrmGetActiveForm();

    if ((gAddrCategory = CategoryFind(dbP, gPrefsR->addrCategory)) ==
        dmAllCategories) {
        MemSet(gPrefsR->addrCategory, sizeof(gPrefsR->addrCategory), 0);
        CategoryGetName(dbP, gAddrCategory, gPrefsR->addrCategory);
    }

    lst = GetObjectPointer(frm, MainFormAddrCategories);

    // create a list of categories 
    CategoryCreateList(dbP, lst, gAddrCategory, true, true, 1, 0, true);

    // display the category list
    curSelection = LstGetSelection(lst);
    newSelection = LstPopupList(lst);

    // was a new category selected? 
    if ((newSelection != curSelection) && (newSelection != -1)) {
        if ((name = LstGetSelectionText(lst, newSelection))) {
            gAddrCategory = CategoryFind(dbP, name);

            MemSet(gPrefsR->addrCategory, sizeof(gPrefsR->addrCategory), 0);
            MemMove(gPrefsR->addrCategory, name, StrLen(name));

            gPrefsRdirty = true;
            MainFormDisplayCategory(true);
            
            gMainTableStart = 0;
            RereadBirthdateDB();
            MainFormLoadTable(frm, true);
            FldDrawField(ClearFieldText(MainFormField));
        }
    }
    CategoryFreeList(dbP, lst, false, false);
}

static Int CalculateAge(DateType converted, DateType birthdate,
                        BirthdateFlag flag)
{
    DateTimeType rtVal;
    Int age;
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


static void MainFormDrawRecord(VoidPtr tableP, Word row, Word column, 
                               RectanglePtr bounds)
{
    FontID currFont;
    VoidHand recordH = 0;
    SWord x, y;
    UInt nameExtent;
    PackedBirthDate* rp;
    BirthDate r;
    LineItemPtr ptr;
    LineItemType drawRecord;
	Boolean ignored = true;
    
    CALLBACK_PROLOGUE;

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
		SWord width, length;
		Int age;
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
						drawRecord.date.year+ 1904, gPrefdfmts, gAppErrStr);
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

        if (gPrefsR->BirthPrefs.internal == '1'
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

    CALLBACK_EPILOGUE;
}

// IN : index - mainDB index
//
static void SetBirthdateViewForm(Int index, DateType converted)
{
    FontID currFont;
    VoidHand recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    FormPtr frm;
    Char displayStr[255];
    UInt addrattr;
    Int age = 0;

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
                        converted.year+ 1904, gPrefdfmts,
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
                            gPrefdfmts, gAppErrStr);
        }
        else {
            DateToAsciiLong(r.date.month, r.date.day, -1, gPrefdfmts,
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
                             CharPtr pString)
{
    if (TimeToInt(when) != noTime) {
        TimeToAscii(when.hours, when.minutes, timeFormat, pString);
    }
    else {
        SysCopyStringResource(gAppErrStr, NoTimeString);
    }
}

static void LoadNotifyPrefsFields(void)
{
    FormPtr frm;

    if ((frm = FrmGetFormPtr(NotifySettingForm)) == 0) return;

    if (gPrefsR->NotifyPrefs.records == '0') {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormRecordAll), 1);
    }
    else if (gPrefsR->NotifyPrefs.records == '1') {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormRecordSlct), 1);
    }
    else {          //  '3' means that All is selected and slct is unusable
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormRecordAll), 1);
        CtlSetUsable(GetObjectPointer(frm, NotifySettingFormRecordSlct),
                     false);
    }
    
    if (gPrefsR->NotifyPrefs.existing == '0') {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormEntryKeep), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormEntryModify), 1);
    }

    if (gPrefsR->NotifyPrefs.private == '1') {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormPrivate), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormPrivate), 0);
    }

    if (gPrefsR->NotifyPrefs.datebk3 == '1') {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormBK3), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormBK3), 0);
	}

    if (gPrefsR->NotifyPrefs.alarm == '1') {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormAlarm), 1);
        FrmShowObject(frm, 
            FrmGetObjectIndex(frm, NotifySettingFormBefore));
    }
    else {
        CtlSetValue(GetObjectPointer(frm, NotifySettingFormAlarm), 0);
        FrmHideObject(frm,
            FrmGetObjectIndex(frm, NotifySettingFormBefore));
    }
    
    SetFieldTextFromStr(NotifySettingFormBefore,
        StrIToA(gAppErrStr, gPrefsR->NotifyPrefs.notifybefore));
    SetFieldTextFromStr(NotifySettingFormDuration,
        StrIToA(gAppErrStr, gPrefsR->NotifyPrefs.duration));
    
    TimeToAsciiLocal(gPrefsR->NotifyPrefs.when, gPreftfmts, gAppErrStr);
    CtlSetLabel(GetObjectPointer(frm, NotifySettingFormTime), gAppErrStr);
}

static void UnloadNotifyPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    
    if ((frm = FrmGetFormPtr(NotifySettingForm)) == 0) return;
    
    ptr = GetObjectPointer(frm, NotifySettingFormRecordAll);
    if (CtlGetValue(ptr)) gPrefsR->NotifyPrefs.records = '0';
    else gPrefsR->NotifyPrefs.records = '1';

    ptr = GetObjectPointer(frm, NotifySettingFormEntryKeep);
    if (CtlGetValue(ptr)) gPrefsR->NotifyPrefs.existing = '0';
    else gPrefsR->NotifyPrefs.existing = '1';

    ptr = GetObjectPointer(frm, NotifySettingFormPrivate);
    if ( CtlGetValue(ptr) ) gPrefsR->NotifyPrefs.private = '1';
    else gPrefsR->NotifyPrefs.private = '0';

    ptr = GetObjectPointer(frm, NotifySettingFormBK3);
    if (CtlGetValue(ptr)) gPrefsR->NotifyPrefs.datebk3 = '1';
    else gPrefsR->NotifyPrefs.datebk3 = '0';

    ptr = GetObjectPointer(frm, NotifySettingFormAlarm);
    if ( CtlGetValue(ptr) ) gPrefsR->NotifyPrefs.alarm = '1';
    else gPrefsR->NotifyPrefs.alarm = '0';

    if (FldDirty(GetObjectPointer(frm, NotifySettingFormBefore))) {
        gPrefsR->NotifyPrefs.notifybefore =
            (int) StrAToI(FldGetTextPtr(
                GetObjectPointer(frm, NotifySettingFormBefore)));
    }
    // notification duration
    if (FldDirty(GetObjectPointer(frm, NotifySettingFormDuration))) {
        gPrefsR->NotifyPrefs.duration =
            (int) StrAToI(FldGetTextPtr(
                GetObjectPointer(frm, NotifySettingFormDuration)));
    }
    
    // NotifySettingFormTime is set by handler
}

static Int CheckDatebookRecord(DateType when, const char* str)
{
    UInt numAppoints = 0;
    VoidHand apptListH;
    ApptDBRecordType dbRecord;
    ApptInfoPtr apptList;
    UInt recordNum;
    VoidHand recordH;
    Int i;

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
                if (StrNCaselessCompare(dbRecord.description, str,
                                    MIN(StrLen(dbRecord.description),
                                        StrLen(str))) == 0) {
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

// check global notify preference, and make datebookDB entry private
//
static void ChkNMakePrivateRecord(Int index)
{
    Int attributes;
    // if private is set, make the record private
    //
    if (gPrefsR->NotifyPrefs.private == '1') {
        DmRecordInfo(DatebookDB, index, &attributes,
                     NULL,NULL);
        attributes |= dmRecAttrSecret;
        DmSetRecordInfo(DatebookDB, index, &attributes, NULL);
    }
}

static Int PerformNotify(BirthDate birth, DateType when,
                         RepeatInfoType* repeatInfoPtr,
                         Int *created, Int *touched)
{
    ApptDBRecordType datebook;
    ApptDateTimeType dbwhen;
    AlarmInfoType dbalarm;
    Int age = 0;
    Char* description = 0;
    Int index;
    ApptDBRecordFlags changedFields;

    /* Zero the memory */
    MemSet(&datebook, sizeof(ApptDBRecordType), 0);
    MemSet(&dbwhen, sizeof(ApptDateTimeType), 0);
    MemSet(&dbalarm, sizeof(AlarmInfoType), 0);
    datebook.when = &dbwhen;
    datebook.alarm = &dbalarm;

    // set the date and time
    //
    dbwhen.date = when;
    dbwhen.startTime = gPrefsR->NotifyPrefs.when;
    dbwhen.endTime = gPrefsR->NotifyPrefs.when;

    // if alarm checkbox is set, set alarm
    //
    if (gPrefsR->NotifyPrefs.alarm == '1' &&
        gPrefsR->NotifyPrefs.notifybefore >= 0) {
        dbalarm.advanceUnit = aauDays;
        dbalarm.advance = gPrefsR->NotifyPrefs.notifybefore;
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

	// if datebook is datebk3, set icon
	//
	if (gPrefsR->NotifyPrefs.datebk3 == '1') {
    	datebook.note = DATEBK3_ICON;
	}
	else datebook.note = NULL;

    // make the description
        
    description = MemPtrNew(1024);
    SysCopyStringResource(gAppErrStr, NotEnoughMemoryString);
    ErrFatalDisplayIf(!description, gAppErrStr);
        
    StrCopy(description, "[");

    // make description
    StrNCat(description, birth.name1, 1024);
    if (birth.name2 && birth.name2[0]) {
        if (birth.name1 && birth.name1[0])
            StrNCat(description, ", ", 1024);
        StrNCat(description, birth.name2, 1024);
    }
    StrNCat(description, "] ", 1024);

    StrNCat(description, gPrefsR->BirthPrefs.notifywith, 1024);
    // set event type
    StrNCat(description, EventTypeString(birth), 1024);
    StrNCat(description, " ", 1024);
    
    if (birth.flag.bits.lunar || birth.flag.bits.lunar_leap) {
        
        if (birth.flag.bits.lunar) {
            StrNCat(description, "-)", 1024);
        }
        else if (birth.flag.bits.lunar_leap) {
            StrNCopy(description, "#)", 1024);
        }

        if (birth.flag.bits.year) {
            DateToAsciiLong(birth.date.month, birth.date.day,
                            birth.date.year + 1904,
                            gPrefdfmts, gAppErrStr);
        }
        else {
            DateToAsciiLong(birth.date.month, birth.date.day, -1, gPrefdfmts,
                            gAppErrStr);
        }
    
        StrNCat(description, gAppErrStr, 1024);
        StrNCat(description, " ", 1024);
    }
    if (birth.flag.bits.year) {
		if (!birth.flag.bits.solar || gPrefsR->NotifyPrefs.duration ==1) {
        	age = CalculateAge(when, birth.date, birth.flag);
        	if (age >= 0) {
           		StrPrintF(gAppErrStr, " (%d)", age);
            	StrNCat(description, gAppErrStr, 1024);
        	}
		}
		else if (birth.flag.bits.solar && gPrefsR->NotifyPrefs.duration != 1) {
			StrPrintF(gAppErrStr, "%d", birth.date.year + 1904);
			StrNCat(description, gAppErrStr, 1024);
		}
    }

    datebook.description = description;

    if ((index = CheckDatebookRecord(when, description)) < 0) {
        // if not exists
        // write the new record (be sure to fill index)
        //
        ApptNewRecord(DatebookDB, &datebook, &index);
        // if private is set, make the record private
        //
        ChkNMakePrivateRecord(index);
        (*created)++;
    }
    else {                                      // if exists
        if (gPrefsR->NotifyPrefs.existing == '0') {
            // keep the record
        }
        else {
            // modify
            changedFields.when = 1;
            changedFields.description = 1;
            changedFields.repeat = 1;
            changedFields.alarm = 1;
			changedFields.note = 1;

            ApptChangeRecord(DatebookDB, &index, &datebook,
                             changedFields);
            // if private is set, make the record private
            //
            ChkNMakePrivateRecord(index);
        }
        (*touched)++;
    }

    if (description) MemPtrFree(description);
    
    return 0;
}

static void NotifyDatebook(int mainDBIndex, DateType when,
                           Int *created, Int *touched)
{
    VoidHand recordH = 0;
    PackedBirthDate* rp;
    BirthDate r;
    RepeatInfoType repeatInfo;
    Int i;
    
    // if duration is equal to 0, no notify record is created
    //
    if (gPrefsR->NotifyPrefs.duration == 0) return;

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

            if (gPrefsR->NotifyPrefs.duration > 1) {
                // if solar date, make entry repeat
                //
                repeatInfo.repeatEndDate = when;
                repeatInfo.repeatEndDate.year +=
                    gPrefsR->NotifyPrefs.duration -1;

                // if duration > 1, 'when' is the birthdate;
                if (r.flag.bits.year) when = r.date;
                
                PerformNotify(r, when, &repeatInfo, created, touched);
            }
            else if (gPrefsR->NotifyPrefs.duration == -1) {
                // if duration > 1, 'when' is the birthdate;
                if (r.flag.bits.year) when = r.date;
                DateToInt(repeatInfo.repeatEndDate) = -1;
                PerformNotify(r, when, &repeatInfo, created, touched);
            }
            else PerformNotify(r, when, NULL, created, touched);
        }
        else if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {
            // if lunar date, make each entry
            //
            int duration = gPrefsR->NotifyPrefs.duration;
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

                PerformNotify(r, converted, NULL, created, touched);

                // process next target day
                //
                current = converted;
                DateAdjust(&current, 1);
            }
        }
        
        MemHandleUnlock(recordH);
    }
}

static Boolean NotifyFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetActiveForm();
    LineItemPtr ptr;
    Int i;

    CALLBACK_PROLOGUE;
    
    switch (e->eType) {
    case frmOpenEvent:

        LoadNotifyPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case NotifySettingFormOk: 
        {
            Int created = 0, touched = 0;
            Char* info = 0;
            Char temp[255];
            
            UnloadNotifyPrefsFields();
            WritePrefsRec();
            
            // clear memory
            //
            ClearFieldText(NotifySettingFormBefore);

			if (gMainTableTotals >0) {
                ptr = MemHandleLock(gTableRowHandle);
                if (gPrefsR->NotifyPrefs.records == '0') {  // all
                    StrPrintF(gAppErrStr, "%d", gMainTableTotals);
                
                    if (FrmCustomAlert(NotifyWarning, gAppErrStr, " ", " ")
                        == 0) {             // user select OK button
                        for (i=0; i < gMainTableTotals; i++) {
                            if (ptr[i].date.year != INVALID_CONV_DATE) {
                                NotifyDatebook(ptr[i].birthRecordNum,
                                               ptr[i].date, &created,
                                               &touched);
                            }
                        }
                    }
                }
                else {                                      // selected
                    if (ptr[gMainTableHandleRow].date.year
                        != INVALID_CONV_DATE) {
                        NotifyDatebook(ptr[gMainTableHandleRow].birthRecordNum,
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

                if (gPrefsR->NotifyPrefs.existing == '0') {  // keep
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
            FrmReturnToForm(0);
            handled = true;
            break;
        }
        
        case NotifySettingFormCancel:
            UnloadNotifyPrefsFields();
            WritePrefsRec();

            FrmReturnToForm(0);
            handled = true;
            break;

        case NotifySettingFormAlarm:
            if (CtlGetValue(GetObjectPointer(frm,
                                             NotifySettingFormAlarm))) {
                FrmShowObject(frm,
                    FrmGetObjectIndex(frm, NotifySettingFormBefore));
            }
            else {
                FrmHideObject(frm, 
                    FrmGetObjectIndex(frm, NotifySettingFormBefore));
            }
            
            handled = true;
            break;
        case NotifySettingFormTime:
        {
            TimeType startTime, endTime;
            Boolean untimed = false;

            startTime = endTime = gPrefsR->NotifyPrefs.when;
            if (TimeToInt(startTime) == noTime) {
                untimed = true;
                // set dummy time
                startTime.hours = endTime.hours  = 8;
                startTime.minutes = endTime.minutes = 0;
            }
            
            SysCopyStringResource(gAppErrStr, SelectTimeString);
            if ((SelectTime(&startTime, &endTime, untimed, gAppErrStr, 8))) {

                gPrefsR->NotifyPrefs.when = startTime;
                TimeToAsciiLocal(gPrefsR->NotifyPrefs.when, gPreftfmts,
                                gAppErrStr);
                
                CtlSetLabel(GetObjectPointer(frm, NotifySettingFormTime),
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

    CALLBACK_EPILOGUE;

    return handled;
}

static Boolean BirthdateFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    FormPtr frm = FrmGetActiveForm();

    CALLBACK_PROLOGUE;
    
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
        case BirthdateNotify:
            //  Notify event into the datebook entry
            //
            gPrefsR->NotifyPrefs.records = '1';
            FrmPopupForm(NotifySettingForm);

            handled = true;
            break;
        case BirthdateGoto:
        {
            VoidHand recordH = 0;
            PackedBirthDate* rp;
            DWord gotoIndex;
            
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

    CALLBACK_EPILOGUE;

    return handled;
}

static Boolean StartFormHandlerEvent(EventPtr e)
{
    Err err;
    Boolean handled = false;
    FormPtr frm = FrmGetActiveForm();

    CALLBACK_PROLOGUE;

    // no event procedure
    //
    switch (e->eType) {
    case frmOpenEvent:
    case frmUpdateEvent:
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
    
    CALLBACK_EPILOGUE;
    
    return handled;
}

static Boolean ApplicationHandleEvent(EventPtr e)
{
    FormPtr frm;
    Word    formId;
    Boolean handled = false;

    if (e->eType == frmLoadEvent) {
        formId = e->data.frmLoad.formID;
        frm = FrmInitForm(formId);
        FrmSetActiveForm(frm);

        switch(formId) {
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
        case NotifySettingForm:
            FrmSetEventHandler(frm, NotifyFormHandleEvent);
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
    Ptr p;

    if (PrefsDB) {
        p = MemHandleLock(PrefsRecHandle);
        MemMove(gPrefsR, p, sizeof(struct sPrefsR));
        MemPtrUnlock(p);
        gPrefsRdirty = false;
        gPrefsWeredirty = false;
    }
}

static void WritePrefsRec(void)
{
    Ptr p;
    Err err;
    
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
    
static Int OpenDatabases(void)
{
    SystemPreferencesType sysPrefs;
    UInt mode = 0;
    UInt cardNo;
    LocalID dbID, appInfoID;
    DatabaseHdrType hdr;
    Boolean newSearch = true;
    ULong creator;
    UInt attributes;
    Ptr p, p2;
    Err err;
    DmSearchStateType searchInfo;
    DWord cdate, mdate;
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

    gPrefdfmts = sysPrefs.dateFormat;       // save global data format
    gPreftfmts = sysPrefs.timeFormat;       // save global time format
    
    /*
     * Quick way to open all needed databases.  Thanks to Danny Epstein.
     */
    while ((DmGetNextDatabaseByTypeCreator(newSearch, &searchInfo, 'DATA',
                NULL, false, &cardNo, &dbID) == 0) &&
           (!DatebookDB || !AddressDB || !MemoDB || !MainDB || !PrefsDB)) {
        if (DmDatabaseInfo(cardNo, dbID, hdr.name, NULL, NULL, &cdate, &mdate,
                           NULL, NULL, &appInfoID, NULL, NULL, &creator))
            break;
        if ((creator == DatebookAppID) &&
            (StrCaselessCompare(hdr.name, DatebookDBName) == 0)) {
            DatebookDB = DmOpenDatabase(cardNo, dbID, mode |
                                        dmModeReadWrite);
        }
        else if ((creator == MemoAppID)
            && (StrCaselessCompare(hdr.name, MemoDBName) == 0)) {
            MemoDB = DmOpenDatabase(cardNo, dbID, mode | dmModeReadWrite);
        } else if ((creator == AddressAppID) &&
                   (StrCaselessCompare(hdr.name, AddressDBName) == 0)) {
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
                   (StrCaselessCompare(hdr.name, MainDBName) == 0)) {
            MainDB = DmOpenDatabase(cardNo, dbID, dmModeReadWrite);
        } else if ((creator == MainAppID) &&
                   (StrCaselessCompare(hdr.name, PrefsDBName) == 0)) {
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
        p2 = (Ptr) &DefaultPrefsR;
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
    if (!DatebookDB || !AddressDB || !MemoDB) return EDBCREATE;

    return 0;
}

void CloseDatabases(void)
{
    if (DatebookDB) DmCloseDatabase(DatebookDB);
    if (AddressDB) DmCloseDatabase(AddressDB);
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
static Word StartApplication(void)
{
    Err err;
    PrefsRHandle = 0;
    gTableRowHandle = 0;
    
    if ((PrefsRHandle = MemHandleNew(sizeof (struct sPrefsR))) == 0) {
        freememories();
        return 1;
    }
    gPrefsR = MemHandleLock(PrefsRHandle);

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

/* The main event loop */
static void EventLoop(void)
{
    Word err;
    EventType e;

    do {
        EvtGetEvent(&e, evtWaitForever);
        if (! SysHandleEvent (&e))
            if (! MenuHandleEvent (NULL, &e, &err))
                if (! ApplicationHandleEvent (&e))
                    FrmDispatchEvent (&e);
    } while (e.eType != appStopEvent && !gProgramExit);
}

/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Word err;

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
