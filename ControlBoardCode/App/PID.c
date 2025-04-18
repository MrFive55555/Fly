#include "PID.h"
void pid_init(PID_TypeDef *pid, float kp, float ki, float kd,
              float output_limit, float integral_limit)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_limit = output_limit;
    pid->integral_limit = integral_limit;

    pid->setpoint = 0;
    pid->integral = 0;
    pid->last_error = 0;
}
float pid_update(PID_TypeDef* pid, float measurement, float dt){
    float error = pid->setpoint - measurement;
    pid->integral += error * dt;

    // 积分限幅
    if (pid->integral > pid->integral_limit) pid->integral = pid->integral_limit;
    if (pid->integral < -pid->integral_limit) pid->integral = -pid->integral_limit;

    float derivative = (error - pid->last_error) / dt;
    pid->last_error = error;

    float output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;

    // 输出限幅
    if (output > pid->output_limit) output = pid->output_limit;
    if (output < -pid->output_limit) output = -pid->output_limit;

    return output;
}
