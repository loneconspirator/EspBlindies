= EspBlindies

Blindies is the term for the stupidly bright blinky lights that [Chaotic Noise Marching Corps](https://www.chaoticnoise.com) wears when performing. This is the code for their "brains", which is programmed on ESP8266 and powers said lights. It can also drive a separate strip of addressable RGB LEDs simultaneously.

== Setup
I have used the [AdaFruit Huzzah](https://www.adafruit.com/product/2471) to is a fairly simple circuit driving 12v blindies through a transistor. There are excellent instructions there on getting code on the chip linked from the Huzzah page.

To get the code set up, you'll need to open the patch in the Arduino editor and load the basic EEPROM library, [install the NeoPixel library](https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-installation) and download the [WifiCreds](https://github.com/loneconspirator/WifiCreds) library.

You will then need to copy `WifiDefaults.example.h` to `WifiDefaults.h` and include the default credentials you want to use.

Once the brains are running on your network, you send udp packets with commands at them. 

(Sorry I don't have a circut diagram or specs on the udp commands, these are a few years old and I'm putting this together mostly from memory, hopefully this project will get warmed up again and the documentation will get better)
