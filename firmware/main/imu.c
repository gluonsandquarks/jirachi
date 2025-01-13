/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * imu.c - api and management of the inertial measurement unit onboard. i2c driver to initialize,
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

#include <math.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "imu.h"

void init_i2c(void)
{
    i2c_port_t i2c_master_port = (i2c_port_t)I2C_PORT_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;

    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) { ESP_LOGE("init_i2c", "I2C bus config failed: %d", err); }

    err = i2c_driver_install(i2c_master_port, conf.mode,
                             I2C_MASTER_RX_BUF_DISABLE,
                             I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err != ESP_OK) { ESP_LOGE("init_i2c", "Failed to install I2C driver: %d", err); }
}

static void set_register(uint8_t register_address, uint8_t set_value)
{
    register_address = register_address & 0x007F; /* clamp address range from 0 - 127 */

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    esp_err_t err = i2c_master_start(cmd);                                                    // send start bit
    if (err != ESP_OK) { ESP_LOGE("set_register", "i2c_master_start failed: %d", err); }

    err = i2c_master_write_byte(cmd, (ICM42688_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);         // IMU 7-bit address + write bit
    if (err != ESP_OK) { ESP_LOGE("set_register", "i2c_master_write_byte failed: %d", err); }

    err = i2c_master_write_byte(cmd, register_address, ACK_CHECK_EN);                         // target register
    if (err != ESP_OK) { ESP_LOGE("set_register", "i2c_master_write_byte failed: %d", err); }

    err = i2c_master_write_byte(cmd, set_value, ACK_CHECK_EN);                                // target value
    if (err != ESP_OK) { ESP_LOGE("set_register", "i2c_master_write_byte failed: %d", err); }

    err = i2c_master_stop(cmd);                                                               // send stop bit
    if (err != ESP_OK) { ESP_LOGE("set_register", "i2c_master_stop failed: %d", err); }

    err = i2c_master_cmd_begin((i2c_port_t)I2C_PORT_NUM, cmd, 1000U / portTICK_PERIOD_MS);     // send all queued commands
    if (err != ESP_OK) { ESP_LOGE("set_register", "i2c_master_cmd_begin failed: %d", err); }

    i2c_cmd_link_delete(cmd);
}

static uint8_t read_register(uint8_t register_address)
{
    uint8_t data = 0x00;
    register_address = register_address & 0x007F; /* clamp address range from 0 - 127 */

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    esp_err_t err = i2c_master_start(cmd);                                                     // send start bit
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_start failed: %d", err); }

    err = i2c_master_write_byte(cmd, (ICM42688_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);          // IMU 7-bit address + write bit
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_write_byte failed: %d", err); }

    err = i2c_master_write_byte(cmd, register_address, ACK_CHECK_EN);                          // register to be read
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_write_byte failed: %d", err); }

    err = i2c_master_start(cmd);                                                               // resend start bit
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_start failed: %d", err); }

    err = i2c_master_write_byte(cmd, (ICM42688_ADDR << 1) | READ_BIT, ACK_CHECK_EN);           // IMU 7-bit address + read bit
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_write_byte failed: %d", err); }

    err = i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);                                   // read into data buffer
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_read_byte failed: %d", err); }

    err = i2c_master_stop(cmd);                                                                // send stop bit
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_stop failed: %d", err); }

    err = i2c_master_cmd_begin((i2c_port_t)I2C_PORT_NUM, cmd, 1000U / portTICK_PERIOD_MS);      // send all queued commands
    if (err != ESP_OK) { ESP_LOGE("read_register", "i2c_master_cmd_begin failed: %d", err); }

    i2c_cmd_link_delete(cmd);

    return data;
}

static void set_accel_resolution(IMU *imu, uint8_t scale)
{
    switch(scale)
    {
        case AFS_2G:
            imu->accel_resolution = 2.0F / 32768.0F;
            break;
        case AFS_4G:
            imu->accel_resolution = 4.0F / 32768.0F;
            break;
        case AFS_8G:
            imu->accel_resolution = 8.0F / 32768.0F;
            break;
        case AFS_16G:
            imu->accel_resolution = 16.0F / 32768.0F;
            break;
    }
}

static void set_gyro_resolution(IMU *imu, uint8_t scale)
{
    switch(scale)
    {
        case GFS_15_625DPS:
            imu->gyro_resolution = 15.625F / 32768.0F;
            break;
        case GFS_31_25DPS:
            imu->gyro_resolution = 31.25F / 32768.0F;
            break;
        case GFS_62_50DPS:
            imu->gyro_resolution = 62.5F / 32768.0F;
            break;
        case GFS_125DPS:
            imu->gyro_resolution = 125.0F / 32768.0F;
            break;
        case GFS_250DPS:
            imu->gyro_resolution = 250.0F / 32768.0F;
            break;
        case GFS_500DPS:
            imu->gyro_resolution = 500.0F / 32768.0F;
            break;
        case GFS_1000DPS:
            imu->gyro_resolution = 1000.0F / 32768.0F;
            break;
        case GFS_2000DPS:
            imu->gyro_resolution = 2000.0F / 32768.0F;
            break;
    }
}

static void get_accel_data_into_buffer(int16_t *buffer)
{
    uint8_t raw_data = 0x00U;

    raw_data = read_register(ICM42688_ACCEL_DATA_X1);
    buffer[0] = (int16_t)raw_data << 8U;
    raw_data = read_register(ICM42688_ACCEL_DATA_X0);
    buffer[0] |= raw_data;
    raw_data = read_register(ICM42688_ACCEL_DATA_Y1);
    buffer[1] = (int16_t)raw_data << 8U;
    raw_data = read_register(ICM42688_ACCEL_DATA_Y0);
    buffer[1] |= raw_data;
    raw_data = read_register(ICM42688_ACCEL_DATA_Z1);
    buffer[2] = (int16_t)raw_data << 8U;
    raw_data = read_register(ICM42688_ACCEL_DATA_Z0);
    buffer[2] |= raw_data;
}

static void get_gyro_data_into_buffer(int16_t *buffer)
{
    uint8_t raw_data = 0x00U;

    raw_data = read_register(ICM42688_GYRO_DATA_X1);
    buffer[0] = (int16_t)raw_data << 8U;
    raw_data = read_register(ICM42688_GYRO_DATA_X0);
    buffer[0] |= raw_data;
    raw_data = read_register(ICM42688_GYRO_DATA_Y1);
    buffer[1] = (int16_t)raw_data << 8U;
    raw_data = read_register(ICM42688_GYRO_DATA_Y0);
    buffer[1] |= raw_data;
    raw_data = read_register(ICM42688_GYRO_DATA_Z1);
    buffer[2] = (int16_t)raw_data << 8U;
    raw_data = read_register(ICM42688_GYRO_DATA_Z0);
    buffer[2] |= raw_data;
}

static void imu_self_test(IMU *imu, uint8_t st_accel_scale, uint8_t st_gyro_scale)
{
    int16_t accel_nominal[3U] = { 0, 0, 0 }; /* { x, y, z } */
    int16_t gyro_nominal[3U] = { 0, 0, 0 };
    int16_t accel_self_test[3U] = { 0, 0, 0 };
    int16_t gyro_self_test[3U] = { 0, 0, 0 };
    int16_t accel_diff[3U] = { 0, 0, 0 };
    int16_t gyro_diff[3U] = { 0, 0, 0 };
    float ratio[6U] = { 0, 0, 0, 0, 0, 0 }; /* { ax, ay, az, gx, gy, gz } */
    int16_t st_manufacturer[6U] = { 0, 0, 0, 0, 0, 0 };

    ESP_LOGI("imu_self_test", "-----------------------");
    ESP_LOGI("imu_self_test", "START OF SELF TEST");

    set_accel_resolution(imu, st_accel_scale);
    set_gyro_resolution(imu, st_gyro_scale);

    set_register(ICM42688_REG_BANK_SEL, 0x00); /* go to register bank 0 */
    set_register(ICM42688_PWR_MGMT0, 0x0F); /* turn on accel and gyro in low noise mode */
    vTaskDelay(1U / portTICK_PERIOD_MS); /* as per the datasheet, wait at least 200us after turning on accel and gyro */

    set_register(ICM42688_ACCEL_CONFIG0, st_accel_scale << 5U | AODR_1kHz); /* FS = 2 */
    set_register(ICM42688_GYRO_CONFIG0, st_gyro_scale << 5U | GODR_1kHz); /* FS = 3 */
    set_register(ICM42688_GYRO_ACCEL_CONFIG0, 0x44); /* set gyro and accel bandwith to ODR/10 */
    vTaskDelay(100U / portTICK_PERIOD_MS); /* wait for readings to stabilize */

    get_accel_data_into_buffer(accel_nominal);
    get_gyro_data_into_buffer(gyro_nominal);

    set_register(ICM42688_SELF_TEST_CONFIG, 0x78); /* set accel self test */
    vTaskDelay(100U / portTICK_PERIOD_MS); /* let accel respond */
    get_accel_data_into_buffer(accel_self_test);

    set_register(ICM42688_SELF_TEST_CONFIG, 0x07); /* set gyro self test */
    vTaskDelay(100U / portTICK_PERIOD_MS); /* let gyro respond */
    get_gyro_data_into_buffer(gyro_self_test);

    set_register(ICM42688_SELF_TEST_CONFIG, 0x00); /* go back to normal mode */

    /* as per the datasheet: self-test response = reading with self-test enabled - reading without self-test enabled */
    accel_diff[0] = accel_self_test[0] - accel_nominal[0];
    if (accel_diff[0] < 0) { accel_diff[0] *= -1; } /* ensure difference is positive */
    accel_diff[1] = accel_self_test[1] - accel_nominal[1];
    if (accel_diff[1] < 0) { accel_diff[1] *= -1; }
    accel_diff[2] = accel_self_test[2] - accel_nominal[2];
    if (accel_diff[2] < 0) { accel_diff[2] *= -1; }

    gyro_diff[0] = gyro_self_test[0] - gyro_nominal[0];
    if (gyro_diff[0] < 0) { gyro_diff[0] *= -1; }
    gyro_diff[1] = gyro_self_test[1] - gyro_nominal[1];
    if (gyro_diff[1] < 0) { gyro_diff[1] *= -1; }
    gyro_diff[2] = gyro_self_test[2] - gyro_nominal[2];
    if (gyro_diff[2] < 0) { gyro_diff[2] *= -1; }

    set_register(ICM42688_REG_BANK_SEL, 0x01); /* go to register bank 1 */
    st_manufacturer[3] = read_register(ICM42688_XG_ST_DATA);
    st_manufacturer[4] = read_register(ICM42688_YG_ST_DATA);
    st_manufacturer[5] = read_register(ICM42688_ZG_ST_DATA);

    set_register(ICM42688_REG_BANK_SEL, 0x02); /* go to register bank 2 */
    st_manufacturer[0] = read_register(ICM42688_XA_ST_DATA);
    st_manufacturer[1] = read_register(ICM42688_YA_ST_DATA);
    st_manufacturer[2] = read_register(ICM42688_ZA_ST_DATA);

    /* calculate ratio between manufacturer self-test and user self-test */
    ratio[0] = ((float)accel_diff[0]) / (1310.0F * powf(1.01F, (float)st_manufacturer[0] - 1.0F) + 0.5F); 
    ratio[1] = ((float)accel_diff[1]) / (1310.0F * powf(1.01F, (float)st_manufacturer[1] - 1.0F) + 0.5F); 
    ratio[2] = ((float)accel_diff[2]) / (1310.0F * powf(1.01F, (float)st_manufacturer[2] - 1.0F) + 0.5F); 
    ratio[3] = ((float)gyro_diff[0]) / (2620.0F * powf(1.01F, (float)st_manufacturer[3] - 1.0F) + 0.5F); 
    ratio[4] = ((float)gyro_diff[1]) / (2620.0F * powf(1.01F, (float)st_manufacturer[4] - 1.0F) + 0.5F); 
    ratio[5] = ((float)gyro_diff[2]) / (2620.0F * powf(1.01F, (float)st_manufacturer[5] - 1.0F) + 0.5F); 

    set_register(ICM42688_REG_BANK_SEL, 0x00); /* go to register bank 0 */

    /* log output to console */
    ESP_LOGI("imu_self_test", "Accelerometer Self Test");
    ESP_LOGI("imu_self_test", "-----------------------");
    ESP_LOGI("imu_self_test", "Ax diff -> %.0f mg", ((float)accel_diff[0] * imu->accel_resolution * 1000.0F));
    ESP_LOGI("imu_self_test", "Ay diff -> %.0f mg", ((float)accel_diff[1] * imu->accel_resolution * 1000.0F));
    ESP_LOGI("imu_self_test", "Az diff -> %.0f mg", ((float)accel_diff[2] * imu->accel_resolution * 1000.0F));
    ESP_LOGI("imu_self_test", "^^^~~~~~~~~ Normal values between 50 and 1200 mg");
    ESP_LOGI("imu_self_test", "Ax ratio -> %.0f %%", (ratio[0] * 100.0F));
    ESP_LOGI("imu_self_test", "Ay ratio -> %.0f %%", (ratio[1] * 100.0F));
    ESP_LOGI("imu_self_test", "Az ratio -> %.0f %%", (ratio[2] * 100.0F));
    ESP_LOGI("imu_self_test", "^^^~~~~~~~~ Normal values between 50 and 150 %%");
    ESP_LOGI("imu_self_test", "");

    ESP_LOGI("imu_self_test", "Gyroscope Self Test");
    ESP_LOGI("imu_self_test", "-----------------------");
    ESP_LOGI("imu_self_test", "Gx diff -> %.0f dps", ((float)gyro_diff[0] * imu->gyro_resolution));
    ESP_LOGI("imu_self_test", "Gy diff -> %.0f dps", ((float)gyro_diff[1] * imu->gyro_resolution));
    ESP_LOGI("imu_self_test", "Gz diff -> %.0f dps", ((float)gyro_diff[2] * imu->gyro_resolution));
    ESP_LOGI("imu_self_test", "^^^~~~~~~~~ Normal values > 60 dps");
    ESP_LOGI("imu_self_test", "Gx ratio -> %.0f %%", (ratio[3] * 100.0F));
    ESP_LOGI("imu_self_test", "Gy ratio -> %.0f %%", (ratio[4] * 100.0F));
    ESP_LOGI("imu_self_test", "Gz ratio -> %.0f %%", (ratio[5] * 100.0F));
    ESP_LOGI("imu_self_test", "^^^~~~~~~~~ Normal values between 50 and 150 %%");
    ESP_LOGI("imu_self_test", "END OF SELF TEST");
    ESP_LOGI("imu_self_test", "-----------------------");

}

void imu_init(IMU *imu, uint8_t accel_scale, uint8_t gyro_scale, uint8_t accel_odr, uint8_t gyro_odr, uint8_t accel_mode, uint8_t gyro_mode, bool clock_in)
{
    imu->axbias = 0.0F;
    imu->aybias = 0.0F;
    imu->azbias = 0.0F;
    imu->gxbias = 0.0F;
    imu->gybias = 0.0F;
    imu->gzbias = 0.0F;
    imu->ax = 0.0F;
    imu->ay = 0.0F;
    imu->az = 0.0F;
    imu->gx = 0.0F;
    imu->gy = 0.0F;
    imu->gz = 0.0F;
    imu->accel_resolution = 0.0F;
    imu->gyro_resolution = 0.0F;

    set_register(ICM42688_REG_BANK_SEL, 0x00); /* go to register bank 0 */
    set_register(ICM42688_DEVICE_CONFIG, 0x01); /* set bit 0 to 1 to issue soft reset */
    vTaskDelay(1U / portTICK_PERIOD_MS); /* wait for registers to reset */

    /* default the self test with accel scale set to 4g and gyro scale set to 250 dps, not necessary but recommended, can be commented out */
    imu_self_test(imu, AFS_4G, GFS_250DPS); 

    /* update imu strut with scale resolutions */
    set_accel_resolution(imu, accel_scale);
    set_gyro_resolution(imu, gyro_scale);

    set_register(ICM42688_REG_BANK_SEL, 0x00); /* go to register bank 0 */
    set_register(ICM42688_PWR_MGMT0, gyro_mode << 2U | accel_mode); /* set desired accel and gyro modes */
    vTaskDelay(1U / portTICK_PERIOD_MS); /* wait for at least 200us according to datasheet */
    set_register(ICM42688_ACCEL_CONFIG0, accel_scale << 5U | accel_odr); /* set accel Full Scale (FS) and Output Data Rate (ODR) */
    set_register(ICM42688_GYRO_CONFIG0, gyro_scale << 5U | gyro_odr); /* set gyro FS and ODR */
    set_register(ICM42688_GYRO_ACCEL_CONFIG0, 0x44); /* set accel and gyro bandwith to ODR/10 */
    vTaskDelay(100U / portTICK_PERIOD_MS); /* wait for registers to stabilize */

    /* configure interrupt handling */
    set_register(ICM42688_INT_CONFIG, 0x18 | 0x03); /* push-pull, pulsed, active HIGH interrupts */
    uint8_t temp = read_register(ICM42688_INT_CONFIG1); /* read current interrupt config */
    set_register(ICM42688_INT_CONFIG1, temp & ~(0x10)); /* clear bit 4 to allow async interrupt reset (required for proper interrupt operation) */
    set_register(ICM42688_INT_SOURCE0, 0x08); /* route data ready interrupt to INT1 pin */

    /* use external clock source? */
    if (clock_in)
    {
        set_register(ICM42688_REG_BANK_SEL, 0x00); /* go to register bank 0 */
        set_register(ICM42688_INTF_CONFIG1, 0x95); /* enable RTC */
        set_register(ICM42688_REG_BANK_SEL, 0x01); /* go to register bank 1 */
        set_register(ICM42688_INTF_CONFIG5, 0x04); /* use CLKIN as clock source */
    }

    set_register(ICM42688_REG_BANK_SEL, 0x00); /* go to register bank 0 */
}

void imu_calculate_bias(IMU *imu)
{
    int16_t temp[3U] = { 0, 0, 0 }; /* x, y, z */
    int32_t sum[6U] = { 0, 0, 0, 0, 0, 0 }; /* ax, ay, az, gx, gy, gz */

    /* fetch accel and gyro readings over 128 samples and sum them */
    for (int i = 0; i < 128; i++)
    {
        get_accel_data_into_buffer(temp);
        sum[0U] += temp[0U];
        sum[1U] += temp[1U];
        sum[2U] += temp[2U];
        get_gyro_data_into_buffer(temp);
        sum[3U] += temp[0U];
        sum[4U] += temp[1U];
        sum[5U] += temp[2U];
        vTaskDelay(50U / portTICK_PERIOD_MS);
    }

    /* calculate average of the past readings */
    imu->axbias = ((float)sum[0] * imu->accel_resolution / 128.0F);
    imu->aybias = ((float)sum[1] * imu->accel_resolution / 128.0F);
    imu->azbias = ((float)sum[2] * imu->accel_resolution / 128.0F);
    imu->gxbias = ((float)sum[3] * imu->gyro_resolution / 128.0F);
    imu->gybias = ((float)sum[4] * imu->gyro_resolution / 128.0F);
    imu->gzbias = ((float)sum[5] * imu->gyro_resolution / 128.0F);

    /* remove gravity from the axis currently experiencing the gravitational acceleration */
    if (imu->axbias >  0.8F) { imu->axbias -= 1.0F; }
    if (imu->axbias < -0.8F) { imu->axbias += 1.0F; }
    if (imu->aybias >  0.8F) { imu->aybias -= 1.0F; }
    if (imu->aybias < -0.8F) { imu->aybias += 1.0F; }
    if (imu->azbias >  0.8F) { imu->azbias -= 1.0F; }
    if (imu->azbias < -0.8F) { imu->azbias += 1.0F; }

    /* log output to the console */
    ESP_LOGI("imu_calculate_bias", "-----------------------");
    ESP_LOGI("imu_calculate_bias", "-> Accelerometer Bias:");
    ESP_LOGI("imu_calculate_bias", "Ax Bias: %.2f g", imu->axbias);
    ESP_LOGI("imu_calculate_bias", "Ay Bias: %.2f g", imu->aybias);
    ESP_LOGI("imu_calculate_bias", "Az Bias: %.2f g", imu->azbias);
    ESP_LOGI("imu_calculate_bias", "-> Gyroscope Bias:");
    ESP_LOGI("imu_calculate_bias", "Gx Bias: %.2f dps", imu->gxbias);
    ESP_LOGI("imu_calculate_bias", "Gy Bias: %.2f dps", imu->gybias);
    ESP_LOGI("imu_calculate_bias", "Gz Bias: %.2f dps", imu->gzbias);
    ESP_LOGI("imu_calculate_bias", "-----------------------");
}

void imu_read(IMU *imu)
{
    int16_t temp[3U] = { 0, 0, 0 }; /* x, y, z */
    get_accel_data_into_buffer(temp);
    /* reading in g (g force) */
    imu->ax = ((float)temp[0] * imu->accel_resolution) - imu->axbias;
    imu->ay = ((float)temp[1] * imu->accel_resolution) - imu->aybias;
    imu->az = ((float)temp[2] * imu->accel_resolution) - imu->azbias;
    get_gyro_data_into_buffer(temp);
    /* reading in dps (degrees per second) */
    imu->gx = ((float)temp[0] * imu->gyro_resolution) - imu->gxbias;
    imu->gy = ((float)temp[1] * imu->gyro_resolution) - imu->gybias;
    imu->gz = ((float)temp[2] * imu->gyro_resolution) - imu->gzbias;
}

uint8_t imu_get_id(void)
{
    return read_register(ICM42688_WHO_AM_I);
}