/**
 * @file LN298.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-02-11
 * 
 * @copyright Copyright (c) 2025
 * 
 * NOTE: ecause of how the ESP32_PWM library was written, and
 *      the need to share the timer and interrupt with multiple
 *      motors, some variables, and the ESP32_PWM instance are 
 *      static (and therefore local), but not part of the LN298 class.
 */

#include "Config.h"
#include "LN298.h"

#if !defined( ESP32 )
  #error This code is designed to run on ESP32 platform, not Arduino nor ESP8266! Please check your Tools->Board setting.
#endif
// These define's must be placed at the beginning before #include "ESP32_PWM.h"
// _PWM_LOGLEVEL_ from 0 to 4
// Don't define _PWM_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define _PWM_LOGLEVEL_                3
#define USING_MICROS_RESOLUTION       true    //false

#include "ESP32_PWM.h"

#define HW_TIMER_INTERVAL_US      20L
#define USING_PWM_FREQUENCY     false //true

// These are static, but NOT members of the class...
volatile uint32_t startMicros;
static ESP32Timer ITimer(1);   // Init timer 1: On timer to rule them all...
static ESP32_PWM   pwm;    // was ISR_PWM...

/** - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * @brief 
 * 
 * @param timerNo 
 * @return true 
 * @return false 
 */
bool IRAM_ATTR TimerHandler(void * timerNo)
{
    pwm.run();
    return(true);
}

/** - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * @brief The timer interrupt - we need to do something!
 *
 * @param timerNo - the timer number (we don't care?)
 * @return true   - normal return
 * @return false  - failed.
 */
bool LN298::setup(int ENA_pin, int pinA, int pinB)
{
    motorEnPin = ENA_pin; // this is what we will pulse
    motorDirAPin = pinA;  // one of the bridge pins ln the LN298
    motorDirBPin = pinB;  // one of the bridge pins ln the LN298

// INITIALIZE PINA/PINB for output, both off (stoped)
    pinMode(pinA, OUTPUT);
    pinMode(pinB, OUTPUT);
    pinMode(ENA_pin, OUTPUT); 
    stop();       // ensure the motor is stopped!

// Set up the timer
    if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_US, TimerHandler))
    {
        startMicros = micros();
        return(true);
    }
    else
    {
        Serial.println("Error: cant set frequency?");
        return (false);
    }
    #define PWM_SERVO_FREQ 1000L
    channelNo=pwm.setPWM(motorEnPin, PWM_SERVO_FREQ, 50); // 50 % duty cycle

}

/**
 * @brief Set the LN298 bridge  'forward'
 *
 */
void LN298::forward()
{
    digitalWrite(motorDirAPin, HIGH);
    digitalWrite(motorDirBPin, LOW);
    if (! pwm.isEnabled(channelNo)) pwm.enable(channelNo);
}

/**
 * @brief Put the LN298 bridge in 'reverse'
 * 
 */
void LN298::reverse()
{
    digitalWrite(motorDirAPin, LOW);
    digitalWrite(motorDirBPin, HIGH);
    if (! pwm.isEnabled(channelNo)) pwm.enable(channelNo);
}

/**
 * @brief Disables the drviver, robot will drift
 * 
 */
void LN298::drift()
{
    digitalWrite(motorDirAPin, LOW);
    digitalWrite(motorDirBPin, LOW);
    pwm.disable(channelNo);
    // TODO: STOP THE TIMER ON THIS MOTOR ? ? ?
}

/**
 * @brief Bring the motor to a 'stop'.
 * 
 * @return true 
 * @return false 
 */
void LN298::stop()
{
    digitalWrite(motorDirAPin, LOW);
    digitalWrite(motorDirBPin, LOW);    
    // TODO: SET PULSE RATE HIGH TO STOP MOTOR???
}


