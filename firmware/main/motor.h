/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * motor.h - motor control api and management, sets up PWM channels too
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

#ifndef _MOTOR_H
#define _MOTOR_H
#include <stdint.h>

#define IN1_GPIO_PIN          GPIO_NUM_3
#define IN2_GPIO_PIN          GPIO_NUM_4
#define PWM_FREQUENCY         250000U          /* 250 kHz */
#define PWM_DUTY_RESOLUTION   LEDC_TIMER_8_BIT /* 8-bit resolution (0-255 duty cycle) */

#define LEDC_IN1_CHANNEL      LEDC_CHANNEL_0
#define LEDC_IN2_CHANNEL      LEDC_CHANNEL_1
#define LEDC_TIMER            LEDC_TIMER_0
#define LEDC_SPEED_MODE       LEDC_LOW_SPEED_MODE /* this is the only available speed mode for the esp32c3 */

void init_pwm(void);
void set_motor_pwm(uint8_t duty_cycle_in1, uint8_t duty_cycle_in2);

#endif /* _MOTOR_H */