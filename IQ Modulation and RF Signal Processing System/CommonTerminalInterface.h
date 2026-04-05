/*
 * CommonTerminalInterface.h
 *
 *  Created on: Feb 5, 2026
 *      Author: mqtra
 */

#ifndef COMMONTERMINALINTERFACE_H_
#define COMMONTERMINALINTERFACE_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_CHARS 80
#define MAX_FIELDS 32

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS + 1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;

extern volatile uint32_t elapsedTime;
void initSysTick(void);
void getsUart0(USER_DATA *data);
void parseFields(USER_DATA *data);
char*   getFieldString(USER_DATA *data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber);
bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments);
int strcmp(const char *s1, const char *s2);
float getFieldFloat(USER_DATA *data, uint8_t fieldNumber);


#endif /* COMMONTERMINALINTERFACE_H_ */
