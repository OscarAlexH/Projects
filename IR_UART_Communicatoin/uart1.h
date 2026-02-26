#ifndef UART1_H_
#define UART1_H_

#include <stdint.h>
#include <stdbool.h>

void initUart1();
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc);

void putcUart1(char c);
void putsUart1(char *str);
char getcUart1();

#endif
