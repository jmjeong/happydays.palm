/*
HappyDays - A HDdate displayer for the PalmPilot
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
#include <SonyCLIE.h>
#include "util.h"
#include "happydaysRsc.h"
#include "calendar.h"


/***********************************************************************
 * FUNCTION:    ResizeSilk
 * DESCRIPTION: resizes the virtual silkscreen to the specified size
 * PARAMETERS:  silkRefNum - reference number to the Silk Manager library
 *              size       - the desired silkscreen size;
 *                           must be vskResizeMax, vskResizeMin, or
 *                             vskResizeNone
 * RETURNS:     Err value
 ***********************************************************************/
Err ResizeSilk(UInt16 silkRefNum, UInt16 size)
{
    Err error = errNone;
    UInt16 silkMgrVersion = VskGetAPIVersion(silkRefNum);
    if (silkMgrVersion < 2)
    {
        error = SilkLibResizeDispWin(silkRefNum, size);
    }
    else
    {
        error = VskSetState(silkRefNum, vskStateResize, size);
    }
    return error;
}

/***********************************************************************
 * FUNCTION:    EnableSilkResize
 * DESCRIPTION: enables or disables silkscreen resizing
 * PARAMETERS:  silkRefNum - reference number to the Silk Manager library
 *              state      - the resize state to enable;
 *                           must be a vskResizeDisable or a combination of
 *                             the vskResizeVertically and
 *                             vskResizeHorizontally bit flags
 * RETURNS:     Err value
 ***********************************************************************/
Err EnableSilkResize(UInt16 silkRefNum, UInt16 state)
{
    Err error = errNone;
    UInt16 silkMgrVersion = VskGetAPIVersion(silkRefNum);
    if (silkMgrVersion < 2)
    {
        if (state & vskResizeVertically)
        {
            error = SilkLibEnableResize(silkRefNum);
        }
        else if (state == vskResizeDisable)
        {
            error = SilkLibDisableResize(silkRefNum);
        }
    }
    else
    {
        if (silkMgrVersion < 3)
        {
            // versions of the silk manager earlier than 3 do not recognize
            // the vskResizeHorizontally bit
            state &= vskResizeVertically;
        }
        error = VskSetState(silkRefNum, vskStateEnable, state);
    }
    return error;
}

UInt16 GetSilkPos(UInt16 silkRefNum)
{
    // these values are no longer defined in SonySilkLib.h
#ifndef stdSilkHeight
#define stdSilkHeight   (65)
#define stdStatusHeight (15)
#endif
    
    UInt16 silkMgrVersion = VskGetAPIVersion(silkRefNum);
    if (silkMgrVersion < 2)
    {
        Coord w, h;
        UInt32 winMgrVersion;

        // on Sony High-Resolution devices, WinGetDisplayExtent always returns
        // low-res (standard) values...
        WinGetDisplayExtent(&w, &h);

        // ... however, on high-density devices, we might need to scale the
        // dimensions

        if (   FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winMgrVersion) == errNone
            && winMgrVersion >= 4)
        {
            w = WinUnscaleCoord(w, false);
            h = WinUnscaleCoord(h, false);
        }

        switch (h)
        {
            default:
                ErrNonFatalDisplay("Unrecognized display extent");
                // fall through

            case stdHeight:
                return vskResizeMax;

            case stdHeight + stdSilkHeight:
                return vskResizeMin;

            case stdHeight + stdSilkHeight + stdStatusHeight:
                return vskResizeNone;
        }
    }
    else
    {
        UInt16 state;
        VskGetState(silkRefNum, vskStateResize, &state);
        return state;
    }
}

Boolean TextMenuHandleEvent(UInt16 menuID, UInt16 objectID)
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
 
//
// GetObjectPointer - return an object pointer given a form and objID
//
void * GetObjectPointer(FormPtr frm, UInt16 objID)
{
     return FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, objID));
}


//
//  SetFieldTextFromHandle - set txtH into field object text
//
FieldPtr SetFieldTextFromHandle(UInt16 fieldID, MemHandle txtH)
{
   MemHandle   oldTxtH;
   FormPtr     frm = FrmGetActiveForm();
   FieldPtr    fldP;

   // get the field and the field's current text handle.
   fldP     = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldID));
   ErrNonFatalDisplayIf(!fldP, "missing field");
   oldTxtH  = FldGetTextHandle(fldP);
   
   // set the field's text to the new text.
   FldSetTextHandle(fldP, txtH);

   // free the handle AFTER we call FldSetTextHandle().
   if (oldTxtH) 
      MemHandleFree((MemHandle)oldTxtH);
   
   return fldP;
}

// Allocates new handle and copies incoming string
FieldPtr SetFieldTextFromStr(UInt16 fieldID, Char * strP)
{
   MemHandle    txtH;
   
   // get some space in which to stash the string.
   txtH  = MemHandleNew(StrLen(strP) + 1);
   if (!txtH)
      return NULL;

   // copy the string to the locked handle.
   StrCopy((Char *)MemHandleLock(txtH), strP);

   // unlock the string handle.
   MemHandleUnlock(txtH);
   
   // set the field to the handle
   return SetFieldTextFromHandle(fieldID, txtH);
}

FieldPtr ClearFieldText(UInt16 fieldID)
{
    return SetFieldTextFromHandle(fieldID, NULL);
}

Boolean FindNearLunar(DateType *dt, DateType base, Boolean leapyes)
{
    int lunyear, lunmonth, lunday;
    DateTimeType rtVal;
    Int16 i;

    lunmonth = dt->month; lunday = dt->day;

    for (i = -1; i < 5; i++) {
        lunyear = base.year + 1904 + i;

        if  (lun2sol(lunyear, lunmonth, lunday, leapyes, &rtVal)) continue;
        
        dt->year = rtVal.year - 1904;
        dt->month = rtVal.month; dt->day = rtVal.day;

        if (DateCompare(base, *dt) <= 0) return true;
    }
    return false;
}

// if year is -1, ignore year field
//
Char* DateToAsciiLong(UInt8 months, UInt8 days, Int16 year,
                            DateFormatType dateFormat, Char * pstring)
{
    if (year >= 0) {
        switch (dateFormat) {
        case dfDMYWithSlashes:
            StrPrintF(pstring, "%d/%d/%d", days, months, year);
            break;
        case dfDMYWithDots:
            StrPrintF(pstring, "%d.%d.%d", days, months, year);
            break;
        case dfDMYWithDashes:
            StrPrintF(pstring, "%d-%d-%d", days, months, year);
            break;
        case dfYMDWithSlashes:
            StrPrintF(pstring, "%d/%d/%d", year, months, days);
            break;
        case dfYMDWithDots:
            StrPrintF(pstring, "%d.%d.%d", year, months, days);
            break;
        case dfYMDWithDashes:
            StrPrintF(pstring, "%d-%d-%d", year, months, days);
            break;
        case dfMDYWithSlashes:
        default:
            StrPrintF(pstring, "%d/%d/%d", months, days, year);
            break;
        }
    }
    else {      // no year
        switch (dateFormat) {
        case dfDMYWithSlashes:
            StrPrintF(pstring, "%d/%d", days, months);
            break;
        case dfDMYWithDots:
            StrPrintF(pstring, "%d.%d.", days, months);
            break;
        case dfDMYWithDashes:
            StrPrintF(pstring, "%d-%d", days, months);
            break;
        case dfYMDWithSlashes:
            StrPrintF(pstring, "%d/%d", months, days);
            break;
        case dfYMDWithDots:
            StrPrintF(pstring, "%d.%d", months, days);
            break;
        case dfYMDWithDashes:
            StrPrintF(pstring, "%d-%d", months, days);
            break;
        case dfMDYWithSlashes:
        default:
            StrPrintF(pstring, "%d/%d", months, days);
            break;
        }
    }
    return pstring;
}

/************************************************************
 *
 *  FUNCTION: AppInfoGetPtr
 *  DESCRIPTION: Return a locked pointer to the AppInfo or NULL
 *  PARAMETERS: dbP - open database pointer
 *  RETURNS: locked ptr to the AppInfo or NULL
 *
 *************************************************************/
MemPtr AppInfoGetPtr(DmOpenRef dbP)
{
    UInt16       cardNo;
    LocalID    dbID;
    LocalID    appInfoID;
   
    if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
        return NULL;
    if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL,
                       NULL, &appInfoID, NULL, NULL, NULL))
        return NULL;

    if (appInfoID == NULL)
        return NULL;
    else
        return MemLocalIDToLockedPtr(appInfoID, cardNo);
}   


// Elf hash
//
// A very good hash function. A return value should be
// taken modulo m, where m is the prime number of buckets
//
UInt32 Hash(Char* name1, Char* name2)
{
    UInt32 h = 0, g;

    while (*name1) {
        h = ( h << 4) + *name1++;
        g = h & 0xF0000000L;
        if (g) h ^= g >> 24;
        h &= ~g;
    }
    while (*name2) {
        h = ( h << 4) + *name2++;
        g = h & 0xF0000000L;
        if (g) h ^= g >> 24;
        h &= ~g;
    }

    // more large number will be added.
    // return h & 65213L;
    return h % 524289L;
}

void DisplayCategory(UInt32 trigger, Char* string, Boolean redraw)
{
    ControlPtr ctl;
//    FontID currFont;
    UInt16 categoryLen;
    FormPtr frm = FrmGetActiveForm();

/*	if (gbVgaExists) 
		currFont = VgaBaseToVgaFont(stdFont);
	else 
		currFont = stdFont;
*/
    categoryLen = StrLen(string);
    
    ctl = GetObjectPointer(frm, trigger);
    if ( redraw ) CtlEraseControl(ctl);

    CtlSetLabel(ctl, string);

    if (redraw) CtlDrawControl(ctl);
    
//    FntSetFont (currFont);
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

void PrvMoveObject(FormPtr frmP, UInt16 objIndex, 
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

void PrvResizeObject(FormPtr frmP, UInt16 objIndex, 
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
