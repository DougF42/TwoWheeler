/**
 * @file QuadEncoder.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-04-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "QuadEncoder.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_err.h"


/**
 * @brief 
 * 
 */
 QuadEncoder::QuadEncoder()
 {
    pina = GPIO_NUM_NC;
    pinb = GPIO_NUM_NC;
    position.store(0);
    last_state = AoffBoff;
 }

/**
 * @brief Construct a new Quad Encoder object
 * This attaches the appropriate interrupt, and
 * enables the input pullup resistor.
 * 
 * @param _pina - the pin number for the 'A' phase
 * @param _pinb  - the pin number for the 'B' phase
 */
void QuadEncoder::begin(gpio_num_t _pina, gpio_num_t _pinb)
{
    pina = _pina;
    pinb = _pinb;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1UL << pina) | (1UL << pinb), /*!< GPIO pin: set with bit mask, each bit maps to a GPIO */
        .mode = GPIO_MODE_INPUT,                     /*!< GPIO mode: set input/output mode                     */
        .pull_up_en = GPIO_PULLUP_ENABLE,            /*!< GPIO pull-up                                         */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,       /*!< GPIO pull-down                                       */
        .intr_type = GPIO_INTR_ANYEDGE               /*!< GPIO interrupt type -both positive and negative edge */
    };
    ESP_ERROR_CHECK ( gpio_install_isr_service(ESP_INTR_FLAG_EDGE | ESP_INTR_FLAG_LOWMED) );
    ESP_ERROR_CHECK ( gpio_config(&io_conf) );
    ESP_ERROR_CHECK ( gpio_isr_handler_add( pina, gpio_isr_handler, this) );
    ESP_ERROR_CHECK ( gpio_isr_handler_add( pinb, gpio_isr_handler, this) );
    ESP_ERROR_CHECK ( gpio_intr_enable( pina) );
    gpio_intr_enable( pinb);
}

QuadEncoder::~QuadEncoder()
{

}

/**
 * @brief This accepts interrupts from pina OR pinb, and updates the position
 * 
 */
void IRAM_ATTR QuadEncoder::gpio_isr_handler(void* arg)
{
   QuadEncoder *me = (QuadEncoder *)arg;
   bool valA = gpio_get_level(me->pina);
   bool valB = gpio_get_level(me->pinb);

   // based on the previous state, determine what state we
   // transitioned to, and incr/decr the position accordingly
   //
   // Note that with Quadrature encoding, only one of the two 
   // pins will have changed state!
   switch(me->last_state)
   {
    case(AoffBoff):
        if (valA) 
        {
            me->position++;
            me->last_state=AonBoff;
        } else // B is on
        {
            me->position--;
            me->last_state=AoffBon;
        }
        break;

        
    case(AonBoff):
        if (! valA)
        {
            me->position--;
            me->last_state=AoffBon;

        } else
        { // B is on
            me->position++;
            me->last_state=AonBon;
        }
        break;


    case(AoffBon):
        if (valA)
        {
            me->position--;
            me->last_state=AonBon;

        } else 
        {  //  valB is off
            me->position++;
            me->last_state=AoffBon;
        }
        break;


    case(AonBon ):
        if (! valA)
        {
            me->position--;
            me->last_state=AoffBon;

        } else 
        { // B is off
            me->position++;
            me->last_state=AonBoff;
        }
        break;
   }
   return;
}

/**
 * @brief Force the current position
 * 
 * @param pos 
 */
void QuadEncoder::forcePos(uint32_t pos)
{
    position.store(pos);
}

/**
 * @brief Get the Position object
 *    
 * @return uint32_t - the curent position
 */
uint32_t QuadEncoder::getPosition()
{
    return ( position.load());
}