/**
 * @file UdpCmd.cpp
 * @author your name (you@domain.com)
 * @brief accept/process commands over UDP
 * @version 0.1
 * @date 2025-03-28
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "UdpCmd.h"
#include "config.h"

/**
 * @brief General - lets start a connection
 *    NOTE: This WAITS for a connection!
 * 
 * @param ssid - the ssid to connect to
 * @param pwd  - the network password
 */
void UdpCmd::connectToWiFi(const char *ssid, const char *pwd, bool waitFlag)
{
    Serial.println("Connecting to WiFi network");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pwd);
    
    while (waitFlag && (WiFi.status() != WL_CONNECTED) )
    {
        Serial.println("Waiting for connection...");
        uint32_t waitTime=1000 * 1000; // wait time millisecs->uSecs
        delayMicroseconds(waitTime);
    };
    myAddr=WiFi.localIP();
    mySubnet=WiFi.subnetMask();
    myGateway=WiFi.gatewayIP();

    Serial.print("Connected to WIFI: IP addr: "); Serial.print(myAddr);
    Serial.print("/"); Serial.print(mySubnet);
    Serial.print("  Gateway:"); Serial.println(myGateway);
    return;
}

UdpCmd::UdpCmd()
{
    connected=false;
}

UdpCmd::~UdpCmd()
{
    WiFi.disconnect(true);
}


/**
 * @brief connect to WiFi HUB, start listening
 * 
 * @return true 
 * @return false 
 */
void UdpCmd::begin(const char *ssid, const char *pass, bool waitFlag)
{
    // CONNECT TO WIFI HUB
    connectToWiFi(ssid, pass,  true);
    udp.begin(UDP_PORT);       // listen on IPANNY
    Serial.println("NOW LISTENING ON UDP PORT 23");
}

/**
 * @brief Stop listening
 * 
 */
void UdpCmd::end()
{
    udp.stop();
}


/**
 * @brief does this character represent an End Of line?
 *    Not used - we assume a packet is itself a complete command line.
 * @param ch 
 * @return true 
 * @return false 
 */
bool UdpCmd::isEndOfChar(char ch)
{
    return(true);
}

/**
 * @brief This should be called fairly frequently
 *    This receives a packet (if one is available) and processes it.
 * 
 * NOTE: We expect that a packet contains a complete command line.
 */
void UdpCmd::loop()
{
    
    int len=udp.parsePacket();
    if(len==0) return; // no packet is available
    len=udp.read(inBuf, UDP_INPUT_BUFFER_SIZE);
    if (len>0)
    {
        respondAddr = udp.remoteIP(); // Who to send a reply to
        respondPort = udp.remotePort();

        inBuf[len+1]='\0'; // so the Serial.print works properly
        Serial.print("UDP packet from:"); Serial.print(respondAddr);  Serial.print(":"); Serial.println(respondPort);
        addBlock((char *)inBuf, len); // process and execute the block as a complete command
    }
}


/**
 * @brief Add data to be sent to originator
 *    We look for a LF character to signify end-of-line.align.
 * If bufer is full, we send it as-is.
 * 
 * @return size_t 1 if okay, 0 if I/O error
 */
size_t UdpCmd::write(uint8_t ch) 
{
    if (! WiFi.isConnected()) return(0);

    outBuf[nextInoutBuf++]=ch;

    if ( (ch == '\n') || (nextInoutBuf >= UDP_OUTPUT_BUFFER_SIZE))
    {  // Send the packet
        outBuf[nextInoutBuf]='\0'; // so the 'print' works properly
        Serial.print("Send packet to :"); Serial.print(respondAddr);
        Serial.print(" Port: "); Serial.print(respondPort);        
        Serial.print("  Packet is:");    Serial.print((const char *)outBuf);
        udp.beginPacket(respondAddr, respondPort);
        udp.write(outBuf, nextInoutBuf+1);
        udp.endPacket();
        nextInoutBuf=0;
    }

    return(1);
}


/**
 * @brief Add data to be sent to originator
 *    We look for a LF character to signify end-of-line.align
 * 
 * @return size_t 
 */
size_t UdpCmd::write(const uint8_t *buffer, size_t len)
{
    for (size_t idx=0; idx<len; idx++)
    {
        write(buffer[idx]);
    }
    return(len);
}


/**
 * @brief Is there space in the buffer to write?
 * 
 * @return int 
 */
int UdpCmd::availableForWrite()
{
    int res=0;
    if (nextInoutBuf < UDP_OUTPUT_BUFFER_SIZE)
    res=UDP_OUTPUT_BUFFER_SIZE - nextInoutBuf;
    return(res);
}