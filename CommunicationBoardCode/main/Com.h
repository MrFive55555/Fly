#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include "GlobalVar.h"
void uart_init(uint32_t baud_rate); 
void uart_task(void *pvParameters);
void wifi_init(void);
void mqtt_init(void);
void send_mqtt_data(void);
#endif
