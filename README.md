# HolidayLights - A HomeSpan Project

HolidayLights demonstaret is a universal, multi-channel, HomeKit Controller for Somfy RTS Motorized Window Shades and Patio Screens.  Built using the [HomeSpan](https://github.com/HomeSpan/HomeSpan) HomeKit Library, SomfyPlus is designed to run on an ESP32 device as an Arduino sketch.  

Hardware used for this project:

* An ESP32 board, such as the [Adafruit HUZZAH32 – ESP32 Feather Board](https://www.adafruit.com/product/3405)
* A 433 MHz RFM69 Transceiver, such as this [RFM69HCW FeatherWing](https://www.adafruit.com/product/3230) from Adafruit
* Three pushbuttons (normally-open) to serve as the Somfy UP, DOWN, and MY buttons (the MY button also serves as the HomeSpan Control Button), such as these [12 mm round tactile buttons](https://www.adafruit.com/product/1009) from Adafruit
* One LED and current-limiting resistor to serve as the HomeSpan Status LED
* One LED and current-limiting resistor to provide visual feedback when the RFM69 is transmitting Somfy RTS signals
* a [Adafruit FeatherWing Proto Board](https://www.adafruit.com/product/2884) to mount the buttons and LEDs
