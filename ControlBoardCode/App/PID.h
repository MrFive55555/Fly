#ifndef PID_H
#define PID_H
#include "GlobalVar.h"
typedef struct
{
    float kp;
    float ki;
    float kd;

    float setpoint; // 目标值
    float integral;
    float last_error;

    float output_limit;   // 输出限幅（如 ±1000）
    float integral_limit; // 积分限幅
} PID_TypeDef;
void pid_init(PID_TypeDef *pid, float kp, float ki, float kd,
              float output_limit, float integral_limit);

float pid_update(PID_TypeDef *pid, float measurement, float dt);
#endif
