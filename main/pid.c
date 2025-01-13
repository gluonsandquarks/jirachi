/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * pid.c - controller api and management
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

#include "pid.h"

void pid_init(PID *controller, float kp, float kd, float ki)
{
    controller->integral = 0.0F;
    controller->kd = kd;
    controller->kp = kp;
    controller->ki = ki;
    controller->last_update = 0;
    controller->prev_err = 0.0F;
}

void pid_update_consts(PID *controller, float kp, float kd, float ki)
{
    controller->kp = kp;
    controller->kd = kd;
    controller->ki = ki;
}

float pid_compute(PID *controller, float set_point, float measured, float deltat)
{
    float error = set_point - measured;
    controller->integral += error * deltat;
    float derivative = (error - controller->prev_err) / deltat;
    float output = (controller->kp * error) + (controller->ki * controller->integral) + (controller->kd * derivative);
    controller->prev_err = error;

    return output;
}