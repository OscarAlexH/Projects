/*
 * debug.c
 *
 *  Created on: Feb 6, 2026
 *      Author: mqtra
 */
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>
char* toAsciiHex(uint32_t val,char *buffer)
{
const char digits[] = "0123456789ABCDEF";
int i;
for(i = 0; i < 8; i++)
{
uint32_t shift =(7-i)*4;
uint8_t bit = (val >> shift) & 0xF; //masks the number
buffer[i] =digits[bit];
}
buffer[8] = '\0';
return buffer;
}
