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

#include "address.h"
#include "memodb.h"

#include "birthdate.h"
#include "happydays.h"
#include "happydaysRsc.h"
#include "calendar.h"
#include "s2lconvert.h"
#include "util.h"
#include "notify.h"

#define frmRescanUpdateCode (frmRedrawUpdateCode + 1)
#define frmUpdateFontCode (frmRedrawUpdateCode + 2)

////////////////////////////////////////////////////////////////////
// 	Global variable
////////////////////////////////////////////////////////////////////

MemHandle gTableRowHandle;

Int16 gHappyDaysField;
Char gAppErrStr[AppErrStrLen];
Char gLookupDate[12];			// date string of look up
UInt32 gAdcdate, gAdmdate;      // AddressBook create/modify time
UInt32 gMmcdate, gMmmdate;      // Memo create/modify time
Boolean gSortByCompany=true;    // sort by company is set in AddressBook?
UInt16 gAddrCategory;           // address book category
Int16 gMainTableTotals;       	// Main Form table total rows;
Int16 gMainTableStart = 0;      // Main Form table starts
Int16 gMainTablePageSize = 0;   // Page size of Maintable
Int16 gMainTableHandleRow = -1; // The row of birthdate view to display
DateFormatType gPrefdfmts;  	// global date format for Birthday field
DateFormatType gSystemdfmts;  	// global system date format 
TimeFormatType gPreftfmts;  	// global time format
DateType gStartDate;			// staring date of birthday listing

Boolean gbVgaExists = false;

Boolean gProgramExit = false;   // Program exit control(set by Startform)

////////////////////////////////////////////////////////////////////
// Prefs stuff 
////////////////////////////////////////////////////////////////////

struct sPrefsR gPrefsR;
struct sPrefsR defaultPref = {
    HDAppVer,
    0, 0, 0,              // all/selected, keep/modified, private
    {   1, 0, 9, 3, 1, {-1, -1}, "145" },   // Datebook notify prefs
#ifdef GERMAN
    {  1, "Alle" },                         // Todo notify prefs
    {  "Geburtstag", "*HD:",                // Prefs
       1, 1, 0, 0, 0, dfDMYWithDots },   
#else
    {  1, "All" },                          // Todo notify prefs
    {  "Birthday", "*HD:",                  // Prefs
       1, 1, 0, 0, 0, dfMDYWithSlashes },   
#endif
    {  1, 0, 0 },                           // Display Preferences
    0,                                      // hide secret record
#ifdef GERMAN
    "Alle",                                 // Address category
#else
    "All", 
#endif
    "\0\0\0", "\0\0\0",         /* addr create/modify date */
    "\0\0\0", "\0\0\0"          /* memo create/modify date */
};


enum ViewFormType { ViewType = 0,
                    ViewNext,
                    ViewSrc,
                    ViewSrcExtra,
                    ViewAge,
                    ViewRemained,
                    ViewSpace
};

////////////////////////////////////////////////////////////////////
// function declaration 
////////////////////////////////////////////////////////////////////
static UInt16 StartApplication(void);
static void EventLoop(void);
static void StopApplication(void);
static void Search(FindParamsPtr findParams);
static void GoToItem (GoToParamsPtr goToParams, Boolean launchingApp);
static Boolean ApplicationHandleEvent(EventPtr e);
static Boolean SpecialKeyDown(EventPtr e);
static Int16 OpenDatabases(void);
static void freememories(void);
static void CloseDatabases(void);
static void MainFormReadDB();
static int CalcPageSize(FormPtr frm);

static void ViewFormLoadTable(FormPtr frm);

static Boolean MenuHandler(FormPtr frm, EventPtr e);

static Boolean StartFormHandleEvent(EventPtr e);
static Boolean PrefFormHandleEvent(EventPtr e);
static Boolean DispPrefFormHandleEvent(EventPtr e);
static Boolean ViewFormHandleEvent(EventPtr e);
static Boolean MainFormHandleEvent(EventPtr e);

static void HighlightMatchRowDate(DateTimeType inputDate);
static void HighlightMatchRowName(Char first);

static void WritePrefsRec(void);
static void MainFormLoadTable(FormPtr frm, Int16 listOffset);
static void MainFormInit(FormPtr formP, Boolean resize);
static void MainFormResize(FormPtr frmP, Boolean draw);
static void MainTableSelectItem(TablePtr table, Int16 row, Boolean selected);
static void MainFormDrawRecord(MemPtr tableP, Int16 row, Int16 column, 
                               RectanglePtr bounds);

////////////////////////////////////////////////////////////////////
// private database for HappyDays
////////////////////////////////////////////////////////////////////

DmOpenRef MainDB;
 
////////////////////////////////////////////////////////////////////
//  accessing the built-in DatebookDB
////////////////////////////////////////////////////////////////////

DmOpenRef DatebookDB;

////////////////////////////////////////////////////////////////////
//  accessing the built-in ToDoDB
////////////////////////////////////////////////////////////////////

DmOpenRef ToDoDB;

////////////////////////////////////////////////////////////////////
// These are for accessing the built-in AddressDB
////////////////////////////////////////////////////////////////////

DmOpenRef AddressDB;


////////////////////////////////////////////////////////////////////
// These are for accessing the built-in MemoDB
////////////////////////////////////////////////////////////////////

DmOpenRef MemoDB;

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

/* Get preferences, open (or create) app database */
static UInt16 StartApplication(void)
{
    Int8 err;
	UInt32 version;

    gTableRowHandle = 0;

	if (_TRGVGAFeaturePresent(&version)) gbVgaExists = true;
	else gbVgaExists = false;

    if(gbVgaExists)
    	VgaSetScreenMode(screenMode1To1, rotateModeNone);

    // set current date to  the starting date
	DateSecondsToDate(TimGetSeconds(), &gStartDate);
    
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
	StrCopy(gPrefsR.addrCategory, "All");

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

	// gMainTableStart = MAX(0, selected-4);
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

static void DrawSearchLine(HappyDays hd, RectangleType r)
{
	DrawRecordName(hd.name1, hd.name2, 
                   r.extent.x, &r.topLeft.x, 
                   r.topLeft.y, false,
                   hd.flag.bits.priority_name1);
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

     if (hd.flag.bits.lunar) {
     StrCopy(displayStr, "-)");
     }
     else if (hd.flag.bits.lunar_leap) {
     StrCopy(displayStr, "#)");
     }
     else displayStr[0] = 0;

     if (hd.flag.bits.year) {
     DateToAsciiLong(hd.date.month, hd.date.day, 
     hd.date.year + 1904,
     gPrefdfmts, displayStr + StrLen(displayStr));
     }
     else {
     DateToAsciiLong(hd.date.month, hd.date.day, -1, 
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

	// refine this code(**************)
	//
   	// Open the happydays database.
   	// dbP = DmOpenDatabase(cardNo, dbID, findParams->dbAccesMode);
	// 
	// refine this code(**************)
	dbID = DmFindDatabase(cardNo, MainDBName);
	if (! dbID) return;
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
         	HappyDays       hd;
         
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
      
         	UnpackHappyDays(&hd, MemHandleLock(recordH));
         
         	if ((match = FindStrInStr((Char*) hd.name1, 
                                      findParams->strToFind, &pos)) != false)
            	fieldNum = 1;
         	else if ((match = FindStrInStr((Char*) hd.name2, 
                                           findParams->strToFind, &pos)) != false)
            	fieldNum = 2;
            
         	if (match) {
           	 	done = FindSaveMatch(findParams, recordNum, pos, fieldNum, 0, cardNo, dbID);
            	if (done) break;
   
            	// Get the bounds of the region where we will draw the results.
            	FindGetLineBounds(findParams, &r);
            
            	// Display the title of the description.
				DrawSearchLine(hd, r);

            	findParams->lineNumber++;
         	}
         	MemHandleUnlock(recordH);

         	if (done) break;
         	recordNum++;
      	}
   	}  
   	DmCloseDatabase(dbP);   
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
            FrmSetEventHandler(frm, StartFormHandleEvent);
            break;
        case MainForm: 
            FrmSetEventHandler(frm, MainFormHandleEvent);
            break;
        case PrefForm:
            FrmSetEventHandler(frm, PrefFormHandleEvent);
            break;
        case DispPrefForm:
            FrmSetEventHandler(frm, DispPrefFormHandleEvent);
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
        case ViewForm:
            FrmSetEventHandler(frm, ViewFormHandleEvent);
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
    if (gTableRowHandle) {
        MemHandleFree(gTableRowHandle);
        gTableRowHandle = 0;
    }
}

static void ReadPrefsRec(void)
{
	UInt16 prefsSize = sizeof(gPrefsR);
	Int16 version =
		PrefGetAppPreferences(HDAppID, 0, 
                              (void*)&gPrefsR, &prefsSize, true);

	if (prefsSize != sizeof(gPrefsR) 
        || version == noPreferenceFound
        || gPrefsR.version != version) {
		MemMove(&gPrefsR, &defaultPref, sizeof(defaultPref));
		gPrefsR.listFont = (gbVgaExists) ? VgaBaseToVgaFont(stdFont) 
            : stdFont;
        if (gPrefsR.Prefs.sysdateover)
            gPrefdfmts = gPrefsR.Prefs.dateformat;
    }
}

static void WritePrefsRec(void)
{
	PrefSetAppPreferences(HDAppID, 0, HDAppVer, (void*)&gPrefsR, 
                          sizeof(gPrefsR), true);
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
    DmSearchStateType searchInfo;
    UInt32 cdate, mdate;
    AddrAppInfoPtr appInfoPtr;

    /*
     * Set these to zero in case any of the opens fail.
     */
    MainDB = 0;
    DatebookDB = 0;
    AddressDB = 0;
    MemoDB = 0;

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
            || !MemoDB || !MainDB)) {
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
        } else if ((creator == HDAppID) &&
                   (StrCaselessCompare(dbname, MainDBName) == 0)) {
            MainDB = DmOpenDatabase(cardNo, dbID, dmModeReadWrite);
		}
        newSearch = false;
    }

    if (!MainDB){
        //
        // Create our database if it doesn't exist yet
        //
        if (DmCreateDatabase(0, MainDBName, HDAppID, MainDBType, false))
            return -1;
        MainDB = DmOpenDatabaseByTypeCreator(MainDBType, HDAppID,
                                             dmModeReadWrite);
        if (!MainDB) return -1;

        /* Set the backup bit.  This is to aid syncs with non Palm software. */
        DmOpenDatabaseInfo(MainDB, &dbID, NULL, NULL, &cardNo, NULL);

    }
    /*
     * Read in the preferences from the private database.  This also loads
     * some global variables with values for convenience.
     */
    ReadPrefsRec();

    // if hideSecret setting is changed
    if (gPrefsR.gHideSecretRecord != sysPrefs.hideSecretRecords) {
        // reset the address book 
		//
        MemSet(gPrefsR.adrcdate, sizeof(gPrefsR.adrcdate), 0);
        MemSet(gPrefsR.adrmdate, sizeof(gPrefsR.adrmdate), 0);
        
        gPrefsR.gHideSecretRecord = sysPrefs.hideSecretRecords;
    }

    /*
     * Check if the other databases opened correctly.  Abort if not.
     */
    if (!DatebookDB || !ToDoDB || !AddressDB || !MemoDB) return -1;

    return 0;
}

static void CloseDatabases(void)
{
    if (DatebookDB) DmCloseDatabase(DatebookDB);
    if (AddressDB) DmCloseDatabase(AddressDB);
    if (ToDoDB) DmCloseDatabase(ToDoDB);
    if (MemoDB) DmCloseDatabase(MemoDB);
    if (MainDB) DmCloseDatabase(MainDB);

	WritePrefsRec();
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
  gPrefsR.Prefs.sort = 0;     // sort by name
  SndPlaySystemSound(sndClick);

  gMainTableStart = 0;
  FrmUpdateForm(MainForm, frmRedrawUpdateCode);
  return true;
  }
  else if (keyboardNumericChr == chr) {
  gPrefsR.Prefs.sort = 1;     // sort by date
  SndPlaySystemSound(sndClick);

  gMainTableStart = 0;
  FrmUpdateForm(MainForm, frmRedrawUpdateCode);
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

static void RereadHappyDaysDB(DateType start)
{
    gMainTableTotals = AddrGetHappyDays(MainDB, gAddrCategory, start);
}

static void HighlightAction(int selected, Boolean sound)
{
  FormPtr frm = FrmGetActiveForm();
  TablePtr tableP = GetObjectPointer(frm, MainFormTable);

  if (sound) SndPlaySystemSound(sndClick);

  if (gCurrentSelection >= 0) {
      // unhighlight the selection
      MainTableSelectItem(tableP, gCurrentSelection, false);
  }

  if (gMainTableStart > selected
      || selected >= (gMainTableStart + gMainTablePageSize) ) {
      // if not exist in table view, redraw table
      gMainTableStart = MAX(0, selected-4);
      MainFormLoadTable(frm, gMainTableStart);
      TblDrawTable(tableP);
  }
  gCurrentSelection = selected - gMainTableStart;
  MainTableSelectItem(tableP, gCurrentSelection, true);
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
    PackedHappyDays* rp;
    HappyDays r;
    Char p;

    if (gMainTableTotals <= 0) return;

    if ((ptr = MemHandleLock(gTableRowHandle))) {
        for (i = 0; i < gMainTableTotals; i++) {
            if ((recordH = DmQueryRecord(MainDB, ptr[i].birthRecordNum))) {
                rp = (PackedHappyDays*) MemHandleLock(recordH);
                UnpackHappyDays(&r,rp);

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

		gMainTableHandleRow = -1;
		FrmUpdateForm(MainForm, frmRedrawUpdateCode);
    }
}



static Boolean MenuHandler(FormPtr frm, EventPtr e)
{
    Boolean 	handled = false;
	static Char tmpString[25];

	// TablePtr tableP = GetObjectPointer(frm, MainFormTable);
	// TblUnhighlightSelection(tableP);

    MenuEraseStatus(NULL);
    
    switch(e->data.menu.itemID) {
    case MainMenuAbout: 
        frm = FrmInitForm(AboutForm);
		
        if(gbVgaExists)
       		VgaFormModify(frm, vgaFormModify160To240);

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

	case MainMenuFont: {
		FontID fontID;

		fontID = FontSelect(gPrefsR.listFont);

		if (fontID != gPrefsR.listFont) {
			gPrefsR.listFont = fontID;
			FrmUpdateForm(MainForm, frmUpdateFontCode);
		}
		handled = true;
		break;
	}

    case MainMenuPref:
        FrmPopupForm(PrefForm);
            
        handled = true;
        break;

    case MainMenuDispPref:
        FrmPopupForm(DispPrefForm);
            
        handled = true;
        break;

    case MainMenuRescanAddr:
        MemSet(&gPrefsR.adrmdate, 4, 0);  	    // force a re-scan

        if (gPrefsR.Prefs.autoscan == 2) {      // trick (force rescan)
            gPrefsR.Prefs.autoscan = 3;         //    process Never
        }
    
        FrmGotoForm(StartForm);

        handled = true;
        break;
        
    case MainMenuNotifyDatebook:

        // Select Records to all and select is invalid
        //
        gPrefsR.records = 3;
        
        FrmPopupForm(DateBookNotifyForm);

        handled = true;
        break;

    case MainMenuCleanupDatebook: 
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
    case MainMenuNotifyTodo:
        // Select Records to all and select is invalid
        //
        gPrefsR.records = 3;
        
        FrmPopupForm(ToDoNotifyForm);

        handled = true;
        break;
        
    case MainMenuCleanupTodo: 
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
    
    case MainMenuExport:
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
    
    case MainMenuSl2Ln:
        FrmPopupForm(Sl2LnForm);
                
        handled = true;
        break;
    case MainMenuLn2Sl:
        FrmPopupForm(Ln2SlForm);   

        handled = true;
        break;
    }

    return handled;
}

static void MainFormScroll(Int16 newValue, Int16 oldValue, Boolean force_redraw)
{
    TablePtr    tableP;
	FormPtr 	frm = FrmGetActiveForm();

	tableP = GetObjectPointer(frm, MainFormTable);

	if(oldValue != newValue) {
		MainFormLoadTable(frm, newValue);

		if(force_redraw)
			TblDrawTable(tableP);
	}
}

static void MainFormScrollLines(Int16 lines, Boolean force_redraw)
{
	ScrollBarPtr    barP;
    Int16           valueP, minP, maxP, pageSizeP;
    Int16           newValue;
	FormPtr			frm = FrmGetActiveForm();

    barP = GetObjectPointer(frm, MainFormScrollBar);
    SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

    //scroll up
    if(lines < 0)
    {
        //we are at the start
        if(valueP == minP)
            return;

        newValue = valueP + lines;
        if(newValue < minP)
            newValue = minP;
    }
    else
    {
        if(valueP == maxP)
            return;

        newValue = valueP + lines;
        if(newValue  > maxP) 
            newValue = maxP;
    }

    SclSetScrollBar(barP, newValue, minP, maxP, pageSizeP);
    MainFormScroll(newValue, valueP, force_redraw);
}

static void MainFormResize(FormPtr frmP, Boolean draw)
{
	Int16 	valueP, minP, maxP, pageSizeP;
	ScrollBarPtr barP;
	Coord	x, y;
	Coord	y_diff;
	RectangleType r, erase_rect;

	WinGetDisplayExtent(&x, &y);
	FrmGetFormBounds(frmP, &r);

    // calc y difference
    //
	y_diff = y - (r.topLeft.y + r.extent.y);

	if (draw && (y_diff <0)) {
		// if the siklscreen was maximized,
		// 		erase the area under maxmized silkscreen
		erase_rect = r;
		erase_rect.topLeft.y = r.extent.y + y_diff;
		erase_rect.extent.y = -y_diff;
		WinEraseRectangle(&erase_rect, 0);
	}

	// resize the form
	r.extent.y += y_diff;
	WinSetWindowBounds(FrmGetWindowHandle(frmP), &r);

	// move bottom controls button
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, MainFormLookUp), y_diff, draw);
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, MainFormStart), y_diff, draw);
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, MainFormName), y_diff, draw);
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, MainFormDate), y_diff, draw);
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, MainFormAge), y_diff, draw);

	// resize scroll bar
	PrvResizeObject(frmP, FrmGetObjectIndex(frmP, MainFormScrollBar), 
                    y_diff, draw);

	// Resize the table
	PrvResizeObject(frmP, FrmGetObjectIndex(frmP, MainFormTable), 
					y_diff, draw);
    
    barP = GetObjectPointer(frmP, MainFormScrollBar);
    SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);
    
	if (draw) {
		MainFormLoadTable(frmP, valueP);
		FrmDrawForm(frmP);
		gMainTableHandleRow = -1;
	}
}

static void MainFormReadDB()
{
	// category information of MainDB is same as AddressDB
	//
	if ((gAddrCategory = CategoryFind(AddressDB, gPrefsR.addrCategory)) ==
		dmAllCategories) {
		MemSet(gPrefsR.addrCategory, sizeof(gPrefsR.addrCategory), 0);
		CategoryGetName(AddressDB, gAddrCategory, gPrefsR.addrCategory);
	}
	RereadHappyDaysDB(gStartDate);
}

static void HandlePrevKey(void)
{
    ScrollBarPtr barP;
    Int16        valueP, minP, maxP, pageSizeP;
	FormPtr frm = FrmGetActiveForm();

    barP = GetObjectPointer(frm, MainFormScrollBar);
    SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

    if (gCurrentSelection == -1)
        gCurrentSelection = 0;
    else {
        if (gCurrentSelection > 0) {
			MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, false);
            gCurrentSelection --;
		}
        else {
            if (valueP > minP) MainFormScrollLines(-1, true);
        }
    }

	MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                            gCurrentSelection, true);
}

static void HandleNextKey(void)
{
    ScrollBarPtr barP;
    Int16        valueP, minP, maxP, pageSizeP;
	FormPtr frm = FrmGetActiveForm();

    barP = GetObjectPointer(frm, MainFormScrollBar);
    SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

    if (gCurrentSelection == -1)
        gCurrentSelection = 0;
    else
    {
        if (gCurrentSelection < pageSizeP - 1) {
			MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, false);

            gCurrentSelection ++;
		}
        else {
            if (valueP <  maxP) MainFormScrollLines(1, true);
        }
    }

	MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                            gCurrentSelection, true);
}


static void MainFormHandleSelect(FormPtr frm, Int16 row)
{
	ScrollBarPtr barP;
	Int16        valueP, minP, maxP, pageSizeP;

	barP = GetObjectPointer(frm, MainFormScrollBar);
	SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

	gMainTableHandleRow = gCurrentSelection + valueP;

	FrmGotoForm(ViewForm);
}

static Boolean MainFormHandleEvent (EventPtr e)
{
    Boolean handled = false;
	Boolean bformModify = true;

    FormPtr frm = FrmGetFormPtr(MainForm);

    switch (e->eType) {
    case frmUpdateEvent:
        if (e->data.frmUpdate.updateCode == frmRescanUpdateCode)
        {
            MemSet(&gPrefsR.adrmdate, 4, 0);  	// force a re-scan
            FrmGotoForm(StartForm);
            handled = true;
			break;
        }
		MainFormReadDB();
        // frmRedrawUpdateCode invoked by SpecialKeyDown
        //
		// else execute frmOpenEvent
		bformModify = false;
        gMainTableHandleRow = -1;

    case frmOpenEvent: 
    {
        MainFormInit(frm, bformModify);
        gMainTablePageSize = CalcPageSize(frm);

        if (gMainTableHandleRow >= 0) {
            if (gMainTableStart > gMainTableHandleRow ||
                gMainTableHandleRow >= (gMainTableStart + gMainTablePageSize) ) {
                // if not exist in table view, redraw table
            
                gMainTableStart = MAX(0, gMainTableHandleRow-4);
            }
            gCurrentSelection = gMainTableHandleRow - gMainTableStart;
        }
        else {
            gMainTableStart = 0;
        }
        MainFormLoadTable(frm, gMainTableStart);
        
        FrmDrawForm(frm);
        
        if (gCurrentSelection >= 0) {
            MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, true);
        }
                
        handled = true;
        break;
    }
    
	case displayExtentChangedEvent:
		MainFormResize(FrmGetActiveForm(), true);
		break;

    case menuEvent: 
        handled = MenuHandler(frm, e);
        break;

	case sclRepeatEvent:
	   	gCurrentSelection =  -1; 
		MainFormScroll(e->data.sclRepeat.newValue, 
                       e->data.sclRepeat.value, true);
		break;

            
    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case MainFormStart:
			// display starting date
			DateToAsciiLong(gStartDate.month, gStartDate.day, 
							gStartDate.year+1904, gPrefdfmts, gLookupDate);
			CtlSetLabel(GetObjectPointer(frm, MainFormStart), gLookupDate);

            // popup select day window
            DoDateSelect();
            handled = true;
            break;

        case MainFormPopupTrigger: 
        {
            if (SelectCategoryPopup(AddressDB, &gAddrCategory,
                                    MainFormAddrCategories, MainFormPopupTrigger,
                                    gPrefsR.addrCategory) ) {
                // changed
                gCurrentSelection = -1;
                RereadHappyDaysDB(gStartDate);
                MainFormLoadTable(frm, 0);
				FrmDrawForm(frm);
            }

            handled = true;
            
            break;
        }

		case MainFormName:
		{
			Boolean change = true;
			if (gPrefsR.Prefs.sort == 0) change = false;
			else gPrefsR.Prefs.sort = 0;	// sort by name

			if (change) {
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
				handled = true;
			}

			break;
		}
		case MainFormDate:
		{
			Boolean change = true;
			if (gPrefsR.Prefs.sort == 1) change = false;
			else gPrefsR.Prefs.sort = 1; 	// sort by date

			if (change) {
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
				handled = true;
			}
			break;

		}
		case MainFormAge: 
		{
			Boolean change = true;

			if (gPrefsR.Prefs.sort == 2) 
				gPrefsR.Prefs.sort = 3;  	// sort by age(re)
			else gPrefsR.Prefs.sort = 2; 	// sort by age

			if (change) {
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
				handled = true;
			}
			break;
		}

        default:
            break;
                
        }
        break;

    case keyDownEvent: 
    {
		Int16		valueP, minP, maxP, pageSizeP;

		ScrollBarPtr barP = GetObjectPointer(frm, MainFormScrollBar);
		SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

		switch (e->data.keyDown.chr) {
		case vchrPageUp:
			gCurrentSelection = -1;			
			MainFormScrollLines(-pageSizeP, true);
			handled = true;
			break;
		
		case vchrPageDown:
			gCurrentSelection = -1;			
			MainFormScrollLines(pageSizeP, true);
			handled = true;
			break;

		case vchrPrevField:
			HandlePrevKey();
			handled = true;
			break;

		case vchrNextField:
			HandleNextKey();
			handled = true;
			break;

		case chrCarriageReturn:
			if (gCurrentSelection != -1) {
				MainFormHandleSelect(frm, gCurrentSelection);
			}
			handled = true;
			break;
        }
        break;
    }

    case tblSelectEvent: 
    {
 		MainFormHandleSelect(frm, e->data.tblSelect.row);
		break;
    }
    case tblEnterEvent: 
    {
        if (gCurrentSelection != -1)
            MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, false);
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
    ListPtr lstdate, lstauto, lstnotify, lstaddr;

    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return;
    
    CtlSetValue(GetObjectPointer(frm, PrefFormOverrideSystemDate),
                gPrefsR.Prefs.sysdateover);

    lstdate = GetObjectPointer(frm, PrefFormDateFmts);
    LstSetSelection(lstdate, gPrefsR.Prefs.dateformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormDateTrigger),
                LstGetSelectionText(lstdate, gPrefsR.Prefs.dateformat));

    lstauto = GetObjectPointer(frm, PrefFormAutoRescan);
    LstSetSelection(lstauto, gPrefsR.Prefs.autoscan);
    CtlSetLabel(GetObjectPointer(frm, PrefFormRescanTrigger),
                LstGetSelectionText(lstauto, gPrefsR.Prefs.autoscan));

    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    LstSetSelection(lstnotify, gPrefsR.Prefs.notifyformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormNotifyTrigger),
                LstGetSelectionText(lstnotify, gPrefsR.Prefs.notifyformat));
    
    lstaddr = GetObjectPointer(frm,PrefFormAddress);
    LstSetSelection(lstaddr, gPrefsR.Prefs.addrapp);
    CtlSetLabel(GetObjectPointer(frm,PrefFormAddrTrigger),
                LstGetSelectionText(lstaddr,gPrefsR.Prefs.addrapp));
                    
    SetFieldTextFromStr(PrefFormCustomField, gPrefsR.Prefs.custom);
/*
    SetFieldTextFromStr(PrefFormNotifyWith, gPrefsR.Prefs.notifywith);
*/
    if (gPrefsR.Prefs.scannote == 1) {
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
    ListPtr lstdate, lstnotify, lstaddr;
    Boolean newoverride;
    DateFormatType newdateformat;
    Boolean needrescan = 0;
    
    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return needrescan;
    if (FldDirty(GetObjectPointer(frm, PrefFormCustomField))) {
        needrescan = 1;
        if (FldGetTextPtr(GetObjectPointer(frm, PrefFormCustomField))) {
            StrNCopy(gPrefsR.Prefs.custom,
                     FldGetTextPtr(GetObjectPointer(frm,
                                                    PrefFormCustomField)), 12);
        }
    }
/*    
    if (FldDirty(GetObjectPointer(frm, PrefFormNotifyWith))) {
        if (FldGetTextPtr(GetObjectPointer(frm, PrefFormNotifyWith))) {
            StrNCopy(gPrefsR.Prefs.notifywith,
                     FldGetTextPtr(GetObjectPointer(frm,
                                                    PrefFormNotifyWith)), 5);
        }
    }
*/
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
        if (gPrefsR.Prefs.sysdateover && gPrefdfmts != gSystemdfmts)
        {
            needrescan = 1;
            gPrefdfmts = gSystemdfmts;    // revert to system date format
        }
    }
    gPrefsR.Prefs.sysdateover = newoverride;
    gPrefsR.Prefs.dateformat = newdateformat;

    // notify string
    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    gPrefsR.Prefs.notifyformat = LstGetSelection(lstnotify);

    // automatic scan of Address
    lstaddr = GetObjectPointer(frm, PrefFormAutoRescan);
    gPrefsR.Prefs.autoscan = LstGetSelection(lstaddr);


    // addr goto application id
    lstaddr = GetObjectPointer(frm, PrefFormAddress);
    gPrefsR.Prefs.addrapp = LstGetSelection(lstaddr);
    
    ptr = GetObjectPointer(frm, PrefFormScanNote);
    if (CtlGetValue(ptr) != gPrefsR.Prefs.scannote ) {
        needrescan = 1;
    }
    gPrefsR.Prefs.scannote = CtlGetValue(ptr);

    return needrescan;
}

static void LoadDispPrefsFields()
{
    FormPtr frm;
    
    if ((frm = FrmGetFormPtr(DispPrefForm)) == 0) return;

    if (gPrefsR.DispPrefs.emphasize == 1) {
        CtlSetValue(GetObjectPointer(frm, DispPrefEmphasize), 1);
    }
	else {
        CtlSetValue(GetObjectPointer(frm, DispPrefEmphasize), 0);
	}
    if (gPrefsR.DispPrefs.extrainfo == 1) {
        CtlSetValue(GetObjectPointer(frm, DispPrefExtraInfo), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, DispPrefExtraInfo), 0);
    }
    if (gPrefsR.DispPrefs.zodiac == 1) {
        CtlSetValue(GetObjectPointer(frm, DispPrefZodiac), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, DispPrefZodiac), 0);
    }
}

static Boolean UnloadDispPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    Boolean redraw = 0;

    if ((frm = FrmGetFormPtr(DispPrefForm)) == 0) return redraw;

    ptr = GetObjectPointer(frm, DispPrefEmphasize);
    if (gPrefsR.DispPrefs.emphasize != CtlGetValue(ptr)) {
        redraw = 1;
    }
    gPrefsR.DispPrefs.emphasize = CtlGetValue(ptr);
    
    ptr = GetObjectPointer(frm, DispPrefExtraInfo);
    gPrefsR.DispPrefs.extrainfo = CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, DispPrefZodiac);
    gPrefsR.DispPrefs.zodiac = CtlGetValue(ptr);

    return redraw;
}

static Boolean DispPrefFormHandleEvent(EventPtr e)
{
    Boolean redraw;
    Boolean handled = false;
    FormPtr frm;
    
    switch (e->eType) {
    case frmOpenEvent:
        frm = FrmGetFormPtr(DispPrefForm);
        if(gbVgaExists)
       		VgaFormModify(frm, vgaFormModify160To240);

        LoadDispPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case DispPrefOk:
            redraw = UnloadDispPrefsFields();
            
            FrmReturnToForm(0);
            
            if (redraw) FrmUpdateForm(MainForm, frmRedrawUpdateCode);

            handled = true;
            break;
        case DispPrefCancel:
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

static Boolean PrefFormHandleEvent(EventPtr e)
{
    Boolean rescan;
    Boolean handled = false;
    FormPtr frm;
    
    switch (e->eType) {
    case frmOpenEvent:
        frm = FrmGetFormPtr(PrefForm);
        if(gbVgaExists)
       		VgaFormModify(frm, vgaFormModify160To240);


        LoadPrefsFields();
        FrmDrawForm(frm);
        handled = true;
        break;

        
    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case PrefFormOk:
            rescan = UnloadPrefsFields();
            
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


static void MainTableSelectItem(TablePtr table, Int16 row, Boolean select)
{
	RectangleType r;

	TblGetItemBounds(table, row, 0, &r);

    if (!select) {
        WinEraseRectangle(&r, 0);
        MainFormDrawRecord(table, row, 0, &r);
    }
    else WinInvertRectangle(&r, 0);
}

static void MainFormDrawRecord(MemPtr tableP, Int16 row, Int16 column, 
                               RectanglePtr bounds)
{
    FontID currFont;
    MemHandle recordH = 0;
    Int16 x, y;
    UInt16 nameExtent;
    PackedHappyDays* rp;
    HappyDays r;
    LineItemPtr ptr;
    LineItemType drawRecord;
	Boolean ignored = true;
	FormPtr frm;
	ScrollBarPtr barP;
	Int16	valueP, minP, maxP, pageSizeP;
    
	frm = FrmGetActiveForm();
	WinEraseRectangle(bounds, 0);

	barP = GetObjectPointer(frm, MainFormScrollBar);
	SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);
	row += valueP;

	if (row >= gMainTableTotals) return;
    /*
     * Set the standard font.  Save the current font.
     * It is a Pilot convention to not destroy the current font
     * but to save and restore it.
     */
    currFont = FntSetFont(gPrefsR.listFont);

    x = bounds->topLeft.x;
    y = bounds->topLeft.y;

    ptr = MemHandleLock(gTableRowHandle);
    drawRecord = ptr[row];
    
    if ((recordH =
         DmQueryRecord(MainDB, drawRecord.birthRecordNum))) {
		Int16 width, length;
        Char* eventType;
		Int16 categoryWidth = FntCharWidth('W');
		Int16 ageFieldWidth = FntCharsWidth("100", 3);

        rp = (PackedHappyDays *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackHappyDays(&r, rp);

		// Display date(converted date)
		// 
		if (drawRecord.date.year != INVALID_CONV_DATE) {
			DateToAscii(drawRecord.date.month, drawRecord.date.day,
						drawRecord.date.year+ 1904, gPrefdfmts, gAppErrStr);
		}
		else {
			StrCopy(gAppErrStr, "-");
		}
		width = bounds->extent.x - categoryWidth - ageFieldWidth;
		length = StrLen(gAppErrStr);
		FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
		WinDrawChars(gAppErrStr, length,
					 bounds->topLeft.x + bounds->extent.x 
                     - width - categoryWidth - ageFieldWidth, y);

		// Display name
		// 
		nameExtent = bounds->extent.x - width 
            - ageFieldWidth - categoryWidth -2;
		DrawRecordName(r.name1, r.name2, nameExtent, &x, y,
					   false,
					   r.flag.bits.priority_name1 || !gSortByCompany);

		// Display age
		if (drawRecord.age >= 0) {
			StrPrintF(gAppErrStr, "%d", drawRecord.age);
		}
		else {
			StrCopy(gAppErrStr, "  -");
		}

		width = ageFieldWidth;
		length = StrLen(gAppErrStr);
		FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
		WinDrawChars(gAppErrStr, length,
					 bounds->topLeft.x + bounds->extent.x 
                     - width - categoryWidth -1, y);

        // Display EventType string
        eventType = EventTypeString(r);

		width = categoryWidth;
		length = 1;
		FntCharsInWidth(eventType, &width, &length, &ignored);
		WinDrawChars(eventType, 1,
					 bounds->topLeft.x + bounds->extent.x 
                     - width - (categoryWidth - width)/2, y);

        if (gPrefsR.DispPrefs.emphasize 
            && drawRecord.date.year != INVALID_CONV_DATE
            && (r.flag.bits.lunar || r.flag.bits.lunar_leap)) {
            // draw gray line
            WinDrawGrayLine(bounds->topLeft.x,
                            bounds->topLeft.y + bounds->extent.y-1,
                            bounds->topLeft.x + bounds->extent.x-1,
                            bounds->topLeft.y + bounds->extent.y-1);
        }
        
        MemHandleUnlock(recordH);
    }

    MemPtrUnlock(ptr);

    /* Restore the font */
    FntSetFont (currFont);
}

static void ViewFormResize(FormPtr frmP, Boolean draw)
{
	Coord	x, y;
	Coord	y_diff;
	RectangleType r, erase_rect;

	WinGetDisplayExtent(&x, &y);

	FrmGetFormBounds(frmP, &r);

	y_diff = y - (r.topLeft.y + r.extent.y);

	if (draw && (y_diff <0)) {
		// if the siklscreen was maximized,
		// 		erase the area under maxmized silkscreen
		erase_rect = r;
		erase_rect.topLeft.y = r.extent.y + y_diff;
		erase_rect.extent.y = -y_diff;
		WinEraseRectangle(&erase_rect, 0);
	}

	// resize the form
	r.extent.y += y_diff;
	WinSetWindowBounds(FrmGetWindowHandle(frmP), &r);

	// move bottom controls button
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, ViewFormDone), y_diff, draw);
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, ViewFormNotifyDB), y_diff, draw);
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, ViewFormNotifyTD), y_diff, draw);
	PrvMoveObject(frmP, FrmGetObjectIndex(frmP, ViewFormGoto), y_diff, draw);

    // Resize the table
	PrvResizeObject(frmP, FrmGetObjectIndex(frmP, ViewFormTable), 
					y_diff, draw);

	if (draw) {
		FrmDrawForm(frmP);
	}
}

static void MainFormInitTable(FormPtr frm)
{
    TablePtr 		tableP;
	ScrollBarPtr 	barP;
	Int16 			tblRows, row, row_height;
	FontID			currFont;
    
    tableP = GetObjectPointer(frm, MainFormTable);
	tblRows = TblGetNumberOfRows(tableP); 

	currFont = FntSetFont(gPrefsR.listFont);
	row_height = FntLineHeight();

    for (row=0; row < tblRows; row++) {
		TblSetRowHeight(tableP, row, row_height);
		TblSetItemStyle(tableP, row, 0, customTableItem);
	}
	TblSetCustomDrawProcedure(tableP, 0, MainFormDrawRecord);

	barP = GetObjectPointer(frm, MainFormScrollBar);
    SclSetScrollBar(barP, 0, 0, 0, gMainTableTotals);

	FntSetFont(currFont);
}

static int CalcPageSize(FormPtr frm)
{
    TablePtr 		tableP;
	Int16 			tblRows, row_height;
	RectangleType	rect;
	FontID			currFont;

    tableP = GetObjectPointer(frm, MainFormTable);
	tblRows = TblGetNumberOfRows(tableP); 
	
	currFont = FntSetFont(gPrefsR.listFont);
	row_height = FntLineHeight();
	TblGetBounds(tableP, &rect);

	FntSetFont(currFont);

    return MIN(tblRows, rect.extent.y / row_height);
}

static void MainFormLoadTable(FormPtr frm, Int16 listOffset)
{
    TablePtr 		tableP;
	ScrollBarPtr 	barP;
	Int16 			tblRows, row, visibleRows;
	RectangleType	rect;
	Int16			running_total, row_height;
	FontID			currFont;

    tableP = GetObjectPointer(frm, MainFormTable);
	tblRows = TblGetNumberOfRows(tableP); 
	
    TblSetColumnUsable(tableP, 0, true);

	currFont = FntSetFont(gPrefsR.listFont);
	row_height = FntLineHeight();
	TblGetBounds(tableP, &rect);
	running_total = 0;
	visibleRows = 0;

    for (row=0; row < tblRows; row++) {
		TblSetRowHeight(tableP, row, row_height);
        TblSetItemFont(tableP, row, 0, gPrefsR.listFont);
	
		running_total += row_height;
		if (row < gMainTableTotals)
			TblSetRowUsable(tableP, row, running_total <= rect.extent.y);
		else 
			TblSetRowUsable(tableP, row, false);

		if (running_total <= rect.extent.y) 
			visibleRows++;

		TblMarkRowInvalid(tableP, row);
	}

    gMainTableStart = listOffset;

	barP = GetObjectPointer(frm, MainFormScrollBar);
	if (gMainTableTotals > visibleRows) {
		SclSetScrollBar(barP, listOffset, 0, 
                        gMainTableTotals - visibleRows, visibleRows);
    }
	else {
		SclSetScrollBar(barP, 0, 0, 0, gMainTableTotals);
    }

	FntSetFont(currFont);
}

static void MainFormInit(FormPtr frm, Boolean bformModify)
{
    // table에 대한 기본 설정
    //
    MainFormInitTable(frm);
    
	// display starting date
	if (gbVgaExists) {
		VgaTableUseBaseFont(GetObjectPointer(frm, MainFormTable), 
							!VgaIsVgaFont(gPrefsR.listFont));
		if (bformModify) VgaFormModify(frm, vgaFormModify160To240);
		MainFormResize(frm, false);
	}

	DisplayCategory(MainFormPopupTrigger, gPrefsR.addrCategory, false);
	DateToAsciiLong(gStartDate.month, gStartDate.day, 
					gStartDate.year+1904, gPrefdfmts, gLookupDate);
	CtlSetLabel(GetObjectPointer(frm, MainFormStart), gLookupDate);

    if (gPrefsR.Prefs.sort == 0) {
        CtlSetValue(GetObjectPointer(frm, MainFormName), 1);
    }
    else if (gPrefsR.Prefs.sort == 1) {
        CtlSetValue(GetObjectPointer(frm, MainFormDate), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, MainFormAge), 1);
    }
}


static void ViewTableDrawHdr(MemPtr tableP, Int16 row, Int16 column, 
                             RectanglePtr bounds)
{
    FontID currFont;
    Int16 length, width;
	Boolean ignored = true;
    Int16 viewItem;

	WinEraseRectangle(bounds, 0);
    
    if (gbVgaExists) {
        currFont = FntSetFont(VgaBaseToVgaFont(stdFont));
    }
    else {
        currFont = FntSetFont(stdFont);
    }

    viewItem = TblGetItemInt(tableP, row, column);

    switch (viewItem) {
    case ViewType:
        ///////////////////////////////////
        // Display the Type of Anniversary
        ///////////////////////////////////
        SysCopyStringResource(gAppErrStr, TypeStr);
        break;
    case ViewNext:
        ///////////////////////////////////
        // Display the Next Date
        ///////////////////////////////////
        SysCopyStringResource(gAppErrStr, NextDateStr);
        
        break;
    case ViewSrc:
        ///////////////////////////////////
        // Display the Source Date
        ///////////////////////////////////
        SysCopyStringResource(gAppErrStr, SourceDateStr);
        
        break;
        
    case ViewAge:
        ///////////////////////////////////
        // Process Age
        ///////////////////////////////////
        SysCopyStringResource(gAppErrStr, AgeStr);

        break;
    case ViewRemained:
        SysCopyStringResource(gAppErrStr, RemainedDayStr);

        break;
    default:
        gAppErrStr[0] = 0;
        break;
    }
    
    length = StrLen(gAppErrStr);
    width = bounds->extent.x;
    FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
    WinDrawChars(gAppErrStr, length, bounds->topLeft.x, bounds->topLeft.y);

    // restore font
    FntSetFont (currFont);
}

static void ViewTableDrawData(MemPtr tableP, Int16 row, Int16 column, 
                              RectanglePtr bounds)
{
    FontID currFont;
    
    MemHandle recordH = 0;
    LineItemPtr ptr;
	Boolean ignored = true;
    Int16 index;
    DateType converted;
    Int8 age;

    PackedHappyDays* rp;
    HappyDays r;
	Int16 dateDiff;
	DateType current;
	char displayStr[25];

    Int16 x,y;
    Int16 width, length;
    Int16 viewItem;

    WinEraseRectangle(bounds, 0);
    // current time
    DateSecondsToDate(TimGetSeconds(), &current);

    if (gbVgaExists) {
        currFont = FntSetFont(VgaBaseToVgaFont(stdFont));
    }
    else {
        currFont = FntSetFont(stdFont);
    }

    // Read the necessary information from LineItem
    //
    ptr = MemHandleLock(gTableRowHandle);
    index = ptr[gMainTableHandleRow].birthRecordNum;
    converted = ptr[gMainTableHandleRow].date;
    age = ptr[gMainTableHandleRow].age;
    MemPtrUnlock(ptr);

    recordH = DmQueryRecord(MainDB, index);
    ErrFatalDisplayIf(!recordH, "Invalid record");
        
    x = bounds->topLeft.x;
    y = bounds->topLeft.y;
    width = bounds->extent.x;

    viewItem = TblGetItemInt(tableP, row, column);
    rp = (PackedHappyDays *) MemHandleLock(recordH);
    UnpackHappyDays(&r, rp);

    switch (viewItem) {
    case ViewType: 
    {
        ///////////////////////////////////
        // Display the Type of Anniversary
        ///////////////////////////////////
        char* p;
                
        if (!r.custom[0]) {         // custom does not exist
            p = gPrefsR.Prefs.custom;
        }
        else {
            p = r.custom;
        }
        length = StrLen(p);
        FntCharsInWidth(p, &width, &length, &ignored);
        WinDrawChars(p, length, x, y);
    }
    break;
    case ViewNext:
    {
        ///////////////////////////////////
        // Display the Next Date
        ///////////////////////////////////
        
        char temp[5];
        
        if (converted.year != INVALID_CONV_DATE) {
            DateToAsciiLong(converted.month, converted.day,
                            converted.year+ 1904, gPrefdfmts,
                            gAppErrStr);

            SysCopyStringResource(temp,
                                  DayOfWeek(converted.month, converted.day,
                                            converted.year+1904) + SunString);
            StrNCat(gAppErrStr, " [", AppErrStrLen);
            StrNCat(gAppErrStr, temp, AppErrStrLen);
            StrNCat(gAppErrStr, "]", AppErrStrLen);
        }
        else {
            SysCopyStringResource(gAppErrStr, ViewNotExistString);
        }
        length = StrLen(gAppErrStr);
        FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
        WinDrawChars(gAppErrStr, length, x, y);
    }
    break;
    case ViewSrc:
    {
        ///////////////////////////////////
        // Display the Source Date
        ///////////////////////////////////
            
        if (r.flag.bits.lunar) {
            StrCopy(displayStr, "-)");
        }
        else if (r.flag.bits.lunar_leap) {
            StrCopy(displayStr, "#)");
        }
        else displayStr[0] = 0;

        if (r.flag.bits.year) {
            DateToAsciiLong(r.date.month, r.date.day, r.date.year + 1904,
                            gPrefdfmts, gAppErrStr);
        }
        else {
            DateToAsciiLong(r.date.month, r.date.day, -1, gPrefdfmts,
                            gAppErrStr);
        }
        StrCat(displayStr, gAppErrStr);

        length = StrLen(displayStr);
        FntCharsInWidth(displayStr, &width, &length, &ignored);
        WinDrawChars(displayStr, length, x, y);
    }
    break;
    case ViewAge:
    {
        ///////////////////////////////////
        // Process Age
        ///////////////////////////////////
        
        if (r.flag.bits.year) {
            DateType solBirth;
            DateTimeType rtVal;
            UInt8 ret;
            Int16 d_diff = 0, m_diff = 0, y_diff = 0;
            
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
            
            dateDiff = (Int16)(DateToDays(current)
                               - DateToDays(solBirth));

            if (dateDiff > (Int16)0) {
                if (current.day < solBirth.day) {
                    d_diff = DaysInMonth(solBirth.month, solBirth.year+1904)
                        + current.day- solBirth.day;
                    m_diff--;
                }
                else {
                    d_diff = current.day - solBirth.day;
                }
                        
                if (current.month + m_diff  < solBirth.month) {
                    m_diff = 12 + current.month - solBirth.month;
                    y_diff--;
                }
                else {
                    m_diff = current.month - solBirth.month;
                }
                y_diff += current.year - solBirth.year;
                    
                StrPrintF(gAppErrStr, "%dY %dM %dD (%d)",
                          y_diff, m_diff, d_diff, dateDiff);
            }
            else StrCopy(gAppErrStr, "-");
        }
        else {
            StrCopy(gAppErrStr, "-");
        }
        length = StrLen(gAppErrStr);
        FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
        WinDrawChars(gAppErrStr, length, x, y);
    }
    break;
    case ViewRemained:
    {
        dateDiff = DateToDays(converted) - DateToDays(current);

        if (dateDiff >= (Int16)0) {
            SysCopyStringResource(displayStr, BirthLeftString);
            StrPrintF(gAppErrStr, displayStr, dateDiff);
        }
        else {
            SysCopyStringResource(displayStr, BirthPassedString);
            StrPrintF(gAppErrStr, displayStr, -1 * dateDiff);
        }
            
        length = StrLen(gAppErrStr);
        FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
        WinDrawChars(gAppErrStr, length, x, y);
    }
    
    break;
    }
    MemHandleUnlock(recordH);

    // restore font
    FntSetFont (currFont);
}
static void ViewFormInitTable(FormPtr frm)
{
    TablePtr 		tableP;
    Int16 			tblRows, row;
    Int16			row_height;
	FontID			currFont;

    tableP = GetObjectPointer(frm, ViewFormTable);
	tblRows = TblGetNumberOfRows(tableP); 

    if (gbVgaExists) {
        currFont = FntSetFont(VgaBaseToVgaFont(stdFont));
    }
    else {
        currFont = FntSetFont(stdFont);
    }
    
	row_height = FntLineHeight();

    for (row=0; row < tblRows; row++) {
		TblSetRowHeight(tableP, row, row_height);
        
		TblSetItemStyle(tableP, row, 0, customTableItem);
		TblSetItemStyle(tableP, row, 1, customTableItem);
	}
	TblSetCustomDrawProcedure(tableP, 0, ViewTableDrawHdr);
	TblSetCustomDrawProcedure(tableP, 1, ViewTableDrawData);

	FntSetFont(currFont);
}

static void ViewFormLoadTable(FormPtr frm)
{
    TablePtr 		tableP;
	Int16 			tblRows, row;
	Int16			row_height;
	FontID			currFont;
    Int16           tblItemIdx = ViewType;

    tableP = GetObjectPointer(frm, ViewFormTable);
	tblRows = TblGetNumberOfRows(tableP); 
	
    TblSetColumnUsable(tableP, 0, true);
    TblSetColumnUsable(tableP, 1, true);

    if (gbVgaExists) {
        currFont = FntSetFont(VgaBaseToVgaFont(stdFont));
    }
    else {
        currFont = FntSetFont(stdFont);
    }

	row_height = FntLineHeight();

    for (row=0; row < tblRows; row++) {
        TblSetRowHeight(tableP, row, row_height);
        
        TblSetItemInt(tableP, row, 0, tblItemIdx);
        TblSetItemInt(tableP, row, 1, tblItemIdx);

        TblSetRowSelectable(tableP, row, false);

        if (tblItemIdx == ViewSrc && !gPrefsR.DispPrefs.extrainfo) {
            tblItemIdx += 2;
        }
        else {
            tblItemIdx++;
        }
        
        TblSetRowUsable(tableP, row, true);

		TblMarkRowInvalid(tableP, row);
	}
	FntSetFont(currFont);
}

static void ViewFormSetInfo(FormPtr frm)
{
    UInt16 addrattr;
    LineItemPtr ptr;
    MemHandle recordH = 0;
    Int16 index;
    PackedHappyDays* rp;
    HappyDays r;
    // RectangleType rect;
    
    // Read the necessary information from LineItem
    //
    ptr = MemHandleLock(gTableRowHandle);
    index = ptr[gMainTableHandleRow].birthRecordNum;
    MemPtrUnlock(ptr);

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
        
    // FrmSetCategoryLabel(frm, FrmGetObjectIndex(frm, ViewFormCategory),
    //                    gAppErrStr);
    // error? (suggested by Mike McCollister)

    CategoryGetName(AddressDB, addrattr, gAppErrStr);
    SetFieldTextFromStr(ViewFormCategory, gAppErrStr);

    if ((recordH = DmQueryRecord(MainDB, index))) {
        rp = (PackedHappyDays *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackHappyDays(&r, rp);
//        FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, ViewFormName), &rect);
        
//        DrawRecordName(r.name1, r.name2, rect.extent.x,
//                       &rect.topLeft.x, rect.topLeft.y,
//                       false,
//                       r.flag.bits.priority_name1 || !gSortByCompany);

        if (r.name1[0] && r.name2[0])
            StrPrintF(gAppErrStr, "%s, %s", r.name1, r.name2);
        else if (r.name1[0] && !r.name2[0]) 
            StrPrintF(gAppErrStr, "%s", r.name1);
        else if (r.name2[0] && !r.name1[0])
            StrPrintF(gAppErrStr, "%s", r.name2);
        else gAppErrStr[0] = 0;
        
        SetFieldTextFromStr(ViewFormName, gAppErrStr);
        MemHandleUnlock(recordH);
    }
}

static void ViewFormInit(FormPtr frm, Boolean bformModify)
{
    ViewFormInitTable(frm);
	// display starting date
	if (gbVgaExists) {
		VgaTableUseBaseFont(GetObjectPointer(frm, ViewFormTable), false);
		if (bformModify) VgaFormModify(frm, vgaFormModify160To240);
		ViewFormResize(frm, false);
	}

	ViewFormLoadTable(frm);
    ViewFormSetInfo(frm);
}

static Boolean ViewFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    Boolean bformModify = true;
    FormPtr frm = FrmGetFormPtr(ViewForm);

    switch (e->eType) {
    case frmUpdateEvent:
        bformModify = false;
    case frmOpenEvent: {
        ViewFormInit(frm, bformModify);
        FrmDrawForm(frm);
        
        handled = true;
        break;
    }
	case keyDownEvent:
	{
		if (EvtKeydownIsVirtual(e)) {
            // TablePtr    tableP = GetObjectPointer(frm, ViewFormTable);

            // Before processing the scroll, be sure that the command bar has
            // been closed.
            MenuEraseStatus (0);
    
            switch (e->data.keyDown.chr) {
            case vchrPageDown:
            case vchrNextField:
                if (gMainTableHandleRow < gMainTableTotals-1) {
                    gMainTableHandleRow++;
        
                    SndPlaySystemSound(sndInfo);

//                    ViewFormSetInfo(frm);
//                    ViewFormLoadTable(frm);
//                    FrmDrawForm(FrmGetFormPtr(ViewForm));

                    handled = true;
                }
                break;
            case vchrPageUp:    
            case vchrPrevField:
                if (gMainTableHandleRow > 0 ) {
                    gMainTableHandleRow--;

                    SndPlaySystemSound(sndInfo);

//                    ViewFormSetInfo(frm);
//                    ViewFormLoadTable(frm);
//                    FrmDrawForm(FrmGetFormPtr(ViewForm));


                    handled = true;
                }
                break;
            case chrEscape:
            case chrCarriageReturn:
                FrmGotoForm(MainForm);
                handled = true;
            }
		}
        break;
    }
	case displayExtentChangedEvent:
		ViewFormResize(FrmGetActiveForm(), true);
		break;
    
    case menuEvent: 
    {
        handled = MenuHandler(frm, e);
		break;
	}

    case ctlSelectEvent:
        switch(e->data.ctlSelect.controlID) {
        case ViewFormDone:
            FrmGotoForm(MainForm);
                
            handled = true;
            break;
        case ViewFormNotifyDB:
            //  Notify event into the datebook entry
            //
            gPrefsR.records = 1;
            FrmPopupForm(DateBookNotifyForm);

            handled = true;
            break;

        case ViewFormNotifyTD:
            //  Notify event into the datebook entry
            //
            gPrefsR.records = 1;
            FrmPopupForm(ToDoNotifyForm);

            handled = true;
            break;
            
        case ViewFormGoto:
        {
            MemHandle recordH = 0;
            PackedHappyDays* rp;
            UInt32 gotoIndex;
            
            LineItemPtr ptr = MemHandleLock(gTableRowHandle);
            
            if ((recordH =
                 DmQueryRecord(MainDB,
                               ptr[gMainTableHandleRow].birthRecordNum))) {
                rp = (PackedHappyDays *) MemHandleLock(recordH);
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
        break;  
    default:
        break;
    }

    return handled;
}

static Boolean StartFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    Boolean rescan;
    
    FormPtr frm = FrmGetFormPtr(StartForm);

    // no event procedure
    //
    switch (e->eType) {
	case frmUpdateEvent:
    case frmOpenEvent:
        if(gbVgaExists) 
       		VgaFormModify(frm, vgaFormModify160To240);

        if (!FindHappyDaysField()) {
            char ErrStr[200];

            SysCopyStringResource(ErrStr, CantFindAddrString);
            StrPrintF(gAppErrStr, ErrStr, gPrefsR.Prefs.custom);

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
            rescan = false;
            
            if (IsChangedAddressDB()) {
                switch (gPrefsR.Prefs.autoscan) {
                case 0: // always
                    rescan = true;
                    break;
                case 1: // ask
                    switch (FrmCustomAlert(AddrRescanAlert,
                                           gAppErrStr, " ", " ")) {
                    case 0:             // Yes
                        rescan = true;
                        break;
                    case 1:             // No
                    }
                    break;
                case 2: // no
                    break;
                case 3:
                    rescan = true;      // trick, force rescan
                    
                    gPrefsR.Prefs.autoscan = 2;
                    break;
                }
                if (rescan) UpdateHappyDaysDB(frm);
                SetReadAddressDB();     // mark addressDB is read
            }
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
