#include "Tool.h"
/**
 * num format to string
 * such as: numToStr(1234, 3, 2, strAddress, CHAR_ARRAY_TYPE) will get "012.34"
 */
void numToStr(int32_t num, uint8_t intBits, uint8_t floatBits, uint8_t *strAddress, uint8_t type,uint16_t* str_length)
{
    uint8_t *tail_address = 0;
    uint8_t num_length = (floatBits > 0 ? intBits + floatBits + 1 : intBits); //+1 for dot
	if (num < 0)
    {
        *(strAddress)++ = '-'; 
        num = -num;
        (*str_length)++;
    }
    strAddress += (type != CHAR_ARRAY_TYPE ? num_length : num_length - 1); 
    tail_address = strAddress;                                         
    if (type != CHAR_ARRAY_TYPE)
    {
        *(strAddress)-- = '\0';
        (*str_length)++;
    }
    if (num != 0)
    {
        while (num != 0)
        {
            // when decimals save finined,add dot into array
            if (((tail_address - strAddress) == (type != CHAR_ARRAY_TYPE ? intBits + 1 : intBits)) && floatBits > 0)
            {
                *(strAddress)-- = '.';
                (*str_length)++;
            }
            else
            {
                *(strAddress)-- = num % 10 + '0';
                num /= 10;
                (*str_length)++;
            }
        }
    }
    // fill '0' to surplus
    if (type != CHAR_ARRAY_TYPE)
    {
        while (tail_address - strAddress <= num_length) // exist risk,because there array overrach
        {
            *(strAddress)-- = '0';
            (*str_length)++;    
        }
    }
    else
    {
        while (tail_address - strAddress < num_length) // exist risk,because there array overrach
        {
            *(strAddress)-- = '0';
            (*str_length)++;
        }
    }
}
int32_t strToNum(uint8_t numBit, uint8_t *strAddress)
{
    int32_t num = 0;
    uint8_t firstData = *strAddress;
    
    if (numBit <= 0 || numBit > 10)
        return num;

    if (firstData == '-')
    {
        strAddress++;
    }

    while (numBit)
    {
        num = (*(strAddress)++ - '0') + num * 10;
        numBit--;
    }

    if (firstData == '-')
    {
        num = -num;
    }

    return num;
}
// get length of string
uint16_t getStringLength(uint8_t *pBuf)
{
    uint16_t length = 0;
    while (*pBuf != '\0')
    {
        length++;
        pBuf++;
    }
    return length;
}
// get abs value
uint32_t getAbsValue(int32_t value)
{
    return value < 0 ? -value : value;
}
//CRC32 check
uint32_t crc32(uint8_t *data, uint16_t length)
{
    uint8_t i;
    uint32_t crc = 0xffffffff;        // Initial value
    while(length--)
    {
        crc ^= *data++;                // crc ^= *data; data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;// 0xEDB88320= reverse 0x04C11DB7
            else
                crc = (crc >> 1);
        }
    }
    return ~crc;
}
