/**
 * @file QuadRead.cpp 
 * @author Doug Fajardo
 * @brief  READ the current position from the QUAD devices
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <Arduino.h>
#include "QuadReader.h"
#include "esp_heap_caps.h"
#include "esp_log.h"


static const char *TAG="QuadReader";

// IF defined, then this pin is controlled by the ISR.
// (if not defined, then the relevant code is bypased)
// Using an oscilliscope on this pin, we can observe 
// important timing properties
#define USE_TRACKER_PIN GPIO_NUM_13

// This is used to detect if the ISR is nested
volatile bool inside_isr_flag=false;

// Do not alter - these defs represent phase A in bit 1,  phase B in bit 0.
#define AoffBoff 0
#define AoffBon  1
#define AonBoff  2
#define AonBon   3

//  = = = = = = = = = = = = = = = = = = = =
// Static definitions
bool QuadReader::isrAlreadyInstalled;

/** = = = = = = = = = = = = = = = = = = = =
 * Initialize a new instance of the class
 *  = = = = = = = = = = = = = = = = = = = =
 */
QuadReader::QuadReader(Node *_node, const char *InName) : DefDevice(_node, InName)
{
    myInfo = (quad_reader_info_t *)heap_caps_malloc(sizeof(quad_reader_info_t), MALLOC_CAP_IRAM_8BIT);
    myInfo->curPosition = 0;
    return;
}

/** = = = = = = = = = = = = = = = = = = = =
 * destroy this instance
 *  = = = = = = = = = = = = = = = = = = = =
 */
QuadReader::~QuadReader()
{
    return;
}


/** = = = = = = = = = = = = = = = = = = = =
 * @brief handle the interrupt
 *  = = = = = = = = = = = = = = = = = = = =
 */
void IRAM_ATTR gpio_interupt_isr(void *arg)
{
    int pina, pinb;
    uint8_t newState;
    quad_reader_info_t *me = (quad_reader_info_t *)arg;
    if (inside_isr_flag)
    {
        ESP_DRAM_LOGE(TAG, "****** ISR IS NESTED!!!!");
    };
    inside_isr_flag=true;

#ifdef USE_TRACKER_PIN
    gpio_set_level(USE_TRACKER_PIN, 0);
#endif

    pina = gpio_get_level(me->phaseApin); // get the current value of pinA
    pinb = gpio_get_level(me->phaseBpin);
    newState = (pina << 1) | pinb;      // the state is phaseA in bit 1, phaseB in bit0

    // IF a valid change occured, update the position. ignore invalid combinations
    switch (me->lastState)
    {
    case (AoffBoff):
        if (newState == AonBoff)
        {
            // me->curPosition += 1;
            std::atomic_fetch_add_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        else if (newState == AoffBon)
        {
            std::atomic_fetch_sub_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        break;

    case (AoffBon):
        if (newState == AoffBoff)
        {
            std::atomic_fetch_add_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        else if (newState == AonBon)
        {
            std::atomic_fetch_sub_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        break;

    case (AonBoff):
        if (newState == AonBon)
        {
            std::atomic_fetch_add_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        else if (newState == AoffBoff)
        {
            std::atomic_fetch_sub_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        break;

    case (AonBon):
        if (newState == AoffBon)
        {
            std::atomic_fetch_add_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        else if (newState == AonBoff)
        {
            std::atomic_fetch_sub_explicit(&me->curPosition, 1, std::memory_order_relaxed);
            me->lastState = newState;
        }
        break;
    }

#ifdef USE_TRACKER_PIN
     gpio_set_level(USE_TRACKER_PIN, 1);
#endif
    inside_isr_flag=false;
}


 /** = = = = = = = = = = = = = = = = = = = =
  * @brief configure and start listening to this quad
  *
  * @param cfg     - pointer to MotorControl_config_t structure defining this motor
  * @return true   - normal return
  * @return false  - an error was detected
  *  = = = = = = = = = = = = = = = = = = = =
  */
 bool QuadReader::setup(MotorControl_config_t *cfg)
 {
     myInfo->phaseApin = cfg->quad_pin_a;
     myInfo->phaseBpin = cfg->quad_pin_b;
     myInfo->curPosition   = 0L;
     myInfo->lastState = AoffBoff;

     // - - - - - -
     // CONFIGURE QUAD PINS FOR INPUT WITH INTERRUPT
     gpio_config_t pGpioConfig =
         {
             .pin_bit_mask = (1ull << myInfo->phaseApin) | (1ull << myInfo->phaseBpin),
             .mode = GPIO_MODE_INPUT,               /*!< GPIO mode: set input/output mode  */
             .pull_up_en = GPIO_PULLUP_ENABLE,      /*!< GPIO pull-up                    */
             .pull_down_en = GPIO_PULLDOWN_DISABLE, /*!< GPIO pull-down                 */
             .intr_type = GPIO_INTR_ANYEDGE,        /*!< GPIO interrupt type                */
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
             .hys_ctrl_mode = GPIO_HYS_SOFT_DISABLE, /*!< GPIO hysteresis: hysteresis filter on slope input    */
#endif
         };
     ESP_ERROR_CHECK(gpio_config(&pGpioConfig));

     if (!isrAlreadyInstalled)
     {
         isrAlreadyInstalled = true; // force the handler into IRAM
         ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_EDGE));
         Serial.println("GPIO ISR Service installed");
     }
     ESP_ERROR_CHECK(gpio_isr_handler_add(myInfo->phaseApin, gpio_interupt_isr, myInfo) );
     ESP_ERROR_CHECK(gpio_isr_handler_add(myInfo->phaseBpin, gpio_interupt_isr, myInfo) );
     Serial.println("... Quad ISR handlers added");

#ifdef USE_TRACKER_PIN
     gpio_config_t pTrackerCfg =
         {
             .pin_bit_mask = (1ull << USE_TRACKER_PIN),
             .mode = GPIO_MODE_OUTPUT,              /*!< GPIO mode: set input/output mode */
             .pull_up_en = GPIO_PULLUP_DISABLE,     /*!< GPIO pull-up                    */
             .pull_down_en = GPIO_PULLDOWN_DISABLE, /*!< GPIO pull-down                  */
             .intr_type = GPIO_INTR_DISABLE,        /*!< GPIO interrupt type             */
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
             .hys_ctrl_mode = GPIO_HYS_SOFT_DISABLE, /*!< GPIO hysteresis: hysteresis filter on slope input    */
#endif
         };
     ESP_ERROR_CHECK(gpio_config(&pTrackerCfg));
     ESP_ERROR_CHECK(gpio_set_level(USE_TRACKER_PIN, 1)); // set high
     gpio_dump_io_configuration(stdout, pTrackerCfg.pin_bit_mask | pGpioConfig.pin_bit_mask);
#else
     gpio_dump_io_configuration(stdout, pGpioConfig.pin_bit_mask);
#endif // End of USE_TRACKER_PIN
     return (true);
 }


 /** = = = = = = = = = = = = = = = = = = = =
  * @brief: If enabled, print the current pulse count and state
  *  = = = = = = = = = = = = = = = = = = = =
  */
 ProcessStatus QuadReader::DoPeriodic()
 {
    pulse_t pos   = std::atomic_load_explicit(&myInfo->curPosition, std::memory_order_relaxed);
    uint8_t state = std::atomic_load_explicit(&myInfo->lastState,   std::memory_order_relaxed);
    sprintf(DataPacket.value, "QPOS|%ld|%u", pos, state);
    return(SUCCESS_DATA);
 }

 /** = = = = = = = = = = = = = = = = = = = =
  * @brief: return the current position
  *  = = = = = = = = = = = = = = = = = = = =
  */
 pulse_t QuadReader::getPosition()
 {
     pulse_t pos = std::atomic_load_explicit(&myInfo->curPosition, std::memory_order_relaxed);
     return ( pos );
 }

 /** = = = = = = = = = = = = = = = = = = = =
  * @brief: return the current position
  *  = = = = = = = = = = = = = = = = = = = =
  */
 void QuadReader::resetPosition()
 {
     std::atomic_store_explicit(&myInfo->curPosition, 0, std::memory_order_relaxed);
 }

 /** = = = = = = = = = = = = = = = = = = = =
  * @brief: Custom command handler
  *  = = = = = = = = = = = = = = = = = = = =
  */
 // Override this method to handle custom commands
 ProcessStatus QuadReader::ExecuteCommand()
 {
    ProcessStatus retVal = NOT_HANDLED;
    // TBD:
    return(retVal);
 }