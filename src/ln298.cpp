/**
 * @file ln298.cpp
 * @author Doug Fajardo
 * @brief Drive one DC motor via an L298 full-wave bridge.
 * @version 0.5
 * @date 2025-04-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "ln298.h"
#include "esp_check.h"
#include "driver/gpio.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

LN298::LN298()
{

}

LN298::~LN298()
{
    
}

/**
 * @brief Configure the LN298 object
 *   Each motor requires 3 pins - phase A, phase B and enable.
 *   The channel number is the channel for the ESP32 fastclock driver. Is shoould be separate for different
 * motors. 
 *    
 * 
 * @param chnlNo - the LEDC channel number to use.
 */
void LN298::setupLN298(ledc_channel_t chnlNo, gpio_num_t _ena_pin, gpio_num_t _dir_pin_a, gpio_num_t _dir_pin_b)
{
    led_channel = chnlNo;
    ena_pin   = _ena_pin;
    dir_pin_a = _dir_pin_a;
    dir_pin_b = _dir_pin_b;

    gpio_config_t  pinOutputscfg=
    {
        .pin_bit_mask = (1ull<< ena_pin) | (1ull << dir_pin_a) | (1ull << dir_pin_b),
        .mode=GPIO_MODE_OUTPUT,               /*!< GPIO mode: set input/output mode                     */
        .pull_up_en=GPIO_PULLUP_DISABLE,       /*!< GPIO pull-up                                         */
        .pull_down_en=GPIO_PULLDOWN_DISABLE,   /*!< GPIO pull-down                                       */
        .intr_type=GPIO_INTR_DISABLE,      /*!< GPIO interrupt type                                  */
    #if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        .hys_ctrl_mode=GPIO_HYS_SOFT_DISABLE       /*!< GPIO hysteresis: hysteresis filter on slope input    */
    #endif
    };
    ESP_ERROR_CHECK(gpio_config(&pinOutputscfg)) ;
    gpio_set_level( ena_pin,   0);
    gpio_set_level( dir_pin_a, 0);
    gpio_set_level( dir_pin_b, 0);

    // configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // ***TODO!!!! INITIALIZE the LEDC Channel -
    // channel number is the same as the index of the MotorTable.
    ledc_channel_config_t  chnl_config=
    {
        .gpio_num   = ena_pin,
        .speed_mode = LEDC_MODE,
        .channel=led_channel,
        .intr_type=LEDC_INTR_DISABLE,     /*!< configure interrupt, Fade interrupt enable  or Fade interrupt disable */
        .timer_sel=(ledc_timer_t)chnlNo,  /*!< Select the timer source of channel (0 - LEDC_TIMER_MAX-1) */
        .duty = LEDC_DUTY,                /*!< LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)] */
        .hpoint=0,                        /*!< LEDC channel hpoint value, the range is [0, (2**duty_resolution)-1] */
        .sleep_mode=LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,   /*!< choose the desired behavior for the LEDC channel in Light-sleep */
        .flags= 0,
    } ;
    ESP_ERROR_CHECK(ledc_channel_config( &chnl_config));

}


/**
 * @brief Set the pulse width
 *   This is the main control for this motor, expressed as a percentage (0..+-100)
 *  the timer is 13 bits precision( 0..8192). Direction is set based on the sign of
 *  the percentage. (Negative percents for motor in reverse).
 * 
 * @param pcnt  percentage - 0 thru + or - 100
 * 
 */
void LN298::setPulseWidth(int pcnt)
{
    uint32_t duty = pcnt* (1ul <<LEDC_DUTY_RES);
    if (duty<0) 0-duty;
    setDirection(pcnt);
    ESP_ERROR_CHECK( ledc_set_duty(LEDC_MODE, led_channel, duty) );
}; // Set the pulse width (0..100)


/**
 * @brief Set the direction based on the sign of the argument.
 *    NOTE: Zero is treated as 'forward'
 * @param pcnt - positive for forward, negative for reverse.
 */
void LN298::setDirection(int pcnt)
{
    if (pcnt>=0)
    {  // forward
        gpio_set_level(dir_pin_a, true);
        gpio_set_level(dir_pin_b,false);
    } else { // reverse
        gpio_set_level(dir_pin_a, false);
        gpio_set_level(dir_pin_b,true);
    }
}


/**
 * @brief Configure the motor to drift
 *    The pulse width is 0 (not driving anything)
 *    and ena_a and ena_b are LOW (off).
 */
void LN298::drift()
{
    // configure DIR pins
    gpio_set_level(dir_pin_a, false);
    gpio_set_level(dir_pin_b,false);

    setPulseWidth(0);
}


/**
 * @brief Stop the motor
 *  This drives the motor to 'stop'. 
 * This is done by setting the pulse width to
 * 10% (very little drive), and the direction pins both high.
 * 
 * Stop rate: range 10..50 (10 to 50 %) indicating how hard to
 * try and stop).
 */
void LN298::stop(int stopRate)
{
    uint32_t pw=stopRate;
    if (stopRate<0) pw=10;
    if (stopRate>50) pw=50;

    setPulseWidth(pw);
    gpio_set_level(dir_pin_a, true);
    gpio_set_level(dir_pin_b, true);

} // stop this motor.


/**
 * @brief Stop the motor
 *  This drives the motor to 'stop'. 
 * This is done by setting the pulse width to
 * 60%, and the direction pins both high.
 * 
 * NOTE: This leaves the idle power to the motors 
 * high (60%). User should call 'stop' with
 * a lower rate after motion has stopped.
 */
void LN298::hardStop()
{
    setPulseWidth(60);
    gpio_set_level(dir_pin_a, true);
    gpio_set_level(dir_pin_b, true);

} // stop this motor.