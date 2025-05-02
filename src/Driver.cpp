/**
 * @file Driver.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-05-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Driver.h"
#include "QuadDecoder.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// We have a new driver
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
Driver::Driver() : Device{"Driver"}
{
    nextMotorIdx=0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
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
    if (nextMotorIdx+1 > MAX_MOTOR_COUNT) return(false);
    motors[nextMotorIdx] = new MotorControl();
    motors[nextMotorIdx]->setup(configuration);
    nextMotorIdx++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
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
  //   QUAD  <pulsesPerRev>, <circum>, <units>
  //   PID   <Kp>,<Kd>,<Ki>
  //   RATE  <quadRate> <pidRate>  // in millisecs between operations
  //   FWD    <rate>      // mm per second. May be negative.
  //   ROT <degrees>      // number of degrees to rotate heading
  //   stop (int stopRate); // 0..100 0 means drift, 100 means emergency stop, otherwise percentage
ProcessStatus  Driver::ExecuteCommand ()
{
    ProcessStatus status=Device::ExecuteCommand();
    if (status != NOT_HANDLED) return;
    // we try to handle it...
    if (strcmp(CommandPacket.command, "QUAD") == 0)
    {   // TODO: Set/get Quad paramters        
        status=QuadParameters();

    }  else if ((CommandPacket.command, "PID") == 0)
    {  // TODO: Set PID parameters
        status=PidParameters();

    }  else if ((CommandPacket.command, "FWD") == 0)
    {  // TODO: Move forward
        status=moveFwd();

    } else if ((CommandPacket.command, "ROT") == 0)
    {  // TODO: Rotate by given degrees
        status=rotate();
    }  else if ((CommandPacket.command, "STOP") == 0)
    { // TODO: Stop all motion
        status=stop();
    } 
    return(pStatus);
}

// - - - - - - - - - - - - - - - - - -
// set or get the quad parameters.
// quad|<pulsesPerRev>|<circum>
// If no parameters, we return the pulses per rev and circum(in mm).
// NOTE: Both motors will be set to the same values.
ProcessStatus Driver::QuadParameters()
{
    uint pulsesPerRev;
    uint circumfrence; // in mm
    QuadDecoder::QuadUnits_t _units=QuadDecoder::UNITS_MM;
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
            result = FAIL_DATA;
        }

        // Decode circumfrence
        ptr = strtok(nullptr, COMMAND_WHITE_SPACE);
        if (ptr == nullptr)
        {
            // TODO: ERROR - missing Curcumfrence
            result = FAIL_DATA;
        }
        else
        {
            errno = 0;
            circumfrence = strtoul(ptr, nullptr, 10);
            if (errno != 0)
            {
                // TODO: ERROR invalid argument
                result = FAIL_DATA;
            };
        }

        if (result == SUCCESS_DATA)
        {
            // SET the new values
            motors[0]->setQUADcalibration(pulsesPerRev, circumfrence, _units);
            motors[1]->setQUADcalibration(pulsesPerRev, circumfrence, _units);
        }
    }


    // we always return the current values
    sprintf(DataPacket.value,"pulse_per_rev=%ud Circum=%ud units=%2s", 
        pulsesPerRev, circumfrence,(_units==QuadDecoder::UNITS_MM)?"MM":"IN");

    return(SUCCESS_DATA);
}

ProcessStatus Driver::PidParameters()
{

}

ProcessStatus Driver::rate()
{

}

ProcessStatus Driver::moveFwd()
{

}

ProcessStatus Driver::rotate()
{

}

ProcessStatus Driver::stop()
{

}