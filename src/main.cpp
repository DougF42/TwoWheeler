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
#include "LN298.h"
#include "UdpCmd.h"


UdpCmd Udp;
LN298 ln298Left;
LN298 ln298Right;

void setup() {
  Serial.begin(115200);

  // LED pin is output
  pinMode(LED_BUILTIN, OUTPUT);

  // Set up Wifi/UDP
  Udp.begin(UDP_SSID, UDP_PASS);
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
