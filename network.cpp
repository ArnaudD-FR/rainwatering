#include <Arduino.h>
#include <EtherCard.h>


#include "coap.h"
#include "gpio.h"
#include "level.h"
#include "lpm.h"
#include "network.h"
#include "settings.h"


#define COAP_PORT 5683
uint8_t Ethernet::buffer[700]; // configure buffer size to 700 octets
static uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 }; // define (unique on LAN) hardware (MAC) address
const static uint8_t ip[] = {192,168,1,7};
const static uint8_t gw[] = { 192,168,1,2};
const static uint8_t dns[] = {0,0,0,0};
const static uint8_t mask[] = {255,255,255,0};

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

static const coap_endpoint_path_t network_coap_path_levels_ext = { 2, { "levels", "ext" } };
static int network_coap_handle_levels_ext(
        coap_rw_buffer_t *scratch,
        const coap_packet_t *inpkt,
        coap_packet_t *outpkt,
        uint8_t id_hi,
        uint8_t id_lo
    )
{
    static const char minstr[] = "{min:";
    static const char maxstr[] = ",max:";
    static const char currentstr[] = ",current:";
    static const char percentstr[] = ",percent:";
    static const char literstr[] = ",liter:";
    static const char endstr[] = "}";

    SettingsLevels levels;
    settings_load(levels);

    static char json[64];
    char *iter = json;
    const int distance = level_get_distance();
    const int percent = ((distance - levels.max)/(levels.min - levels.max))*100;
    const int liter = 6000/100*percent;
    // json = "{min:123,max:123,current=123,percent:100,liter:11}";
    iter = append(iter, minstr);
    iter = append(iter, (int)levels.min);
    iter = append(iter, maxstr);
    iter = append(iter, (int)levels.max);
    iter = append(iter, currentstr);
    iter = append(iter, distance);
    iter = append(iter, percentstr);
    iter = append(iter, percent);
    iter = append(iter, literstr);
    iter = append(iter, liter);
    iter = append(iter, endstr);
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
    DDRD &= ~(GPIO_BIT(ENC28J60_INT));
    PCICR |= (1 << PCIE2);
    PCMSK2 |= GPIO_BIT(ENC28J60_INT);
}

void network_loop()
{
    while (GPIO_GET_IN(ENC28J60_INT) > 0)
    {
        timer0_acquire();
        ether.packetLoop(ether.packetReceive());
        timer0_release();
    }
}

