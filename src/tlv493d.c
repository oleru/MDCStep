#include "definitions.h"
#include "tlv493d.h"

/* ===================== Konstanter ===================== */
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

static TLV_STATE tlvState = TLV_ST_RESET;

static uint8_t   tlvAddr = 0x1Fu;

static uint32_t  tlvT0 = 0u;
static uint32_t  nowMs = 0u;

static volatile bool i2cDone = false;

static uint8_t tlvFactory[10];
static uint8_t tlvData[7];

static uint8_t tlvResetBuf[1] = {0x00};

static uint32_t i2cStartT = 0u;
static uint32_t i2cBusyT  = 0u;
static uint8_t  tlvFails  = 0u;

/* konfig-buffer – justeres etter factory-read */
static uint8_t tlvConfigBuf[4] = {
    0x00, /* W0: register pointer */
    0x00, /* W1: MOD1 */
    0x00, /* W2: reserved */
    0x00  /* W3: MOD2 */
};

/* sample tracking */
static TLV493D_Data_t lastSample;
static uint8_t  lastFrm = 0xFFu;
static bool     sampleValid = false;
static uint32_t lastUpdateMs = 0u;

/* ===================== I2C callback ===================== */
void TLV493D_I2C_Callback(uintptr_t context)
{
    (void)context;
    i2cDone = true;
}

/* ===================== Parity helpers (MOD1 bit7) ===================== */
static uint8_t popcount8(uint8_t v)
{
    v = (uint8_t)(v - ((v >> 1) & 0x55u));
    v = (uint8_t)((v & 0x33u) + ((v >> 2) & 0x33u));
    return (uint8_t)((v + (v >> 4)) & 0x0Fu);
}

static void TLV_SetOddParity(uint8_t w[4])
{
    /* Parity bit er bit7 i w[1] (MOD1). Beregn med parity=0 først */
    uint8_t mod1_no_p = (uint8_t)(w[1] & 0x7Fu);

    uint32_t ones =
        (uint32_t)popcount8(w[0]) +
        (uint32_t)popcount8(mod1_no_p) +
        (uint32_t)popcount8(w[2]) +
        (uint32_t)popcount8(w[3]);

    /* Totalen skal være ODD. Hvis EVEN → sett parity-bit = 1 */
    if ((ones & 1u) == 0u) w[1] = (uint8_t)(mod1_no_p | 0x80u);
    else                  w[1] = (uint8_t)(mod1_no_p);
}

/* ===================== TLV state machine helpers ===================== */
static inline void I2C_GuardStart(void) { i2cStartT = nowMs; }
static inline bool I2C_GuardTimeout(void){ return (uint32_t)(nowMs - i2cStartT) > I2C_TIMEOUT_MS; }

static inline void TLV_FailStep(void)
{
    sampleValid = false;

    if (++tlvFails >= TLV_MAX_FAILS) {
        tlvFails = 0u;
        tlvState = TLV_ST_RESET;      /* full reinit */
    } else {
        tlvState = TLV_ST_READ_DATA;  /* prøv å fortsette lesing */
    }
}

/* ===================== Public API ===================== */
void TLV493D_Init(uint8_t i2c_addr)
{
    tlvAddr = i2c_addr;

    tlvState = TLV_ST_RESET;
    tlvT0 = 0u;
    i2cDone = false;

    i2cStartT = 0u;
    i2cBusyT = 0u;
    tlvFails = 0u;

    lastFrm = 0xFFu;
    sampleValid = false;
    lastUpdateMs = 0u;

    /* Clear sample */
    lastSample.x = 0;
    lastSample.y = 0;
    lastSample.z = 0;
    lastSample.temperature = 0;
    lastSample.frame = 0;
    lastSample.channel = 0;
    lastSample.powerDown = false;
}

void TLV493D_Task(uint32_t now_ms)
{
    nowMs = now_ms;

    /* --- Busy watchdog --- */
    if (I2C1_IsBusy()) {
        if (i2cBusyT == 0u) i2cBusyT = nowMs;
        if ((uint32_t)(nowMs - i2cBusyT) > I2C_BUSY_MAX_MS) {
            i2cBusyT = 0u;
            i2cDone = false;
            tlvState = TLV_ST_RESET;
            sampleValid = false;
        }
    } else {
        i2cBusyT = 0u;
    }

    switch (tlvState)
    {
        case TLV_ST_RESET:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C_GuardStart();
                I2C1_Write(0x00, tlvResetBuf, 1);   /* general call reset */
                tlvT0 = nowMs;
                tlvState = TLV_ST_WAIT_RESET;
                sampleValid = false;
                lastFrm = 0xFFu;
            }
            break;

        case TLV_ST_WAIT_RESET:
            /* 20ms holder i praksis */
            if ((uint32_t)(nowMs - tlvT0) >= 20u) {
                tlvState = TLV_ST_READ_FACTORY;
            }
            break;

        case TLV_ST_READ_FACTORY:
            if (!I2C1_IsBusy()) {
                i2cDone = false;
                I2C_GuardStart();
                I2C1_Read(tlvAddr, tlvFactory, 10);
                tlvState = TLV_ST_WAIT_FACTORY;
            }
            break;

        case TLV_ST_WAIT_FACTORY:
            if (i2cDone) {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
                    TLV_FailStep();
                    break;
                }

                /* Bygg write regs W0..W3 */
                tlvConfigBuf[0] = 0x00u;          /* W0 */
                tlvConfigBuf[2] = tlvFactory[8];  /* W2 mirrors factory reg8 */

                /*
                 * MOD1 (W1):
                 *  - behold IICAddr og reserved bits fra factory
                 *  - INT=0, FAST=0, LOW=1
                 *  - parity settes etterpå
                 */
                uint8_t mod1 = (uint8_t)(tlvFactory[7] & 0x7Fu);     /* clear parity */
                mod1 = (uint8_t)((mod1 & 0xF8u) | 0x01u);            /* ..xxx000 + LOW */
                tlvConfigBuf[1] = mod1;

                /*
                 * MOD2 (W3):
                 *  - behold reserved [4:0]
                 *  - LP=1 (12ms), T=0 (temp enabled)
                 *  - PT beholdes fra factory (typisk 1)
                 */
                uint8_t mod2 = tlvFactory[9];
                mod2 = (uint8_t)((mod2 & 0x3Fu) | 0x40u);            /* LP=1 */
                mod2 = (uint8_t)(mod2 & ~(1u << 7));                 /* T=0 */
                tlvConfigBuf[3] = mod2;

                /* Sett odd parity over W0..W3 */
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
                I2C1_Write(tlvAddr, tlvConfigBuf, 4);
                tlvState = TLV_ST_WAIT_CONFIG;
            }
            break;

        case TLV_ST_WAIT_CONFIG:
            if (i2cDone) {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
                    TLV_FailStep();
                    break;
                }
                tlvFails = 0u;
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
                I2C1_Read(tlvAddr, tlvData, 7);
                tlvState = TLV_ST_WAIT_DATA;
            }
            break;

        case TLV_ST_WAIT_DATA:
            if (i2cDone) {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
                    TLV_FailStep();
                    break;
                }

                uint8_t frm = (uint8_t)((tlvData[3] >> 2) & 0x03u);
                uint8_t ch  = (uint8_t)((tlvData[3] >> 0) & 0x03u);

                uint8_t reg5 = tlvData[5];
                uint8_t pd   = (uint8_t)((reg5 >> 4) & 0x01u);  /* 1=conversion completed */
                uint8_t ff   = (uint8_t)((reg5 >> 5) & 0x01u);
                uint8_t t    = (uint8_t)((reg5 >> 6) & 0x01u);

                /*
                 * Validering:
                 *  - CH==0 (konsistent sample)
                 *  - PD==1 (ferdig)
                 *  - FF==1 (ok)
                 *  - T==0 (ikke testmode)
                 *  - FRM må endre seg (ny sample)
                 */
                if (ch != 0u || pd == 0u || ff == 0u || t != 0u || frm == lastFrm) {
                    sampleValid = false;
                    tlvState = TLV_ST_READ_DATA;
                    break;
                }

                /* X/Y/Z 12-bit signed */
                int16_t x = (int16_t)(((uint16_t)tlvData[0] << 4) | (uint16_t)(tlvData[4] >> 4));
                if (x & 0x0800) x = (int16_t)(x - 0x1000);

                int16_t y = (int16_t)(((uint16_t)tlvData[1] << 4) | (uint16_t)(tlvData[4] & 0x0Fu));
                if (y & 0x0800) y = (int16_t)(y - 0x1000);

                int16_t z = (int16_t)(((uint16_t)tlvData[2] << 4) | (uint16_t)(tlvData[5] & 0x0Fu));
                if (z & 0x0800) z = (int16_t)(z - 0x1000);

                /* Temp 12-bit: MS nibble in reg3, LSB byte in reg6 */
                int16_t rawT12 = (int16_t)(((uint16_t)(tlvData[3] & 0xF0u) << 4) |
                                           ((uint16_t) tlvData[6]));
                if (rawT12 & 0x0800) rawT12 = (int16_t)(rawT12 - 0x1000);

                lastSample.x = x;
                lastSample.y = y;
                lastSample.z = z;
                lastSample.temperature = rawT12;
                lastSample.frame = frm;
                lastSample.channel = ch;
                lastSample.powerDown = (pd != 0u);

                lastFrm = frm;
                sampleValid = true;
                lastUpdateMs = nowMs;

                tlvFails = 0u;
                tlvState = TLV_ST_READ_DATA;
            } else {
                if (I2C1_ErrorGet() != I2C_ERROR_NONE || I2C_GuardTimeout()) {
                    TLV_FailStep();
                }
            }
            break;

        default:
            tlvState = TLV_ST_RESET;
            sampleValid = false;
            break;
    }
}

bool TLV493D_GetLatest(TLV493D_Data_t *out, uint32_t now_ms, uint32_t *age_ms)
{
    if (out != NULL) {
        *out = lastSample;
    }
    if (age_ms != NULL) {
        *age_ms = (sampleValid ? (uint32_t)(now_ms - lastUpdateMs) : 0xFFFFFFFFu);
    }
    return sampleValid;
}
