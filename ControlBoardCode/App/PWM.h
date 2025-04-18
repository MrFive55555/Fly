#ifndef PWM_H
#define PWM_H
#include "GlobalVar.h"
#include "PID.h"
/**
 * PWM IO definition
 */
#define PWM_AFIO_CLK RCC_APB2Periph_AFIO
#define PWM_GPIO_CLK RCC_APB1Periph_TIM3
#define PWM_GPIO_1_2_CLK RCC_APB2Periph_GPIOA
#define PWM_GPIO_3_4_CLK RCC_APB2Periph_GPIOB
#define PWM_GPIO_1_2 GPIOA
#define PWM_GPIO_3_4 GPIOB
#define PWM1_Pin GPIO_Pin_6
#define PWM2_Pin GPIO_Pin_7
#define PWM3_Pin GPIO_Pin_0
#define PWM4_Pin GPIO_Pin_1
void pwm_init(uint16_t prescaler,uint16_t period,uint16_t pulse);
void TIM3_SetPWM(uint8_t channel, uint16_t duty);
void update_motor_speed(PID_TypeDef *pid_pitch, PID_TypeDef *pid_roll, MPU6050_Data *mpu6050_data);
#endif
