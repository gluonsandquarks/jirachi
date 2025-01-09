#ifndef _MOTOR_H
#define _MOTOR_H

#define IN1_GPIO_PIN          GPIO_NUM_3
#define IN2_GPIO_PIN          GPIO_NUM_4
#define PWM_FREQUENCY         250000U          /* 250 kHz */
#define PWM_DUTY_RESOLUTION   LEDC_TIMER_8_BIT /* 8-bit resolution (0-255 duty cycle) */
#define MAX_DUTY_CYCLE        255U

#define LEDC_IN1_CHANNEL      LEDC_CHANNEL_0
#define LEDC_IN2_CHANNEL      LEDC_CHANNEL_1
#define LEDC_TIMER            LEDC_TIMER_0
#define LEDC_SPEED_MODE       LEDC_LOW_SPEED_MODE /* this is the only available speed mode for the esp32c3 */

void init_pwm(void);
void set_motor_pwm(uint8_t duty_cycle_in1, uint8_t duty_cycle_in2);

#endif /* _MOTOR_H */