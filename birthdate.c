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


#include "birthdate.h"
#include "util.h"
#include "happydays.h"
#include "address.h"
#include "happydaysRsc.h"

void PackBirthdate(BirthDate *birthdate, void* recordP)
{
    UInt16 offset = 0;

    DmWrite(recordP, offset, (Char*)&birthdate->addrRecordNum,
            sizeof(birthdate->addrRecordNum));
    offset += sizeof(birthdate->addrRecordNum);
    DmWrite(recordP, offset, (Char*)&birthdate->date,
            sizeof(birthdate->date));
    offset += sizeof(birthdate->date);
    DmWrite(recordP, offset, (Char*)&birthdate->flag,
            sizeof(birthdate->flag));
    offset += sizeof(birthdate->flag);  // for corealign

    if (birthdate->name1) {
        DmStrCopy(recordP, offset, (Char*)birthdate->name1);
        offset += StrLen(birthdate->name1) +1;
    }
    else {
        DmStrCopy(recordP, offset, "\0");
        offset += 1;
    }
    if (birthdate->name2) {
        DmStrCopy(recordP, offset, (Char*)birthdate->name2);
        offset += StrLen(birthdate->name2) +1;
    }
    else {
        DmStrCopy(recordP, offset,"\0");
        offset += 1;
    }

    if (birthdate->custom) {
        DmStrCopy(recordP, offset, (Char*)birthdate->custom);
        offset += StrLen(birthdate->custom) +1;
    }
    else {
        DmStrCopy(recordP, offset,"\0");
        offset += 1;
    }
}

void UnpackBirthdate(BirthDate *birthdate,
                            const PackedBirthDate *packedBirthdate)
{
    char *s = packedBirthdate->name;

    birthdate->addrRecordNum = packedBirthdate->addrRecordNum;
    birthdate->date = packedBirthdate->date;
    birthdate->flag = packedBirthdate->flag;

    birthdate->name1 = s;
    s += StrLen(s) + 1;
    birthdate->name2 = s;
    s += StrLen(s) + 1;
    birthdate->custom = s;
    s += StrLen(s) + 1;
}

static Int16 BirthPackedSize(BirthDate* birthdate)
{
    Int16 size;

    size = sizeof(PackedBirthDate) - sizeof(char);    // corect
    if (birthdate->name1) size += StrLen(birthdate->name1) +1;
    else size += 1;     // '\0'

    if (birthdate->name2) size += StrLen(birthdate->name2) +1;
    else size += 1;     // '\0'

    if (birthdate->custom) size+= StrLen(birthdate->custom) +1;
    else size+= 1;

    return size;
}

Int16 CompareBirthdateFunc(LineItemPtr p1, LineItemPtr p2, Int32 extra)
{   
    return DateCompare(p1->date, p2->date);
}

UInt16 AddrGetBirthdate(DmOpenRef dbP, UInt16 AddrCategory)
{
    UInt16 totalItems;
    UInt16 recordNum = 0;
    MemHandle recordH = 0;
    LineItemPtr ptr;
    PackedBirthDate* rp;
    BirthDate r;
    DateType dt;
    UInt16 currindex = 0;

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

                    rp = (PackedBirthDate *) MemHandleLock(recordH);
                    /*
                     * Build the unpacked structure for an AddressDB
                     * record.  It is just a bunch of pointers into the rp
                     * structure.
                     */
                    UnpackBirthdate(&r, rp);

                    // original r(in MainDB) not changed
                    //     local change for LineItemType
                    //
                    if (r.flag.bits.lunar || r.flag.bits.lunar_leap) {
                        // now use the same routine in lunar an lunar_leap
                        //
                        DateType current;
                        DateSecondsToDate(TimGetSeconds(), &current);

                        if (!FindNearLunar(&r.date, current,
                                           r.flag.bits.lunar_leap)) {
                            // ignore the records
                            //
                            r.date.year = INVALID_CONV_DATE;
                            // max year(for sorting)
                            //   indicate for invalid date
                            
                        }
                    }
                    else if (r.flag.bits.solar) {
                        int maxtry = 4;
                        // get current date
                        //
                        DateSecondsToDate(TimGetSeconds(), &dt);

                        if (r.date.month < dt.month ||
                            ( r.date.month == dt.month
                              && r.date.day < dt.day)) {
                            // birthdate is in last year?
                            while (DaysInMonth(r.date.month, ++dt.year) <
                                   r.date.day && maxtry-- > 0) 
                                ;
                        }
                        else {
                            while (DaysInMonth(r.date.month, dt.year) <
                                   r.date.day && maxtry-- >0) {
                                dt.year++;
                            }
                            
                        }

                        if (maxtry >0) r.date.year = dt.year;
                        else {
                            r.date.year = INVALID_CONV_DATE;
                            // max year
                            //   indicate for invalid date
                        }
                    }
                    // save the converted data
                    //
                    ptr[recordNum].date = r.date;

                    MemHandleUnlock(recordH);
                    currindex++;
                }
            }
            
            // sort the order if sort order is converted date
            //
            if (gPrefsR->BirthPrefs.sort == 1) {      // date sort
                SysInsertionSort(ptr, totalItems, sizeof(LineItemType),
                                 (_comparF *)CompareBirthdateFunc, 0L);
            }
            
            MemPtrUnlock(ptr);
        } else return 0;
    }
    
    return totalItems;
}


Boolean AnalysisBirthDate(const char* birthdate,
                          BirthdateFlag *flag,
                          Int16* dYear, Int16* dMonth, Int16* dDay)
{
    Char * s = (Char *)birthdate;
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
    
    num[numIdx] = StrAToI(s);

    if (numOfDelimeter == 1) {
        flag->bits.year = 0;
    }
    else {
        flag->bits.year = 1;
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
        if (year > 0 && year < 100) *dYear = year + 1900;
        else if (year >= 100) *dYear = year;
        else if (year == 0) {
            flag->bits.year = 0;
            *dYear = 0;
        }
        else return false;
    }
    *dMonth = month; *dDay = day;

    return true;
}

static void CleanupBirthdateCache(DmOpenRef dbP)
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

static void BirthFindKey(PackedBirthDate* r, char **key, Int16* whichKey)
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
static Int16 BirthComparePackedRecords(PackedBirthDate *rec1,
                                     PackedBirthDate *rec2,
                                     Int16 unusedInt,
                                     SortRecordInfoPtr unused1,
                                     SortRecordInfoPtr unused2,
                                     MemHandle appInfoH)
{
    Int16 result;
    Int16 whichKey1 = 1, whichKey2 = 1;
    char *key1, *key2;

    do {
        BirthFindKey(rec1, &key1, &whichKey1);
        BirthFindKey(rec2, &key2, &whichKey2);

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

static UInt16 BirthFindSortPosition(DmOpenRef dbP, PackedBirthDate* newRecord)
{
    return DmFindSortPosition(dbP, (void*)newRecord, 0,
                               (DmComparF *)BirthComparePackedRecords, 0);
}

static Int16 BirthNewRecord(DmOpenRef dbP, BirthDate *r, Int16 *index)
{
    MemHandle recordH;
    PackedBirthDate* recordP;
    Int16 err;
    UInt16 newIndex;

    // 1) and 2) (make a new chunk with the correct size)
    recordH = DmNewHandle(dbP, (Int16)BirthPackedSize(r));
    if (recordH == NULL)
        return dmErrMemError;

    // 3) Copy the data from the unpacked record to the packed one.
    recordP = MemHandleLock(recordH);
    PackBirthdate(r, recordP);

    // Get the index
    newIndex = BirthFindSortPosition(dbP, recordP);
    MemPtrUnlock(recordP);

    // 4) attach in place
    err = DmAttachRecord(dbP, &newIndex, recordH, 0);
    if (err)
        MemHandleFree(recordH);
    else *index = newIndex;

    return err;
}

                          
static Int16 AnalOneRecord(UInt16 addrattr, Char* src,
                  BirthDate* birthdate, Boolean *ignore)
{
    Char* p;
    UInt16 index;
    Int16 err;
    Int16 year, month, day;

	while (*src == ' ' || *src == '\t') src++;    // skip white space
    if (*src == '*') {
        
        // this is multiple event
        birthdate->flag.bits.multiple_event = true;
        if ((p = StrChr(src, ' '))) *p = 0;
        else goto ErrHandler;
    
        if ( *(src+1) != 0 ) {                    // if src have 'custom' tag
            birthdate->custom = src+1;
            UnderscoreToSpace(src+1);
        }
		else birthdate->custom = 0;

        src = p+1;
        while (*src == ' ' || *src == '\t') src++;     // skip white space

        if ((p = StrChr(src, ' '))) *p = 0;
        else goto ErrHandler;

        if (*src == '.') {
            birthdate->name1 = src+1;
            birthdate->name2 = 0;
        }
        else {
            birthdate->name2 = src;
        }
        
        UnderscoreToSpace(src);
        
        src = p+1;
        while (*src == ' ' || *src == '\t') src++;     // skip white space
    }
    
    if (!AnalysisBirthDate(src, &birthdate->flag,
                           &year, &month, &day)) goto ErrHandler;

    // convert into date format
    //
    if (birthdate->flag.bits.year) {
        if ((year < 1904) || (year > 2031) ) goto ErrHandler;
        else birthdate->date.year = year - 1904; 
    }
    else birthdate->date.year = 0;
        
    birthdate->date.month = month;
    birthdate->date.day = day;

    // maintain address book order(name order)
    //      list order is determined by sort
    err = BirthNewRecord(MainDB, birthdate, &index);
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
                           ((!birthdate->name2) ?
                            ((!birthdate->name1) ? " " : birthdate->name1)
                            : birthdate->name2),
                               " ", " ")) {
    case 0:                 // EDIT
        if (!GotoAddress(birthdate->addrRecordNum)) return -1;
    case 2:                 // Ignore all
        *ignore = true;
    case 1:                 // Ignore
    }
    return 0;
}

Int16 UpdateBirthdateDB(DmOpenRef dbP, FormPtr frm)
{
    AddrAppInfoPtr addrInfoPtr;
    UInt16 currIndex = 0;
    AddrPackedDBRecord *rp;
    AddrDBRecordType r;
    MemHandle recordH = 0;
    Int16 i;

    gBirthDateField = -1;
    // use for next version, will support Address+, ActionNames.. etc
    // if (!FindAddressApp('addr', &AcardNo, &AdbID)) return -1;
    
    if ((addrInfoPtr = (AddrAppInfoPtr)AppInfoGetPtr(AddressDB))) {
        for (i= firstRenameableLabel; i <= lastRenameableLabel; i++) {
            if (!StrCaselessCompare(addrInfoPtr->fieldLabels[i],
                                    gPrefsR->BirthPrefs.custom)) {
                gBirthDateField = i;
                break;
            }
        }
        MemPtrUnlock(addrInfoPtr);
    }
    if (gBirthDateField < 0) {
        return ENONBIRTHDATEFIELD;
    }

    if (!MainDB) return EDBCREATE;

    // cache the birthdate DB for the performance
    //
    if ((MemCmp((char*) &gAdcdate, gPrefsR->adrcdate, 4) == 0) &&
        (MemCmp((char*) &gAdmdate, gPrefsR->adrmdate, 4) == 0)) {
        // if matched, use the original birthdate db
        //
    }
    else {
        // create the birthdate cache db
        BirthDate   birthdate;
        Boolean     ignore = false;         // ignore error record
        Char*       birthdateField;
        UInt16      addrattr;
        Char        *p, *q, *end;

        // display collecting information
        //
        FrmDrawForm(frm);

        // clean up old database
        //
        CleanupBirthdateCache(MainDB);
            
        while (1) {
            char *name1, *name2;
            Int8 whichField;        // birthday field or note field?
            
            recordH = DmQueryNextInCategory(AddressDB, &currIndex,
                                            dmAllCategories);
            if (!recordH) break;

            DmRecordInfo(AddressDB, currIndex, &addrattr, NULL, NULL);
            addrattr &= dmRecAttrCategoryMask;      // get category info
                
            rp = (AddrPackedDBRecord*)MemHandleLock(recordH);
            /*
             * Build the unpacked structure for an AddressDB record.  It
             * is just a bunch of pointers into the rp structure.
             */
            AddrUnpack(rp, &r);

            if (!r.fields[gBirthDateField]
                && !(gPrefsR->BirthPrefs.scannote && r.fields[note]
                     && StrStr(r.fields[note], gPrefsR->BirthPrefs.notifywith) ) ) {
                // not exist in birthdate field or note field
                //
                MemHandleUnlock(recordH);
                currIndex++;
                continue;
            }
			
			MemSet(&birthdate, sizeof(birthdate), 0);
            birthdate.addrRecordNum = currIndex;
            if (DetermineRecordName(&r, gSortByCompany,
                                    &birthdate.name1,
                                    &birthdate.name2)) {
                // name 1 has priority;
                birthdate.flag.bits.priority_name1 = 1;
            }

            // save the temporary name
            name1 = birthdate.name1;
            name2 = birthdate.name2;

            whichField = (r.fields[gBirthDateField]) ? gBirthDateField : note;

            while (whichField >= 0) {

                if (whichField == note) {
                    p = StrStr(r.fields[note], gPrefsR->BirthPrefs.notifywith)
                        + StrLen(gPrefsR->BirthPrefs.notifywith)+1;
                }
                else {
                    p = r.fields[whichField];
                }
                
                birthdateField =
                    MemPtrNew(StrLen(r.fields[whichField]) - (p - r.fields[whichField])+1);

                SysCopyStringResource(gAppErrStr, NotEnoughMemoryString);
                ErrFatalDisplayIf(!birthdateField, gAppErrStr);
            
                p = StrCopy(birthdateField, p);

				if (whichField == note && 
					(end = StrStr(p, gPrefsR->BirthPrefs.notifywith))) {
					// end delimeter
					//
					*end = 0;
				}
            
                while ((q = StrChr(p, '\n'))) {
                    // multiple event
                    //

                    *q = 0;
                    if (AnalOneRecord(addrattr, p, &birthdate, &ignore)) return 0;
                    p = q+1;

                    // restore the saved name
                    birthdate.name1 = name1;
                    birthdate.name2 = name2;
                
                    // reset multiple flag
                    birthdate.flag.bits.multiple_event = 0;

                    while (*p == ' ' || *p == '\t' || *p == '\n')
                        p++;     // skip white space
                }
                // last record
                if (*p) {
                    // check the null '\n'
                    if (AnalOneRecord(addrattr, p, &birthdate, &ignore)) return 0;
                }
                
                if (whichField == gBirthDateField       // next is note field
                    && (gPrefsR->BirthPrefs.scannote    // scanNote & exists
                        && r.fields[note]       
                        && StrStr(r.fields[note], gPrefsR->BirthPrefs.notifywith)) ) {
                    whichField = note;
                }
                else whichField = -1;

                MemPtrFree(birthdateField);
            }
            MemHandleUnlock(recordH);
            currIndex++;
        }

        MemMove(gPrefsR->adrcdate, (char *)&gAdcdate, 4);
        MemMove(gPrefsR->adrmdate, (char *)&gAdmdate, 4);
        gPrefsRdirty = true;
    }
    
    return 0;
}
