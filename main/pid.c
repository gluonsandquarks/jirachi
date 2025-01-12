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