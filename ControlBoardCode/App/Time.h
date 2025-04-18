#ifndef __MEASURE_TIME_H__
#define __MEASURE_TIME_H__

#include "GlobalVar.h"
// 宏：开始计时
#define TIME_START()   uint32_t start = TIM_GetCounter(TIM2)
// 宏：结束计时
#define TIME_END()     uint32_t end   = TIM_GetCounter(TIM2)
// 宏：打印耗时（单位：us）
#define TIME_PRINT(time) do { \
     time = end >= start ? (end - start) : (0xFFFFFFFF - start + end); \
} while (0)
void time_init(uint16_t prescaler,uint16_t period);

#endif
