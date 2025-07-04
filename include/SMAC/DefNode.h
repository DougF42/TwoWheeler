/**
 * @file DefNode.h - add functionality to 'node' class
 * 
 */
#pragma once
#include "Node.h"

class DefNode: public Node
{
    public:
    DefNode (const char *inName, int inNodeID);
    ~DefNode();
    void execute_local(const char *string);
    void execute_local(const char *devNAME, const char *commandString);
    int findDevId(const char *devName);
};