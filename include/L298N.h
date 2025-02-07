/**
 * @file LN298N.h
 * @author Doug Fajardo (doug.fajardo@gmail.com)
 * @brief  Class to drvie the DC motor on the LN298N
 * @version 0.1
 * @date 2024-05-26
 *
 * @copyright Copyright (c) 2024
 *
 * Drive the L298N motor driver
 * 
 * This supports 2 motors on a LN298N type driver.
 * 
 * The pulse rate and pulse width is determined by the LED controler, which drives the EN pin.
 * Direction and duration of any motion is detemined by the high-speed timer and the event queue.
 * 
 * 
 */
#include "Config.h"

#define NOOFMOTORS 2

#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_err.h"

#ifndef L_2_9_8_n__H
#define L_2_9_8_n__H


class L298N
{
public:
    // What motor are we working on?
    typedef enum
    {
        MOTOR_A = 0,
        MOTOR_B,
        MOTOR_ID_MAX
    } MOTOR_ID_t;

    // What function?
    typedef enum
    {
        MOTOR_IDLE,
        MOTOR_FWD,
        MOTOR_REV,
        MOTOR_ACT_MAX
    } MOTOR_ACTION_t;

    L298N();
    ~L298N();
    void stopAll();        // stop all motors
    void stopMotor(MOTOR_ID_t mtr); // Stop all action on this motor
    bool addToQue(MOTOR_ID_t mtr, MOTOR_ACTION_t act, int pwr, uint64_t duration);
    uint32_t mapPcntToPwr(MOTOR_ID_t mtr, uint32_t pcntPower);
    bool configureMotorPins(MOTOR_ID_t mtr, int ena_pin, int in1_pin, int in2_pin);

protected:
    // *******
    // This subclass defines a single event
    class EVENT
    {
    public:
        EVENT() {};
        ~EVENT() {};        
        MOTOR_ACTION_t motorAction;
        uint8_t  pcntPower; // 0..100
        uint64_t duration;
        uint64_t ragnorak; // A.K.A the end of time for this event <grin>
        bool isRunning;   // Is this event running?
        // DUMP an event.
        inline void dump()
        {
            static const char *EV = "***EVENT:";
            const char *run;
            const char *act;
            switch (motorAction)
            {
            case (MOTOR_IDLE):
                act = "IDLE";
                break;
            case (MOTOR_FWD):
                act = "FWD";
                break;
            case (MOTOR_REV):
                act = "REV";
                break;
            default:
                act = "UNKNOWN ACTION";
                break;
            }
            run = (isRunning) ? "IS" : "NOT";
            ESP_LOGI(EV, " %s running ACT=%s power=%d ", run, act, pcntPower);
        }
    };
    // ******* END OF EVENT SUBCLASS

    // Used to auto-assign led driver channels
    ledc_channel_t nextAvailLCDChannel;

    // Basewd on the resolution, what is the largest PWM setting?
    uint32_t maxPwmValue;

    // For tracking motor configuration
    typedef struct  {
        int ena;      // enable pin
        int in1;      // in1 pin
        int in2;      // in2 pin
        ledc_channel_t chan; // timer Chanel number for this motor

    } MOTOR_PIN_t;
    MOTOR_PIN_t motorPins[NOOFMOTORS];
    DEFQue<EVENT, 3> que[NOOFMOTORS]; // event queues - one per motor. room for 2^3 (8) entries

    // LCD stuff...
    bool setMotor(MOTOR_ID_t mtr, EVENT *event);
    static IRAM_ATTR bool ledc_cb();    // process timeout

    // event TIMER...
    void setUpEventTimer(); // Initialize the event timer

    esp_timer_handle_t oneshot_timer; // The alarm clock...
    static void oneShotCallback(void *arg); // process an event
};
#endif