/*
HappyDays - A HDday displayer for the PalmPilot
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


#include "birthdate.h"
#include "util.h"
#include "happydays.h"
#include "address.h"
#include "happydaysRsc.h"
#include "calendar.h"

extern Boolean gbVgaExists;

void PackHappyDays(HappyDays *hd, void* recordP)
{
    UInt16 offset = 0;

    DmWrite(recordP, offset, (Char*)&hd->addrRecordNum,
            sizeof(hd->addrRecordNum));
    offset += sizeof(hd->addrRecordNum);
    DmWrite(recordP, offset, (Char*)&hd->date,
            sizeof(hd->date));
    offset += sizeof(hd->date);
    DmWrite(recordP, offset, (Char*)&hd->flag,
            sizeof(hd->flag));
    offset += sizeof(hd->flag);  // for corealign

    if (hd->name1) {
        DmStrCopy(recordP, offset, (Char*)hd->name1);
        offset += StrLen(hd->name1) +1;
    }
    else {
        DmStrCopy(recordP, offset, "\0");
        offset += 1;
    }
    if (hd->name2) {
        DmStrCopy(recordP, offset, (Char*)hd->name2);
        offset += StrLen(hd->name2) +1;
    }
    else {
        DmStrCopy(recordP, offset,"\0");
        offset += 1;
    }

    if (hd->custom) {
        DmStrCopy(recordP, offset, (Char*)hd->custom);
        offset += StrLen(hd->custom) +1;
    }
    else {
        DmStrCopy(recordP, offset,"\0");
        offset += 1;
    }
}

void UnpackHappyDays(HappyDays *hd,
                            const PackedHappyDays *packedHappyDays)
{
    char *s = packedHappyDays->name;

    hd->addrRecordNum = packedHappyDays->addrRecordNum;
    hd->date = packedHappyDays->date;
    hd->flag = packedHappyDays->flag;

    hd->name1 = s;
    s += StrLen(s) + 1;
    hd->name2 = s;
    s += StrLen(s) + 1;
    hd->custom = s;
    s += StrLen(s) + 1;
}

static Int16 HDPackedSize(HappyDays* hd)
{
    Int16 size;

    size = sizeof(PackedHappyDays) - sizeof(char);    // corect
    if (hd->name1) size += StrLen(hd->name1) +1;
    else size += 1;     // '\0'

    if (hd->name2) size += StrLen(hd->name2) +1;
    else size += 1;     // '\0'

    if (hd->custom) size+= StrLen(hd->custom) +1;
    else size+= 1;

    return size;
}

Int16 CompareHappyDaysFunc(LineItemPtr p1, LineItemPtr p2, Int32 extra)
{   
    return DateCompare(p1->date, p2->date)*extra;
}
Int16 CompareAgeFunc(LineItemPtr p1, LineItemPtr p2, Int32 extra)
{   
	if (p1->age > p2->age) return 1*extra;
	else if (p1->age < p2->age) return -1*extra;
	else return 0;
}

static Int16 CalculateAge(DateType converted, DateType origin,
                          HappyDaysFlag flag)
{
    DateTimeType rtVal;
    int dummy = 0;
    int ret;
    
    if (flag.bits.lunar_leap || flag.bits.lunar) {
        // lunar birthdate
        //          change to lunar date
        //
        // converted solar is again converted into lunar day for calculation
        ret = sol2lun(converted.year + 1904, converted.month, converted.day,
                      &rtVal, &dummy);
        if (ret) return -1;     // error

        converted.year = rtVal.year - 1904;
        converted.month = rtVal.month;
        converted.day = rtVal.day;
    }
    
    return converted.year - origin.year;
}

UInt16 AddrGetHappyDays(DmOpenRef dbP, UInt16 AddrCategory, DateType start)
{
    UInt16 totalItems;
    UInt16 recordNum = 0;
    MemHandle recordH = 0;
    LineItemPtr ptr;
    PackedHappyDays* rp;
    HappyDays r;
    DateType converted;          
    UInt16 currindex = 0;
	Int16 age;

    // if exist, free the memory
    //
    if (gTableRowHandle) {
        MemHandleFree(gTableRowHandle);
        gTableRowHandle = 0;
    }
    
    totalItems = DmNumRecordsInCategory(dbP, AddrCategory);
    if (totalItems > 0) {
        gTableRowHandle = MemHandleNew(sizeof(LineItemType)* totalItems);
        ErrFatalDisplayIf(!gTableRowHandle, "Out of memory");
        
        if ((ptr = MemHandleLock(gTableRowHandle))) {
            for (recordNum = 0; recordNum < totalItems; recordNum++) {
                if ((recordH = DmQueryNextInCategory(dbP, &currindex,
                                                     (UInt16)AddrCategory))) {
                
                    ptr[recordNum].birthRecordNum = currindex;

                    rp = (PackedHappyDays *) MemHandleLock(recordH);
                    /*
                     * Build the unpacked structure for an AddressDB
                     * record.  It is just a bunch of pointers into the rp
                     * structure.
                     */
                    UnpackHappyDays(&r, rp);

                    // original r(in MainDB) not changed
                    //     local change for LineItemType
                    //
					converted = r.date;

                    if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {

                        if (!FindNearLunar(&converted, start,
                                           r.flag.bits.lunar_leap)) {
                            // ignore the records
                            //
                            converted.year = INVALID_CONV_DATE;
                            // max year(for sorting)
                            //   indicate for invalid date
                            
                        }
                    }
                    else if (r.flag.bits.solar) {
                        int maxtry = 4;
						DateType dt;

						dt = start;

                        DateToInt(dt) = 
							(DateToInt(dt) > DateToInt(converted))
                            ? DateToInt(dt) : DateToInt(converted);

                        if (converted.month < dt.month ||
                            ( converted.month == dt.month
                              && converted.day < dt.day)) {
                            // birthdate is in last year?
                            while (DaysInMonth(converted.month, ++dt.year) < converted.day
                                   && maxtry-- > 0) 
                                ;
                        }
                        else {
                            while (DaysInMonth(converted.month, dt.year) < converted.day
                                   && maxtry-- >0) {
                                dt.year++;
                            }
                            
                        }

                        if (maxtry >0) converted.year = dt.year;
                        else {
                            converted.year = INVALID_CONV_DATE;
                            // max year
                            //   indicate for invalid date
                        }
                    }
        
					if (converted.year != INVALID_CONV_DATE 
						&& r.flag.bits.year 
						&& (age = CalculateAge(converted, r.date, r.flag)) >= 0) {
						// calculate age if year exists
						//
						ptr[recordNum].age = age;
					}
					else {
						ptr[recordNum].age = -1;
					}

                    // save the converted data
                    //
                    ptr[recordNum].date = converted;

                    MemHandleUnlock(recordH);
                    currindex++;
                }
            }
            
            // sort the order if sort order is converted date
            //
            if (gPrefsR.Prefs.sort == 1) {      	// date sort
                SysInsertionSort(ptr, totalItems, sizeof(LineItemType),
                                 (_comparF *)CompareHappyDaysFunc, 1L);
            }
			else if (gPrefsR.Prefs.sort == 2) {  	// age sort
                SysInsertionSort(ptr, totalItems, sizeof(LineItemType),
                                 (_comparF *)CompareAgeFunc, 1L);
            }
			else if (gPrefsR.Prefs.sort == 3) {	// age sort(re)
                SysInsertionSort(ptr, totalItems, sizeof(LineItemType),
                                 (_comparF *)CompareAgeFunc, -1L);
            }
            
            MemPtrUnlock(ptr);
        } else return 0;
    }
    
    return totalItems;
}


Boolean AnalysisHappyDays(const char* field,
                          HappyDaysFlag *flag,
                          Int16* dYear, Int16* dMonth, Int16* dDay)
{
    Char * s = (Char *)field;
    Char * p;
    Char numOfDelimeter = 0;
    Int16 num[3], numIdx = 0;
    Int16 year=0, month=0, day=0;
    Char delimeter = '/';
    Char priorityName1 = flag->bits.priority_name1;
    
    while (*s == ' ' || *s == '\t') s++;     // skip white space
    StrNCopy(gAppErrStr, s, AppErrStrLen);

    s = gAppErrStr;

    // initalize flag except priority_name1;
    flag->allBits = 0; flag->bits.priority_name1 = priorityName1;
    
    if ((p = StrChr(s, ')')))  {    // exist
        if (*s == '-') flag->bits.lunar = 1;
        else if (*s == '#') flag->bits.lunar_leap = 1;
        else return false;
        s = p+1;
    }
    else flag->bits.solar = 1;   // default is solar

    switch (gPrefdfmts) {
    case dfDMYWithDots:          // 31.12.95
    case dfYMDWithDots:          // 95.12.31
        delimeter = '.';
        break;
    case dfDMYWithDashes:        // 31-12-95
    case dfYMDWithDashes:        // 95-12-31
        delimeter = '-';
        break;
    case dfYMDWithSlashes:       // 95/12/31
    case dfDMYWithSlashes:       // 31/12/95
    case dfMDYWithSlashes:       // 12/31/95
    default:
        delimeter = '/';
        break;
    }
    
    p = s;
    // find the number of delimeter
    while ((p = StrChr(p, delimeter))) {
        numOfDelimeter++;
        p++;
    }
    
    p = s;
    if (numOfDelimeter < 1 || numOfDelimeter > 2) return false;
    else if (numOfDelimeter == 2) {
        p = StrChr(p, delimeter); *p = 0; p++;
        num[numIdx++] = StrAToI(s);
        s = p;
    }
    
    p = StrChr(s, delimeter); *p = 0; p++;
    num[numIdx++] = StrAToI(s);
    s = p;
    
	if (*s == 0) flag->bits.year = 0;
	else {
		flag->bits.year = 1;
		num[numIdx] = StrAToI(s);
	}

    if (numOfDelimeter == 1) {
        flag->bits.year = 0;
    }

    switch (gPrefdfmts) {
    case dfDMYWithSlashes:       // 31/12/95
    case dfDMYWithDots:          // 31.12.95
    case dfDMYWithDashes:        // 31-12-95
        if (numOfDelimeter == 2) {
            year = num[numIdx--];
        }
        month = num[numIdx--];
        day = num[numIdx];
        break;
    case dfYMDWithSlashes:       // 95/12/31
    case dfYMDWithDots:          // 95.12.31
    case dfYMDWithDashes:        // 95-12-31
        day = num[numIdx--];
        month = num[numIdx--];

        if (numOfDelimeter == 2) {
            year = num[numIdx];
        }
        break;
    case dfMDYWithSlashes:       // 12/31/95
    default:
        if (numOfDelimeter == 2) {
            year = num[numIdx--];
        }
        day = num[numIdx--];
        month = num[numIdx];
        break;
    }

    if (month < 1 || month > 12)
        return false;
    if (day < 1 || day > 31)
        return false;

    if (flag->bits.year) {
        if (year >= 0 && year <= 31) *dYear = year + 2000;          // 2000
        else if (year > 31 && year <= 99) *dYear = year + 1900;     // 1900
        else if (year >= 1900) *dYear = year;
        else return false;
    }
    *dMonth = month; *dDay = day;

    return true;
}

static void CleanupHappyDaysCache(DmOpenRef dbP)
{
    UInt16 currindex = 0;     
    MemHandle recordH = 0;

    if (dbP) {
        while (1) {
            recordH = DmQueryNextInCategory(dbP, &currindex,
                                            dmAllCategories);
            if (!recordH) break;
            
            DmRemoveRecord(dbP, currindex);     // remove all traces
        }
    }
}

static void UnderscoreToSpace(Char *src)
{
    // change _ into ' '
    //
    while (*src) {
        if (*src == '_') *src = ' ';
        src++;
    }
}

static void HDFindKey(PackedHappyDays* r, char **key, Int16* whichKey)
{
    if (*whichKey == 1) {       // start
        if (r->name[0] == 0) {
            *whichKey = 3;      // end
            *key =  &(r->name[1]);
            return;
        }
        else {
            *whichKey = 2;      // second
            *key = r->name;
            return;
       }
    }
    else if (*whichKey == 2) {
        *whichKey = 3;
        *key = r->name + StrLen(r->name) + 1;
        return;
    }
    else {
        *key = 0;
        return;
    }
}

// null fields are considered less than others
//
static Int16 HDComparePackedRecords(PackedHappyDays *rec1,
                                     PackedHappyDays *rec2,
                                     Int16 unusedInt,
                                     SortRecordInfoPtr unused1,
                                     SortRecordInfoPtr unused2,
                                     MemHandle appInfoH)
{
    Int16 result;
    Int16 whichKey1 = 1, whichKey2 = 1;
    char *key1, *key2;

    do {
        HDFindKey(rec1, &key1, &whichKey1);
        HDFindKey(rec2, &key2, &whichKey2);

        if (!key1 || *key1 == 0) {
            if (!key2 || *key2 == 0) {
                result = 0;
                return result;
            }
            else result = -1;
        }
        else if (!key2 || *key2 == 0) result = 1;
        else {
            result = StrCaselessCompare(key1, key2);
            if (result == 0)
                result = StrCompare(key1, key2);
        }
    } while (!result);

    return result;
}

static UInt16 HDFindSortPosition(DmOpenRef dbP, PackedHappyDays* newRecord)
{
    return DmFindSortPosition(dbP, (void*)newRecord, 0,
                               (DmComparF *)HDComparePackedRecords, 0);
}

static Int16 HDNewRecord(DmOpenRef dbP, HappyDays *r, Int16 *index)
{
    MemHandle recordH;
    PackedHappyDays* recordP;
    Int16 err;
    UInt16 newIndex;

    // 1) and 2) (make a new chunk with the correct size)
    recordH = DmNewHandle(dbP, (Int16)HDPackedSize(r));
    if (recordH == NULL)
        return dmErrMemError;

    // 3) Copy the data from the unpacked record to the packed one.
    recordP = MemHandleLock(recordH);
    PackHappyDays(r, recordP);

    // Get the index
    newIndex = HDFindSortPosition(dbP, recordP);
    MemPtrUnlock(recordP);

    // 4) attach in place
    err = DmAttachRecord(dbP, &newIndex, recordH, 0);
    if (err)
        MemHandleFree(recordH);
    else *index = newIndex;

    return err;
}

                          
static Int16 AnalOneRecord(UInt16 addrattr, Char* src,
                  HappyDays* hd, Boolean *ignore)
{
    Char* p;
    UInt16 index;
    Int16 err;
    Int16 year, month, day;

	while (*src == ' ' || *src == '\t') src++;    // skip white space

    // ignore record with exclamation mark
    if (*src == '!') {
        if (gPrefsR.Prefs.ignoreexclamation) return 0;
        else src++;
    }
    
    if (*src == '*') {
        
        // this is multiple event
        hd->flag.bits.multiple_event = true;
        if ((p = StrChr(src, ' '))) *p = 0;
        else goto ErrHandler;
    
        if ( *(src+1) != 0 ) {                    // if src have 'custom' tag
            hd->custom = src+1;
            UnderscoreToSpace(src+1);
        }
		else hd->custom = 0;

        src = p+1;
        while (*src == ' ' || *src == '\t') src++;     // skip white space

        if ((p = StrChr(src, ' '))) *p = 0;
        else goto ErrHandler;

        if (*src == '.') {
            hd->name1 = src+1;
            hd->name2 = 0;
        }
        else {
            hd->name2 = src;
        }
        
        UnderscoreToSpace(src);
        
        src = p+1;
        while (*src == ' ' || *src == '\t') src++;     // skip white space
    }
    
    if (!AnalysisHappyDays(src, &hd->flag,
                           &year, &month, &day)) goto ErrHandler;

    // convert into date format
    //
    if (hd->flag.bits.year) {
        if ((year < 1904) || (year > 2031) ) goto ErrHandler;
        else hd->date.year = year - 1904; 
    }
    else hd->date.year = 0;
        
    hd->date.month = month;
    hd->date.day = day;

    // maintain address book order(name order)
    //      list order is determined by sort
    err = HDNewRecord(MainDB, hd, &index);
    if (!err) {
        UInt16 attr;
        // set the category of the new record to the category
        // it belongs in
        DmRecordInfo(MainDB, index, &attr, NULL, NULL);
        attr &= ~dmRecAttrCategoryMask;
        attr |= addrattr;

        DmSetRecordInfo(MainDB, index, &attr, NULL);

        DmReleaseRecord(MainDB, index, true);
    }
    return 0;

 ErrHandler:
    if (*ignore) return 0;
                
    switch (FrmCustomAlert(InvalidFormat,
                           ((!hd->name2) ?
                            ((!hd->name1) ? " " : hd->name1)
                            : hd->name2),
                               " ", " ")) {
    case 0:                 // EDIT
        if (!GotoAddress(hd->addrRecordNum)) return -1;
    case 2:                 // Ignore all
        *ignore = true;
    case 1:                 // Ignore
    }
    return 0;
}


Boolean FindHappyDaysField()
{
    AddrAppInfoPtr addrInfoPtr;
    Int16 i;

    gHappyDaysField = -1;
    
    if ((addrInfoPtr = (AddrAppInfoPtr)AppInfoGetPtr(AddressDB))) {
        for (i= firstRenameableLabel; i <= lastRenameableLabel; i++) {
            if (!StrCaselessCompare(addrInfoPtr->fieldLabels[i],
                                    gPrefsR.Prefs.custom)) {
                gHappyDaysField = i;
                break;
            }
        }
        MemPtrUnlock(addrInfoPtr);
    }
    if (gHappyDaysField < 0) {
        // Custom field에 일치하는 게 없는 경우에는 note field를 조사
        return true;
    }
    return true;
}

Boolean IsChangedAddressDB()
{
    return !((MemCmp((char*) &gAdcdate, gPrefsR.adrcdate, 4) == 0) &&
            (MemCmp((char*) &gAdmdate, gPrefsR.adrmdate, 4) == 0));
}

void  SetReadAddressDB()
{
    MemMove(gPrefsR.adrcdate, (char *)&gAdcdate, 4);
    MemMove(gPrefsR.adrmdate, (char *)&gAdmdate, 4);
}

#define	INDICATE_TOP		110
#define	INDICATE_LEFT		35
#define	INDICATE_HEIGHT		6
#define	INDICATE_WIDTH		15
#define INDICATE_NUM        7

void initIndicate()
{
	RectangleType rect;
	CustomPatternType pattern;

	pattern[0] = pattern[2] = pattern[4] = pattern[6] = 0xaa;
    pattern[1] = pattern[3] = pattern[5] = pattern[7] = 0x55;

	WinSetPattern( (const CustomPatternType*)&pattern );

	rect.topLeft.x = INDICATE_LEFT;	
	rect.topLeft.y = INDICATE_TOP;
	rect.extent.x = INDICATE_WIDTH * (INDICATE_NUM -1);
	rect.extent.y = INDICATE_HEIGHT;

    if (gbVgaExists) {
        rect.topLeft.x += rect.topLeft.x / 2;
        rect.topLeft.y += rect.topLeft.y / 2;
        rect.extent.x += rect.extent.x / 2 + 1;
        rect.extent.y += rect.extent.y / 2;
    }

	WinFillRectangle( &rect, 0 );
	return;
}

void displayNextIndicate( int index )
{
	FormPtr form;
	RectangleType rect;
	
	form = FrmGetActiveForm();
    
    rect.topLeft.x = INDICATE_LEFT + INDICATE_WIDTH * (index-1);
	rect.topLeft.y = INDICATE_TOP;
	rect.extent.x = INDICATE_WIDTH;
	rect.extent.y = INDICATE_HEIGHT;
	
    if (gbVgaExists) {
        rect.topLeft.x += rect.topLeft.x / 2;
        rect.topLeft.y += rect.topLeft.y / 2;
        rect.extent.x += rect.extent.x / 2 + 1;
        rect.extent.y += rect.extent.y / 2;
    }
    
	WinDrawRectangle( &rect, 0 );
	return;
}

Boolean UpdateHappyDaysDB(FormPtr frm)
{
    UInt16 currIndex = 0;
    AddrPackedDBRecord *rp;
    AddrDBRecordType r;
    MemHandle recordH = 0;
    UInt16 recordNum;
    int i = 0, indicateNext;

    // create the happydays cache db
    HappyDays   hd;
    Boolean     ignore = false;         // ignore error record
    Char*       hdField;
    UInt16      addrattr;
    Char        *p, *q, *end;

    // display collecting information
    //
    FrmDrawForm(frm);

    // clean up old database
    //
    CleanupHappyDaysCache(MainDB);

    recordNum = DmNumRecords(AddressDB);
    indicateNext = recordNum / INDICATE_NUM;

    if (recordNum > 50) initIndicate();
            
    while (1) {
        char *name1, *name2;
        Int8 whichField;        // birthday field or note field?
            
        recordH = DmQueryNextInCategory(AddressDB, &currIndex,
                                        dmAllCategories);
        if (!recordH) break;
        if (i++ == indicateNext) {
            if (recordNum > 50) displayNextIndicate(i * INDICATE_NUM / recordNum);
            indicateNext += recordNum / INDICATE_NUM;
        }

        DmRecordInfo(AddressDB, currIndex, &addrattr, NULL, NULL);
        addrattr &= dmRecAttrCategoryMask;      // get category info
                
        rp = (AddrPackedDBRecord*)MemHandleLock(recordH);
        /*
         * Build the unpacked structure for an AddressDB record.  It
         * is just a bunch of pointers into the rp structure.
         */
        AddrUnpack(rp, &r);

        if ((gHappyDaysField <= 0 || !r.fields[gHappyDaysField])
            && !(gPrefsR.Prefs.scannote && r.fields[note]
                 && StrStr(r.fields[note], gPrefsR.Prefs.notifywith) ) ) {
            // not exist in happydays field or note field
            //
            MemHandleUnlock(recordH);
            currIndex++;
            continue;
        }
			
        MemSet(&hd, sizeof(hd), 0);
        hd.addrRecordNum = currIndex;
        if (DetermineRecordName(&r, gSortByCompany,
                                &hd.name1,
                                &hd.name2)) {
            // name 1 has priority;
            hd.flag.bits.priority_name1 = 1;
        }

        // save the temporary name
        name1 = hd.name1;
        name2 = hd.name2;

        if (gHappyDaysField >= 0 && r.fields[gHappyDaysField])
            whichField = gHappyDaysField;
        else whichField = note;

        while (whichField >= 0) {

            if (whichField == note) {
                p = StrStr(r.fields[note], gPrefsR.Prefs.notifywith)
                    + StrLen(gPrefsR.Prefs.notifywith) + 1;

                if ( StrLen(r.fields[note]) < (p - r.fields[note]) ) break;
            }
            else {
                p = r.fields[whichField];
            }
                
            hdField =
                MemPtrNew(StrLen(r.fields[whichField]) - (p - r.fields[whichField])+1);

            SysCopyStringResource(gAppErrStr, NotEnoughMemoryString);
            ErrFatalDisplayIf(!hdField, gAppErrStr);
            
            p = StrCopy(hdField, p);

            if (whichField == note && 
                (end = StrStr(p, gPrefsR.Prefs.notifywith))) {
                // end delimeter
                //
                *end = 0;
            }
            
            while ((q = StrChr(p, '\n'))) {
                // multiple event
                //

                *q = 0;
                if (AnalOneRecord(addrattr, p, &hd, &ignore)) 
					goto Update_ErrHandler;
                p = q+1;

                // restore the saved name
                hd.name1 = name1;
                hd.name2 = name2;
                
                // reset multiple flag
                hd.flag.bits.multiple_event = 0;

                while (*p == ' ' || *p == '\t' || *p == '\n')
                    p++;     // skip white space
            }
            // last record
            if (*p) {
                // check the null '\n'
                if (AnalOneRecord(addrattr, p, &hd, &ignore)) 
					goto Update_ErrHandler;
            }
                
            if (whichField == gHappyDaysField  // next is note field
                && (gPrefsR.Prefs.scannote     // scanNote & exists
                    && r.fields[note]       
                    && StrStr(r.fields[note], gPrefsR.Prefs.notifywith)) ) {
                whichField = note;
            }
            else whichField = -1;

            MemPtrFree(hdField);
        }
        MemHandleUnlock(recordH);
        currIndex++;
    }
	return true;

Update_ErrHandler:
	MemPtrFree(hdField);
	MemHandleUnlock(recordH);
	return false;
}
