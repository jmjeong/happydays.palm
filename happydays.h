/*
HappyDays - A Birthdate displayer for the PalmPilot
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

#ifndef _HAPPYDATE_H_
#define _HAPPYDATE_H_


#define AppErrStrLen        256
#define DEFAULT_DURATION    5

#define HDAppID           	'Jmje'
#define HDAppVer        	16

#define MainDBType          'DATA'
#define MainDBName          "HappyDaysDB"
#define DatebookAppID       'date'
#define DatebookDBName      "DatebookDB"
#define ToDoAppID           'todo'
#define ToDoDBName          "ToDoDB"
#define AddressAppID        'addr'
#define AddressDBName       "AddressDB"
#define MemoAppID           'memo'
#define MemoDBName          "MemoDB"

#define DATEBK3_MEMO_STRING "DATEBK3\n"       // datebk3 icon in memo list

/* DateBook Notify Preferences */
struct sDBNotifyPrefs 
{
    char alarm;             // alarm set?
    char icon;              // icon?
    char datebk3icon;       // datebook 3 icon
    int  notifybefore;      // prenotify duration
    int  duration;          // interval of notified records
    TimeType when;          // no-time
    char an_icon[5];        // action name icon?
};

/* ToDo Notify Preferences */
struct sTDNotifyPrefs 
{
    char priority;			// priority of TODO
    char todoCategory[dmCategoryLength];
};

struct sBirthPrefs 
{
    char custom[12+1];          // addr label length
    char notifywith[5+1];       // notify with (HD)
    char sort;                  // sorting order
    char notifyformat;          // notify format list
    char scannote;              // scan from notes?
    char addrapp;               // address appid(used for GOTO operation)
    Boolean sysdateover;        // override system date format
    DateFormatType dateformat;  // if sysdateover is true
};

struct sDispPrefs
{
    char emphasize;             // emphasize lunar
    char dispextrainfo;         // display extra day information
    char dispsexagenary;  
};

struct sPrefsR 
{
    char version;           // version number
    char records;			// all or selected(0: all, 1: selected)
    char existing;			// keep or modify(0: keep, 1: modify)
    char private;			// (0: public, 1: private)

    struct sDBNotifyPrefs DBNotifyPrefs;
    struct sTDNotifyPrefs TDNotifyPrefs;
    struct sBirthPrefs    BirthPrefs;
    struct sDispPrefs     DispPrefs;

    // HideSecretRecord in system global preference
    Boolean gHideSecretRecord;
    
    char addrCategory[dmCategoryLength];
    char adrcdate[4], adrmdate[4];      // address book create/modify date
    char memcdate[4], memmdate[4];      // memo create/modify date

	FontID listFont;		// HappyDays list font
};

typedef union 
{
    struct      
    {
        unsigned reserverd:10;
        unsigned multiple_event:1;      // this is multiple event(for check)
        unsigned year:1;                // date contains year field
        unsigned priority_name1:1;      // name1 has priority?
        unsigned lunar_leap :1;         // lunar leap month?
        unsigned lunar      :1;         // lunar calendar?
        unsigned solar      :1;         // solar calendar?
    } bits;
    Int16 allBits;          // 16 bit
} BirthdateFlag;

typedef struct 
{
    UInt32 addrRecordNum;   // address book record number
    DateType date;          // orignal address book birthdate 
    BirthdateFlag flag;     // about birthdate flag
    char *name1;
    char *name2;
    char *custom;
} BirthDate;

typedef struct
{
    UInt32 addrRecordNum;
    DateType date;
    BirthdateFlag flag;
    char name[1];           // actually may be longer than 1
} PackedBirthDate;

#define INVALID_CONV_DATE       127

//
// Table row item, used for sort
typedef struct 
{
    UInt16      birthRecordNum;     // birthdate+ DB record num
	Int8        age;				// calculated age	
    DateType    date;               // converted date(the incoming birthday)
} LineItemType;

typedef LineItemType* LineItemPtr;

#ifndef MAX
#define MAX(_a,_b) ((_a)<(_b)?(_b):(_a))
#endif
#ifndef MIN
#define MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#endif

// birthdate list
extern MemHandle gTableRowHandle;
extern struct sPrefsR gPrefsR;
extern Char gAppErrStr[AppErrStrLen];
extern Boolean gPrefsRdirty;
extern Boolean gSortByCompany;
extern DmOpenRef MainDB;
extern DmOpenRef AddressDB;
extern UInt32 gDbcdate, gDbmdate, gAdcdate, gAdmdate;
extern DateFormatType gPrefdfmts;  // global date format for Birthday field
extern DateFormatType gDispdfmts;  // global date format for display

#endif
