#ifndef BMP280_H
#define BMP280_H
#include "GlobalVar.h"
void bmp280_init(void);
void bmp280_read_data(BMP280_Data *bmp280_data);
#endif
