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
    input=0;
    output=0;
    setpoint=0;

    setupLN298( conf.chnlNo, conf.ena_pin, conf.dir_pin_a, conf.dir_pin_b);

    // setup QuadDecoder
    setupQuad( conf.dir_pin_a, conf.dir_pin_b);
    
    // setup PID controller
    // TBD:   pidctlr = new PID_def( // TBD ??? );
}


/**
 * @brief loop - call periodically.
 * 
 */
void MotorControl::loop()
{
    // get current speed
    // TODO: quad();  // get my current speed
    pidctlr->Compute();   // determine change to power setting
    // TODO: ln298// call ln298 to set power output
}



void setQUADcalibration(uint pulsesPerRev, uint circumfrence, QuadDecoder::QuadUnits_t _units)
{

}


void getQUADcalibration(uint *pulsesPerRev, uint *circumfrence, QuadDecoder::QuadUnits_t *_units)
{

}


void setPIDcalibrate(float kp, float kd, float ki)
{

}


void getPIDCalibration(float *kp, float *kd, float *ki)
{

}


void setSpeed(float ratemm_Sec)
{

}


void setDrift()
{

}


void setStop(int stopRate)
{

}