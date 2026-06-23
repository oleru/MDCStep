/* ************************************************************************** */
/** Modbus Slave Library

  @Company
    Torka AS

  @File Name
    ModbusSlave.h

  @Summary
    Modbus RTU Slave library.

  @Description
    Small - cut to the bone - Modbus RTU Slave library. 
    Support Modbus Functions 3, 6 and 16.

 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "ModbusSlave.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

// *****************************************************************************
/** Variable for Slave Address

  @Description
    Slave individual Address - valid range 1-247, default 1. 
    0 - Broadcast, 248-255 - Reserved.

  @Remarks
    NA
 */
uint8_t MBS_SlaveAddress = MBS_DEFAULT_ADDRESS;


// *****************************************************************************
/** Buffer for Hold Registers

  @Description
    Buffer for Hold Registers, note Hold register data is stored as uint16_t 
    'big-endian' encoded.

    So for example:

        Register size   value
        16 - bits       0x1234      the first byte sent is 0x12 then 0x34

  @Remarks
    NA
 */
volatile uint16_t MBS_HoldRegisters[MBS_NUMBER_OF_OUTPUT_REGISTERS];


// *****************************************************************************
/** Slave Transmit and Receive Variables
*/
stMBS_RxTxData_t MBS_Tx_Data;
uint32_t MBS_Tx_Current = 0;
uint32_t MBS_Tx_CRC16 = 0xFFFF;
MBS_RXTX_STATE MBS_Tx_State = MBS_RXTX_IDLE;
uint8_t MBS_Tx_Buf[MBS_TRANSMIT_BUFFER_SIZE];
uint32_t MBS_Tx_Buf_Size = 0;

stMBS_RxTxData_t MBS_Rx_Data;
uint32_t MBS_Rx_CRC16 = 0xFFFF;
MBS_RXTX_STATE MBS_Rx_State = MBS_RXTX_IDLE;
uint8_t MBS_Rx_Data_Available = false;

// Modbus RTU Variables
volatile uint8_t MBS_ReceiveBuffer[MBS_RECEIVE_BUFFER_SIZE];   // Buffer to collect data from hardware
volatile uint8_t MBS_ReceiveCounter=0;                                 // Collected data number


// *****************************************************************************
/** Modbus Timer Value [ms] 

  @Description
    Modbus Timer Value in [ms]. Used for check if Message received is completed
    or a timeout event has occurred.

  @Remarks
    NA
*/
volatile uint32_t MBS_TimerValue;



/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/** 
  @Function
    uint8_t MBS_UART_String(uint8_t *s, uint32_t Length) 

  @Summary
    This is used for send string, better to use DMA for it ;)

  @Description
    TBC

  @Precondition
    MBS_UART_Putch "CallBack function" must be defined in main...

  @Parameters
    @param s Data to send.
    
    @param Length Length of s data.

  @Returns
    Always true

  @Remarks
    NA

  @Example
    @code

    MBS_UART_String(MBS_Tx_Buf,MBS_Tx_Buf_Size);
    MBS_Tx_Buf_Size=0;
  
 */
// This is used for send string, better to use DMA for it ;)
uint8_t MBS_UART_String(uint8_t *s, uint32_t Length)
{
    uint32_t i;
    
    for(i=0;i<Length;i++)
        MBS_UART_Putch(s[i]);
    
    return true;
}


/*
 * Function Name        : CRC16
 * @param[in]           : Data  - Data to Calculate CRC
 * @param[in/out]       : CRC   - Pointer to CRC value
 * @How to use          : First initial data has to be 0xFFFF.
 */
void MBS_CRC16(const uint8_t Data, uint32_t* CRC)
{
    uint32_t i;

    *CRC = *CRC ^(uint32_t) Data;
    for (i = 8; i > 0; i--)
    {
        if (*CRC & 0x0001)
            *CRC = (*CRC >> 1) ^ 0xA001;
        else
            *CRC >>= 1;
    }
}


/******************************************************************************/
/*
 * Function Name        : DoTx
 * @param[out]          : TRUE
 * @How to use          : It is used for send data package over physical layer
 */
uint8_t MBS_DoSlaveTX(void)
{  
    MBS_UART_String(MBS_Tx_Buf,MBS_Tx_Buf_Size);

    MBS_Tx_Buf_Size = 0;
    return true;
}


/******************************************************************************/
/*
 * Function Name        : MBS_SendMessage
 * @param[out]          : TRUE/FALSE
 * @How to use          : This function start to sending messages
 */
uint8_t MBS_SendMessage(void)
{
    if (MBS_Tx_State != MBS_RXTX_IDLE)
        return false;

    MBS_Tx_Current  =0;
    MBS_Tx_State    =MBS_RXTX_START;

    return true;
}


/******************************************************************************/
/*
 * Function Name        : MBS_HandleError
 * @How to use          : This function generated errors to Modbus Master
 */
void MBS_HandleError(char ErrorCode)
{
    // Initialize the output buffer. The first byte in the buffer says how many registers we have read
    MBS_Tx_Data.Function    = MBS_Rx_Data.Function | 0x80;
    MBS_Tx_Data.Address     = MBS_SlaveAddress;
    MBS_Tx_Data.DataLen     = 1;
    MBS_Tx_Data.DataBuf[0]  = ErrorCode;
    MBS_SendMessage();
}


/******************************************************************************/
/*
 * Function Name        : MBS_Handle03ReadHoldingRegisters
 * @How to use          : Modbus function 03 - Read holding registers
 */
void MBS_Handle03ReadHoldingRegisters(void)
{
    // Holding registers are effectively numerical outputs that can be written to by the host.
    uint32_t MBS_StartAddress = 0;
    uint32_t MBS_NumberOfRegisters = 0;
    uint32_t MBS_i = 0;

    // The message contains the requested start address and number of registers
    MBS_StartAddress = ((uint32_t) (MBS_Rx_Data.DataBuf[0]) << 8) + (uint32_t) (MBS_Rx_Data.DataBuf[1]);
    MBS_NumberOfRegisters = ((uint32_t) (MBS_Rx_Data.DataBuf[2]) << 8) + (uint32_t) (MBS_Rx_Data.DataBuf[3]);

    // If it is bigger than RegisterNumber return error to Modbus Master
    if((MBS_StartAddress+MBS_NumberOfRegisters)>MBS_NUMBER_OF_OUTPUT_REGISTERS)
        MBS_HandleError(MBS_ERROR_CODE_02);
    else
    {
        // Initialize the output buffer. The first byte in the buffer says how many registers we have read
        MBS_Tx_Data.Function = MBS_READ_HOLDING_REGISTERS;
        MBS_Tx_Data.Address = MBS_SlaveAddress;
        MBS_Tx_Data.DataLen = 1;
        MBS_Tx_Data.DataBuf[0] = 0;

        for (MBS_i = 0; MBS_i < MBS_NumberOfRegisters; MBS_i++)
        {
            unsigned short MBS_CurrentData = MBS_HoldRegisters[MBS_StartAddress+MBS_i];

            MBS_Tx_Data.DataBuf[MBS_Tx_Data.DataLen] = (uint8_t) ((MBS_CurrentData & 0xFF00) >> 8);
            MBS_Tx_Data.DataBuf[MBS_Tx_Data.DataLen + 1] = (uint8_t) (MBS_CurrentData & 0xFF);
            MBS_Tx_Data.DataLen += 2;
            MBS_Tx_Data.DataBuf[0] = MBS_Tx_Data.DataLen - 1;
        }

        MBS_SendMessage();
    }
}


/******************************************************************************/
/*
 * Function Name        : MBS_Handle06WriteSingleRegister
 * @How to use          : Modbus function 06 - Write single register
 */
void MBS_Handle06WriteSingleRegister(void)
{
    // Write single numerical output
    uint32_t MBS_Address = 0;
    uint32_t MBS_Value = 0;
    uint8_t MBS_i = 0;

    // The message contains the requested start address and number of registers
    MBS_Address = ((uint32_t) (MBS_Rx_Data.DataBuf[0]) << 8) + (uint32_t) (MBS_Rx_Data.DataBuf[1]);
    MBS_Value = ((uint32_t) (MBS_Rx_Data.DataBuf[2]) << 8) + (uint32_t) (MBS_Rx_Data.DataBuf[3]);

    // Initialize the output buffer. The first byte in the buffer says how many registers we have read
    MBS_Tx_Data.Function = MBS_WRITE_SINGLE_REGISTER;
    MBS_Tx_Data.Address = MBS_SlaveAddress;
    MBS_Tx_Data.DataLen = 4;

    if(MBS_Address>=MBS_NUMBER_OF_OUTPUT_REGISTERS) {
        if(MBS_Rx_Data.Address != MBS_BROADCAST_ADDRESS) // No respons on Broadcast messages
            MBS_HandleError(MBS_ERROR_CODE_02);
    } else {
        MBS_HoldRegisters[MBS_Address] = MBS_Value;
        // Output data buffer is exact copy of input buffer
        for (MBS_i = 0; MBS_i < 4; ++MBS_i)
            MBS_Tx_Data.DataBuf[MBS_i] = MBS_Rx_Data.DataBuf[MBS_i];
    }

    if(MBS_Rx_Data.Address != MBS_BROADCAST_ADDRESS) // No respons on Broadcast messages
        MBS_SendMessage();
}


/******************************************************************************/
/*
 * Function Name        : MBS_Handle16WriteMultipleRegisters
 * @How to use          : Modbus function 16 - Write multiple registers
 */
void MBS_Handle16WriteMultipleRegisters(void)
{
    // Write single numerical output
    uint32_t MBS_StartAddress = 0;
    //uint8_t MBS_ByteCount;
    uint32_t MBS_NumberOfRegisters = 0;
    uint8_t MBS_i = 0;
    uint32_t MBS_Value = 0;

    // The message contains the requested start address and number of registers
    MBS_StartAddress = ((uint32_t) (MBS_Rx_Data.DataBuf[0]) << 8) + (uint32_t) (MBS_Rx_Data.DataBuf[1]);
    MBS_NumberOfRegisters = ((uint32_t) (MBS_Rx_Data.DataBuf[2]) << 8) + (uint32_t) (MBS_Rx_Data.DataBuf[3]);
    //MBS_ByteCount = MBS_Rx_Data.DataBuf[4];

    // If it is bigger than RegisterNumber return error to Modbus Master
    if((MBS_StartAddress+MBS_NumberOfRegisters)>MBS_NUMBER_OF_OUTPUT_REGISTERS) {
        if(MBS_Rx_Data.Address != MBS_BROADCAST_ADDRESS) // No respons on Broadcast messages
            MBS_HandleError(MBS_ERROR_CODE_02);
    } else {
        // Initialize the output buffer. The first byte in the buffer says how many outputs we have set
        MBS_Tx_Data.Function    = MBS_WRITE_MULTIPLE_REGISTERS;
        MBS_Tx_Data.Address     = MBS_SlaveAddress;
        MBS_Tx_Data.DataLen     = 4;
        MBS_Tx_Data.DataBuf[0]  = MBS_Rx_Data.DataBuf[0];
        MBS_Tx_Data.DataBuf[1]  = MBS_Rx_Data.DataBuf[1];
        MBS_Tx_Data.DataBuf[2]  = MBS_Rx_Data.DataBuf[2];
        MBS_Tx_Data.DataBuf[3]  = MBS_Rx_Data.DataBuf[3];

        // Output data buffer is exact copy of input buffer
        for (MBS_i = 0; MBS_i <MBS_NumberOfRegisters; MBS_i++)
        {
            MBS_Value=(MBS_Rx_Data.DataBuf[5+2*MBS_i]<<8)+(MBS_Rx_Data.DataBuf[6+2*MBS_i]);
            MBS_HoldRegisters[MBS_StartAddress+MBS_i] = MBS_Value;
        }

        if(MBS_Rx_Data.Address != MBS_BROADCAST_ADDRESS) // No respons on Broadcast messages
            MBS_SendMessage();
    }
}


/******************************************************************************/
/*
 * Function Name        : MBS_RxDataAvailable
 * @return              : If Data is Ready, Return TRUE
 *                        If Data is not Ready, Return FALSE
 */
uint8_t MBS_RxDataAvailable(void)
{
    uint8_t Result = MBS_Rx_Data_Available;
    
    MBS_Rx_Data_Available = false;

    return Result;
}


/******************************************************************************/
/*
 * Function Name        : CheckRxTimeout
 * @return              : If Time is out return TRUE
 *                        If Time is not out return FALSE
 */
uint8_t MBS_CheckRxTimeout(void)
{
    // A return value of true indicates there is a timeout    
    if (MBS_TimerValue>= MBS_TIMEOUT)
    {
        MBS_TimerValue = 0;
        MBS_ReceiveCounter = 0;
        return true;
    }

    return false;
}


/******************************************************************************/
/*
 * Function Name        : MBS_CheckBufferComplete
 * @return              : If data is ready, return              DATA_READY
 *                        If slave address is wrong, return     FALSE_SLAVE_ADDRESS
 *                        If data is not ready, return          DATA_NOT_READY
 *                        If functions is wrong, return         FALSE_FUNCTION
 */
uint8_t MBS_CheckBufferComplete(void)
{
    int32_t MBSExpectedReceiveCount=0;

    if(MBS_ReceiveCounter>4)
    {
        if( (MBS_ReceiveBuffer[0]==MBS_SlaveAddress) || (MBS_ReceiveBuffer[0]==MBS_BROADCAST_ADDRESS))
        {
            if(MBS_ReceiveBuffer[1]==0x01 || MBS_ReceiveBuffer[1]==0x02 || MBS_ReceiveBuffer[1]==0x03 || MBS_ReceiveBuffer[1]==0x04 || MBS_ReceiveBuffer[1]==0x05 || MBS_ReceiveBuffer[1]==0x06)  // RHR
            {
                MBSExpectedReceiveCount    =8;
            }
            else if(MBS_ReceiveBuffer[1]==0x0F || MBS_ReceiveBuffer[1]==0x10)
            {
                MBSExpectedReceiveCount=MBS_ReceiveBuffer[6]+9;
            }
            else
            {
                MBS_ReceiveCounter=0;
                return MBS_FALSE_FUNCTION;
            }
        }
        else
        {
            MBS_ReceiveCounter=0;
            return MBS_FALSE_SLAVE_ADDRESS;
        }
    }
    else
        return MBS_DATA_NOT_READY;

    if(MBS_ReceiveCounter==MBSExpectedReceiveCount)
    {
        return MBS_DATA_READY;
    }

    return MBS_DATA_NOT_READY;
}


/******************************************************************************/
/*
 * Function Name        : MBS_RxRTU
 * @How to use          : Check for data ready, if it is good return answer
 */
void MBS_RxRTU(void)
{
    uint8_t MBS_i;
    uint8_t MBS_ReceiveBufferControl=0;

    MBS_ReceiveBufferControl = MBS_CheckBufferComplete();

    if(MBS_ReceiveBufferControl==MBS_DATA_READY)
    {
        MBS_Rx_Data.Address = MBS_ReceiveBuffer[0];
        MBS_Rx_CRC16 = 0xffff;
        MBS_CRC16(MBS_Rx_Data.Address, &MBS_Rx_CRC16);
        MBS_Rx_Data.Function = MBS_ReceiveBuffer[1];
        MBS_CRC16(MBS_Rx_Data.Function, &MBS_Rx_CRC16);

        MBS_Rx_Data.DataLen=0;

        for(MBS_i=2;MBS_i<MBS_ReceiveCounter;MBS_i++)
            MBS_Rx_Data.DataBuf[MBS_Rx_Data.DataLen++]=MBS_ReceiveBuffer[MBS_i];

        MBS_Rx_State =MBS_RXTX_DATABUF;

        MBS_ReceiveCounter=0;
    }

    MBS_CheckRxTimeout();

    if ((MBS_Rx_State == MBS_RXTX_DATABUF) && (MBS_Rx_Data.DataLen >= 2))
    {
        // Finish off our CRC check
        MBS_Rx_Data.DataLen -= 2;
        for (MBS_i = 0; MBS_i < MBS_Rx_Data.DataLen; ++MBS_i)
        {
            MBS_CRC16(MBS_Rx_Data.DataBuf[MBS_i], &MBS_Rx_CRC16);
        }
        
        if (((uint32_t) MBS_Rx_Data.DataBuf[MBS_Rx_Data.DataLen] + ((uint32_t) MBS_Rx_Data.DataBuf[MBS_Rx_Data.DataLen + 1] << 8)) == MBS_Rx_CRC16)
        {
            // Valid message!
            MBS_Rx_Data_Available = true;
        }

        MBS_Rx_State = MBS_RXTX_IDLE;
    }
}


/******************************************************************************/
/*
 * Function Name        : TxRTU
 * @How to use          : If it is ready send answers!
 */
void MBS_TxRTU(void)
{
    MBS_Tx_CRC16                =0xFFFF;
    MBS_Tx_Buf_Size             =0;
    MBS_Tx_Buf[MBS_Tx_Buf_Size++]   =MBS_Tx_Data.Address;
    MBS_CRC16(MBS_Tx_Data.Address, &MBS_Tx_CRC16);
    MBS_Tx_Buf[MBS_Tx_Buf_Size++]   =MBS_Tx_Data.Function;
    MBS_CRC16(MBS_Tx_Data.Function, &MBS_Tx_CRC16);

    for(MBS_Tx_Current=0; MBS_Tx_Current < MBS_Tx_Data.DataLen; MBS_Tx_Current++)
    {
        MBS_Tx_Buf[MBS_Tx_Buf_Size++]=MBS_Tx_Data.DataBuf[MBS_Tx_Current];
        MBS_CRC16(MBS_Tx_Data.DataBuf[MBS_Tx_Current], &MBS_Tx_CRC16);
    }
    
    MBS_Tx_Buf[MBS_Tx_Buf_Size++] = MBS_Tx_CRC16 & 0x00FF;
    MBS_Tx_Buf[MBS_Tx_Buf_Size++] =(MBS_Tx_CRC16 & 0xFF00) >> 8;

    MBS_DoSlaveTX();

    MBS_Tx_State    =MBS_RXTX_IDLE;
}



/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */


// *****************************************************************************
/** 
  @Function
    MBS_InitModbus(uint8_t ModbusSlaveAddress) 

  @Summary
    Init Modbus Driver - more or less just set Modbus Slave Address for now...

  @Remarks
    NA
 */
void MBS_InitModbus(uint8_t ModbusSlaveAddress)
{
    MBS_SlaveAddress = ModbusSlaveAddress;
}
    

// *****************************************************************************
/** 
  @Function
    MBS_ProcessModbus(void) 

  @Summary
    Modbus Slave Process procedure called in main event.

  @Remarks
    NA
 */
void MBS_ProcessModbus(void)
{
    if (MBS_Tx_State != MBS_RXTX_IDLE)                                      // If answer is ready, send it!
        MBS_TxRTU();

    MBS_RxRTU();                                                              // Call this function every cycle

    if (MBS_RxDataAvailable())                                                // If data is ready enter this!
    {
        if( (MBS_Rx_Data.Address == MBS_SlaveAddress) || (MBS_ReceiveBuffer[0]==MBS_BROADCAST_ADDRESS) ) // Is Data for us?
        {
            
            // We have a Host!
            mySystemTimeOutTimer=0;
            
            switch (MBS_Rx_Data.Function)                                     // Data is for us but which function?
            {
                case MBS_READ_HOLDING_REGISTERS:
                    MBS_Handle03ReadHoldingRegisters();
                    break;
                
                case MBS_WRITE_SINGLE_REGISTER:
                    MBS_Handle06WriteSingleRegister();
                    break;
                
                case MBS_WRITE_MULTIPLE_REGISTERS:
                    MBS_Handle16WriteMultipleRegisters();
                    break;
                
                default:
                    MBS_HandleError(MBS_ERROR_CODE_01);
                    break;
            }
        }
    }
    
}


// *****************************************************************************
/** 
  @Function
    MBS_ReciveData(uint8_t Data) 

  @Summary
    Modbus Slave Receive Data procedure called in main event on UART RX data

  @Remarks
    NA
 */
void MBS_ReciveData(uint8_t Data)
{
    MBS_ReceiveBuffer[MBS_ReceiveCounter] = Data;
    MBS_ReceiveCounter++;

    if(MBS_ReceiveCounter>MBS_RECEIVE_BUFFER_SIZE)  
        MBS_ReceiveCounter=0;

    MBS_TimerValue=0;   
}


void __attribute__ ((weak)) MBS_UART_Putch(uint8_t ch)
{
    
}

/* *****************************************************************************
 End of File
 */
