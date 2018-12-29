// arduino includes
#include <Arduino.h>

// avr includes
#include <util/atomic.h>

#include "counters.h"
#include "config.h"
#include "interrupt.h"
#include "settings.h"

static SettingsCounters counters;

template <typename CountersIn, typename CountersOut>
static void copy_counters(CountersOut &out, const CountersIn &in)
{
        out.city = in.city;
        out.rain = in.rain;
}

static void counter_rain_dsr(uint8_t count)
{
    counters.rain += count;
}

static void counter_city_dsr(uint8_t count)
{
    counters.city += count;
}

void counters_log(const char *name, SettingsCounters &c)
{
    Serial.print("counters(");
    Serial.print(name);
    Serial.print("): rain = ");
    Serial.print(c.rain/COUNTER_RAIN_PULSE_PER_LITER);
    Serial.print("L; city = ");
    Serial.print(c.city/COUNTER_CITY_PULSE_PER_LITER);
    Serial.println("L");
}

void counters_setup()
{
    if (!settings_load(counters))
    {
        Serial.println("Failed to load counters");
        counters.city = 0;
        counters.rain = 0;
    }

    // DDRB input
    DDRB &= ~(
            GPIO_BIT(COUNTER_RAIN)
            | GPIO_BIT(COUNTER_CITY)
        );

#ifdef TANK_DEV
    // for debug enable pull-up
    PORTB |= (
            GPIO_BIT(COUNTER_RAIN)
            | GPIO_BIT(COUNTER_CITY)
        );
#endif

    // enable interrupts for input pins
    PCICR |= (1 << PCIE0);
    PCMSK0 |= GPIO_BIT(COUNTER_RAIN)
        | GPIO_BIT(COUNTER_CITY);

    INT_REGISTER_DSR(COUNTER_RAIN, INT_RISING, counter_rain_dsr);
    INT_REGISTER_DSR(COUNTER_CITY, INT_RISING, counter_city_dsr);
}

void counters_commit()
{
    if (!settings_save(counters))
        Serial.println("Failed to save counters");
}

void counters_get(SettingsCounters &c)
{
    c = counters;
}
