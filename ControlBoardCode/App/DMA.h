#ifndef DMA_H
#define DMA_H
#include "GlobalVar.h"
void DMA_Config(DMA_Channel_TypeDef *DMA_Channelx, uint32_t sourceAddress, uint32_t destAddress, uint32_t dataSize);
void DMANormalEnable(DMA_Channel_TypeDef *DMA_Channelx, uint16_t bufferSize);
void DMA1_Channel4_IRQHandler(void);
#endif
