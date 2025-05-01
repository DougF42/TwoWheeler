/**
 * @file Params.h
 * @author Doug Fajardo
 * @brief Use 'parameters" library to save and restore configuration data.
 * @version 0.1
 * @date 2025-04-30
 * 
 * @copyright Copyright (c) 2025 DEF
 * 
 * Genreal usage:  
 *     All functions (Except the initializer) are 'static', This class should
 * be instantiated once, probably in 'main'. 
 * 
 * 
 * 'get' functions are used to retrieve the value of a parameter. These 
 *        only access flash once - after that the value is retained in memory
 *        for additional calls. Default values (if not set in flashed) are defined
 *        in the'config.h' file.
 * 
 *        For strings, they are *always* null-terminated after the last character (dont
 *        forget to allow for this if you copy the string). For 'get' functions, the 
 *        returned pointer is read only and static, and may be used directly. 
 *        If the parameter is changed, the content to will be updated.
 * 
 * 'set' functions are provided for all known parameters. Parameters are
 *        stored immediatly if the corresponding set function is called.
 * 
 * 'clear' will clear all defaults from flash, and clear all in-memory values.
 * 
 * These are the parameters:
 * WIFI_SSID  const char * 
 * WIFI_PASS  const char *
 * WIFI_ADDR  
 * NODE_NAME
 * NODE_ID_NO
 * NODE_RELAY_MAC
 */
#pragma once
#include "config.h"
#include <Arduino.h>
#include <Preferences.h>

 class Params
 {
    private:
        static char ssid[16+1];
        static char pass[32+1];
        static uint16_t port;
        static char nodeName[32+1];
        static int nodeId;
        static uint8_t relayMacAddr[MAC_SIZE];
        static Preferences MyPrefs;
        static void readAll();
        static void ReadOne(const char *pname, uint8_t *value, int paramLen, const uint8_t *deflt, int defLen);


    protected:        
        static void store(char *name, void *value, int len); // ALL data is stored as raw bytes.

    public:
        Params(); // general initialization function.

        static void clearFlash();
        static void WIFI_SSID(const char *newSSID); // Null terminated, max 16 chars
        static const char *WIFI_SSID();

        static void WIFI_PASS(const char *wifiPas); // Null terminated, max 32 chars
        static const char *WIFI_PASS();

        static void WIFI_PORT(uint16_t wifiport); // Null terminated, max 32 chars
        static uint16_t *WIFI_PORT();

        static void NODE_NAME(const char *name);    // Null terminated, up to 16 chars
        static const char *NODE_NAME();

        static void NODE_ID_NO(uint8_t val);
        static uint8_t NODE_ID_NO();

        static void NODE_RELAY_MAC(uint8_t *val);   // val MUST be MAC_SZIE bytes!
        static uint8_t *NODE_RELAY_MAC(); 

 };
