/**
 * @file main.cpp
 * @author Doug Fajardo
 * @brief   MAIN program
 * @version 0.1
 * @date 2025-04-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */
// #include <Arduino.h>
#include "config.h"

#include "UdpCmd.h"
#include "MotorControl.h"
#include "driver/ledc.h"

MotorControl motorLeft;
MotorControl motorRight;

UdpCmd Udp;

void setup() {
  Serial.begin(115200);

  // LED pin is output
  pinMode(LED_BUILTIN, OUTPUT);

  // Set up Wifi/UDP
  Udp.begin(UDP_SSID, UDP_PASS);
  // Motor Driver config
  MotorControl_config_t mtr_config1=
  {
    .chnlNo = LEDC_CHANNEL_1,
    .ena_pin = MOTOR_1_EN,
    .dir_pin_a = MOTOR_1_DRIVE_A,
    .dir_pin_b = MOTOR_1_DRIVE_B,
    .quad_pin_a = MOTOR_1_QUAD_A,
    .quad_pin_b = MOTOR_1_QUAD_B,
    .kp        =0,
    .ki        =0, 
    .kd        =0,
  };
  motorLeft.setup(mtr_config1);

  MotorControl_config_t mtr_config2=
  {
    .chnlNo    = LEDC_CHANNEL_1,
    .ena_pin   = MOTOR_2_EN,
    .dir_pin_a = MOTOR_2_DRIVE_A,
    .dir_pin_b = MOTOR_2_DRIVE_B,
    .quad_pin_a = MOTOR_2_QUAD_A,
    .quad_pin_b = MOTOR_2_QUAD_B,
    .kp         = 0,
    .ki         = 0, 
    .kd         = 0,
  };
  motorRight.setup(mtr_config2);
}


#define BLINK_RATE_MSECS 1000
unsigned long lastBlinkTime=0;
bool last_led_state=false; // true is 


void loop() {

  // toggle the LED periodically.
  if ( (millis()-lastBlinkTime) >= BLINK_RATE_MSECS)
  {
    switch(last_led_state)
    {
      case(true):
        digitalWrite(LED_BUILTIN, false);
        last_led_state=false;
        break;

        case(false):
        digitalWrite(LED_BUILTIN, true);
        last_led_state=true;
        break;
    }
    lastBlinkTime=millis();
  }

  // Check UDP frequently
  Udp.loop();
}
