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
#include "config.h"
#include "Commands.h"

#ifndef C_O_M_M_A_N_D__L_I_S_T__H1
#define C_O_M_M_A_N_D__L_I_S_T__H1

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

extern const Command_t cmdList[];
#endif