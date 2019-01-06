#include <Arduino.h>
#include <EtherCard.h>


#include "coap.h"
#include "config.h"
#include "counters.h"
#include "interrupt.h"
#include "level.h"
#include "lpm.h"
#include "network.h"
#include "settings.h"


uint8_t Ethernet::buffer[ETHERNET_BUFFER_SIZE];
static uint8_t mymac[] = ETHERNET_MAC_ADDR;
const static uint8_t ip[] = ETHERNET_IP_ADDR;
const static uint8_t gw[] = ETHERNET_IP_GW;
const static uint8_t dns[] = ETHERNET_IP_DNS;
const static uint8_t mask[] = ETHERNET_IP_NET_MASK;
static char json[64];

static char *append(char *dest, const char *src, const size_t len)
{
    memcpy(dest, src, len);
    return dest + len;
}

template <typename T>
struct AppendStr;

template <typename T>
static inline char *append(char *dest, const T &src)
{
    return AppendStr<T>::append(dest, src);
}


template <size_t N>
struct AppendStr<char [N]>
{
    static inline char *append(char *dest, const char *src)
    {
        return ::append(dest, src, N)-1;
    }
};

template <>
struct AppendStr<int>
{
    static char *append(char *dest, const int src)
    {
        itoa(src, dest, 10);
        return dest + strlen(dest);
    }
};

template <>
struct AppendStr<unsigned int>
{
    static char *append(char *dest, const int src)
    {
        utoa(src, dest, 10);
        return dest + strlen(dest);
    }
};

template <>
struct AppendStr<long>
{
    static char *append(char *dest, const long src)
    {
        ltoa(src, dest, 10);
        return dest + strlen(dest);
    }
};

template <>
struct AppendStr<unsigned long>
{
    static char *append(char *dest, const long src)
    {
        ultoa(src, dest, 10);
        return dest + strlen(dest);
    }
};

template <>
struct AppendStr<bool>
    : public AppendStr<int>
{
};

static const coap_endpoint_path_t network_coap_path_levels_ext = { 2, { "levels", "ext" } };
static int network_coap_handle_levels_ext(
        coap_rw_buffer_t *scratch,
        const coap_packet_t *inpkt,
        coap_packet_t *outpkt,
        uint8_t id_hi,
        uint8_t id_lo
    )
{
    static const char emptyStr[] = "{empty:";
    static const char measStr[] = ",meas:";
    static const char percentStr[] = ",percent:";
    static const char remainCapacityStr[] = ",remains:";
    static const char endStr[] = "}";
    char *iter = json;

    LevelTankExt l;
    if (!level_get_tank_ext(l))
    {
        return coap_make_response(
                scratch,
                outpkt,
                NULL,
                0,
                id_hi,
                id_lo,
                &inpkt->tok,
                COAP_RSPCODE_INT_SRV_ERR,
                COAP_CONTENTTYPE_NONE
            );
    }



    // json = "{empty:0,meas=170,percent:100,liter:6000}";
    iter = append(iter, emptyStr);
    iter = append(iter, l.empty);
    iter = append(iter, measStr);
    iter = append(iter, l.meas);
    iter = append(iter, percentStr);
    iter = append(iter, l.percent);
    iter = append(iter, remainCapacityStr);
    iter = append(iter, l.capacity);
    iter = append(iter, endStr);
    ++iter;

    Serial.print("json:");
    Serial.println(json);

    return coap_make_response(
            scratch,
            outpkt,
            (const uint8_t *)json,
            iter - json,
            id_hi,
            id_lo,
            &inpkt->tok,
            COAP_RSPCODE_CONTENT,
            COAP_CONTENTTYPE_APPLICATION_JSON
        );
}

static const coap_endpoint_path_t network_coap_path_counters = { 1, { "counters" } };
static int network_coap_handle_counters(
        coap_rw_buffer_t *scratch,
        const coap_packet_t *inpkt,
        coap_packet_t *outpkt,
        uint8_t id_hi,
        uint8_t id_lo
    )
{
    static const char rainStr[] = "{rain:";
    static const char cityStr[] = ",city:";
    static const char endStr[] = "}";
    char *iter = json;

    SettingsCounters c;
    counters_get(c);

    // json = "{rain:900000,city:200000}";
    iter = append(iter, rainStr);
    iter = append(iter, c.rain/COUNTER_RAIN_PULSE_PER_LITER);
    iter = append(iter, cityStr);
    iter = append(iter, c.city/COUNTER_CITY_PULSE_PER_LITER);
    iter = append(iter, endStr);
    ++iter;

    Serial.print("json:");
    Serial.println(json);

    return coap_make_response(
            scratch,
            outpkt,
            (const uint8_t *)json,
            iter - json,
            id_hi,
            id_lo,
            &inpkt->tok,
            COAP_RSPCODE_CONTENT,
            COAP_CONTENTTYPE_APPLICATION_JSON
        );
}

const coap_endpoint_t endpoints[] =
{
    { COAP_METHOD_GET, network_coap_handle_levels_ext, &network_coap_path_levels_ext, "ct=50"},
    { COAP_METHOD_GET, network_coap_handle_counters, &network_coap_path_counters, "ct=50"},
    {(coap_method_t)0, NULL, NULL, NULL}
};

static void network_coap_handle(
        uint16_t dest_port,
        uint8_t src_ip[IP_LEN],
        uint16_t src_port,
        const char *data,
        uint16_t len
    )
{
    // Serial.print("dest_port: ");
    // Serial.println(dest_port);
    // Serial.print("src_port: ");
    // Serial.println(src_port);
    // Serial.print("src_port: ");
    // ether.printIp(src_ip);
    // Serial.println("\ndata: ");
    // Serial.println(data);
    // ether.sendUdp(data, len, dest_port, src_ip, src_port);
    // return;
    int rc;
    coap_packet_t pkt;
    if ((rc = coap_parse(&pkt, (const uint8_t *)data, len)) != 0)
    {
        Serial.print("Bad packet rc=");
        Serial.println(rc, DEC);
        return;
    }

    uint8_t rsp[128];
    size_t rsplen = sizeof(rsp);
    coap_packet_t rsppkt;
    static uint8_t scratch_raw[32];
    static coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};
    coap_handle_req(&scratch_buf, &pkt, &rsppkt);
    if ((rc = coap_build(rsp, &rsplen, &rsppkt)) != 0)
    {
        Serial.print("coap_build failed rc=");
        Serial.println(rc, DEC);
        return;
    }
    ether.sendUdp((const char *)rsp, rsplen, dest_port, src_ip, src_port);
}

void network_dsr(uint8_t)
{
    while (GPIO_GET_IN(ETHERNET_ENC28J60_INT) > 0)
    {
        timer0_acquire();
        ether.packetLoop(ether.packetReceive());
        timer0_release();
    }
}

void network_setup()
{
    coap_setup();

    Serial.println("Configuring Ethernet controller...");
    timer0_acquire();
    if (ether.begin(sizeof Ethernet::buffer, mymac, GPIO_ARDUINO_PIN(SPI_SS)) == 0)
        Serial.println( "Failed to access Ethernet controller");
    ether.staticSetup(ip, gw, dns, mask);
    while (ether.clientWaitingGw())
    {
        ether.packetLoop(ether.packetReceive());
    }
    timer0_release();
    Serial.println("Ethernet controller configured");

    ether.udpServerListenOnPort(&network_coap_handle, COAP_PORT);

    // enable interrupts
    DDRD &= ~(GPIO_BIT(ETHERNET_ENC28J60_INT));
    PCICR |= (1 << PCIE2);
    PCMSK2 |= GPIO_BIT(ETHERNET_ENC28J60_INT);

    INT_REGISTER_DSR(ETHERNET_ENC28J60_INT, INT_RISING, network_dsr);
}

