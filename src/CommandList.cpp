/**
 * @file CommandList.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-04-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "CommandList.h"
#include "config.h"
/**
 * @brief The list of available commands
 * 
 */
const Command_t cmdList[]=
{
    {"help", "Help command",
                                       0, 3,      nullptr,    cmdHelp },
    {"?"   , "Help command",
                                        1, 3,     nullptr,    cmdHelp },

    {"END", "END", -1, -1, nullptr, nullptr} 
};