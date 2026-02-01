/*
 * main.c ? stabil, ikke-blokkerende TLV493D-integrasjon
 *  - TLV_Task kjøres hvert 50 ms
 *  - Data eksponeres til Modbus hvert 250 ms
 *  - Eksisterende timer / Modbus / UART beholdt
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "definitions.h"

#include "ModbusSlave.h"
#include "tlv493d.h"   /* ligger i prosjektet, men brukes ikke direkte */

/* ===================== Konstanter ===================== */
#define TLV_ADDR   0x1F

#define I2C_TIMEOUT_MS      80u
#define I2C_BUSY_MAX_MS     200u
#define TLV_MAX_FAILS       3u


/* ===================== TLV493D state machine ===================== */
typedef enum {
    TLV_ST_RESET = 0,
    TLV_ST_WAIT_RESET,
    TLV_ST_READ_FACTORY,
    TLV_ST_WAIT_FACTORY,
    TLV_ST_WRITE_CONFIG,
    TLV_ST_WAIT_CONFIG,
    TLV_ST_READ_DATA,
    TLV_ST_WAIT_DATA
} TLV_STATE;

//static uint8_t tlvAddr = TLV_ADDR; // kan settes til 0x5E om nødvendig
//static uint8_t lastFrm = 0xFF;

static TLV_STATE tlvState = TLV_ST_RESET;
static uint32_t tlvT0;
static volatile bool i2cDone = false;

static uint8_t tlvFactory[10];
static uint8_t tlvData[7];

static uint8_t tlvResetBuf[1] = {0x00};

static uint32_t i2cStartT = 0;
static uint32_t i2cBusyT  = 0;
static uint8_t  tlvFails  = 0;


/* konfig-buffer ? justeres etter factory-read */
static uint8_t tlvConfigBuf[4] = {
    0x00, /* register pointer */
    0x00, /* MOD1 */
    0x00, /* reserved */
    0x00 /* MOD2 */
};

/* ===================== Timer / Modbus ===================== */
volatile bool TimerEvent1ms;
volatile uint32_t myTime = 0;
bool TimerEvent10ms = false, TimerEvent50ms = false;
bool TimerEvent250ms = false, TimerEvent1s = false, TimerEvent15s = false;
uint32_t myLastTime = 0;
uint32_t mySystemTimeOutTimer;

TLV493D_Data_t mag;

/* ===================== Prototyper ===================== */
void UpdateTimers(void);
void MBS_UART_Putch(uint8_t ch);
void TLV_Task(void);

/* ===================== CoreTimer callback ===================== */
void myCORETIMER(uint32_t status, uintptr_t context) {
    TimerEvent1ms = true;
    myTime++;
}

/* ===================== I2C callback ===================== */
static void I2C_Callback(uintptr_t context) {
    i2cDone = true;
}

// Callback function for the ModBus Slave driver

void MBS_UART_Putch(uint8_t ch) {
    UART1_Write(&ch, 1);
}

static uint8_t popcount8(uint8_t v) {
    v = v - ((v >> 1) & 0x55);
    v = (v & 0x33) + ((v >> 2) & 0x33);
    return (v + (v >> 4)) & 0x0F;
}

static void TLV_SetOddParity(uint8_t w[4]) {
    // Parity bit er bit7 i w[1] (MOD1). Vi beregner med parity=0 først:
    uint8_t mod1_no_p = (uint8_t) (w[1] & 0x7F);

    uint32_t ones =
            popcount8(w[0]) +
            popcount8(mod1_no_p) +
            popcount8(w[2]) +
            popcount8(w[3]);

    // Totalen skal være ODD. Hvis den er EVEN, sett parity-bit = 1.
    if ((ones & 1u) == 0u) w[1] = (uint8_t) (mod1_no_p | 0x80);
    else w[1] = (uint8_t) (mod1_no_p);
}

static inline void TLV_FailReset(void)
{
    // Minimal recovery: gå tilbake til reset-sekvensen.
    // (Evt. legg inn I2C peripheral reset/bus clear senere.)
    tlvState = TLV_ST_RESET;
}

/* ===================== TLV493D state machine ===================== */
// --- tunables ---

static inline void I2C_GuardStart(void) { i2cStartT = myTime; }
static inline bool I2C_GuardTimeout(void){ return (uint32_t)(myTime - i2cStartT) > I2C_TIMEOUT_MS; }

static inline void TLV_FailStep(void)
{
    if (++tlvFails >= TLV_MAX_FAILS) {
        tlvFails = 0;
        tlvState = TLV_ST_RESET;      // full reinit
    } else {
        tlvState = TLV_ST_READ_DATA;  // try resume reads
    }
}

void TLV_Task(void)
{
    // --- Busy watchdog ---
    if (I2C1_IsBusy()) {
        if (i2cBusyT == 0) i2cBusyT = myTime;
        if ((uint32_t)(myTime - i2cBusyT) > I2C_BUSY_MAX_MS) {
            i2cBusyT = 0;
            i2cDone = false;
            tlvState = TLV_ST_RESET;
        }
    } else {
        i2cBusyT = 0;
    }

    switch (tlvState)
    {
        case TLV_ST_RESET:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C_GuardStart();
                I2C1_Write(0x00, tlvResetBuf, 1);   // general call reset
                tlvT0 = myTime;
                tlvState = TLV_ST_WAIT_RESET;
            }
            break;

        case TLV_ST_WAIT_RESET:
            if ((uint32_t)(myTime - tlvT0) >= 20u) {
                tlvState = TLV_ST_READ_FACTORY;
            }
            break;

        case TLV_ST_READ_FACTORY:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C_GuardStart();
                I2C1_Read(TLV_ADDR, tlvFactory, 10);
                tlvState = TLV_ST_WAIT_FACTORY;
            }
            break;

        case TLV_ST_WAIT_FACTORY:
            if (i2cDone) {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
                    TLV_FailStep();
                    break;
                }

                // Build write regs W0..W3
                tlvConfigBuf[0] = 0x00;          // W0
                tlvConfigBuf[2] = tlvFactory[8]; // W2 mirrors factory reg8

                // MOD1 (W1): preserve addr/reserved, INT=0, FAST=0, LOW=1
                // bit2 INT = 0 disables interrupt on SCL/INT pin
                uint8_t mod1 = (uint8_t)(tlvFactory[7] & 0x7F);  // clear parity
                mod1 = (uint8_t)((mod1 & 0xF8) | 0x01);          // INT=0 FAST=0 LOW=1
                tlvConfigBuf[1] = mod1;

                // MOD2 (W3): LP=1 (12ms), T=0 (temp enabled)
                uint8_t mod2 = tlvFactory[9];
                mod2 = (uint8_t)((mod2 & 0x3F) | 0x40);          // LP=1
                mod2 = (uint8_t)(mod2 & ~(1u << 7));             // T=0
                tlvConfigBuf[3] = mod2;

                // Set odd parity over W0..W3
                TLV_SetOddParity(tlvConfigBuf);

                tlvState = TLV_ST_WRITE_CONFIG;
            } else {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE || I2C_GuardTimeout()) {
                    TLV_FailStep();
                }
            }
            break;

        case TLV_ST_WRITE_CONFIG:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C_GuardStart();
                I2C1_Write(TLV_ADDR, tlvConfigBuf, 4);
                tlvState = TLV_ST_WAIT_CONFIG;
            }
            break;

        case TLV_ST_WAIT_CONFIG:
            if (i2cDone) {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
                    TLV_FailStep();
                    break;
                }
                tlvFails = 0;
                tlvState = TLV_ST_READ_DATA;
            } else {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE || I2C_GuardTimeout()) {
                    TLV_FailStep();
                }
            }
            break;

        case TLV_ST_READ_DATA:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C_GuardStart();
                I2C1_Read(TLV_ADDR, tlvData, 7);
                tlvState = TLV_ST_WAIT_DATA;
            }
            break;

        case TLV_ST_WAIT_DATA:
            if (i2cDone) {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
                    TLV_FailStep();
                    break;
                }

                uint8_t frm = (tlvData[3] >> 2) & 0x03;
                uint8_t ch  = (tlvData[3] >> 0) & 0x03;

                uint8_t reg5 = tlvData[5];
                uint8_t pd   = (reg5 >> 4) & 0x01;  // 1=conversion completed
                uint8_t ff   = (reg5 >> 5) & 0x01;
                uint8_t t    = (reg5 >> 6) & 0x01;

                // Require consistent & finished sample
                if (ch != 0 || pd == 0 || ff == 0 || t != 0) {
                    tlvState = TLV_ST_READ_DATA;
                    break;
                }

                // X/Y/Z 12-bit signed
                int16_t x = (int16_t)(((uint16_t)tlvData[0] << 4) | (tlvData[4] >> 4));
                if (x & 0x800) x -= 0x1000;

                int16_t y = (int16_t)(((uint16_t)tlvData[1] << 4) | (tlvData[4] & 0x0F));
                if (y & 0x800) y -= 0x1000;

                int16_t z = (int16_t)(((uint16_t)tlvData[2] << 4) | (tlvData[5] & 0x0F));
                if (z & 0x800) z -= 0x1000;

                // Temp 12-bit: MS nibble in reg3, full byte reg6
                int16_t rawT12 = (int16_t)(((uint16_t)(tlvData[3] & 0xF0) << 4) |
                                           ((uint16_t) tlvData[6]));
                if (rawT12 & 0x800) rawT12 -= 0x1000;

                mag.x = x; mag.y = y; mag.z = z;
                mag.temperature = rawT12;
                mag.frame = frm;
                mag.channel = ch;
                mag.powerDown = pd;

                tlvFails = 0;
                tlvState = TLV_ST_READ_DATA;
            } else {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE || I2C_GuardTimeout()) {
                    TLV_FailStep();
                }
            }
            break;

        default:
            tlvState = TLV_ST_RESET;
            break;
    }
}

/* ===================== Timere (URØRT) ===================== */
void UpdateTimers(void) {
    static int32_t my10msCnt, my50msCnt, my250msCnt, my1000msCnt, my15sCnt;
    int32_t delta;

    delta = myTime - myLastTime;
    myLastTime = myTime;

    my10msCnt += delta;
    MBS_TimerValue += delta;

    // Update Time Keepers
    while (my10msCnt >= 10) {
        TimerEvent10ms = true;
        my10msCnt -= 10;

        my50msCnt += 10;
        while (my50msCnt >= 50) {
            TimerEvent50ms = true;
            my50msCnt -= 50;

            my250msCnt += 50;
            while (my250msCnt >= 250) {
                TimerEvent250ms = true;
                my250msCnt -= 250;

                my1000msCnt += 50;
                while (my1000msCnt >= 1000) {
                    TimerEvent1s = true;
                    my1000msCnt -= 1000;
                    mySystemTimeOutTimer++;
                    my15sCnt += 1;
                    while (my15sCnt >= 15) {
                        TimerEvent15s = true;
                        my15sCnt -= 15;
                    } //..while(my15sCnt>=15)
                } //..while(my1000msCnt>=1000)
            } //..while(my250msCnt>=250)
        } //..while(my50msCnt>=50)
    } //..while(my10msCnt>=10)
}

/* ===================== main ===================== */
int main(void) {
    uint8_t myModBusAddr;
    uint8_t RdBuffer[10];


    /* Initialize all modules */
    SYS_Initialize(NULL);

    I2C1_CallbackRegister(I2C_Callback, 0);

    // Set Modbus Slave Address
    myModBusAddr = 10;
    MBS_InitModbus(myModBusAddr);
    MBS_HoldRegisters[MBS_OWN_ID_SW] =
            10 + ((SW1_8_Get() << 3)
            | (SW1_4_Get() << 2)
            | (SW1_2_Get() << 1)
            | SW1_1_Get());
    MBS_HoldRegisters[MBS_SL_MODEL] = 5; // Searchlight Model
    MBS_HoldRegisters[MBS_HD_ID] = (uint16_t) 'B'; // HW_ID 
    MBS_HoldRegisters[MBS_SW_ID] = 4; // SW_ID

    CORETIMER_CallbackSet(myCORETIMER, (uintptr_t) NULL);
    CORETIMER_Start();

    while (true) {
        SYS_Tasks();

        if (TimerEvent1ms) {
            TimerEvent1ms = false;
            UpdateTimers();
        }

        /* TLV state machine hvert 50 ms */
        if (TimerEvent50ms) {
            TimerEvent50ms = false;
            TLV_Task();
        }

        /* eksponer data hvert 250 ms */
        if (TimerEvent250ms) {
            TimerEvent250ms = false;

            MBS_HoldRegisters[10] = mag.x;
            MBS_HoldRegisters[11] = mag.y;
            MBS_HoldRegisters[12] = mag.z;
            MBS_HoldRegisters[13] = mag.temperature;
            MBS_HoldRegisters[14] = mag.frame;
            MBS_HoldRegisters[15] = mag.channel;
            MBS_HoldRegisters[16] = mag.powerDown;
        }

        if (UART1_ReadCountGet() > 0) {
            UART1_Read(RdBuffer, 1);
            MBS_ReciveData(RdBuffer[0]);
        }

        // Process Modbus
        MBS_ProcessModbus();


        // Guard the Watchdog
        WDTCONbits.WDTCLRKEY = 0x5743; // Magic sequence to reset WDT



    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE);
}


/*******************************************************************************
 End of File
 */

