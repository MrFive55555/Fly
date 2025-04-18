#include "MPU6050.h"
/**
 * cal mpu6050 data
 */
#define CAL_COUNT_MAX 400
#define ACC_NORM_1G 16384                         // ±2g 模式下 1g = 16384 LSB
#define STATIC_G_NORM_SQ_MIN (ACC_NORM_1G * 0.9f) // 0.9g
#define STATIC_G_NORM_SQ_MAX (ACC_NORM_1G * 1.3f) // 1.3g
/**
 * attitude angle calculation parameters
 */
#define DT 0.005f         // 每5ms更新一次
#define GYRO_SENS 131.0f  // ±250°/s，对应比例
#define ACC_SENS 16384.0f // ±2g，对应比例
#define ALPHA 0.98f       // 互补滤波系数
#define GYRO_STATIC_THRESH 0.5f
#define M_PI 3.1415926f
#define ANGLE_LIMIT 90.0f
#define RAD_TO_DEG (180.0f / M_PI)
#define DEG_TO_RAD (M_PI / 180.0f)
/**
 * function decalartion
 */
static void mpu6050_send_byte(uint8_t send_command, uint8_t send_data);
static uint8_t mpu6050_read_byte(uint8_t receive_command);
static void mpu6050_read_multi(uint8_t receive_command, uint8_t *buf, uint8_t len);
void mpu6050_init(void)
{
    iic_init();
    rt_thread_delay(10);                   // 10ms
    mpu6050_send_byte(PWR_MGMT_1, 0x00);   // really wake up MPU6050
    mpu6050_send_byte(SMPLRT_DIV, 0x04);   // sample rate 200Hz
    mpu6050_send_byte(CONFIG, 0x03);       // low passs filter 44 Hz
    mpu6050_send_byte(GYRO_CONFIG, 0x00);  // ±250 dps
    mpu6050_send_byte(ACCEL_CONFIG, 0x00); // ±2g
    mpu6050_send_byte(INT_PIN_CFG, 0x00);  // high level triggle
    mpu6050_send_byte(INT_ENABLE, 0x01);   // enable mpu6050_data ready interrupt
}
#if (USING_IIC_HARDWARE == 0)
static void mpu6050_send_byte(uint8_t send_command, uint8_t send_data)
{
    uint8_t ack;
    iic_start();
    iic_write_byte(SlaveAddress);
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    iic_write_byte(send_command);
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    iic_write_byte(send_data);
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    iic_stop();
}
static uint8_t mpu6050_data_read_byte(uint8_t receive_command)
{
    uint8_t receive_data, ack;
    iic_start();
    iic_write_byte(SlaveAddress);
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    iic_write_byte(receive_command);
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    iic_start();
    iic_write_byte(SlaveAddress + 1); // read
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    receive_data = iic_read_byte(0);
    iic_stop();
    return receive_data;
}
static void mpu6050_read_multi(uint8_t receive_command, uint8_t *buf, uint8_t len)
{
    uint8_t ack;
    iic_start();
    iic_write_byte(SlaveAddress);
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    iic_write_byte(receive_command);
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    iic_start();
    iic_write_byte(SlaveAddress + 1); // read
    ack = iic_wait_ack();
    if (ack)
        debug_iic = 1;
    while (len)
    {
        if (len == 1)
            buf[len - 1] = iic_read_byte(1);
        else
            buf[len - 1] = iic_read_byte(0);
        len--;
    }
    iic_stop();
}
void mpu6050_read_all(MPU6050_Data *mpu6050_data)
{
    uint8_t buf[14];
    mpu6050_data_read_multi(ACCEL_XOUT_H, buf, 14); // mpu6050_data support read 14 bytes at once
    for (int i = 0; i < 3; i++)
    {
        accel[i] = (int16_t)((buf[i * 2] << 8) | buf[i * 2 + 1]);
        gyro[i] = (int16_t)((buf[i * 2 + 8] << 8) | buf[i * 2 + 9]);
    }
}
#else
static void mpu6050_send_byte(uint8_t send_command, uint8_t send_data)
{
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, SlaveAddress, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;

    I2C_SendData(I2C1, send_command);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;

    I2C_SendData(I2C1, send_data);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;

    I2C_GenerateSTOP(I2C1, ENABLE);
}

static uint8_t mpu6050_read_byte(uint8_t receive_command)
{
    uint8_t receive_data = 0;

    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, SlaveAddress, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;

    I2C_SendData(I2C1, receive_command);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;

    I2C_GenerateSTART(I2C1, ENABLE); // 重启
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, SlaveAddress, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
        ;

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
        ;
    receive_data = I2C_ReceiveData(I2C1);

    I2C_GenerateSTOP(I2C1, ENABLE);

    return receive_data;
}
static void mpu6050_read_multi(uint8_t receive_command, uint8_t *buf, uint8_t len)
{
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, SlaveAddress, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;

    I2C_SendData(I2C1, receive_command);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;

    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, SlaveAddress, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
        ;

    while (len)
    {
        if (len == 1)
        {
            I2C_AcknowledgeConfig(I2C1, DISABLE); // 最后一个字节不要ACK
            I2C_GenerateSTOP(I2C1, ENABLE);
        }

        if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
        {
            *buf++ = I2C_ReceiveData(I2C1);
            len--;
        }
    }

    I2C_AcknowledgeConfig(I2C1, ENABLE); // 重新启用ACK
}

void mpu6050_read_all(MPU6050_Data *mpu6050_data)
{
    uint8_t buf[14];
    uint8_t is_static;
    float ax, ay, az;
    float g_norm_sq;
    mpu6050_read_multi(0x3B, buf, 14); // 连续读取加速度 + 陀螺仪，共14字节

    // 读取原始数据
    for (int i = 0; i < 3; i++)
    {
        mpu6050_data->accl[i] = (int16_t)((buf[i * 2] << 8) | buf[i * 2 + 1]);
        mpu6050_data->gyro[i] = (int16_t)((buf[i * 2 + 8] << 8) | buf[i * 2 + 9]);
    }

    // 用 int64_t 防止溢出：g² = ax² + ay² + az²

    if (!mpu6050_data->bias_ok_flag)
    {
        ax = mpu6050_data->accl[0];
        ay = mpu6050_data->accl[1];
        az = mpu6050_data->accl[2];
        g_norm_sq = sqrt(ax * ax + ay * ay + az * az);
        // 是否处于静止（加速度模长平方在 ±10% 范围）
        is_static = (g_norm_sq >= STATIC_G_NORM_SQ_MIN && g_norm_sq <= STATIC_G_NORM_SQ_MAX);
        if (is_static)
        {
            if (mpu6050_data->cal_count == 0)
            {
                for (int i = 0; i < 3; i++)
                {
                    mpu6050_data->accl_sum[i] = 0;
                    mpu6050_data->gyro_sum[i] = 0;
                }
            }
            if (mpu6050_data->cal_count < CAL_COUNT_MAX)
            {
                for (int i = 0; i < 3; i++)
                {
                    mpu6050_data->accl_sum[i] += mpu6050_data->accl[i];
                    mpu6050_data->gyro_sum[i] += mpu6050_data->gyro[i];
                }
                mpu6050_data->cal_count++;
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    mpu6050_data->accl_bias[i] = mpu6050_data->accl_sum[i] / CAL_COUNT_MAX;
                    mpu6050_data->gyro_bias[i] = mpu6050_data->gyro_sum[i] / CAL_COUNT_MAX;
                }

                // 补偿重力加速度（Z 轴减去 +1g = 16384）
                mpu6050_data->accl_bias[2] -= ACC_NORM_1G;
                mpu6050_data->bias_ok_flag = 1;
            }
        }
    }
    else
    {

        for (int i = 0; i < 3; i++)
        {
            mpu6050_data->accl[i] -= mpu6050_data->accl_bias[i];
            mpu6050_data->gyro[i] -= mpu6050_data->gyro_bias[i];
        }
    }
}
#endif

void mpu6050_attitude_angle(MPU6050_Data *mpu6050_data)
{
    if (!mpu6050_data->bias_ok_flag)
        return;
    float accl_temp[3];
    float gyro_temp[2];
    // 1. 单位换算
    for (uint8_t i = 0; i < 3; i++)
    {
        // x y z
        accl_temp[i] = (float)mpu6050_data->accl[i] / ACC_SENS; // g/s
    }
    for (uint8_t i = 0; i < 2; i++)
    {
        // x y
        gyro_temp[i] = (float)mpu6050_data->gyro[i] / GYRO_SENS; // °/s
    }

    // 2. 加速度解算角度（单位：°）
    // accl 0pitch ,1roll
    mpu6050_data->attitude_angle[ACCEL_PITCH] = atan2(-accl_temp[X], sqrt(accl_temp[Y] * accl_temp[Y] + accl_temp[Z] * accl_temp[Z])) * 180.0f / M_PI;
    mpu6050_data->attitude_angle[ACCEL_ROLL] = atan2(accl_temp[Y], accl_temp[Z]) * 180.0f / M_PI;

    // 3. 陀螺仪积分（角速度 × 时间 = 角度）
    // gyro 2pitch , 3roll
    mpu6050_data->attitude_angle[GYRO_PITCH] += gyro_temp[Y] * DT; // notice:gyro pitch round y spin.
    mpu6050_data->attitude_angle[GYRO_ROLL] += gyro_temp[X] * DT;

    // 4. 互补滤波融合
    // final 4pitch , 5roll
    mpu6050_data->attitude_angle[PITCH] = ALPHA * mpu6050_data->attitude_angle[GYRO_PITCH] + (1 - ALPHA) * mpu6050_data->attitude_angle[ACCEL_PITCH];
    mpu6050_data->attitude_angle[ROLL] = ALPHA * mpu6050_data->attitude_angle[GYRO_ROLL] + (1 - ALPHA) * mpu6050_data->attitude_angle[ACCEL_ROLL];
    for (uint8_t i = 4; i < 6; i++)
    {
        if (mpu6050_data->attitude_angle[i] > ANGLE_LIMIT)
            mpu6050_data->attitude_angle[i] = ANGLE_LIMIT;
        else if (mpu6050_data->attitude_angle[i] < -ANGLE_LIMIT)
            mpu6050_data->attitude_angle[i] = -ANGLE_LIMIT;
    }
    if (fabs(gyro_temp[X]) < GYRO_STATIC_THRESH && fabs(gyro_temp[Y]) < GYRO_STATIC_THRESH)
    {
        mpu6050_data->attitude_angle[PITCH] = mpu6050_data->attitude_angle[ACCEL_PITCH];
        mpu6050_data->attitude_angle[ROLL] = mpu6050_data->attitude_angle[ACCEL_ROLL];
    }
}
