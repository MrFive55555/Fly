#ifndef USART_H
#define USART_H
#include "GlobalVar.h"
#include "stdio.h"
#define CHAR_BLANK 32
#define CHAR_ENTER 10
#define CHAR_CR 13
void usartInit(const u32 bound);
void usartSendByte(USART_TypeDef *USARTx, const u8 data);
void usartSendString(USART_TypeDef *USARTx,const u8 *data);
void USART1_IRQHandler(void);
int fputc(int ch, FILE *f);
int fgetc(FILE *f);
#endif
