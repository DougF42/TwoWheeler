/**
 * @file Config.h
 * @author Doug Fajardo (doug.fajardo@gmail.com)
 * @brief  general configuration info
 * @version 0.1
 * @date 2024-05-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef C_O_N_F_I_G__H
#define C_O_N_F_I_G__H
#include "Arduino.h"
#include "driver/gpio.h"

// When comparing floating point numbers, 
// How close is close enough to match?
#define FUZZ .001
#define ISEQUAL(A,B) (fabs(A-B) < FUZZ)
#define ISNOTEQUAL(A,B) (fabs(A-B) >= FUZZ)


// For consistency, time is time_t (long long )
//     from _timeval.h (ESP32 idf?)
// DISTANCE:
typedef int32_t pulse_t;
typedef double  dist_t;

// Robot Dimensions (in mm)
#define WHEEL_BASE_MM   (17.0*25.4)
#define WHEEL_DIAM_MM  (25.4*6.0)

// Network Definitions
#define UDP_SSID "defnet"
#define UDP_PASS "iknowits42"
#define UDP_PORT 23

// SMAC defintitions
// Set true if we want debugging messages from SMAC
#define SMAC_DEBUGING false 

#define MAC_SIZE  6
#define SMAC_NODENAME "TWOWHEEL"
#define SMAC_NODENO   0

#define LCD_PULSE_FREQ  5000
#define LCD_RES_BITS      13
// from ledc_timer_t
#define LCD_TIMER_NO      LEDC_TIMER_0


// Pins for DC motor 1 (right)
#define MOTOR_1_EN      GPIO_NUM_9
#define MOTOR_1_DRIVE_A GPIO_NUM_18
#define MOTOR_1_DRIVE_B GPIO_NUM_8

// Pins for DC motor 2 (left)
#define MOTOR_2_EN      GPIO_NUM_15
#define MOTOR_2_DRIVE_A GPIO_NUM_16
#define MOTOR_2_DRIVE_B GPIO_NUM_17

// Motor 1 Encoder
#define MOTOR_1_QUAD_A  GPIO_NUM_4
#define MOTOR_1_QUAD_B  GPIO_NUM_5
// Motor 2 Encoder
#define MOTOR_2_QUAD_A  GPIO_NUM_6
#define MOTOR_2_QUAD_B  GPIO_NUM_7

// Default scaling for quad
#define QUAD_PULSES_PER_REV   600
#define SPEED_CHECK_INTERVAL_mSec 5000


// * * * * * * * 
// I2C pins... These (currently) are the
//    default I2C pins - here for documentation purposes.
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 23

// - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief This class is used to configure a motor.
 * 
 */
typedef struct {
    esp_timer_handle_t spdUpdateTimer; // The timer handle. This timer defined in main   
    ledc_channel_t  chnlNo;  // e.g. LEDC_CHANNEL_0 - used by ln298
    gpio_num_t ena_pin;      // e.g. pin_num_NC
    gpio_num_t dir_pin_a;    // e.g. pin_num_NC
    gpio_num_t dir_pin_b;    // e.g. pin_num_NC
    gpio_num_t quad_pin_a;   // e.g. pin_num_NC
    gpio_num_t quad_pin_b;   // e.g. pin_num_NC
    unsigned long loop_rate; // millisecoonds between speed updates
    double kp;
    double ki;
    double kd;
} MotorControl_config_t;


#endif