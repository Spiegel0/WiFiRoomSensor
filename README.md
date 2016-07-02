WiFiRoomSensor
==============

The WiFiRoomSensor is a piece of hardware which provides a network interface to measure the temperature and humidity. It also provides some buttons and RGB LEDs to interact with the HVAC controller. It acts as a distributed IO device and does not implement any HVAC logic. The WiFiRoomSensor is tested with the IEC 61499 compatible HVAC controller [4DIAC Forte](http://www.eclipse.org/4diac/) and [FBRT](http://holobloc.com/). Since the sensor is designed to fulfill a very specific purpose, it may not be directly useful. However, I tried to abstract the functionality in several modules which might be useful by its own. Please be aware that the sensor is still under active development. If you find a bug or if you need some information, please feel free to write me an e-mail or open a ticket. If you want to start hacking into the design, I suggest building the doxygen documentation which may give further hints.

# Features

Currently, the following features are implemented:

- WiFi interface by an [ESP8266 module](https://www.olimex.com/Products/IoT/MOD-WIFI-ESP8266/open-source-hardware) with its default firmware.
- IEC 61499 (ASN.1) - based transmission protocol
- Up to two independent temperature/humidity channels (DHT22/DHT11/AM2303)
- Drives WS2801 compatible RGB LED strips
- Based on a [AVR Mega8](http://www.atmel.com/Images/Atmel-2486-8-bit-AVR-microcontroller-ATmega8_L_datasheet.pdf) processor

Planned features which are still under development:
- Simple user interface which is driven by three buttons ("Up", "Down", "Ok")

I used the external processor instead of the ESP 8266 because the available documentation of the ESP8266 was not satisfying. It turned out that the AT command set of the flashed firmware also deviates from the documentation. In addition, the available documentation of the ESP 8266 was significantly improved recently. From todays point of view I would use the ESP 8266 only, but I also don't want to rewrite all the code.

# License and Warranty

The project is primarily licensed by the GPL. If you need a more commercially 
friendly open source license, please feel free to contact me.

WiFiRoomSensor

Copyright (C) 2016 Michael Spiegel
E-Mail: michael.h.spiegel@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

