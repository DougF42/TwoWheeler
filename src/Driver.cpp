/**
 * @file Driver.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-05-01
 * 
 * @copyright Copyright (c) 2025
 * 
 * This sets up - and controls - two wheels. Feedback
 * thru the PID class is used to govern the actual 
 * power applied to each wheel.
 */
#include "config.h"
#include "Driver.h"
#include "QuadDecoder.h"

// What we consider delimiters for commands 
#define COMMAND_WHITE_SPACE " |\r\n"

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// We have a new driver
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
Driver::Driver() : Device{"Driver"}
{
    nextMotorIdx=0;    
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
Driver::~Driver()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Add a motor to our list. 
// Return:  true normally, false if too many motors
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool Driver::addNewMotor(const MotorControl_config_t &configuration)
{
    if (nextMotorIdx+1 >= MAX_MOTOR_COUNT) return(false);
    motors[nextMotorIdx] = new MotorControl();
    motors[nextMotorIdx]->setup(configuration);
    nextMotorIdx++;
    return(true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Main loop - This calls both low-level motor 
//    controllers (which uses the Position sensor and
//    a PID loop to govern the motor speed). It is 
//    separate from the 'doPeriodic' routine which 
//    processes commands.
//  
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
void Driver::loop()
{
    for (int idx=0; idx<nextMotorIdx; idx++)
    {
        motors[idx]->loop();
    }
}


  // If your child Device class needs to handle custom commands, then override this method:
  //
  // The global <CommandPacket> will have the command definition.
  // First call this base class method to handle the built-in Device commands:
  //
  //   Device::ExecuteCommand ();
  //
  // If this call returns NOT_HANDLED, then your child class should handle the command.
  //
  // If your ExecuteCommand() method has data to return, it should populate the
  // global <DataPacket> and return an appropriate ProcessStatus.
  //
  // When populating the global <DataPacket>, value strings that start with a dash or a digit
  // will be interpreted by the Interface as periodic process data, say from a sensor reading.
  //
  // Commands recognized by the driver:
  //   QUAD  <pulsesPerRev>, <circum>, <units>   // configure the Quadrature encodere.
  //   PID   <Kp>,<Kd>,<Ki>                      // configure the PID controler.
  //   SPD   <rate>      // +/- 2048  heading change in mm per Millisecond. May be negative.
  //   ROT <degrees>      // +/- 2048 degrees per Millisecond. Negative is right, positive is left
  //   stop (int stopRate); // 0..100 0 means drift, 100 means emergency stop, otherwise percentage
ProcessStatus  Driver::ExecuteCommand ()
{
    ProcessStatus status=Device::ExecuteCommand();
    if (status != NOT_HANDLED) return(status);
    // we try to handle it...
    if (strcmp(CommandPacket.command, "QUAD") == 0)
    {   // TODO: Set/get Quad paramters        
        status=cmdQUAD();

    }  else if ((CommandPacket.command, "PID") == 0)
    {  // TODO: Set PID parameters
        status=cmdPID();

    }  else if ((CommandPacket.command, "FWD") == 0)
    {  // Move forward at a given speed
        status=cmdFWD();

    } else if ((CommandPacket.command, "ROT") == 0)
    {  // TODO: Rotate by given degrees
        status=cmdSROT();
        
    }  else if ((CommandPacket.command, "STOP") == 0)
    { // TODO: Stop all motion
        status=cmdSTOP();
    } 
    return(pStatus);
}


// - - - - - - - - - - - - - - - - - -
// set or get the quad parameters.
// quad            - with no arguments, acts as 'get'
// quad|<pulsesPerRev>|<circum>
//    pulsesPerRev -  self explanatory
//    circum       - circumfrence in mm.
// We return the new (current) pulses per rev and circum(in mm).
// NOTE: Both motors will be set to the same values.
ProcessStatus Driver::cmdQUAD()
{
    uint pulsesPerRev;
    uint circumfrence; // in mm
    char *ptr;
    char *endptr;

    ProcessStatus result=SUCCESS_DATA;

    // Do we have any arguments?
    ptr = strtok(CommandPacket.params, COMMAND_WHITE_SPACE);
    if (ptr != nullptr)
    { // we have arguments - this is a 'Set' command
        errno = 0;
        pulsesPerRev = strtoul(ptr, nullptr, 10);
        if ((errno != 0) || (pulsesPerRev > 5000))
        {
            // TODO: ERROR - invalid ppr
            sprintf(DataPacket.value, "Invalid pulses-per-revolution value");
            result = FAIL_DATA;
        }

        // Decode circumfrence
        ptr = strtok(nullptr, COMMAND_WHITE_SPACE);
        if (ptr == nullptr)
        {
            // TODO: ERROR - missing Curcumfrence
            sprintf(DataPacket.value,"Missing or invalid circumfrence value");
            result = FAIL_DATA;
        }

        if (result == SUCCESS_DATA)
        {
            // SET the new values
            motors[0]->setQUADcalibration(pulsesPerRev, circumfrence);
            motors[1]->setQUADcalibration(pulsesPerRev, circumfrence);
        }
    }

    // we always return the current values (unless there was an error)
    if (result==SUCCESS_DATA)
    {
        sprintf(DataPacket.value,"pulse_per_rev=%ud Circum=%ud ",pulsesPerRev, circumfrence);
    }
    return(result);
}


/**
 * @brief Set the paramters for the PID loop
 * Format: "PID|step_time|kp|ki|kd"
 *     Time is in steps per second
 *     kp, ki, kd are the PID parameters. 
 * 
 * @return ProcessStatus 
 * 
 * The command is processed in-place with strtok
 * to parse tokens.
 */
ProcessStatus Driver::cmdPID()
{
    ProcessStatus result = SUCCESS_DATA;

    unsigned long tmpTime;
    float tmpkp, tmpki, tmpkd;
    char *ptr;
    errno = 0;

    ptr = strtok(CommandPacket.params, COMMAND_WHITE_SPACE); // 1st pointer
    if (ptr != nullptr)
    { // yes, we have arguments... process the settng}
        
        errno = 0;
        // STEP TIME
        tmpTime = strtol( ptr, nullptr, 10);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Kp parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        }

        // Kp
        ptr =  strtok(nullptr, COMMAND_WHITE_SPACE);
        tmpkp = strtof(ptr, nullptr);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Kp parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        };

        // Ki
        ptr = strtok(CommandPacket.params, COMMAND_WHITE_SPACE); // 1st pointer
        errno=0;
        tmpkd = strtof(ptr, nullptr);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Kd parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        };

        // Kd
        ptr = strtok(CommandPacket.params, COMMAND_WHITE_SPACE); // 1st pointer
        errno=0;
        tmpki = strtof(ptr, nullptr);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Ki parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        };
    }
    // SET THE PID PARAMETERS    
    motors[0]->setPIDTuning( tmpkp, tmpki, tmpkd); 
    motors[1]->setPIDTuning( tmpkp, tmpki, tmpkd);
    sprintf(DataPacket.value, "Kp=%f  kd=%f  ki=%f", tmpkp, tmpki, tmpkd);
    // SUPLLY CURRENT VALUE AS RESPONSE

 endOfSetPidParameters:
    return(result);
}


/**
 * @brief Set the rate (and direction) of rotation
 * Format "SROT|rate"
 *    rate is in radians per second
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdSROT()
{
    // TODO
    return(NOT_HANDLED);
}


/**
 * @brief Set the forward motion to a given speed
 *  Format:  "FWD|speed"
 *      The speed is in mm/sec.
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdFWD()
{

    // TODO:
    return(NOT_HANDLED);
}


/**
 * @brief Stop driving the motors
 *  Format: STOP|stoprate
 *     Stoprate is 0 (drift, motors not engaged) to 100 (panic stop)
 * 
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdSTOP()
{
    // TODO:
    return(NOT_HANDLED);
}