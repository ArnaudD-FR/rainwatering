#ifndef _COUNTERS_H
#define _COUNTERS_H

#include "settings.h"

void counters_setup();
void counters_commit();

void counters_get(SettingsCounters &c);
void counters_log(const char *name, SettingsCounters &c);

#endif /* _COUNTERS_H */
