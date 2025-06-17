/**
 * @file MotorControl.h
 * @author Doug Fajardo
 * @brief  Use a PID to control the power going to a motor, based on its desired speed.
 * @version 0.1
 * @date 2025-04-28
 * 
 * @copyright Copyright (c) 2025
 * 
 * This 'device' inherits the h298 AND QuadDecoder classes. It adds a PID controler
 * that accepts the desired speed AND the actual speed from the Quad Decoder, and
 * modifies the Power going to the motor to correct the motor speed to match the target.
 * 
 * Distance Units are set to Millimeters.
 * 
 * 'Input' is the speed measured by the QuadDecoder (in mm/sec or in/sec?)
 * The 'output' is thru the ln298 driver. Range 0..100 (TBD: Is this fine enough?)
 *
 *  Loop is used to must be called periodically (should this be from a timer????) 
 * 
 * TODO: Should we vary the K-factors depending on current power level?
 */
#pragma once
#include "DefDevice.h"
#include "PidDevice.h"
#include "ln298.h"
#include "QuadDecoder.h"
#include "driver/ledc.h"

// #define USE_PID



// - - - - - - - - - - - - - - - - - - - - - - - - -
// This class links the LN298 driver, the QUAD position decoder,
//    and the PID motor adjustment/feedback
//    for one motor.
// - - - - - - - - - - - - - - - - - - - - - - - - - -
class MotorControl: public DefDevice
{
    private:
        // These are used by PID
        double input_val;   // the actual speed (mm/sec)
        double output_val;  // PWM percentage to set the motor(0..100)
        double setpoint;    // The target string (mm/sec)

        QuadDecoder *quadDecoder; // The quadrature decoder class instance
        LN298     *ln298;     // The ln298 instance
        PidDevice *piddev;    // The PID controler instance

    public:
        MotorControl( Node *_node, const char * Name);
        ~MotorControl();
        void setup(MotorControl_config_t *cfg, const char *prefix);
        ProcessStatus  DoPeriodic() override;
        ProcessStatus  ExecuteCommand() override;

        // Operations - make it go
        void setSpeed(dist_t ratemm_sec);
        void setDrift();
        void setStop(int stopRate);  // rate is 0..100%
};