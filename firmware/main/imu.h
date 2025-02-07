/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * imu.h - api and management of the inertial measurement unit onboard. i2c driver to initialize,
 * setup and custom config the imu, helper functions to get sensor readings, drift/bias estimation,
 * self-test, etc.
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

#ifndef _IMU_H
#define _IMU_H

#include <stdint.h>
#include <stdbool.h>

#define I2C_PORT_NUM                       0
#define I2C_MASTER_SCL_IO                  0                /* GPIO_NUM_0 */
#define I2C_MASTER_SDA_IO                  1                /* GPIO_NUM_1 */
#define IMU_INT1                           6                /* GPIO_NUM_6 */
#define I2C_MASTER_FREQ_HZ                 400000           /* 400 khz max freq for esp32c3 */
#define I2C_MASTER_TX_BUF_DISABLE          0
#define I2C_MASTER_RX_BUF_DISABLE          0
#define ICM42688_ADDR                      0x68             /* 0b1101000 (7-bit address) cause AP_AD0 = LOW */
#define ICM42688_ID                        0x47
#define WRITE_BIT                          I2C_MASTER_WRITE
#define READ_BIT                           I2C_MASTER_READ
#define ACK_CHECK_EN                       1

/* User Bank 0 */
#define ICM42688_DEVICE_CONFIG             0x11
#define ICM42688_DRIVE_CONFIG              0x13
#define ICM42688_INT_CONFIG                0x14
#define ICM42688_FIFO_CONFIG               0x16
#define ICM42688_TEMP_DATA1                0x1D
#define ICM42688_TEMP_DATA0                0x1E
#define ICM42688_ACCEL_DATA_X1             0x1F
#define ICM42688_ACCEL_DATA_X0             0x20
#define ICM42688_ACCEL_DATA_Y1             0x21
#define ICM42688_ACCEL_DATA_Y0             0x22
#define ICM42688_ACCEL_DATA_Z1             0x23
#define ICM42688_ACCEL_DATA_Z0             0x24
#define ICM42688_GYRO_DATA_X1              0x25
#define ICM42688_GYRO_DATA_X0              0x26
#define ICM42688_GYRO_DATA_Y1              0x27
#define ICM42688_GYRO_DATA_Y0              0x28
#define ICM42688_GYRO_DATA_Z1              0x29
#define ICM42688_GYRO_DATA_Z0              0x2A
#define ICM42688_TMST_FSYNCH               0x2B
#define ICM42688_TMST_FSYNCL               0x2C
#define ICM42688_INT_STATUS                0x2D
#define ICM42688_FIFO_COUNTH               0x2E
#define ICM42688_FIFO_COUNTL               0x2F
#define ICM42688_FIFO_DATA                 0x30
#define ICM42688_APEX_DATA0                0x31
#define ICM42688_APEX_DATA1                0x32
#define ICM42688_APEX_DATA2                0x33
#define ICM42688_APEX_DATA3                0x34
#define ICM42688_APEX_DATA4                0x35
#define ICM42688_APEX_DATA5                0x36
#define ICM42688_INT_STATUS2               0x37   
#define ICM42688_INT_STATUS3               0x38   
#define ICM42688_SIGNAL_PATH_RESET         0x4B
#define ICM42688_INTF_CONFIG0              0x4C
#define ICM42688_INTF_CONFIG1              0x4D
#define ICM42688_PWR_MGMT0                 0x4E
#define ICM42688_GYRO_CONFIG0              0x4F
#define ICM42688_ACCEL_CONFIG0             0x50
#define ICM42688_GYRO_CONFIG1              0x51
#define ICM42688_GYRO_ACCEL_CONFIG0        0x52
#define ICM42688_ACCEL_CONFIG1             0x53
#define ICM42688_TMST_CONFIG               0x54
#define ICM42688_APEX_CONFIG0              0x56
#define ICM42688_SMD_CONFIG                0x57
#define ICM42688_FIFO_CONFIG1              0x5F
#define ICM42688_FIFO_CONFIG2              0x60
#define ICM42688_FIFO_CONFIG3              0x61
#define ICM42688_FSYNC_CONFIG              0x62
#define ICM42688_INT_CONFIG0               0x63
#define ICM42688_INT_CONFIG1               0x64
#define ICM42688_INT_SOURCE0               0x65
#define ICM42688_INT_SOURCE1               0x66
#define ICM42688_INT_SOURCE3               0x68
#define ICM42688_INT_SOURCE4               0x69
#define ICM42688_FIFO_LOST_PKT0            0x6C
#define ICM42688_FIFO_LOST_PKT1            0x6D
#define ICM42688_SELF_TEST_CONFIG          0x70
#define ICM42688_WHO_AM_I                  0x75 /* should return 0x47 */
#define ICM42688_REG_BANK_SEL              0x76
/* User Bank 1 */
#define ICM42688_SENSOR_CONFIG0            0x03
#define ICM42688_GYRO_CONFIG_STATIC2       0x0B
#define ICM42688_GYRO_CONFIG_STATIC3       0x0C
#define ICM42688_GYRO_CONFIG_STATIC4       0x0D
#define ICM42688_GYRO_CONFIG_STATIC5       0x0E
#define ICM42688_GYRO_CONFIG_STATIC6       0x0F
#define ICM42688_GYRO_CONFIG_STATIC7       0x10
#define ICM42688_GYRO_CONFIG_STATIC8       0x11
#define ICM42688_GYRO_CONFIG_STATIC9       0x12
#define ICM42688_GYRO_CONFIG_STATIC10      0x13
#define ICM42688_XG_ST_DATA                0x5F
#define ICM42688_YG_ST_DATA                0x60
#define ICM42688_ZG_ST_DATA                0x61
#define ICM42688_TMSTAL0                   0x63
#define ICM42688_TMSTAL1                   0x64
#define ICM42688_TMSTAL2                   0x62
#define ICM42688_INTF_CONFIG4              0x7A
#define ICM42688_INTF_CONFIG5              0x7B
#define ICM42688_INTF_CONFIG6              0x7C
/* User Bank 2 */
#define ICM42688_ACCEL_CONFIG_STATIC2      0x03
#define ICM42688_ACCEL_CONFIG_STATIC3      0x04
#define ICM42688_ACCEL_CONFIG_STATIC4      0x05
#define ICM42688_XA_ST_DATA                0x3B
#define ICM42688_YA_ST_DATA                0x3C
#define ICM42688_ZA_ST_DATA                0x3D
/* User Bank 4 */
#define ICM42688_APEX_CONFIG1              0x40
#define ICM42688_APEX_CONFIG2              0x41
#define ICM42688_APEX_CONFIG3              0x42
#define ICM42688_APEX_CONFIG4              0x43
#define ICM42688_APEX_CONFIG5              0x44
#define ICM42688_APEX_CONFIG6              0x45
#define ICM42688_APEX_CONFIG7              0x46
#define ICM42688_APEX_CONFIG8              0x47
#define ICM42688_APEX_CONFIG9              0x48
#define ICM42688_ACCEL_WOM_X_THR           0x4A
#define ICM42688_ACCEL_WOM_Y_THR           0x4B
#define ICM42688_ACCEL_WOM_Z_THR           0x4C
#define ICM42688_INT_SOURCE6               0x4D
#define ICM42688_INT_SOURCE7               0x4E
#define ICM42688_INT_SOURCE8               0x4F
#define ICM42688_INT_SOURCE9               0x50
#define ICM42688_INT_SOURCE10              0x51
#define ICM42688_OFFSET_USER0              0x77
#define ICM42688_OFFSET_USER1              0x78
#define ICM42688_OFFSET_USER2              0x79
#define ICM42688_OFFSET_USER3              0x7A
#define ICM42688_OFFSET_USER4              0x7B
#define ICM42688_OFFSET_USER5              0x7C
#define ICM42688_OFFSET_USER6              0x7D
#define ICM42688_OFFSET_USER7              0x7E
#define ICM42688_OFFSET_USER8              0x7F

/* accelerometer scale */
#define AFS_2G                             0x03
#define AFS_4G                             0x02
#define AFS_8G                             0x01
#define AFS_16G                            0x00 /* default */
/* gyroscope scale */
#define GFS_2000DPS                        0x00 /* default */
#define GFS_1000DPS                        0x01
#define GFS_500DPS                         0x02
#define GFS_250DPS                         0x03
#define GFS_125DPS                         0x04
#define GFS_62_50DPS                       0x05
#define GFS_31_25DPS                       0x06
#define GFS_15_625DPS                      0x07

/* accelerometer and gyroscope output data rate (sampling rate) */
/* Low Noise mode */
#define AODR_32kHz                         0x01   
#define AODR_16kHz                         0x02
#define AODR_8kHz                          0x03
#define AODR_4kHz                          0x04
#define AODR_2kHz                          0x05
#define AODR_1kHz                          0x06 /* default */
/* Low Noise or Low Power modes */
#define AODR_500Hz                         0x0F
#define AODR_200Hz                         0x07
#define AODR_100Hz                         0x08
#define AODR_50Hz                          0x09
#define AODR_25Hz                          0x0A
#define AODR_12_5Hz                        0x0B
/* Low Power mode */
#define AODR_6_25Hz                        0x0C  
#define AODR_3_125Hz                       0x0D
#define AODR_1_5625Hz                      0x0E

#define GODR_32kHz                         0x01   
#define GODR_16kHz                         0x02
#define GODR_8kHz                          0x03
#define GODR_4kHz                          0x04
#define GODR_2kHz                          0x05
#define GODR_1kHz                          0x06 /* default */
#define GODR_500Hz                         0x0F
#define GODR_200Hz                         0x07
#define GODR_100Hz                         0x08
#define GODR_50Hz                          0x09
#define GODR_25Hz                          0x0A
#define GODR_12_5Hz                        0x0B

/* power modes */
#define aMode_OFF                          0x01 /* accelerometer off*/
#define aMode_LP                           0x02 /* accelerometer low power mode */
#define aMode_LN                           0x03 /* accelerometer low nois mode */
#define gMode_OFF                          0x00 /* gyroscope off */
#define gMode_SBY                          0x01 /* gyroscope standby mode */
#define gMode_LN                           0x03 /* gyroscope low noise mode  */

typedef struct {
    float accel_resolution;
    float gyro_resolution;
    float axbias; /* these biases are the average drift over 128 samples, per axis */
    float aybias;
    float azbias;
    float gxbias;
    float gybias;
    float gzbias;
    float ax; /* accel and gyro readings */
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} IMU;

void init_i2c(void);
uint8_t imu_get_id(void);
void imu_init(IMU *imu, uint8_t accel_scale, uint8_t gyro_scale, uint8_t accel_odr, uint8_t gyro_odr, uint8_t accel_mode, uint8_t gyro_mode, bool clock_in);
void imu_calculate_bias(IMU *imu);
void imu_read(IMU *imu);

#endif /* _IMU_H */