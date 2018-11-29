#include <Arduino.h>
#include <EtherCard.h>
#include <util/delay.h>

// local includes
#include "gpio.h"
#include "level.h"
#include "lpm.h"

ISR(PCINT1_vect)
{
    level_PCINT1();
}

ISR(PCINT2_vect)
{
    level_PCINT2();
}


uint8_t Ethernet::buffer[700]; // configure buffer size to 700 octets
static uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 }; // define (unique on LAN) hardware (MAC) address
const static uint8_t ip[] = {192,168,1,7};
const static uint8_t gw[] = {192,168,1,2};
const static uint8_t dns[] = {192,168,1,4};

void setup()
{
    lpm_setup();
    level_setup();

    Serial.begin(57600);
    Serial.println("Enjoy distance sensor: ");
}

void loop()
{
}

int main(void)
{
    init();

    setup();

    for (;;) {
        loop();
        if (serialEventRun) serialEventRun();
        lpm_sleep();
    }

    return 0;
}

