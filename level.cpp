#include <Arduino.h>

// local includes
#include "gpio.h"
#include "lpm.h"

// #define DEBUG_LEVELS

volatile static unsigned long echoStart;
volatile static unsigned long echoEnd;

volatile static bool pressureBooster = false;
#ifdef DEBUG_LEVELS
volatile static bool solenoidValve = false;
volatile static bool transfertPump = false;
#endif

static void level_refresh_pumps();

#ifdef DEBUG_LEVELS
static void log_levels()
{
    cli(); // disable interrupts

    const bool _pressureBooster = pressureBooster;
    const bool _solenoidValve = solenoidValve;
    const bool _transfertPump = transfertPump;
    const uint8_t _portC = PORTC;
    const uint8_t _pinC = PINC;
    const uint8_t _portD = PORTD;
    const uint8_t _pinD = PIND;
    const bool tankExtEmpty = GPIO_GET_IN(TANK_EXT_EMPTY);
    const bool tankIntEmpty = GPIO_GET_IN(TANK_INT_EMPTY);
    const bool cityLow = GPIO_GET_IN(TANK_INT_CITY_LOW);
    const bool rainLow = GPIO_GET_IN(TANK_INT_RAIN_LOW);
    const bool rainHigh = GPIO_GET_IN(TANK_INT_RAIN_HIGH);

    sei(); // enable interrupts

    Serial.print("pressureBooster: ");
    Serial.print(_pressureBooster);
    Serial.print("; solenoid valve: ");
    Serial.print(_solenoidValve);
    Serial.print("; transfert pump: ");
    Serial.println(_transfertPump);

    Serial.print("C: out = ");
    Serial.print(_portC, BIN);
    Serial.print("; in = ");
    Serial.print(_pinC, BIN);
    Serial.print("; D: out = ");
    Serial.print(_portD, BIN);
    Serial.print("; in = ");
    Serial.println(_pinD, BIN);

    Serial.print("\ttankExtEmpty: ");
    Serial.println(tankExtEmpty);
    Serial.print("\ttankIntEmpty: ");
    Serial.println(tankIntEmpty);
    Serial.print("\tcity low: ");
    Serial.println(cityLow);
    Serial.print("\train low: ");
    Serial.println(rainLow);
    Serial.print("\train high: ");
    Serial.println(rainHigh);
}
#endif

void level_setup()
{
    // DDRC output
    DDRC |= GPIO_BIT(DIST_SENSOR_TRIGGER)
            | GPIO_BIT(DIST_SENSOR_POWER)
            | GPIO_BIT(PRESSURE_BOOSTER)
            | GPIO_BIT(SOLENOID_VALVE)
            | GPIO_BIT(TRANSFERT_PUMP);

    // DDRC input + pull-up
    DDRC &= ~(GPIO_BIT(DIST_SENSOR_ECHO));
    PORTC |= GPIO_BIT(DIST_SENSOR_ECHO);

    // DDRD input + pull-up
    DDRD &= ~(
                GPIO_BIT(TANK_EXT_EMPTY)
                | GPIO_BIT(TANK_INT_EMPTY)
                | GPIO_BIT(TANK_INT_CITY_LOW)
                | GPIO_BIT(TANK_INT_RAIN_LOW)
                | GPIO_BIT(TANK_INT_RAIN_HIGH)
            );
    PORTD |=
            GPIO_BIT(TANK_EXT_EMPTY)
            | GPIO_BIT(TANK_INT_EMPTY)
            | GPIO_BIT(TANK_INT_CITY_LOW)
            | GPIO_BIT(TANK_INT_RAIN_LOW)
            | GPIO_BIT(TANK_INT_RAIN_HIGH);

    // Set distance sensor as off
    GPIO_SET_OFF(DIST_SENSOR_POWER);
    GPIO_SET_OFF(DIST_SENSOR_TRIGGER);

    // enable interrupts for input pins
    PCICR |= (1 << PCIE1) | (1 << PCIE2);
    PCMSK1 |= GPIO_BIT(DIST_SENSOR_ECHO);
    PCMSK2 |= GPIO_BIT(TANK_EXT_EMPTY)
            | GPIO_BIT(TANK_INT_EMPTY)
            | GPIO_BIT(TANK_INT_CITY_LOW)
            | GPIO_BIT(TANK_INT_RAIN_LOW)
            | GPIO_BIT(TANK_INT_RAIN_HIGH);

    level_refresh_pumps();
}

void level_PCINT1()
{
    static uint8_t previous = 0;

    if ((previous & GPIO_BIT(DIST_SENSOR_ECHO)) != (PINC & GPIO_BIT(DIST_SENSOR_ECHO)))
        (PINC & GPIO_BIT(DIST_SENSOR_ECHO) ? echoStart : echoEnd) = micros();

    previous = PINC;
}

void level_PCINT2()
{
    level_refresh_pumps();
}

static void level_refresh_pumps()
{
    // pressureBooster is ON if internal tank is not empty
    const bool _pressureBooster = !GPIO_GET_IN(TANK_INT_EMPTY);

    // electrovanne is switch to ON if CITY_LOW is OFF while RAIN_LOW is OFF
    const bool _solenoidValve
        = (!GPIO_GET_OUT(SOLENOID_VALVE) && !GPIO_GET_IN(TANK_INT_CITY_LOW)) // switch ON when !CITY_LOW && !SOLENOID_VALVE
        || (GPIO_GET_OUT(SOLENOID_VALVE) && !GPIO_GET_IN(TANK_INT_RAIN_LOW)); // until RAIN_LOW && SOLENOID_VALVE

    // transfertPump pump is ON if RAIN_LOW if OFF while RAIN_HIGH is OFF and there is watter in external tank
    const bool _transfertPump = !GPIO_GET_IN(TANK_EXT_EMPTY) && (
            (!GPIO_GET_OUT(TRANSFERT_PUMP) && !GPIO_GET_IN(TANK_INT_RAIN_LOW))
            || (GPIO_GET_OUT(TRANSFERT_PUMP) && !GPIO_GET_IN(TANK_INT_RAIN_HIGH))
        );
    GPIO_SET(PRESSURE_BOOSTER, _pressureBooster);
    GPIO_SET(SOLENOID_VALVE, _solenoidValve);
    GPIO_SET(TRANSFERT_PUMP, _transfertPump);

    pressureBooster = _pressureBooster;
#ifdef DEBUG_LEVELS
    solenoidValve = _solenoidValve;
    transfertPump = _transfertPump;
#endif
}

static int level_get_distance_()
{
    unsigned long duration = 0;
    int distance = -1;
    long _echoEnd;
    long _echoStart;
    // reset timings
    cli(); // disable interrupts
    echoStart = 0;
    echoEnd = 0;
    sei();

    GPIO_SET_ON(DIST_SENSOR_TRIGGER);
    delayMicroseconds(13);
    GPIO_SET_OFF(DIST_SENSOR_TRIGGER);

    // get echo, 50ms max
    for (unsigned char wdt = 0; wdt < 250 ; ++wdt)
    {
        delayMicroseconds(200);
        cli(); // disable interrupts
        if (echoEnd)
        {
            _echoEnd = echoEnd;
            _echoStart = echoStart;
            sei();
            duration = _echoEnd - _echoStart;
            distance = (duration <= 25000 ? duration / 58 : -1);
            break;
        }
        sei();
    }

    // Serial.print("duration: ");
    // Serial.print(duration);
    // Serial.print("; distance: ");
    // Serial.print(distance);
    // Serial.print("; start: ");
    // Serial.print(_echoStart);
    // Serial.print("; end: ");
    // Serial.print(_echoEnd);
    // Serial.println(";");

    return distance;
}

#undef max

template <typename T>
T max(const T t1, const T t2)
{
    return t1 > t2 ? t1 : t2;
}

// return distance from sensor or -1 in case of error
int level_get_distance()
{
    int distance = -1;
    GPIO_SET_ON(DIST_SENSOR_POWER);

    timer0_acquire();

    // empiric delay to allow distance sensor to start
    delay(30);

    // sometimes, mesure seems to be shorter as expecter, so do 3 measurements
    // and takes the max
    for (int i = 0; i < 3; ++i)
        distance = max(distance, level_get_distance_());

    GPIO_SET_OFF(DIST_SENSOR_POWER);
    Serial.print("final distance: ");
    Serial.println(distance);

    timer0_release();

    return distance;
}

void level_loop()
{
#ifdef DEBUG_LEVELS
    log_levels();
#endif
}
