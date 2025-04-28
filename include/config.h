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
#include "CommandList.h"
#include "driver/gpio.h"

// Network Definitions
#define UDP_SSID "defnet"
#define UDP_PASS "iknowits42"
#define UDP_PORT 23

// Some command processing definitions
extern void showHelp(Print *outdev, const Command_t *cmd);
extern void cmdHelp(Print *outdev, int tokCnt, char **tokList);

//#define BUILT_IN_LED_PIN ( (gpio_num_t)2 )
#define BUILT_IN_LED_PIN  LED_PIN

#define LCD_PULSE_FREQ  5000
#define LCD_RES_BITS      13
// from ledc_timer_t
#define LCD_TIMER_NO      LEDC_TIMER_0


// Pins for DC motor 1
#define MOTOR_1_EN      GPIO_NUM_20
#define MOTOR_1_DRIVE_A GPIO_NUM_21
#define MOTOR_1_DRIVE_B GPIO_NUM_22

// Pins for DC motor 2
#define MOTOR_2_EN      GPIO_NUM_25
#define MOTOR_2_DRIVE_A GPIO_NUM_26
#define MOTOR_2_DRIVE_B GPIO_NUM_27

// Speed Sensors 
#define QUAD_PULSES_PER_REV   600
#define SPEED_CHECK_INTERVAL_uSec 1000
#define WHEEL_DIA_IN         4
#define MOTOR_1_QUAD_A  GPIO_NUM_20
#define MOTOR_1_QUAD_B  GPIO_NUM_21
#define MOTOR_2_QUAD_A  GPIO_NUM_28
#define MOTOR_2_QUAD_B  GPIO_NUM_29


// * * * * * * * 
// I2C pins... These (currently) are the
//    default I2C pins - here for documentation purposes.
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 23

#endif