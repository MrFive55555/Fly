#include "DMA.h"
void DMA_Config(DMA_Channel_TypeDef *DMA_Channelx, uint32_t sourceAddress, uint32_t destAddress, uint32_t dataSize)
{
	DMA_InitTypeDef DMA_InitStructure;
	uint32_t MemDataSize, perDataSize;
	switch (dataSize)
	{
	case 8:
		perDataSize = DMA_PeripheralDataSize_Byte;
		MemDataSize = DMA_MemoryDataSize_Byte;
		break;
	case 16:
		perDataSize = DMA_PeripheralDataSize_HalfWord;
		MemDataSize = DMA_MemoryDataSize_HalfWord;
		break;
	case 32:
		perDataSize = DMA_PeripheralDataSize_Word;
		MemDataSize = DMA_MemoryDataSize_Word;
		break;
	}
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); // don't forget to enable peripher clock
	DMA_DeInit(DMA_Channelx);
	DMA_InitStructure.DMA_MemoryBaseAddr = sourceAddress;	// memory address
	DMA_InitStructure.DMA_PeripheralBaseAddr = destAddress; // peripher address
	DMA_InitStructure.DMA_BufferSize = 0;					// transmitter data size
	DMA_InitStructure.DMA_MemoryDataSize = MemDataSize;		// data unit is byte
	DMA_InitStructure.DMA_PeripheralDataSize = perDataSize; // make sure data unit keep same

	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;				 // direction is memory to peripher
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;			 // memory addres auto-increment
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // periph address dont't auto-increment
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;					 // only transimtter once
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;			 // medium priority
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;					 // don't use memory to memory

	DMA_Init(DMA_Channelx, &DMA_InitStructure); // channel3 is SPI_TX
	// 允许 USART1 DMA 发送请求
 	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	// 开启 DMA1 Channel4 传输完成中断
	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);                                                           

	// 使能 NVIC 对应的中断
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
void DMANormalEnable(DMA_Channel_TypeDef *DMA_Channelx, uint16_t bufferSize)
{
	DMA_Cmd(DMA_Channelx, DISABLE);
	DMA_SetCurrDataCounter(DMA_Channelx, bufferSize);
	DMA_Cmd(DMA_Channelx, ENABLE);
}
void DMA1_Channel4_IRQHandler(void)
{
	rt_interrupt_enter();
	if (DMA_GetITStatus(DMA1_IT_TC4)) // 检查是否是传输完成中断
	{
		DMA_ClearITPendingBit(DMA1_IT_TC4); // 清除中断标志
											// 可选：在这里通知应用层数据已经发送完毕，比如设置一个标志位
		debug_flag = 1;
	}
	rt_interrupt_leave();
}
