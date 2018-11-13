#include <Arduino.h>
#include <avr/sleep.h>
#include <EtherCard.h>

// local includes
#include "gpio.h"

// #define DEBUG_LEVELS

volatile static int countC = 0;
volatile static int countD = 0;
volatile static unsigned long echoStart;
volatile static unsigned long echoEnd;

volatile static bool surpresseur = false;
#ifdef DEBUG_LEVELS
volatile static bool electrovanne = false;
volatile static bool transfert = false;
#endif

void refresh_pompes();

ISR(PCINT1_vect)
{
    static uint8_t previous = 0;

    ++countC;

    if ((previous & GPIO_BIT(DIST_SENSOR_ECHO)) != (PINC & GPIO_BIT(DIST_SENSOR_ECHO)))
        (PINC & GPIO_BIT(DIST_SENSOR_ECHO) ? echoStart : echoEnd) = micros();

    previous = PINC;
}

ISR(PCINT2_vect)
{
    ++countD;

    refresh_pompes();
}

#ifdef DEBUG_LEVELS
void log_levels()
{
    const bool _surpresseur = surpresseur;
    const bool _electrovanne = electrovanne;
    const bool _transfert = transfert;
    const int _countC = countC;
    const int _countD = countD;
    const uint8_t _portC = PORTC;
    const uint8_t _pinC = PINC;
    const uint8_t _portD = PORTD;
    const uint8_t _pinD = PIND;
    const bool tankExtEmpty = GPIO_GET_IN(TANK_EXT_EMPTY);
    const bool tankIntEmpty = GPIO_GET_IN(TANK_INT_EMPTY);
    const bool cityLow = GPIO_GET_IN(TANK_INT_CITY_LOW);
    const bool rainLow = GPIO_GET_IN(TANK_INT_RAIN_LOW);
    const bool rainHigh = GPIO_GET_IN(TANK_INT_RAIN_HIGH);

    sei();

    Serial.print("surpresseur: ");
    Serial.print(_surpresseur);
    Serial.print("; electrovanne: ");
    Serial.print(_electrovanne);
    Serial.print("; transfert: ");
    Serial.println(_transfert);

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


    Serial.print("; C: ");
    Serial.print(_countC);
    Serial.print("; D: ");
    Serial.println(_countD);
    cli();
}
#endif

void setup()
{
    //
    // intialize I/O
    //
    // set full PORT B as input. EtherCard will configure SPI pins
    DDRB = 0;

    // DDRC output
    DDRC |= GPIO_BIT(DIST_SENSOR_TRIGGER)
            | GPIO_BIT(DIST_SENSOR_POWER)
            | GPIO_BIT(SURPRESSEUR)
            | GPIO_BIT(ELECTROVANNE)
            | GPIO_BIT(POMPE_TRANSFERT);

    // DDRC input + pull-up
    DDRC &= ~(GPIO_BIT(DIST_SENSOR_ECHO));
    PORTC |= GPIO_BIT(DIST_SENSOR_ECHO);

    // DDRD input + pull-up
    DDRD &= ~(
                GPIO_BIT(ENC28J60_INT)
                | GPIO_BIT(TANK_EXT_EMPTY)
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

    refresh_pompes();

    Serial.begin(57600);
    Serial.println("Enjoy distance sensor: ");
#ifdef DEBUG_LEVELS
    log_levels();
#endif
}

void refresh_pompes()
{
    // surpresseur is ON if internal tank is not empty
    const bool _surpresseur = !GPIO_GET_IN(TANK_INT_EMPTY);
    // electrovanne is switch to ON if CITY_LOW is OFF while RAIN_LOW is OFF
    const bool _electrovanne
        = (!GPIO_GET_OUT(ELECTROVANNE) && !GPIO_GET_IN(TANK_INT_CITY_LOW)) // switch ON when !CITY_LOW && !ELECTROVANNE
        || (GPIO_GET_OUT(ELECTROVANNE) && !GPIO_GET_IN(TANK_INT_RAIN_LOW)); // until RAIN_LOW && ELECTROVANNE
    // pompe de transfert is ON if RAIN_LOW if OFF while RAIN_HIGH is OFF and there is watter in external tank
    const bool _transfert = !GPIO_GET_IN(TANK_EXT_EMPTY) && (
            (!GPIO_GET_OUT(POMPE_TRANSFERT) && !GPIO_GET_IN(TANK_INT_RAIN_LOW))
            || (GPIO_GET_OUT(POMPE_TRANSFERT) && !GPIO_GET_IN(TANK_INT_RAIN_HIGH))
        );
    GPIO_SET(SURPRESSEUR, _surpresseur);
    GPIO_SET(ELECTROVANNE, _electrovanne);
    GPIO_SET(POMPE_TRANSFERT, _transfert);

    surpresseur = _surpresseur;
#ifdef DEBUG_LEVELS
    electrovanne = _electrovanne;
    transfert = _transfert;
#endif
}

int _get_distance()
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
int get_distance()
{
    int distance = -1;
    GPIO_SET_ON(DIST_SENSOR_POWER);

    // empiric delay to allow distance sensor to start
    delay(30);

    // sometimes, mesure seems to be shorter as expecter, so do 3 measurements
    // and takes the max
    for (int i = 0; i < 3; ++i)
        distance = max(distance, _get_distance());

    GPIO_SET_OFF(DIST_SENSOR_POWER);
    Serial.print("final distance: ");
    Serial.println(distance);

    return distance;
}

void loop()
{
    Serial.println("loop");
    GPIO_TOGGLE(GPB5());
    get_distance();
    delay(1000);// Wait 50mS before next ranging

#if 0
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sei(); // enable interrupts

    sleep_cpu();
    sleep_disable();

    cli(); // disable interrupts
#endif
#ifdef DEBUG_LEVELS
    log_levels();
#endif
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

