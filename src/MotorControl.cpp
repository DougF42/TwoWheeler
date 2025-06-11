/**
 * @file MotorControl.cpp
 * @author Doug Fajardo
 * @brief
 * @version 0.1
 * @date 2025-04-28
 *
 * @copyright Copyright (c) 2025
 *
 * NOTE: We use the 'High Resolution' timer to drive the PID loop
 */

#include "MotorControl.h"


MotorControl::MotorControl()
{
    loopRate=0;
    pidctlr = nullptr;
 }


MotorControl::~MotorControl()
{

}


/**
 * @brief Define the basic motor parameters
 * 
 * @param mtr  - pointer to the motor definition
 * @param kp 
 * @param ki 
 * @param kd 
 */
void MotorControl::setup(const MotorControl_config_t &conf)
{
    input_val=0;
    output_val=0;
    setpoint=0;
    loopRate=conf.loop_rate;
    lastLoopTime=millis();
    setupLN298( conf.chnlNo, conf.ena_pin, conf.dir_pin_a, conf.dir_pin_b);

    // setup QuadDecoder
    setupQuad( conf.dir_pin_a, conf.dir_pin_b);
    
    // setup PID controller
    pidctlr = new PID(&input_val, &output_val,  &setpoint,
        conf.kp, conf.ki, conf.kd, P_ON_M, false);
}

void MotorControl::setLoopRate(time_t milliseconds)
{
    // Internally, time is in uSecs
    loopRate= milliseconds*1000;
}

template <typename A>
A defmap(A x, A in_min, A in_max, A out_min, A out_max)
{
    A res = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return(res);
}


/**
 * @brief loop - call periodically.
 * 
 */
void MotorControl::loop()
{
    static double last_output_val = 0;
    time_t now = esp_timer_get_time();
    time_t timeChange = (now - lastLoopTime);
    if (timeChange < loopRate)
        return;

    // get current speed (from quad)
    input_val = getSpeed();

#ifdef USE_PID
    pidctlr->Compute(); // determine change to power setting
#else
    // map input directly to ouput
    output_val = defmap<double>(input_val, -2048, 2048, -100, 100);
#endif

    if ( ISNOTEQUAL(last_output_val, output_val) )
    { // IF there was a noticable change, then update the ln298 pulse rate
        setPulseWidth(output_val);
    }

    last_output_val = output_val;
    lastLoopTime = now;
}


/**
 * @brief Set the Quadrature decoding parameters for this motor
 * 
 * @param pulsesPerRev  - how many 'slots' in the quadrature wheel?
 * @param diameter  - Waht is the diameter of the wheel, in mm 
 */
void MotorControl::setQUADcalibration(pulse_t pulsesPerRev, dist_t didiameteram)
{
    calibrate (pulsesPerRev,didiameteram);  

}

/**
 * @brief get the current quadrature config info
 * 
 * @param pulsesPerRev   - how many 'slots' in the quadrature wheel?
 * @param diameter  - Waht is the diameter of the wheel, in mm 
 */
void MotorControl::getQUADcalibration(pulse_t *pulsesPerRev, dist_t *diameter)
{
    getCalibration(pulsesPerRev, diameter);
    return;
}


void MotorControl::setPIDTuning(double kp, double ki, double kd)
{
    pidctlr-> SetTunings(kp, ki, kd);
}


/**
 * @brief Get the P I and D tuning paramters
 * 
 * @param kp 
 * @param kd 
 * @param ki 
 */
void MotorControl::getPIDTuning(double *kp, double *ki, double *kd)
{
    *kp = pidctlr->GetKp();
    *ki = pidctlr->GetKi();
    *kd = pidctlr->GetKd();
    return;
}


/**
 * @brief Set the Speed.
 * 
 * @param ratemm_Sec - speed, mm per millisecond???
 */
void MotorControl::setSpeed(double rate_mm_mmsec)
{
    setpoint = rate_mm_mmsec;
}


/**
 * @brief Set the robot to drift.
 * 
 */
void MotorControl::setDrift()
{

}


/**
 * @brief Stop the robot (Work in progress)
 *    The motor has  motors engaged, but stopped.
 * (Later, we will ramp the speed down at the 'stopRate')
 *  
 * @param stopRate  - TBD:
 */
void MotorControl::setStop(int stopRate)
{
    setSpeed(0);  // for now, just stop...
}