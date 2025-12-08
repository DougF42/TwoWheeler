//
//
#ifndef user_global_once
#define user_global_once

// SMAC Systems can have up to 20 Nodes.
// Set the Name and NodeID for this ESP32 module (0-19).
// The NodeID's for a SMAC Systems with multiple Nodes
// must be unique and cannot be duplicated.
char  ThisNodeName[] = "TwoWheeler";  // Name for this node (max 32 chars)
int   ThisNodeID     = 1;                // NodeID (0-19)

// = = = = = PUT INCLUDES here
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "DEV_QuadDecoder.h"
#include "DEV_ln298.h"
#include "DEV_Pid.h"
#include "DEV_MotorControl.h"
#include "DEV_Driver.h"
#include "DEV_INA3221.h"


// This MUTEX is used to control access to the I2C bus
xSemaphoreHandle I2CMutex=xSemaphoreCreateMutex();

//xSemaphoreTake(I2CMutex, portMAX_DELAY))
//xSemmaphoreGive(I2CMutex);


// LEFT side
DEV_QuadDecoder  *l_quad   = new DEV_QuadDecoder("L-Quad");
DEV_LN298        *l_ln298  = new DEV_LN298("L-298n");
DEV_Pid          *l_pid    = new DEV_Pid("L-pid");
DEV_MotorControl *l_motor  = new DEV_MotorControl("L-Motor");

DEV_Driver       *driver   = new DEV_Driver("Driver");

// RIGHT side
DEV_LN298        *r_ln298  = new DEV_LN298("R-298n");
DEV_QuadDecoder  *r_quad   = new DEV_QuadDecoder("R-Quad");
DEV_Pid          *r_pid    = new DEV_Pid("R-pid");
DEV_MotorControl *r_motor  = new DEV_MotorControl("R-Motor");

DEV_INA3221      *myIna3221Device = new DEV_INA3221("Power", I2C_INA3221_ADDR, &Wire);

#endif
