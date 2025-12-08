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
#include "DEV_Driver.h"
#include "DEV_INA3221.h"

DEV_INA3221      *myIna3221Device;

#endif
