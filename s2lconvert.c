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

#include "happydays.h"
#include "happydaysRsc.h"
#include "calendar.h"
#include "birthdate.h"
#include "util.h"

static Boolean TextMenuHandleEvent(UInt16 menuID, UInt16 objectID)
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


Boolean Sl2LnFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    DateType dt = {0, 0};
    FormPtr frm = FrmGetFormPtr(Sl2LnForm);

    switch (e->eType) {
    case frmOpenEvent:
        DateSecondsToDate(TimGetSeconds(), &dt);

        DateToAsciiLong(dt.month, dt.day, dt.year + 1904,
                        gDispdfmts, gAppErrStr);

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
            Int16 year, month, day;
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
                                    gDispdfmts, &gAppErrStr[2]);
                          
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

    return handled;
}

Boolean Ln2SlFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    DateType dt = {0, 0};
    FormPtr frm = FrmGetFormPtr(Ln2SlForm);

    switch (e->eType) {
    case frmOpenEvent:
        DateSecondsToDate(TimGetSeconds(), &dt);
        DateToAsciiLong(dt.month, dt.day, dt.year + 1904,
                        gDispdfmts, gAppErrStr);
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
            Int16 year, month, day;
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
                                    gDispdfmts, gAppErrStr);
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

    return handled;
}
