/**
 * @file DefNode.cpp
 *     Add some functionality to the 'node' class
 */
#include "DefNode.h"
#include <string.h>
#include <stdlib.h>

DefNode::DefNode(const char *inName, int inNodeID) : Node(inName, inNodeID)
{
    return;
}

DefNode::~DefNode()
{
    return;
}

/** = = = = = = = = = = = = = = = = = = = = = = = 
 * Given a device name, what is its ID?
 * @param devName - NOT case sensitive!
 * @return  <0 if not found, otherwise the device ID
 */
int DefNode::findDevId(const char *devName)
{
    for (int devIdx=0; devIdx<MAX_DEVICES; devIdx++)
    {
        if (0 == strcasecmp(devName, devices[devIdx]->GetName()) )
        {
            return(devIdx);
        }
    }

    return(-1);
}
/** = = = = = = = = = = = = = = = = = = = = = = =
 * Execute a command that is generated internally
 *   Node->Execute is tried first, if not then 
 *  we try the device.
 * 
 * IF the command returns data, we print it on the 
 *  serial port.
 * Currently, we ignore the 'NN' (node).
 * @param str - the full SMAC command string (e.g.: DD|CCCC|params...)
 *     This is broken into the 'CommandString CPacket structure, and
 * the device is called...
 */
void DefNode::execute_local(const char *commandString)
{
    char buf[250];
    int devIdx=0;
    size_t cLength = strlen(commandString);

    if (cLength < 10)
    {
        Serial.print("Invalid Local command - too short: ");
        Serial.println(commandString);
        return;
    }

    // Device Index
    devIdx = strtol(commandString, nullptr, 10);
    if (devIdx > 99) devIdx=99;
    if (devIdx < 0)
    {
        Serial.print("Invalid local command - device id is negative?");
        Serial.println(commandString);
    }

    // COMMAND
    memcpy(CommandPacket.command, commandString+3, COMMAND_SIZE);
    CommandPacket.command[COMMAND_SIZE] = 0;

    // Params (if any)
    if (cLength > MIN_COMMAND_LENGTH + 1)
        strcpy(CommandPacket.params, commandString + 8);
    else
        CommandPacket.params[0] = 0;

    // Execute the command
    pStatus = ExecuteCommand(); // this should execute 'node' level commands
    if (pStatus == NOT_HANDLED)
    {
        //=================================================
        // Not a Node command, so pass to Device to handle
        //=================================================

        // Check deviceIndex range
        if (devIdx >= numDevices)
        {
            if (Debugging)
            {
                Serial.print("Command targeted for unknown device: ");
                Serial.print("deviceIndex=");
                Serial.print(devIdx);
                Serial.print(", numDevices=");
                Serial.println(numDevices);
            }

            strcpy(DataPacket.value, "ERROR: Command targeted for unknown device");
            pStatus = FAIL_DATA;
        }
        else
            pStatus = devices[devIdx]->ExecuteCommand();
    }

    // Any data to send?
    if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
    {
        // Populate deviceID
        memcpy(DataPacket.deviceID, commandString, ID_SIZE);
        sprintf(buf, "XX|%s|%s\n", DataPacket.deviceID, DataPacket.value );
    }
}