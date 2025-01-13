/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * madgwick.h - api and management of the Madgwick filter used to perform sensor fusion and
 * attitude estimation in 3d space. manages the madgwick object and exposes helper functions
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

#ifndef _MADGWICK_H
#define _MADGWICK_H
#include <stdint.h>


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

#endif /* _MADGWICK_H */