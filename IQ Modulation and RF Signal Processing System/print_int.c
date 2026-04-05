/*
 * print_int.c
 *
 *  Created on: Feb 13, 2026
 *      Author: oscar
 */

#include <stdint.h>
#include "uart0.h"
#include "print_int.h"

// Print unsigned integer
void print_int(uint32_t value)
{
    char buffer[10];
    int i = 0;

    if(value == 0)
    {
        putcUart0('0');
        return;
    }

    while(value > 0)
    {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    while(i > 0)
    {
        putcUart0(buffer[--i]);
    }
}

// Print signed integer
void print_int_signed(int32_t value)
{
    if(value < 0)
    {
        putcUart0('-');
        value = -value;
    }

    print_int((uint32_t)value);
}
