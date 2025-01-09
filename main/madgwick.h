#ifndef _MADGWICK_H
#define _MADGWICK_H


typedef struct {
    float q1, q2, q3, q4;   /* quaternion components */
    float beta;             /* gain for gradient descent */
    float roll, pitch, yaw; /* in degrees */
    int64_t last_update;    /* last time filter was updated */
} Madgwick;

void madgwick_init(Madgwick *filter, float beta);
/* gx, gy, and gz need to be in radians per second, deltat needs to be in seconds */
void madgwick_update(Madgwick *filter, float gx, float gy, float gz, float ax, float ay, float az, float deltat);
/* get roll, pitch and yaw in degrees */
void madgwick_get_rpy(Madgwick *filter);
void madgwick_get_rpy_old(Madgwick *filter);

#endif /* _MADGWICK_H */