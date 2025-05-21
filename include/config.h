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

// For consistency, time is time_t (long long )
//     from _timeval.h (ESP32 idf?)
// DISTANCE:
typedef  long long dist_t;

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

//#define BUILT_IN_LED_PIN ( (gpio_num_t)2 )
#define BUILT_IN_LED_PIN  LED_PIN

#define LCD_PULSE_FREQ  5000
#define LCD_RES_BITS      13
// from ledc_timer_t
#define LCD_TIMER_NO      LEDC_TIMER_0


// Pins for DC motor 1
#define MOTOR_1_EN      GPIO_NUM_14
#define MOTOR_1_DRIVE_A GPIO_NUM_27
#define MOTOR_1_DRIVE_B GPIO_NUM_26

// Pins for DC motor 2
#define MOTOR_2_EN      GPIO_NUM_32
#define MOTOR_2_DRIVE_A GPIO_NUM_25
#define MOTOR_2_DRIVE_B GPIO_NUM_33

// Speed Sensors 
#define QUAD_PULSES_PER_REV   600
#define SPEED_CHECK_INTERVAL_mSec 20000
#define MOTOR_1_QUAD_A  GPIO_NUM_36
#define MOTOR_1_QUAD_B  GPIO_NUM_39
#define MOTOR_2_QUAD_A  GPIO_NUM_35
#define MOTOR_2_QUAD_B  GPIO_NUM_34


// * * * * * * * 
// I2C pins... These (currently) are the
//    default I2C pins - here for documentation purposes.
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 23

#endif