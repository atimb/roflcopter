# RoflCopter quadrotor project

The result of hundreds of hours of research work on our quadrotor (http://en.wikipedia.org/wiki/Quadrotor)
(the ROFLcopter), shared and open sourced for your pleasure.

There are quite a few RC UAV platforms out there, but not too many with an HTML5 control interface,
so if you like the idea, feel free to incorporate in your (non-commercial) project the best ideas
you find here.

## How it works

The flight platform is an Atmega8 based circuit. The attached sensors are:

* 3x ADXRS610 angular rate sensor
* 1x LIS3LV02DQ 3-axis acceleration sensor

The communication link is provided by an XBee Pro 60mW I/O device used in point-2-point raw serial mode.

In our model, the four TowerPro 2410-08T 890kv engines are driven by four modified TowerPro 18A BEC ESC. (Reprogrammed
to use I2C instead PPM input. See http://www.mikrokopter.de/ucwiki/TowerPro18A-I2C)

On the computer (PC/Mac/whatever) side sits an XBee Exporer USB, that with the installed FTDI driver appears as a serial
device on the computer.
The HTML5 control interface is ready to establish the communication link to the RoflCopter, so here comes
the node.js proxy, that makes it possible. It effectively creates a tunnel: /dev/tty-usb-xyz <-> socket.io,
making it possible in the browser to have full async access to the XBee serial interface.

The HTML5 interface provides controlling and debugging functionalities, that can be easily implemented/extended
given the simplicity of html/js.
Current features include:

* Pull-in HEX file to bootload new firmware (HTML5 File Api)
* Copter orientation 3D display (CSS3 transformations)
* Engine status display (new `progress` tag)
* Sensor output graphs (`canvas` element)
* RC Transmitter inputs display (`canvas` element)

## Structure

### atmega8-platform

The platform firmware itself. Not too much magic (as of now), quite consistent and self-describing function and
variable names.
The main modules are:

* USART communication: This is the interface toward the computer through the XBee. Checksum validation, re-synching,
and handles the debug- and control messages received from the HTML5 side.
* TWI/I2C communication: Sending thrust data to engines, interrogating the acceleration sensor.
* RC receiver: Decoding the HobbyKing 2.4GHz 6-channel rx data. (See: some RCgroups thread... cant find now)
* Analog Digital Converter (ADC): To read the rate sensors' output signal.
* Business Logics: The part that calculates the actual engine thrust values from all the inputs.

### nodejs-server

First install node.js and npm, then proceed with installing the neccessary modules:

    npm install express socket.io serialport

Finally start it (sudo is only neccessary to bind to port 80):

    sudo node xbee-proxy.js

All the HTML5 goodie can be found under the `/web` directory. It is automatically served by the node.js process:
Just open http://localhost/ in your browser (preferably Chrome), and enjoy!

### tools/bootloader8

The SparkFun atmega wireless bootloader modified to have a stronger integrity check (instead 1-byte checksum, 4-byte hash),
because we faced some noisy reception issues.
Basically this is a 2K program code residing in the upper section of the atmega flash, and is write protected with
appropriate fuses. Every time the atmega starts up, code execution starts from this address, and it can update the firmware
with data received from XBee on USART, or otherwise, simply jump to the execution to our main function.

### tools/matlab-simulation

A quadrotor stability & autopilot simulation written in MATLAB. Uses rigid body mechanics theory to derive
the differential equations.

## License

(C) 2011 Attila Incze <attila.incze@gmail.com>, Balazs Nagy <balazs.1.nagy@nsn.com>

http://atimb.me

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit
http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
