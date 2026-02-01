//=============================================================================
//
//       FILE : ThisNode.h
//
//    PROJECT : SMAC Framework - Example 1
//
//     AUTHOR : Bill Daniels
//              Copyright 2025-2026, D+S Tech Labs, Inc.
//              All Rights Reserved
//
//=============================================================================

//--- Includes --------------------------------------------

#include "Node.h"

//--- Defines ---------------------------------------------

// Built-in LED on your board
//----------------------------
// #define USE_MONO_LED   // Use this for boards with mono color leds
#define USE_RGB_LED       // Use this for boards with RGB leds

//--- Status LED ---
#if defined USE_MONO_LED && defined USE_RGB_LED
#error "ERROR: Only one of USE_MONO_LED or USE_RGB_LED may be defined"
#endif

#if (defined(USE_MONO_LED))
#define STATUS_LED_PIN         LED_BUILTIN
#define STATUS_LED_BAD         (digitalWrite (STATUS_LED_PIN, LOW))
#define STATUS_LED_GOOD        (digitalWrite (STATUS_LED_PIN, HIGH))

#elif (defined(USE_RGB_LED))
#define STATUS_LED_PIN         38  // GPIO-48 for v1.0 boards, GPIO-38 for v1.1 boards
#define STATUS_LED_BRIGHTNESS  20  // Not recommended above 64
#define STATUS_LED_BAD         (rgbLedWrite (STATUS_LED_PIN, STATUS_LED_BRIGHTNESS, 0, 0))
#define STATUS_LED_GOOD        (rgbLedWrite (STATUS_LED_PIN, 0, STATUS_LED_BRIGHTNESS, 0))
#endif


//=========================================================
//  class ThisNode
//=========================================================

class ThisNode
{
  protected:
    bool   goodToGo = false;
    Node  *thisNode = nullptr;

  public:
    ThisNode ();

    Node  *GetNode  ();
    bool   GoodToGo ();
    void   AuxLoop  ();
};
