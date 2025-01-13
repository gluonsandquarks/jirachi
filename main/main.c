/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * main.c - main entry point. sets up the needed objects from the different apis, sets up
 * interrupts, calls to initialize peripherals, and overall functionality including the
 * main task which focuses on active control of the device
 * 
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 gluons.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "globals.h"
#include "rgb.h"
#include "motor.h"
#include "imu.h"
#include "madgwick.h"
#include "pid.h"
#include "ble.h"

#define COLOR_SEQUENCE_SIZE      3U
#define PI                       (3.14159265358979F)
#define GYRO_MEASURE_ERROR       (PI * (40.0F / 180.0F))    /* this really should be measured, but estimated to 40deg/s -> omega_b on the original white paper */
                                                            /* technically this should be done by getting the mean bias of the gyro on every axis */
#define BETA(x)                  (sqrtf(3.0F / 4.0F) * (x)) /* compute beta for madgwick filter >w<!! */

#define DESIRED_ANGLE            (-60.0F)
#define DEFAULT_PID_KP           (3500.0F)
#define DEFAULT_PID_KD           (63.0F)
#define DEFAULT_PID_KI           (10.0F)
#define MAX_DUTY_CYCLE           (200U) /* full power!!!! :P */

volatile bool imu_data_ready = false;
volatile bool control_active = false;
volatile float pid_kp = DEFAULT_PID_KP;
volatile float pid_kd = DEFAULT_PID_KD;
volatile float pid_ki = DEFAULT_PID_KI;

static void IRAM_ATTR imu_isr_handler()
{
    gpio_intr_disable(IMU_INT1);
    gpio_isr_handler_remove(IMU_INT1);
    imu_data_ready = true;
    gpio_isr_handler_add(IMU_INT1, imu_isr_handler, NULL);
    gpio_intr_enable(IMU_INT1);
}

void app_main(void)
{

    /* sadly esp32c3 is single core so we need to do this in a different thread rather than a different core */
    /* might have some impact on active control?? need to stress test the app i guess */
    xTaskCreate((TaskFunction_t)ble_task, "ble_task", 4096, NULL, 1, NULL); /* start BLE task */

    /* set up color sequences for rgb led */
    RGB setup_state = { .hex = SUNSET_ORANGE };
    RGB control_sequence[COLOR_SEQUENCE_SIZE] = { { .hex = HOT_PINK },  { .hex = SORA_BLUE  }, { .hex = KUROMI_PURPLE } };
    RGB error_sequence[COLOR_SEQUENCE_SIZE]   = { { .hex = PLAIN_RED }, { .hex = SOSO_BLACK }, { .hex = PLAIN_RED     } };
    Morph morph = { 0 }; /* lightshow morph/blender manager for addressable LED */
    IMU imu = { 0 };
    Madgwick filter = { 0 };
    PID controller = { 0 };
    float control_signal = 0.0F;
    float deltat = 0.0F;
    int64_t now = 0.0F;
    uint8_t duty_cycle = 0U;

    pid_init(&controller, pid_kp, pid_kd, pid_ki);

    /* initialize peripherals */
    init_rmt();
    set_led_rgb(setup_state); /* init state */
    init_pwm();
    init_i2c();
    vTaskDelay(1000U / portTICK_PERIOD_MS);

    setup_state.hex = EVA_GREEN;
    set_led_rgb(setup_state); /* imu config state */

    /* check imu presence */
    uint8_t imu_id = imu_get_id();
    if (imu_id != ICM42688_ID)
    { 
        ESP_LOGE("main", "Critical error: IMU not connected or bad response. IMU_ID from response: 0x%X, should be 0x%X.", imu_id, ICM42688_ID);
        morph_set_sequence(&morph, error_sequence, COLOR_SEQUENCE_SIZE, 2000);
        while(1) { morph_tick(&morph); } /* show error sequence on led */
    }
    /* initialize imu struct + basic device config */
    imu_init(&imu, AFS_2G, GFS_500DPS, AODR_200Hz, GODR_200Hz, aMode_LN, gMode_LN, false);
    /* not necessary but helps with accuracy, the bias calculation can be commented out */
    ESP_LOGI("main", "CALCULATING ACCELEROMETER AND GYROSCOPE BIAS");
    ESP_LOGI("main", "KEEP DEVICE FLAT AND STABLE RELATIVE TO ONE AXIS ONLY");
    vTaskDelay(1000U / portTICK_PERIOD_MS);
    imu_calculate_bias(&imu);
    madgwick_init(&filter, BETA(GYRO_MEASURE_ERROR));

    /* configure IMU_INT1 pin for data ready interrupts coming from imu */
    gpio_reset_pin(IMU_INT1);
    gpio_set_direction(IMU_INT1, GPIO_MODE_INPUT);
    gpio_pullup_dis(IMU_INT1);
    gpio_pulldown_en(IMU_INT1);
    gpio_set_intr_type(IMU_INT1, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0U);
    gpio_isr_handler_add(IMU_INT1, imu_isr_handler, NULL);
    gpio_intr_enable(IMU_INT1);

    /* set lightshow to signal user control is active */
    morph_set_sequence(&morph, control_sequence, COLOR_SEQUENCE_SIZE, 2000);

    while (1)
    {
        if (imu_data_ready)
        {
            imu_data_ready = false;
            imu_read(&imu); /* INT1 cleared on any read BTW :p */
            now = esp_timer_get_time();
            deltat = ((float)(now - filter.last_update)) / 1000000.0F; /* calculate deltat and convert from us to s */
            filter.last_update = now;
            /* inputs flipped and fixed signs given the actual orientation of the imu on the board */
            madgwick_update(&filter, (imu.gy*PI/180.0F), (imu.gx*PI/180.0F), -(imu.gz*PI/180.0F), imu.ay, imu.ax, -imu.az, deltat);
        }
        madgwick_get_rpy(&filter);
        ESP_LOGD("main", "R: %03.2f,\tP: %03.2f,\tY: %03.2f", filter.roll, filter.pitch, filter.yaw);

        /* controller */
        /* pid control to make pitch = ~-60 degrees */
        if (control_active)
        {
            pid_update_consts(&controller, pid_kp, pid_kd, pid_ki); /* update constants from the BLE service */
            now = esp_timer_get_time();
            deltat = ((float)(now - controller.last_update)) / 1000000.0F;
            controller.last_update = now;
            control_signal = pid_compute(&controller, DESIRED_ANGLE, filter.pitch, deltat);
            ESP_LOGD("main", "control_signal = %f", control_signal);

            if (control_signal > 0.0F)
            {
                duty_cycle = (uint8_t)(control_signal > (float)MAX_DUTY_CYCLE ? MAX_DUTY_CYCLE : control_signal);
                set_motor_pwm(duty_cycle, 0U);
            }
            else
            {
                duty_cycle = (uint8_t)(-control_signal > (float)MAX_DUTY_CYCLE ? MAX_DUTY_CYCLE : -control_signal);
                set_motor_pwm(0U, duty_cycle);
            }
        } else { set_motor_pwm(0U, 0U); }

        
        morph_tick(&morph);
    }
}