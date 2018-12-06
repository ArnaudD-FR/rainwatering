#include <Arduino.h>
#include <EtherCard.h>


#include "gpio.h"
#include "network.h"
#include "lpm.h"


#define COAP_PORT 5683
static uint8_t Ethernet::buffer[700]; // configure buffer size to 700 octets
static uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 }; // define (unique on LAN) hardware (MAC) address
const static uint8_t ip[] = {192,168,1,7};
const static uint8_t gw[] = { 192,168,1,2};
const static uint8_t dns[] = {0,0,0,0};
const static uint8_t mask[] = {255,255,255,0};

void network_PCINT2()
{
}

void network_coap_handle(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len)
{
    Serial.print("dest_port: ");
    Serial.println(dest_port);
    Serial.print("src_port: ");
    Serial.println(src_port);
    Serial.print("src_port: ");
    ether.printIp(src_ip);
    Serial.println("\ndata: ");
    Serial.println(data);
    ether.sendUdp(data, len, dest_port, src_ip, src_port);
}

void network_setup()
{
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
    // if (intCount >0 )
    // {
    //     Serial.print(intPending);
    //     Serial.print('/');
    //     Serial.print(intCount);
    //     Serial.println(")");
    // }
    // // cli(); // disable interrupts
    while (GPIO_GET_IN(ENC28J60_INT) > 0)
    {
    //     // sei();
        timer0_acquire();
        ether.packetLoop(ether.packetReceive());
        timer0_release();
        // cli(); // disable interrupts
    }
    // sei();
}

