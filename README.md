
Drive a two-wheel robot. 
    * Platform: ESP32 variant
    * Uses dual L298N H-bridge to drive two DC motors.
    * Uses Quadrature encoder on the Wheels.

    Sensor Fusion used to determine motion (and position?)
        Uses GY-521 Accel/gryo for direction control
        Opt: Use LIDAR for position

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
LN298N motor driver for 2 DC motors, using a L298N chip. 

NO: Arduino PWM library (lib name in platformio is khoih-prog/ESP32_PWM@^1.3.3)
Using ESP_IDF 'ledc' timer.
Pulse is currently 4khz.

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
Quadrature encoding used on the two motors. Link sensor to shaft
via 1:1 pulley (need to design in onshape!) (?Use T2 belt?)

The arduino PID library by Brett Beauregard is used. (library name in platformio.ini is br3ttb/PID@^1.2.1)
Documentation at https://playground.arduino.cc/Code/PIDLibrary/

Configuration:   pulses per rev, wheel diameter, speed check interval.

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
MotorControl - one per motor
   This combines the position and speed from the Quadrature decoder with the
   pulse with setting of the h298 driver to vary the power to the motor
   as needed to maintain the requested motor speed.
   Motor speed is in MM/milliseconds.

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
Driver - this implements 2 MotorControl instances, receives commands from
   various input channges (e.g.: turn left at given rate, move forward at given speed), and sets the two motors appropriatly.

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
FUTURE
= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
GY-521 ( 5/14/2025) not currently implemented)- uses I2C for communication. Provides Gyroscope and  Accelerometer
    AND tempeture      There is a Motion Processor on board

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
Future: LIDAR - located on top of robot, should provide distance and direction
    input via serial port (input only).   This will require some processing...

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
Serial port AND UDP...

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
COMMANDS (def style):
We assume that commands come from a channel (e.g.: UDP Port, Serial port, TCP port etc),
which is implemented by a 'channel' class.   This class will have "Commands" as a base class. 
    The "Command" class handles parsing commands from an input stream provided by the 'channel',
    running those commands, and formatting responses which are to be sent back on the
    'channel'
    The following virtual routines need to be implemented to send responses
   back to the originator

    // Determine if the argument indicates 'end-of-command'
    virtual bool isEndOfChar(char ch);

    // Write repsponses back to the originator of the command
    virtual size_t write(uint8_t) = 0;                        // user specified

    virtual size_t write(const uint8_t *buffer, size_t size); // user specified

    virtual int availableForWrite() { return 0; }   // default - does nothing

The actual commands are defined in CommandsList.h. All commands are parsed
(by 'commands') in the same way - the command line is broken down into
a list of whitespace-separated tokens. The first token is expected to be the
command. All other words are arguments to the command line. 
The command parser scans thew cmdList for an entry where the first argument 
matches a command, and the total number of arguments is within the specified
limits. If found, the command is called with the following arguments:

void (*function)(Print *outdev, int tokCnt, char *toklist[]);
    if 'function' is part of a class, it *must* be 'static'.

    outdev points to a 'print' class device to output responses that will
    be sent back to the originator of the command.

    tokCnt is the total number of tokens

    toklist is the list of tokens from the command line. It MUST NOT 
    be altered.

Also note that a 'help' function is available, which simply reports the description
of each command (as entered in 'cmdList').

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
SMAC Commands:

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
Backtrace:
   ::: cd to ${PROJECT}/.pio/build/esp32doit-devkit-v1
   ::: esp32_decoder.sh firmware.elf
   ::: <input backtrace line>
   


   