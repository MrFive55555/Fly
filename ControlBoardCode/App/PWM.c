#include "PWM.h"
/**
 * motor pwm parameters
 */
typedef struct
{
	float throttle;
	float pitch_out;
	float roll_out;

	int32_t motor_pwm[4]; // motor pwm value
} Motor_TypeDef;
static Motor_TypeDef motor = {
	.motor_pwm = {0, 0, 0, 0},
	.throttle = 600,
	.pitch_out = 0,
	.roll_out = 0,
};
static int32_t constrain(int32_t val, int32_t min, int32_t max);
void pwm_init(uint16_t prescaler, uint16_t period, uint16_t pulse)
{
	RCC_APB1PeriphClockCmd(PWM_GPIO_CLK | PWM_AFIO_CLK, ENABLE);		 // enable timer clock
	RCC_APB2PeriphClockCmd(PWM_GPIO_1_2_CLK | PWM_GPIO_3_4_CLK, ENABLE); // enable GPIO clock
	// 1.initialize GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = PWM1_Pin | PWM2_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(PWM_GPIO_1_2, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = PWM3_Pin | PWM4_Pin;
	GPIO_Init(PWM_GPIO_3_4, &GPIO_InitStructure);
	// 2.initialize TimeBase
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_Prescaler = (prescaler - 1);
	TIM_TimeBaseInitStructure.TIM_Period = (period - 1);
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
	// 3.intialize ouput compare
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // up mode,when cnt < cc, ouput High
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // output polarity
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_Pulse = pulse; // can be adjusted speed for motor
	// 4.initialize channel 1,2,3,4
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
	// 5.enable output
	TIM_ARRPreloadConfig(TIM3, ENABLE); // enable auto reload register
	TIM_Cmd(TIM3, ENABLE);				// enable timer
}
void TIM3_SetPWM(uint8_t channel, uint16_t duty)
{
	if (duty > 1800)
		duty = 1800;

	switch (channel)
	{
	case 0:
		TIM_SetCompare1(TIM3, duty);
		break;
	case 1:
		TIM_SetCompare2(TIM3, duty);
		break;
	case 2:
		TIM_SetCompare3(TIM3, duty);
		break;
	case 3:
		TIM_SetCompare4(TIM3, duty);
		break;
	}
}
void update_motor_speed(PID_TypeDef *pid_pitch, PID_TypeDef *pid_roll, MPU6050_Data *mpu6050_data)
{
	motor.pitch_out = pid_update(pid_pitch, mpu6050_data->attitude_angle[PITCH], 0.005f);
	motor.roll_out = pid_update(pid_roll, mpu6050_data->attitude_angle[ROLL], 0.005f);

	motor.motor_pwm[0] = motor.throttle + motor.pitch_out + motor.roll_out;
	motor.motor_pwm[1] = motor.throttle + motor.pitch_out - motor.roll_out;
	motor.motor_pwm[2] = motor.throttle - motor.pitch_out + motor.roll_out;
	motor.motor_pwm[3] = motor.throttle - motor.pitch_out - motor.roll_out;

	for (uint8_t i = 0; i < 4; i++)
	{
		motor.motor_pwm[i] = constrain(motor.motor_pwm[i], 100, 200); // constrain pwm value
	}

	// update duty cycle
	for (uint8_t i = 0; i < 4; i++)
	{
		TIM3_SetPWM(i, motor.motor_pwm[i]);
	}
}
static int32_t constrain(int32_t val, int32_t min, int32_t max)
{
	if (command.take_off && command.online)
	{
		if (val < min)
			return min;
		if (val > max)
			return max;
		return val;
	}
	else
	{
		return 0;
	}
}
