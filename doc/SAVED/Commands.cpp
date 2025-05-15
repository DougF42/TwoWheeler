/**
 * @file Commands.cpp
 * @author Doug F (doug@fajaardo.hm)
 * @brief
 * @version 0.1
 * @date 2024-12-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <Arduino.h>
#include "config.h"
#include "CommandList.h"

/**
 * @brief Construct a new Commands object
 *    Argument is buffer len and output stream.
 */
Commands::Commands()
{
    nxtCharInBuf=buffer;
    bufMaxPtr  = buffer+MAX_LINE_LENGTH;
    nxtToken=0;    
}

Commands::~Commands()
{
    return;
}

void Commands::flush()
{
    nxtCharInBuf= buffer;
    return;
}


/**
 * @brief Get the integer value from a token
 *    The string may be a simple integer, or 0x?? for hex, 
 *    It is an error  for extra chars after the integer.
 * 
 * @param outdev - where to output any error message
 * @param token   - the string to parse
 * @Param baseNo  - if 0, any base (indicated by syntax). Base of number.
 * @param newVal  - the resulting value if valid
 * @return true   - normal return
 * @return false  - Error in parsing - invalid string
 */
bool Commands::getIntFromToken(char *token, int &newVal, int baseNo)
{
    if (token==nullptr) return(false); // sanity check

    char *endptr=nullptr;
    long v = strtol( token, &endptr, 0);
    if (*endptr == '\0') return(true);
    return(false);
}

/**
 * @brief This utility outputs a specific entry from cmdlist
 *   NOTE: IT IS NOT PART OF 'Commands' class
 * @param outdev - the device to print on
 * @param idx    - the index of the desired entry
 */
void showHelp(Print *outdev, const Command_t *cmd)
{
            // no target, just list the brief description of all commands
            outdev->print(" CMD: '"); outdev->print(cmd->cmdName); 
            outdev->print("'  with "); outdev->print(cmd->minTokCount);
            outdev->print(" to ");   outdev->print(cmd->maxTokCount); 
            outdev->print(" tokens.  ");
            outdev->println(cmd->description);
}

/**
 * @brief Print a brief help message
 * If no arguments, just list all the commands
 * If 1 argument, print extra help for that command
 * 
 * freindly error messages indicate if the command given (but not found)
 * or if the command does not have extra help
 * ** NOTE: THIS IS NOT PART OF Commands class.
 * 
 * @param outdev - device to output messages to
 * @param tokCnt  - the number of tokens on the command line 
 * @param tokList - the list of commands in this command.
 */
void cmdHelp(Print *outdev, int tokCnt, char **tokList)
{
    bool foundTarget = false;
    const Command_t *cmd;
    if (tokCnt == 1)
    {
        // No target...simple output
        foundTarget = true;
        for ( cmd=cmdList; cmd->maxTokCount > 0; cmd++)
        {
            showHelp(outdev, cmd);
        }
    }
    else
    {
        // Output extra help for specific command:
        char *targetName = tokList[1];

        for (cmd = cmdList; cmd->maxTokCount > 0; cmd++)
        {
            if (0 == strcasecmp(targetName, cmd->cmdName))
            {
                foundTarget = true;
                showHelp(outdev, cmd); // generic description of command
                if (cmd->xtrahelp)
                {
                    cmd->xtrahelp(outdev, tokCnt, tokList);
                }
                break;
            }
        }
    }

    if (!foundTarget)
    {
        outdev->println("Sorry, that command was not found");
    }
    else
    {
        outdev->println("OK");
    }
}

/**
 * @brief Placeholder until a command can be implemented
 *    It only prints a 'lonely' message <grin>
 * @param outdev - device to output messages to
 * @param tokCnt  - the number of tokens on the command line 
 * @param tokList - the list of commands in this command.
 */
void Commands::notImplemented(Print *outdev, int tokcnt, char *tokList[])
{
    outdev->println ("Sorry, that command not implemented");
}


/**
 * @brief Add 1 character to buffer
 *    If the buffer is full, ignore it.
 * @param ch - the character to add
 * @return true - normal return
 *         false - buffer not allocated, or no room in buffer
 * 
 */
bool Commands::addChar(char ch)
{
    if (buffer==nullptr)   
    { 
        return(false);
    }

    if (isEndOfChar(ch))
    {
        parseCommand();
        flush();
    }

    if (nxtCharInBuf >= bufMaxPtr) return(false);
    *(nxtCharInBuf++) = ch;

    return(true);
}


/**
 * @brief Add a block and parse it
 *   Upon entry, the buffer is cleared, and the new block
 * is moved into the buffer and parsed.
 *  
 *   The new block must not be larger than the size of the buffer.
 *   The new block is assumed to be a complete command, ready to parse.
 *  
 * @param blk 
 * @param len 
 * @param return true - normal
 * @param return false - Either not enough room in buffer,
 *     or buffer has not been allocated.
 */
bool Commands::addBlock(char *blk, size_t len)
{
    if ((blk == nullptr) || (*blk=='\0')) 
    {
        return (false);
    }

    if (len > MAX_LINE_LENGTH) return(false);

    memcpy(buffer, blk, len);
    nxtCharInBuf= buffer+len;
    parseCommand();
    return(true);
}

/**
 * @brief Default 'isEndOfCommand' routine
 *  (This is virtual, can be overidden by user)
 *   Ifd a CR or LF is seen, it is considered end
 * @param ch     - character to test
 * @return true  -  Yes, this represents an end of command.
 * @return false  - no, this is part of the command.
 */
bool Commands::isEndOfChar(char ch)
{
    if ( (ch=='\r') || (ch=='\n') ) return(true);
    return(false);
}


/**
 * @brief  * @brief Break the command into component parts.
 * Commands are whitespace-separated tokens (sp, tab, cr, nl, comma, semi-coln)
 * 
 *   This builds a list of pointers to the 
 * begining of each 'token' into the 'tokens' array, 
 * and replaces the terminating character with a null.
 * 
 * (Note that the command itself is at index 0)
 * 
 * (Note: The buffer is defined with 1 more character
 * than its nominal size - this guarentees we can null-termninate
 * the last argument).
 * 
 */
void Commands::parseCommand()
{
    
    *nxtCharInBuf='\0'; // make sure we are null-terminated.

    nxtToken=0;
    nxtCharInBuf=buffer;
    for (nxtToken=0; nxtToken<MAX_ARGUMENTS; nxtToken++)
    {
        tokens[nxtToken] = strsep((char **) &nxtCharInBuf, COMMAND_WHITE_SPACE);
        if (tokens[nxtToken] == NULL)  break;
    }

    Serial.print("Commands::parseCommand tokcnt=");
    Serial.println(nxtToken);
    if ( (tokens[0] != nullptr) && (*tokens[0] != '\0' ) )    // ignore empty line.
    {
        for (int i = 0; i < nxtToken; i++)
        {
            Serial.print("Token "); Serial.print(i); Serial.println(tokens[i]);        
        }
 
        {
            runCommand(); 
        }
    }
    nxtToken=0;
    nxtCharInBuf=buffer;
    return;
}



/**
 * @brief Run the command, based on the command list
 * 
 * This searches the command list for the first entry where:
 *  (1) the command matches,
 *  (2) The actual number of arguments is allowed for that command.
 * 
 * Error message is printed if not found.
 */
void Commands::runCommand()
{
    // TODO: Find this command in the command list
    bool foundit=false;
    Serial.print("runCommand: token is "); Serial.println(tokens[0]);
    Serial.print("   token Length is "); Serial.println(strlen(tokens[0]));
    int cmdidx;
    for (cmdidx=0; cmdList[cmdidx].minTokCount != -1; cmdidx++)
    {
        Serial.print("Try command no ");Serial.print(cmdidx);         
        Serial.print(" NAME=");Serial.println(cmdList[cmdidx].cmdName);

        if (0 != strcasecmp(cmdList[cmdidx].cmdName, tokens[0]) ) continue;
        Serial.println("NAME MATCH");

        if ( (nxtToken < cmdList[cmdidx].minTokCount) || (nxtToken > cmdList[cmdidx].maxTokCount) )continue;
        Serial.println("MATCH!!!");

        cmdList[cmdidx].function(this, nxtToken, tokens); // run this command and stop
        foundit=true;
        break;
    }

    if ( ! foundit )
    {
        Serial.println(); Serial.println("***Command not found");
        this->println("ERROR: COMMAND NOT FOUND");
    }
    return;
}