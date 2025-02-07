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
#include "CommandList.h"

/**
 * @brief Construct a new Commands object
 *    Argument is buffer len and output stream.
 */
Commands::Commands()
{
    buffer=nullptr;
    nxtToken=0;    
    setBufLen((size_t)128);
}

Commands::Commands(size_t buflen)
{
    buffer=nullptr;
    setBufLen(bufLen);
}

Commands::~Commands()
{
    free(buffer);
    buffer=nullptr;
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
 * @brief Print a brief help message
 * If no arguments, just list all the commands
 * If 1 argument, print extra help for that command
 * 
 * freindly error messages indicate if the command given (but not found)
 * or if the command does not have extra help
 * 
 * @param outdev - device to output messages to
 * @param tokCnt  - the number of tokens on the command line 
 * @param tokList - the list of commands in this command.
 */
void Commands::cmdHelp(Print *outdev, int tokCnt, char **tokList)
{
    char *targetName;
    bool foundTarget=false;
    if (tokCnt ==2)
    {  
        targetName==tokList[2];

    } else {
        // No target...
        targetName=nullptr;
    }


    for (int i=0; cmdList[i].minTokCount<0; i++)
    {
        if (targetName !=nullptr) 
        {
            // no target, just list the brief description of all commands
            outdev->print("*CMD: "); outdev->print(cmdList[i].cmdName); 
            outdev->print(" with "); outdev->print(cmdList[i].minTokCount);
            outdev->print(" to ");   outdev->print(cmdList[i].maxTokCount); 
            outdev->println(" :");
            outdev->print("    " );  outdev->print(cmdList[i].description);
            foundTarget=true;
        } else {
            // Specific target - is this it?
            if ( 0== strcasecmp(targetName, cmdList[i].cmdName))
            {
                foundTarget=true;
                if (cmdList[i].xtrahelp) {
                    cmdList[i].xtrahelp(outdev, tokCnt, tokList);
                }  else
                {
                    outdev->print("Sorry, there is no help for that command");
                }
            }

        }
    }
    if (! foundTarget)
    {
        outdev->println("Sorry, that commands was not found");
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



bool Commands::setBufLen(size_t newBufLen)
{
    if (buffer!=nullptr) free(buffer);

    bufLen=newBufLen;
    buffer=(char *)malloc(bufLen+1);
    if (buffer==nullptr) 
    {
        Serial.println("Error: could not allocate "); Serial.print(bufLen); Serial.println(" character buffer");
        return(false);
    }
    nxtCharInBuf = buffer;
    bufMaxPtr = buffer+bufLen;
    return(false);
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

    if (nxtCharInBuf != bufMaxPtr) return(false);
    *(nxtCharInBuf++) = ch;

    return(true);
}

/**
 * @brief Add a block and parse it
 *   Upon entry, the buffer is cleared, and the new block
 * is moved into the buffer and parsed.
 * 
 *   The new block must not be larger than the size of the buffer.
 *   
 * @param blk 
 * @param len 
 * @param return true - normal
 * @param return false - Either not enough room in buffer,
 *     or buffer has not been allocated.
 */
bool Commands::addBlock(char *blk, size_t len)
{
    if (buffer == nullptr)
    {
        return (false);
    }

    if (len > bufLen) return(false);
    memcpy(buffer, blk, len);
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
 * Commands are space-separated strings
 *   This builds a list of pointers to the 
 * begining of each 'word' into the 'tokens' array, 
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
    nxtToken=0;
    // TODO: Parse the command into an argument list
}



/**
 * @brief Run the command, based on the command list
 * 
 * This searches the command list for the first entry where:
 *  (1) the command matches,
 *  (2) The actual number of arguments matches.
 * Error message is printed if not found.
 */
void Commands::runCommand()
{
    // TODO: Find this command in the command list
}