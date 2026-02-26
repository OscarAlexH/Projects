

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart1.h"
#include "uart0.h"

// PC4 = U1RX
// PC5 = U1TX

#define UART1_TX_MASK 0x20   // PC5
#define UART1_RX_MASK 0x10   // PC4

void initUart1()
{
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;
    _delay_cycles(3);

    GPIO_PORTC_DEN_R    |= UART1_TX_MASK | UART1_RX_MASK;
    GPIO_PORTC_AFSEL_R  |= UART1_TX_MASK | UART1_RX_MASK;
    GPIO_PORTC_PCTL_R   &= ~(GPIO_PCTL_PC4_M | GPIO_PCTL_PC5_M);
    GPIO_PORTC_PCTL_R   |=  (GPIO_PCTL_PC4_U1RX | GPIO_PCTL_PC5_U1TX);

    UART1_CTL_R = 0;
    UART1_CC_R  = UART_CC_CS_SYSCLK;

    UART1_IBRD_R = 8333;
    UART1_FBRD_R = 0;

    UART1_LCRH_R = UART_LCRH_WLEN_8 |UART_LCRH_PEN | UART_LCRH_EPS | UART_LCRH_FEN;


    UART1_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
}


void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;
    divisorTimes128 += 1;

    UART1_CTL_R = 0;
    UART1_IBRD_R = divisorTimes128 >> 7;
    UART1_FBRD_R = ((divisorTimes128 >> 1) & 63);

    UART1_LCRH_R = UART_LCRH_WLEN_8 |UART_LCRH_PEN | UART_LCRH_EPS | UART_LCRH_FEN;

    UART1_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
}
