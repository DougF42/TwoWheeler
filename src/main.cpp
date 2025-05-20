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
// #define USE_SMAC
// #define TEST_LN298
#define TEST_QUAD

#include <Arduino.h>
#include "config.h"
#include "Params.h"
#include "Nodex.h"
#include "MotorControl.h"
#include "driver/ledc.h"

#ifdef USE_SMAC
MotorControl motorLeft;
MotorControl motorRight;
#endif

// SMAC related...
Preferences  MCUPreferences;  // Non-volatile memory

#ifdef USE_SMAC
Nodex         *ThisNode;  // SMAC Node
#endif
uint8_t      RelayerMAC[MAC_SIZE];  // MAC Address of the Relayer Module stored in non-volatile memory.
                                    // This is set using the <SetMAC.html> tool in the SMAC_Interface folder.
                                    // { 0x7C, 0xDF, 0xA1, 0xE0, 0x92, 0x98 }
#ifdef TEST_LN298
#include "ln298.h"
LN298 ln298_1;
LN298 ln298_2;
#endif

#ifdef TEST_QUAD
#include "QuadDecoder.h"
QuadDecoder quad1;
QuadDecoder quad2;
#endif

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

  // Use interrupts for Quad decoder
  // The ISR service is only installed once for all pins
  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE | ESP_INTR_FLAG_LOWMED));
  //  ESP_INTR_FLAG_IRAM
  // ESP_INTR_FLAG_SHARED ????


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


  MotorControl_config_t mtr_config2=
  {
    .chnlNo    = LEDC_CHANNEL_2,
    .ena_pin   = MOTOR_2_EN,
    .dir_pin_a = MOTOR_2_DRIVE_A,
    .dir_pin_b = MOTOR_2_DRIVE_B,
    .quad_pin_a = MOTOR_2_QUAD_A,
    .quad_pin_b = MOTOR_2_QUAD_B,
    .kp         = 0,
    .ki         = 0, 
    .kd         = 0,
  };

#ifdef USE_SMAC
  motorLeft.setup(mtr_config1);
  motorRight.setup(mtr_config2);
#endif

#if !defined TEST_LN298 && !defined TEST_QUAD
  // Define the TWO? MOTORS and a driver
  motorRight.setup(mtr_config2);
  // Nodex(const char *inName, int inNodeID, const uint8_t *macAddr);
  ThisNode = new Nodex("TwoWheeler", 0, Params::NODE_RELAY_MAC()); // NODE Number 0
#endif

#if defined TEST_LN298
  ln298_1.setupLN298(LEDC_CHANNEL_0, MOTOR_1_EN, MOTOR_1_DRIVE_A, MOTOR_1_DRIVE_B);
  ln298_1.setPulseWidth(45); // 45 percent power
  Serial.println("SETUP of #1 complete\r\n");

  ln298_2.setupLN298(LEDC_CHANNEL_1, MOTOR_2_EN, MOTOR_2_DRIVE_A, MOTOR_2_DRIVE_B);
  ln298_2.setPulseWidth(45); // 45 percent power
  Serial.println("Setup of #2 complete\r\n");


#endif

#if defined TEST_QUAD
  quad1.setupQuad( MOTOR_1_QUAD_A, MOTOR_1_QUAD_B);
  quad1.calibrate_raw_pos();
  Serial.printf("Quad 1 is defined\r\n");

  quad2.setupQuad( MOTOR_2_QUAD_A, MOTOR_2_QUAD_B, true);
  quad2.calibrate_raw_pos();
  Serial.println("Quad 2 is defined\r\n");

#endif
}

// - - - - -Just some stuff for 'loop' - - - - - - - - - - - - - - - -
#define BLINK_RATE_MSECS 2000
unsigned long lastBlinkTime=0;
bool last_led_state=false; // true is 

#ifdef TEST_LN298
int speed1=0;
int speed2=0;
bool invertFlag=true;
#endif

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
    lastBlinkTime = millis();
#ifdef TEST_LN298
    invertFlag= !invertFlag;
     speed1 += 10;
     if (speed1 > 100) speed1 = 45;
     if (invertFlag)
       ln298_1.setPulseWidth(speed1 * -1);
     else
       ln298_1.setPulseWidth(speed1);

     speed2 += 10;
     if (speed2 > 100) speed2 = 45;
     if (invertFlag)
       ln298_2.setPulseWidth(speed2 * -1);
     else
       ln298_2.setPulseWidth(speed2);
    Serial.println(" ");
#endif

#ifdef TEST_QUAD
    quad1.quadLoop();
    Serial.printf("Q1: Position is %8.2f   speed=%8lld\r\n", quad1.getPosition(), quad1.getSpeed());
    quad1.resetPos();

    quad2.quadLoop();
    Serial.printf("Q2: Position is %8.2f   speed=%8lld\r\n", quad2.getPosition(), quad2.getSpeed());
    quad2.resetPos();
#endif
  }

#ifdef TEST_LN298
  // No test actions at this time
#endif

#ifdef USE_SMAC
  // Run the SMAC nodes and devices...
  ThisNode->Run();
#endif
}
