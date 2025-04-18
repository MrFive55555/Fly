#include "SPI.h"
#define SPI_FLAG_TIMEOUT 0xFFFFFFFF
void spi_init(void)
{
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	// 1.init GPIO
	RCC_APB1PeriphClockCmd(SPI_PERIPHERAL_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(SPI_GPIO_CLK|SPI_AFIO_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = SPI_CS_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI_GPIO, &GPIO_InitStructure);
	GPIO_SetBits(SPI_GPIO, SPI_CS_Pin);
	GPIO_InitStructure.GPIO_Pin = SPI_MISO_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(SPI_GPIO, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = SPI_CLK_Pin | SPI_MOSI_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI_GPIO, &GPIO_InitStructure);
	// 2.init SPI
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // 36 / 4 = 9Mhz
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init(SPI_PERIPHERAL, &SPI_InitStructure);
	// 3.enable SPI
	SPI_Cmd(SPI_PERIPHERAL, ENABLE);
}
uint8_t spi_write_read_byte(uint8_t sned_byte)
{
	static uint8_t read_byte;
	static uint32_t spi_wait_time;
	spi_wait_time = SPI_FLAG_TIMEOUT;
	while (SPI_I2S_GetFlagStatus(SPI_PERIPHERAL, SPI_I2S_FLAG_TXE) == RESET)
	{
		if ((spi_wait_time--) <= 0)
		{
			return 0;
		}
	}
	SPI_I2S_SendData(SPI_PERIPHERAL, sned_byte);
	spi_wait_time = SPI_FLAG_TIMEOUT;
	while (SPI_I2S_GetFlagStatus(SPI_PERIPHERAL, SPI_I2S_FLAG_RXNE) == RESET)
	{
		if ((spi_wait_time--) <= 0)
		{
			return 0;
		}
	}
	read_byte = SPI_I2S_ReceiveData(SPI_PERIPHERAL);
	return read_byte;
}
