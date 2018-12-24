#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <inttypes.h>

// keep enum order
enum Settings
{
    SETTINGS_COUNTERS,
    SETTINGS_LEVELS,
};

template <typename S>
struct SettingsMap;

struct SettingsCounters
{
    static Settings kind() { return SETTINGS_COUNTERS; }

    uint32_t city;
    uint32_t rain;
};

struct SettingsLevels
{
    static Settings kind() { return SETTINGS_LEVELS; }

    char min;
    char max;
};

void settings_dump();
void settings_clear(); // erase eeprom
bool settings_load(const Settings s, void *data, const uint8_t size);
bool settings_save(const Settings s, const void *data, const uint8_t size);

template <typename S>
bool settings_load(S &s)
{
    return settings_load(S::kind(), &s, sizeof(S));
}

template <typename S>
bool settings_save(const S &s)
{
    return settings_save(S::kind(), &s, sizeof(S));
}

#endif // _SETTINGS_H
