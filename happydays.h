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

#ifndef _HAPPYDATE_H_
#define _HAPPYDATE_H_

#define AppErrStrLen        256
#define DEFAULT_DURATION    5
#define MAINTABLEAGEFIELD   24

#define MainAppID           'Jmje'
#define MainDBType          'DATA'
#define MainDBName          "HappyDaysDB-Jmje"
#define PrefsDBType         'DATA'
#define PrefsDBName         "HappyDaysPrefs-Jmje"
#define DatebookAppID       'date'
#define DatebookDBName      "DatebookDB"
#define AddressAppID        'addr'
#define AddressDBName       "AddressDB"
#define MemoAppID           'memo'
#define MemoDBName          "MemoDB"

#define DATEBK3_ICON		"##@@@J@@@"		// birthday-cake icon

/* Notify Preferences */
struct sNotifyPrefs 
{
    char records;
    char existing;
    char private;
	char datebk3;			// datebk3 icon used? 
    char alarm;             // alarm set?
    int  notifybefore;      // prenotify duration
    int  duration;          // interval of notified records
    TimeType when;          // no-time
};

struct sBirthPrefs 
{
    char custom[12+1];          // addr label length
    char notifywith[5+1];       // notify with (HD)
    char sort;                  // sorting order
    char internal;              // internal option(used for emphasize lunar)
};

struct sPrefsR 
{
    struct sNotifyPrefs NotifyPrefs;
    struct sBirthPrefs  BirthPrefs;

    // HideSecretRecord in system global preference
    Boolean gHideSecretRecord;
    
    char addrCategory[33];
    char adrcdate[4], adrmdate[4];      // address book create/modify date
};


typedef union 
{
    struct      
    {
        unsigned reserverd:11;
        unsigned year:1;                // date contains year field
        unsigned priority_name1:1;      // name1 has priority?
        unsigned lunar_leap :1;         // lunar leap month?
        unsigned lunar      :1;         // lunar calendar?
        unsigned solar      :1;         // solar calendar?
    } bits;
    SWord allBits;          // 16 bit
} BirthdateFlag;

typedef struct 
{
    DWord addrRecordNum;    // address book record number
    DateType date;          // orignal address book birthdate 
    BirthdateFlag flag;     // about birthdate flag
    char *name1;
    char *name2;
    char *custom;
} BirthDate;

typedef struct
{
    DWord addrRecordNum;
    DateType date;
    BirthdateFlag flag;
    char name[1];           // actually may be longer than 1
} PackedBirthDate;

#define INVALID_CONV_DATE       127

//
// Table row item, used for sort
typedef struct 
{
    UInt birthRecordNum;    // birthdate+ DB record num
    DateType    date;       // converted date
} LineItemType;

typedef LineItemType* LineItemPtr;

#ifndef MAX
#define MAX(_a,_b) ((_a)<(_b)?(_b):(_a))
#endif
#ifndef MIN
#define MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#endif

#define         ENONBIRTHDATEFIELD      -100
#define         EDBCREATE               -101

// birthdate list
extern VoidHand gTableRowHandle;
extern struct sPrefsR *gPrefsR;
extern Char gAppErrStr[AppErrStrLen];
extern Boolean gPrefsRdirty;
extern Boolean gSortByCompany;
extern DmOpenRef MainDB;
extern DmOpenRef AddressDB;
extern DWord gDbcdate, gDbmdate, gAdcdate, gAdmdate;
extern DateFormatType gPrefdfmts;  // global data format

#endif
