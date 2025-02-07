/**
 * @file CommandList.h
 * @author Doug F (doug@fajaardo.hm)
 * @brief 
 * @version 0.1
 * @date 2024-12-18
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "Commands.h"
#ifndef C_O_M_M_A_N_D__L_I_S_T__H
#define C_O_M_M_A_N_D__L_I_S_T__H


// This structure defines the format of the list
// (the list is terminated when a minArgCount of -1 is detected))
typedef struct
{
    const char *cmdName;  // The name of the command (as given by the user)
    const char *description; // Points to the description of the command
    int minTokCount;  // How many tokens (minimum). includes command itself
    int maxTokCount;  // How many tokens (max). Includes command itself
    void (*xtrahelp)(Print *outdev, int tokCnt, char *toklist[]); // function to print detail help about this command
    void (*function)(Print *outdev, int tokCnt, char *toklist[]);  // The function. 'outdev' is where to output results.    
} Command_t;


/**
 * @brief The list of available commands
 * 
 */
static Command_t cmdList[]=
{
    {"help", "Help command",            0, 3,      nullptr, Commands::cmdHelp },
    {"?"   , "Help command",            0, 3,      nullptr, Commands::cmdHelp },
    {"END", "END", -1, -1, nullptr, nullptr} 
};


#endif