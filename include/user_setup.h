//
//
#ifndef user_setup_once
Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

//=======================================================
// This structure is used to define all parameters
//   for a given MOTOR (and the QUAD/L298 and PID classes)
MotorControl_config_t left_mtr_cfg =
    {
        .chnlNo = LEDC_CHANNEL_1,
        .ena_pin = MOTOR_1_EN,
        .dir_pin_a = MOTOR_1_DRIVE_A,
        .dir_pin_b = MOTOR_1_DRIVE_B,
        .quad_pin_a = MOTOR_1_QUAD_A,
        .quad_pin_b = MOTOR_1_QUAD_B,
        .kp = 0,
        .ki = 0,
        .kd = 0,
};

MotorControl_config_t right_mtr_cfg =
    {
        .chnlNo = LEDC_CHANNEL_2,
        .ena_pin = MOTOR_2_EN,
        .dir_pin_a = MOTOR_2_DRIVE_A,
        .dir_pin_b = MOTOR_2_DRIVE_B,
        .quad_pin_a = MOTOR_2_QUAD_A,
        .quad_pin_b = MOTOR_2_QUAD_B,
        .kp = 0,
        .ki = 0,
        .kd = 0,
};

// Dev 0 is the left QUAD device.

// Dev 1 is the left L298 device.
// Dev 2 is the left PID device.
// Dev 3 is the left MOTOR device.

// LEFT side
l_quad    ->setup(&left_mtr_cfg);
ThisNode  ->AddDevice(l_quad);       // device 0
l_ln298   ->setup(&left_mtr_cfg);
ThisNode  ->AddDevice(l_ln298);      // Device 1
l_pid     ->setup(&left_mtr_cfg, l_quad, l_ln298);
ThisNode  ->AddDevice(l_pid);        // Device 2
l_motor   ->setup(&left_mtr_cfg, l_quad, l_ln298, l_pid);
ThisNode  ->AddDevice(l_motor);      // Device 3

// RIGHT side
r_quad->setup(&right_mtr_cfg);
ThisNode->AddDevice(r_quad); // Device 4
r_ln298->setup(&right_mtr_cfg);
ThisNode->AddDevice(r_ln298); // Device 5
r_pid->setup(&right_mtr_cfg, r_quad, r_ln298);
ThisNode->AddDevice(r_pid); // Device 6
r_motor->setup(&right_mtr_cfg, r_quad, r_ln298, r_pid);
ThisNode->AddDevice(r_motor); // Device 7

// void setup(MotorControl_config_t *left_cfg, MotorControl_config_t *right_cfg); // Instantiate all the subtasks...
driver->setup(l_motor, r_motor);
ThisNode->AddDevice(driver); // Device 8

// CREATE Power Monitor device
Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
myIna3221Device = new DEV_INA3221("Power", I2C_INA3221_ADDR, &Wire);
ThisNode->AddDevice(myIna3221Device);

#endif
