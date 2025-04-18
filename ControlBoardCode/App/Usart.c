#include "Usart.h"
#include "Tool.h"
#include "DMA.h"

// 重定向 c 库函数 printf 到串口，重定向后可使用 printf 函数
int fputc(int ch, FILE *f)
{
	/* 发送一个字节数据到串口 */
	USART_SendData(USART1, (uint8_t)ch);
	/* 等待发送完毕 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
		;
	return (ch);
}
// 重定向 c 库函数 scanf 到串口，重写向后可使用 scanf、getchar 等函数
int fgetc(FILE *f)
{
	/* 等待串口输入数据 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
		;
	return (int)USART_ReceiveData(USART1);
}
static void init_string_bufffer(void)
{
	/**
	 * uart manage data by circle queue
	 */
	com_data.head_queue = com_data.tail_queue = com_data.data_count = 0;
}
void usartInit(const u32 bound)
{
	// 初始化配置
	GPIO_InitTypeDef GPIO_InitStructure;   // GPIO
	USART_InitTypeDef USART_InitStructure; // 串口
	NVIC_InitTypeDef NVIC_InitStructure;   // 中断
	// 1.使能 串口 GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
	// 2.GPIO 端口模式设置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // USART1_TX PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);			// 初始化 GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;		// USART1_RX PA.10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;	// 下拉输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);			// 初始化 GPIOA.10
	// 3.串口参数初始化
	USART_InitStructure.USART_BaudRate = bound;										// 波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 字长为 8 位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 收发模式
	USART_Init(USART1, &USART_InitStructure);
	// 4.初始化 NVIC 分组2
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 抢占优先级 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		  // 子优先级 1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ 通道使能
	NVIC_Init(&NVIC_InitStructure);							  // 中断优先级初始化
	// 5.开启中断
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 开启中断
	// 6.使能串口
	USART_Cmd(USART1, ENABLE); // 使能串口

	init_string_bufffer();
}
// 发送字符
void usartSendByte(USART_TypeDef *USARTx, const u8 data)
{
	// 发送数据
	USART_SendData(USARTx, data);
	// 等待发送数据寄存器为空
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
		;
}
// 发送字符串
void usartSendString(USART_TypeDef *USARTx, const u8 *data)
{

	// 发送数据
	while (*data != '\0')
		usartSendByte(USARTx, *(data++));
	// 等待发送完成
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
		;
}
// 中断函数 接收到数据触发中断
/* void USART1_IRQHandler(void)
{
	static uint8_t head_queue = 0;
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		receiveBuffer[head_queue] = USART_ReceiveData(USART1);
		if (receiveBuffer[head_queue++] == CHAR_ENTER)
		{
			receiveBuffer[head_queue] = '\0';
			head_queue = 0;
			// printf("%s\r\n",receiveBuffer);
			//printf("%s","receive all fill.\r\n");
			event_send(usaevent, USART_RECEIVE_ELEC_DATA_EVENT_FLAG);
		}
	}
} */
void USART1_IRQHandler(void)
{
	uint8_t receive_data;
	static uint8_t head_frame_count = 0;
	rt_interrupt_enter();
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		receive_data = USART_ReceiveData(USART1);
		// head of frame
		if (head_frame_count < 4)
		{
			com_data.head_of_frame[head_frame_count++] = receive_data;
		}
		if (com_data.data_count >= BUFFER_SIZE)
		{
			// printf("data had full,please take data.\r\n");
		}
		else
		{
			if (head_frame_count >= 4)
			{
				if (com_data.head_of_frame[head_frame_count - 4] == '$' && com_data.head_of_frame[head_frame_count - 3] == 'C' && com_data.head_of_frame[head_frame_count - 2] == 'M')
				{
					receive_buffer[com_data.tail_queue] = receive_data;
					if (com_data.data_count > 0 && (receive_buffer[com_data.tail_queue - 1] == CHAR_CR && receive_buffer[com_data.tail_queue] == CHAR_ENTER))
					{
						com_data.end_of_frame = 1;
					}
					if (com_data.tail_queue < BUFFER_SIZE - 1)
						com_data.tail_queue++;
					else
						com_data.tail_queue = 0;
					com_data.data_count++;
					if (com_data.end_of_frame)
					{
						com_data.end_of_frame = 0;
						head_frame_count = 0;
						rt_event_send(com_event, COM_RECEIVE_EVENT_FLAG);
					}
				}
				else
				{
					head_frame_count = 0;
				}
			}
		}
	}
	rt_interrupt_leave();
}
