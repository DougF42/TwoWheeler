/**
 * @file L298N.cpp
 * @author Doug Fajardo (doug.fajardo@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-06-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "digitalIO.h"

#include "L298N.h"

static const  char *TAG = "L298N_Driver:::";

// * * * * * * * * * * * * * * * * * * * * * * * * *
// * * * * * * * * * * * * * * * * * * * * * * * * *
// * * * * * * * * * * * * * * * * * * * * * * * * *
L298N::L298N()
{
    maxPwmValue = (1<<LCD_RES_BITS)-1;

    // Set up my event timer...
    const esp_timer_create_args_t oneshot_timer_args = {
        .callback = &oneShotCallback,
        /* argument specified here will be passed to timer callback function */
        .arg = (void *)this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "one-shot",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));


    // Set up LED(PWM) clocking
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE, // LEDC_LOW_SPEED_MODE
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .timer_num = LCD_TIMER_NO,   // WHICH TIMER?
        .freq_hz = LCD_PULSE_FREQ,
        .clk_cfg = LEDC_AUTO_CLK, 
    };
    ESP_ERROR_CHECK( ledc_timer_config(&ledc_timer) );

    nextAvailLCDChannel=LEDC_CHANNEL_0; // used to allocate next avail channel.

    // Zero out the motor array
    for (int mtr=0; mtr<NOOFMOTORS; mtr++)
    {
        // Use .chan == LEDC_CHANNEL_MAX to determine if not configured
        motorPins[mtr].chan =LEDC_CHANNEL_MAX;
        motorPins[mtr].ena = -1;
        motorPins[mtr].in1 = -1;
        motorPins[mtr].in2 = -1;
    }
 

}

/**
 * @brief Configure a motor
 *   This configures the motor AND sets up an LCD channel to
 * drive it.
 * 
 * @param mtr      - the motor number to configure
 * @param ena_pin  - the enable pin
 * @param in1_pin  - the in1 pin
 * @param in2_pin  - the in2 pin
 * @return true   - normal return
 * @return false  - can't re-define existing config!
 */
bool L298N::configureMotorPins(MOTOR_ID_t mtr, int ena_pin, int in1_pin, int in2_pin)
{
    if ((int)nextAvailLCDChannel > 7) {
        ESP_LOGE(TAG, " Error in configureMotorPins - Too many channels ");
        return(false); // too many channels!
    }

    if (motorPins[mtr].chan != LEDC_CHANNEL_MAX)
    {
        ESP_LOGE(TAG, " Error in configureMotorPins motors - motor already configured!");
        return(false);
    }

    // Set up pins for motor
    motorPins[mtr].ena  = ena_pin;
    DEF_gpio_Configure(ena_pin, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE);
    DEF_digitalWrite( ena_pin, LOW);

    motorPins[mtr].in1  = in1_pin;
    DEF_gpio_Configure(in1_pin, GPIO_MODE_OUTPUT,  GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE);
    DEF_digitalWrite( in1_pin, LOW);

    motorPins[mtr].in2  = in2_pin;
    DEF_gpio_Configure(in2_pin, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE);
    DEF_digitalWrite( in2_pin, LOW);

    // select and set up LCD channel        
    motorPins[mtr].chan = nextAvailLCDChannel;
    ledc_channel_t channel= (ledc_channel_t)((int)nextAvailLCDChannel+1);
    nextAvailLCDChannel = channel;
    ledc_channel_config_t ledc_channel =
        {
            .gpio_num   = motorPins[mtr].ena,
            .speed_mode = LEDC_HIGH_SPEED_MODE, //LEDC_LOW_SPEED_MODE 
            .channel    = motorPins[mtr].chan,
            .intr_type  = LEDC_INTR_DISABLE,
            .timer_sel  = LCD_TIMER_NO,
            .duty       = 0,
            .hpoint     = 0,
            .flags      = 0,
        };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel)); // 1st motor
    return(true);
}


/**
 * @brief Destroy the L298N::L298N object
 * 
 */
L298N::~L298N()
{
    stopAll();
}

/**
 * @brief Stop all motors
 * 
 */
void L298N::stopAll()
{
    for (int m = 0; m < (int)MOTOR_ID_MAX; m++)
    {
        stopMotor((MOTOR_ID_t)m);
    }
}

/**
 * @brief This flushes the command queue, and stops the motor.
 * 
 * @param mtr - the motor to stop. The motor direction is NOT
 * changed. If MOTOR_ID_MAX, then stop ALL motors
 */
void L298N::stopMotor(MOTOR_ID_t mtr)
{
    // Stop  the motor.
    ESP_ERROR_CHECK(esp_timer_stop(oneshot_timer));
 
    EVENT ev= *que[mtr].getFirst(); // copy the current op.
    if ((ev.motorAction != MOTOR_FWD) && (ev.motorAction != MOTOR_REV))
        ev.motorAction=MOTOR_FWD;

    que[mtr].flush();
    addToQue(mtr, ev.motorAction, 0, 1);
}


/**
 * @brief Process any possible timeing change
 *  This happens when:
 *     (1) an event is added
 *     (2) The alarm triggers, because an event completed.
 *  
 * NOTE: To avoid overlap problems, the one-shot timer MUST
 *       be stopped before this is called!
 * @param arg - pointer to the L298N instance
 */
void L298N::oneShotCallback(void *arg)
   {
    static const char *ONESHOT="---ONESHOT:";
    L298N *me = (L298N *) arg;
    ESP_LOGD(ONESHOT,"CALLBACK INVOKED");
    if (esp_timer_is_active(me->oneshot_timer))
    {
        ESP_LOGD(ONESHOT, "Stop timer");
        ESP_ERROR_CHECK(esp_timer_stop(me->oneshot_timer));
    }

    uint64_t now = esp_timer_get_time(); 
    uint64_t nxtEventTime = UINT_MAX;

    for (int mtr=0; mtr < NOOFMOTORS; mtr++)
    {
        if (me->motorPins[mtr].chan == LEDC_CHANNEL_MAX) 
        {
            ESP_LOGD(ONESHOT, "Motor %d skipped-not defined", mtr);
            continue; // this motor not defined
        }
        
        // Did we complete a task?
        EVENT *curEvent = me->que[mtr].getFirst();
        if ((curEvent!=nullptr) && (curEvent->isRunning) && (now >= curEvent->ragnorak))
        {  // Task is complete - delete it
            ESP_LOGD(ONESHOT, "Event complete---");
            curEvent->dump();
            me->que[mtr].popFirst();
            curEvent = me->que[mtr].getFirst(); // point to new head of q
        }

        if ((curEvent != nullptr) && (!curEvent->isRunning))
        { // Start this event
            ESP_LOGD(ONESHOT, "Start new event---");
            curEvent->ragnorak = now + curEvent->duration;
            me->setMotor((MOTOR_ID_t)mtr, curEvent);
            curEvent->isRunning = true;
        }
    
        if ((curEvent != nullptr) && (curEvent->isRunning) && (curEvent->ragnorak < nxtEventTime))
        {
            nxtEventTime = curEvent->ragnorak;
        }
    }

    if (nxtEventTime != UINT_MAX)
    { // start the one-shot
        ESP_LOGD(ONESHOT, "Re-start timer.  next Event in %llu ", nxtEventTime - now);
        ESP_ERROR_CHECK(esp_timer_start_periodic(me->oneshot_timer, nxtEventTime - now));
    }

    return;
   }



/**
 * @brief Add a motor action.
 * 
 * @param mtr   - motor number
 * @param act   - action requested
 * @param duration  - duration (microseconds)
 * @return true  - normal return
 * @return false - error: queue is full
 */
bool L298N::addToQue(MOTOR_ID_t mtr, MOTOR_ACTION_t act, int _pcntPwr, uint64_t duration)
{
    EVENT ev; // workspace to build an event
    ev.isRunning = false;
    ev.motorAction = act;
    ev.ragnorak   = 0UL;
    ev.pcntPower = _pcntPwr;
    ev.duration = duration;

    if (!que[mtr].pushToQue(&ev))
    {
       ESP_LOGE(TAG,"ERROR Queue full motor no %d ", (int)mtr);
        return(false);
    }
    
    if (esp_timer_is_active(oneshot_timer))
    {
        ESP_LOGD("ADDTOQUE:","STOP ACTIVE TIMER BEFORE INVOKING ONESHOT");
        ESP_ERROR_CHECK(esp_timer_stop(oneshot_timer));
    }
    oneShotCallback(this);
    return(true);
}



/**
 * @brief Convert Percentage power to the LCD driver setting
 * 
 * @param mtr        - which motor ?
 * @param pcntPower  - what is desired power setting?
 * @return uint32_t  - The value to use for the LCD driver
 */
uint32_t L298N::mapPcntToPwr(MOTOR_ID_t mtr,  uint32_t pcntPower)
{
    uint32_t res;
    res=mapl(pcntPower, 0, 100, 0, maxPwmValue);    
    return(res);
}


/**
 * @brief Set the motor to the right direction and power level
 *
 * @param mtr
 * @param act
 * @param pwrPcnt
 * @return true
 * @return false
 */
bool L298N::setMotor(MOTOR_ID_t mtr, EVENT *event)
{
    uint32_t pw;

    // Sanity check - motor is defined
    if (mtr >= MOTOR_ID_MAX)
        return (false);

    // Sanity Check - event is defined
    if ((event == nullptr)||(event->isRunning))
        return (false);

    if (event->motorAction == MOTOR_IDLE)
    { // IDLE - disable the driver (stop LCD)
        DEF_digitalWrite(motorPins[mtr].in1, LOW);
        DEF_digitalWrite(motorPins[mtr].in2, LOW);
        pw=0;
        ESP_ERROR_CHECK(ledc_stop(LEDC_HIGH_SPEED_MODE, motorPins[mtr].chan, 0));
    }

    else if (event->motorAction == MOTOR_FWD)
    {   // FWD
        DEF_digitalWrite(motorPins[mtr].in1, HIGH);
        DEF_digitalWrite(motorPins[mtr].in2, LOW);
        pw = mapPcntToPwr(mtr, event->pcntPower);

    } else  {
        DEF_digitalWrite(motorPins[mtr].in1, LOW);
        DEF_digitalWrite(motorPins[mtr].in2, HIGH);
        pw = mapPcntToPwr(mtr, event->pcntPower);
    }    
    
    ESP_LOGE("SETMOTOR:"," channel is %d. Duty is %ld", motorPins[mtr].chan, pw);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, motorPins[mtr].chan, pw));    
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, motorPins[mtr].chan));  

    return(true);
}