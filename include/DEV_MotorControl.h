/**
 * @file DEV_MotorControl.h
 * @author Doug Fajardo
 * @brief  Use a PID to control the power going to a motor, based on its desired speed.
 * @version 0.1
 * @date 2025-04-28
 * 
 * @copyright Copyright (c) 2025
 * 
 * This 'device' inherits the h298 AND DEV_QuadDecoder classes. It adds a PID controler
 * that accepts the desired speed AND the actual speed from the Quad Decoder, and
 * modifies the Power going to the motor to correct the motor speed to match the target.
 * 
 * Distance Units are set to Millimeters.
 * 
 * 'Input' is the speed measured by the DEV_QuadDecoder (in mm/sec or in/sec?)
 * The 'output' is thru the ln298 driver. Range 0..100 (TBD: Is this fine enough?)
 *
 *  Loop is used to must be called periodically (should this be from a timer????) 
 * 
 * TODO: Should we vary the K-factors depending on current power level?
 */
#pragma once
#include "Node.h"
#include "DefDevice.h"
#include "DEV_Pid.h"
#include "DEV_ln298.h"
#include "DEV_QuadDecoder.h"
#include "driver/ledc.h"

// - - - - - - - - - - - - - - - - - - - - - - - - -
// This class links the LN298 driver, the QUAD position decoder,
//    and the PID motor adjustment/feedback
//    for one motor.
// - - - - - - - - - - - - - - - - - - - - - - - - - -
class DEV_MotorControl: public DefDevice
{
    private:
        Node *myNode;
        
    public:
        // These are used by PID
        double input_val;   // the actual speed (mm/sec)
        double output_val;  // PWM percentage to set the motor(0..100)
        double setpoint;    // The target string (mm/sec)

        DEV_QuadDecoder *myQuadDecoder;
        DEV_LN298   *ln298;     // The ln298 instance
        DEV_Pid     *piddev;    // The PID controler instance

    
        DEV_MotorControl(const char * Name, Node *_nodePtr);
        void setup( MotorControl_config_t *cfg, const char *prefix);
        ~DEV_MotorControl();

        ProcessStatus  DoPeriodic() override;
        ProcessStatus  ExecuteCommand() override;
        ProcessStatus cmdSetSpeed(int argCnt, char **argv);

        // Operations - make it go
        void setSpeed(dist_t ratemm_sec);
        void setDrift();
        void setStop(int stopRate);  // rate is 0..100%

};