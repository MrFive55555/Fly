#include "Com.h"
/**
 * function declaration
 */
static void send_mpu6050_data(uint16_t *length, void *send_data);
static void send_bmp280_data(uint16_t *length, void *send_data);
static void send_mpu6050_attitude_angle_data(uint16_t *length, void *send_data);
static void send_debug_data(uint16_t *length, void *send_data);
static void send_data_to_other_device(uint16_t *length, void *send_data);
typedef void (*SendDataFunc)(uint16_t *, void *);
static const SendDataFunc send_data_func[] = {
    send_mpu6050_data,
    send_bmp280_data,
    send_mpu6050_attitude_angle_data,
    send_debug_data,
    send_data_to_other_device,
};
static void send_mpu6050_data(uint16_t *length, void *send_data)
{
    MPU6050_Data *mpu6050_data = (MPU6050_Data *)send_data;
    sprintf(&send_buffer[*length], "ACC:%d,%d,%d GYRO:%d,%d,%d ",
            mpu6050_data->accl[0], mpu6050_data->accl[1], mpu6050_data->accl[2],
            mpu6050_data->gyro[0], mpu6050_data->gyro[1], mpu6050_data->gyro[2]);
    *length = rt_strlen(send_buffer);
}
static void send_bmp280_data(uint16_t *length, void *send_data)
{
    BMP280_Data *bmp280_data = (BMP280_Data *)send_data;
    sprintf(&send_buffer[*length], "TEM:%d,PRE:%d ", bmp280_data->temp, bmp280_data->press);
    *length = rt_strlen(send_buffer);
}
static void send_mpu6050_attitude_angle_data(uint16_t *length, void *send_data)
{
    MPU6050_Data *mpu6050_data = (MPU6050_Data *)send_data;
    sprintf(&send_buffer[*length], "acc_pitch_roll:%.2f,%.2f gyro_pitch_roll:%.2f,%.2f pitch_roll:%.2f,%.2f ",
            mpu6050_data->attitude_angle[0], mpu6050_data->attitude_angle[1], mpu6050_data->attitude_angle[2],
            mpu6050_data->attitude_angle[3], mpu6050_data->attitude_angle[4], mpu6050_data->attitude_angle[5]);
    *length = rt_strlen(send_buffer);
}
static void send_debug_data(uint16_t *length, void *send_data)
{
    // Debug_Data *debug_data = (Debug_Data *)send_data;
    Command *command = (Command *)send_data;
    sprintf(&send_buffer[*length], "$FB%d", command->feed_back);
    *length = rt_strlen(send_buffer);
}
static void send_data_to_other_device(uint16_t *length, void *send_data)
{
    sprintf(&send_buffer[*length], "\r\n");
    DMANormalEnable(DMA1_Channel4, rt_strlen(send_buffer));
    *length = 0;
}
void send_data_add(Send_Data_Type type, void *send_data)
{
    static uint16_t length = 0;
    send_data_func[type](&length, send_data);
}
void command_parse(void)
{
    uint8_t data_length = com_data.tail_queue > com_data.head_queue ? com_data.tail_queue - com_data.head_queue : BUFFER_SIZE - com_data.head_queue + com_data.tail_queue;

    if (receive_buffer[com_data.head_queue] == 'T' && receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == 'A' && receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == 'K')
    {
        if (receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == '1')
        {
            command.take_off = 1;
            command.feed_back = command.take_off;
            // feed back
            send_data_add(DEBUG, &command);
            send_data_add(OK, NULL);
        }
    }
    else if (receive_buffer[com_data.head_queue] == 'S' && receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == 'T' && receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == 'P')
    {
        if (receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == '1')
        {
            command.take_off = 0;
            command.feed_back = !command.take_off;
            // feed back
            send_data_add(DEBUG, &command);
            send_data_add(OK, NULL);
        }
    }else if (receive_buffer[com_data.head_queue] == 'O' && receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == 'N' && receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == 'L')
    {
        if (receive_buffer[(++com_data.head_queue) % BUFFER_SIZE] == '1')
        {
            command.online = 1;
            command.online_time_out = 0; //clear count to recount
        }
    }
    com_data.head_queue += 3; // set head track tail
    com_data.head_queue %= BUFFER_SIZE;
    com_data.data_count -= data_length; // clear had read data
}

/**
 * send data to other device by USART1
 */
// static void send_mpu6050_data(void *send_data);
// static void send_bmp280_data(void *send_data);
// static void send_debug_data(void *send_data);
/* static void send_mpu6050_data(void *send_data);
{
    // uint16_t length = 0;
    MPU6050_Data *mpu6050_data = (MPU6050_Data *)send_data;

     numToStr(mpu6050_data->accl[0], 5, 0, &send_buffer[length], CHAR_ARRAY_TYPE, &length);
    send_buffer[length++] = CHAR_BLANK;

    numToStr(mpu6050_data->accl[1], 5, 0, &send_buffer[length], CHAR_ARRAY_TYPE, &length);
    send_buffer[length++] = CHAR_BLANK;

    numToStr(mpu6050_data->accl[2], 5, 0, &send_buffer[length], CHAR_ARRAY_TYPE, &length);
    send_buffer[length++] = CHAR_BLANK;

    numToStr(mpu6050_data->gyro[0], 5, 0, &send_buffer[length], CHAR_ARRAY_TYPE, &length);
    send_buffer[length++] = CHAR_BLANK;

    numToStr(mpu6050_data->gyro[1], 5, 0, &send_buffer[length], CHAR_ARRAY_TYPE, &length);
    send_buffer[length++] = CHAR_BLANK;

    numToStr(mpu6050_data->gyro[2], 5, 0, &send_buffer[length], CHAR_ARRAY_TYPE, &length);
    send_buffer[length++] = CHAR_BLANK;

    DMANormalEnable(DMA1_Channel4, rt_strlen(send_buffer)); // send data by DMA
}
static void send_bmp280_data(void *send_data)
{
    // uint16_t length = 0;
    BMP280_Data *bmp280_data = (BMP280_Data *)send_data;
    // numToStr(bmp280_data->press, 5, 0, &send_buffer[length], CHAR_ARRAY_TYPE,&length);
    // length += 5;

    sprintf(&send_buffer[0], "Tem:%d,PRE:%d\r\n", bmp280_data->temp, bmp280_data->press);
    DMANormalEnable(DMA1_Channel4, rt_strlen(send_buffer)); // send data by DMA
}
static void send_debug_data(void *send_data)
{
    usartSendString(USART1, (uint8_t *)send_data);
}
void send_data_to_other_device(DataType type, void *send_data)
{
    send_data_func[type](send_data);
} */
