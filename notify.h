#ifndef _NOTIFY_H__
#define _NOTIFY_H__

extern Boolean DBNotifyFormHandleEvent(EventPtr e);
extern Boolean NotifyStringFormHandleEvent(EventPtr e);
extern Boolean ToDoFormHandleEvent(EventPtr e);
extern int CleanupFromDB(DmOpenRef db);
extern int CleanupFromTD(DmOpenRef db);
extern void PerformExport(Char * memo, int mainDBIndex, DateType when);

// must resident in main section(use temporary variable)
extern Char* EventTypeString(HappyDays r);

#endif
