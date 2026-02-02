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
#include "SMAC/Node.h"
#include "config.h"
#include "DEV_Driver.h"
#include <stdlib.h>
#include "Util.h"

// What we consider delimiters for commands 
#define COMMAND_WHITE_SPACE " |\r\n"

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// We have a new driver
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
DEV_Driver::DEV_Driver( const char *_name) : Device(_name)
{
    nextMotorIdx=0;   
    mySpeed=0;
    myDirect=0; 
    // SetID(devid);  // TBD: Do I need this?
    Serial.print(" ");
    periodicEnabled=false; // Start with NO periodic reports
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
DEV_Driver::~DEV_Driver()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
/**
 * @brief Set up the left and right motors.
 *      (The motors, in turn, will set up the ln298, QuadDecoder and PID devices)
 * 
 * @param left_cfg 
 * @param right_cfg 
 */
void DEV_Driver::setup(DEV_Pid *_left, DEV_Pid *_right)
{
    leftPid  = _left;
    rightPid = _right;
}


// 
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// TODO Periodically report the driver status:
//     current positionm current direction, current (average)speed over ground
//  
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
ProcessStatus DEV_Driver::DoPeriodic()
{
    // TODO:
    return(NODATA);
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
//
//
ProcessStatus  DEV_Driver::ExecuteCommand (char *command, char *params)
{
    ProcessStatus status;
    status = Device::ExecuteCommand(command, params);
    if (status != NOT_HANDLED) return(status);

    status=NODATA;
    char *cmdPtr = command;

    if (strcasecmp(cmdPtr, "MOVE") == 0)
    {  // Move  at a given speed AND rate of rotation
        status=cmdMOV(command, params);
        
    }  else if (strcasecmp(cmdPtr, "STOP") == 0)
    { // Stop all motion
        status=cmdSTOP(command, params);

    } else if (strcasecmp(cmdPtr,"SPED") == 0)
    {  // Set speed
        status = cmdSPEED(command, params);

    } else if (strcasecmp(cmdPtr,"ROTA" ) == 0)
    {  // Set rotation rate
        status = cmdROTATION(command, params);

    }  else if ( strcasecmp(cmdPtr, "TANK") )
    {   // set speed of each tread independently
        status = cmdTANK(command, params);
    } else       
    {
        status = NOT_HANDLED;
    } 
    // Serial.print("STATUS:  "); Serial.println(status);
    return(status);
}

/**
 * set the speed of both wheels in a tank-like fashion.
 *
 * Format:   TANK|<left-speed>|<right-speed>
 */
ProcessStatus DEV_Driver::cmdTANK(char *command, char *param)
{
    int8_t leftSpd, rightSpd = 0;
    ProcessStatus retVal = NODATA;

    char *firstArg = strtok(param, ",\r\n");
    char *secondArg = nullptr;

    if (firstArg == nullptr)
    {
        sprintf(SMACData.values, "EROR - Missing arguments");
        retVal = SYSTEM_DATA;
    }
    else
    {
        leftSpd = Util::getint8(firstArg, &leftSpd, "Left Speed ");
        if (leftSpd == WIDGET_DATA)
        {
            secondArg = strtok(nullptr, ",\r\n");
            retVal = Util::getint8(secondArg, &rightSpd, "Right Speed ");
            if (retVal == WIDGET_DATA)
            { // SET THE LEFT AND RIGHT SPEED
                leftPid->setSpeed(leftSpd);
                rightPid->setSpeed(rightSpd);
            }
            return (retVal);
        }
    }
    return(retVal);
}

/**
 * @brief Internal - Set the speed for the two motors.
 *   speed is +/- 2048,  rotation is +/- 2048.
 * This handles all normalization and limits.
 * (Note: If the speed and rotation haven't changed, then
 *  the motor speeds are not changed. )
 *
 * It also puts the current speed/rotation response in the Datapacket.
 * @param speed     - the desired speed (0 +/-2048).
 * @param rotation  - the desired rotation (0 +/- 2048)
 */
void DEV_Driver::setMotion(int speed, int rotation)
{
    Serial.printf("** In setMotion: Speed=%d  rotation=%d\n", speed, rotation);
    int tmpSpeed, tmpRotate = 0; // these are the raw joystick readings, 0 to +/-2048
    dist_t m1, m2 = 0.0;         // These are in mm/sec.

    tmpSpeed = constrain(speed, -2048, 2048);
    tmpRotate = constrain(rotation, -2048, 2048);

    // calculate motor 1 speed. limit to +/-2048
    m1 = mySpeed + myDirect;
    m1 = constrain(m1, -2048, 2048);

    // calculate motor 2 speed. Limit to +/- 2048
    m2 = mySpeed - myDirect;
    m2 = constrain(m2, -2048, 2048);

    if ((tmpSpeed != mySpeed) || (tmpRotate != myDirect))
    {
        // Serial.println("*** In SetMotion: Setting new motor speeds");
        mySpeed = tmpSpeed;   // TODO: Convert +/-2048 to mm/second
        myDirect = tmpRotate; // TODO: Convert +/-2048 to mm/second
        leftPid->setSpeed(m1);
        rightPid->setSpeed(m2);
    }
}

/**
 * @brief Set the forward motion to a given speed
 *  Format:  "FWD|speed|turnRate"
 *      The speed is 0 +/-2048,  dir is 0 +/-2048
 *  If no arguments, then just report the current motion.
 *  If no turnRate, assume straight ahead
 * @return ProcessStatus
 */
ProcessStatus DEV_Driver::cmdMOV(char *command, char *param)
{
    ProcessStatus retVal = NODATA;

    char *pSpd = strtok(param, ",\r\n");
    char *pRot = nullptr;
    int tmpSpd = 0;
    int tmpRot = 0;
    Serial.println("See cmdMOV");

    if (pSpd == nullptr)
    {
        sprintf(SMACData.values, "EROR - missing arguments");
        retVal = SYSTEM_DATA;
    }
    else
    {
        pRot = strtok(nullptr, ",\r\n");
        if (pRot = nullptr)
        {
            tmpRot = 0;
        }
        else
        {
            retVal = Util::getint_t(param, &tmpRot, "Speed ");
        }
        setMotion(tmpSpd, tmpRot);
    }
    return (retVal);
}


/**
 * @brief Stop driving the motors
 *  Format: STOP
 *
 * @return ProcessStatus
 */
ProcessStatus DEV_Driver::cmdSTOP(char *command, char *param)
{
    setMotion(0, 0);
    return (NODATA);
}


/**
 * @brief SMAC command handler - set speed
 * @return ProcessStatus
 */
ProcessStatus DEV_Driver::cmdSPEED(char *command, char *param)
{
    ProcessStatus retVal = NODATA;
    errno = 0;
    int tmpSpd;

    retVal = Util::getint_t(param, &tmpSpd, "Speed ");
    if (retVal == WIDGET_DATA)
    {
        setMotion(tmpSpd, myDirect);
    }
 
    return (retVal);
}


/**
 * @brief SMAC command handler - set rotation rate
 *
 * @return ProcessStatus
 */
ProcessStatus DEV_Driver::cmdROTATION(char *command, char *param)
{
    ProcessStatus retVal = NODATA;
   
    int tmpRot;
    errno = 0;
    retVal = Util::getint_t(param, &tmpRot, "Rotation rate");

    if (retVal == WIDGET_DATA)
    {
        setMotion(mySpeed, tmpRot);
    }
    return(retVal);
  }
