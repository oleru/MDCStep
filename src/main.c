/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdint.h>
#include "definitions.h"                // SYS function prototypes

#include "ModbusSlave.h"
#include "tlv493d.h"


#define TLV_ADDR   0x1F

typedef enum {
    ST_RESET = 0,
    ST_WAIT_AFTER_RESET,
    ST_READ_FACTORY,
    ST_WAIT_AFTER_FACTORY,
    ST_WRITE_CONFIG,
    ST_WAIT_AFTER_CONFIG,
    ST_READ_DATA
} tlv_state_t;

static tlv_state_t state = ST_RESET;
static uint32_t t0;

static uint8_t factory[10];
static uint8_t dataBuf[7];


static uint8_t resetBuf[1] = {0x00};

static uint8_t configBuf[4] = {
    0x00, // register pointer (ALLTID 0)
    0x03, // MOD1: FAST=1, LOW=1, INT=0 (eksempel)
    0x00, // reserved (overskrives etter factory-read)
    0x40 // MOD2: LP=1, T=0
};




// Version Tags
char VersionTag1[] = __DATE__; // "Jan 24 2011"
char VersionTag2[] = __TIME__; // "14:45:20"
char RevBuild[] = "Rev.: 001";

volatile bool TimerEvent1ms;
volatile uint32_t myTime = 0;
bool TimerEvent10ms = false, TimerEvent50ms = false, TimerEvent250ms = false, TimerEvent1s = false, TimerEvent15s = false;
uint32_t myLastTime = 0;
uint32_t mySystemTimeOutTimer;

uint8_t myData[25];

TLV493D_Data_t mag;



/**
    Prototypes
 */
void MBS_UART_Putch(uint8_t ch);
void UpdateTimers(void);

void myCORETIMER(uint32_t status, uintptr_t context) {
    TimerEvent1ms = true;
    myTime++;
}

//void MyI2CCallback(uintptr_t context) {
//    //This function will be called when the transfer completes. Note
//    //that this functioin executes in the context of the I2C interrupt.
//    MBS_HoldRegisters[10] = myData[0];
//    MBS_HoldRegisters[11] = myData[1];
//    MBS_HoldRegisters[12] = myData[2];
//    MBS_HoldRegisters[13] = myData[3];
//    MBS_HoldRegisters[14] = myData[4];
//    MBS_HoldRegisters[15] = myData[5];
//    MBS_HoldRegisters[16] = myData[6];
//}


// Callback function for the ModBus Slave driver

void MBS_UART_Putch(uint8_t ch) {
    UART1_Write(&ch, 1);
}

void UpdateTimers(void) {
    static int32_t my10msCnt, my50msCnt, my250msCnt, my1000msCnt, my15sCnt;
    int32_t delta;

    delta = myTime - myLastTime;
    myLastTime = myTime;
    my10msCnt += delta;
    MBS_TimerValue += delta; // Modbus 1ms Time Keeper
    //lastHorizInt += delta;

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
                } //..while(my15sCnt>=15)
            } //..while(my1000msCnt>=1000)
        } //..while(my50msCnt>=50)
    } //..while(my10msCnt>=10)

}

bool TLV493D_I2C_Write(uint8_t addr, uint8_t *data, uint8_t len) {
    return I2C1_Write(addr, data, len);
}

bool TLV493D_I2C_Read(uint8_t addr, uint8_t *data, uint8_t len) {
    //    uint8_t reg = 0x00;

    // 1. Dummy WRITE (setter read pointer)
    //    if (!I2C1_Write(addr, &reg, 1))
    //        return false;

    //    while (I2C1_IsBusy());

    // 2. READ (ny START)
    if (!I2C1_Read(addr, data, len))
        return false;

    return true;
}

void TLV_Task(void) {
    switch (state) {
        case ST_RESET:
            if (!I2C1_IsBusy()) {
                I2C1_Write(0x00, resetBuf, 1); // general reset
                t0 = myTime;
                state = ST_WAIT_AFTER_RESET;
            }
            break;

        case ST_WAIT_AFTER_RESET:
            if (myTime - t0 >= 1000) {
                state = ST_READ_FACTORY;
            }
            break;

        case ST_READ_FACTORY:
            if (!I2C1_IsBusy()) {
                I2C1_Read(TLV_ADDR, factory, 10);
                t0 = myTime;
                state = ST_WAIT_AFTER_FACTORY;
            }
            break;

        case ST_WAIT_AFTER_FACTORY:
            if (myTime - t0 >= 1000) {
                /* Bevar factory-bits */
                configBuf[2] = factory[8]; // reserved
                state = ST_WRITE_CONFIG;
            }
            break;

        case ST_WRITE_CONFIG:
            if (!I2C1_IsBusy()) {
                I2C1_Write(TLV_ADDR, configBuf, 4);
                t0 = myTime;
                state = ST_WAIT_AFTER_CONFIG;
            }
            break;

        case ST_WAIT_AFTER_CONFIG:
            if (myTime - t0 >= 1000) {
                state = ST_READ_DATA;
            }
            break;

        case ST_READ_DATA:
            if (!I2C1_IsBusy()) {
                I2C1_Read(TLV_ADDR, dataBuf, 7);

                /* ? HER har du rådata i dataBuf[] */

                t0 = myTime; // hvis du vil lese 1 Hz
            }
            break;

        default:
            state = ST_RESET;
            break;
    }
}



// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main(void) {
    uint8_t myModBusAddr;
    uint8_t RdBuffer[10];
    uint8_t init_mode = 0;
    //bool ret=true;

    /* Initialize all modules */
    SYS_Initialize(NULL);

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

    //I2C1_CallbackRegister(MyI2CCallback, (uintptr_t)NULL);

    //    ret = TLV493D_Init();
    //    if(ret==true) {
    //        MBS_HoldRegisters[18] = 321;
    //    } else {
    //        MBS_HoldRegisters[18] = 123;        
    //    }




    while (true) {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks();


        // 1ms Timer Event
        if (TimerEvent1ms) {
            TimerEvent1ms = false;
            UpdateTimers();


        } // ..if(TimerEvent1ms)

        // 50ms Timer Event
        if (TimerEvent50ms) {
            TimerEvent50ms = false;





            // Init gyro
            if (init_mode == 0) {
                init_mode++;
                //            TLV493D_Init();
            }

            if (VERT_B_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_VERT_CW;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_VERT_CW;
            }
            if (VERT_G_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_VERT_CCW;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_VERT_CCW;
            }
            if ((MBS_HoldRegisters[MBS_SL_STATUS] & (MBS_SL_STATUS_END_STOP_VERT_CW | MBS_SL_STATUS_END_STOP_VERT_CCW)) == 0) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_VERT_SENSOR_FAULT;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_VERT_SENSOR_FAULT;
            }

            if (FOCUS_B_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_FOCUS_OUT;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_FOCUS_OUT;
            }
            if (FOCUS_G_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_FOCUS_IN;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_FOCUS_IN;
            }
            if ((MBS_HoldRegisters[MBS_SL_STATUS] & (MBS_SL_STATUS_END_STOP_FOCUS_IN | MBS_SL_STATUS_END_STOP_FOCUS_OUT)) == 0) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_FOCUS_SENSOR_FAULT;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_FOCUS_SENSOR_FAULT;
            }

        } // ..if(TimerEvent1ms)


        // 1s Timer Event
        if (TimerEvent1s) {
            TimerEvent1s = false;
            MBS_HoldRegisters[MBS_OWN_ID_SW] =
                    10 + ((SW1_8_Get() << 3)
                    | (SW1_4_Get() << 2)
                    | (SW1_2_Get() << 1)
                    | SW1_1_Get());

            TLV_Task();

            //            if(!I2C1_Read( TLV493D_ADDR, &myData[0], NUM_BYTES ))
            //            {
            //                //error handling
            //                    MBS_HoldRegisters[19] = 321;
            //            } else {
            //                MBS_HoldRegisters[19] = 123;        
            //            }

            //            if (TLV493D_Read(&mag)) {
            // Gyldige data
            // mag.bx, mag.by, mag.bz
            // mag.heading_deg
            /*
                int16_t x;
                int16_t y;
                int16_t z;
                int16_t temperature;

                uint8_t frame;
                uint8_t channel;
                uint8_t powerDown;
             */
            MBS_HoldRegisters[10] = mag.x;
            MBS_HoldRegisters[11] = mag.y;
            MBS_HoldRegisters[12] = mag.z;
            MBS_HoldRegisters[13] = mag.temperature;
            MBS_HoldRegisters[14] = mag.frame;
            MBS_HoldRegisters[15] = mag.channel;
            MBS_HoldRegisters[16] = mag.powerDown;

            //          } else {
            // Ingen nye data (eller I2C-feil)
            //          }

        }

        // Check for incoming data
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

