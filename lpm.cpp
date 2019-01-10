#include <Arduino.h>
#include <wiring_private.h>

// avr includes
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#include "lpm.h"

static uint8_t _timer0Counter = 0;
static volatile uint8_t _lpmLockCount = 0;

void timer0_acquire()
{
    if (!_timer0Counter)
    {
        power_timer0_enable();
        sbi(TIMSK0, TOIE0);
    }
    ++_timer0Counter;
}

void timer0_release()
{
    if (_timer0Counter == 0)
        return;

    --_timer0Counter;
    if (!_timer0Counter)
    {
        cbi(TIMSK0, TOIE0);
        power_timer0_disable();
    }
}

void lpm_lock_acquire()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ++_lpmLockCount;
    }
}

void lpm_lock_release()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (_lpmLockCount > 0)
            --_lpmLockCount;
    }
}

void lpm_setup()
{
    // by default disable timer0
    sbi(PRR, PRTIM0);
}

void lpm_sleep()
{
    set_sleep_mode(SLEEP_MODE_STANDBY);
    // cli();
    // uint8_t count = _lpmLockCount;
    // sei();
    // Serial.print("lpm locks: ");
    // Serial.println(count);
    // Serial.flush();
    cli();
    if (!_lpmLockCount)
    {
        sleep_enable();
        // enable interrupts
        sei();
        // operation right after sei is guaranteed to be
        // executed be an interrupt could trigger (see avr/sleep.h)
        sleep_cpu();
        sleep_disable();
    }
    sei();
}
