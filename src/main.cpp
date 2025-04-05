#include <Arduino.h>
#include "UdpCmd.h"
#include "LN298.h"
#include "QuadEncoder.h"

UdpCmd Udp;
LN298 ln298Left;
LN298 ln298Right;
QuadEncoder m1quad;
QuadEncoder m2quad;

void setup() {
  Serial.begin(115200);

  // LED pin is output
  pinMode(LED_BUILTIN, OUTPUT);

  // Set up Wifi/UDP
  Udp.begin(UDP_SSID, UDP_PASS);
  ln298Left.begin(MOTOR_1_EN, MOTOR_1_DRIVE_A, MOTOR_1_DRIVE_B); 
  ln298Right.begin(MOTOR_2_EN, MOTOR_2_DRIVE_A, MOTOR_2_DRIVE_B); 
  m1quad.begin(MOTOR_1_QUAD_A, MOTOR_1_QUAD_A);
  m2quad.begin(MOTOR_2_QUAD_A, MOTOR_2_QUAD_B);
}


#define BLINK_RATE_MSECS 1000
unsigned long lastBlinkTime=0;
bool last_led_state=false; // true is 


void loop() {
  // put your main code here, to run repeatedly:

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
