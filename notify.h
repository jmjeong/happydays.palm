#ifndef _NOTIFY_H__
#define _NOTIFY_H__

extern Boolean DBNotifyFormHandleEvent(EventPtr e);
extern Boolean DBNotifyFormMoreHandleEvent(EventPtr e);
extern Boolean ToDoFormHandleEvent(EventPtr e);
extern int CleanupFromDB(DmOpenRef db);
extern int CleanupFromTD(DmOpenRef db);
extern void PerformExport(Char * memo, int mainDBIndex, DateType when);
extern Char* EventTypeString(BirthDate r);

#endif
