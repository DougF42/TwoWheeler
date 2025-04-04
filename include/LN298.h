/**
 * @file LN298.h- drive the LN298 motor driver
 * @authorDoug Fajardo (doug.fajardo@gmail.com)
 * @brief use the ESP_32 dreiver and LN298 to drive DC motor speed and direction
 * @version 0.1
 * @date 2025-02-11
 * 
 * @copyright Copyright (c) 2025
 * 
 * TODO: Right now, control changes are instant - do we want to
 *       implement acceleration?
 * 
 * NOTE: Recommended to use ADC1, pins GPIO32 thru 39 for the PWM output!
 */

#ifndef L_N_2_9_8
#define L_N_2_9_8

#define NO_POWER_CHANGE -32767

class LN298
{
    private:
        int motorEnPin;  // this is what we will pulse
        int motorDirAPin;   // one of the bridge pins ln the LN298
        int motorDirBPin;   // one of the bridge pins ln the LN298
        int channelNo;      // What channel ?
        bool timerIsRunning;

    protected:
        typedef enum {M_COLD, M_DRIFT, M_STOP, M_FORWWARD, M_REVERSE} motorState_t;
        void forward();
        void reverse();
        motorState_t getMotorDir(int motor); // what is motor control state?

    public:
        LN298();
        ~LN298();
        bool begin(int ENA_pin, int PINA, int PNIB);
        void setPWM(int pwmRate); // note: UNITS TBD!
        void power(int left, int right); // set speeds for left and right wheels
        void drift(bool coldFlag=false); // allow machine to drift
        void stop();  // stop all motion (immediate)

};
#endif