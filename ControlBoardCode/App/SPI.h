#ifndef SPI_H
#define SPI_H
#include "GlobalVar.h"
/**
 * SPI IO definition by hardware
 */
#define SPI_AFIO_CLK RCC_APB2Periph_AFIO    
#define SPI_PERIPHERAL SPI2
#define SPI_PERIPHERAL_CLK RCC_APB1Periph_SPI2
#define SPI_GPIO_CLK RCC_APB2Periph_GPIOB
#define SPI_GPIO GPIOB
#define SPI_CS_Pin GPIO_Pin_12
#define SPI_CLK_Pin GPIO_Pin_13
#define SPI_MISO_Pin GPIO_Pin_14
#define SPI_MOSI_Pin GPIO_Pin_15
#define SPI_CS PBout(12)
void spi_init(void);
uint8_t spi_write_read_byte(uint8_t sned_byte);
#endif
