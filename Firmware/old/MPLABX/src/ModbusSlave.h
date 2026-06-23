/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _EXAMPLE_FILE_NAME_H    /* Guard against multiple inclusion */
#define _EXAMPLE_FILE_NAME_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
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

#ifndef _MODBUS_SLAVE_H    /* Guard against multiple inclusion */
#define _MODBUS_SLAVE_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <stdint.h>
#include <stdbool.h>


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif


    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */

    
    /* ************************************************************************** */
    /** Modbus RTU Slave Output Register Number
     */
#define MBS_NUMBER_OF_OUTPUT_REGISTERS      100

    /* ************************************************************************** */
    /** Modbus RTU Slave Addresses
     */
#define MBS_BROADCAST_ADDRESS               0
#define MBS_DEFAULT_ADDRESS                 1    
    
    /* ************************************************************************** */
    /** Buffer sizes for Modbus RTU Slave
     */
#define MBS_RECEIVE_BUFFER_SIZE             (MBS_NUMBER_OF_OUTPUT_REGISTERS*2 + 5) 
#define MBS_TRANSMIT_BUFFER_SIZE            MBS_RECEIVE_BUFFER_SIZE
#define MBS_RXTX_BUFFER_SIZE                MBS_TRANSMIT_BUFFER_SIZE

    
    /* ************************************************************************** */
    /** Timeout Constant for Petit Modbus RTU Slave [millisecond]
     */
#define MBS_TIMEOUT                         200


    /* ************************************************************************** */
    /** Modbus Functions
     */
//#define MBS_READ_COILS                      1
//#define MBS_READ_DISCRETE_INPUTS            2
#define MBS_READ_HOLDING_REGISTERS          3
//#define MBS_READ_INPUT_REGISTERS            4
//#define MBS_WRITE_SINGLE_COIL               5
#define MBS_WRITE_SINGLE_REGISTER           6
//#define MBS_WRITE_MULTIPLE_COILS            15
#define MBS_WRITE_MULTIPLE_REGISTERS        16
  

    /* ************************************************************************** */
    /** Modbus Status Flags and Error Codes Const.
     */
#define MBS_FALSE_FUNCTION                  0
#define MBS_FALSE_SLAVE_ADDRESS             1
#define MBS_DATA_NOT_READY                  2
#define MBS_DATA_READY                      3

#define MBS_ERROR_CODE_01                   0x01                            // Function code is not supported
#define MBS_ERROR_CODE_02                   0x02                            // Register address is not allowed or write-protected


    /* ************************************************************************** */
    /** Modbus Index Registers Description 
     */
#define MBS_OWN_ID_SW                       1                               // 1    OwnID_SW	UINT16	
#define MBS_SL_MODEL                        2                               // 2	Searchlight Model   UINT16	
#define MBS_HD_ID                           3                               // 3	HW_ID	UINT16	
#define MBS_SW_ID                           4                               // 4	SW_ID	UINT16	
#define MBS_SN                              5                               // 5	SerialNo	UINT16	
#define MBS_SET_HORIZ_POS_RAW               7                               // 7	Set_Horizontal_pos_raw	INT16	
#define MBS_SET_VERT_POS_RAW                8                               // 8	Set_Vertical_pos_raw	INT16	
#define MBS_PKT_CNT_IN                      10                              // 10	PacketCounter	UINT16	
#define MBS_HORIZ_INPUT                     11                              // 11	horizInput	INT16	
#define MBS_VERT_INPUT                      12                              // 12	vertInput	INT16	
#define MBS_FOCUS_INPUT                     13                              // 13	focusInput	INT16	
#define MBS_COMMAND                         14                              // 14	Command	UINT16	
#define MBS_HORIZ_POS_RAW                   20                              // 20	Horizontal_pos_raw	INT16	
#define MBS_VERT_POS_RAW                    21                              // 21	Vertical_pos_raw	INT16	
#define MBS_SL_STATUS                       22                              // 22	SLStatus	UINT16	
#define MBS_IGNITION_TIME                   24                              // 24	IgnitionTime	UINT16	
#define MBS_MD_FOCUS_FB                     25                              // 25	MDFocusFB	UINT16	    uint16_t MD_FOCUS_FB_AN4
#define MBS_MD_VERT_FB                      26                              // 26	MDVertFB	UINT16	    uint16_t MD_VERT_FB_AN5
#define MBS_MD_HORIZ_FB                     27                              // 27	MDHorFB	UINT16	    uint16_t MD_HORIZ_FB_AN6
#define MBS_POWER_IN_V                      28                              // 28	PowerInV	UINT16	    uint16_t POWER_IN_V_AN7
#define MBS_FOCUS_POS_RAW                   29                              // 29	focusPos	UINT16	    uint16_t FOCUS_POS_AN18
#define MBS_PKT_CNT_RESPONS                 30                              // 30	PacketCounterRespons	UINT16	
#define MBS_PORTA                           31                              // 31	portA	UINT16	    uint16_t PortA;       
#define MBS_PORTB                           32                              // 32	portB	UINT16	    uint16_t PortB;       
#define MBS_PORTC                           33                              // 33	portC	UINT16	    uint16_t PortC;       
#define MBS_PORTD                           34                              // 34	portD	UINT16	    uint8_t PortD;       
#define MBS_HORIZ_R5_SI                     35                              // 35   HorizR5SpeedIndex UINT16
#define MBS_VERT_R5_SI                      36                              // 36   VertR5SpeedIndex UINT16
#define MBS_HORIZ_PWR_LIMIT                 40                              // 40	HorizPowerLimit	UINT16	
#define MBS_VERT_PWR_LIMIT                  41                              // 41	VertPowerLimit	UINT16	
#define MBS_PWR_DROP_LIMIT                  42                              // 42	PowerDropLimit	UINT16	
//#define MBS_HORIZ_SLOW_STOP_FACTOR          43                              // 43	SlowStopHorz	UINT16	
#define MBS_FOCUS_PWR_LIMIT                 43                              // 41	FocusPowerLimit	UINT16	
//#define MBS_VERT_SLOW_STOP_FACTOR           44                              // 44	SlowStopVert	UINT16	
#define MBS_MAX_PORT_HEADING                45                              // 45	MaxPortHeading	INT16	
#define MBS_MAX_STBD_HEADING                46                              // 46	MaxStbdHeading	INT16	
#define MBS_MAX_VERT_POS                    47                              // 47	MaxVertPos	INT16	
#define MBS_MIN_VERT_POS                    48                              // 48	MinVertPos	INT16	

#define MBS_JOYSTICK_SETUP                  50                              // 50 - Joystick setting

/* ================= TLV493D ? Modbus Holding Registers =================
 * Base register = 51
 * Alle påfølgende verdier er sekvensielle
 */

#define MBS_TLV493D_X                       51u
#define MBS_TLV493D_Y                       (MBS_TLV493D_X + 1u)
#define MBS_TLV493D_Z                       (MBS_TLV493D_X + 2u)
#define MBS_TLV493D_TEMP                    (MBS_TLV493D_X + 3u)
#define MBS_TLV493D_FRAME                   (MBS_TLV493D_X + 4u)
#define MBS_TLV493D_CH                      (MBS_TLV493D_X + 5u)
#define MBS_TLV493D_PWRDOWN                 (MBS_TLV493D_X + 6u)
#define MBS_TLV493D_VALID                   (MBS_TLV493D_X + 7u)
#define MBS_TLV493D_AGE                     (MBS_TLV493D_X + 8u)
#define MBS_TLV493D_HEADING                 (MBS_TLV493D_X + 9u)            // int16: [-180..180]
#define MBS_TLV493D_TEMP_C                  (MBS_TLV493D_X + 10u)           // int16: whole °C    

//           556677889900
#define MBS_FW_VER_DATE_TAG                 75                              // __DATE__ "Jan 24 2011"
                                                                            //                1122
#define MBS_FW_VER_REV_TAG                  81                              // RevBuild "Rev.: 002"

    
    /* ************************************************************************** */
    /** MBS_COMMAND bit mask Description 
     */
#define MBS_COMMAND_LAMP_ON_OFF             0x0001    
#define MBS_COMMAND_CHANGE_LAMP_ON_OFF      0x0002    
#define MBS_COMMAND_FOCUS_LIMP_MODE         0x0040
#define MBS_COMMAND_R60                     0x0080    
#define MBS_COMMAND_STOP_HORIZ              0x0100
#define MBS_COMMAND_STOP_VERT               0x0200    
#define MBS_COMMAND_STOP_FOCUS              0x0400    
#define MBS_COMMAND_BLOCK_LAMP              0x0800    
#define MBS_COMMAND_GOTO_HORIZ_POS          0x1000    
#define MBS_COMMAND_GOTO_VERT_POS           0x2000    
#define MBS_COMMAND_SET_HORIZ_POS           0x4000    
#define MBS_COMMAND_SET_VERT_POS            0x8000    
    
        
    /* ************************************************************************** */
    /** MBS_SL_STATUS bit mask Description 
     */
#define MBS_SL_STATUS_LAMP_ON               0x0001    
#define MBS_SL_STATUS_END_STOP_HORIZ_CW     0x0002    
#define MBS_SL_STATUS_END_STOP_HORIZ_CCW    0x0004    
#define MBS_SL_STATUS_END_STOP_VERT_B       0x0008    
#define MBS_SL_STATUS_END_STOP_VERT_Y       0x0010    
#define MBS_SL_STATUS_END_STOP_FOCUS_B      0x0020    
#define MBS_SL_STATUS_END_STOP_FOCUS_Y      0x0040    
#define MBS_SL_STATUS_ALARM                 0x0080    
#define MBS_SL_STATUS_GOTO_HORIZ_ALARM      0x0100    
#define MBS_SL_STATUS_GOTO_VERT_ALARM       0x0200    
#define MBS_SL_STATUS_GOTO_HORIZ_POS        0x1000    
#define MBS_SL_STATUS_GOTO_VERT_POS         0x2000    
#define MBS_SL_STATUS_VERT_SENSOR_FAULT     0x4000    
#define MBS_SL_STATUS_FOCUS_SENSOR_FAULT    0x8000    

    
    /* ************************************************************************** */
    /** MBS_JOYSTICK_SETUP bit mask Description (0x0000 - Default R5 functionallity)
     */
#define MBS_JS_SLAVE_ENABLED                0x0001    
    
    
    // *****************************************************************************
    // *****************************************************************************
    // Section: Data Types
    // *****************************************************************************
    // *****************************************************************************

 
    // *****************************************************************************
    /** Modbus Slave State
     */
typedef enum
{
    MBS_RXTX_IDLE,
    MBS_RXTX_START,
    MBS_RXTX_DATABUF,
    MBS_RXTX_WAIT_ANSWER,
    MBS_RXTX_TIMEOUT
}MBS_RXTX_STATE;

    // *****************************************************************************
    /** Modbus Data Frame Structure Type
     */
typedef struct
{
  unsigned char     Address;
  unsigned char     Function;
  unsigned char     DataBuf[MBS_RXTX_BUFFER_SIZE];
  unsigned short    DataLen;
} __attribute__ ((packed)) stMBS_RxTxData_t;



    // *****************************************************************************
    // *****************************************************************************
    // Section: Interface Functions
    // *****************************************************************************


    extern uint8_t MBS_SlaveAddress;
    extern volatile uint16_t MBS_HoldRegisters[MBS_NUMBER_OF_OUTPUT_REGISTERS];
    extern volatile uint32_t MBS_TimerValue;
    extern uint32_t mySystemTimeOutTimer;
    
    void MBS_InitModbus(uint8_t ModbusSlaveAddress);
    void MBS_ProcessModbus(void);
    void MBS_ReciveData(uint8_t Data);
    void MBS_UART_Putch(uint8_t ch);

/* ************************************************************************** */
/** Helper functions for 16-bit register bit manipulation
 */
static inline void MBS_RegSetBits(volatile uint16_t *reg, uint16_t mask)
{
    *reg |= mask;
}

static inline void MBS_RegClearBits(volatile uint16_t *reg, uint16_t mask)
{
    *reg &= (uint16_t)(~mask);
}

static inline bool MBS_RegIsBitsSet(uint16_t reg, uint16_t mask)
{
    return ((reg & mask) == mask);
}

    
    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _MODBUS_SLAVE_H */

/* *****************************************************************************
 End of File
 */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif


    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */

    /*  A brief description of a section can be given directly below the section
        banner.
     */


    /* ************************************************************************** */
    /** Descriptive Constant Name

      @Summary
        Brief one-line summary of the constant.
    
      @Description
        Full description, explaining the purpose and usage of the constant.
        <p>
        Additional description in consecutive paragraphs separated by HTML 
        paragraph breaks, as necessary.
        <p>
        Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
      @Remarks
        Any additional remarks
     */
#define EXAMPLE_CONSTANT 0


    // *****************************************************************************
    // *****************************************************************************
    // Section: Data Types
    // *****************************************************************************
    // *****************************************************************************

    /*  A brief description of a section can be given directly below the section
        banner.
     */


    // *****************************************************************************

    /** Descriptive Data Type Name

      @Summary
        Brief one-line summary of the data type.
    
      @Description
        Full description, explaining the purpose and usage of the data type.
        <p>
        Additional description in consecutive paragraphs separated by HTML 
        paragraph breaks, as necessary.
        <p>
        Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.

      @Remarks
        Any additional remarks
        <p>
        Describe enumeration elements and structure and union members above each 
        element or member.
     */
    typedef struct _example_struct_t {
        /* Describe structure member. */
        int some_number;

        /* Describe structure member. */
        bool some_flag;

    } example_struct_t;


    // *****************************************************************************
    // *****************************************************************************
    // Section: Interface Functions
    // *****************************************************************************
    // *****************************************************************************

    /*  A brief description of a section can be given directly below the section
        banner.
     */

    // *****************************************************************************
    /**
      @Function
        int ExampleFunctionName ( int param1, int param2 ) 

      @Summary
        Brief one-line description of the function.

      @Description
        Full description, explaining the purpose and usage of the function.
        <p>
        Additional description in consecutive paragraphs separated by HTML 
        paragraph breaks, as necessary.
        <p>
        Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.

      @Precondition
        List and describe any required preconditions. If there are no preconditions,
        enter "None."

      @Parameters
        @param param1 Describe the first parameter to the function.
    
        @param param2 Describe the second parameter to the function.

      @Returns
        List (if feasible) and describe the return values of the function.
        <ul>
          <li>1   Indicates an error occurred
          <li>0   Indicates an error did not occur
        </ul>

      @Remarks
        Describe any special behavior not described above.
        <p>
        Any additional remarks.

      @Example
        @code
        if(ExampleFunctionName(1, 2) == 0)
        {
            return 3;
        }
     */
    int ExampleFunction(int param1, int param2);


    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
