#ifndef _GPIO_H
#define _GPIO_H

#define _CAT(A, B) A ## B
#define CAT(A, B) _CAT(A, B)


//
// define GPIO(PORT, IDX, INVERSED)
//
#define _GPIO_2_ARGS(PORT, IDX)             (PORT, IDX, FALSE)
#define _GPIO_3_ARGS(PORT, IDX, INVERSED)   (PORT, IDX, INVERSED)
// GPIO INVERSED is false by default
#define _GET_GPIO_FCT(arg1, arg2, arg3, fct, ...) fct
#define _GPIO_FCT(...) _GET_GPIO_FCT(__VA_ARGS__, _GPIO_3_ARGS, _GPIO_2_ARGS)
#define GPIO(...)                                        \
    _GPIO_FCT(__VA_ARGS__)(__VA_ARGS__)


//
// get GPIO port
//
#define _GPIO_PORT(P, I, INV) P
#define GPIO_PORT(G) _GPIO_PORT G
#define GPIO_PORT_DIR(G) CAT(DDR, GPIO_PORT(G))
#define GPIO_PORT_NAME(G) CAT(PORT, GPIO_PORT(G))


//
// get GPIO IDX
//
#define _GPIO_IDX(P, I, INV) I
#define GPIO_IDX(G) _GPIO_IDX G


//
// get GPIO is inversed (TRUE) or not (FALSE)
//
#define _GPIO_INV(P, I, INV) INV
#define GPIO_INV(G) _GPIO_INV G


//
// get GPIO bit mask
//
#define GPIO_BIT(G) (1 << GPIO_IDX(G))

//
// Transform GPIO to arduino pin number
//
#define _GPIO_ARDUINO_PORT_OFFSET_B 8
#define _GPIO_ARDUINO_PORT_OFFSET_C 14
#define _GPIO_ARDUINO_PORT_OFFSET_D 0
#define GPIO_ARDUINO_PIN(G) (CAT(_GPIO_ARDUINO_PORT_OFFSET_, GPIO_PORT(G)) + GPIO_IDX(G))


//
// turn GPIO ON/OFF
//

// internal API to set ON/OFF GPIO, does not take into account INVERSED GPIO
#define _GPIO_SET_ON(G)                                             \
    do { GPIO_PORT_NAME(G) |= GPIO_BIT(G); } while (1 == 0)
#define _GPIO_SET_OFF(G)                                            \
    do { GPIO_PORT_NAME(G) &= ~GPIO_BIT(G); } while (1 == 0)

// set ON normal GPIO
#define _GPIO_SET_ON_FALSE(G) _GPIO_SET_ON(G)
// set OFF inversed GPIO
#define _GPIO_SET_ON_TRUE(G) _GPIO_SET_OFF(G)
#define GPIO_SET_ON(G) CAT(_GPIO_SET_ON_, GPIO_INV(G))(G)

// set OFF normal GPIO
#define _GPIO_SET_OFF_FALSE(G) _GPIO_SET_OFF(G)
// set OFF inversed GPIO
#define _GPIO_SET_OFF_TRUE(G) _GPIO_SET_ON(G)
#define GPIO_SET_OFF(G) CAT(_GPIO_SET_OFF_, GPIO_INV(G))(G)

// dynamically set OFF/ON GPIO
#define GPIO_SET(G, v)                                              \
    do { if (v) GPIO_SET_ON(G); else GPIO_SET_OFF(G); } while (1 == 0)

#define _GPIO_TOGGLE(G)                                              \
    do { CAT(PIN, GPIO_PORT(G)) |= GPIO_BIT(G); } while (1 == 0)

#define GPIO_TOGGLE(G) _GPIO_TOGGLE(G)

//
// get GPIO value from INPUT (GPIO_GET_IN) or from OUTPUT (GPIO_GET_OUT)
//

// get normal GPIO value
#define _GPIO_GET_FALSE(vALUE) (vALUE ? true : false)
//get inversed GPIO value
#define _GPIO_GET_TRUE(vALUE) (vALUE ? false : true)

#define _GPIO_GET(G, rEGISTER)                                      \
    CAT(_GPIO_GET_, GPIO_INV(G))(CAT(rEGISTER, GPIO_PORT(G)) & GPIO_BIT(G))

#define GPIO_GET_IN(G) _GPIO_GET(G, PIN)
#define GPIO_GET_OUT(G) _GPIO_GET(G, PORT)



#define GPB0(...) GPIO(B, 0, ##__VA_ARGS__) // [IN]  (D8)  Rain counter
#define GPB1(...) GPIO(B, 1, ##__VA_ARGS__) // [IN]  (D9)  City counter
#define GPB2(...) GPIO(B, 2, ##__VA_ARGS__) // [OUT] (D10) SPI: Slave Select
#define GPB3(...) GPIO(B, 3, ##__VA_ARGS__) // [OUT] (D11) SPI: MOSI
#define GPB4(...) GPIO(B, 4, ##__VA_ARGS__) // [IN]  (D12) SPI: MISO
#define GPB5(...) GPIO(B, 5, ##__VA_ARGS__) // [OUT] (D13) SPI: SCK
#define GPB6(...) GPIO(B, 6, ##__VA_ARGS__) // reserved for XTAL1
#define GPB7(...) GPIO(B, 7, ##__VA_ARGS__) // reserved for XTAL2

#define GPC0(...) GPIO(C, 0, ##__VA_ARGS__) // [OUT] (A0)  DIST_SENSOR_POWER
#define GPC1(...) GPIO(C, 1, ##__VA_ARGS__) // [OUT] (A1)  DIST_SENSOR_TRIGGER
#define GPC2(...) GPIO(C, 2, ##__VA_ARGS__) // [IN]  (A2)  DIST_SENSOR_ECHO
#define GPC3(...) GPIO(C, 3, ##__VA_ARGS__) // [OUT] (A3)  PRESSURE_BOOSTER
#define GPC4(...) GPIO(C, 4, ##__VA_ARGS__) // [OUT] (A4)  SOLENOID_VALVE
#define GPC5(...) GPIO(C, 5, ##__VA_ARGS__) // [OUT] (A5)  TRANSFERT_PUMP
#define GPC6(...) GPIO(C, 6, ##__VA_ARGS__) // reserved for reset

#define GPD0(...) GPIO(D, 0, ##__VA_ARGS__) // reserved for UART/USB RX
#define GPD1(...) GPIO(D, 1, ##__VA_ARGS__) // reserved for UART/USB TX
#define GPD2(...) GPIO(D, 2, ##__VA_ARGS__) // [IN]  (D2)  ENC28J60_INT
#define GPD3(...) GPIO(D, 3, ##__VA_ARGS__) // [IN]  (D3)  TANK_EXT_EMPTY
#define GPD4(...) GPIO(D, 4, ##__VA_ARGS__) // [IN]  (D4)  TANK_INT_EMPTY
#define GPD5(...) GPIO(D, 5, ##__VA_ARGS__) // [IN]  (D5)  TANK_INT_CITY_LOW
#define GPD6(...) GPIO(D, 6, ##__VA_ARGS__) // [IN]  (D6)  TANK_INT_RAIN_LOW
#define GPD7(...) GPIO(D, 7, ##__VA_ARGS__) // [IN]  (D7)  TANK_INT_RAIN_HIGH

#define DIST_SENSOR_POWER       GPC0()
#define DIST_SENSOR_TRIGGER     GPC1()
#define DIST_SENSOR_ECHO        GPC2()

#define TANK_EXT_EMPTY          GPD3(FALSE) // inversed: pull up, logic empty: !!GPD3
#define TANK_INT_EMPTY          GPD4(TRUE)  // inversed: pull up, sensor inversed (0: empty, 1: not empty): !GPD4
#define TANK_INT_CITY_LOW       GPD5(TRUE)  // inversed: pull up
#define TANK_INT_RAIN_LOW       GPD6(TRUE)  // inversed: pull up
#define TANK_INT_RAIN_HIGH      GPD7(TRUE)  // inversed: pull up

#define PRESSURE_BOOSTER        GPC3() // green led
#define SOLENOID_VALVE          GPC4() // yellow led
#define TRANSFERT_PUMP          GPC5() // red led

#define ENC28J60_INT            GPD2(TRUE) // inversed: interrupted by 0 in hardware

// SPI
#define SPI_SS                  GPB2(TRUE)
#define SPI_MOSI                GPB3()
#define SPI_MISO                GPB4()
#define SPI_SCK                 GPB5()

#endif // _GPIO_H
