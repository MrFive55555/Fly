#ifndef IIC_H
#define IIC_H
#include "GlobalVar.h"   
/**
 * IIC IO definition by software 
 */
#define USING_IIC_HARDWARE 1
#define IIC_PERIPHERAL I2C1
#define IIC_PERIPHERAL_CLK RCC_APB1Periph_I2C1
#define IIC_AFIO_CLK RCC_APB2Periph_AFIO
#define IIC_GPIO_CLK RCC_APB2Periph_GPIOB
#define IIC_GPIO GPIOB  
#define IIC_SCL_Pin GPIO_Pin_6
#define IIC_SDA_Pin GPIO_Pin_7
void iic_init(void);

#if (USING_IIC_HARDWARE == 0)
void iic_start(void);
void iic_stop(void);
uint8_t iic_wait_ack(void);
void iic_ack(void);
void iic_no_ack(void);
void iic_write_byte(uint8_t dat);
uint8_t iic_read_byte(uint8_t ack);
#endif

#endif
