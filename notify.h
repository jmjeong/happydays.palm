#ifndef _NOTIFY_H__
#define _NOTIFY_H__

extern Boolean DBNotifyFormHandleEvent(EventPtr e) SECT2;
extern Boolean NotifyStringFormHandleEvent(EventPtr e) SECT2;
extern Boolean ToDoFormHandleEvent(EventPtr e) SECT2;
extern int CleanupFromDB(DmOpenRef db) SECT2;
extern int CleanupFromTD(DmOpenRef db) SECT2;
extern void PerformExport(Char * memo, int mainDBIndex, DateType when) SECT2;

// must resident in main section(use temporary variable)
extern Char* EventTypeString(HappyDays r);

#endif
