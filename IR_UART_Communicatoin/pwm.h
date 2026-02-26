#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>
#include <stdbool.h>

// Initializes PWM output on PB6 at 38 kHz for IR carrier
void initPwm(void);

#endif
