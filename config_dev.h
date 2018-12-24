#ifndef _CONFIG_H
#error "do not include config_dev.h directly, use config.h"
#endif

#undef PRESSURE_BOOSTER
#define PRESSURE_BOOSTER        GPC3() // green led

#undef SOLENOID_VALVE
#define SOLENOID_VALVE          GPC4() // yellow led

#undef TRANSFERT_PUMP
#define TRANSFERT_PUMP          GPC5() // red led

#undef ETHERNET_MAC_ADDR
#define ETHERNET_MAC_ADDR       {0x74,0x69,0x69,0x2D,0x30,0x32}

#undef ETHERNET_IP_ADDR
#define ETHERNET_IP_ADDR        {192,168,1,200}
