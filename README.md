# AirDuino

## Data Logger for Ultralight Aircraft

### Installation

#### Required Hardware

- [Arduino MEGA 2560](https://www.arduino.cc/en/Main/arduinoBoardMega)
- [2.8" Resistive Touch TFT](http://www.adafruit.com/products/1651)
- [10-DOF](http://www.adafruit.com/products/1604)
- [HC-SR04 Ultrasonic Ranger](http://arduinobasics.blogspot.com.au/2012/11/arduinobasics-hc-sr04-ultrasonic-sensor.html)

#### Required Libraries

- [Adafruit ILI9341](https://github.com/adafruit/Adafruit_ILI9341)
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit STMPE610](https://github.com/adafruit/Adafruit_STMPE610)
- [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor)
- [Adafruit LSM303DLHC](https://github.com/adafruit/Adafruit_LSM303DLHC)
- [Adafruit L3GD20 U](https://github.com/adafruit/Adafruit_L3GD20_U)
- [Adafruit BMP085](https://github.com/adafruit/Adafruit_BMP085_Unified)
- [Adafruit 10DOF](https://github.com/adafruit/Adafruit_10DOF)

#### Connecting it Up

All hardware is tested to work off 5V USB power. All sensors use the Arduino 5V rails.

##### 10-DOF

Two digital pins are required of the 10-DOF, SCL and SDA. Connect these to the `SCL` and `SDA pins`, pins `21` and `20` of the MEGA, respectively. Connect `VIN` to 5V, and `GND` to ground.

##### HC-SR04

Two digital pins are required of the HC-SR04. `TRIG` is configured to connect to pin `22`, and `ECHO` to pin `23`. Modify `#define trigPin` and `#define echoPin` to use different pins.

##### 2.8" Resistive Touch TFT

When using the shield form factor, all pins are predefined. If the shield is connected, but not directly on top of the MEGA, `TFT_DC`, `TFT_CS` and `SD_CS` may be modified if needed.

The TFT is [not quite ready to work with a MEGA out of the box](https://learn.adafruit.com/adafruit-2-8-tft-touch-shield-v2/connecting#using-with-a-mega-slash-leonardo). The `11`, `12` & `13` pads must be cut, and the `ISCP` pads must be soldered together.

### License

The MIT License (MIT)

Copyright (c) 2015 Ellis Clayton

See LICENSE for full license

**Neither the AirDuino software, or connected hardware is a replacement for any certified aircraft instrument. Any use of this software and hardware is done at your own risk.** 