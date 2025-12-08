//
//
#ifndef user_setup_once
  //=======================================================
  //Set up the DRIVER device
  //=======================================================
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

  // CREATE DRIVER device
  DEV_Driver * driver= new DEV_Driver("Driver", ThisNode);
  driver->setup(&left_mtr_cfg, &right_mtr_cfg);
  ThisNode->AddDevice(driver);


  #ifdef USE_INA3221
  // CREATE Power Monitor device
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    myIna3221Device = new DEV_INA3221("Power", I2C_INA3221_ADDR, ThisNode,  &Wire);
    ThisNode->AddDevice(myIna3221Device);
  #endif

#endif
