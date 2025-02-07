
Drive a two-wheel robot. 
    Uses dual L298N H-bridge to drive two DC motors.
    Sensor Fusion to determine motion (and position?)
        Uses GY-521 Accel/gryo 
        Uses wheel position encoder
        Opt: Use LIDAR for position

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
LN298N is a motor driver for 2 DC motors, using a L298N chip. 
It has two inputs for each motor. 

= = = = = = = 
COMMANDS:
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

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =

= = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = =
Backtrace:
   ::: cd to ${PROJECT}/.pio/build/esp32doit-devkit-v1
   ::: esp32_decoder.sh firmware.elf
   ::: <input backtrace line>
   


   