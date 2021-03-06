#ifndef _CONFIG_H
#define _CONFIG_H

#include "gpio.h"

//
// levels
//
#define DIST_SENSOR_POWER       GPC0()
#define DIST_SENSOR_TRIGGER     GPC1()
#define DIST_SENSOR_ECHO        GPC2()

#define TANK_EXT_EMPTY          GPD3(FALSE) // inversed: pull up, logic empty: !!GPD3
#define TANK_INT_EMPTY          GPD4(TRUE)  // inversed: pull up, sensor inversed (0: empty, 1: not empty): !GPD4
#define TANK_INT_CITY_LOW       GPD5(TRUE)  // inversed: pull up
#define TANK_INT_RAIN_LOW       GPD6(TRUE)  // inversed: pull up
#define TANK_INT_RAIN_HIGH      GPD7(TRUE)  // inversed: pull up

#define PRESSURE_BOOSTER        GPC3(FALSE) // normal: relay command inversed, relay output inversed
#define SOLENOID_VALVE          GPC4(TRUE) // inversed
#define TRANSFERT_PUMP          GPC5(TRUE) // inversed

#define TANK_EXT_CAPACITY       6000 // external tank capacity is 6000L
#define TANK_EXT_DIST_MIN       28 // distance between sensor and water when tank is full

//
// SPI
//
#define SPI_SS                  GPB2(TRUE)
#define SPI_MOSI                GPB3()
#define SPI_MISO                GPB4()
#define SPI_SCK                 GPB5()

//
// counters
//
#define COUNTER_RAIN            GPB0()
#define COUNTER_CITY            GPB1()
#define COUNTER_CITY_PULSE_PER_LITER 590
#define COUNTER_RAIN_PULSE_PER_LITER 573


//
// ethernet
//
// configure ethernet buffer size
#define ETHERNET_BUFFER_SIZE    400
// define (unique on LAN) hardware (MAC) address
#define ETHERNET_MAC_ADDR       {0x74,0x69,0x69,0x2D,0x30,0x31}
#define ETHERNET_IP_ADDR        {192,168,1,7}
#define ETHERNET_IP_NET_MASK    {255,255,255,0}
#define ETHERNET_IP_GW          {192,168,1,2}
#define ETHERNET_IP_DNS         {0,0,0,0}

#define ETHERNET_ENC28J60_INT   GPD2(TRUE) // inversed: interrupted by 0 in hardware


#define COAP_PORT 5683

// override "production" env configuration for development env
#ifdef TANK_DEV
#include "config_dev.h"
#endif

#endif // _CONFIG_H
