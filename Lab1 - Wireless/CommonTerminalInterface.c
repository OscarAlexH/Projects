
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "clock80.h"
#include "wait.h"
#include "uart0.h"
#include "debug.h"
#include <stdbool.h>
#include <spi1.h>
#define MAX_CHARS 80
#define MAX_FIELDS 32



//#define DEBUG
typedef struct _USER_DATA
    {
        char buffer[MAX_CHARS+1];
        uint8_t fieldCount;
        uint8_t fieldPosition[MAX_FIELDS];
        char fieldType[MAX_FIELDS];
    } USER_DATA;

volatile uint32_t elapsedTime = 0;

void SysTick_Handler()
    {
        elapsedTime++;
    }

void initSysTick()
    {//use code from exam as example
        NVIC_ST_RELOAD_R = 3999999;
        NVIC_ST_CURRENT_R = 0;
        NVIC_ST_CTRL_R = 0;
        NVIC_ST_CTRL_R |= NVIC_ST_CTRL_INTEN;
    }

void getsUart0(USER_DATA *data)
   {
    char tempc;
    volatile uint32_t count = 0;
    while(1)
    {
    tempc = getcUart0();
    if((tempc == 8 || tempc == 127) && count > 0)
        {
         count--;
         continue;
        }
    if( tempc == 13)
        {
        data -> buffer[count] = 0;
        return;
        }

    if( tempc >= 32)
        {
        data -> buffer[count] = tempc;
        count++;
        }
    if( count == MAX_CHARS)
        {
        data -> buffer[count] = 0;
        return;
        }
    }
   }


void parseFields(USER_DATA *data)
{
    char previousType = 'd';
    char currentType;
    char currentChar;

    data->fieldCount = 0;
    uint32_t i;

    for (i = 0; data->buffer[i] != '\0'; i++)
    {
        currentChar = data->buffer[i];

        // Alphabetic character
        if ((currentChar >= 0x41 && currentChar <= 0x5A) ||
            (currentChar >= 0x61 && currentChar <= 0x7A))
        {
            currentType = 'a';
        }
        // Numeric character
        else if ((currentChar >= '0' && currentChar <= '9') || currentChar == '-' || currentChar == '.')
        {
            currentType = 'n';
        }
        // Delimiter
        else
        {
            currentType = 'd';
            data->buffer[i] = '\0';
        }

        // Start of a new field
        if (previousType == 'd' && (currentType == 'a' || currentType == 'n'))
        {
            if (data->fieldCount < MAX_FIELDS)
            {
                data->fieldType[data->fieldCount] = currentType;
                data->fieldPosition[data->fieldCount] = i;
                data->fieldCount++;
            }
        }

        previousType = currentType;
    }
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber < data->fieldCount)
    {
        return &data->buffer[data->fieldPosition[fieldNumber]];
    }
    return 0;
}

int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber < data->fieldCount &&
        data->fieldType[fieldNumber] == 'n')
    {
        char *str = &data->buffer[data->fieldPosition[fieldNumber]];
        uint32_t value = 0;

        while (*str >= 0x30 && *str <= 0x39)
        {
            value = value * 10 + (*str - '0');
            str++;
        }
        return value;
    }
    return 0;
}

float getFieldFloat(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber < data->fieldCount &&
        data->fieldType[fieldNumber] == 'n')
    {
        char *str = &data->buffer[data->fieldPosition[fieldNumber]];
        float value = 0;
        float sign = 1.0;
        float divisor = 1.0;
        bool decimal = false;

        if (*str == '-')
        {
            sign = -1.0;
            str++;
        }

        while (*str != '\0')
        {
            if (*str == '.')
            {
                decimal = true;
            }
            else if (*str >= '0' && *str <= '9')
            {
                if (!decimal)
                {
                    value = value * 10 + (*str - '0');
                }
                else
                {
                    divisor *= 10;
                    value += (*str - '0') / divisor;
                }
            }
            else
            {
                break;
            }
            str++;
        }

        return value * sign;
    }
    return 0;
}



bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments)
{
    if ((data->fieldCount > 0) &&
        (strcmp(&data->buffer[data->fieldPosition[0]], strCommand) == 0) &&
        ((data->fieldCount - 1) >= minArguments))
    {
        return true;
    }
    return false;
}


