#ifndef _NOTIFY_H__
#define _NOTIFY_H__

extern Boolean DBNotifyFormHandleEvent(EventPtr e) SECT1;
extern Boolean DBNotifyFormMoreHandleEvent(EventPtr e) SECT1;
extern Boolean ToDoFormHandleEvent(EventPtr e) SECT1;
extern int CleanupFromDB(DmOpenRef db) SECT1;
extern int CleanupFromTD(DmOpenRef db) SECT1;
extern void PerformExport(Char * memo, int mainDBIndex, DateType when) SECT1;
extern Char* EventTypeString(HappyDays r) SECT1;

#endif
