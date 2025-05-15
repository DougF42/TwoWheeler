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
    loopRate=conf.loop_rate;
    lastLoopTime=millis();
    setupLN298( conf.chnlNo, conf.ena_pin, conf.dir_pin_a, conf.dir_pin_b);

    // setup QuadDecoder
    setupQuad( conf.dir_pin_a, conf.dir_pin_b);
    
    // setup PID controller
    // TBD:   pidctlr = new PID_def( // TBD ??? );
}

void MotorControl::setLoopRate(unsigned long milliseconds)
{
    loopRate= milliseconds;
}

/**
 * @brief loop - call periodically.
 * 
 */
void MotorControl::loop()
{
    unsigned long now = millis();
    unsigned long timeChange = (now - lastLoopTime);
    if(timeChange < loopRate) return;

    // get current speed (from quad)
    input=getSpeed();

    pidctlr->Compute();   // determine change to power setting

    // set power output (from ln298)
    setPulseWidth(output);

    // Remember when we did this
    lastLoopTime=now;
}



void setQUADcalibration(uint pulsesPerRev, uint circumfrence)
{

}


void getQUADcalibration(uint *pulsesPerRev, uint *circumfrence)
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