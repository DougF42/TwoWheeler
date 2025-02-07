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

// built-in LED
#define BUILT_IN_LED_PIN ( (gpio_num_t)2 )
/*
* LCD uses APB clock (80 Mhz)
 * freq/resolution tradeoff (see table 14-1 pg 389 )
 *  FREQ    HIGHEST_RES_BITS  LOWEST_RES_BITS
 *  1Khz       16                7
 *  5khz       13                4
 *  10 khz     12                3
*/
#define LCD_PULSE_FREQ  5000
#define LCD_RES_BITS      13
// from ledc_timer_t
#define LCD_TIMER_NO      LEDC_TIMER_0

// channel number and Pins for motor A
#define MOTOR_A_LED_CHANNEL_NO  LEDC_CHANNEL_0
#define MOTOR_A_ENA_PIN 32
#define MOTOR_A_IN1_PIN 33
#define MOTOR_A_IN2_PIN 25

// Pins for motor B
#define MOTOR_B_LED_CHANNEL_NO LEDC_CHANNEL_1
#define MOTOR_B_ENA_PIN 27
#define MOTOR_B_IN1_PIN 14
#define MOTOR_B_IN2_PIN 13

// * * * * * * * 
// I2C pins... These (currently) are the
//    default I2C pins - here for documentation purposes.
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 23

// * * * * * * * * * * * * *
//  Arduino GPIO simulation stuff...
#ifndef HIGH
#define HIGH 1
#endif

#ifndef LOW 
#define LOW 0
#endif


#endif