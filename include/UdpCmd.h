/**
 * @file UdpCmd.h
 * @author Doug Fajardo
 * @brief Handle commands from a UDP connection
 * @version 0.1
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <Arduino.h>
#include "Commands.h"
#include <WiFi.h>
#define UDP_OUTPUT_BUFFER_SIZE 128
#define UDP_INPUT_BUFFER_SIZE  128

class UdpCmd: public Commands
{
private:
    bool connected;
    WiFiUDP udp; // the UDP class object

    void connectToWiFi(const char *ssid, const char *pwd, bool waitFlag=false);
    IPAddress myAddr;    // my local address
    IPAddress mySubnet;  // subnet for my local addr
    IPAddress myGateway; // my default Gateway
    IPAddress respondAddr; // Who to send a repl to
    uint16_t  respondPort; // what port to send a response to
    uint8_t inBuf[UDP_INPUT_BUFFER_SIZE+1];
    uint8_t outBuf[UDP_OUTPUT_BUFFER_SIZE+1];
    int nextInoutBuf; // index of next buffer position to write

public:
    UdpCmd();
    ~UdpCmd();
    void begin(const char * ssid, const char *pass, bool waitflag=false);
    void end();
    void loop(); // check for incoming commands and process them.

    bool isEndOfChar(char ch) override;

    size_t write(uint8_t) override; 
    size_t write(const uint8_t *buffer, size_t size) override;
    int availableForWrite() override;
};

