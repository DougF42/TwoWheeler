/**
 * @fileParams.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-04-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <Arduino.h>
#include "Params.h"

 //=============================================================================
 // Internal definitions
 //=============================================================================
#define PARAM_NAME "TwoWheeler"
#define PARAM_SSID_KEY "ssid"
#define PARAM_PASS_KEY "key"
#define PARAM_PORT_KEY "port"
#define PARAM_NODENAME_KEY "nodeName"
#define PARAM_NODEID_KEY "nodeId"
#define PARAM_RELAYADDR_KEY "relayMac"

 //=============================================================================
 // static variables 
 //=============================================================================

 // If 0, we are not inited. If greater than one, we need to wait (someone is 
//     writing to flash)
 char     Params::ssid[16+1];
 char     Params::pass[32+1];
 uint16_t Params::port;
 char     Params::nodeName[32+1];
 int      Params::nodeId;
 uint8_t  Params::relayMacAddr[MAC_SIZE];

  //=============================================================================
  //   This is called by all get and set functions. It checks
  // to see if we are initialzed, and if not this is where it will 
  // happen.
  //
   //=============================================================================
 Params::Params()
 {
    // INITIALIZE EVERYTHING
    MyPrefs.begin(PARAM_NAME, false);
    readAll();
    MyPrefs.end();
 }

 //=============================================================================
 // Get all parameters from flash into local memory.
 // This assumes that 'MyPrefs' is already open for read and write.
 //
 //  If there is no value set for the parameter, then we set the default
 //=============================================================================
 void Params::readAll()
 {
  uint8_t wrk8[6];   // tmp working variable 
  uint16_t wrk16; // tmp working variable

  MyPrefs.begin(PARAM_NAME, false);

  ReadOne(PARAM_SSID_KEY,    (uint8_t *) ssid, sizeof(ssid), (uint8_t *) UDP_SSID, strlen(UDP_SSID)+1 );
  
  ReadOne(PARAM_PASS_KEY,    (uint8_t *)pass, sizeof(pass),(uint8_t *) UDP_PASS, strlen(UDP_PASS)+1 );
  
  wrk16 = UDP_PORT;
  ReadOne(PARAM_PORT_KEY,     (uint8_t *)&port, sizeof(port), (uint8_t *) &wrk16, sizeof(uint16_t));

  ReadOne(PARAM_NODENAME_KEY, (uint8_t *)nodeName, sizeof(nodeName), (const uint8_t *)SMAC_NODENAME, strlen(SMAC_NODENAME)+1);

  wrk8[0] = SMAC_NODENO;
  ReadOne(PARAM_NODEID_KEY,   (uint8_t *)&nodeId, sizeof(nodeId),  &wrk8[0], 1);

  //uint8_t  Params::relayMacAddr[MAC_SIZE];
  // TODO: 

  MyPrefs.end();
 }


//=============================================================================
// General purpose 'read' one value.
// It is normally only called from readAll() routine. We expect that
// 'MyPrefs' is already open.
//
// IF the value is not in Flash, then we write the default value to flash,
// and use that value. Defaults are defined in config.h
//
// The value (and the default) are treated as raw unsigned bytes.
//            If this is a string, include the terminating null in the size.
//
// pname - the name of the paramter in the flash
// value - where to store the value.
// paramLen - the length of the value (MUST be max len of the paramter)
// default  - the default value (used if paramter is not in flash)
// defLen   - the length of the default value (NOTE: We will pad with 0 bytes to paramLen)
//=============================================================================
void Params::ReadOne(const char *pname, uint8_t *value, int paramLen, const uint8_t *deflt, int defLen)
{
  if (0== MyPrefs.getBytes(pname, value, paramLen))
  {  // Not readable... write default
    memset(value, 0, paramLen);
    memcpy(value, deflt, defLen);
    MyPrefs.putBytes(pname, value, paramLen);
  }
  
}

//=============================================================================
// General purpose 'store' one value. 
// The flash is opened, the value written, then the flash is closed.
// ALL values are treated as raw bytes of an appropriate length - in the 
// case of strings, random data may be stored if the value is shorted than
// the max.
//=============================================================================
void Params::store(char *name, void *value, int len)
{
  MyPrefs.begin("TwoWheeler", false);
  MyPrefs.putBytes(name, value, len);
  MyPrefs.end();
}


 //=============================================================================
 // Clear the flash
 //=============================================================================
 void Params::clearFlash()
 {
  // TODO:
 }

//=============================================================================
// Set and get the ssid.
// We allow the SSID to be up to 16 chars (+1 for the terminating null)
// newSSID is a string - it must be null terminated.
 //=============================================================================
 void Params::WIFI_SSID(const char *newSSID)
 {
  MyPrefs.putBytes(PARAM_SSID_KEY, newSSID, strlen(newSSID)+1 );
 }

 const char *Params::WIFI_SSID()
 {
  return(ssid);
 }


//=============================================================================
// Set and get the Wifi Password
// The password can be up to 32 characters long
//=============================================================================
 void Params::WIFI_PASS(const char *wifiPas)
 {

 }

 const char *Params::WIFI_PASS()
 {
  return(pass);
 }

 //=============================================================================
// Set and get the IPv4 port number. 
// This is 16 bit integer
//=============================================================================
 void Params::WIFI_PORT(uint16_t wifiport)
 {

 }

 uint16_t *Params::WIFI_PORT()
 {
  return(&port);
 }

//=============================================================================
// Set and get the SMAC Node Name.
// this can be up to 32 bytes
//=============================================================================
 void Params::NODE_NAME(const char *name)
 {

 }

 const char *Params::NODE_NAME()
 {
  return(nodeName);
 }
 
 //=============================================================================
 // Set and get the SMAC Node ID number
 // This is a single unsigned byte
 //=============================================================================
 void Params::NODE_ID_NO(uint8_t val)
 {

 }

 uint8_t Params::NODE_ID_NO()
 {
  return(nodeId);
 }

//=============================================================================
// Set and get the MAC address of the SMAC Relay device.
// This is always MAC_SIZE (i.e.: 6) Octets (binary data, NOT text)
//=============================================================================
 void Params::NODE_RELAY_MAC(uint8_t *val)
 {

 }

 uint8_t *Params::NODE_RELAY_MAC()
 {
  return(relayMacAddr);
 }


