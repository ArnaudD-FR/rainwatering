#include "settings.h"

#include <Arduino.h>
#include <avr/eeprom.h>
#include <stddef.h>

static uint32_t MAGIC = 0x53455454;
#define EEPROM_MIN ((char *)0)
#define EEPROM_MAX (EEPROM_MIN + 1024)

// EEPROM page size: 4B
struct SettingsHeader
{
    uint32_t magic;

    uint16_t settingsId;
    uint16_t blockSize;

    uint16_t blockCurrent;
    uint16_t blockCurrentInv;

    uint16_t blockPrevious;
    uint16_t blockPreviousInv;
};

static char *_settings_find_header(const Settings s, SettingsHeader &h)
{
    char *ePos = EEPROM_MIN;
    for (; ePos + sizeof(h) < (const char *)EEPROM_MAX; ePos += sizeof(h) + h.blockSize)
    {
        eeprom_read_block(&h, ePos, sizeof(h));
        if (h.magic != MAGIC)
            break;

        if (h.settingsId == s)
            break;
    }
    return ePos;
}

static inline uint16_t _settings_block_id(const SettingsHeader h)
{
    return (h.blockCurrent == ~h.blockCurrentInv) ? h.blockCurrent : h.blockPrevious;
}

void settings_dump()
{
    uint32_t v;
    for (uint32_t *ePos = (uint32_t *)EEPROM_MIN; ePos < (uint32_t *)EEPROM_MAX; ++ePos)
    {
        v = eeprom_read_dword(ePos);
        Serial.print("eeprom@");
        Serial.print((uint32_t)ePos, HEX);
        Serial.print(": ");
        Serial.println(v, HEX);
    }
}

void settings_clear()
{
    for (uint32_t *ePos = (uint32_t *)EEPROM_MIN; ePos < (uint32_t *)EEPROM_MAX; ++ePos)
        eeprom_update_dword(ePos, 0x44454144);
}

bool settings_load(const Settings s, void *data, const uint8_t size)
{
    SettingsHeader h;
    char *ePos = _settings_find_header(s, h);
    if (h.magic != MAGIC || h.settingsId != s || h.blockSize < size)
        return false;

    const uint16_t blockId = _settings_block_id(h);

    ePos += sizeof(h) + (blockId%2)*h.blockSize;

    eeprom_read_block(data, ePos, size);
    return true;
}

bool settings_save(const Settings s, const void *data, const uint8_t size)
{
    SettingsHeader h;
    char *hPos = _settings_find_header(s, h);

    const bool newHeader = h.magic != MAGIC;
    uint16_t blockId;
    if (newHeader)
    {
        // align size to word size (4 bytes)
        h.settingsId = s;
        h.blockSize = ((size + 3)/4)*4;
        blockId = 0;
        if (hPos + sizeof(h) + h.blockSize * 2 >= EEPROM_MAX)
        {
            Serial.println("settings: EEPROM full!");
            return false;
        }
    }
    else if (h.settingsId != s || h.blockSize < size)
    {
        Serial.println("settings: invalid ID/size");
        return false;
    }
    else
        blockId = _settings_block_id(h) + 1;

    h.blockPrevious = h.blockCurrent;
    h.blockPreviousInv = h.blockCurrentInv;
    h.blockCurrent = blockId;
    h.blockCurrentInv = ~blockId;
    char *bPos = hPos + sizeof(h) + (blockId%2)*h.blockSize;

    eeprom_update_block(data, bPos, size);
    // write block previous + inv
    eeprom_write_block(&h.blockPrevious, hPos + offsetof(SettingsHeader, blockPrevious), sizeof(uint32_t));
    // write block current + inv
    eeprom_write_block(&h.blockCurrent, hPos + offsetof(SettingsHeader, blockCurrent), sizeof(uint32_t));

    if (newHeader)
    {
        // write settings ID + block size
        eeprom_write_block(&h.settingsId, hPos + offsetof(SettingsHeader, settingsId), sizeof(uint32_t));
        // end with write of magic
        eeprom_write_dword((uint32_t *)hPos + offsetof(SettingsHeader, magic), MAGIC);
    }

    eeprom_busy_wait();
    return true;
}

