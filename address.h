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

#ifndef __ADDRESS_H__
#define __ADDRESS_H__

#include "birthdate.h"

#define shortenedFieldString              "\205"
#define shortenedFieldLength              1
#define fieldSeparatorString              ", "
#define fieldSeparatorLength              2
#define spaceBetweenNamesAndPhoneNumbers  4
#define addrLabelLength                   16
#define addrNumFields                     19
#define PhoneColumnWidth                  90

typedef union {
    struct {
        unsigned reserved    :13;
        unsigned note        :1;    /* set if record contains a note handle */
        unsigned custom4     :1;    /* set if record contains a custom4 */
        unsigned custom3     :1;    /* set if record contains a custom3 */
        unsigned custom2     :1;    /* set if record contains a custom2 */
        unsigned custom1     :1;    /* set if record contains a custom1 */
        unsigned title       :1;    /* set if record contains a title */
        unsigned country     :1;    /* set if record contains a birthday */
        unsigned zipCode     :1;    /* set if record contains a birthday */
        unsigned state       :1;    /* set if record contains a birthday */
        unsigned city        :1;    /* set if record contains a birthday */
        unsigned address     :1;    /* set if record contains a address */
        unsigned phone5      :1;    /* set if record contains a phone5 */
        unsigned phone4      :1;    /* set if record contains a phone4 */
        unsigned phone3      :1;    /* set if record contains a phone3 */
        unsigned phone2      :1;    /* set if record contains a phone2 */
        unsigned phone1      :1;    /* set if record contains a phone1 */
        unsigned company     :1;    /* set if record contains a company */
        unsigned firstName   :1;    /* set if record contains a firstName */
        unsigned name        :1;    /* set if record contains a name (bit 0) */
    } bits;
    UInt32 allBits;
} AddrDBRecordFlags;

typedef struct {
    unsigned reserved       :7;
    unsigned sortByCompany  :1;
} AddrDBMisc;

typedef enum {
    name,
    firstName,
    company,
    phone1,
    phone2,
    phone3,
    phone4,
    phone5,
    address,
    city,
    state,
    zipCode,
    country,
    title,
    custom1,
    custom2,
    custom3,
    custom4,
    note,            /* This field is assumed to be < 4K */
    addressFieldsCount
} AddressFields;

typedef enum {
    workLabel,
    homeLabel,
    faxLabel,
    otherLabel,
    emailLabel,
    mainLabel,
    pagerLabel,
    mobileLabel
} AddressPhoneLabels;

#define firstAddressField name
#define firstPhoneField phone1
#define lastPhoneField phone5
#define numPhoneLabels 8
#define numPhoneFields (lastPhoneField - firstPhoneField + 1)
#define numPhoneLabelsStoredFirst numPhoneFields
#define numPhoneLabelsStoredSecond (numPhoneLabels - numPhoneLabelsStoredFirst)
#define firstRenameableLabel custom1
#define lastRenameableLabel custom4
#define lastLabel (addressFieldsCount + numPhoneLabelsStoredSecond)

typedef union {
    struct {
        unsigned reserved:8;
        /* The phone displayed for the list view 0 - 4 */
        unsigned displayPhoneForList:4;
        /* Which phone (home, work, car, ...) */
        unsigned phone5:4;
        unsigned phone4:4;
        unsigned phone3:4;
        unsigned phone2:4;
        unsigned phone1:4;
    } phones;
    UInt32 phoneBits;
} AddrOptionsType;

/*
 * AddrDBRecord.
 *
 * This is the unpacked record form as used by the app.  Pointers are 
 * either NULL or point to strings elsewhere on the card.  All strings 
 * are null character terminated.
 */
typedef struct {
    AddrOptionsType options; /* Display by company or by name */
    Char * fields[addressFieldsCount];
} AddrDBRecordType;

typedef AddrDBRecordType * AddrDBRecordPtr;

/*
 * The following structure doesn't really exist.  The first field
 * varies depending on the data present.  However, it is convient
 * (and less error prone) to use when accessing the other information.
 */
typedef struct {
    AddrOptionsType options; /* Display by company or by name */
    AddrDBRecordFlags flags;
    unsigned char companyFieldOffset; /* Offset from firstField */
    char firstField;
} AddrPackedDBRecord;

typedef union {
    struct {
        unsigned reserved    :10;
        unsigned phone8      :1;    /* set if phone8 label is dirty */
        unsigned phone7      :1;    /* set if phone7 label is dirty */
        unsigned phone6      :1;    /* set if phone6 label is dirty */
        unsigned note        :1;    /* set if note label is dirty */
        unsigned custom4     :1;    /* set if custom4 label is dirty */
        unsigned custom3     :1;    /* set if custom3 label is dirty */
        unsigned custom2     :1;    /* set if custom2 label is dirty */
        unsigned custom1     :1;    /* set if custom1 label is dirty */
        unsigned title       :1;    /* set if title label is dirty */
        unsigned country     :1;    /* set if country label is dirty */
        unsigned zipCode     :1;    /* set if zipCode label is dirty */
        unsigned state         :1;  /* set if state label is dirty */
        unsigned city         :1;   /* set if city label is dirty */
        unsigned address     :1;    /* set if address label is dirty */
        unsigned phone5      :1;    /* set if phone5 label is dirty */
        unsigned phone4      :1;    /* set if phone4 label is dirty */
        unsigned phone3      :1;    /* set if phone3 label is dirty */
        unsigned phone2      :1;    /* set if phone2 label is dirty */
        unsigned phone1      :1;    /* set if phone1 label is dirty */
        unsigned company     :1;    /* set if company label is dirty */
        unsigned firstName   :1;    /* set if firstName label is dirty */
        unsigned name        :1;    /* set if name label is dirty (bit 0) */
    } bits;
    UInt32 allBits;
} AddrDBFieldLabelsDirtyFlags;

/*
 * The labels for phone fields are stored specially.  Each phone field
 * can use one of eight labels.  Part of those eight labels are stored
 * where the phone field labels are.  The remainder (phoneLabelsStoredAtEnd)
 * are stored after the labels for all the fields.
 */

typedef char addressLabel[addrLabelLength];

typedef struct {
    UInt16 renamedCategories;    /* bitfield of categories with a different name */
    char categoryLabels[dmRecNumCategories][dmCategoryLength];
    UInt8 categoryUniqIDs[dmRecNumCategories];
    UInt8 lastUniqID; /* Uniq IDs generated by the device are between */
                     /* 0 - 127.  Those from the PC are 128 - 255. */
    UInt8 reserved1; /* from the compiler word aligning things */
    UInt16 reserved2;
    AddrDBFieldLabelsDirtyFlags dirtyFieldLabels;
    addressLabel fieldLabels[addrNumFields + numPhoneLabelsStoredSecond];
    CountryType country; /* Country the database (labels) is formatted for */
    AddrDBMisc misc;
} AddrAppInfoType;

typedef AddrAppInfoType * AddrAppInfoPtr;

#define GetPhoneLabel(r, p) (((r)->options.phoneBits >> (((p) - firstPhoneField) << 2)) & 0xF)

/*
 * Function prototypes
 */
extern void AddrUnpack(AddrPackedDBRecord *src, AddrDBRecordPtr dest);
extern Int16 GotoAddress(Int16 index);
extern Boolean DetermineRecordName(AddrDBRecordPtr recordP, 
                           Boolean sortByCompany,
                           Char **name1,
                           Char **name2);
extern void DrawRecordName(char* name1, char* name2,
                           UInt16 nameExtent, Int16 *x, Int16 y,
                           Boolean center, Boolean priorityIsName1);

extern char *UnnamedRecordStringPtr;

#endif __ADDRESS_H__
