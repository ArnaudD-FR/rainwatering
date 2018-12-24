#include <Arduino.h>
#include "interrupt.h"
#include "lpm.h"
#ifndef TEST

#include <util/atomic.h>
#define TEST_LOG(...)

#else

#include <stdio.h>

#define ATOMIC_BLOCK(T)
#define TEST_LOG printf
#endif

// FLAGS, 4bits: txmm with:
//  - t as type 1bit: InterruptType
//  - x (1bit): reserved
//  - mm (2 bits): InterruptMode

#define INT_FLAGS_SIZE 4
#define INT_FLAGS_MASK (0xFF >> (8 - INT_FLAGS_SIZE))
#define INT_FLAGS_PIN_ARR_IDX(PIN) ( (PIN)/(8/INT_FLAGS_SIZE) )
#define INT_FLAGS_PIN_BYTE_OFFSET(PIN) ( ((PIN)%(8/INT_FLAGS_SIZE)) * INT_FLAGS_SIZE )

#define INT_MODE_SIZE 2
#define INT_MODE_MASK (0xFF >> (8 - INT_MODE_SIZE))
#define INT_MODE_OFFSET 0
#define INT_MODE_GET(v) ((InterruptMode)((v >> INT_MODE_OFFSET) & INT_MODE_MASK))

#define INT_TYPE_SIZE 1
#define INT_TYPE_MASK (0xFF >> (8 - INT_TYPE_SIZE))
#define INT_TYPE_OFFSET 3
#define INT_TYPE_GET(v) ((InterruptType)((v >> INT_TYPE_OFFSET) & INT_TYPE_MASK))

typedef void (*InterruptCbk)();

static InterruptCbk interrupt_cbk_arr[3 * 8] = {};
static uint8_t interrupt_flags[3 * INT_FLAGS_SIZE];
static volatile uint8_t interrupt_dsr_counter[3 * 8];

static void interrupt_register(const uint8_t arduinoPin, const uint8_t flags, const InterruptCbk cbk)
{
    interrupt_cbk_arr[arduinoPin] = cbk;
    uint8_t &byte = interrupt_flags[INT_FLAGS_PIN_ARR_IDX(arduinoPin)];
    const uint8_t byteOffset = INT_FLAGS_PIN_BYTE_OFFSET(arduinoPin);
    byte = (byte & ~(INT_FLAGS_MASK << byteOffset)) // clear bits
        | (flags << byteOffset); // set bits
}

void interrupt_register_dsr(const uint8_t arduinoPin, const InterruptMode mode, const InterruptDsr dsr)
{
    interrupt_register(
            arduinoPin,
            (INT_DSR << INT_TYPE_OFFSET) | (mode << INT_MODE_OFFSET),
            (InterruptCbk)dsr
        );
}

void interrupt_register_isr(const uint8_t arduinoPin, const InterruptMode mode, const InterruptIsr isr)
{
    interrupt_register(
            arduinoPin,
            (INT_ISR << INT_TYPE_OFFSET) | (mode << INT_MODE_OFFSET),
            (InterruptCbk)isr
        );
}

void interrupt_loop()
{
    for (uint8_t idx = 0; idx < sizeof(interrupt_dsr_counter)/sizeof(interrupt_dsr_counter[0]); ++idx)
    {
        uint8_t counter;
        ATOMIC_BLOCK(ATOMIC_FORCEON)
        {
            counter = interrupt_dsr_counter[idx];
            interrupt_dsr_counter[idx] = 0;
        }

        InterruptDsr dsr = (InterruptDsr)interrupt_cbk_arr[idx];
        if (counter)
            lpm_lock_release();
        if (!dsr || !counter)
            continue;

        dsr(counter);
    }
}

static void interrupt_isr(const uint8_t offset, uint8_t &previous, const uint8_t current)
{
    int idx = 0;
    for (InterruptCbk *iter = interrupt_cbk_arr + offset,
            *last = iter + 8;
            iter < last; ++iter, ++idx)
    {
        const uint8_t arduinoPin = offset + idx;
        InterruptCbk cbk = *iter;

        if (!cbk)
            continue;

        const uint8_t flags = interrupt_flags[INT_FLAGS_PIN_ARR_IDX(arduinoPin)] >> INT_FLAGS_PIN_BYTE_OFFSET(arduinoPin);
        const InterruptMode m = INT_MODE_GET(flags);
        uint8_t match = false;

        switch (m)
        {
            case INT_NONE:
                break;

            case INT_CHANGE:
                match = ((previous & (1 << idx)) != (current & (1 << idx)));
                break;

            case INT_RISING:
                match = ((0x1 & (previous >> idx)) == 0) && ((0x1 & (current >> idx)) == 1);
                break;

            case INT_FALLING:
                match = ((0x1 & (previous >> idx)) == 1) && ((0x1 & (current >> idx)) == 0);
                break;
        }

        TEST_LOG(
                "\t%d: m = %-8s; o = %d; n = %d; match = %d\n",
                arduinoPin,
                m == INT_NONE ? "NONE" : m == INT_CHANGE ? "CHANGE": m == INT_RISING ? "RISING" : "FALLING",
                0x1 & (previous >> idx),
                0x1 & (current >> idx),
                match
            );

        if (match)
        {
            const InterruptType t = INT_TYPE_GET(flags);
            switch (t)
            {
                case INT_ISR:
                    ((InterruptIsr)cbk)();
                    break;

                case INT_DSR:
                    if (interrupt_dsr_counter[arduinoPin] == 0)
                        lpm_lock_acquire();
                    if (interrupt_dsr_counter[arduinoPin] < 0xFF)
                        ++interrupt_dsr_counter[arduinoPin];
                    break;
            }
        }
    }
    previous = current;
}

#ifndef TEST
ISR(PCINT0_vect)
{
    static uint8_t previous = 0;
    interrupt_isr(_GPIO_ARDUINO_PORT_OFFSET_B, previous, PINB);
}

ISR(PCINT1_vect)
{
    static uint8_t previous = 0;
    interrupt_isr(_GPIO_ARDUINO_PORT_OFFSET_C, previous, PINC);
}

ISR(PCINT2_vect)
{
    static uint8_t previous = 0;
    interrupt_isr(_GPIO_ARDUINO_PORT_OFFSET_D, previous, PIND);
}
#else

#define DSR_RISING_PIN 8
#define DSR_FALLING_PIN 2
#define DSR_CHANGE_PIN 16

#define ISR_RISING_PIN 9
#define ISR_FALLING_PIN 3
#define ISR_CHANGE_PIN 17

struct Count
{
    uint8_t rising;
    uint8_t falling;
    uint8_t change;
};

static Count dsr;
static Count isr;

static uint8_t pins[3];

void dsr_rising(uint8_t count)
{
    dsr.rising += count;
    printf("rising count: %d\n", count);
}

void dsr_falling(uint8_t count)
{
    dsr.falling += count;
    printf("falling count: %d\n", count);
}

void dsr_change(uint8_t count)
{
    dsr.change += count;
    printf("change count: %d\n", count);
}

void isr_rising()
{
    ++isr.rising;
}

void isr_falling()
{
    ++isr.falling;
}

void isr_change()
{
    ++isr.change;
}

void toggle_pin(uint8_t pin)
{
    const uint8_t port = pin/8;
    const uint8_t gpio = pin%8;

    uint8_t old = pins[port];
    if (old & (1 << gpio))
        pins[port] &= ~(1 << gpio);
    else
        pins[port] |= 1 << gpio;
    interrupt_isr(port * 8, old, pins[port]);
}

void check_counts(Count &c, uint8_t rising, uint8_t falling, uint8_t change)
{
    if (rising != c.rising)
        printf("error: rising: %d != %d\n", rising, c.rising);
    if (falling != c.falling)
        printf("error: falling: %d != %d\n", falling, c.falling);
    if (change != c.change)
        printf("error: change: %d != %d\n", change, c.change);
}

int main()
{
    interrupt_register_dsr(DSR_RISING_PIN, INT_RISING, dsr_rising);
    interrupt_register_dsr(DSR_FALLING_PIN, INT_FALLING, dsr_falling);
    interrupt_register_dsr(DSR_CHANGE_PIN, INT_CHANGE, dsr_change);

    interrupt_register_isr(ISR_RISING_PIN, INT_RISING, isr_rising);
    interrupt_register_isr(ISR_FALLING_PIN, INT_FALLING, isr_falling);
    interrupt_register_isr(ISR_CHANGE_PIN, INT_CHANGE, isr_change);

    toggle_pin(DSR_RISING_PIN);
    interrupt_loop();
    check_counts(dsr, 1, 0, 0);
    toggle_pin(DSR_RISING_PIN);
    interrupt_loop();
    check_counts(dsr, 1, 0, 0);

    toggle_pin(DSR_FALLING_PIN);
    interrupt_loop();
    check_counts(dsr, 1, 0, 0);
    toggle_pin(DSR_FALLING_PIN);
    interrupt_loop();
    check_counts(dsr, 1, 1, 0);
    toggle_pin(DSR_FALLING_PIN);
    toggle_pin(DSR_FALLING_PIN);
    interrupt_loop();
    check_counts(dsr, 1, 2, 0);

    toggle_pin(DSR_CHANGE_PIN);
    toggle_pin(DSR_CHANGE_PIN);
    toggle_pin(DSR_CHANGE_PIN);
    toggle_pin(DSR_CHANGE_PIN);
    interrupt_loop();
    check_counts(dsr, 1, 2, 4);

    /**************************/

    toggle_pin(ISR_RISING_PIN);
    check_counts(isr, 1, 0, 0);
    toggle_pin(ISR_RISING_PIN);
    check_counts(isr, 1, 0, 0);

    toggle_pin(ISR_FALLING_PIN);
    check_counts(isr, 1, 0, 0);
    toggle_pin(ISR_FALLING_PIN);
    check_counts(isr, 1, 1, 0);
    toggle_pin(ISR_FALLING_PIN);
    toggle_pin(ISR_FALLING_PIN);
    check_counts(isr, 1, 2, 0);

    toggle_pin(ISR_CHANGE_PIN);
    toggle_pin(ISR_CHANGE_PIN);
    toggle_pin(ISR_CHANGE_PIN);
    toggle_pin(ISR_CHANGE_PIN);
    check_counts(isr, 1, 2, 4);

    /**************************/

    interrupt_loop();
    check_counts(dsr, 1, 2, 4);
    check_counts(isr, 1, 2, 4);

    return 0;
}

#endif
