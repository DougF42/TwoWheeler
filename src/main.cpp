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
#include "MotorDefs.h"
#include "UdpCmd.h"
#include "driver/gpio.h"


UdpCmd Udp;
MotorDefs motors;

void setup() {
  Serial.begin(115200);

  // LED pin is output
  pinMode(LED_BUILTIN, OUTPUT);

  // Define the motors
  motors.defineMotor(0, MOTOR_1_EN, MOTOR_1_DRIVE_A, MOTOR_1_DRIVE_B, MOTOR_1_QUAD_A, MOTOR_1_QUAD_B);
  motors.defineMotor(1, MOTOR_2_EN, MOTOR_2_DRIVE_A, MOTOR_2_DRIVE_B, MOTOR_2_QUAD_A, MOTOR_2_QUAD_B);


  // Set up Wifi/UDP
  Udp.begin(UDP_SSID, UDP_PASS);

  // TODO: Define left and right motors
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
