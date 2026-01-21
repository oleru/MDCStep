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

static TLV_STATE tlvState = TLV_ST_RESET;
static uint32_t  tlvT0;
static volatile bool i2cDone = false;

static uint8_t tlvFactory[10];
static uint8_t tlvData[7];

static uint8_t tlvResetBuf[1] = {0x00};

/* konfig-buffer ? justeres etter factory-read */
static uint8_t tlvConfigBuf[4] = {
    0x00,   /* register pointer */
    0x00,   /* MOD1 */
    0x00,   /* reserved */
    0x00    /* MOD2 */
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
void TLV_Task(void);

/* ===================== CoreTimer callback ===================== */
void myCORETIMER(uint32_t status, uintptr_t context)
{
    TimerEvent1ms = true;
    myTime++;
}

/* ===================== I2C callback ===================== */
static void I2C_Callback(uintptr_t context)
{
    i2cDone = true;
}

/* ===================== TLV493D state machine ===================== */
void TLV_Task(void)
{
    switch (tlvState)
    {
        case TLV_ST_RESET:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C1_Write(0x00, tlvResetBuf, 1);   /* general reset */
                tlvT0 = myTime;
                tlvState = TLV_ST_WAIT_RESET;
            }
            break;

        case TLV_ST_WAIT_RESET:
            if ((myTime - tlvT0) >= 20) {          /* >14 µs, god margin */
                tlvState = TLV_ST_READ_FACTORY;
            }
            break;

        case TLV_ST_READ_FACTORY:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C1_Read(TLV_ADDR, tlvFactory, 10);
                tlvState = TLV_ST_WAIT_FACTORY;
            }
            break;

        case TLV_ST_WAIT_FACTORY:
            if (i2cDone) {
                /* MOD1: bevar IICAddr + reserved, sett LOW=1 */
                tlvConfigBuf[1] = (tlvFactory[7] & 0x60) | 0x01;

                /* reserved */
                tlvConfigBuf[2] = tlvFactory[8];

                /* MOD2: bevar reserved, sett LP=1 (12 ms), T=0 */
                tlvConfigBuf[3] = (tlvFactory[9] & 0x1F) | 0x40;

                tlvState = TLV_ST_WRITE_CONFIG;
            }
            break;

        case TLV_ST_WRITE_CONFIG:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C1_Write(TLV_ADDR, tlvConfigBuf, 4);
                tlvState = TLV_ST_WAIT_CONFIG;
            }
            break;

        case TLV_ST_WAIT_CONFIG:
            if (i2cDone) {
                tlvState = TLV_ST_READ_DATA;
            }
            break;

        case TLV_ST_READ_DATA:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C1_Read(TLV_ADDR, tlvData, 7);
                tlvState = TLV_ST_WAIT_DATA;
            }
            break;

        case TLV_ST_WAIT_DATA:
            if (i2cDone) {
                /* parse rådata */
                mag.x = ((tlvData[0] << 4) | (tlvData[4] >> 4));
                if (mag.x & 0x800) mag.x -= 0x1000;

                mag.y = ((tlvData[1] << 4) | (tlvData[4] & 0x0F));
                if (mag.y & 0x800) mag.y -= 0x1000;

                mag.z = ((tlvData[2] << 4) | (tlvData[5] & 0x0F));
                if (mag.z & 0x800) mag.z -= 0x1000;

                mag.temperature = tlvData[3];
                mag.frame   = (tlvData[3] >> 2) & 0x03;
                mag.channel = tlvData[3] & 0x03;
                mag.powerDown = (tlvData[5] >> 4) & 0x01;

                tlvState = TLV_ST_READ_DATA;   /* kontinuerlig */
            }
            break;
    }
}

/* ===================== Timere (URØRT) ===================== */
void UpdateTimers(void)
{
    static int32_t my10msCnt, my50msCnt, my250msCnt, my1000msCnt, my15sCnt;
    int32_t delta;

    delta = myTime - myLastTime;
    myLastTime = myTime;

    my10msCnt += delta;
    MBS_TimerValue += delta;

    while (my10msCnt >= 10)
    {
        TimerEvent10ms = true;
        my10msCnt -= 10;

        my50msCnt += 10;
        while (my50msCnt >= 50)
        {
            TimerEvent50ms = true;
            my50msCnt -= 50;

            my250msCnt += 50;
            while (my250msCnt >= 250)
            {
                TimerEvent250ms = true;
                my250msCnt -= 250;
            }

            my1000msCnt += 50;
            while (my1000msCnt >= 1000)
            {
                TimerEvent1s = true;
                my1000msCnt -= 1000;
                mySystemTimeOutTimer++;

                my15sCnt++;
                if (my15sCnt >= 15)
                {
                    TimerEvent15s = true;
                    my15sCnt = 0;
                }
            }
        }
    }
}

/* ===================== main ===================== */
int main(void)
{
    uint8_t RdBuffer[10];

    SYS_Initialize(NULL);

    CORETIMER_CallbackSet(myCORETIMER, 0);
    CORETIMER_Start();

    I2C1_CallbackRegister(I2C_Callback, 0);

    MBS_InitModbus(10);

    while (true)
    {
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

        MBS_ProcessModbus();
        WDTCONbits.WDTCLRKEY = 0x5743;
    }
}
