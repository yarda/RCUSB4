# About
USB joystick adapter for RC receivers with PWM output. It can be used for
VRC Pro or other simulators. Tested on Fedora Linux and Steam Deck.

This is heavily based on the excellent work of Nicholas Regitz:
https://kekse23.de/2017.02.22/778/diy-rc-usb-adapter-fuer-vrc-pro/

![Dongle](/dongle.jpg)

# Code changes
In comparison with the original code the following changes were made:

* Initial auto calibration.
* EEPROM storage of the calibration.
* Refactor, joystick events are now sent at once, not individually for
  each axis, which may improve real-time performance.

# Usage
Wire the dongle to the RC receiver which is paired with the transmitter.

For cars the channel 1 is usually steering, channel 2 is throttle/break.
Channel 2 is usually also used for powering of the receiver.

For aircrafts/drones the mapping is usually AETR (Aileron, Elevator,
Throttle, Rudder), i.e. the channel 1 is usually roll, channel 2 pitch,
channel 3 throttle, and channel 4 yaw.

Connect the dongle to the USB port. Shortly, after the connection to the USB
port the TX LED (usually the blue LED), will start blinking fast. Then you
have cca. 10 seconds for moving of all channel controls through the limit
positions (calibration). When the LED stops blinking the joystick will
start function normally. If you did the calibration, it is saved to the
EEPROM.

If you didn't touch the controls during the 10 seconds calibration
interval, the calibration is restored from the EEPROM.

So the typical usage is:
- connect the dongle to the USB port, wait cca. 14 seconds (4 + 10) until
the blue LED stops blinking, enjoy your game
- if you want to (re)calibrate, (re)plug the dongle to the USB port, wait
cca. 4 seconds until the blue LED starts blinking fast, then you have
cca. 10 seconds interval for calibration, move the controls of the channels
you are interested in through the limit positions. After 10 seconds, the LED
stops blinking and the calibration is saved to the EEPROM, enjoy your game.

# Wiring
I used Arduino Pro Micro from Aliexpress, the 3.3 V / 16 MHz version with
the ATmega 32U4. It seems there are two versions, 3.3 V and 5 V. Both versions
works. For the 3.3 V version you need to close (solder bridge) the J1 jumper
(near the USB connector and the TX0 pin). It will bypass the 3.3 V internal
regulator and it will run on the 5V directly from the USB port. For the 5 V
version bypassing the regulator is optional, but recommended, because you
will get 5 V instead of the cca. 4.8 V due to the voltage drop on the internal
regulator.

![Jumper J1](/promicro_j1.png)

Channels wiring for the Arduino Pro Micro board from Aliexpress:
|PCB marking|ATmega 32U4 marking|Channel|
|-----------|-------------------|-------|
|3          |PD0                |1      |
|2          |PD1                |2      |
|RXI        |PD2                |4      |
|TX0        |PD3                |3      |

Connect VCC and GND only for the channel 2. For the rest of channels leave it
unconnected. I guess this is to lower signal noise floor and to prevent
possible shorts if the receiver is really badly wired (normally, reversing the
servo connector is safe, because VCC is in the middle and PWM output can
usually withstand 5V/GND without problem).

For some reason currently unknown to me, the Arduino boards have the port D
marked differently from the datasheet, so I used the wiring table above.

![Wiring](/promicro_wiring.png)

# Case
Good 3D printed case design:
https://www.thingiverse.com/thing:4543077

It seems it's designed for the 33 mm long boards, which are probably boards with the
USB micro connector. I used board with the USB-C connector which is 35 mm long,
so I had to enlarge the longer side of the case by 2.6 mm for a perfect fit.

It's probably for 2 channels dongles, to accomodate wires for 4 channels I had
to enlarge the height of both lids 0.5 mm each, i.e. the 1 mm total enlargement.
I also had to file a cable hole a bit for 4 channels to fit.

Finally I drilled two mini holes (1 mm) for the RX/TX LEDs.

It would be better to modify the 3D design, but unfortunately, there is only
STL file available and I didn't have a time to mod it or redesign it from the
scratch.

# Compilation
You will need the Joystick library from the:
https://github.com/MHeironimus/ArduinoJoystickLibrary

Put it to your Arduino libraries directory.

In the project set the board to "Arduino Micro".

# History
Jaroslav Škarvada <jskarvad@redhat.com>
2025/04/20, refactor, added auto calibration, EEPROM support

kekse23.de RCUSB4 v1.1
Copyright (c) 2020, Nicholas Regitz

# License
Creative Commons 4.0 CC BY-NC-SA
https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
