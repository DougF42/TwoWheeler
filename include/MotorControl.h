/**
 * @file MotorControl.h
 * @author Doug Fajardo
 * @brief  Use a PID to control the power going to a motor, based on its desired speed.
 * @version 0.1
 * @date 2025-04-28
 * 
 * @copyright Copyright (c) 2025
 * 
 * Distance Units are set to Millimeters.
 * Input is the speed measured by the QuadDecoder (in mm/sec or in/sec?)
 * The output is thru the ln298 driver. Range 0..100 (TBD: Is this fine enough?)
 *
 *  Loop must be called periodically (should this be from a timer????) 
 * 
 * TODO: Should we vary the K-factors depending on current power level?
 */
#pragma once
#include "PID_def.h"
#include "ln298.h"
#include "QuadDecoder.h"
#include "driver/ledc.h"


// - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief This class is used to configure the setup of the motor.
 * 
 */
typedef struct {
    ledc_channel_t  chnlNo;  // e.g. LEDC_CHANNEL_0
    gpio_num_t ena_pin;      // e.g. pin_num_NC
    gpio_num_t dir_pin_a;    // e.g. pin_num_NC
    gpio_num_t dir_pin_b;    // e.g. pin_num_NC
    gpio_num_t quad_pin_a;   // e.g. pin_num_NC
    gpio_num_t quad_pin_b;   // e.g. pin_num_NC
    unsigned long loop_rate; // millisecoonds between speed updates
    float kp;
    float ki;
    float kd;
} MotorControl_config_t;

// - - - - - - - - - - - - - - - - - - - - - - - - -
class MotorControl: public LN298, public QuadDecoder
{
    private:
        // These are used by PID
        float input_val;
        float output_val;
        float setpoint;
        //
        unsigned long loopRate; // how long between checks (msec)?
        unsigned long lastLoopTime;
        PID_def   *pidctlr;     // We dont inherit because PID requires too many initialization parameters

    public:
        MotorControl();
        ~MotorControl();
        void setup(const MotorControl_config_t &configuration);
        void setLoopRate(unsigned long millsecs); // How long between each Speed check, PID update and power adjustment
        void loop();
        
        // Main configuration...
        void setQUADcalibration(uint pulsesPerRev, dist_t diameter); 
        void getQUADcalibration(uint *pulsesPerRev, dist_t *diameter);

        void setPIDTuning(float kp, float ki, float kd);
        void getPIDTuning(float *kp, float *ki, float *kd);

        // Operations
        void setSpeed(float ratemm_sec);
        void setDrift();
        void setStop(int stopRate);  // rate is 0..100%
};