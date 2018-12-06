#include <Arduino.h>
#include <util/delay.h>

// local includes
#include "gpio.h"
#include "level.h"
#include "lpm.h"
#include "network.h"

ISR(PCINT1_vect)
{
    level_PCINT1();
}

ISR(PCINT2_vect)
{
    level_PCINT2();
    network_PCINT2();
}

void setup()
{
    Serial.begin(57600);

    lpm_setup();
    level_setup();
    network_setup();

    Serial.println("Enjoy distance sensor: ");
}

void loop()
{
    if (serialEventRun) serialEventRun();
    network_loop();
}

int main(void)
{
    init();

    setup();

    for (;;) {
        loop();
        Serial.flush();
        lpm_sleep();
    }

    return 0;
}

