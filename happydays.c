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
#define frmUpdateFontCode (frmRedrawUpdateCode + 2)

////////////////////////////////////////////////////////////////////
// 	Global variable
////////////////////////////////////////////////////////////////////

MemHandle gTableRowHandle;

Int16 gBirthDateField;
Char gAppErrStr[AppErrStrLen];
Char gLookupDate[12];			// date string of look up
Char gDateBk3Icon[52][8];      	// dateBk3 icon string
Boolean gDateBk3IconLoaded = false;     // datebk3 icon loaded
UInt32 gAdcdate, gAdmdate;      // AddressBook create/modify time
UInt32 gMmcdate, gMmmdate;      // Memo create/modify time
Boolean gSortByCompany=true;    // sort by company is set in AddressBook?
UInt16 gAddrCategory;           // address book category
UInt16 gToDoCategory;           // todo category
Int16 gMainTableTotals;       	// Main Form table total rows;
Int16 gMainTableHandleRow = -1; // The row of birthdate view to display
Int16 gCurrentSelection = -1;   // current selection of table
Int16 gLastStart = 0;           // Last table start
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


enum ViewFormType { ViewName = 0,
                    ViewType,
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
static Boolean ToDoFormHandleEvent(EventPtr e);

static void ViewFormLoadTable(FormPtr frm, Int16 listOffset);

static Boolean MenuHandler(FormPtr frm, EventPtr e);

static Boolean StartFormHandleEvent(EventPtr e);
static Boolean PrefFormHandleEvent(EventPtr e);
static Boolean ViewFormHandleEvent(EventPtr e);
static Boolean MainFormHandleEvent(EventPtr e);
static Boolean DBNotifyFormHandleEvent(EventPtr e);
static Boolean DBNotifyFormMoreHandleEvent(EventPtr e);

static void HighlightMatchRowDate(DateTimeType inputDate);
static void HighlightMatchRowName(Char first);

static void WritePrefsRec(void);
static Boolean SelectCategoryPopup(DmOpenRef dbP, UInt16* selected,
                                   UInt32 list, UInt32 trigger, Char *string);
static void MainFormLoadTable(FormPtr frm, Int16 listOffset);
static void MainFormInit(FormPtr formP, Boolean resize);
static void MainFormResize(FormPtr frmP, Boolean draw);
static void MainFormSelectTableItem(Boolean selected, TablePtr table,
                                    Int16 row, Int16 column);
static void MainFormDrawRecord(MemPtr tableP, Int16 row, Int16 column, 
                               RectanglePtr bounds);

static void PrvMoveObject(FormPtr frmP, UInt16 objIndex, 
                          Coord y_diff, Boolean draw);
static void PrvResizeObject(FormPtr frmP, UInt16 objIndex, 
                            Coord y_diff, Boolean draw);

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

    if(gbVgaExists)
    	VgaSetScreenMode(screenMode1To1, rotateModeNone);

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
		gPrefsR.viewFont = (gbVgaExists) ? VgaBaseToVgaFont(stdFont) 
            : stdFont;
        if (gPrefsR.BirthPrefs.sysdateover)
            gPrefdfmts = gPrefsR.BirthPrefs.dateformat;
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
  gPrefsR.BirthPrefs.sort = 0;     // sort by name
  SndPlaySystemSound(sndClick);

  gMainTableStart = 0;
  FrmUpdateForm(MainForm, frmRedrawUpdateCode);
  return true;
  }
  else if (keyboardNumericChr == chr) {
  gPrefsR.BirthPrefs.sort = 1;     // sort by date
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
static void DisplayCategory(UInt32 trigger, Char* string, Boolean redraw)
{
    ControlPtr ctl;
    FontID currFont;
    UInt16 categoryLen;
    FormPtr frm = FrmGetActiveForm();

	if (gbVgaExists) 
		currFont = VgaBaseToVgaFont(stdFont);
	else 
		currFont = stdFont;

    categoryLen = StrLen(string);
    
    ctl = GetObjectPointer(frm, trigger);
    if ( redraw ) CtlEraseControl(ctl);

    CtlSetLabel(ctl, string);

    if (redraw) CtlDrawControl(ctl);
    
    FntSetFont (currFont);
}

static void RereadHappyDaysDB(DateType start)
{
    gMainTableTotals = AddrGetBirthdate(MainDB, gAddrCategory, start);
}

static void HighlightAction(int selected, Boolean sound)
{
/*
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
*/
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

		gCurrentSelection = -1;
		FrmUpdateForm(MainForm, frmRedrawUpdateCode);
    }
}

static Boolean IsHappyDaysRecord(Char* notefield)
{
    if (notefield && StrStr(notefield, gPrefsR.BirthPrefs.notifywith)) {
        return true; 
    }
    return false;
}

// check if description has the information about name1 and name2
//
static Boolean IsSameRecord(Char* notefield, BirthDate birth)
{
    Char *p;
    
    if (notefield && (p = StrStr(notefield,gPrefsR.BirthPrefs.notifywith))) {
        p += StrLen(gPrefsR.BirthPrefs.notifywith);

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

// temporary return value;
static Char* EventTypeString(BirthDate r)
{
	static char tmpString[4];
    if (!r.custom[0]) {
        tmpString[0] = gPrefsR.BirthPrefs.custom[0];
    }
    else tmpString[0] = r.custom[0];
    tmpString[1] = 0;

    StrToLower(&tmpString[3], tmpString);     // use temporary
    tmpString[3] = tmpString[3] - 'a' + 'A';  // make upper

    return &tmpString[3];
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
    Boolean 	handled = false;
	static Char tmpString[25];

	// TablePtr tableP = GetObjectPointer(frm, MainFormTable);
	// TblUnhighlightSelection(tableP);

    MenuEraseStatus(NULL);
    
    switch(e->data.menu.itemID) {
    case MainFormMenuAbout: 
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

	case MainFormMenuFont: {
		FontID fontID;

		fontID = FontSelect(gPrefsR.listFont);

		if (fontID != gPrefsR.listFont) {
			gPrefsR.listFont = fontID;
			FrmUpdateForm(MainForm, frmUpdateFontCode);
		}
		handled = true;
		break;
	}

    case MainFormMenuPref:
        FrmPopupForm(PrefForm);
            
        handled = true;
        break;

    case MainFormMenuNotifyDatebook:

        // Select Records to all and select is invalid
        //
        gPrefsR.records = 3;
        
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
        gPrefsR.records = 3;
        
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

	if (draw) {
		barP = GetObjectPointer(frmP, MainFormScrollBar);
		SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

		MainFormLoadTable(frmP, valueP);
		FrmDrawForm(frmP);
		gCurrentSelection = -1;
	}
}

static void PrvMoveObject(FormPtr frmP, UInt16 objIndex, 
                          Coord y_diff, Boolean draw)
{
    RectangleType r;

	FrmGetObjectBounds(frmP, objIndex, &r);
	if (draw) {
		RctInsetRectangle(&r, -2);   //need to erase the frame as well
		WinEraseRectangle(&r, 0);
		RctInsetRectangle(&r, 2);
	}
	r.topLeft.y += y_diff;
	FrmSetObjectBounds(frmP, objIndex, &r);
}

static void PrvResizeObject(FormPtr frmP, UInt16 objIndex, 
                            Coord y_diff, Boolean draw)
{
    RectangleType r;

	FrmGetObjectBounds(frmP, objIndex, &r);
	if (draw) WinEraseRectangle(&r, 0);
	r.extent.y += y_diff;
	FrmSetObjectBounds(frmP, objIndex, &r);
}

/*
static UInt16 PrvFrmGetGSI(FormPtr frmP)
{
    UInt16 i, numObjects;

	numObjects = FrmGetNumberOfObjects(frmP);

	for (i=0; i<numObjects; i++)
	{
		if (FrmGetObjectType(frmP, i) == frmGraffitiStateObj)
			return(i);
	}
	return(-1);
}
*/

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
			MainFormSelectTableItem(false, GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, 0);
            gCurrentSelection --;
		}
        else {
            if (valueP > minP) MainFormScrollLines(-1, true);
        }
    }

	MainFormSelectTableItem(true, GetObjectPointer(frm, MainFormTable),
                            gCurrentSelection, 0);
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
			MainFormSelectTableItem(false, GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, 0);

            gCurrentSelection ++;
		}
        else {
            if (valueP <  maxP) MainFormScrollLines(1, true);
        }
    }

	MainFormSelectTableItem(true, GetObjectPointer(frm, MainFormTable),
                            gCurrentSelection, 0);
}


static void MainFormHandleSelect(FormPtr frm, Int16 row)
{
	ScrollBarPtr barP;
	Int16        valueP, minP, maxP, pageSizeP;

	barP = GetObjectPointer(frm, MainFormScrollBar);
	SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

	gCurrentSelection = row;
	gMainTableHandleRow = gCurrentSelection + valueP;
    gLastStart = valueP;

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
        gCurrentSelection = -1;

    case frmOpenEvent: 
    {
        MainFormInit(frm, bformModify);

        // 실제 읽어들일 데이터를 처리
        //
        // gLastStart is set in 'MainFormHandleSelect'
        MainFormLoadTable(frm, gLastStart);

        FrmDrawForm(frm);
        if (gCurrentSelection != -1) {
            MainFormSelectTableItem(true, GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, 0);
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
			if (gPrefsR.BirthPrefs.sort == 0) change = false;
			else gPrefsR.BirthPrefs.sort = 0;	// sort by name

			if (change) {
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
				handled = true;
			}

			break;
		}
		case MainFormDate:
		{
			Boolean change = true;
			if (gPrefsR.BirthPrefs.sort == 1) change = false;
			else gPrefsR.BirthPrefs.sort = 1; 	// sort by date

			if (change) {
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
				handled = true;
			}
			break;

		}
		case MainFormAge: 
		{
			Boolean change = true;

			if (gPrefsR.BirthPrefs.sort == 2) 
				gPrefsR.BirthPrefs.sort = 3;  	// sort by age(re)
			else gPrefsR.BirthPrefs.sort = 2; 	// sort by age

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
            MainFormSelectTableItem(false, GetObjectPointer(frm, MainFormTable),
                                    gCurrentSelection, 0);
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
    ListPtr lstdate, lstnotify, lstaddr;

    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return;
    
    CtlSetValue(GetObjectPointer(frm, PrefFormOverrideSystemDate),
                gPrefsR.BirthPrefs.sysdateover);
    lstdate = GetObjectPointer(frm, PrefFormDateFmts);
    LstSetSelection(lstdate, gPrefsR.BirthPrefs.dateformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormDateTrigger),
                LstGetSelectionText(lstdate, gPrefsR.BirthPrefs.dateformat));

    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    LstSetSelection(lstnotify, gPrefsR.BirthPrefs.notifyformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormNotifyTrigger),
                LstGetSelectionText(lstnotify, gPrefsR.BirthPrefs.notifyformat));
    lstaddr = GetObjectPointer(frm,PrefFormAddress);
    LstSetSelection(lstaddr, gPrefsR.BirthPrefs.addrapp);
    CtlSetLabel(GetObjectPointer(frm,PrefFormAddrTrigger),
                LstGetSelectionText(lstaddr,gPrefsR.BirthPrefs.addrapp));
                    
    SetFieldTextFromStr(PrefFormCustomField, gPrefsR.BirthPrefs.custom);
    SetFieldTextFromStr(PrefFormNotifyWith, gPrefsR.BirthPrefs.notifywith);

    if (gPrefsR.DispPrefs.emphasize == 1) {
        CtlSetValue(GetObjectPointer(frm, PrefFormEmphasize), 1);
    }
	else {
        CtlSetValue(GetObjectPointer(frm, PrefFormEmphasize), 0);
	}
    if (gPrefsR.BirthPrefs.scannote == 1) {
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
            StrNCopy(gPrefsR.BirthPrefs.custom,
                     FldGetTextPtr(GetObjectPointer(frm,
                                                    PrefFormCustomField)), 12);
        }
    }
    if (FldDirty(GetObjectPointer(frm, PrefFormNotifyWith))) {
        if (FldGetTextPtr(GetObjectPointer(frm, PrefFormNotifyWith))) {
            StrNCopy(gPrefsR.BirthPrefs.notifywith,
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
        if (gPrefsR.BirthPrefs.sysdateover && gPrefdfmts != gSystemdfmts)
        {
            needrescan = 1;
            gPrefdfmts = gSystemdfmts;    // revert to system date format
        }
    }
    gPrefsR.BirthPrefs.sysdateover = newoverride;
    gPrefsR.BirthPrefs.dateformat = newdateformat;

    // notify string
    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    gPrefsR.BirthPrefs.notifyformat = LstGetSelection(lstnotify);

    // addr goto application id
    lstaddr = GetObjectPointer(frm, PrefFormAddress);
    gPrefsR.BirthPrefs.addrapp = LstGetSelection(lstaddr);
    
    ptr = GetObjectPointer(frm, PrefFormEmphasize);
    gPrefsR.DispPrefs.emphasize = CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, PrefFormScanNote);
    if (CtlGetValue(ptr) != gPrefsR.BirthPrefs.scannote ) {
        needrescan = 1;
    }
    gPrefsR.BirthPrefs.scannote = CtlGetValue(ptr);

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

            DisplayCategory(trigger, string, true);
            ret = true;
        }
    }
    CategoryFreeList(dbP, lst, false, false);
    return ret;
}

static void MainFormSelectTableItem(Boolean selected, TablePtr table,
                                    Int16 row, Int16 column)
{
	RectangleType r;

	TblGetItemBounds(table, row, column, &r);

    if (!selected) {
        WinEraseRectangle(&r, 0);
        MainFormDrawRecord(table, row, column, &r);
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
    PackedBirthDate* rp;
    BirthDate r;
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

	barP = GetObjectPointer(frm, MainFormScrollBar);
	if (gMainTableTotals > visibleRows)
		SclSetScrollBar(barP, listOffset, 0, 
                        gMainTableTotals - visibleRows, visibleRows);
	else 
		SclSetScrollBar(barP, 0, 0, 0, gMainTableTotals);

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

    if (gPrefsR.BirthPrefs.sort == 0) {
        CtlSetValue(GetObjectPointer(frm, MainFormName), 1);
    }
    else if (gPrefsR.BirthPrefs.sort == 1) {
        CtlSetValue(GetObjectPointer(frm, MainFormDate), 1);
    }
    else {
        CtlSetValue(GetObjectPointer(frm, MainFormAge), 1);
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
    if (gPrefsR.private == 1) {
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
    bk3Icon[5] = gPrefsR.DBNotifyPrefs.datebk3icon + 'A';

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
    pfmtString = gNotifyFormatString[(int)gPrefsR.BirthPrefs.notifyformat];
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
                    StrCopy(pDesc, gPrefsR.BirthPrefs.custom);
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

    StrCat(noteField, gPrefsR.BirthPrefs.notifywith);
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

static Int16 PerformNotifyTD(BirthDate birth, DateType when, Int8 age,
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

    StrCopy(noteField, gPrefsR.BirthPrefs.notifywith);
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
    PackedBirthDate* rp;
    BirthDate r;
    RepeatInfoType repeatInfo;
    Int16 i;
    
    // if duration is equal to 0, no notify record is created
    //
    if (gPrefsR.DBNotifyPrefs.duration == 0) return;

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

            if (gPrefsR.DBNotifyPrefs.duration > 1) {
                // if solar date, make entry repeat
                //
                repeatInfo.repeatEndDate = when;
                repeatInfo.repeatEndDate.year +=
                    gPrefsR.DBNotifyPrefs.duration -1;

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


static Boolean DBNotifyFormHandleEvent(EventPtr e)
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

static Boolean ToDoFormHandleEvent(EventPtr e)
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
    
    x = bounds->topLeft.x + (bounds->extent.x - 8) / 2;
    y = bounds->topLeft.y + (bounds->extent.y - 8) / 2; 

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

static void ViewFormDrawRecord(MemPtr tableP, Int16 row, Int16 column, 
                               RectanglePtr bounds)
{
    FontID currFont;
    MemHandle recordH = 0;
    LineItemPtr ptr;
	Boolean ignored = true;
    Int16 index;
    DateType converted;
    Int8 age;

    PackedBirthDate* rp;
    BirthDate r;
	Int16 dateDiff;
	DateType current;
	char displayStr[25];

	WinEraseRectangle(bounds, 0);

    // Read the necessary information from LineItem
    //
    ptr = MemHandleLock(gTableRowHandle);
    index = ptr[gMainTableHandleRow].birthRecordNum;
    converted = ptr[gMainTableHandleRow].date;
    age = ptr[gMainTableHandleRow].age;
    MemPtrUnlock(ptr);

    // current time
    DateSecondsToDate(TimGetSeconds(), &current);

    if ((recordH = DmQueryRecord(MainDB, index))) {
        Int16 x,y;
        Int16 width, length;
        Int16 viewItem;
        
        x = bounds->topLeft.x;
        y = bounds->topLeft.y;
        width = bounds->extent.x;

        viewItem = TblGetItemInt(tableP, row, column);
        rp = (PackedBirthDate *) MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        UnpackBirthdate(&r, rp);

        currFont = FntSetFont(gPrefsR.viewFont);

        switch (viewItem) {
        case ViewName :
            if (column == 0) {
                RectangleType rect;
                TblGetBounds(tableP, &rect);

                ///////////////////////////////////
                // Display name in large bold font
                ///////////////////////////////////
                if (gbVgaExists && VgaIsVgaFont(gPrefsR.viewFont)) {
                    FntSetFont(VgaBaseToVgaFont(largeBoldFont));
                }
                else FntSetFont(largeBoldFont);
                
                DrawRecordName(r.name1, r.name2, rect.extent.x,
                               &rect.topLeft.x, rect.topLeft.y,
                               false,
                               r.flag.bits.priority_name1 || !gSortByCompany);
            }
            break;
        case ViewType:
            ///////////////////////////////////
            // Display the Type of Anniversary
            ///////////////////////////////////
            if (column == 0) {
                SysCopyStringResource(gAppErrStr, TypeStr);
                length = StrLen(gAppErrStr);
                FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
                WinDrawChars(gAppErrStr, length, x, y);
            }
            else {
                char* p;
                
                if (!r.custom[0]) {         // custom does not exist
                    p = gPrefsR.BirthPrefs.custom;
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
            ///////////////////////////////////
            // Display the Next Date
            ///////////////////////////////////
            if (column == 0) {
                SysCopyStringResource(gAppErrStr, NextDateStr);
                length = StrLen(gAppErrStr);
                FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
                WinDrawChars(gAppErrStr, length, x, y);
            }
            else {
                char temp[5];
                if (converted.year != INVALID_CONV_DATE) {
                    DateToAsciiLong(converted.month, converted.day,
                                    converted.year+ 1904, gPrefdfmts,
                                    gAppErrStr);

                    SysCopyStringResource(temp,
                                          DayOfWeek(converted.month, converted.day,
                                                    converted.year+1904) + SunString);
                    StrNCat(gAppErrStr, " (", AppErrStrLen);
                    StrNCat(gAppErrStr, temp, AppErrStrLen);
                    StrNCat(gAppErrStr, ")", AppErrStrLen);
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
            ///////////////////////////////////
            // Display the Source Date
            ///////////////////////////////////

            if (column == 0) {
                SysCopyStringResource(gAppErrStr, SourceDateStr);
                length = StrLen(gAppErrStr);
                FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
                WinDrawChars(gAppErrStr, length, x, y);
            }
            else {
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
            ///////////////////////////////////
            // Process Age
            ///////////////////////////////////
            if (column == 0) {
                SysCopyStringResource(gAppErrStr, AgeStr);
                length = StrLen(gAppErrStr);
                FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
                WinDrawChars(gAppErrStr, length, x, y);
            }
            else {
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
            if (column == 0) {
                SysCopyStringResource(gAppErrStr, RemainedDayStr);
                    
                length = StrLen(gAppErrStr);
                FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
                WinDrawChars(gAppErrStr, length, x, y);
            }
            else {

                dateDiff = DateToDays(converted) - DateToDays(current);

                if (dateDiff >= (Int16)0) {
                    SysCopyStringResource(displayStr, BirthLeftString);
                    StrPrintF(gAppErrStr, displayStr, dateDiff);
                }
                else {
                    SysCopyStringResource(displayStr, BirthPassedString);
                    StrPrintF(gAppErrStr, displayStr, -1 * dateDiff);
                }
            }
            
            length = StrLen(gAppErrStr);
            FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
            WinDrawChars(gAppErrStr, length, x, y);
            break;
        }
        MemHandleUnlock(recordH);

		// restore font
		FntSetFont (currFont);
    }
}
static void ViewFormInitTable(FormPtr frm)
{
    TablePtr 		tableP;
    Int16 			tblRows, row;
    Int16			row_height;
	FontID			currFont;

    tableP = GetObjectPointer(frm, ViewFormTable);
	tblRows = TblGetNumberOfRows(tableP); 

	currFont = FntSetFont(gPrefsR.viewFont);
	row_height = FntLineHeight();

    for (row=0; row < tblRows; row++) {
		TblSetRowHeight(tableP, row, row_height);
		TblSetItemStyle(tableP, row, 0, customTableItem);
		TblSetItemStyle(tableP, row, 1, customTableItem);
	}
	TblSetCustomDrawProcedure(tableP, 0, ViewFormDrawRecord);
	TblSetCustomDrawProcedure(tableP, 1, ViewFormDrawRecord);

	FntSetFont(currFont);
}

static void ViewFormLoadTable(FormPtr frm, Int16 listOffset)
{
    TablePtr 		tableP;
	Int16 			tblRows, row, visibleRows;
	RectangleType	rect;
	Int16			running_total, row_height;
	FontID			currFont;
    Int16           tblItemIdx = ViewName;

    tableP = GetObjectPointer(frm, ViewFormTable);
	tblRows = TblGetNumberOfRows(tableP); 
	
    TblSetColumnUsable(tableP, 0, true);
    TblSetColumnUsable(tableP, 1, true);

	currFont = FntSetFont(gPrefsR.viewFont);
	row_height = FntLineHeight();
	TblGetBounds(tableP, &rect);
	running_total = 0;
	visibleRows = 0;

    for (row=0; row < tblRows; row++) {
        if (row == ViewName) 
            TblSetRowHeight(tableP, row, row_height*2);
        else 
            TblSetRowHeight(tableP, row, row_height);
        
        TblSetItemInt(tableP, row, 0, tblItemIdx);
        TblSetItemInt(tableP, row, 1, tblItemIdx);

        TblSetRowSelectable(tableP, row, false);

        if (tblItemIdx == ViewSrc && !gPrefsR.DispPrefs.dispextrainfo) {
            tblItemIdx += 2;
        }
        else {
            tblItemIdx++;
        }
        
		running_total += row_height;
		if (row < gMainTableTotals)
			TblSetRowUsable(tableP, row, running_total <= rect.extent.y);
		else 
			TblSetRowUsable(tableP, row, false);

		if (running_total <= rect.extent.y) 
			visibleRows++;

		TblMarkRowInvalid(tableP, row);
	}
	FntSetFont(currFont);
}

static void ViewFormSetCategory()
{
    UInt16 addrattr;
    LineItemPtr ptr;
    Int16 index;

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
}

static void ViewFormInit(FormPtr frm, Boolean bformModify)
{

    ViewFormInitTable(frm);
	// display starting date
	if (gbVgaExists) {
		VgaTableUseBaseFont(GetObjectPointer(frm, ViewFormTable), 
							!VgaIsVgaFont(gPrefsR.viewFont));
		if (bformModify) VgaFormModify(frm, vgaFormModify160To240);
		ViewFormResize(frm, false);
	}

	ViewFormLoadTable(frm, 0);
    ViewFormSetCategory();
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
            case vchrPrevField:
                if (gMainTableHandleRow < gMainTableTotals-1) {
                    gMainTableHandleRow++;
        
                    SndPlaySystemSound(sndInfo);

                    ViewFormSetCategory();
                    ViewFormLoadTable(frm, 0);
                    FrmDrawForm(FrmGetFormPtr(ViewForm));
                    // TblDrawTable(tableP);

                    handled = true;
                }
                break;
            case vchrPageUp:    
            case vchrNextField:
                if (gMainTableHandleRow > 0 ) {
                    gMainTableHandleRow--;

                    SndPlaySystemSound(sndInfo);

                    ViewFormSetCategory();
                    ViewFormLoadTable(frm, 0);
                    FrmDrawForm(FrmGetFormPtr(ViewForm));

                    // TblDrawTable(tableP);

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
        break;  
    default:
        break;
    }

    return handled;
}

static Boolean StartFormHandleEvent(EventPtr e)
{
    Int16 err;
    Boolean handled = false;
    FormPtr frm = FrmGetFormPtr(StartForm);

    // no event procedure
    //
    switch (e->eType) {
	case frmUpdateEvent:
    case frmOpenEvent:
        if(gbVgaExists) 
       		VgaFormModify(frm, vgaFormModify160To240);

        err = UpdateBirthdateDB(MainDB, frm);
        if (err < 0) {
            char ErrStr[200];

            SysCopyStringResource(ErrStr, CantFindAddrString);
            StrPrintF(gAppErrStr, ErrStr, gPrefsR.BirthPrefs.custom);

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
