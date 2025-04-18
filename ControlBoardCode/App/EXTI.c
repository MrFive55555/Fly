#include "EXTI.h"
void exti_init(void)
{
    // 开启GPIOB和AFIO时钟
    RCC_APB2PeriphClockCmd(EXTI_GPIO_CLK | EXTI_AFIO_CLK, ENABLE);

    // 1.配置PB5为浮空输入（根据需求可选上拉/下拉）
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
    // GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;      // 上拉输入
    // GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD; // 下拉输入
    GPIO_Init(EXTI_GPIO, &GPIO_InitStruct);

    // 将PB5映射到EXTI5中断线
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource5);

    // 2.初始化EXTI5
    EXTI_InitTypeDef EXTI_InitStruct;
    EXTI_InitStruct.EXTI_Line = EXTI_LINE;              // 选择EXTI5
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;    // 中断模式（非事件）
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising; // 触发方式：上升沿
    // EXTI_Trigger_Falling 下降沿 | EXTI_Trigger_Rising_Falling 双边沿
    EXTI_InitStruct.EXTI_LineCmd = ENABLE; // 使能中断线
    EXTI_Init(&EXTI_InitStruct);

    // 3.配置中断优先级
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;        // EXTI5~9共享中断通道
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0; // 抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;        // 子优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;           // 使能中断
    NVIC_Init(&NVIC_InitStruct);
}

void EXTI9_5_IRQHandler(void)
{
    rt_interrupt_enter(); // 进入中断保护区
    if (EXTI_GetITStatus(EXTI_LINE) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_LINE);
        //debug_flag++; // 中断计数器加1
        rt_event_send(cal_event, MPU6050_CAL_EVENT_FLAG);
    }
    rt_interrupt_leave(); // 退出中断保护区
}
