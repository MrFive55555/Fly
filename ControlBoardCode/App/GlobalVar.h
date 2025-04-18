#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H
#include "stm32f10x.h"
#include "rtthread.h"
/**
 * global extern flag declaration
 */
#ifdef GLOBAL_VAR
#define EXT_VAR
#else
#define EXT_VAR extern
#endif
/**
 * debug parameters
 */
EXT_VAR uint8_t debug_flag; // 0: no debug, 1: debug
/**
 * rtthread event flag
 */
EXT_VAR rt_event_t cal_event;
#define MPU6050_CAL_EVENT_FLAG 0x01
EXT_VAR rt_event_t motor_speed_event;
#define MOTOR_SPEDDD_EVENT_FLAG 0x01
EXT_VAR rt_event_t com_event;
#define COM_SEND_EVENT_FLAG 0x01
#define COM_RECEIVE_EVENT_FLAG 0x02
/**
 * uart parameters
 */
#define STRING_TYPE 1
#define CHAR_ARRAY_TYPE 0
#define BUFFER_SIZE 256
EXT_VAR char receive_buffer[BUFFER_SIZE];
EXT_VAR char send_buffer[BUFFER_SIZE];
// loop buffer
typedef struct
{
    uint16_t head_queue;
    uint16_t tail_queue;
    uint16_t data_count;
    uint8_t head_of_frame[4]; //head frame is 3 bit,but use more bit to judge
    uint8_t end_of_frame;
} Com_Data;
EXT_VAR Com_Data com_data;
// command parameters
typedef struct
{
    uint8_t take_off;
    uint8_t stop;
    uint8_t feed_back;
    uint8_t online;
    uint32_t online_time_out;
} Command;
EXT_VAR Command command;
/**
 * sensor data structure
 */
// MPU6050 data structure
typedef struct
{
    /**
     * MPU6050 data
     */
    int16_t accl[3];      // x, y, z axis acceleration data
    int16_t gyro[3];      // x, y, z axis gyroscope data
    int16_t accl_bias[3]; // zero bias of acceleration data
    int16_t gyro_bias[3]; // zero bias of gyroscope data
    int16_t temp;         // temperature data
    int16_t temp_bias;    // zero bias of temperature data
    /**
     * MPU6050 flag
     */
    int32_t accl_sum[3]; // 临时积累
    int32_t gyro_sum[3];
    uint16_t cal_count;
    uint8_t bias_ok_flag; // 0: no bias, 1: bias ok
    /**
     * attitude angle
     */
    float attitude_angle[6]; // accl_pitch,accl_roll,gryo_pitch,gryo_roll,pitch,roll
} MPU6050_Data;
typedef enum
{
    ACCEL_PITCH = 0,
    ACCEL_ROLL,
    GYRO_PITCH,
    GYRO_ROLL,
    PITCH,
    ROLL
} Angle_Name_Enum;
typedef enum
{
    X,
    Y,
    Z
} Coordinate_Enum;
// BMP280 data structure
typedef struct
{
    int32_t temp;
    int32_t press;
} BMP280_Data;
/**
 * enum declaration
 */
typedef enum
{
    MPU6050,
    BMP280,
    MPU6050_ATTITUDE_ANGLE,
    DEBUG,
    OK,
} Send_Data_Type;
typedef struct
{
    uint32_t cycle;
    float time;
} Debug_Data;

/**
 * function declaration
 */
void global_var_init(void);
/**
 * bit banding operation
 */
// 位带操作,实现51类似的GPIO控制功能
// 具体实现思想,参考<<CM3权威指南>>第五章(87页~92页).
// IO口操作宏定义
#define BITBAND(addr, bitnum) ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2))
#define MEM_ADDR(addr) *((volatile unsigned long *)(addr))
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum))
// IO口地址映射
#define GPIOA_ODR_Addr (GPIOA_BASE + 12) // 0x4001080C
#define GPIOB_ODR_Addr (GPIOB_BASE + 12) // 0x40010C0C
#define GPIOC_ODR_Addr (GPIOC_BASE + 12) // 0x4001100C
#define GPIOD_ODR_Addr (GPIOD_BASE + 12) // 0x4001140C
#define GPIOE_ODR_Addr (GPIOE_BASE + 12) // 0x4001180C
#define GPIOF_ODR_Addr (GPIOF_BASE + 12) // 0x40011A0C
#define GPIOG_ODR_Addr (GPIOG_BASE + 12) // 0x40011E0C

#define GPIOA_IDR_Addr (GPIOA_BASE + 8) // 0x40010808
#define GPIOB_IDR_Addr (GPIOB_BASE + 8) // 0x40010C08
#define GPIOC_IDR_Addr (GPIOC_BASE + 8) // 0x40011008
#define GPIOD_IDR_Addr (GPIOD_BASE + 8) // 0x40011408
#define GPIOE_IDR_Addr (GPIOE_BASE + 8) // 0x40011808
#define GPIOF_IDR_Addr (GPIOF_BASE + 8) // 0x40011A08
#define GPIOG_IDR_Addr (GPIOG_BASE + 8) // 0x40011E08

// IO口操作,只对单一的IO口!
// 确保n的值小于16!
#define PAout(n) BIT_ADDR(GPIOA_ODR_Addr, n) // 输出
#define PAin(n) BIT_ADDR(GPIOA_IDR_Addr, n)  // 输入

#define PBin(n) BIT_ADDR(GPIOB_IDR_Addr, n)  // 输入
#define PBout(n) BIT_ADDR(GPIOB_ODR_Addr, n) // 输出

#define PCout(n) BIT_ADDR(GPIOC_ODR_Addr, n) // 输出
#define PCin(n) BIT_ADDR(GPIOC_IDR_Addr, n)  // 输入

#define PDout(n) BIT_ADDR(GPIOD_ODR_Addr, n) // 输出
#define PDin(n) BIT_ADDR(GPIOD_IDR_Addr, n)  // 输入

#define PEout(n) BIT_ADDR(GPIOE_ODR_Addr, n) // 输出
#define PEin(n) BIT_ADDR(GPIOE_IDR_Addr, n)  // 输入

#define PFout(n) BIT_ADDR(GPIOF_ODR_Addr, n) // 输出
#define PFin(n) BIT_ADDR(GPIOF_IDR_Addr, n)  // 输入

#define PGout(n) BIT_ADDR(GPIOG_ODR_Addr, n) // 输出
#define PGin(n) BIT_ADDR(GPIOG_IDR_Addr, n)  // 输入

#endif
