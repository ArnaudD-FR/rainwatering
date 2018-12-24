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

void interrupt_loop();

void interrupt_register_dsr(const uint8_t arduinoPin, const InterruptMode mode, const InterruptDsr dsr);
void interrupt_register_isr(const uint8_t arduinoPin, const InterruptMode mode, const InterruptIsr isr);

#define INT_REGISTER_DSR(GPiO, MODE, DSR)                                           \
    interrupt_register_dsr(GPIO_ARDUINO_PIN(GPiO), MODE, DSR)

#define INT_REGISTER_ISR(GPiO, MODE, ISR)                                           \
    interrupt_register_isr(GPIO_ARDUINO_PIN(GPiO), MODE, ISR)

#endif /* _INTERRUPT_H */
