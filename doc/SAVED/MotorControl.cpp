/**
 * @file MotorControl.cpp
 * @author Doug Fajardo
 * @brief Apply PID logic to control one motor, uses LN298 driver and Quad encod
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 *  
 * The arduino PID library by Brett Beauregard is used. (library name in platformio.ini is br3ttb/PID@^1.2.1)
 *      Documentation at https://playground.arduino.cc/Code/PIDLibrary/
 * 
 */
#include "MotorControl.h"

// Static items
bool MotorControl::haveInitialized=false;
MotorControlConfig_t cfg[MC_MAX_MOTOR_COUNT];


MotorControl::MotorControl()
{
   myPid=nullptr;
   for (int idx=0; idx<MC_MAX_MOTOR_COUNT; idx++)
   {
    cfg[idx].motorNumber=-1;
   }

}


MotorControl::~MotorControl()
{

}


/**
 * @brief Quad encoder - apply the current pin change
 *   Note - this DOES presuppose that we know (accuratly)
 *  what our previous state was!
 * @param mtr  - pointer to motor config structure
 * @param pinA - true if pinA was the culpret who changed
 */
void MotorControl::applyState(MotorControlConfig_t *mtr, bool pinA)
{
    switch(mtr->last_state)
    {
        case (MotorControlConfig_t::AoffBoff):
        if (pinA)
        {   // A is now on
            mtr->position++;
            mtr->last_state = MotorControlConfig_t::AonBoff;
        } else {  // B is now on
            mtr->position--;
            mtr->last_state = MotorControlConfig_t::AoffBon;
        }
        break;

        case(MotorControlConfig_t::AonBoff):
        if (pinA) { // Pin A is now off
            mtr->position++;
            mtr->last_state = MotorControlConfig_t::AoffBoff;
        } else {  // Pin B is now on
            mtr->position--;
            mtr->last_state = MotorControlConfig_t::AonBon;
        }
        break;

        case(MotorControlConfig_t::AoffBon):
        if (pinA) 
        {   // pin A is now on
            mtr->position--;
            mtr->last_state = MotorControlConfig_t::AonBon;
        } else {  // pin B is now off
            mtr->position++;
            mtr->last_state = MotorControlConfig_t::AoffBoff;
        }
        break;

        case(MotorControlConfig_t::AonBon):
        if (pinA) 
        {   // Pin A is now off
            mtr->position--;
            mtr->last_state = MotorControlConfig_t::AoffBon;
        } else { // Pin B is now off
            mtr->position++;
            mtr->last_state = MotorControlConfig_t::AonBoff;
        }
        break;
    }
    return;
}


/**
 * @brief ISE to read Quad encoder pin changes. 
 * NOTE: This is static, common to all motors.
 *    Changes only come from Phase A or Phase B;
 * When received, this increments (or decrements)
 * a counter as appropriate
 * 
 */
void IRAM_ATTR MotorControl::gpio_isr_handler(void* args)
{
  int thisPin = (int)args;
  int pinState = gpio_get_level((gpio_num_t)thisPin);

  for (int idx = 0; idx < MC_MAX_MOTOR_COUNT; idx++)
  {
      if (thisPin == cfg[idx].QuadPinA)
      {
          applyState(&cfg[idx], true);
          break;
      } else if (thisPin == cfg[idx].QuadPinB)
      {
          applyState(&cfg[idx], false);
          break;
      }
  }

  return;
}

/**
 * @brief Set up the 'run time' stuff...
 * 
 * @param userConfig - the user's configuration structure
 */
int MotorControl::begin(gpio_num_t quad_a, gpio_num_t quad_b, gpio_num_t ena_pin, gpio_num_t dirA_pin, gpio_num_t dirB_pin)
{
    int mtr;

    if ((ena_pin == GPIO_NUM_NC)  ||
        (dirA_pin == GPIO_NUM_NC) ||
        (dirB_pin == GPIO_NUM_NC) ||
        (quad_a == GPIO_NUM_NC)   ||
        (quad_b == GPIO_NUM_NC))
    {
        Serial.println("ERROR IN MotorControl 'begin' - User configuration is invalid");
        return;
    }

// Is there a motor config avail?
    for (int mtr = 0; mtr < MC_MAX_MOTOR_COUNT; mtr++)
    {
        if (!cfg[mtr].motorIsDefined)
        { // use this motor
            cfg[mtr].motorNumber = LN298::addMotor(ena_pin, dirA_pin, dirB_pin);
            cfg[mtr].QuadPinA = quad_a;
            cfg[mtr].QuadPinB = quad_b;
            break;
        }
    }

    if (mtr >= MC_MAX_MOTOR_COUNT)     return (-1); // No motor available

    // Note - using Porpotional on Measurement(P_ON_M) vrs (not Porportional on Error (P_ON_E)
    myPid = new PID(&curPos, &setMotorTo, &setPoint, 2, 5, 1, P_ON_M, DIRECT);
    myPid->SetSampleTime(cfg.sampleTime);
    myPid->SetTunings(cfg.Kp, cfg.Ki, cfg.Kd);
    // Configure the interrupt
    //attachInterrupt (gpio_isr_handler (PIN_A), pinChange, CHANGE);
    return(mtr).;
}


/**
 * @brief Call frequently!
 *   This determins the current speed, and converts
 * it to <units>/sec
 */
void MotorControl::loop()
{
    // Note: there is no periodic loop for LN298 or QuadEncoder
    // get position once every 20 msecs.
    unsigned long elapsed =  (millis() - lastEncoderSpeedCheck); // interval
    uint32_t pos_change;
    if ( elapsed > cfg.encoderCheckInterval)
    {
        // TODO: pos_change = quad.getPosition()-lastEncoderPos;
        
        double rate = pos_change/elapsed; 

    }
    // TODO: curPos = quad.getPosition();
    myPid->Compute();
    ln298.setPWM(setMotorTo);
}

/**
 * @brief Get the Current Config object for this motor
 *    The 'newConfig' structure is updated with the current 
 * configuration values. 
 * 
 * @param newConfig POINTER to memory to store the configuration. 
 *                 this *must* point to enough memory to contain
 *                 the MotorControlConfig_t structure.
 */
void MotorControl::getCurrentConfig(MotorControlConfig_t *newConfig)
{
    memcpy(newConfig, &cfg, sizeof(MotorControlConfig_t));
    return;
}


/**
 * @brief Set the PID parameters
 * 
 * @param Kp 
 * @param Ki 
 * @param Kd 
 */
void MotorControl::setPID(double Kp, double Ki, double Kd)
{
    cfg.Kp = Kp;
    cfg.Ki = Ki;
    cfg.Kd = Kd;
    myPid->SetTunings(cfg.Kp, cfg.Ki, cfg.Kd);
    return;
}

/**
 * @brief Set the sample time for the PID controller
 * 
 * @param stime - sample time in milliseconds
 */
void MotorControl::setSampleTimeInterval(int stime)
{
    cfg.sampleTime = stime;
    myPid->SetSampleTime(cfg.sampleTime);
}

/**
 * @brief set the wheel diameter
 *    NOTE: The distance units used here define the
 *         distance units used for speed
 *        (e.g. if inches, then speed is inches/sec)
 * @param diameter 
 */
void MotorControl::setWheel(int diameter)
{
    cfg.wheelDia = diameter;
    return;
}

/**
 * @brief set the pulses per revolution
 * 
 * @param ppr 
 */
void MotorControl::setPPR(int ppr)
{
    cfg.PulsesPerRev = ppr;
    return;
}



/**
 * @brief Set the Speed
 *    This is the main user control point. The target speed is specified in units/sec
 * where 'units' are the units chosen to define the wheel diameter.  The speed is 
 * signed, a negative number is 'reverse'.
 * 
 *    (NOTE: Currently we do not implement acceleration, though this *could* be added in
 * the future.)
 * 
 * @param targetSpeed - Speed in units/sec, see comments in description.
 * 
 * @param driftFlag  - 'Drift'. If set,  disable the LN298 driver until we reach the 
 *                     target speed. All remains disabled if target speed is 0.
 *                     Default False (do not drift).
 *                     Driver will be automatically re-enabled if this is called 
 *                        with driftFlag=false.
 */
void MotorControl::setSpeed(int targetSpeed, bool driftFlag)
{
    // TODO:
}