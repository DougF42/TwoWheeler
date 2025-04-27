/**
 * @file MotorControl.cpp
 * @author Doug Fajardo
 * @brief Apply PID logic to control DC motors. Uses LN298 class and implements Quad position decoder.
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 * 
 * once per wheel - This reads the Quad pulses to determine actual motor speed, and uses
 *            a PID library to adjust the motor speed to match the desired value.
 *            The 16-bit 
 *  
 *             The actual motor control is done via an instance of the 'LN298' class.
 * USAGE: 
 *        * Include this header file.
 *        * Instantiate the MotorControl class.
 *        * Define a MotorControlConfig_t structure and poulate it.
 *           NOTE: The units used to define the wheel diameter will be used when setting
 *                 speed (e.g.: a 6 in wheel, then speed is inches/sec)
 *        * Call 'begin' with the MotorControlConfig_t structure as its only argument.
 *        * Call setSpeed to set the motor to a desired speed.
 */
#pragma once
#include "Arduino.h"
#include "LN298.h"
#include "PID_v1.h"
#include "atomic"

#define MC_MAX_MOTOR_COUNT 2

struct MotorControlConfig
{
    int motorNumber; // the motor index assigned by LN298 -1 if not assigned

    gpio_num_t QuadPinA;
    gpio_num_t QuadPinB;
    // QUAD encoding stuff
    typedef enum { AoffBoff, AonBoff, AoffBon, AonBon } QUAD_STATE_t;
    QUAD_STATE_t last_state;
    std::atomic_long position;

     // PID values
    double Kp;
    double Ki;
    double Kd;

    // Wheel and quad parameters
    int wheelDia;   // The wheel diameter
    int PulsesPerRev; // encoder - number of 'slots' per revolution
    int encoderCheckInterval; // TBD
};
typedef struct MotorControlConfig MotorControlConfig_t;

// - - - - - - - - - - - - -- 
class MotorControl_old
{
    private:
    static bool haveInitialized;
    static MotorControlConfig_t cfg[MC_MAX_MOTOR_COUNT];

    unsigned long lastEncoderSpeedCheck; // when we last checked the position
    uint32_t lastEncoderPos;

    double curPos;     // PID routine input
    double setMotorTo; // PID output - set the motor to this
    double setPoint;   // Our target
    PID *myPid;
    static void IRAM_ATTR gpio_isr_handler(void* arg);
    static void applyState(MotorControlConfig *mtr, bool pinA);

    public:
        MotorControl_old();
        ~MotorControl_old();

        int begin(gpio_num_t quad_a, gpio_num_t quad_b, gpio_num_t ena, gpio_num_t dirB_pin, gpio_num_t dirB_pin);
        void loop();

        void setSampleTimeInterval(int stime);
        void setSpeed(int targetSpeed, bool driftFlag=false);
};
