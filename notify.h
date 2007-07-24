#ifndef _NOTIFY_H__
#define _NOTIFY_H__

#include "section.h"

Boolean DBNotifyFormHandleEvent(EventPtr e) SECT2;
Boolean NotifyStringFormHandleEvent(EventPtr e) SECT2;
Boolean ToDoFormHandleEvent(EventPtr e) SECT2;
int CleanupFromDB(DmOpenRef db) SECT2;
void PerformExport(Char * memo, int mainDBIndex, DateType when) SECT2;
Char* EventTypeString(HappyDays r) SECT2;
Int16 FindEventNoteInfo(HappyDays birth);
void LoadEventNoteString(char *p, Int16 idx, Int16 maxString) SECT2;
Char* NotifyDescString(DateType when, HappyDays birth, 
                       Int8 age, Boolean todo) SECT2;
void ChkNMakePrivateRecord(DmOpenRef db, Int16 index) SECT2;
Boolean IsSameRecord(Char* notefield, HappyDays birth) SECT2;
Boolean IsHappyDaysRecord(Char* notefield) SECT2;

// defined in newtodo.c
Int16 NewActualPerformNotifyTD(HappyDays birth, DateType when, Int8 age,
                               Int16 *created, Int16 *touched, Int16 existIndex) SECT2;
Int16 NewCheckToDoRecord(DateType when, HappyDays birth) SECT2;

#endif
