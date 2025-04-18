#ifndef EXTI_H
#define EXTI_H
#include "GlobalVar.h"
/**
 * EXTI IO interrupt definition
 */
#define EXTI_AFIO_CLK RCC_APB2Periph_AFIO
#define EXTI_GPIO_CLK RCC_APB2Periph_GPIOB
#define EXTI_GPIO GPIOB
#define EXTI_GPIO_PIN GPIO_Pin_5
#define EXTI_LINE EXTI_Line5
void exti_init(void);
void EXTI9_5_IRQHandler(void);
#endif
