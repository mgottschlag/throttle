
throttle HID device
================================

This project is the firmware of an AVR based project which implements throttle
and flap controls for flight simulator as a HID compatible device. The project
uses the awesome [vusb library](www.obdev.at/products/vusb/) for USB support.

Compiling
--------------------------------

On linux, just run *make hex* to produce a .hex file which can be flashed
onto the device. Run *make* to get a list of the possible targets.

Hardware
--------------------------------

In our case, the hardware consists of an Atmega8535 microcontroller clocked at
16Mhz, two toggle switches connected to PC0 and PC1, five buttons connected to
PC2-PC6 and three linear potentiometers connected to PA5-PA7. The USB port is
connected to PD2 and PC4 using two zener diodes to pull the voltage at the data
wires down to 3.3V, for a detailed description see the vusb circuit examples.

If your setup varies from this, you need to change the source accordingly
(mainly usbconfig.h for USB connection changes and throttle.c for input pin
changes). Also you need to edit the makefile to set the target chip model and
speed.

License
--------------------------------

The project is licensed under the GPLv3, while the vusb code in the repository
is licensed under either the GPLv2/v3 or a commercial license, see the license
files in usbdrv/ for more information.
