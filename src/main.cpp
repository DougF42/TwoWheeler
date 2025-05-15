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
#include <Arduino.h>
#include "config.h"
#include "Params.h"
#include "Nodex.h"
#include "MotorControl.h"
#include "driver/ledc.h"

MotorControl motorLeft;
MotorControl motorRight;

// SMAC related...
Preferences  MCUPreferences;  // Non-volatile memory
Nodex         *ThisNode;  // SMAC Node
uint8_t      RelayerMAC[MAC_SIZE];  // MAC Address of the Relayer Module stored in non-volatile memory.
                                    // This is set using the <SetMAC.html> tool in the SMAC_Interface folder.
                                    // { 0x7C, 0xDF, 0xA1, 0xE0, 0x92, 0x98 }

// - - - - - - - - - - - - - - - - - - - - -
// General setup -
//   Serial port
//   LED
//   Motor Driver for left and right motors
//   WiFi and ESP-NOW
//   
// - - - - - - - - - - - - - - - - - - - - -

void setup() {
  Serial.begin(115200);
  Serial.println("I AM ALIVE....");

  // LED pin is output
  pinMode(LED_BUILTIN, OUTPUT);

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


  // Define the NODE and its DEVICES
  //Nodex(const char *inName, int inNodeID, const uint8_t *macAddr);
  ThisNode = new Nodex("TwoWheeler", 0, Params::NODE_RELAY_MAC() ); // NODE Number 0

}

// - - - - -Just some stuff for 'loop' - - - - - - - - - - - - - - - -
#define BLINK_RATE_MSECS 1000
unsigned long lastBlinkTime=0;
bool last_led_state=false; // true is 

// - - - - - - - - - - - - - - - - - - - - -
// Main operating loop
// - - - - - - - - - - - - - - - - - - - - -
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
  ThisNode->Run();
}
