#include "BMP280.h"
#include "SPI.h"
/**
 * inner function declaration
 */
static void bmp280_spi_write(uint8_t reg, uint8_t data);
static uint8_t bmp280_spi_read(uint8_t reg);
static void bmp280_spi_read_burst(uint8_t reg, uint8_t *read_bytes, uint16_t len);
static uint8_t bmp280_read_id(void);
static void bmp280_read_calibration(void);
static int32_t bmp280_compensate_t(int32_t adc_t);
static int32_t bmp280_compensate_p(int32_t adc_p);
#define MEASURE_WAIT_TIME 0xFFFFFFFF
// 校准参数结构体
typedef struct 
{
    /**
     * notation sign and unsign by datasheet
     */
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    int32_t  t_fine;
}BMP280_Calib;

static BMP280_Calib calib;
// SPI写寄存器
static void bmp280_spi_write(uint8_t reg, uint8_t data)
{
    SPI_CS = 0;
    spi_write_read_byte(reg & 0x7F); // 清除最高位（写标志）
    spi_write_read_byte(data);
    SPI_CS = 1;
}
// SPI读寄存器（单字节）
static uint8_t bmp280_spi_read(uint8_t reg)
{
    uint8_t read_byte;
    SPI_CS = 0;
    spi_write_read_byte(reg | 0x80);       // 置位最高位（读标志）
    read_byte = spi_write_read_byte(0xFF); // send 0xff only used to generate clock
    SPI_CS = 1;
    return read_byte;
}

// SPI突发读（多字节）
static void bmp280_spi_read_burst(uint8_t reg, uint8_t *read_bytes, uint16_t len)
{
    SPI_CS = 0;
    spi_write_read_byte(reg | 0x80); // 读模式
    for (uint16_t i = 0; i < len; i++)
    {
        read_bytes[i] = spi_write_read_byte(0xFF); // send 0xff only used to generate clock
    }
    SPI_CS = 1;
}
// 读取设备ID（应返回0x58）
static uint8_t bmp280_read_id(void)
{
    return bmp280_spi_read(0xD0);
}

// 读取校准参数（0x88-0xA1）
static void bmp280_read_calibration(void)
{
    uint8_t data[24];
    bmp280_spi_read_burst(0x88, data, 24);

    // 组合16位数据（LSB在前，MSB在后）
    calib.dig_T1 = (data[1] << 8) | data[0];
    calib.dig_T2 = (data[3] << 8) | data[2];
    calib.dig_T3 = (data[5] << 8) | data[4];
    calib.dig_P1 = (data[7] << 8) | data[6];
    calib.dig_P2 = (data[9] << 8) | data[8];
    calib.dig_P3 = (data[11] << 8) | data[10];
    calib.dig_P4 = (data[13] << 8) | data[12];
    calib.dig_P5 = (data[15] << 8) | data[14];
    calib.dig_P6 = (data[17] << 8) | data[16];
    calib.dig_P7 = (data[19] << 8) | data[18];
    calib.dig_P8 = (data[21] << 8) | data[20];
    calib.dig_P9 = (data[23] << 8) | data[22];
}
// 温度补偿（根据文档3.11.3）
static int32_t bmp280_compensate_t(int32_t adc_t)
{
    int32_t var1, var2;
    var1 = ((((adc_t >> 3) - ((int32_t)calib.dig_T1 << 1))) * (int32_t)calib.dig_T2) >> 11;
    var2 = (((((adc_t >> 4) - (int32_t)calib.dig_T1) * (adc_t >> 4) - (int32_t)calib.dig_T1) >> 12) * (int32_t)calib.dig_T3) >> 14;
    calib.t_fine = var1 + var2;
    return ((calib.t_fine * 5 + 128) >> 8) / 100; // 分辨率0.01°C，返回整数（如5123=51.23°C）
}

// 压力补偿
static int32_t bmp280_compensate_p(int32_t adc_p)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)calib.dig_P1) >> 33;
    if (var1 == 0)
        return 0; // prevent divide zero
    p = 1048576 - adc_p;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (uint32_t)p / 25600;
}
// 读取原始数据（20位）
void bmp280_read_data(BMP280_Data *bmp280_data)
{
    uint8_t buf[6];
    static uint32_t wait_time = 0;
    wait_time = MEASURE_WAIT_TIME;
    while (bmp280_read_id() != 0x58)
    {
        bmp280_init(); // 如果ID不正确，重新初始化
    }
    // 等待测量完成（0xF3: status寄存器）
    while (bmp280_spi_read(0xF3) & 0x08)
    {
        /**
         * make sure the wait time to satisfy wait requirement,else ouccur errors.
         */
        wait_time--;
        if (wait_time <= 0)
        {
            return;
        }
    }
    bmp280_spi_read_burst(0xF7, buf, 6); // 0xF7: P_MSB, P_LSB, P_XLSB, T_MSB, T_LSB, T_XLSB

    // 20 bits
    int32_t adc_p = (int32_t)(((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | (buf[2] >> 4));
    int32_t adc_t = (int32_t)(((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | (buf[5] >> 4));

    bmp280_data->temp = bmp280_compensate_t(adc_t); // must calculate temp first
    bmp280_data->press = bmp280_compensate_p(adc_p);
}
// 初始化BMP280
void bmp280_init(void)
{
    // 复位（可选）
    spi_init();
    rt_thread_delay(10); // 10ms
    bmp280_spi_write(0xE0, 0xB6); //soft reset
    rt_thread_delay(10);

    // 配置控制寄存器：正常模式，过采样率（示例：标准分辨率）
    bmp280_spi_write(0xF4, 0x6F); // osrs_t=011（x4）,osrs_p=011（×4）, mode=11（正常模式）
    bmp280_spi_write(0xF5, 0x3C); // standby:62.5ms sampling:x16
    bmp280_read_calibration();
}
