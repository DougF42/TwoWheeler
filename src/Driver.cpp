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
#include "stdlib.h"

// What we consider delimiters for commands 
#define COMMAND_WHITE_SPACE " |\r\n"

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// We have a new driver
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
Driver::Driver(Node *myNode, const char *_name) : DefDevice(myNode, _name)
{
    nextMotorIdx=0;   
    mySpeed=0;
    myDirect=0; 
    // SetID(devid);  // TBD: Do I need this?
    Serial.print(" ");
    periodicEnabled=false; // Start with NO periodic reports
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
Driver::~Driver()
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
void Driver::setup(MotorControl_config_t *left_cfg, MotorControl_config_t *right_cfg)
{
    leftMtr  = new MotorControl( myNode, "leftMotor");
    leftMtr->setup(left_cfg, "left_");
    myNode->AddDevice(leftMtr);
    rightMtr = new MotorControl( myNode, "rightMotor");
    rightMtr->setup(right_cfg, "right_");
    myNode->AddDevice(rightMtr);
    periodicEnabled = false; 
}


// 
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// TODO Periodically report the driver status:
//     current positionm current direction, current (average)speed over ground
//  
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
ProcessStatus Driver::DoPeriodic()
{
    // TODO:
    return(SUCCESS_NODATA);
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
ProcessStatus  Driver::ExecuteCommand ()
{
    ProcessStatus status;
    status = Device::ExecuteCommand();
    if (status != NOT_HANDLED) return(status);

    status=FAIL_NODATA;
    scanParam();
    char *cmdPtr = CommandPacket.command;

    if (strncmp(cmdPtr, "MOVE",4) == 0)
    {  // Move  at a given speed and rate of rotation
        status=cmdMOV(argCount, arglist);
        
    }  else if (strncmp(cmdPtr, "STOP",4) == 0)
    { // Stop all motion
        status=cmdSTOP(argCount ,arglist);

    } else if (strncmp(cmdPtr,"SPED",4) == 0)
    {  // Set speed
        status = cmdSPEED(argCount ,arglist);

    } else if (strncmp(cmdPtr,"ROTA",4 ) == 0)
    {  // Set rotation rate
        status = cmdROTATION(argCount ,arglist);
    } else if (strncmp(cmdPtr, "DRFT", 4) == 0)
    {
        status = cmdDrift(argCount, arglist);
    }
    // Serial.print("STATUS:  "); Serial.println(status);
    return(status);
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
void Driver::setMotion(int speed, int rotation)
{
    Serial.printf("** In setMotion: Speed=%d  rotation=%d\n", speed, rotation);
    int tmpSpeed, tmpRotate = 0;   // these are the raw joystick readings, 0 to +/-2048
    dist_t m1, m2 = 0.0;           // These are in mm/sec.

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
        //Serial.println("*** In SetMotion: Setting new motor speeds");
        mySpeed = tmpSpeed;  // TODO: Convert +/-2048 to mm/second
        myDirect = tmpRotate;  // TODO: Convert +/-2048 to mm/second
        leftMtr  -> setSpeed(m1);
        rightMtr -> setSpeed(m2);
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
ProcessStatus Driver::cmdMOV(int argcnt, char *argv[])
{
    ProcessStatus retVal = SUCCESS_NODATA;
    int tmpval = 0;
    char *pSpd = nullptr;
    char *pRot = nullptr;

    Serial.println("See cmdMOV");
    ProcessStatus result = SUCCESS_NODATA;

   if (argcnt==2)
    {   // We have two args - speed and turnrate
        errno = 0;
        tmpval = strtol(argv[0], nullptr, 10);  // speed
        if (errno != 0)
        { // bad value (overflow/underflow)
            result = FAIL_DATA;
            sprintf(DataPacket.value, "speed parameter is not a valid value");
            retVal = FAIL_DATA;
            goto endCmdMOV;
        }
        else
        {
            mySpeed = tmpval;
        }


        errno = 0;
        tmpval = strtol(argv[1], nullptr, 10); // rotation rate
        if (errno != 0)
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "speed parameter is not a valid value");
            retVal = FAIL_DATA;
            goto endCmdMOV;
        }
        else
        {
            myDirect = tmpval;
        }
    }

    // Using actual values, set the motors and report settings
    setMotion(mySpeed, myDirect);

    // Report current speed and rotation rate
    DataPacket.timestamp = millis();
    sprintf(DataPacket.value, "*** In SetMotion: Speed|%d| dir|%d| m1|%f| M2|%f",
             mySpeed, myDirect, leftMtr->GetRate(), rightMtr->GetRate());

endCmdMOV:
    return (result);
}


/**
 * @brief Stop driving the motors
 *  Format: STOP|stoprate
 *     Stoprate is 0 (drift, motors not engaged) to 100 (panic stop)
 *
 * @return ProcessStatus
 */
ProcessStatus Driver::cmdSTOP(int argcnt, char *argv[])
{
    ProcessStatus retVal = SUCCESS_NODATA;
    Serial.println("See cmdSTOP");
    setMotion(0,0);
    return (retVal);
}


/**
 * @brief SMAC command handler - set speed
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdSPEED(int argcnt, char *argv[])
{
    ProcessStatus retVal=SUCCESS_NODATA;   
    errno = 0;
    double tmpSpd;

    if (argcnt == 1)
    {
        if (0 != getDouble(0, &tmpSpd, "Speed value:"))
        {
            retVal = FAIL_DATA;
            goto cmdSPEEDend;
        }
        else
        {
            setMotion(tmpSpd, myDirect);
        }
    }
    else if (argcnt != 0)
    {
        sprintf(DataPacket.value, "too many arguments");
        retVal=FAIL_DATA;
        goto cmdSPEEDend;
    }

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "SPED|%f", mySpeed);
        retVal = SUCCESS_DATA;
    }

cmdSPEEDend:
    defDevSendData(0, false);
    return (retVal);
}

/**
 * @brief SMAC command handler - set rotation rate
 * 
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdROTATION(int argcnt, char *argv[])
{
    ProcessStatus retVal = SUCCESS_DATA;

    char *pRot;
    double tmpRot;
    errno = 0;

    if (argcnt == 1)
    {
        if (0 != getDouble( 0, &tmpRot, "Rotation:"))
        {
            retVal - FAIL_DATA;
            goto cmdROTATIONend;

        } else {
            setMotion(mySpeed, tmpRot);
        }
        if (errno != 0)
        { //to many arguments
            retVal = FAIL_DATA;
            sprintf(DataPacket.value, "Too many arguments");
            goto cmdROTATIONend;
        }
    }

    if (retVal==SUCCESS_NODATA)
    {
        sprintf(DataPacket.value,"ROTA|%d", myDirect);
        retVal=SUCCESS_DATA;
    }

cmdROTATIONend:
    defDevSendData(0,false);
    return(retVal);
}


/**
 * @brief disable PID and LN298 drivers
 *   FORMAT:    DRFT   (no arguments)
 * @param argcnt
 * @param argv 
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdDrift(int argcnt, char *argv[])
{
    ProcessStatus retVal = SUCCESS_NODATA;
    // PID to manunal
    leftMtr ->setDrift();
    rightMtr->setDrift();
    sprintf(DataPacket.value, "DRFT|OK");
    retVal = SUCCESS_DATA;
    return(retVal);
}