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
#include <Arduino.h>
#include "DEV_ln298.h"
#include "esp_check.h"
#include "driver/gpio.h"

volatile uint8_t DEV_LN298::timer_is_inited=0;

// Timer - samefor all channels. 
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

DEV_LN298::DEV_LN298(const char * Name) : DefDevice(Name)
{
    lastPcnt = 0;
    motorStatus=MOTOR_DIS;
    return;
}

DEV_LN298::~DEV_LN298()
{
    return;
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
// void LN298::setupLN298(ledc_channel_t chnlNo, gpio_num_t _ena_pin, gpio_num_t _dir_pin_a, gpio_num_t _dir_pin_b)
void DEV_LN298::setupLN298(MotorControl_config_t *cfg)
{
    led_channel = cfg->chnlNo;
    ena_pin   = cfg->ena_pin;
    dir_pin_a = cfg->dir_pin_a;
    dir_pin_b = cfg->dir_pin_b;

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
    motorStatus = MOTOR_DIS;

    // initialize PWM timer - timer is common to all channels - only do one time
    timer_is_inited=timer_is_inited+1;
    if (timer_is_inited == 1)
    {  
        timer_is_inited = true;
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_MODE,
            .duty_resolution = LEDC_DUTY_RES,
            .timer_num = LEDC_TIMER,
            .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
            .clk_cfg = LEDC_AUTO_CLK};
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    }

    // INITIALIZE the LEDC Channelfor this motor
    // channel number is the same as the index of the MotorTable.
    ledc_channel_config_t  chnl_config=
    {
        .gpio_num   = ena_pin,
        .speed_mode = LEDC_MODE,
        .channel=led_channel,
        .intr_type=LEDC_INTR_DISABLE,  /*!< configure interrupt, Fade interrupt enable  or Fade interrupt disable */
        .timer_sel=LEDC_TIMER,         /*!< Select the timer source of channel (0 - LEDC_TIMER_MAX-1) */
        .duty = LEDC_DUTY,             /*!< LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)] */
        .hpoint=0,                     /*!< LEDC channel hpoint value, the range is [0, (2**duty_resolution)-1] */
        .sleep_mode=LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,   /*!< choose the desired behavior for the LEDC channel in Light-sleep */
        .flags= 0,
    } ;
    ESP_ERROR_CHECK(ledc_channel_config( &chnl_config));
    periodicEnabled=false;
}

/**
 * @brief Handle commands:
 *  (1) SPWM | <percnt>
 *  (2) ENAB 
 *  (3) DISA
 * 
 * @return ProcessStatus 
 */
ProcessStatus DEV_LN298::ExecuteCommand()
{

    ProcessStatus retVal = SUCCESS_NODATA;
    retVal = Device::ExecuteCommand();
    if (retVal == NOT_HANDLED)
    {

        scanParam();

        if (isCommand("SPWM"))
        { 
            retVal=setPulseWidthCommand();
        }
        else if (isCommand("ENAB"))
        { // Enable
            retVal =enable(true);
        }

        else if (isCommand("DISA"))
        { // Disable
            retVal =disable(true);
        }
        else
        {
            sprintf(DataPacket.value, "EROR|LN298|Unknown command");
            retVal = FAIL_DATA;
        }
    }

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "OK|%d|%s", ledc_get_duty(LEDC_MODE, led_channel), (motorStatus = MOTOR_DIS) ? "DIS" : "ENA");
        retVal = SUCCESS_DATA;
    }

    return (retVal);
}

/**
 * @brief Periodic event - report status
 * 
 * @return ProcessStatus 
 */
 ProcessStatus DEV_LN298::DoPeriodic()
 {
        DataPacket.timestamp = millis();
        sprintf(DataPacket.value, "L298|%d|%s", lastPcnt, (motorStatus == MOTOR_DIS)?"DIS":"ENA");
        return (SUCCESS_DATA);
 }

/**
 * @brief process the SPWM  (set pulse width) command
 *   Format: SPWM <pulseWidt>
 *      <pulseWidth> is percentage -
 *                   positive for forward, negative is reverse
 */
ProcessStatus DEV_LN298::setPulseWidthCommand()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    int32_t val = 0;
    // set pulse width (autmatically enables driver)
    if (SUCCESS_NODATA == getInt32(0, &val, GetName()))
    {
        if ((val < -100) || (val > 100))
        {
            sprintf(DataPacket.value, "EROR|SPWM|%s|Value must be 0 +/- 100", GetName());
            retVal = FAIL_DATA;
        }

        else if (motorStatus == MOTOR_DIS)
        {
            sprintf(DataPacket.value, "EROR|SPWM|%s is not enabled", GetName());
            retVal = FAIL_DATA;
        }
    }

    if (retVal==SUCCESS_NODATA)
    {
        if (argCount == 1)
        {
            setPulseWidth((int)val);
        }
 
        sprintf(DataPacket.value, "OK|SPWM|Pulse width is %d", lastPcnt);
        retVal = SUCCESS_DATA;
    }
    DataPacket.timestamp = millis();
    return (retVal);
}

/**
 * @brief Set the pulse width
 *   This is the main control for this motor, expressed as a percentage (0..+/-100)
 *   The PWM timer is 13 bits precision( 0..8192). 
 *   Direction is set based on the sign of the percentage. (Negative 
 *       percents for motor in reverse).
 * 
 * @param pcnt  percentage - 0 thru + or - 100
 * @return true normally, false if the motorStatus is disabled
 */
bool DEV_LN298::setPulseWidth(int pcnt)
{
    if (motorStatus == MOTOR_DIS) 
    {
        return(false);
    }
    uint32_t duty;
    if (pcnt >  100) pcnt =  100;
    if (pcnt < -100) pcnt = -100;
    duty = map(abs(pcnt), 0, 100, 0, ((1<<LEDC_DUTY_RES)-1) );
    lastPcnt= pcnt;
    setDirection(pcnt);

    ESP_ERROR_CHECK( ledc_set_duty(LEDC_MODE, led_channel, duty) );
    ledc_update_duty(LEDC_MODE, led_channel);
    return(true);
}; 


/**
 * @brief INTERNAL:  Set the direction based on the sign of the pcnt argument.
 *    NOTE: Zero is treated as 'forward'. This has the affect of enabling the driver
 * @param pcnt - positive for forward, negative for reverse.
 */
void DEV_LN298::setDirection(int pcnt)
{
    ProcessStatus retVal=SUCCESS_NODATA;

    if (motorStatus == MOTOR_DIS) return;
    if (pcnt>=0)
    {  // forward
        // Serial.printf("Set Motor %d direction FORWARD\r\n", (int)led_channel);
        gpio_set_level(dir_pin_a, true);
        gpio_set_level(dir_pin_b,false);
        motorStatus = MOTOR_FWD;
    } else { // reverse
        // Serial.printf("Set Motor %d direction REVERSE\r\n", (int)led_channel);
        gpio_set_level(dir_pin_a, false);
        gpio_set_level(dir_pin_b,true);
        motorStatus = MOTOR_REV;
    }
}

/**
 * @brief Disable the motor driver
 *     This is shared as a SMAC command and (optionally) a
 * method call from an external function.
 * 
 * @param isRemoteCmd - if true, then we send an 'ok' message via SMAC.
 * @return ProcessStatus - SUCCESS_DATA if this is a remote command,
 *                         SUCCESS_NODATA if this is not a remote command
 */
ProcessStatus DEV_LN298::disable(bool isRemoteCmd)
{
    ProcessStatus retVal = SUCCESS_NODATA;
    setPulseWidth(0);
    gpio_set_level(dir_pin_a, false);
    gpio_set_level(dir_pin_b, false);
    gpio_set_level(ena_pin, false);
    motorStatus=MOTOR_DIS;
    ledc_stop(LEDC_MODE, led_channel, 0 );
    if (isRemoteCmd)
    {
        DataPacket.timestamp = millis();
        retVal = SUCCESS_DATA;
        sprintf(DataPacket.value, "OK|DISA|%s Disabled", GetName());    
    }
    return(retVal);
}


/**
 * @brief Enable the motor driver
 *    *     This is shared as a SMAC command and (optionally) a
 * method call from an external function.
 * 
 * @param isRemoteCmd  if true, then a response message is sent via SMAC.
 * @return ProcessStatus - SUCCESS_DATA if this is a remote command,
 *                         SUCCESS_NODATA if this is not a remote command
 */
ProcessStatus DEV_LN298::enable(bool isRemoteCmd)
{
    ProcessStatus retVal = SUCCESS_NODATA;

    gpio_set_level(ena_pin, true);
    setPulseWidth(0);
    motorStatus = MOTOR_IDLE;

    if (isRemoteCmd)
    {
        DataPacket.timestamp = millis();
        sprintf(DataPacket.value, "OK|ENAB|%s enabled", GetName());
        retVal = SUCCESS_DATA;
    }
    
    return (retVal);
}
