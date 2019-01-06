#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#ifndef TEST
#include "gpio.h"
#else
#include <stdint.h>
#endif

typedef void (*InterruptIsr)();
typedef void (*InterruptDsr)(uint8_t count);

enum InterruptType {
    INT_ISR = 0,
    INT_DSR = 1,
};

enum InterruptMode {
    INT_NONE        = 0,
    INT_RISING      = 1 << 0,
    INT_FALLING     = 1 << 1,
    INT_CHANGE      = INT_RISING | INT_FALLING,
};

void interrupt_setup();
void interrupt_loop();

void interrupt_register_dsr(const uint8_t arduinoPin, const InterruptMode mode, const InterruptDsr dsr);
void interrupt_register_isr(const uint8_t arduinoPin, const InterruptMode mode, const InterruptIsr isr);

#define _INT_ADAPT_MODE_INV_FALSE(MoDe) MoDe

// GPIO is inversed: RISING become FALLING and FALLING become RISING
#define _INT_ADAPT_MODE_INV_INT_NONE INT_NONE
#define _INT_ADAPT_MODE_INV_INT_RISING INT_FALLING
#define _INT_ADAPT_MODE_INV_INT_FALLING INT_RISING
#define _INT_ADAPT_MODE_INV_INT_CHANGE INT_CHANGE
#define _INT_ADAPT_MODE_INV_TRUE(MoDe) CAT(_INT_ADAPT_MODE_INV_, MoDe)

#define _INT_ADAPT_MODE(GPiO, MoDe)                                                 \
    CAT(_INT_ADAPT_MODE_INV_, GPIO_INV(GPiO))(MoDe)

#define INT_REGISTER_DSR(GPiO, MODE, DSR)                                           \
    interrupt_register_dsr(GPIO_ARDUINO_PIN(GPiO), _INT_ADAPT_MODE(GPiO, MODE), DSR)

#define INT_REGISTER_ISR(GPiO, MODE, ISR)                                           \
    interrupt_register_isr(GPIO_ARDUINO_PIN(GPiO), _INT_ADAPT_MODE(GPiO, MODE), ISR)

#endif /* _INTERRUPT_H */
