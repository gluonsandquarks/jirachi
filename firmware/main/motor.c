/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * motor.c - motor control api and management, sets up PWM channels too
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

#include "driver/ledc.h"
#include "esp_log.h"
#include "motor.h"

void init_pwm(void)
{
    /* configure PWM timer */
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_SPEED_MODE,
        .duty_resolution = PWM_DUTY_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK)
    {
        ESP_LOGE("init_pwm", "Timer config failed: %d", err);
    }

    /* configure PWM channel for IN1 */
    ledc_channel_config_t channel_in1 = {
        .gpio_num = IN1_GPIO_PIN,
        .speed_mode = LEDC_SPEED_MODE,
        .channel = LEDC_IN1_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0U, /* start with 0% duty cycle */
        .hpoint = 0U
    };

    err = ledc_channel_config(&channel_in1);
    if (err != ESP_OK)
    {
        ESP_LOGE("init_pwm", "IN1 channel config failed: %d", err);
    }

    /* configure PWM channel for IN2 */
    ledc_channel_config_t channel_in2 = {
        .gpio_num = IN2_GPIO_PIN,
        .speed_mode = LEDC_SPEED_MODE,
        .channel = LEDC_IN2_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0U, /* start with 0% duty cycle */
        .hpoint = 0U
    };

    err = ledc_channel_config(&channel_in2);
    if (err != ESP_OK)
    {
        ESP_LOGE("init_pwm", "IN2 channel config failed: %d", err);
    }
}

void set_motor_pwm(uint8_t duty_cycle_in1, uint8_t duty_cycle_in2)
{
    /* set duty cycle for IN1 */
    esp_err_t err = ledc_set_duty(LEDC_SPEED_MODE, LEDC_IN1_CHANNEL, (uint32_t)duty_cycle_in1);
    if (err == ESP_OK)
    {
        ledc_update_duty(LEDC_SPEED_MODE, LEDC_IN1_CHANNEL);
    }
    else
    {
        ESP_LOGE("set_motor_pwm", "Failed to set IN1 duty: %d", err);
    }

    /* set duty cycle for IN2 */
    err = ledc_set_duty(LEDC_SPEED_MODE, LEDC_IN2_CHANNEL, (uint32_t)duty_cycle_in2);
    if (err == ESP_OK)
    {
        ledc_update_duty(LEDC_SPEED_MODE, LEDC_IN2_CHANNEL);
    }
    else
    {
        ESP_LOGE("set_motor_pwm", "Failed to set IN2 duty: %d", err);
    }
}