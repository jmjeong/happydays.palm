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
#define HDAppVer        	25

#define MainDBType          'DATA'
#define MainDBName          "HappyDaysDB"
#define MainAppID           'Jmje'

#define DatebookAppID       'date'
#define ToDoAppID           'todo'
#define AddressAppID        'addr'
#define MemoAppID           'memo'

#define NewDatebookAppID    'PAdd'
#define NewAddrAppID        'PDat'
#define NewTaskAppID        'PTod'
#define NewMemoAppID        'PMem'

#define DATEBK3_MEMO_STRING "DATEBK3\n"       // datebk3 icon in memo list

#define HappydaysEventNoteTitle "= HappyDays Notes"

struct sPrefsR 
{
    char version;           // version number
    char records;			// all or selected(0: all, 1: selected)
    char existing;			// keep or modify(0: keep, 1: modify)
    char private;			// (0: public, 1: private)

    // Datebook Prefs
    char alarm;             // alarm set?
    int  notifybefore;      // prenotify duration
    int  duration;          // interval of notified records
    TimeType when;          // no-time
    UInt16 apptCategory;
    char dusenote;           // icon?
    char dnote[135];          // note field

    // Todo Prefs
    char priority;			// priority of TODO
    UInt16 todoCategory;
	char tusenote;			// use note?
	char tnote[135];			// note field

    // Prefs
    char custom[12+1];          // addr label length
    char notifywith[5+1];       // notify with (HD)
    char sort;                  // sortbing order
    char notifyformat;          // notify format list
    char notifyformatstring[40];// actual string
    char autoscan;              // Automatic scan of address
	char ignoreexclamation;		// ignore the record with prefix '!'
    char scannote;              // scan from notes?
    char addrapp;               // address appid(used for GOTO operation)
    Boolean sysdateover;        // override system date format
    DateFormatType dateformat;  // if sysdateover is true

    // Display Prefs
    char emphasize;             // emphasize lunar

    // HideSecretRecord in system global preference
    Boolean gHideSecretRecord;
    
    char addrCategory[dmCategoryLength];
    char adrcdate[4], adrmdate[4];      // address book create/modify date

	FontID  listFont;		            // HappyDays list font
    UInt16  eventNoteIndex;             // Event Note Index in MemoDB
    Boolean eventNoteExists;
};

typedef union 
{
    struct      
    {
        unsigned int reserverd:9;
        unsigned int nthdays:1;             // n-th day
        unsigned int multiple_event:1;      // this is multiple event(for check)
        unsigned int year:1;                // date contains year field
        unsigned int priority_name1:1;      // name1 has priority?
        unsigned int lunar_leap :1;         // lunar leap month?
        unsigned int lunar      :1;         // lunar calendar?
        unsigned int solar      :1;         // solar calendar?
    } bits;
    Int16 allBits;          // 16 bit
} HappyDaysFlag;

typedef struct 
{
    UInt32              addrRecordNum;      // address book record number
    DateType            date;               // orignal address book birthdate 
    HappyDaysFlag       flag;               // about birthdate flag
    Int16               nth;                // valid only if nthdays is on
    char *name1;
    char *name2;
    char *custom;
} HappyDays;

typedef struct
{
    UInt32              addrRecordNum;
    DateType            date;
    HappyDaysFlag       flag;
    Int16               nth;                // valid only if nthdays is on
    char                name[1];            // actually may be longer than 1
} PackedHappyDays;

#define INVALID_CONV_DATE       127

//
// Table row item, used for sort
typedef struct 
{
    UInt16      birthRecordNum;     // birthdate+ DB record num
	Int8        age;				// calculated age	
    DateType    date;               // converted date(the incoming birthday)
} LineItemType;

typedef struct
{
    Char    name[21];               // event type name
    UInt16  start;                  // starting position
    UInt16  len;                    // length
} EventNoteInfo;

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
extern DmOpenRef DatebookDB;
extern DmOpenRef ToDoDB;
extern DmOpenRef MemoDB;

extern UInt32 gDbcdate, gDbmdate, gAdcdate, gAdmdate;
extern DateFormatType gPrefdfmts;  // global date format for Birthday field
extern DateFormatType gDispdfmts;  // global date format for display

#endif
