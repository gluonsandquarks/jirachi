#include <stdint.h>
#include <math.h>
#include "madgwick.h"

void madgwick_init(Madgwick *filter, float beta)
{
    filter->q1 = 1.0F;
    filter->q2 = 0.0F;
    filter->q3 = 0.0F;
    filter->q4 = 0.0F;
    filter->beta = beta;
    filter->roll = 0.0F;
    filter->pitch = 0.0F;
    filter->yaw = 0.0F;
    filter->last_update = 0;
}

void madgwick_update(Madgwick *filter, float gx, float gy, float gz, float ax, float ay, float az, float deltat)
{
    float norm         = 0.0F; /* vector norm */
    float q_dot_omega1 = 0.0F; /* quaternion derivative from gyroscope elements */
    float q_dot_omega2 = 0.0F;
    float q_dot_omega3 = 0.0F;
    float q_dot_omega4 = 0.0F;
    float f_1          = 0.0F; /* objective function elements */
    float f_2          = 0.0F;
    float f_3          = 0.0F;
    float J_11or24     = 0.0F; /* objective function Jacobian elements */
    float J_12or23     = 0.0F;
    float J_13or22     = 0.0F;
    float J_14or21     = 0.0F;
    float J_32         = 0.0F;
    float J_33         = 0.0F;
    float q_hat_dot1   = 0.0F; /* estimated direction of the gyroscope error */
    float q_hat_dot2   = 0.0F;
    float q_hat_dot3   = 0.0F;
    float q_hat_dot4   = 0.0F;

    /* auxiliary variables to avoid repeated calculations */
    float half_q1 = 0.5F * filter->q1;
    float half_q2 = 0.5F * filter->q2;
    float half_q3 = 0.5F * filter->q3;
    float half_q4 = 0.5F * filter->q4;
    float two_q1  = 2.0F * filter->q1;
    float two_q2  = 2.0F * filter->q2;
    float two_q3  = 2.0F * filter->q3;

    /* normalize the accelerometer measurement */
    norm = sqrtf(ax * ax + ay * ay + az * az);
    ax /= norm;
    ay /= norm;
    az /= norm;

    /* compute the objective and the Jacobian */
    f_1 = two_q2 * filter->q4 - two_q1 * filter->q3 - ax;
    f_2 = two_q1 * filter->q2 + two_q3 * filter->q4 - ay;
    f_3 = 1.0F - two_q2 * filter->q2 - two_q3 * filter->q3 - az;
    J_11or24 = two_q3;
    J_12or23 = 2.0F * filter->q4;
    J_13or22 = two_q1;
    J_14or21 = two_q2;
    J_32 = 2.0F * J_14or21;
    J_33 = 2.0F * J_11or24;

    /* compute the gradient (matrix multiplication) */
    q_hat_dot1 = J_14or21 * f_2 - J_11or24 * f_1;
    q_hat_dot2 = J_12or23 * f_1 + J_13or22 * f_2 - J_32 * f_3;
    q_hat_dot3 = J_12or23 * f_2 - J_33 * f_3 - J_13or22 * f_1;
    q_hat_dot4 = J_14or21 * f_1 + J_11or24 * f_2;

    /* normalize the gradient */
    norm = sqrtf(q_hat_dot1 * q_hat_dot1 + q_hat_dot2 * q_hat_dot2 + q_hat_dot3 * q_hat_dot3 + q_hat_dot4 * q_hat_dot4);
    q_hat_dot1 /= norm;
    q_hat_dot2 /= norm;
    q_hat_dot3 /= norm;
    q_hat_dot4 /= norm;

    /* compute the quaternion derivative measured by gyroscopes */
    q_dot_omega1 = -half_q2 * gx - half_q3 * gy - half_q4 * gz;
    q_dot_omega2 =  half_q1 * gx + half_q3 * gz - half_q4 * gy;
    q_dot_omega3 =  half_q1 * gy - half_q2 * gz + half_q4 * gx;
    q_dot_omega4 =  half_q1 * gz + half_q2 * gy - half_q3 * gx;

    /* compute then integrate the estimated quaternion derivative */
    filter->q1 += (q_dot_omega1 - (filter->beta * q_hat_dot1)) * deltat;
    filter->q2 += (q_dot_omega2 - (filter->beta * q_hat_dot2)) * deltat;
    filter->q3 += (q_dot_omega3 - (filter->beta * q_hat_dot3)) * deltat;
    filter->q4 += (q_dot_omega4 - (filter->beta * q_hat_dot4)) * deltat;

    /* normalize the quaternion */
    norm = sqrtf(filter->q1 * filter->q1 + filter->q2 * filter->q2 + filter->q3 * filter->q3 + filter->q4 * filter->q4);
    filter->q1 /= norm;
    filter->q2 /= norm;
    filter->q3 /= norm;
    filter->q4 /= norm;
}


void madgwick_get_rpy(Madgwick *filter)
{
    float a12 = 2.0f * (filter->q2 * filter->q3 + filter->q1 * filter->q4);
    float a22 = filter->q1 * filter->q1 + filter->q2 * filter->q2 - filter->q3 * filter->q3 - filter->q4 * filter->q4;
    float a31 = 2.0f * (filter->q1 * filter->q2 + filter->q3 * filter->q4);
    float a32 = 2.0f * (filter->q2 * filter->q4 - filter->q1 * filter->q3);
    float a33 = filter->q1 * filter->q1 - filter->q2 * filter->q2 - filter->q3 * filter->q3 + filter->q4 * filter->q4;

    filter->roll = atan2f(a31, a33) * 180.0F / 3.14159265358979F;
    filter->pitch = -asinf(a32) * 180.0F / 3.14159265358979F;
    filter->yaw = atan2f(a12, a22) * 180.0F / 3.14159265358979F;
    filter->yaw += 13.8F;
    if (filter->yaw < 0) { filter->yaw += 360.0F; }
}