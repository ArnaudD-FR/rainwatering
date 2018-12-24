#include <Arduino.h>
#include <util/delay.h>

// local includes
#include "counters.h"
#include "config.h"
#include "interrupt.h"
#include "level.h"
#include "lpm.h"
#include "network.h"

void setup()
{
    Serial.begin(57600);

    // timer0_acquire();
    lpm_setup();
    counters_setup();
    level_setup();
    // network_setup();

    Serial.println("Enjoy distance sensor: ");
}

void loop()
{
    Serial.println("loop");
    if (serialEventRun) serialEventRun();

    interrupt_loop();

#if 0
    static uint8_t count = 0;
    if (count++ == 0)
    {
        Serial.print("Distance: ");
        Serial.println(level_get_distance());
    }
#endif

#if 1
    SettingsCounters counters;
    counters_get(counters);
    counters_log("global", counters);
    // counters_commit();
#endif

    // network_loop();

    // Serial.println(__FUNCTION__);
    // Serial.print("\train high: ");
    // Serial.println(GPIO_GET_IN(TANK_INT_RAIN_HIGH));

    Serial.flush();
    lpm_sleep();
}

int main(void)
{
    init();

    setup();

    for (;;) {
        loop();
    }

    return 0;
}

