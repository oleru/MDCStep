/*
 * main.c ? stabil, ikke-blokkerende TLV493D-integrasjon
 *  - TLV_Task kjres hvert 50 ms
 *  - Data eksponeres til Modbus hvert 250 ms
 *  - Eksisterende timer / Modbus / UART beholdt
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "definitions.h"

#include "ModbusSlave.h"
#include "tlv493d.h"   /* TLV493D driver */
#include "endstop.h"

/* ===================== Konstanter ===================== */
#define TLV_ADDR            0x1F

/* ===================== Timer / Modbus ===================== */

volatile bool TimerEvent1ms;
volatile uint32_t myTime = 0;
bool TimerEvent10ms = false, TimerEvent50ms = false;
bool TimerEvent250ms = false, TimerEvent1s = false, TimerEvent15s = false;
uint32_t myLastTime = 0;
uint32_t mySystemTimeOutTimer;

unsigned char Blink=0xF0;


TLV493D_Data_t mag;

/* ===================== Prototyper ===================== */
void UpdateTimers(void);
void MBS_UART_Putch(uint8_t ch);


/* ===================== CoreTimer callback ===================== */
void myCORETIMER(uint32_t status, uintptr_t context)
{
    (void)status; (void)context;
    TimerEvent1ms = true;
    myTime++;
}

/* Callback function for the ModBus Slave driver */
void MBS_UART_Putch(uint8_t ch)
{
    UART1_Write(&ch, 1);
}


/* ===================== Timere (URRT ? men med riktig nesting) ===================== */
void UpdateTimers(void)
{
    static int32_t my10msCnt, my50msCnt, my250msCnt, my1000msCnt, my15sCnt;
    int32_t delta;

    delta = (int32_t)(myTime - myLastTime);
    myLastTime = myTime;

    my10msCnt += delta;
    MBS_TimerValue += (uint32_t)delta; /* Modbus 1ms Time Keeper */

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
            }

            my1000msCnt += 50;
            while (my1000msCnt >= 1000) {
                TimerEvent1s = true;
                my1000msCnt -= 1000;
                mySystemTimeOutTimer++;

                my15sCnt += 1;
                while (my15sCnt >= 15) {
                    TimerEvent15s = true;
                    my15sCnt -= 15;
                }
            }
        }
    }
}

/* ===================== main ===================== */
int main(void)
{
    uint8_t myModBusAddr;
    uint8_t RdBuffer[10];
    int16_t headingDeg = 0;
    int16_t tempC = 0;
    uint32_t cmdTimeOutTimer=0;
    uint8_t BlinkCnt = 0;


    /* Initialize all modules */
    SYS_Initialize(NULL);

    // Endstop inputs are configured by MCC (GPIO_Initialize) already.
    // This driver uses the existing 1ms CoreTimer tick for edge-detection.
    ENDSTOP_Init();

    I2C1_CallbackRegister(TLV493D_I2C_Callback, 0);
    TLV493D_Init(TLV_ADDR);

    /* Set Modbus Slave Address */
    myModBusAddr = 10;
    MBS_InitModbus(myModBusAddr);

    MBS_HoldRegisters[MBS_OWN_ID_SW] =
        10 + ((SW1_8_Get() << 3) |
              (SW1_4_Get() << 2) |
              (SW1_2_Get() << 1) |
              (SW1_1_Get()));

    MBS_HoldRegisters[MBS_SL_MODEL] = 6;            /* Searchlight Model */
    MBS_HoldRegisters[MBS_HD_ID] = (uint16_t)'-';   /* HW_ID */
    MBS_HoldRegisters[MBS_SW_ID] = 1;               /* SW_ID */

    CORETIMER_CallbackSet(myCORETIMER, (uintptr_t)NULL);
    CORETIMER_Start();

    while (true) {
        SYS_Tasks();

        if (TimerEvent1ms) {
            TimerEvent1ms = false;
            UpdateTimers();
            ENDSTOP_Task_1ms();
        }

        /* TLV state machine hvert 50 ms */
        if (TimerEvent50ms) {
            TimerEvent50ms = false;
            TLV493D_Task(myTime);
            
            if(cmdTimeOutTimer==0) {
                if(Blink!=0x0A) {
                    Blink=0x0F;  // Standby - No bus
                }
            } else {
                Blink=0xAA;  // Communication is detected...
            }
            
            // Active Command?
            if(cmdTimeOutTimer>50) {
                cmdTimeOutTimer -= 50;
            } else if(cmdTimeOutTimer==50) {

                // Control Timeout
                cmdTimeOutTimer = 0;
                MBS_HoldRegisters[MBS_SL_STATUS] = 0x0000;  // Clear SLStaus
            }

        }

        /* eksponer data hvert 250 ms */
        if (TimerEvent250ms) {
            TimerEvent250ms = false;

            // Status blink
            BlinkCnt++;
            BlinkCnt &= 0x07;
            if(Blink&(0x01<<BlinkCnt)) {
                BLUE_LED_Set();
            } else {
                BLUE_LED_Clear();    
            }
                        
            uint32_t tlvAgeMs = 0;
            bool tlvValid = TLV493D_GetLatest(&mag, myTime, &tlvAgeMs);

            MBS_HoldRegisters[MBS_TLV493D_X] = (uint16_t)mag.x;
            MBS_HoldRegisters[MBS_TLV493D_Y] = (uint16_t)mag.y;
            MBS_HoldRegisters[MBS_TLV493D_Z] = (uint16_t)mag.z;
            MBS_HoldRegisters[MBS_TLV493D_TEMP] = (uint16_t)(int16_t)mag.temperature;
            MBS_HoldRegisters[MBS_TLV493D_FRAME] = (uint16_t)mag.frame;
            MBS_HoldRegisters[MBS_TLV493D_CH] = (uint16_t)mag.channel;
            MBS_HoldRegisters[MBS_TLV493D_PWRDOWN] = (uint16_t)mag.powerDown;

            (void)TLV493D_GetHeadingTemp(&headingDeg, &tempC, myTime, NULL);
            MBS_HoldRegisters[MBS_TLV493D_HEADING] = (uint16_t)(int16_t)headingDeg; /* [-180..180] */
            MBS_HoldRegisters[MBS_TLV493D_TEMP_C] = (uint16_t)(int16_t)tempC; /* whole C */

            MBS_HoldRegisters[MBS_TLV493D_VALID] = (uint16_t)(tlvValid ? 1u : 0u);
            MBS_HoldRegisters[MBS_TLV493D_AGE] = (uint16_t)tlvAgeMs; /* ms siden sist gyldig */
        }

        
        if (TimerEvent1s) {
            TimerEvent1s = false;

            MBS_HoldRegisters[MBS_OWN_ID_SW] =
            10 + ((SW1_8_Get() << 3) |
                  (SW1_4_Get() << 2) |
                  (SW1_2_Get() << 1) |
                  (SW1_1_Get()));
        
        } //..if (TimerEvent1s)
            
        if (UART1_ReadCountGet() > 0) {
            UART1_Read(RdBuffer, 1);
            MBS_ReciveData(RdBuffer[0]);
            
            //cmdTimeOutTimer = 2000;
            Blink = 0xA;
        }

        /* Process Modbus */
        MBS_ProcessModbus();

        /* Guard the Watchdog */
        WDTCONbits.WDTCLRKEY = 0x5743;
    }

    return (EXIT_FAILURE);
}
