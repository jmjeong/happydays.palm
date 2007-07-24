/*
HappyDays - A Birthdate displayer for Palm
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
#include "Handera/Vga.h"

#include "happydays.h"
#include "happydaysRsc.h"
#include "lunar.h"
#include "birthdate.h"
#include "util.h"
#include "s2lconvert.h"

static void DisplayInvalidDateErrorString(UInt16 id)
{
    FldDrawField(ClearFieldText(Ln2SlFormResult));
    SysCopyStringResource(gAppErrStr, InvalidDateString);
    FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " ");
}

Boolean Ln2SlFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    DateType dt = {0, 0};
    FormPtr frm = FrmGetFormPtr(Ln2SlForm);

    switch (e->eType) {
    case frmOpenEvent:
        if(gbVgaExists)
       		VgaFormModify(frm, vgaFormModify160To240);

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
            HappyDaysFlag dummy;
            Int16 year, month, day;
            Char* input;
            int ret;
            
            input = FldGetTextPtr(GetObjectPointer(frm, Ln2SlFormInput));

            if ((ret = AnalysisHappyDays(input, &dummy,
                                         &year, &month, &day))) {
                int syear, smonth, sday;
                int leapyes
                    = CtlGetValue(GetObjectPointer(frm, Ln2SlFormInputLeap));

                ret = lunarL2S(lunarRefNum, year, month, day, leapyes, &syear, &smonth, &sday);
                if (ret == errNone) {
                    Char temp[15];
                    SysCopyStringResource(temp, DayOfWeek(smonth, sday, syear) + SunString);

                    DateToAsciiLong(smonth, sday, syear, gPrefdfmts, gAppErrStr);
                    StrNCat(gAppErrStr, " [", AppErrStrLen);
                    StrNCat(gAppErrStr, temp, AppErrStrLen);
                    StrNCat(gAppErrStr, "]", AppErrStrLen);

                    FldDrawField(SetFieldTextFromStr(Ln2SlFormResult, gAppErrStr));
                }
                else DisplayInvalidDateErrorString(Ln2SlFormResult);
            }
            else {
                DisplayInvalidDateErrorString(Ln2SlFormResult);
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

Boolean Sl2LnFormHandleEvent(EventPtr e)
{
    Boolean handled = false;
    DateType dt = {0, 0};
    FormPtr frm = FrmGetFormPtr(Sl2LnForm);

    switch (e->eType) {
    case frmOpenEvent:
        if(gbVgaExists)
       		VgaFormModify(frm, vgaFormModify160To240);

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
            HappyDaysFlag dummy;
            Int16 year, month, day;
            int lyear, lmonth, lday;
            Char* input;
            int ret = false;

            input = FldGetTextPtr(GetObjectPointer(frm, Sl2LnFormInput));
            if ((ret = AnalysisHappyDays(input, &dummy,
                                         &year, &month, &day))) {
                int leapyes = 0;

                ret = lunarS2L(lunarRefNum, year, month, day, &lyear, &lmonth, &lday, &leapyes);
                if (ret == errNone) {
                    if (leapyes) {
                        StrCopy(gAppErrStr, "#)");
                    }
                    else {
                        StrCopy(gAppErrStr, "-)");
                    }
                    DateToAsciiLong(lmonth, lday, lyear, gPrefdfmts, &gAppErrStr[2]);
                          
                    FldDrawField(SetFieldTextFromStr(Sl2LnFormResult,
                                                     gAppErrStr));
                }
                else DisplayInvalidDateErrorString(Sl2LnFormResult);

            }
            else DisplayInvalidDateErrorString(Sl2LnFormResult);

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

