#ifndef COM_H
#define COM_H
#include "GlobalVar.h"
#include "Tool.h"
#include "DMA.h"
#include "Usart.h"
void send_data_add(Send_Data_Type type, void *send_data);
void command_parse(void);
#endif  // COM_H
