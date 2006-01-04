#include <PalmOS.h>
#include <SonyCLIE.h>
#include "Handera/Vga.h"
#include "Handera/Silk.h"

#include "address.h"
#include "memodb.h"
#include "lunar.h"
#include "birthdate.h"
#include "happydays.h"
#include "happydaysRsc.h"
#include "s2lconvert.h"
#include "util.h"
#include "notify.h"
#include "section.h"

Int16 OpenPIMDatabases(UInt32 dbid, UInt32 adid, UInt32 tdid, UInt32 mmid, UInt16 mode)
{
    UInt16 cardNo;
    LocalID dbID, appInfoID;
    UInt32 cdate, mdate;
    AddrAppInfoPtr appInfoPtr;
    
    DatebookDB = DmOpenDatabaseByTypeCreator('DATA', dbid, mode | dmModeReadWrite);
    if (!DatebookDB) {
        /*
         * BTW make sure there is " " (single space) in unused ^1 ^2 ^3
         * entries or PalmOS <= 2 will blow up.
         */

        SysCopyStringResource(gAppErrStr, DateBookFirstAlertString);
        FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " ");
        return -1;
    }

    ToDoDB = DmOpenDatabaseByTypeCreator('DATA', tdid, mode | dmModeReadWrite);
    if (!ToDoDB) {
        SysCopyStringResource(gAppErrStr, ToDoFirstAlertString);
        FrmCustomAlert(ErrorAlert, gAppErrStr, " ", " ");
        return -1;
    }

    AddressDB = DmOpenDatabaseByTypeCreator('DATA', adid, mode | dmModeReadOnly);
    ErrFatalDisplayIf(!AddressDB, "AddressDB not exist");

    DmOpenDatabaseInfo(AddressDB, &dbID, NULL,  NULL, &cardNo, NULL);
    if (!DmDatabaseInfo(cardNo, dbID, NULL , NULL, NULL, &cdate, &mdate,
                        NULL, NULL, &appInfoID, NULL, NULL, NULL)) {
        gAdcdate = cdate;
        gAdmdate = mdate;
        /*
         * We want to get the sort order for the address book.  This
         * controls how to display the birthday list.
         */
        if (appInfoID) {
            if ((appInfoPtr = (AddrAppInfoPtr)
                 MemLocalIDToLockedPtr(appInfoID, cardNo))) {
                gSortByCompany = appInfoPtr->misc.sortByCompany;
                MemPtrUnlock(appInfoPtr);
            }
        }

    }
    else return -1;
    
    MemoDB = DmOpenDatabaseByTypeCreator('DATA', mmid,
                                         mode | dmModeReadWrite);
    ErrFatalDisplayIf(!MemoDB, "MemoDB not exist");

    return 0;
}

