#include "App.h"
/**
 * create thread parameters
 */
static rt_thread_t create_tid = RT_NULL;
static void create_thread(void *parameter);
#define CREATE_STACK_SIZE 256
#define CREATE_PRIO 1
#define CREATE_TICK 20
/**
 * com thread
 */
static rt_thread_t com_tid = RT_NULL;
static void com_thread(void *parameter);
#define COM_STACK_SIZE 1024
#define COM_PRIO 2
#define COM_TICK 20
/**
 * control motor speed thread parameters
 */
static rt_thread_t motor_speed_tid = RT_NULL;
static void motor_speed_thread(void *parameter);
#define MOTOR_SPEED_STACK_SIZE 256
#define MOTOR_SPEED_PRIO 3
#define MOTOR_SPEED_TICK 20
/**
 * mpu6050 data cal thread parameters
 */
static rt_thread_t cal_tid = RT_NULL;
static void cal_thread(void *parameter);
#define CAL_STACK_SIZE 512
#define CAL_PRIO 4
#define CAL_TICK 20
/**
 * PID parameters
 */
PID_TypeDef pid_pitch, pid_roll;
/**
 * initialize variables
 */
#define TIM3_PRESCALER 2 // 36MZ
#define TIM3_PERIOD 1800 // 36MZ / 1800 = 20KHz
#define TIM3_DUTY_MAX 1800
#define TIM3_DUTY_MIN 0
MPU6050_Data mpu6050_data = {
    .accl = {0, 0, 0},
    .gyro = {0, 0, 0},
    .accl_bias = {0, 0, 0},
    .gyro_bias = {0, 0, 0},
    .accl_sum = {0, 0, 0},
    .gyro_sum = {0, 0, 0},
    .cal_count = 0,
    .temp = 0,
    .temp_bias = 0,
    .bias_ok_flag = 0,
    .attitude_angle = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
};
BMP280_Data bmp280_data = {
    .temp = 0,
    .press = 0,
};
Debug_Data debug_data = {
    .cycle = 0,
    .time = 0,
};
/**
 * application entry
 */
void start_app(void)
{
    /**
     * global variable initialization
     */
    global_var_init(); // must init global var before using it
    // time_init(72,0xFFFF); //1us
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); // 设置 NVIC 中断分组 1 (0位抢占优先级，4位响应优先级)
    usartInit(9600);
    pwm_init(TIM3_PRESCALER, TIM3_PERIOD, TIM3_DUTY_MIN); // 20KHz
    mpu6050_init();
    bmp280_init();
    DMA_Config(DMA1_Channel4, (uint32_t)&send_buffer[0], (uint32_t)&USART1->DR, 8);
    pid_init(&pid_pitch, 6.0f, 0.02f, 1.5f, 500, 300); // 可调参数
    pid_init(&pid_roll, 6.0f, 0.02f, 1.5f, 500, 300);
    /**
     * event group initialization
     */
    cal_event = rt_event_create("cal_event", RT_IPC_FLAG_PRIO);
    if (cal_event == RT_NULL)
    {
        // rt_kprintf("create cal_event failed.\r\n");
    }
    else
    {
        // rt_kprintf("create cal_event success.\r\n");
    }
    motor_speed_event = rt_event_create("motor_speed", RT_IPC_FLAG_PRIO);
    if (motor_speed_event == RT_NULL)
    {
    }
    else
    {
    }
    com_event = rt_event_create("com_event", RT_IPC_FLAG_PRIO);
    if (com_event == RT_NULL)
    {
    }
    else
    {
    }
    /**
     * thread initialization
     */
    create_tid = rt_thread_create("create",
                                  create_thread,
                                  RT_NULL,
                                  CREATE_STACK_SIZE,
                                  CREATE_PRIO,
                                  CREATE_TICK);
    if (create_tid != RT_NULL)
    {
        rt_thread_startup(create_tid);
    }
    else
    {
        // rt_kprintf("create_tid failed.\r\n");
    }
}
static void create_thread(void *parameter)
{
    com_tid = rt_thread_create("com", com_thread,
                               RT_NULL,
                               COM_STACK_SIZE,
                               COM_PRIO,
                               COM_TICK);
    if (com_tid != RT_NULL)
    {
        rt_thread_startup(com_tid);
    }
    motor_speed_tid = rt_thread_create("motor_speed",
                                       motor_speed_thread,
                                       RT_NULL,
                                       MOTOR_SPEED_STACK_SIZE,
                                       MOTOR_SPEED_PRIO,
                                       MOTOR_SPEED_TICK);
    if (motor_speed_tid != RT_NULL)
    {
        rt_thread_startup(motor_speed_tid);
    }
    cal_tid = rt_thread_create("mpu6050_cal",
                               cal_thread,
                               RT_NULL,
                               CAL_STACK_SIZE,
                               CAL_PRIO,
                               CAL_TICK);
    if (cal_tid != RT_NULL)
    {
        rt_thread_startup(cal_tid);
    }
    /**
     * initialize configuration
     */
    rt_thread_delete(create_tid);
}
static void com_thread(void *parameter)
{
    rt_err_t err;
    rt_uint32_t event;
    while (1)
    {
        err = rt_event_recv(com_event, COM_SEND_EVENT_FLAG | COM_RECEIVE_EVENT_FLAG, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
        if (err == RT_EOK)
        {
            if ((event & COM_SEND_EVENT_FLAG) == COM_SEND_EVENT_FLAG)
            {
                // rt_enter_critical();
                // send_data_add(MPU6050_ATTITUDE_ANGLE, &mpu6050_data);
                // send_data_add(BMP280, &bmp280_data);
                // send_data_add(OK, NULL);
                // rt_critical_level();
            }
            if ((event & COM_RECEIVE_EVENT_FLAG) == COM_RECEIVE_EVENT_FLAG)
            {
                command_parse();
            }
        }
    }
}
static void motor_speed_thread(void *parameter)
{
    rt_err_t err;
    rt_uint32_t event;
    while (1)
    {
        err = rt_event_recv(motor_speed_event, MOTOR_SPEDDD_EVENT_FLAG, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
        if (err == RT_EOK)
        {
            if ((event & MOTOR_SPEDDD_EVENT_FLAG) == MOTOR_SPEDDD_EVENT_FLAG)
            {
                update_motor_speed(&pid_pitch, &pid_roll, &mpu6050_data);
            }
        }
    }
}
static void cal_thread(void *parameter)
{
    rt_err_t err;
    rt_uint32_t event;
    rt_uint32_t time_count = 0;
    exti_init();
    while (1)
    {
        err = rt_event_recv(cal_event, MPU6050_CAL_EVENT_FLAG, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
        if (err == RT_EOK)
        {
            if ((event & MPU6050_CAL_EVENT_FLAG) == MPU6050_CAL_EVENT_FLAG)
            {
                rt_enter_critical(); // double protection to finish cal
                mpu6050_read_all(&mpu6050_data);
                mpu6050_attitude_angle(&mpu6050_data);
                rt_event_send(motor_speed_event, MOTOR_SPEDDD_EVENT_FLAG);
                if (mpu6050_data.bias_ok_flag)
                {
                    time_count++;
                    if (time_count % 20 == 0) // 100ms
                    {
                        bmp280_read_data(&bmp280_data);
                    }
                    if (time_count >= 200) // 1s
                    {
                        time_count = 0;
                        // rt_event_send(com_event, COM_SEND_EVENT_FLAG);
                    }
                    // check offline
                    if (command.online)
                    {
                        command.online_time_out++;
                        if (command.online_time_out >= 300) // 1.5s
                        {
                            command.online_time_out = 0;
                            command.online = 0; // offline
                        }
                    }
                }
                rt_exit_critical();
            }
        }
    }
}
