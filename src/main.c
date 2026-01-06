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

// Version Tags
char VersionTag1[] = __DATE__;     // "Jan 24 2011"
char VersionTag2[] = __TIME__;     // "14:45:20"
char RevBuild[] = "Rev.: 001";

volatile bool TimerEvent1ms;
volatile uint32_t myTime=0;
bool TimerEvent10ms=false, TimerEvent50ms=false, TimerEvent250ms=false, TimerEvent1s=false, TimerEvent15s=false;
uint32_t myLastTime=0;
uint32_t mySystemTimeOutTimer;

/**
    Prototypes
 */
void MBS_UART_Putch(uint8_t ch);
void UpdateTimers(void);

void myCORETIMER(uint32_t status, uintptr_t context)
{
    TimerEvent1ms=true;
    myTime++;
}

// Callback function for the ModBus Slave driver
void MBS_UART_Putch(uint8_t ch)
{
    UART1_Write(&ch,1);
}           

void UpdateTimers(void)
{
    static int32_t my10msCnt, my50msCnt, my250msCnt, my1000msCnt, my15sCnt;
    int32_t delta;
    
    delta = myTime-myLastTime;
    myLastTime = myTime;
    my10msCnt += delta;
    MBS_TimerValue += delta;  // Modbus 1ms Time Keeper
    //lastHorizInt += delta;
    
    // Update Time Keepers
    while(my10msCnt>=10) {
        TimerEvent10ms = true;
        my10msCnt -= 10;
        my50msCnt += 10;
        while(my50msCnt>=50) {
            TimerEvent50ms = true;
            my50msCnt -= 50;
            
            my250msCnt += 50;
            while(my250msCnt>=250) {
                TimerEvent250ms=true;
                my250msCnt -= 250;
            }
            
            my1000msCnt += 50;
            while(my1000msCnt>=1000) {
                TimerEvent1s=true;
                my1000msCnt -= 1000;
                mySystemTimeOutTimer++;
                my15sCnt += 1;
                while(my15sCnt>=15) {
                    TimerEvent15s=true;
                    my15sCnt -= 15;
                }  //..while(my15sCnt>=15)
            }  //..while(my1000msCnt>=1000)
        }  //..while(my50msCnt>=50)
    }  //..while(my10msCnt>=10)
    
}



// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    uint8_t myModBusAddr;
    uint8_t RdBuffer[10];
    
    
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    

    // Set Modbus Slave Address
    myModBusAddr = 10;
    MBS_InitModbus(myModBusAddr);
    MBS_HoldRegisters[MBS_OWN_ID_SW] =
        10 + ((SW1_8_Get() << 3)
            | (SW1_4_Get() << 2)
            | (SW1_2_Get() << 1)
            |  SW1_1_Get());
    MBS_HoldRegisters[MBS_SL_MODEL] = 5;       // Searchlight Model
    MBS_HoldRegisters[MBS_HD_ID] = (uint16_t)'B';     // HW_ID 
    MBS_HoldRegisters[MBS_SW_ID] = 4;  // SW_ID
 
    CORETIMER_CallbackSet(myCORETIMER, (uintptr_t)NULL);
    CORETIMER_Start();
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
        
        // 1ms Timer Event
        if(TimerEvent1ms) {
            TimerEvent1ms = false;
            UpdateTimers();
                        
        }  // ..if(TimerEvent1ms)

       // 50ms Timer Event
        if(TimerEvent50ms) {
            TimerEvent50ms = false;
          
            if(VERT_B_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_VERT_CW;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_VERT_CW;
            }
            if(VERT_G_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_VERT_CCW;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_VERT_CCW;
            }
            if((MBS_HoldRegisters[MBS_SL_STATUS] & (MBS_SL_STATUS_END_STOP_VERT_CW | MBS_SL_STATUS_END_STOP_VERT_CCW))==0) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_VERT_SENSOR_FAULT;  
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_VERT_SENSOR_FAULT;                  
            }
                       
            if(FOCUS_B_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_FOCUS_OUT;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_FOCUS_OUT;
            }
            if(FOCUS_G_Get()) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_END_STOP_FOCUS_IN;
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_END_STOP_FOCUS_IN;
            }
            if((MBS_HoldRegisters[MBS_SL_STATUS] & (MBS_SL_STATUS_END_STOP_FOCUS_IN | MBS_SL_STATUS_END_STOP_FOCUS_OUT))==0) {
                MBS_HoldRegisters[MBS_SL_STATUS] |= MBS_SL_STATUS_FOCUS_SENSOR_FAULT;  
            } else {
                MBS_HoldRegisters[MBS_SL_STATUS] &= ~MBS_SL_STATUS_FOCUS_SENSOR_FAULT;                  
            }
                
        }  // ..if(TimerEvent1ms)

        
        // 1s Timer Event
        if(TimerEvent1s) {
            TimerEvent1s=false;
            MBS_HoldRegisters[MBS_OWN_ID_SW] =
            10 + ((SW1_8_Get() << 3)
                | (SW1_4_Get() << 2)
                | (SW1_2_Get() << 1)
                |  SW1_1_Get());
        }
        
       // Check for incoming data
        if(UART1_ReadCountGet()>0) {
            UART1_Read(RdBuffer,1);
            MBS_ReciveData(RdBuffer[0]);
        }
        
        // Process Modbus
        MBS_ProcessModbus();
        
        
        // Guard the Watchdog
        WDTCONbits.WDTCLRKEY=0x5743;  // Magic sequence to reset WDT
                

        
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

