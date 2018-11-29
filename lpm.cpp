#include <Arduino.h>
#include <wiring_private.h>

// avr includes
#include <avr/sleep.h>

#include "lpm.h"

static uint8_t _timer0Counter = 0;

void timer0_acquire()
{
    if (!_timer0Counter)
        cbi(PRR, PRTIM0);
    ++_timer0Counter;
}

void timer0_release()
{
    if (_timer0Counter == 0)
        return;

    --_timer0Counter;
    if (!_timer0Counter)
        sbi(PRR, PRTIM0);
}

void lpm_setup()
{
    // by default disable timer0
    sbi(PRR, PRTIM0);
}

void lpm_sleep()
{
    set_sleep_mode(SLEEP_MODE_STANDBY);
    sleep_enable();
    sei(); // enable interrupts

    sleep_cpu();
    sleep_disable();
}
