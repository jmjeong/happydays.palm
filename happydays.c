/*
  HappyDays - A Birthday displayer for the Palm
  Copyright (C) 1999-2007 JaeMok Jeong

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
#include <SonyCLIE.h>
#include "Handera/Vga.h"
#include "Handera/Silk.h"

#include "section.h"

#include "address.h"
#include "memodb.h"
#include "lunar.h"
#include "todo.h"
#include "newtodo.h"
#include "birthdate.h"
#include "happydays.h"
#include "happydaysRsc.h"
#include "s2lconvert.h"
#include "util.h"
#include "notify.h"
#include "addresscommon.h"
#include "newaddress.h"

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

Int16         gNumOfEventNote = -1;
EventNoteInfo *gEventNoteInfo = 0;   // event note string
Boolean  is35;

UInt16      gFormID;            // current form id

UInt16      hrRefNum, hrSRefNum;   // clie library reference number
Boolean     gbVgaExists = false;

Boolean     gSonyClie = false;
Boolean     gSilkLibLoaded;

Boolean     gProgramExit = false;   // Program exit control(set by Startform)
Boolean     gIsNewPIMS;

////////////////////////////////////////////////////////////////////
// Prefs stuff 
////////////////////////////////////////////////////////////////////

struct sPrefsR gPrefsR;

enum ViewFormType { ViewType = 0,
                    ViewNext,
                    ViewSrc,
                    ViewAge,
                    ViewRemained,
//					ViewBioRhythm,
                    ViewSpace
};

////////////////////////////////////////////////////////////////////
// function declaration 
////////////////////////////////////////////////////////////////////
static void EventLoop(void);
static UInt16 StartApplication(void);
static void StopApplication(void);

static void Search(FindParamsPtr findParams);
static void DrawSearchLine(HappyDays hd, RectangleType r);

static void GoToItem (GoToParamsPtr goToParams, Boolean launchingApp);
static Boolean ApplicationHandleEvent(EventPtr e);

static void GetEventNoteInfo(); 
static Int16 GetNumOfEventNoteInfo(Char *rp);
static void ProcessEventNoteInfo(EventNoteInfo* eventNoteInfo, Char* rp, Int16 num);
static Boolean SpecialKeyDown(EventPtr e);

static Err NR70GraffitiHandleEvent(SysNotifyParamType * /* notifyParamsP */) SECT1;

static Int16 OpenDatabases(void);
static void freememories(void) SECT2;
static void CloseDatabases(void) SECT2;
static void MainFormReadDB() SECT2;
static int CalcPageSize(FormPtr frm) SECT2;

static void ViewFormLoadTable(FormPtr frm) SECT2;
static void ViewFormResize(FormPtr frmP, Boolean draw) SECT2;
static void ViewFormSilk() SECT2;

static Boolean StartFormHandleEvent(EventPtr e) SECT1;
static Boolean PrefFormHandleEvent(EventPtr e) SECT1;
static Boolean DispPrefFormHandleEvent(EventPtr e) SECT1;
static Boolean ViewFormHandleEvent(EventPtr e) SECT1;
static Boolean MainFormHandleEvent(EventPtr e) SECT1;

static void HighlightMatchRowDate(DateTimeType inputDate) SECT1;
static void HighlightMatchRowName(Char first) SECT1;

static void WritePrefsRec(void) SECT1;

static void MainFormLoadTable(FormPtr frm, Int16 listOffset) SECT1;
static void MainFormInit(FormPtr formP, Boolean resize) SECT1;

static void MainFormResize(FormPtr frmP, Boolean draw) SECT1;
static void MainTableSelectItem(TablePtr table, Int16 row, Boolean selected) SECT1;

static void MainFormDrawRecord(MemPtr tableP, Int16 row, Int16 column, RectanglePtr bounds) SECT1;

static Boolean MenuHandler(FormPtr frm, EventPtr e) SECT1;
static void MainFormScroll(Int16 newValue, Int16 oldValue, Boolean force_redraw) SECT1;
static void MainFormScrollLines(Int16 lines, Boolean force_redraw) SECT1;
static void ViewTableDrawData(MemPtr tableP, Int16 row, Int16 column, 
                       RectanglePtr bounds) SECT1;

static void DrawTiny(int size,int x,int y,int n) SECT1;
static void DrawSilkMonth(int mon, int year, int day, int x, int y) SECT1;

////////////////////////////////////////////////////////////////////
// private database for HappyDays
////////////////////////////////////////////////////////////////////

DmOpenRef MainDB;
 
////////////////////////////////////////////////////////////////////
//  accessing the built-in DB
////////////////////////////////////////////////////////////////////

DmOpenRef DatebookDB;
DmOpenRef ToDoDB;
DmOpenRef AddressDB;
DmOpenRef MemoDB;

UInt16    lunarRefNum;
UInt32    lunarClientText;

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
	UInt32  version;
	Err     err;
    UInt32  val;
	SonySysFtrSysInfoP sonySysFtrSysInfoP;
	UInt16  refNum;

    gTableRowHandle = 0;

    if (OpenDatabases() < 0) {
/*         if (!DatebookDB) { */
/*             /\* */
/*              * BTW make sure there is " " (single space) in unused ^1 ^2 ^3 */
/*              * entries or PalmOS <= 2 will blow up. */
/*              *\/ */

/*             SysCopyStringResource(gAppErrStr, DateBookFirstAlertString); */
/*             FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " "); */
/*         } */

        return 1;
    }
    
    err = lunar_OpenLibrary(&lunarRefNum, &lunarClientText);
    if (err != errNone) {
        FrmCustomAlert(ErrorAlert, "Please install lunarlib.prc", " ", " ");
        return 1;
    }

	if (_TRGVGAFeaturePresent(&version)) gbVgaExists = true;
	else gbVgaExists = false;
	
	err = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &version);
	is35 = (version >= 0x03503000);

    if (!FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &val)) {
        Err         error;
        UInt32      width, height;

        if (val == sonyHwrOEMCompanyID_Sony) {
       		if ((error = FtrGet(sonySysFtrCreator,
                                sonySysFtrNumSysInfoP, (UInt32*)&sonySysFtrSysInfoP))) {
                // NOT CLIE
            }
            else {
                if ((sonySysFtrSysInfoP->extn & sonySysFtrSysInfoExtnSilk) &&
                    (sonySysFtrSysInfoP->libr & sonySysFtrSysInfoLibrSilk)) {
                    if ((error = SysLibFind(sonySysLibNameSilk, &refNum))) {
                        if (error == sysErrLibNotFound) {
                            // coulnd't find lib
                            error = SysLibLoad('libr', sonySysFileCSilkLib, &refNum);
                        }
                    }
                    if (!error ) {
                        // the silkscreen library is available and usable
                        if (VskGetAPIVersion(refNum) < 2) {
                            error = SilkLibOpen(refNum);
                        }
                        else error = VskOpen(refNum); 
                        
                        if (error == errNone) {
                            EnableSilkResize(refNum, vskResizeVertically | vskResizeHorizontally);
                        
                            hrSRefNum = refNum;
                            gSilkLibLoaded = true;
                        }
                    }
                }
            }

            error = SysLibLoad('libr', sonySysFileCHRLib, &hrRefNum);
            
            if (!error) {
                gSonyClie = true;

                HROpen(hrRefNum);
                width = hrWidth; height = hrHeight;
                HRWinScreenMode(hrRefNum, winScreenModeSet, &width, &height, NULL, NULL);
            }
        }
    }

    if(gbVgaExists)
    	VgaSetScreenMode(screenMode1To1, rotateModeNone);

    // set current date to  the starting date
	DateSecondsToDate(TimGetSeconds(), &gStartDate);
    
    GetEventNoteInfo();

    if (gSilkLibLoaded) {
    	// for NR70, Notification manager
		LocalID             ProgramdbID;
		UInt16              ProgramcardNo;
	
    	SysCurAppDatabase (&ProgramcardNo, &ProgramdbID); 
        err = SysNotifyRegister(ProgramcardNo, ProgramdbID,
                                sysNotifyDisplayChangeEvent, NR70GraffitiHandleEvent, 
                                sysNotifyNormalPriority, NULL);
        ErrFatalDisplayIf( ( err != errNone ), "can't register" );
    }
    

    return 0;
}

/* Save preferences, close forms, close app database */
static void StopApplication(void)
{
    if (gSilkLibLoaded) {
    	// for NR70, Notification manager
		LocalID             ProgramdbID;
		UInt16              ProgramcardNo;
        Int16 	            error;

    	SysCurAppDatabase (&ProgramcardNo, &ProgramdbID);

        error = SysNotifyUnregister(ProgramcardNo, ProgramdbID,
                                    sysNotifyDisplayChangeEvent, sysNotifyNormalPriority);
        ErrFatalDisplayIf( ( error != errNone ), "can't unregister" );
        
        if (VskGetAPIVersion(hrSRefNum) < 2) {
            // old devices didn't restore the silkscreen automatically
            // for applications that don't support silkscreen resizing;
            // therefore, we need to restore the silkscreen explicitly
            (void) ResizeSilk(hrSRefNum, vskResizeMax);
        }

        // disable silkscreen resizing before we quit
        (void) EnableSilkResize(hrSRefNum, vskResizeDisable);

        if (VskGetAPIVersion(hrSRefNum) < 2) {
            SilkLibClose(hrSRefNum);
        }
        else VskClose(hrSRefNum); // same as SilkLibClose
    }

    if (gSonyClie) {
        HRWinScreenMode(hrRefNum, winScreenModeSetToDefaults,
                        NULL, NULL, NULL, NULL);
        HRClose(hrRefNum);
    }
    
    FrmSaveAllForms();
    FrmCloseAllForms();
    CloseDatabases();
    freememories();

    lunar_CloseLibrary(lunarRefNum, lunarClientText);
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
         
         	// Search each of the fields of the birthday
      
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

static Boolean ApplicationHandleEvent(EventPtr e)
{
    FormPtr frm;
    Boolean handled = false;

    if (e->eType == frmLoadEvent) {
        gFormID = e->data.frmLoad.formID;
        frm = FrmInitForm(gFormID);
        FrmSetActiveForm(frm);

        switch(gFormID) {
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
		case NotifyStringForm:
			FrmSetEventHandler(frm, NotifyStringFormHandleEvent);
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

static void GetEventNoteInfo()
{
    MemHandle recordH = 0;
    UInt16  currindex = 0;
    char*   rp;
    Boolean found = false;

    gEventNoteInfo = 0;

    if (gPrefsR.eventNoteExists) {
        if ((recordH = DmQueryRecord(MemoDB, gPrefsR.eventNoteIndex)) != NULL) {
            rp = (char *) MemHandleLock(recordH);
            if (StrNCaselessCompare(rp, HappydaysEventNoteTitle, StrLen(HappydaysEventNoteTitle)) == 0) {
                gNumOfEventNote = GetNumOfEventNoteInfo(rp);
                if (gNumOfEventNote > 0) {
                    gEventNoteInfo = MemPtrNew(sizeof(EventNoteInfo) * gNumOfEventNote);

                    ProcessEventNoteInfo(gEventNoteInfo, rp, gNumOfEventNote);
                }

                MemHandleUnlock(recordH);
                return;
            }
            MemHandleUnlock(recordH);
        }
    }
    
    while ((recordH = DmQueryNextInCategory(MemoDB,
                                            &currindex, dmAllCategories)) ) {
        rp = (char *) MemHandleLock(recordH);
        if (StrNCaselessCompare(rp, HappydaysEventNoteTitle, StrLen(HappydaysEventNoteTitle)) == 0) {
            gNumOfEventNote = GetNumOfEventNoteInfo(rp);

            if (gNumOfEventNote > 0) {
                gEventNoteInfo = MemPtrNew(sizeof(EventNoteInfo) * gNumOfEventNote);

                ProcessEventNoteInfo(gEventNoteInfo, rp, gNumOfEventNote);
            }

            gPrefsR.eventNoteIndex = currindex;
            found = true;
            MemHandleUnlock(recordH);
            break;
        }

        MemHandleUnlock(recordH);
        currindex++;
    }
    gPrefsR.eventNoteExists = found;
}

static Int16 GetNumOfEventNoteInfo(Char *rp)
{
    char* p = rp;
    char *q;
    Int16 num = 0;

    while ( (q = StrChr(p, '\n')) )
    {
        if (*(q+1) == '*') {
            p = q+1;
            num++;
        }
        else p++;
    }
     
    return num;
}

static void ProcessEventNoteInfo(EventNoteInfo* eventNoteInfo, Char* rp, Int16 num)
{
    char *p = rp;
    char *q;
    Int16 i = 0;
    Int16 len;

    while (*p++ != '*' && *p)
        ;

    for (i = 0; i < num; i++) 
    {
        if (!*p) break;

        while (*p && (*p == ' ' || *p == '\n'))
            p++;

        q = p;

        while (*p && *p++ != '\n')
            ;

        len = (p - q) - 1;
        if (len > 20) len = 20;
            
        StrNCopy(eventNoteInfo[i].name, q, len);
        eventNoteInfo[i].name [len] = 0;

        eventNoteInfo[i].start = p - rp;

        while (*p && *p != '*')
            p++;

        eventNoteInfo[i].len = (p - rp) - eventNoteInfo[i].start;
        if (*p == '*') {
        	eventNoteInfo[i].len--;
        	p++;
        }
    }
    gNumOfEventNote = i;
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

static Err NR70GraffitiHandleEvent(SysNotifyParamType *notifyParamsP)
{
    if (gFormID == MainForm) {
        MainFormResize(FrmGetActiveForm(), true);
    }
    else if (gFormID == ViewForm) {
        ViewFormResize(FrmGetActiveForm(), true);
        ViewFormSilk();
    }
    return errNone;
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
    
    if (gEventNoteInfo) MemPtrFree(gEventNoteInfo);
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

        gPrefsR.version = HDAppVer;
        gPrefsR.records = gPrefsR.existing = gPrefsR.private = 0;

        gPrefsR.alarm = 1;
        gPrefsR.notifybefore = 3;
        gPrefsR.duration = 1;
        *((int*)&gPrefsR.when) = noTime;
        gPrefsR.apptCategory = 0;
        gPrefsR.dusenote = 0;
        gPrefsR.dnote[0] = 0;

        gPrefsR.priority = 1;
        gPrefsR.todoCategory = 0;
        gPrefsR.tusenote = 0;
        gPrefsR.tnote[0] = 0;

        StrCopy( gPrefsR.notifywith, "*HD:");
        gPrefsR.sort = 1;
        gPrefsR.notifyformat = 1;
        StrCopy( gPrefsR.notifyformatstring, "[+F] +e +y" );

        gPrefsR.autoscan = 0;
        gPrefsR.ignoreexclamation = 1;
        gPrefsR.scannote = 0;
        gPrefsR.addrapp = 0;
        gPrefsR.sysdateover = false;
        gPrefsR.dateformat = dfMDYWithSlashes;
 
#if defined(GERMAN)
        StrCopy( gPrefsR.custom, "Geburtstag");
        gPrefsR.dateformat = dfDMYWithDots; 
#elif defined(PORTUGUESE_BR)
        StrCopy( gPrefsR.custom, "Aniversario");
#elif defined(ITALIAN)
        StrCopy( gPrefsR.custom, "Compleanno");
#else
        StrCopy( gPrefsR.custom, "Birthday");
#endif
        gPrefsR.emphasize = 1;

        gPrefsR.gHideSecretRecord = 0;
        StrCopy(gPrefsR.addrCategory, "All");
        StrCopy(gPrefsR.adrcdate, "\0\0\0");
        StrCopy(gPrefsR.adrmdate, "\0\0\0");
        
        gPrefsR.listFont = (gbVgaExists) ? VgaBaseToVgaFont(stdFont) : stdFont;
        gPrefsR.eventNoteIndex = 0;
        gPrefsR.eventNoteExists = false;
    }

    if (gPrefsR.sysdateover) gPrefdfmts = gPrefsR.dateformat;
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
    DmOpenRef compDBRef;
    char ForDateBookCategoryCheck[dmCategoryLength];

    /*
     * Set these to zero in case any of the opens fail.
     */
    MainDB = 0;
    DatebookDB = 0;
    AddressDB = 0;
    MemoDB = 0;

    compDBRef = DmOpenDatabaseByTypeCreator('aexo', 'pdmE', dmModeReadOnly);
    if (!compDBRef) {
        gIsNewPIMS = false;
    }
    else {
        DmCloseDatabase(compDBRef);
        gIsNewPIMS = true;
    }

    /* Determine if secret records should be shown. */
    PrefGetPreferences(&sysPrefs);
    if (!sysPrefs.hideSecretRecords) mode |= dmModeShowSecret;

    gSystemdfmts = gPrefdfmts = sysPrefs.dateFormat;// save global date format
    gPreftfmts = sysPrefs.timeFormat;               // save global time format

    if (!gIsNewPIMS) {
        int ret = OpenPIMDatabases(DatebookAppID, AddressAppID, ToDoAppID, MemoAppID, mode);
        if (ret < 0) return ret;
    }
    else {
          int ret = OpenPIMDatabases(NewDatebookAppID, NewAddrAppID, NewTaskAppID, NewMemoAppID, mode);
          if (ret < 0) return ret;
    }

    MainDB = DmOpenDatabaseByTypeCreator('DATA', MainAppID, mode | dmModeReadWrite);
    if (!MainDB){
        //
        // Create our database if it doesn't exist yet
        //
        if (DmCreateDatabase(0, MainDBName, HDAppID, MainDBType, false))
            return -1;
        MainDB = DmOpenDatabaseByTypeCreator(MainDBType, HDAppID,
                                             dmModeReadWrite);
        if (!MainDB) return -1;
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

	//Prepare DateBook AppInfoBlock
	//if Needed
	CategoryGetName(DatebookDB, 0, ForDateBookCategoryCheck);
	if (!*ForDateBookCategoryCheck) CategorySetName(DatebookDB,0,"Unfiled");

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


static void RereadHappyDaysDB(DateType start)
{
    gMainTableTotals = AddrGetHappyDays(MainDB, gAddrCategory, start);
}

static void HighlightAction(int selected, Boolean sound)
{
    FormPtr frm = FrmGetActiveForm();
    TablePtr tableP = GetObjectPointer(frm, MainFormTable);

    if (sound) SndPlaySystemSound(sndClick);

    if (gMainTableHandleRow >= 0) {
        // unhighlight the selection
        MainTableSelectItem(tableP, gMainTableHandleRow - gMainTableStart, false);
    }

    if (gMainTableStart > selected
        || selected >= (gMainTableStart + gMainTablePageSize) ) {
        // if not exist in table view, redraw table
        gMainTableStart = MAX(0, selected-4);
        MainFormLoadTable(frm, gMainTableStart);
        TblDrawTable(tableP);
    }
    gMainTableHandleRow = selected;
    MainTableSelectItem(tableP, gMainTableHandleRow - gMainTableStart, true);
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

Boolean MenuHandler(FormPtr frm, EventPtr e)
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

        if (gPrefsR.autoscan == 2) {      // trick (force rescan)
            gPrefsR.autoscan = 3;         //    process Never
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
            if (!gIsNewPIMS) {
                ret = CleanupFromTD(ToDoDB);
            }
            else {
                ret = NewCleanupFromTD(ToDoDB);
            }
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

void MainFormScroll(Int16 newValue, Int16 oldValue, Boolean force_redraw)
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

void MainFormScrollLines(Int16 lines, Boolean force_redraw)
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

    if (gMainTableHandleRow == -1)
        gMainTableHandleRow = gMainTableStart;
    else if (gMainTableHandleRow > gMainTableStart) {
        // unhighlight
        MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                            gMainTableHandleRow - gMainTableStart, false);
        gMainTableHandleRow--;
    }
    else {
        if (valueP > minP) {
            MainFormScrollLines(-1, true);
            gMainTableHandleRow--;
        }
    }
	MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                        gMainTableHandleRow - gMainTableStart, true);
}

static void HandleNextKey(void)
{
    ScrollBarPtr barP;
    Int16        valueP, minP, maxP, pageSizeP;
	FormPtr frm = FrmGetActiveForm();

    barP = GetObjectPointer(frm, MainFormScrollBar);
    SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

    if (gMainTableHandleRow == -1)
        gMainTableHandleRow = gMainTableStart;
    else if (gMainTableHandleRow < gMainTableStart + pageSizeP - 1) {
        MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                            gMainTableHandleRow - gMainTableStart, false);

        gMainTableHandleRow++;
    }
    else {
        if (valueP <  maxP) {
            MainFormScrollLines(1, true);
            gMainTableHandleRow++;
        }
    }
	MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                        gMainTableHandleRow - gMainTableStart, true);
}


static void MainFormHandleSelect(FormPtr frm, Int16 row)
{
	ScrollBarPtr barP;
	Int16        valueP, minP, maxP, pageSizeP;

	barP = GetObjectPointer(frm, MainFormScrollBar);
	SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

	gMainTableHandleRow = row + valueP;

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
        }
        else {
            gMainTableStart = 0;
        }
        MainFormLoadTable(frm, gMainTableStart);
        
        FrmDrawForm(frm);
        
        if (gMainTableHandleRow >= 0) {
            MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                                gMainTableHandleRow - gMainTableStart, true);
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
	   	gMainTableHandleRow = -1; 
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
                gMainTableHandleRow = -1;
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
			if (gPrefsR.sort == 0) change = false;
			else gPrefsR.sort = 0;	// sort by name

			if (change) {
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
				handled = true;
			}

			break;
		}
		case MainFormDate:
		{
			Boolean change = true;
			if (gPrefsR.sort == 1) change = false;
			else gPrefsR.sort = 1; 	// sort by date

			if (change) {
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
				handled = true;
			}
			break;

		}
		case MainFormAge: 
		{
			Boolean change = true;

			if (gPrefsR.sort == 2) 
				gPrefsR.sort = 3;  	// sort by age(re)
			else gPrefsR.sort = 2; 	// sort by age

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
			gMainTableHandleRow = -1;			
			MainFormScrollLines(-(pageSizeP-1), true);
			handled = true;
			break;
		
		case vchrPageDown:
			gMainTableHandleRow = -1;			
			MainFormScrollLines((pageSizeP-1), true);
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
			if (gMainTableHandleRow != -1) {
				MainFormHandleSelect(frm, gMainTableHandleRow-gMainTableStart);
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
        if (gMainTableHandleRow >= 0) {
            // unhighlight
            MainTableSelectItem(GetObjectPointer(frm, MainFormTable),
                                gMainTableHandleRow-gMainTableStart, false);
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
    ListPtr lstdate, lstauto, lstnotify, lstaddr;

    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return;
    
    CtlSetValue(GetObjectPointer(frm, PrefFormOverrideSystemDate),
                gPrefsR.sysdateover);

    lstdate = GetObjectPointer(frm, PrefFormDateFmts);
    LstSetSelection(lstdate, gPrefsR.dateformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormDateTrigger),
                LstGetSelectionText(lstdate, gPrefsR.dateformat));

    lstauto = GetObjectPointer(frm, PrefFormAutoRescan);
    LstSetSelection(lstauto, gPrefsR.autoscan);
    CtlSetLabel(GetObjectPointer(frm, PrefFormRescanTrigger),
                LstGetSelectionText(lstauto, gPrefsR.autoscan));

    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    LstSetSelection(lstnotify, gPrefsR.notifyformat);
    CtlSetLabel(GetObjectPointer(frm, PrefFormNotifyTrigger),
                LstGetSelectionText(lstnotify, gPrefsR.notifyformat));
    SetFieldTextFromStr(PrefFormNotifyField, gPrefsR.notifyformatstring);
    
    lstaddr = GetObjectPointer(frm,PrefFormAddress);
    LstSetSelection(lstaddr, gPrefsR.addrapp);
    CtlSetLabel(GetObjectPointer(frm,PrefFormAddrTrigger),
                LstGetSelectionText(lstaddr,gPrefsR.addrapp));
                    
    SetFieldTextFromStr(PrefFormCustomField, gPrefsR.custom);
/*
  SetFieldTextFromStr(PrefFormNotifyWith, gPrefsR.Prefs.notifywith);
*/
    CtlSetValue(GetObjectPointer(frm, PrefFormScanNote), 
                gPrefsR.scannote);
    CtlSetValue(GetObjectPointer(frm, PrefFormIgnorePrefix), 
                gPrefsR.ignoreexclamation);

}

static Boolean UnloadPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    ListPtr lstdate, lstnotify, lstaddr;
    Boolean newoverride;
    DateFormatType newdateformat;
    Boolean needrescan = false;
    
    if ((frm = FrmGetFormPtr(PrefForm)) == 0) return needrescan;
    if (FldDirty(GetObjectPointer(frm, PrefFormCustomField))) {
        needrescan = true;
        if (FldGetTextPtr(GetObjectPointer(frm, PrefFormCustomField))) {
            StrNCopy(gPrefsR.custom,
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
            needrescan = true;
            gPrefdfmts = newdateformat;
        }
    }
    else
    {
        if (gPrefsR.sysdateover && gPrefdfmts != gSystemdfmts)
        {
            needrescan = true;
            gPrefdfmts = gSystemdfmts;    // revert to system date format
        }
    }
    gPrefsR.sysdateover = newoverride;
    gPrefsR.dateformat = newdateformat;

    // notify string
    lstnotify = GetObjectPointer(frm, PrefFormNotifyFmts);
    gPrefsR.notifyformat = LstGetSelection(lstnotify);

    // if (FldDirty(GetObjectPointer(frm, PrefFormNotifyField))) {
    if (FldGetTextPtr(GetObjectPointer(frm, PrefFormNotifyField))) {
        StrNCopy(gPrefsR.notifyformatstring,
                 FldGetTextPtr(GetObjectPointer(frm, PrefFormNotifyField)), 39);
    }
    // }

    // automatic scan of Address
    lstaddr = GetObjectPointer(frm, PrefFormAutoRescan);
    gPrefsR.autoscan = LstGetSelection(lstaddr);


    // addr goto application id
    lstaddr = GetObjectPointer(frm, PrefFormAddress);
    gPrefsR.addrapp = LstGetSelection(lstaddr);
    
    ptr = GetObjectPointer(frm, PrefFormScanNote);
    if (CtlGetValue(ptr) != gPrefsR.scannote ) {
        needrescan = true;
    }
    gPrefsR.scannote = CtlGetValue(ptr);

    ptr = GetObjectPointer(frm, PrefFormIgnorePrefix);
    if (CtlGetValue(ptr) != gPrefsR.ignoreexclamation ) {
        needrescan = true;
    }
    gPrefsR.ignoreexclamation = CtlGetValue(ptr);

    return needrescan;
}

static void LoadDispPrefsFields()
{
    FormPtr frm;
    
    if ((frm = FrmGetFormPtr(DispPrefForm)) == 0) return;

    CtlSetValue(GetObjectPointer(frm, DispPrefEmphasize), gPrefsR.emphasize);
}

static Boolean UnloadDispPrefsFields()
{
    FormPtr frm;
    ControlPtr ptr;
    Boolean redraw = 0;

    if ((frm = FrmGetFormPtr(DispPrefForm)) == 0) return redraw;

    ptr = GetObjectPointer(frm, DispPrefEmphasize);
    if (gPrefsR.emphasize != CtlGetValue(ptr)) {
        redraw = 1;
    }
    gPrefsR.emphasize = CtlGetValue(ptr);
    
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

        case PrefFormNotifyTrigger: {
            extern Char* gNotifyFormatString[];
            
            ListPtr lst = GetObjectPointer(FrmGetActiveForm(), PrefFormNotifyFmts);
            FieldPtr fld = GetObjectPointer(FrmGetActiveForm(), PrefFormNotifyField);
            
            Int16 selection = LstPopupList(lst);
            if (selection >= 0) {
                CtlSetLabel(GetObjectPointer(FrmGetActiveForm(), PrefFormNotifyTrigger),
                            LstGetSelectionText(lst, selection));
            }
            if (selection >= 0 && selection < 6) {      // 6 : Custom
                SetFieldTextFromStr(PrefFormNotifyField, gNotifyFormatString[selection]);
                FldDrawField(fld);
            }
            handled = true;
            
            break;
        }
            
        default:
            break;
        }
        break;

    case fldEnterEvent:
    case keyDownEvent:
    {
        if (gPrefsR.notifyformat != 6) {
            ListPtr lst = GetObjectPointer(FrmGetActiveForm(), PrefFormNotifyFmts);

            gPrefsR.notifyformat = 6;       // custom
            CtlSetLabel(GetObjectPointer(FrmGetActiveForm(), PrefFormNotifyTrigger), LstGetSelectionText(lst, 6));
        }

        break;
    }
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
  	if (select) {
		TblSelectItem(table, row, 0);
    }
}

void MainFormDrawRecord(MemPtr tableP, Int16 row, Int16 column, RectanglePtr bounds)
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
		width = nameExtent = bounds->extent.x - width 
            - ageFieldWidth - categoryWidth -2;


        if (r.flag.bits.nthdays) {
            char format[25];

            SysCopyStringResource(format, NthListFormatString);
            StrPrintF(gAppErrStr, format, r.nth);
            
            length = StrLen(gAppErrStr);
            FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
            WinDrawChars(gAppErrStr, length, x, y);

            x += width;
            nameExtent -= width;
        }
        
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

        if (gPrefsR.emphasize 
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
    Int16           valueP, minP, maxP, pageSizeP;

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
	if (gMainTableTotals > visibleRows) {
		SclSetScrollBar(barP, listOffset, 0, 
                        gMainTableTotals - visibleRows, visibleRows);
    }
	else {
		SclSetScrollBar(barP, 0, 0, 0, gMainTableTotals);
    }
    SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);
    gMainTableStart = valueP;

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
	}
	MainFormResize(frm, false);

	DisplayCategory(MainFormPopupTrigger, gPrefsR.addrCategory, false);
	DateToAsciiLong(gStartDate.month, gStartDate.day, 
					gStartDate.year+1904, gPrefdfmts, gLookupDate);
	CtlSetLabel(GetObjectPointer(frm, MainFormStart), gLookupDate);

    if (gPrefsR.sort == 0) {
        CtlSetValue(GetObjectPointer(frm, MainFormName), 1);
    }
    else if (gPrefsR.sort == 1) {
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

//    case ViewBioRhythm:
//        SysCopyStringResource(gAppErrStr, BioRhythmStr);

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

void ViewTableDrawData(MemPtr tableP, Int16 row, Int16 column, 
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
	Int32 dateDiff;
	DateType current;
	char displayStr[55];
    char tempStr[25];

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
        char* p = r.custom;
                
        if (!r.custom[0]) {         // custom does not exist
            p = gPrefsR.custom;
        }
        length = StrLen(p);
        FntCharsInWidth(p, &width, &length, &ignored);
        WinDrawChars(p, length, x, y);

        break;
    }
    case ViewNext:
    {
        ///////////////////////////////////
        // Display the Next Date
        ///////////////////////////////////
        
        if (converted.year != INVALID_CONV_DATE) {
            DateToAsciiLong(converted.month, converted.day,
                            converted.year+ 1904, gPrefdfmts,
                            gAppErrStr);

            SysCopyStringResource(tempStr,
                                  DayOfWeek(converted.month, converted.day,
                                            converted.year+1904) + SunString);
            StrNCat(gAppErrStr, " [", AppErrStrLen);
            StrNCat(gAppErrStr, tempStr, AppErrStrLen);
            StrNCat(gAppErrStr, "]", AppErrStrLen);
        }
        else {
            SysCopyStringResource(gAppErrStr, ViewNotExistString);
        }
        length = StrLen(gAppErrStr);
        FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
        WinDrawChars(gAppErrStr, length, x, y);
        break;
    }
    case ViewSrc:
    {
        ///////////////////////////////////
        // Display the Source Date
        ///////////////////////////////////

        if (r.flag.bits.lunar) {
            StrCopy(gAppErrStr, "-)");
        }
        else if (r.flag.bits.lunar_leap) {
            StrCopy(gAppErrStr, "#)");
        }
        else gAppErrStr[0] = 0;

        if (r.flag.bits.year) {
            DateToAsciiLong(r.date.month, r.date.day, r.date.year + 1904,
                            gPrefdfmts, displayStr);
        }
        else {
            DateToAsciiLong(r.date.month, r.date.day, -1, gPrefdfmts,
                            displayStr);
        }
        StrCat(gAppErrStr, displayStr);


        if (r.flag.bits.year) {
            if ( !(r.flag.bits.lunar || r.flag.bits.lunar_leap) ) {
                // make day of week
                SysCopyStringResource(tempStr,
                                      DayOfWeek(r.date.month, r.date.day,
                                                r.date.year+1904) + SunString);
                StrNCat(gAppErrStr, " [", AppErrStrLen);
                StrNCat(gAppErrStr, tempStr, AppErrStrLen);
                StrNCat(gAppErrStr, "]", AppErrStrLen);
            }
        }
        
        length = StrLen(gAppErrStr);
        FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
        WinDrawChars(gAppErrStr, length, x, y);
        break;
    }
        
    case ViewAge:
    {
        ///////////////////////////////////
        // Process Age
        ///////////////////////////////////

        if (r.flag.bits.nthdays) {
            SysCopyStringResource(gAppErrStr, NthFormatString);

            StrPrintF(displayStr, gAppErrStr, r.nth);
        }
        else if (r.flag.bits.year) {
            DateType solBirth;
            int syear, smonth, sday;
            Int16 d_diff = 0, m_diff = 0, y_diff = 0;
            
            if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {
                lunarL2S(lunarRefNum, r.date.year+1904,
                         r.date.month,
                         r.date.day,
                         r.flag.bits.lunar_leap, &syear, &smonth, &sday);

                
                solBirth.year = syear - 1904;
                solBirth.month = smonth;
                solBirth.day = sday;
            }
            else solBirth = r.date;
            
            dateDiff = (Int32)(DateToDays(current)
                               - (Int32)DateToDays(solBirth));

            if (dateDiff > (Int32)0) {
                if (current.day < solBirth.day) {
                    d_diff = DaysInMonth(solBirth.month, solBirth.year+1904)
                        + current.day - solBirth.day;
                    m_diff--;
                }
                else {
                    d_diff = current.day - solBirth.day;
                }
                        
                if ((Int32)current.month + m_diff  < (Int32)solBirth.month) {
                    m_diff += 12 + current.month - solBirth.month;
                    y_diff--;
                }
                else {
                    m_diff += current.month - solBirth.month;
                }
                y_diff += current.year - solBirth.year;
                    
        		SysCopyStringResource(gAppErrStr, AgeFormatString);
                StrPrintF(displayStr, gAppErrStr,
                          y_diff, m_diff, d_diff, dateDiff);
            }
            else StrCopy(displayStr, "-");
        }
        else {
            StrCopy(displayStr, "-");
        }
        length = StrLen(displayStr);
        FntCharsInWidth(displayStr, &width, &length, &ignored);
        WinDrawChars(displayStr, length, x, y);
    }
    break;
    case ViewRemained:
    {
		if (converted.year != INVALID_CONV_DATE) {
			dateDiff = (Int32)DateToDays(converted) - (Int32)DateToDays(current);

			if (dateDiff >= (Int32)0) {
				SysCopyStringResource(displayStr, BirthLeftString);
				StrPrintF(gAppErrStr, displayStr, dateDiff);
			}
			else {
				SysCopyStringResource(displayStr, BirthPassedString);
				StrPrintF(gAppErrStr, displayStr, -1 * dateDiff);
			}
		}
		else {
			StrCopy(gAppErrStr, "-");
		}
            
        length = StrLen(gAppErrStr);
        FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
        WinDrawChars(gAppErrStr, length, x, y);
    }
    break;
/*    
      case ViewBioRhythm:
      {
      Int16 phys, emot, intel;

      dateDiff = (Int32)DateToDays(converted) - (Int32)DateToDays(current);

      if (dateDiff >= (Int32)0) {
      phys = (dateDiff+12) % 23;
      emot = (dateDiff+14) % 28;
      intel = (dateDiff+17) % 33;

      SysCopyStringResource(displayStr, BioString);
      StrPrintF(gAppErrStr, displayStr,
      (phys<12 ? "+" : (phys>12 ? "-" : "C" )),
      (emot<14 ? "+" : (emot>14 ? "-" : "C" )),
      (intel<17 ? "+" : (phys>17 ? "-" : "C" )));

      length = StrLen(gAppErrStr);
      FntCharsInWidth(gAppErrStr, &width, &length, &ignored);
      WinDrawChars(gAppErrStr, length, x, y);
      }
            
      }
      break;
*/    
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

        tblItemIdx++;
        
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
	}
	ViewFormResize(frm, false);

	ViewFormLoadTable(frm);
    ViewFormSetInfo(frm);
}


void DrawTiny(int size,int x,int y,int n) 
{
    int a,b,c;

    if (size==1)
    {
        a=4;
        b=3;
        c=6;
    } else
    {
        a=2;
        b=2;
        c=4;
    }


    if (n & 1)
        WinDrawLine(x,y,x+a,y);
    if (n & 2)
        WinDrawLine(x,y,x,y+b);
    if (n & 4)
        WinDrawLine(x+a,y,x+a,y+b);
    if (n & 8)
        WinDrawLine(x,y+b,x+a,y+b);
    if (n & 16)
        WinDrawLine(x,y+b,x,y+c);
    if (n & 32)
        WinDrawLine(x+a,y+b,x+a,y+c);
    if (n & 64)
        WinDrawLine(x,y+c,x+a,y+c);
}

static void TinyNumber(int size,int x,int y,int num) 
{
    const int numbers[10] = {119,36,93,109,46,107,123,37,127,111};
    //if ((num >=0) && (num<=9))
    DrawTiny(size,x,y,numbers[num ]);
}

void DrawSilkMonth(int mon, int year, int day, int x, int y) 
{
	int i, width;
	char tempStr[255];
	int day_max, week;
	int day_cnt, week_cnt;
	RectangleType rc;

	FntSetFont(1);

	for (i = 0; i < 7; i++) {
		SysCopyStringResource(tempStr, WeekInitialString);
		width = FntCharWidth(tempStr[i]);
		WinDrawChars(&tempStr[i], 1, x+i * 11+ 5 - (width/2), y);
	}

	year += 1904 - (mon < 1) + ( mon > 12);
	if (mon < 1) mon = 12;
	else if ( mon > 12 ) mon = 1;

	day_max = DaysInMonth(mon, year);
	week = DayOfWeek(mon, 1, year);

	day_cnt = 1;
	week_cnt = 0;

	rc.extent.x = 9;
	rc.extent.y = 7;

	while ( day_cnt <= day_max && week_cnt < 7 ) {
		rc.topLeft.x = x + week * 11;
		rc.topLeft.y = y + week_cnt * 6 + 11;


		if ((day_cnt / 10) != 0) {
			TinyNumber(0, x + week*11, week_cnt*6+y+12, day_cnt / 10);
			TinyNumber(0, x + week*11+4, week_cnt*6+y+12, day_cnt % 10);
		}
		else {
			TinyNumber(0, x+week*11+2, week_cnt*6+y+12, day_cnt % 10);
		}

		if ( day == day_cnt ) {
			WinInvertRectangle(&rc, 0);
		}

		week++;
		if ( week >= 7) {
			week_cnt++; week = 0;
		}

		day_cnt++;
	}

	FntSetFont(0);

	rc.topLeft.x = x;
	rc.topLeft.y = y + 48;

	rc.extent.x = 74;
	rc.extent.x = FntLineHeight();

	WinEraseRectangle(&rc, 0);

	SysCopyStringResource(gAppErrStr, mon - 1 + JanString);
	StrPrintF(tempStr, "%d", year);
	StrCat(gAppErrStr, " ");
	StrCat(gAppErrStr, tempStr);
	width = FntCharsWidth(gAppErrStr,StrLen(gAppErrStr));
  	WinDrawChars( gAppErrStr,StrLen(gAppErrStr),x+(240/6)-(width/2),y+48);
}

static void DrawMonth(DateType converted)
{
	RectangleType rc;
	int start;	
	
	if (gbVgaExists) start= 185;
	else start = 120;

	RctSetRectangle(&rc, 0, start, 240, 66);
	WinEraseRectangle(&rc, 0);

	WinDrawLine(1,start,239,start);

	DrawSilkMonth(converted.month-1, converted.year, 0, 1, start+4);
	WinDrawLine(79,start,79,start+66);
	DrawSilkMonth(converted.month, converted.year, converted.day, 80+4, start+4);
	WinDrawLine(161,start,161,start+66);
	DrawSilkMonth(converted.month+1, converted.year, 0, 160+6, start+4);

	WinDrawLine(1,start+66,239,start+66);
}

static void ViewFormSilk()
{
    LineItemPtr ptr;

    DateType converted;

	if ((gbVgaExists && !SilkWindowMaximized())  || (gSilkLibLoaded && GetSilkPos(hrSRefNum) == vskResizeMin) ) {
		ptr = MemHandleLock(gTableRowHandle);
		converted = ptr[gMainTableHandleRow].date;
		MemPtrUnlock(ptr);

		DrawMonth(converted);
	}
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

		ViewFormSilk();
        
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

                    ViewFormSetInfo(frm);
                    ViewFormLoadTable(frm);
                    FrmDrawForm(FrmGetFormPtr(ViewForm));

					ViewFormSilk();

                    handled = true;
                }
                break;
            case vchrPageUp:    
            case vchrPrevField:
                if (gMainTableHandleRow > 0 ) {
                    gMainTableHandleRow--;

                    SndPlaySystemSound(sndInfo);

                    ViewFormSetInfo(frm);
                    ViewFormLoadTable(frm);
                    FrmDrawForm(FrmGetFormPtr(ViewForm));

					ViewFormSilk();

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
		ViewFormSilk();

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
	Boolean ok = true;

    FormPtr frm = FrmGetFormPtr(StartForm);

    // no event procedure
    //
    switch (e->eType) {
    case frmOpenEvent:
        
        if(gbVgaExists) 
       		VgaFormModify(frm, vgaFormModify160To240);

        if (!gIsNewPIMS) {
            FindHappyDaysField();
        }
        else {
            NewFindHappyDaysField();
        }

        rescan = false;
            
        if (IsChangedAddressDB()) {
            switch (gPrefsR.autoscan) {
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
                	break;
                }
                break;
            case 2: // no
                break;
            case 3:
                rescan = true;      // trick, force rescan
                    
                gPrefsR.autoscan = 2;
                break;
            }
            if (rescan) {
                if (!gIsNewPIMS) {
                    ok = UpdateHappyDaysDB(frm);
                }
                else {
                    ok = NewUpdateHappyDaysDB(frm);
                }
            }
            if (ok) SetReadAddressDB();     // mark addressDB is read
        }
        MainFormReadDB();
        FrmGotoForm(MainForm);
        handled = true;
        break;
    default:
        break;
    }
    
    return handled;
}
