#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "rgb.h"
#include "motor.h"
#include "imu.h"
#include "madgwick.h"
#include "pid.h"

#define COLOR_SEQUENCE_SIZE      3U
#define PI                       (3.14159265358979F)
#define GYRO_MEASURE_ERROR       (PI * (40.0F / 180.0F))     /* this really should be measured, but estimated to 40deg/s -> omega_b on the original white paper */
                                                            /* technically this should be done by getting the mean bias of the gyro on every axis */
#define GRYE_DPS_TO_RPS(x, y, z) ((x + y + z) / 3.0F * PI / 180.0F) /* get mean error and convert to radians per second the gyro bias */
#define BETA(x)                  (sqrtf(3.0F / 4.0F) * (x)) /* compute beta for madgwick filter >w<!! */

#define DESIRED_ANGLE            (65.0F)
#define PID_KP                   (50.0F)
#define PID_KD                   (0.0F)
#define PID_KI                   (0.0F)

bool imu_data_ready = false;
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

    pid_init(&controller, PID_KP, PID_KD, PID_KI);

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
    // madgwick_init(&filter, BETA(GRYE_DPS_TO_RPS(imu.gxbias, imu.gybias, imu.gzbias)));
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
            for (uint8_t i = 0; i < 20; i++) /* iterate a fixed number of times */
            {
                now = esp_timer_get_time();
                deltat = (float)(now - filter.last_update) * 1000000.0F; /* calculate deltat and convert from us to s */
                filter.last_update = now;
                madgwick_update(&filter, (imu.gy*PI/180.0F), (imu.gx*PI/180.0F), -(imu.gz*PI/180.0F), imu.ay, imu.ax, -imu.az, deltat);
            }
        }
        madgwick_get_rpy(&filter);
        // ESP_LOGI("main", "Y: %03.2f,\tP: %03.2f,\tR: %03.2f", filter.yaw, filter.pitch, filter.roll);
        // ESP_LOGI("main", "AX: %03.2f,\tAY: %03.2f,\tAZ: %03.2f", imu.ax, imu.ay, imu.az);
        // ESP_LOGI("main", "GX: %03.2f,\tGY: %03.2f,\tGZ: %03.2f", imu.gx, imu.gy, imu.gz);

        /* controller */
        /* pid control to make pitch = ~65 degrees */
        now = esp_timer_get_time();
        deltat = (float)(now - controller.last_update) * 1000000.0F;
        controller.last_update = now;
        control_signal = pid_compute(&controller, DESIRED_ANGLE, filter.pitch, deltat);
        ESP_LOGI("main", "control_signal = %f", control_signal);

        if (control_signal > 0.0F)
        {
            duty_cycle = (uint8_t)(control_signal > (float)MAX_DUTY_CYCLE ? MAX_DUTY_CYCLE : control_signal);
            set_motor_pwm(0U, duty_cycle);
        }
        else
        {
            duty_cycle = (uint8_t)(-control_signal > (float)MAX_DUTY_CYCLE ? MAX_DUTY_CYCLE : -control_signal);
            set_motor_pwm(duty_cycle, 0U);
        }

        
        morph_tick(&morph);
    }
}