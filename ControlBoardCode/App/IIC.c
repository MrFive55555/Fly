#include "IIC.h"
#if (USING_IIC_HARDWARE == 0)
#define WAIT_TIME 0xFF
#define SCL PBout(6)
#define SDA PBout(7)
#define ReadSDABit PBin(7)
#define SDAInMode()                                       \
    GPIO_InitTypeDef GPIO_InitStructure;                  \
    GPIO_InitStructure.GPIO_Pin = IIC_SDA_Pin;            \
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; \
    GPIO_Init(IIC_GPIO, &GPIO_InitStructure)
#define SDAOutMode()                                  \
    GPIO_InitTypeDef GPIO_InitStructure;              \
    GPIO_InitStructure.GPIO_Pin = IIC_SDA_Pin;        \
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  \
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; \
    GPIO_Init(IIC_GPIO, &GPIO_InitStructure)
#endif

/**
 * IIC hardware initialization
 */
#if (USING_IIC_HARDWARE == 1)
void iic_init(void)
{
    RCC_APB2PeriphClockCmd(IIC_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(IIC_PERIPHERAL_CLK, ENABLE);

    // 1.gpio init
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IIC_SCL_Pin | IIC_SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_GPIO, &GPIO_InitStructure);

    // 2.iic init
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000; // 100KHz
    I2C_Init(IIC_PERIPHERAL, &I2C_InitStructure);
    I2C_Cmd(IIC_PERIPHERAL, ENABLE); // 使能IIC外设
}
#else
/**
 * IIC software initialization
 */
void iic_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(IIC_GPIO_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = IIC_SCL_Pin | IIC_SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_GPIO, &GPIO_InitStructure);
    SCL = 0;
    SDA = 0;
}
// start signal
void iic_start(void)
{
    SDAOutMode();
    SDA = 1;
    SCL = 1;
    rt_hw_us_delay(5);
    SDA = 0;
    rt_hw_us_delay(5);
    SCL = 0;
}
// stop signal
void iic_stop(void)
{
    SDAOutMode();
    SCL = 0;
    SDA = 0;
    SCL = 1;
    rt_hw_us_delay(5);
    SDA = 1;
    rt_hw_us_delay(5);
}
// wait ack
uint8_t iic_wait_ack(void)
{
    uint16_t timeCount = 0;
    SDAInMode();
    SDA = 1; // release SDA
    rt_hw_us_delay(5);
    SCL = 1;
    rt_hw_us_delay(5);
    while (ReadSDABit)
    {
        if (++timeCount > WAIT_TIME) // notation: wait time can't be too long
        {
            iic_stop();
            return 1;
        }
    }
    SCL = 0;
    return 0;
}
void iic_ack(void)
{
    SDAOutMode();
    SDA = 0;
    rt_hw_us_delay(5);
    SCL = 1;
    rt_hw_us_delay(5);
    SCL = 0;
}
void iic_no_ack(void)
{
    SDAOutMode();
    SDA = 1;
    rt_hw_us_delay(5);
    SCL = 1;
    rt_hw_us_delay(5);
    SCL = 0;
}
// send bytes
void iic_write_byte(uint8_t data)
{
    uint8_t i;
    SDAOutMode();
    for (i = 0; i < 8; i++)
    {
        if (data & (0x80 >> i))
            SDA = 1;
        else
            SDA = 0;
        rt_hw_us_delay(5);
        SCL = 1;
        rt_hw_us_delay(5);
        SCL = 0;
    }
}
// receive bytes
uint8_t iic_read_byte(uint8_t ack)
{
    uint8_t i, read_data = 0x00;
    SDAInMode();
    for (i = 0; i < 8; i++)
    {
        read_data <<= 1;
        SCL = 1;
        rt_hw_us_delay(5);
        if (ReadSDABit)
            read_data++;
        SCL = 0;
        rt_hw_us_delay(5);
    }
    if (!ack)
        iic_ack();
    else
        iic_no_ack();
    return read_data; // corrected from 'data' to 'read_data'
}
#endif
