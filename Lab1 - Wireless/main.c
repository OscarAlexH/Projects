#include <stdint.h>
#include <stdio.h>
#include <clock80.h>
#include <gpio.h>
#include <nvic.h>
#include <spi1.h>
#include <tm4c123gh6pm.h>
#include <uart0.h>
#include <wait.h>
#include <wd0.h>
#include <CommonTerminalInterface.h>
#include <math.h>
#include "print_int.h"
#include <string.h>

#define LDAC (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 4*4)))

#define MAX_CHARS 80

#define Gain_I 1980
#define Gain_Q 1990

#define OFS_I 2111
#define OFS_Q 2105

#define FCYC 80e6
#define FDAC 20e6
#define DEFAULT_FS 100000

#define LUT_SIZE 256
#define LUT_LOG2_SIZE 8

#define PI 3.14159265359

#define USE_SSI_FSS 1

#define DATA_LEN 16   // 16 bytes = 128 bits

//filtering
#define UPSAMPLE    4
#define FIR_LEN     31
#define H_GAIN      65536
#define H_SHIFT     12


#define SYMAMP      512
#define SYMAMP_I  (Gain_I / 4)    // = 495
#define SYMAMP_Q  (Gain_Q / 4)    // = 497

volatile bool rrcEnabled = false;

volatile bool    newSampleReady = false;
volatile int32_t pendingI = 0;
volatile int32_t pendingQ = 0;
volatile uint16_t dacOutI = OFS_I;
volatile uint16_t dacOutQ = OFS_Q;

// FIR delay lines (circular buffers, zero-initialized)
static int32_t firI[FIR_LEN] = {0};
static int32_t firQ[FIR_LEN] = {0};
static int     firIdx = 0;

// Upsample counter: 0 = real symbol tick, 1/2/3 = zero insertion ticks
static uint32_t upsampleCount = 0;

static const int32_t h_rrc[FIR_LEN] =
{
     (int32_t)( 0.0023 * H_GAIN),   //  0
    -(int32_t)( 0.0043 * H_GAIN),   //  1
    -(int32_t)( 0.0102 * H_GAIN),   //  2
    -(int32_t)( 0.0090 * H_GAIN),   //  3
     (int32_t)( 0.0015 * H_GAIN),   //  4
     (int32_t)( 0.0159 * H_GAIN),   //  5
     (int32_t)( 0.0230 * H_GAIN),   //  6
     (int32_t)( 0.0130 * H_GAIN),   //  7
    -(int32_t)( 0.0136 * H_GAIN),   //  8
    -(int32_t)( 0.0422 * H_GAIN),   //  9
    -(int32_t)( 0.0493 * H_GAIN),   // 10
    -(int32_t)( 0.0160 * H_GAIN),   // 11
     (int32_t)( 0.0593 * H_GAIN),   // 12
     (int32_t)( 0.1553 * H_GAIN),   // 13
     (int32_t)( 0.2357 * H_GAIN),   // 14
     (int32_t)( 0.2671 * H_GAIN),   // 15   center tap
     (int32_t)( 0.2357 * H_GAIN),   // 16
     (int32_t)( 0.1553 * H_GAIN),   // 17
     (int32_t)( 0.0593 * H_GAIN),   // 18
    -(int32_t)( 0.0160 * H_GAIN),   // 19
    -(int32_t)( 0.0493 * H_GAIN),   // 20
    -(int32_t)( 0.0422 * H_GAIN),   // 21
    -(int32_t)( 0.0136 * H_GAIN),   // 22
     (int32_t)( 0.0130 * H_GAIN),   // 23
     (int32_t)( 0.0230 * H_GAIN),   // 24
     (int32_t)( 0.0159 * H_GAIN),   // 25
     (int32_t)( 0.0015 * H_GAIN),   // 26
    -(int32_t)( 0.0090 * H_GAIN),   // 27
    -(int32_t)( 0.0102 * H_GAIN),   // 28
    -(int32_t)( 0.0043 * H_GAIN),   // 29
     (int32_t)( 0.0023 * H_GAIN)    // 30
};

// this is for defining the modulations

typedef enum
{
    MOD_BPSK,
    MOD_QPSK,
    MOD_8PSK,
    MOD_16QAM
} MOD_TYPE;

volatile MOD_TYPE modType = MOD_BPSK;

typedef enum
{
    MODE_IDLE,
    MODE_DDS,
    MODE_STREAM
} OUTPUT_MODE;

volatile OUTPUT_MODE outputMode = MODE_IDLE;

uint8_t txData[DATA_LEN] =
{
    0xA5,0x3C,0x7E,0x91,
    0xFF,0x00,0x55,0xAA,
    0x12,0x34,0x56,0x78,
    0x9A,0xBC,0xDE,0xF0
};

volatile uint32_t samplesPerSymbol = 10;   // default
volatile uint32_t sampleCounter = 0;
volatile uint32_t bitIndex = 0;

volatile int32_t currentI = 0;
volatile int32_t currentQ = 0;

// we have fixed I/Q vaules

// BPSK
int32_t bpskI[2] = {  SYMAMP_I, -SYMAMP_I };
int32_t bpskQ[2] = {  0, 0 };

// QPSK
int32_t qpskI[4] = {  SYMAMP_I, -SYMAMP_I, -SYMAMP_I,  SYMAMP_I };
int32_t qpskQ[4] = {  SYMAMP_Q,  SYMAMP_Q, -SYMAMP_Q, -SYMAMP_Q };

// 8PSK
int32_t psk8I[8] =
{
     SYMAMP_I,
    (int32_t)(SYMAMP_I * 0.7071),
     0,
    -(int32_t)(SYMAMP_I * 0.7071),
    -SYMAMP_I,
    -(int32_t)(SYMAMP_I * 0.7071),
     0,
    (int32_t)(SYMAMP_I * 0.7071)
};

int32_t psk8Q[8] =
{
     0,
    (int32_t)(SYMAMP_Q * 0.7071),
     SYMAMP_Q,
    (int32_t)(SYMAMP_Q * 0.7071),
     0,
    -(int32_t)(SYMAMP_Q * 0.7071),
    -SYMAMP_Q,
    -(int32_t)(SYMAMP_Q * 0.7071)
};

// 16QAM — levels are ±1 and ±3 so multiply accordingly
int32_t qam16I[4] = { -3*SYMAMP_I, -SYMAMP_I,  SYMAMP_I,  3*SYMAMP_I };
int32_t qam16Q[4] = { -3*SYMAMP_Q, -SYMAMP_Q,  SYMAMP_Q,  3*SYMAMP_Q };


// ================= DDS GLOBALS =================

int16_t sineLUT[LUT_SIZE];

volatile uint32_t phase = 0;
volatile uint32_t phaseIncrement = 0;

// =================================================

// Generate sine lookup table
void initLUT()
{
    int i;
    for(i = 0; i < LUT_SIZE; i++)
    {
        /*So what this is doing here is that its making
         * i into an angle in radians
         * for a full cycle of sin from 0 to 2pi
         * and dividing it by LUT size which is
         * going to be 256 and after compiling
         * the LUT should have all the values*/
        float angle = 2.0f * PI * i / LUT_SIZE;
        sineLUT[i] = (int16_t)(Gain_I * sinf(angle));
    }
}

// Set frequency using phase accumulator
/* Function to set the phaseIncrement for phase to get what ever
 * frequency we want by dviding the feq the user inputs
 * and then / by 100kHz and times 2^32 */
void setFrequency(float freq)
{
    phaseIncrement = (uint32_t)((freq / DEFAULT_FS) * 4294967296.0);
}


// Timer1A at 100 kHz
void initTimer1A()
{
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    _delay_cycles(3);

    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER1_CFG_R = TIMER_CFG_16_BIT;
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;

    TIMER1_TAILR_R = 800;   // 80MHz / 100kHz

    TIMER1_IMR_R |= TIMER_IMR_TATOIM;
    NVIC_EN0_R |= 1 << (INT_TIMER1A - 16);

    TIMER1_CTL_R |= TIMER_CTL_TAEN;
}

// DDS ISR
void Timer1A_Handler()
{
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;

    if(outputMode == MODE_IDLE)
        return;

    uint16_t I, Q;

    if(outputMode == MODE_DDS)
    {
        phase += phaseIncrement;

        uint16_t index = phase >> (32 - LUT_LOG2_SIZE);
        uint16_t index90 = (index + (LUT_SIZE/4)) & (LUT_SIZE - 1);

        I = sineLUT[index90] + OFS_I;
        Q = sineLUT[index] + OFS_Q;
    }
    else if(outputMode == MODE_STREAM)
    {
        if(rrcEnabled)
        {
            // Just output the pre-computed value from main
            I = dacOutI;
            Q = dacOutQ;

            // Load new sample into pending and signal main to process
            if(upsampleCount == 0)
            {
                uint8_t byte  = txData[bitIndex / 8];
                uint8_t shift = 7 - (bitIndex % 8);
                uint8_t bit   = (byte >> shift) & 1;

                if(modType == MOD_BPSK)
                {
                    pendingI = bpskI[bit];
                    pendingQ = bpskQ[bit];
                    bitIndex += 1;
                }
                else if(modType == MOD_QPSK)
                {
                    uint8_t bit2 = (txData[(bitIndex+1)/8] >> (7-((bitIndex+1)%8))) & 1;
                    uint8_t symbol = (bit << 1) | bit2;
                    pendingI = qpskI[symbol];
                    pendingQ = qpskQ[symbol];
                    bitIndex += 2;
                }
                else if(modType == MOD_8PSK)
                {
                    uint8_t bit2 = (txData[(bitIndex+1)/8] >> (7-((bitIndex+1)%8))) & 1;
                    uint8_t bit3 = (txData[(bitIndex+2)/8] >> (7-((bitIndex+2)%8))) & 1;
                    uint8_t symbol = (bit << 2) | (bit2 << 1) | bit3;
                    pendingI = psk8I[symbol];
                    pendingQ = psk8Q[symbol];
                    bitIndex += 3;
                }
                else
                {
                    uint8_t bit2 = (txData[(bitIndex+1)/8] >> (7-((bitIndex+1)%8))) & 1;
                    uint8_t bit3 = (txData[(bitIndex+2)/8] >> (7-((bitIndex+2)%8))) & 1;
                    uint8_t bit4 = (txData[(bitIndex+3)/8] >> (7-((bitIndex+3)%8))) & 1;
                    uint8_t symbol = (bit << 3) | (bit2 << 2) | (bit3 << 1) | bit4;
                    pendingI = qam16I[(symbol >> 2) & 3];
                    pendingQ = qam16Q[symbol & 3];
                    bitIndex += 4;
                }

                if(bitIndex >= DATA_LEN * 8) bitIndex = 0;
            }
            else
            {
                // ticks and adds the zero padding
                pendingI = 0;
                pendingQ = 0;
            }

            upsampleCount++;
            if(upsampleCount >= UPSAMPLE) upsampleCount = 0;

            newSampleReady = true;   // tell main to run the FIR
        }
        else
        {
            // No RRC - original fast path
            if(upsampleCount == 0)
            {
                uint8_t byte  = txData[bitIndex / 8];
                uint8_t shift = 7 - (bitIndex % 8);
                uint8_t bit   = (byte >> shift) & 1;

                if(modType == MOD_BPSK)
                {
                    currentI = bpskI[bit];
                    currentQ = bpskQ[bit];
                    bitIndex += 1;
                }
                else if(modType == MOD_QPSK)
                {
                    uint8_t bit2 = (txData[(bitIndex+1)/8] >> (7-((bitIndex+1)%8))) & 1;
                    uint8_t symbol = (bit << 1) | bit2;
                    currentI = qpskI[symbol];
                    currentQ = qpskQ[symbol];
                    bitIndex += 2;
                }
                else if(modType == MOD_8PSK)
                {
                    uint8_t bit2 = (txData[(bitIndex+1)/8] >> (7-((bitIndex+1)%8))) & 1;
                    uint8_t bit3 = (txData[(bitIndex+2)/8] >> (7-((bitIndex+2)%8))) & 1;
                    uint8_t symbol = (bit << 2) | (bit2 << 1) | bit3;
                    currentI = psk8I[symbol];
                    currentQ = psk8Q[symbol];
                    bitIndex += 3;
                }
                else
                {
                    uint8_t bit2 = (txData[(bitIndex+1)/8] >> (7-((bitIndex+1)%8))) & 1;
                    uint8_t bit3 = (txData[(bitIndex+2)/8] >> (7-((bitIndex+2)%8))) & 1;
                    uint8_t bit4 = (txData[(bitIndex+3)/8] >> (7-((bitIndex+3)%8))) & 1;
                    uint8_t symbol = (bit << 3) | (bit2 << 2) | (bit3 << 1) | bit4;
                    currentI = qam16I[(symbol >> 2) & 3];
                    currentQ = qam16Q[symbol & 3];
                    bitIndex += 4;
                }

                if(bitIndex >= DATA_LEN * 8) bitIndex = 0;
            }

            upsampleCount++;
            if(upsampleCount >= UPSAMPLE) upsampleCount = 0;

            I = (uint16_t)(OFS_I + currentI);
            Q = (uint16_t)(OFS_Q + currentQ);
        }
    }

    uint16_t spiI = 0x3000 | (I & 0x0FFF);
    uint16_t spiQ = 0xB000 | (Q & 0x0FFF);

    writeSpi1Data(spiI);
    writeSpi1Data(spiQ);

    LDAC = 0;
    _delay_cycles(2);
    LDAC = 1;
}

int main(void)
{
    enablePort(PORTA);
    selectPinPushPullOutput(PORTA,4);
    LDAC = 1;

    initSystemClockTo80Mhz();
    initSysTick();

    initUart0();
    setUart0BaudRate(115200,FCYC);

    initSpi1(USE_SSI_FSS);
    setSpi1BaudRate(FDAC, FCYC);
    setSpi1Mode(0,0);

    initLUT();
    initTimer1A();

    putsUart0("\r\nIQ DAC Control Ready\r\n");
    putsUart0("Type 'help' to see available commands\r\n");

    while(1)
    {
        if(newSampleReady)
        {
            newSampleReady = false;

            firI[firIdx] = pendingI;
            firQ[firIdx] = pendingQ;

            int32_t accI = 0, accQ = 0;
            int k = firIdx;
            int n;

            // convolution
            for(n = 0; n < FIR_LEN; n++)
            {
                accI += h_rrc[n] * firI[k];
                accQ += h_rrc[n] * firQ[k];
                k--;
                if(k < 0) k = FIR_LEN - 1;
            }

            firIdx++;
            if(firIdx >= FIR_LEN) firIdx = 0;

            // here we are shifting becuase 65536 so it has to be 14 *4
            int32_t outI = accI >> 14;
            int32_t outQ = accQ >> 14;

            int32_t dI = OFS_I + outI;
            int32_t dQ = OFS_Q + outQ;

            if(dI < 0)    dI = 0;
            if(dI > 4095) dI = 4095;
            if(dQ < 0)    dQ = 0;
            if(dQ > 4095) dQ = 4095;

            dacOutI = (uint16_t)dI;
            dacOutQ = (uint16_t)dQ;
        }

        if(kbhitUart0())
        {
            USER_DATA data;
            bool valid = false;

            putsUart0("\r\n ");
            getsUart0(&data);
            parseFields(&data);

            if(isCommand(&data, "set", 2))
            {
                char *channel = getFieldString(&data, 1);
                int32_t value = getFieldInteger(&data, 2);

                if(value >= 0 && value <= 4095)
                {
                    uint16_t spiWord = value & 0x0FFF;

                    if(channel[0] == 'i' || channel[0] == 'I')
                        spiWord |= 0x3000;
                    else if(channel[0] == 'q' || channel[0] == 'Q')
                        spiWord |= 0xB000;
                    else
                    {
                        putsUart0("Invalid channel\r\n");
                        valid = true;
                        goto end_set;
                    }

                    writeSpi1Data(spiWord);
                    LDAC = 0; _delay_cycles(3); LDAC = 1;
                    valid = true;
                }
            end_set:;
            }

            else if(isCommand(&data, "help", 0))
            {
                putsUart0("\r\nAvailable Commands:\r\n");
                putsUart0("  dc i <voltage>         - Set I channel DC (-0.5 to 0.5 V)\r\n");
                putsUart0("  dc q <voltage>         - Set Q channel DC (-0.5 to 0.5 V)\r\n");
                putsUart0("  freq <Hz>              - Start waveform at given frequency\r\n");
                putsUart0("  stop                   - Stop waveform\r\n");
                putsUart0("  run                    - Start streaming modulation\r\n");
                putsUart0("  mod <modulation mode>  - BPSK, QPSK, 8PSK, 16QAM\r\n");
                putsUart0("  rrc on/off             - Enable or disable RRC filter\r\n\r\n");
                valid = true;
            }

            else if(isCommand(&data, "run", 0))
            {
                bitIndex      = 0;
                sampleCounter = 0;
                upsampleCount = 0;
                outputMode    = MODE_STREAM;
                putsUart0("Streaming started\r\n");
                valid = true;
            }

            else if(isCommand(&data, "mod", 1))
            {
                char *type = getFieldString(&data, 1);

                if(strcmp(type, "bpsk") == 0)
                    modType = MOD_BPSK;
                else if(strcmp(type, "qpsk") == 0)
                    modType = MOD_QPSK;
                else if(strcmp(type, "8psk") == 0)
                    modType = MOD_8PSK;
                else if(strcmp(type, "16qam") == 0)
                    modType = MOD_16QAM;
                else
                {
                    putsUart0("Unknown modulation\r\n");
                    valid = true;
                    goto end_mod;
                }

                putsUart0("Modulation set\r\n");
                valid = true;
            end_mod:;
            }

            else if(isCommand(&data, "dc", 2))
            {
                char *channel = getFieldString(&data, 1);
                float voltage = getFieldFloat(&data, 2);

                if(voltage >= -0.5 && voltage <= 0.5)
                {
                    uint16_t dacValue = 0;

                    if(channel[0] == 'i' || channel[0] == 'I')
                        dacValue = OFS_I - (voltage * (Gain_I / 0.5));
                    else if(channel[0] == 'q' || channel[0] == 'Q')
                        dacValue = OFS_Q - (voltage * (Gain_Q / 0.5));
                    else
                    {
                        putsUart0("Invalid channel\r\n");
                        valid = true;
                        goto end_dc;
                    }

                    if(dacValue > 4095) dacValue = 4095;
                    if((int32_t)dacValue < 0) dacValue = 0;

                    uint16_t spiWord =
                        ((channel[0]=='i'||channel[0]=='I') ? 0x3000 : 0xB000)
                        | (dacValue & 0x0FFF);

                    writeSpi1Data(spiWord);
                    LDAC = 0; _delay_cycles(5); LDAC = 1;

                    putsUart0("Channel ");
                    putcUart0(channel[0]);
                    putsUart0(" set to ");

                    int32_t voltage_mV = (int32_t)(voltage * 1000);

                    if(voltage_mV < 0)
                    {
                        putcUart0('-');
                        voltage_mV = -voltage_mV;
                    }

                    print_int(voltage_mV / 1000);
                    putcUart0('.');

                    int32_t frac = voltage_mV % 1000;

                    if(frac < 100) putcUart0('0');
                    if(frac < 10)  putcUart0('0');

                    print_int(frac);

                    putsUart0(" V (DAC=");
                    print_int(dacValue);
                    putsUart0(")\r\n");

                    valid = true;
                }
            end_dc:;
            }

            else if(isCommand(&data, "freq", 1))
            {
                float freq = getFieldFloat(&data, 1);

                if(freq > 0 && freq < (DEFAULT_FS / 2))
                {
                    phase = 0;
                    setFrequency(freq);
                    outputMode = MODE_DDS;
                    putsUart0("freq ON at ");
                    print_int(freq);
                    putsUart0(" Hz\r\n");
                }
                else
                {
                    putsUart0("Invalid frequency\r\n");
                }

                valid = true;
            }

            else if(isCommand(&data, "stop", 0))
            {
                outputMode = MODE_IDLE;
                putsUart0("Wave stopped\r\n");
                valid = true;
            }

            else if(isCommand(&data, "rrc", 1))
            {
                char *onoff = getFieldString(&data, 1);

                if(strcmp(onoff, "on") == 0)
                {
                    outputMode = MODE_IDLE;

                    int i;
                    for(i = 0; i < FIR_LEN; i++)
                    {
                        firI[i] = 0;
                        firQ[i] = 0;
                    }

                    firIdx        = 0;
                    upsampleCount = 0;
                    bitIndex      = 0;
                    rrcEnabled    = true;

                    outputMode = MODE_STREAM;
                    putsUart0("RRC ON\r\n");
                }
                else
                {
                    outputMode    = MODE_IDLE;
                    rrcEnabled    = false;
                    upsampleCount = 0;
                    bitIndex      = 0;
                    outputMode    = MODE_STREAM;
                    putsUart0("RRC OFF\r\n");
                }

                valid = true;
            }

            if(!valid)
                putsUart0("Invalid Command\r\n");
        }
    }
}
