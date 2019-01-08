#include <Arduino.h>
#include <math.h>

// local includes
#include "counters.h"
#include "config.h"
#include "interrupt.h"
#include "lpm.h"
#include "level.h"

volatile static unsigned long echoStart;
volatile static unsigned long echoEnd;

static void level_refresh_pumps();

void level_sensor_echo_isr()
{
    (!echoStart ? echoStart : echoEnd) = micros();
}

void level_switch_dsr(uint8_t)
{
    level_refresh_pumps();
}

void level_setup()
{
    GPIO_SET_OFF(PRESSURE_BOOSTER);
    GPIO_SET_OFF(SOLENOID_VALVE);
    GPIO_SET_OFF(TRANSFERT_PUMP);

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
    DDRD &= ~(0
                | GPIO_BIT(TANK_EXT_EMPTY)
                | GPIO_BIT(TANK_INT_EMPTY)
                | GPIO_BIT(TANK_INT_CITY_LOW)
                | GPIO_BIT(TANK_INT_RAIN_LOW)
                | GPIO_BIT(TANK_INT_RAIN_HIGH)
            );
    PORTD |= 0
            | GPIO_BIT(TANK_EXT_EMPTY)
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
    PCMSK2 = PCMSK2
            | GPIO_BIT(TANK_EXT_EMPTY)
            | GPIO_BIT(TANK_INT_EMPTY)
            | GPIO_BIT(TANK_INT_CITY_LOW)
            | GPIO_BIT(TANK_INT_RAIN_LOW)
            | GPIO_BIT(TANK_INT_RAIN_HIGH);

    INT_REGISTER_ISR(DIST_SENSOR_ECHO, INT_CHANGE, level_sensor_echo_isr);
    INT_REGISTER_DSR(TANK_EXT_EMPTY, INT_CHANGE, level_switch_dsr);
    INT_REGISTER_DSR(TANK_INT_EMPTY, INT_CHANGE, level_switch_dsr);
    INT_REGISTER_DSR(TANK_INT_CITY_LOW, INT_CHANGE, level_switch_dsr);
    INT_REGISTER_DSR(TANK_INT_RAIN_LOW, INT_CHANGE, level_switch_dsr);
    INT_REGISTER_DSR(TANK_INT_RAIN_HIGH, INT_CHANGE, level_switch_dsr);

    level_refresh_pumps();
}

// #define PURGE
// #define CALIBRATE_CITY
// #define CALIBRATE_RAIN

#ifdef PURGE
static void level_refresh_pumps()
{
    // pressureBooster is switch OFF if RAIN_LOW is OFF while RAIN_HIGH
    const bool ps = GPIO_GET_OUT(PRESSURE_BOOSTER) && !GPIO_GET_IN(TANK_INT_RAIN_LOW) || !GPIO_GET_OUT(PRESSURE_BOOSTER) && !GPIO_GET_IN(TANK_INT_RAIN_HIGH);
            // (!GPIO_GET_OUT(PRESSURE_BOOSTER) && !GPIO_GET_IN(TANK_INT_RAIN_LOW))
            // || (GPIO_GET_OUT(PRESSURE_BOOSTER) && !GPIO_GET_IN(TANK_INT_RAIN_HIGH));
            //
    GPIO_SET(PRESSURE_BOOSTER, !ps);
}
#elif defined CALIBRATE_CITY
static void level_refresh_pumps()
{
    const bool before = GPIO_GET_OUT(SOLENOID_VALVE);
    const bool transfertPump = (!before && !GPIO_GET_IN(TANK_INT_RAIN_LOW))
            || (before && !GPIO_GET_IN(TANK_INT_RAIN_HIGH));

    GPIO_SET(SOLENOID_VALVE, transfertPump);
}
#elif defined CALIBRATE_RAIN
static void level_refresh_pumps()
{
    const bool before = GPIO_GET_OUT(TRANSFERT_PUMP);
    const bool transfertPump = (!before && !GPIO_GET_IN(TANK_INT_RAIN_LOW))
            || (before && !GPIO_GET_IN(TANK_INT_RAIN_HIGH));

    GPIO_SET(TRANSFERT_PUMP, transfertPump);
}
#else
static void level_refresh_pumps()
{
    // pressureBooster is switch ON while internal tank is NOT empty or CITY_LOW is ON
    const bool pressureBoosterOff =
        (GPIO_GET_OUT(PRESSURE_BOOSTER) && GPIO_GET_IN(TANK_INT_EMPTY)) // switch ON while !TANK_INT_EMPTY
        || (!GPIO_GET_OUT(PRESSURE_BOOSTER) && !GPIO_GET_IN(TANK_INT_CITY_LOW)); // or CITY_LOW is ON

    // electrovanne is switch to ON if CITY_LOW is OFF while RAIN_LOW is OFF
    const bool solenoidValve =
        (!GPIO_GET_OUT(SOLENOID_VALVE) && !GPIO_GET_IN(TANK_INT_CITY_LOW)) // switch ON when !CITY_LOW && !SOLENOID_VALVE
        || (GPIO_GET_OUT(SOLENOID_VALVE) && !GPIO_GET_IN(TANK_INT_RAIN_LOW)); // until RAIN_LOW && SOLENOID_VALVE

    // transfertPump pump is ON if RAIN_LOW if OFF while RAIN_HIGH is OFF and there is watter in external tank
    const bool transfertPump = !GPIO_GET_IN(TANK_EXT_EMPTY) && (
            (!GPIO_GET_OUT(TRANSFERT_PUMP) && !GPIO_GET_IN(TANK_INT_RAIN_LOW))
            || (GPIO_GET_OUT(TRANSFERT_PUMP) && !GPIO_GET_IN(TANK_INT_RAIN_HIGH))
        );

    if ((GPIO_GET_OUT(SOLENOID_VALVE) && !solenoidValve) || (GPIO_GET_OUT(TRANSFERT_PUMP) && !transfertPump))
        counters_commit();

    GPIO_SET(PRESSURE_BOOSTER, !pressureBoosterOff);
    GPIO_SET(SOLENOID_VALVE, solenoidValve);
    GPIO_SET(TRANSFERT_PUMP, transfertPump);
}
#endif

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

    timer0_release();

    return distance;
}

bool level_get_tank_ext(LevelTankExt &l)
{
    // simplified area of the external tank:
    //  - half pipe on the top (R ~= 0.85m)
    //  - isoceles trapezoid at the bottom (top: ~1.70; bottom: ~1.10)
    //   \             /
    //    \           /
    //     \_________/
    //
    // half pipe area:
    // see http://villemin.gerard.free.fr/GeomLAV/Cercle/aaaAIRE/Segment.htm
    // area is equal to R^2 * acos(h/R) - h*sqrt(R^2 - h^2)
    // with h = 2R - MesuredDistance
    const int H = 170; // cm
    const int R = 85; // cm
    const int trapezoidTop = 2 * R; // cm
    const int trapezoidBottom = 110; // cm
    const int trapezoidHeight = H - R; // cm
    const double halfPipeFullArea = M_PI_2 * R * R;
    const double trapezoidFullArea = (trapezoidBottom + (trapezoidTop - trapezoidBottom)/2) * trapezoidHeight; // isoceles trapezoid
    const double fullArea = halfPipeFullArea + trapezoidFullArea;
    int meas = level_get_distance();
    if (meas < 0)
        return false;

    meas -= TANK_EXT_DIST_MIN;
    if (meas < 0)
        meas = 0;
    else if (meas > H)
        meas = H;
    double measArea = 0.0;
    if (meas < R)
    {
        // something in half pipe
        const int h = R - meas;
        measArea += halfPipeFullArea - (R*R * acos(((double)h)/R) - h*sqrt(R*R - h*h));
    }

    const int measTrapezoidHeight = min(trapezoidHeight, H - meas);
    const int measTrapezoidTop = trapezoidBottom + (measTrapezoidHeight / trapezoidHeight)*(trapezoidTop - trapezoidBottom);
    measArea += (trapezoidBottom + (measTrapezoidTop - trapezoidBottom)/2) * measTrapezoidHeight;

    const double percent = 100*measArea/fullArea;

    l.empty = GPIO_GET_IN(TANK_EXT_EMPTY);
    l.meas = H-meas;
    l.percent = (int)percent;
    l.capacity = TANK_EXT_CAPACITY/100*percent;

    return true;
}
