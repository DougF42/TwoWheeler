//=============================================================================
//
//       FILE : ThisNode.cpp
//
//    PROJECT : SMAC Framework - Example 1
//
//      NOTES : This is the PIO firmware for the SMAC Node of Example 1.
//
//              About this template:
//              - The SMAC System uses Espressif's ESP-NOW protocol between Node Modules and the Relayer Module.
//              - Device is the base class from which your custom Devices are derived.
//              - This "template" creates a Node with a single Device, a LightSensor.
//              - Node Modules first attempt to connect to the Relayer Module.
//              - Once connected, the LightSensor Device "measures" a value and outputs a "Data String" with its value.
//              - The above operation is performed periodically to maintain continuous data.
//              - All Device data can be visualized with gauges and graphs using the SMAC Interface (a Chrome browser app).
//              - The SMAC System is bidirectional. You can send commands to both Nodes and individual Devices.
//              - Commands can be sent directly from the SMAC Interface using buttons, dials, sliders, etc.
//              - The Node and Device base classes handle standard commands and child classes can handle custom commands.
//
//              Devices in this example:
//                LightSensor -- Demo to show how sensor data can be sent to the SMAC Interface
//
//  DEBUGGING : Set the global <Debugging> to true to see debugging info in Serial Monitor.
//              Be sure to set <Debugging> to false for production builds!
//
//     AUTHOR : Bill Daniels
//              Copyright 2025-2026, D+S Tech Labs, Inc.
//              All Rights Reserved
//
//   2/1/2026 DEF Drop 'motor' device, resequence devices.
//
//=============================================================================

//--- Includes --------------------------------------------

#include "ThisNode.h"
#define USE_RIGHT_WHEEL_DRIVERS false
#define USE_POWER_MONITOR       false
#define NOTESTING               false
// Place your Device includes here
#include "SMAC/Device.h"
#include "DEV_Driver.h"
#include "DEV_Pid.h"
#include "DEV_ln298.h"
#include "DEV_QuadDecoder.h"
#include "DEV_INA3221.h"

//--- Debugging -------------------------------------------

bool  Debugging = false;  // ((( Set to false for production builds )))

//--- Constructor -----------------------------------------

ThisNode::ThisNode ()
{
  // Start with a bad status
  goodToGo = false;


  // SMAC Systems can have up to 20 Nodes.
  // Set the Name and unique Node index for this Node (0-19).
  // The Node indexes for a SMAC System with multiple Nodes must be unique and cannot be duplicated.

  //--- Create the Node Instance ---
  //                          ┌───────────── The Name of your Node
  //                          │         ┌─── The Unique Node Index (0-19)
  //                          │         │
  thisNode = new Node ("TwoWheeler", 1);
  if (thisNode == nullptr) return;



  //=======================================================
  // Create or Start/Begin any infrastructure your Devices
  // may need. Then, if necessary, pass those references
  // to your Devices' constructors.
  //=======================================================
//

//=======================================================
// This structure is used to define all parameters
//   for a given MOTOR (and the QUAD/L298 and PID classes)
MotorControl_config_t left_mtr_cfg =
    {
        .chnlNo = LEDC_CHANNEL_1,
        .ena_pin = MOTOR_1_EN,
        .dir_pin_a = MOTOR_1_DRIVE_A,
        .dir_pin_b = MOTOR_1_DRIVE_B,
        .quad_pin_a = MOTOR_1_QUAD_A,
        .quad_pin_b = MOTOR_1_QUAD_B,
        .kp = 0,
        .ki = 0,
        .kd = 0,
};

#if USE_RIGHT_WHEEL_DRIVERS
MotorControl_config_t right_mtr_cfg =
    {
        .chnlNo = LEDC_CHANNEL_2,
        .ena_pin = MOTOR_2_EN,
        .dir_pin_a = MOTOR_2_DRIVE_A,
        .dir_pin_b = MOTOR_2_DRIVE_B,
        .quad_pin_a = MOTOR_2_QUAD_A,
        .quad_pin_b = MOTOR_2_QUAD_B,
        .kp = 0,
        .ki = 0,
        .kd = 0,
};
#endif

//=======================================================
// Add all Devices to your Node
//=======================================================
// LEFT SIDE
#if NOTESTING
// Dev 0 is the left L298 device.
DEV_LN298  l_ln298("Left298");
l_ln298   .setup(&left_mtr_cfg);
thisNode->AddDevice(&l_ln298);      // Device 1
#endif

// Dev 1 is the left QUAD device.
DEV_QuadDecoder l_quad("LeftQuad");
l_quad    .setup(&left_mtr_cfg);
thisNode->AddDevice(&l_quad);       // device 0

#if NOTESTING

// Dev 2 is the left PID device.
DEV_Pid   l_pid("LeftPID");
l_pid     .setup(&left_mtr_cfg, &l_quad, &l_ln298);
thisNode  ->AddDevice(&l_pid);        // Device 2
#endif

#if USE_RIGHT_WHEEL_DRIVERS
// RIGHT side
// Dev 3 is the right quad decoder
DEV_QuadDecoder r_quad("rightQUAD");
r_quad    .setup(&right_mtr_cfg);
thisNode->AddDevice(&r_quad); // Device 4

// Dev 4 is the right ln298 driver
DEV_LN298  r_ln298("RightLN298");
r_ln298   .setup(&right_mtr_cfg);
thisNode->AddDevice(&r_ln298); // Device 5

// Dev 5 is the right PID controller
DEV_Pid  r_pid("RightPID");
r_pid     .setup(&right_mtr_cfg, &r_quad, &r_ln298);
thisNode->AddDevice(&r_pid); // Device 6

// Dev 6 is the right PID controller
DEV_Driver driver("Driver");
driver   .setup(&l_pid, &r_pid);
thisNode->AddDevice(&driver); // Device 8
#endif

#if USE_POWER_MONITOR
// Dev 7 is the power monitor
//Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
// DEV_INA3221  power("power", I2C_INA3221_ADDR, &Wire);
// myIna3221Device = new DEV_INA3221("Power", I2C_INA3221_ADDR, &Wire);
// ThisNode->AddDevice(myIna3221Device);
#endif

// Startup is good
  goodToGo = true;
}


//--- GetNode ---------------------------------------------

Node  *ThisNode::GetNode ()
{
  return thisNode;
}

//--- GoodToGo --------------------------------------------

bool  ThisNode::GoodToGo ()
{
  return goodToGo;
}

//--- AuxLoop ---------------------------------------------

void  ThisNode::AuxLoop ()
{
  // If you have any "loop()" code that is outside the SMAC System, place it here.
  // It will be called in the main loop() function.

  // ...

}
