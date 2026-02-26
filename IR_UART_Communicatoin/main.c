#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "pwm.h"
#include "CommonTerminalInterface.h"
#include "debug.h"
#include "uart1.h"

#define blueLED 0x04

void initUart1RxInterrupt()
{
    UART1_CTL_R &= ~UART_CTL_UARTEN;    // disable UART1 during config
    UART1_IFLS_R &= ~UART_IFLS_RX_M;     // interrupt when RX FIFO has  1 byte
    UART1_IFLS_R |= UART_IFLS_RX2_8;
    UART1_ICR_R = UART_ICR_RXIC | UART_ICR_RTIC;    // clear prior interrupts
    UART1_IM_R |= UART_IM_RXIM | UART_IM_RTIM;  // enable RX + RX timeout interrupt
    NVIC_EN0_R |= (1 << 6);     // enable in NVIC is UART1 = interrupt 6
    NVIC_PRI1_R &= ~0x00E00000; // priority 0
    UART1_CTL_R |= UART_CTL_UARTEN;// turn UART back on
 }

void UART1Handler(void)
    {
    GPIO_PORTF_DATA_R |= blueLED;     // for blue led
    UART1_ICR_R = (UART_ICR_RXIC | UART_ICR_RTIC); // clears the interrupt flags
    char c;
    static bool start = true;
    while(!(UART1_FR_R & UART_FR_RXFE))        // will go unitl the fifo is empy
    {
        if(start)
        {
            putsUart0("UART1 RX Message: ");       // prints for the first character
            start = false;
        }
         c = UART1_DR_R & 0xFF; // reads 8 bit one character
        putcUart0(c);      // prints it
        if(c == 0)
        {
            putsUart0("\r\n");
            start = true;
         }
     }
     GPIO_PORTF_DATA_R &= ~blueLED;        // turn off the blue led
     }

 int main(void)
 {
     initSystemClockTo40Mhz();
     initUart0();
     setUart0BaudRate(115200,40000000);
     initPwm();
     initUart1();
     initUart1RxInterrupt();

     SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
     GPIO_PORTF_DIR_R |= blueLED;
     GPIO_PORTF_DEN_R |= blueLED;

     USER_DATA value;

     while(1)
     {
         getsUart0(&value);
         parseFields(&value);
         bool valid = false;

         if (isCommand(&value, "baud", 1))
         {
             uint32_t baud = getFieldInteger(&value, 1);

             if (baud == 300)
             {
                 setUart1BaudRate(baud, 40000000);
                 putsUart0("UART1 baud rate set to ");
                 putsUart0("300");
                 putsUart0("\r\n");
                 valid = true;
             }
             else if ( baud == 1200)
             {
                 setUart1BaudRate(baud, 40000000);
                 putsUart0("UART1 baud rate set to ");
                 putsUart0("1200");
                 putsUart0("\r\n");
                 valid = true;
             }

             else if (baud == 2400)
             {
                 setUart1BaudRate(baud, 40000000);
                 putsUart0("UART1 baud rate set to ");
                 putsUart0("2400");
                 putsUart0("\r\n");
                 valid = true;
             }

             else if (baud == 4800)
             {
                 setUart1BaudRate(baud, 40000000);
                 putsUart0("UART1 baud rate set to ");
                 putsUart0("4800");
                 putsUart0("\r\n");
                 valid = true;
             }

             else
             {
                 putsUart0("Invalid baud (allowed: 300,1200,2400,4800)\r\n");
             }
         }

         if(isCommand(&value, "send", 1))
         {
             char temp[64];
             uint32_t len = 0;
             uint8_t i;

             for(i = 1; i < value.fieldCount; i++)
             {
                 char *str = &value.buffer[value.fieldPosition[i]];       // skips send

                 while(*str && len < 63) // puts character in temp
                     temp[len++] = *str++;

                 if(i < value.fieldCount - 1 && len < 63)
                     temp[len++] = ' ';     // the space
             }

             temp[len] = 0; // null

             if(len >= 63)      // less then 64bytes
             {
                 putsUart0("Message too long the is max 64 bytes\r\n");
                 continue;
             }

             for(i = 0; i <= len; i++)
             {
                 while(UART1_FR_R & UART_FR_TXFF);      // loops and waits for fifo to be full
                 UART1_DR_R = temp[i];
             }
         }

         else if (!valid)
         {
             putsUart0("Not a command\r\n");
         }
     }
     }
