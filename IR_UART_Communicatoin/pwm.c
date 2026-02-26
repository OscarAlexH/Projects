#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "commonterminalinterface.h"
#include "uart0.h"
#include "clock.h"
#include "strings.h"
//#include <wait.h>

/*
 *

 * this is a pwm example taken directly out of the data sheet.
 *0
     // M0PWM0  Pin 1 PB6 (4)
     // M0PWM1  Pin 4 PB7 (4)
 *
 */
#define PB6 0x40


void initPwm(void)
{
    // Enable the clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R3;

    _delay_cycles(3);

    GPIO_PORTB_AFSEL_R |= PB6;
    GPIO_PORTB_ODR_R &= ~PB6;       //    disable open drain on PB6, PB7
    GPIO_PORTB_DEN_R |= PB6;          //    enable digital I/O on PB6, PB7
    GPIO_PORTB_AMSEL_R &= ~PB6;     //    disable analog function on PB6, PB7

    GPIO_PORTB_PCTL_R &= ~GPIO_PCTL_PB6_M;
    GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB6_M0PWM0;


    // Clear PWM clock
    SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M;

    // system clock is 40 MHZ
    // pwm clock is 10 MHZ
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_4;

    // Disable PWM0
    PWM0_CTL_R = 0x0;

    PWM0_0_GENA_R = PWM_0_GENA_ACTLOAD_M | PWM_0_GENA_ACTCMPAD_ZERO;

    // 10 mhz clock
    // 10 kkhz period

    PWM0_0_LOAD_R = 262;
    PWM0_0_CMPA_R = 131;

    // sync
    PWM0_CTL_R |= PWM_CTL_GLOBALSYNC0;

    // enable output

    PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN;
    PWM0_0_CTL_R |= PWM_0_CTL_ENABLE; // start PWM0 Generator 0
}
