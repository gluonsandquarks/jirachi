#ifndef _PID_H
#define _PID_H

typedef struct {
    float kp;
    float ki;
    float kd;
    float prev_err;
    float integral;
    int64_t last_update;
} PID;

void pid_init(PID *controller, float kp, float kd, float ki);
float pid_compute(PID *controller, float set_point, float measured, float deltat);

#endif /* _PID_H */