/**
 * @file QuadDecoder.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-04-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "QuadDecoder.h"

QuadDecoder::QuadDecoder(int motor_index)
{
    pmdefs = & (MotorDefs::MotorTable[motor_index]);
    last_state = AoffBoff;
    position = 0;
    // TODO: CONFIGURE QUAD PINS FOR INPUT WITH INTERRUPT
    gpio_config_t pGpioConfig=
    {   .pin_bit_mask = (1ull << pmdefs->quad_pin_a) | (1ull<<pmdefs->quad_pin_b),
        .mode=GPIO_MODE_INPUT,               /*!< GPIO mode: set input/output mode  */
        .pull_up_en=GPIO_PULLUP_ENABLE,       /*!< GPIO pull-up                    */
        .pull_down_en=GPIO_PULLDOWN_DISABLE,   /*!< GPIO pull-down                 */
        .intr_type=GPIO_INTR_ANYEDGE;      /*!< GPIO interrupt type                */
    #if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        .hys_ctrl_mode=GPIO_HYS_SOFT_DISABLE;       /*!< GPIO hysteresis: hysteresis filter on slope input    */
    #endif
    ESP_ERROR_CHECK(gpio_config( &pGpioConfig) );
    ESP_ERROR_CHECK (gpio_install_isr_service(ESP_INTR_FLAG_LEVEL4) );
    gpio_isr_handler_add(pmdefs->quad_pin_a, ISR_handlePhaseA, this);
    gpio_isr_handler_add(pmdefs->quad_pin_b, ISR_handlePhaseB, this);

}


QuadDecoder::~QuadDecoder()
{
    gpio_uninstall_isr_service();
    gpio_reset_pin(pmdefs->quad_pin_a);
    gpio_reset_pin(pmdefs->quad_pin_b);

}

/**
 * @brief Phase A changed.
 *
 * @param arg - points to the current instance of QuadDecoder
 */
void QuadDecoder::ISR_handlePhaseA(void *arg)
{
    QuadDecoder *me = (QuadDecoder *)arg;
    switch(me->last_state)
    {
        case(AoffBoff):
            me->position++;
            me->last_state=AonBoff;
        break;

        case(AonBoff):
            me->position--;
            me->last_state=AoffBoff;
        break;

        case(AoffBon):
        me->position++;
        me->last_state=AonBon;
        break;

        case(AonBon):
        me->position--;
        me->last_state=AoffBon;
        break;
    }
}

/**
 * @brief Phase B changed
 * 
 * @param arg - points to the current instance of QuadDecoder
 */
void QuadDecoder::ISR_handlePhaseB(void *arg)
{
    QuadDecoder *me = (QuadDecoder *)arg;
    switch(me->last_state)
    {
        case(AoffBoff):
            me->position--;
            me->last_state=AoffBon;
        break;

        case(AonBoff):
            me->position++;
            me->last_state=AonBon;
        break;

        case(AoffBon):
        me->position++;
        me->last_state=AoffBoff;
        break;

        case(AonBon):
        me->position--;
        me->last_state=AonBoff;
        break;
    }

}
