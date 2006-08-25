#ifndef _ADDRESSCOMMON_H_
#define _ADDRESSCOMMON_H_

#include "section.h"

#define shortenedFieldString              "\205"
#define shortenedFieldLength              1
#define fieldSeparatorString              ", "
#define fieldSeparatorLength              2

extern Int16 GotoAddress(Int16 index);
// called from Find function(must be in main section)
extern void DrawRecordName(char* name1, char* name2,
                           UInt16 nameExtent, Int16 *x, Int16 y,
                           Boolean center, Boolean priorityIsName1);

#endif
