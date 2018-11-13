#include <Arduino.h>
#include <avr/sleep.h>
#include <EtherCard.h>

// local includes
#include "gpio.h"
#include "level.h"

ISR(PCINT1_vect)
{
    level_PCINT1();
}

ISR(PCINT2_vect)
{
    level_PCINT2();
}



void setup()
{
    //
    // intialize I/O
    //
    // set full PORT B as input. EtherCard will configure SPI pins
    DDRB = 0;


    // DDRD input
    DDRD &= ~(
                GPIO_BIT(ENC28J60_INT)
            );

    level_setup();

    Serial.begin(57600);
    Serial.println("Enjoy distance sensor: ");
}

void loop()
{

    {
    }




#if 0
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sei(); // enable interrupts

    sleep_cpu();
    sleep_disable();

    cli(); // disable interrupts
#endif
    level_loop();
}

int main(void)
{
    init();

    setup();

    for (;;) {
        loop();
        if (serialEventRun) serialEventRun();
    }

    return 0;
}

