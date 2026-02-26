#include <stdint.h>
#include <stdbool.h>
#include "uart0.h"

#define MAX_CHARS 80
#define MAX_FIELDS 5
typedef struct _USER_DATA
{
char buffer[MAX_CHARS+1];
uint8_t fieldCount;
uint8_t fieldPosition[MAX_FIELDS];
char fieldType[MAX_FIELDS];
} USER_DATA;

// also need a gets Uart0(USER_DATA *data)
// this will be the function to receive character from the user interface getting characters and making
// the string into a buffer

// feildCount is like the amount of words or numbers in the string
// feildPositiotn is like when a certain string or number starts in the whole text that is typed
void getsUart0(USER_DATA *data)
{
    uint32_t count = 0;

    while (1)
    {
        char c = getcUart0();

        if ((c == 8 || c == 127))
        {
            if (count > 0)
                count--;
        }
        else if (c == 13 || c == 10)
        {
            data->buffer[count] = '\0';
            data->fieldCount = 0;
            return;
        }
        else if (c >= 32 && count < MAX_CHARS)
        {
            data->buffer[count++] = c;
        }
    }
}


//Will also nned a parseFields(USER_DATA_*data)
/* this function will take the buffer string from the getsUart0() function and processs the string in place
 * and returns the information about the parsed fields in fieldCount, field Position, and fieldType*/
void parseFields(USER_DATA *data)
{
    // going to be from hex value form 41 5A for A - Z and 61 - 7A
    //uint32_t currrentType = 0;
    //uint8_t previousType;
    int previousValueDelimiter = 1;
    uint8_t position = 0;
    uint8_t field = 0;

    while (data->buffer[position] != '\0' && field < MAX_FIELDS)
    {
        char c = data->buffer[position];

        if ((c >= 0x41 &&  c <= 0x5A)  || (c >= 0x61 && c <= 0x7A))   // And this is just the hex valueof A- Z and a - z
        {
            if (previousValueDelimiter == 1)
            {
                data->fieldPosition[field] = position;
                data->fieldType[field] = 'a';
                field++;
                previousValueDelimiter = 0;
            }
        }

        else if (c >= 0x30 && c <= 0x39) //This is just the hex value of 0 - 9
            {
                if (previousValueDelimiter == 1)
                {
                    data->fieldPosition[field] = position;
                    data->fieldType[field] = 'n';
                    field++;
                    previousValueDelimiter = 0;
                }
            }

            else    // everything else is a dimiliter
            {
                data->buffer[position] = '\0';
                previousValueDelimiter = 1;
            }
            position++;
        // This is to check for a new field
            data->fieldCount = field;
    }

}

//Will also need char* getafieldString(USER_DATA * data, uint8_t fieldNumber)
/* This will return the integer value of the field if the field number is in range */
char * getFieldString(USER_DATA * data, uint8_t fieldNumber)
{
    if (fieldNumber < data->fieldCount && data->fieldType[fieldNumber] == 'a')
        return &data->buffer[data->fieldPosition[fieldNumber]];
    else
        return 0;

}

int32_t getFieldInteger(USER_DATA*data, uint8_t fieldNumber)
{
    // make sure the field is valid and numeric
        if (fieldNumber < data->fieldCount && data->fieldType[fieldNumber] == 'n')
        {
            int32_t value = 0;
            char *ptr = &data->buffer[data->fieldPosition[fieldNumber]];

             // loop through each character until null terminator
             while (*ptr != '\0')
             {
                 // check if character is between '0' (0x30) and '9' (0x39)
                 if (*ptr >= 0x30 && *ptr <= 0x39)
                 {
                     value = (value * 10) + (*ptr - 0x30);
                 }
                 else
                 {
                     // not a number stop reading
                     break;
                 }
                 ptr++;
             }

             return value;
        }

            return 0;
}

bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments)
{
    uint8_t args = data->fieldCount - 1;
        char *cmd = &data->buffer[data->fieldPosition[0]];
        bool match = true;
        uint8_t i = 0;

        // compare both strings (case-insensitive)
        while (cmd[i] != '\0' && strCommand[i] != '\0')
        {
            char a = cmd[i];
            char b = strCommand[i];

            // convert to lowercase if uppercase
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;

            if (a != b)
            {
                match = false;
                break;
            }
            i++;
        }

        // check if one string is longer than the other
        if (cmd[i] != '\0' || strCommand[i] != '\0')
            match = false;

        // return true if command matches and there are enough arguments
        if (match && args >= minArguments)
            return true;
        else
            return false;
}


