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
#define USE_TRACKER_PIN GPIO_NUM_13

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
    int pina,pinb;
    uint8_t newState;
    QuadReader *me = (QuadReader *)arg;

#ifdef USE_TRACKER_PIN
    gpio_set_level(USE_TRACKER_PIN, 0);
#endif

     pina = gpio_get_level(me->quad_pin_a); // get the current value of pinA
     pinb = gpio_get_level(me->quad_pin_b);
     newState = (pina << 1) | pinb;

     // BEGIN CRITICAL
     switch (me->last_state)
     {

     case (AoffBoff):
         if (newState == AonBoff)
         {
             me->curPosition++;
             me->last_state = newState;
         }
         else if (newState == AoffBon)
         {
             me->curPosition--;
             me->last_state = newState;
             break;
         }

     case (AoffBon):
         if (newState == AoffBoff)
         {
             me->curPosition++;
             me->last_state = newState;
         }
         else if (newState == AonBon)
         {
             me->curPosition--;
             me->last_state = newState;
         }
         break;

     case (AonBoff):
         if (newState == AonBon)
         {
             me->curPosition++;
             me->last_state = newState;
         }
         else if (newState == AoffBoff)
         {
             me->curPosition--;
             me->last_state = newState;
         }
         break;

     case (AonBon):
         if (newState == AoffBon)
         {
             me->curPosition++;
             me->last_state = newState;
         }
         else if (newState == AonBoff)
         {
             me->curPosition--;
             me->last_state = newState;
         }
         break;
     }
     // END CRITICAL SECTION

#ifdef USE_TRACKER_PIN
     gpio_set_level(USE_TRACKER_PIN, 1);
#endif

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
     quad_pin_a = cfg->quad_pin_a;
     quad_pin_b = cfg->quad_pin_b;
     last_state = AoffBoff;

     // - - - - - -
     // CONFIGURE QUAD PINS FOR INPUT WITH INTERRUPT
     gpio_config_t pGpioConfig =
         {
             .pin_bit_mask = (1ull << quad_pin_a) | (1ull << quad_pin_b),
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
     ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_a, gpio_interupt_isr, this));
     ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_b, gpio_interupt_isr, this));
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
  * @brief: If enabled, print the current pulse count
  *  = = = = = = = = = = = = = = = = = = = =
  */
 ProcessStatus QuadReader::DoPeriodic()
 {

    sprintf(DataPacket.value, "QPOS|%lld", curPosition);
    return(SUCCESS_DATA);
 }

 /** = = = = = = = = = = = = = = = = = = = =
  * @brief: return the current position
  *  = = = = = = = = = = = = = = = = = = = =
  */
 pulse_t QuadReader::getPosition()
 {
     // TODO: LOCKS?
     return (curPosition);
 }

 /** = = = = = = = = = = = = = = = = = = = =
  * @brief: return the current position
  *  = = = = = = = = = = = = = = = = = = = =
  */
 void QuadReader::resetPosition()
 {
     curPosition = 0;
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