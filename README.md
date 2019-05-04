# rainwatering

This project has for purpose to completely automate the usage of rain water collected from roof in an external tank and when this one is empty then switch to water from city supply network. This water can then be used for example for garden watering when required.

As physical constraint, the water pipe coming from city supply network must never be in contact with rain water to avoid city water contamination. To avoid this a small internal tank is used to mix water sources.

This project is also used to monitor:
 * the rain water level in external tank and if empty or not.
 * rain water usage
 * city water usage

Low power components:
 * Arduino Nano
 * ENC28J60 chip as Ethernet interface (see [Ethercard](https://github.com/ArnaudD-FR/EtherCard), some fix are not yet included in upstream)
 * COAP protocol (no other purpose than testing COAP, sources are from [microcoap](https://github.com/1248/microcoap))
 * Jeedom as monitoring interface
 
 And high power components:
 * SEN0208 weather proof, ultrasonic sensor
 * solenoid valve to control water from city supply network (Danfoss EV251B 032U538131)
 * transfer pump to move rain water from external tank to internal one.
 * water pressure booster to provide pressure in a dedicated network.
