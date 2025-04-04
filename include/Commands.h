/**
 * @file Commands.h
 * @author Doug F (doug@fajaardo.hm)
 * @brief 
 * @version 0.1
 * @date 2024-12-18
 * 
 * @copyright Copyright (c) 2024
 * 
 * This buffers an incoming sequence, then executes the requested command(s)
 * We inherit from the 'print' class. 
 * 
 * When inheriting this class, the following virtual functions *must* be provided by the caller:
 * 
 *  size_t write(uint8_t)
 *  size_t write(cons6t uint8_t buffer, size_T size)
 * 
 * These virtual functions *may* be overriden by the caller:
 *   int availableForWrite();
 *   bool isEndOfCommand(char ch)
 */
#ifndef C_O_M_M_A_N_D_S__H
#define C_O_M_M_A_N_D_S__H
#include "Arduino.h"
#define MAX_ARGUMENTS 4
#define MAX_LINE_LENGTH 128
#include "CommandList.h"

class Commands : public Print{
    private:
        char buffer[MAX_LINE_LENGTH+1];  // Where to store the incoming message
        char *nxtCharInBuf;   // Points to where next char goes in buffer
        char *bufMaxPtr;      // Points to last char position in buffer

        char *tokens[MAX_ARGUMENTS];
        int nxtToken;        
        void parseCommand();  // Break the command into words
        void runCommand();    // Run the selected command

    public:
        Commands();
        ~Commands();
    
        void flush();
        bool addChar(char ch);
        bool addBlock(char *blk, size_t len);
        static bool getIntFromToken(char *token, int &newVal, int baseNo=0);
        static void notImplemented(Print *outdev, int tokcnt, char *tokList[]);

        virtual bool isEndOfChar(char ch);
        // NOTE: User implementation will decide if a line is complete based on "\r\n" sequence
        //       from the print class (when println(..) is used)
        virtual size_t write(uint8_t) = 0;                        // user specified
        virtual size_t write(const uint8_t *buffer, size_t size)=0; // user specified
        virtual int availableForWrite() { return 0; }   // default - does nothing



};
#endif