/*******************************************************************************
  GPIO PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_gpio.h

  Summary:
    GPIO PLIB Header File

  Description:
    This library provides an interface to control and interact with Parallel
    Input/Output controller (GPIO) module.

*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#ifndef PLIB_GPIO_H
#define PLIB_GPIO_H

#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Data types and constants
// *****************************************************************************
// *****************************************************************************


/*** Macros for MISO pin ***/
#define MISO_Get()               ((PORTA >> 7) & 0x1)
#define MISO_GetLatch()          ((LATA >> 7) & 0x1)
#define MISO_PIN                  GPIO_PIN_RA7

/*** Macros for U2_DIR pin ***/
#define U2_DIR_Set()               (LATBSET = (1<<14))
#define U2_DIR_Clear()             (LATBCLR = (1<<14))
#define U2_DIR_Toggle()            (LATBINV= (1<<14))
#define U2_DIR_OutputEnable()      (TRISBCLR = (1<<14))
#define U2_DIR_InputEnable()       (TRISBSET = (1<<14))
#define U2_DIR_Get()               ((PORTB >> 14) & 0x1)
#define U2_DIR_GetLatch()          ((LATB >> 14) & 0x1)
#define U2_DIR_PIN                  GPIO_PIN_RB14

/*** Macros for MB1_PWM pin ***/
#define MB1_PWM_Set()               (LATBSET = (1<<15))
#define MB1_PWM_Clear()             (LATBCLR = (1<<15))
#define MB1_PWM_Toggle()            (LATBINV= (1<<15))
#define MB1_PWM_OutputEnable()      (TRISBCLR = (1<<15))
#define MB1_PWM_InputEnable()       (TRISBSET = (1<<15))
#define MB1_PWM_Get()               ((PORTB >> 15) & 0x1)
#define MB1_PWM_GetLatch()          ((LATB >> 15) & 0x1)
#define MB1_PWM_PIN                  GPIO_PIN_RB15

/*** Macros for DRV8876_FB pin ***/
#define DRV8876_FB_Get()               ((PORTA >> 13) & 0x1)
#define DRV8876_FB_GetLatch()          ((LATA >> 13) & 0x1)
#define DRV8876_FB_PIN                  GPIO_PIN_RA13

/*** Macros for NTC_PROBE pin ***/
#define NTC_PROBE_Get()               ((PORTA >> 12) & 0x1)
#define NTC_PROBE_GetLatch()          ((LATA >> 12) & 0x1)
#define NTC_PROBE_PIN                  GPIO_PIN_RA12

/*** Macros for U2_STEP pin ***/
#define U2_STEP_Set()               (LATASET = (1<<11))
#define U2_STEP_Clear()             (LATACLR = (1<<11))
#define U2_STEP_Toggle()            (LATAINV= (1<<11))
#define U2_STEP_OutputEnable()      (TRISACLR = (1<<11))
#define U2_STEP_InputEnable()       (TRISASET = (1<<11))
#define U2_STEP_Get()               ((PORTA >> 11) & 0x1)
#define U2_STEP_GetLatch()          ((LATA >> 11) & 0x1)
#define U2_STEP_PIN                  GPIO_PIN_RA11

/*** Macros for uC_U1RX pin ***/
#define uC_U1RX_Get()               ((PORTA >> 6) & 0x1)
#define uC_U1RX_GetLatch()          ((LATA >> 6) & 0x1)
#define uC_U1RX_PIN                  GPIO_PIN_RA6

/*** Macros for LD_PWM pin ***/
#define LD_PWM_Get()               ((PORTB >> 0) & 0x1)
#define LD_PWM_GetLatch()          ((LATB >> 0) & 0x1)
#define LD_PWM_PIN                  GPIO_PIN_RB0

/*** Macros for MB1_INT pin ***/
#define MB1_INT_Get()               ((PORTB >> 1) & 0x1)
#define MB1_INT_GetLatch()          ((LATB >> 1) & 0x1)
#define MB1_INT_PIN                  GPIO_PIN_RB1

/*** Macros for MB1_CS pin ***/
#define MB1_CS_Set()               (LATBSET = (1<<2))
#define MB1_CS_Clear()             (LATBCLR = (1<<2))
#define MB1_CS_Toggle()            (LATBINV= (1<<2))
#define MB1_CS_OutputEnable()      (TRISBCLR = (1<<2))
#define MB1_CS_InputEnable()       (TRISBSET = (1<<2))
#define MB1_CS_Get()               ((PORTB >> 2) & 0x1)
#define MB1_CS_GetLatch()          ((LATB >> 2) & 0x1)
#define MB1_CS_PIN                  GPIO_PIN_RB2

/*** Macros for MB1_MOSI pin ***/
#define MB1_MOSI_Get()               ((PORTB >> 3) & 0x1)
#define MB1_MOSI_GetLatch()          ((LATB >> 3) & 0x1)
#define MB1_MOSI_PIN                  GPIO_PIN_RB3

/*** Macros for MB1_AN pin ***/
#define MB1_AN_Get()               ((PORTC >> 0) & 0x1)
#define MB1_AN_GetLatch()          ((LATC >> 0) & 0x1)
#define MB1_AN_PIN                  GPIO_PIN_RC0

/*** Macros for U3_DIR pin ***/
#define U3_DIR_Set()               (LATCSET = (1<<1))
#define U3_DIR_Clear()             (LATCCLR = (1<<1))
#define U3_DIR_Toggle()            (LATCINV= (1<<1))
#define U3_DIR_OutputEnable()      (TRISCCLR = (1<<1))
#define U3_DIR_InputEnable()       (TRISCSET = (1<<1))
#define U3_DIR_Get()               ((PORTC >> 1) & 0x1)
#define U3_DIR_GetLatch()          ((LATC >> 1) & 0x1)
#define U3_DIR_PIN                  GPIO_PIN_RC1

/*** Macros for MB1_RST pin ***/
#define MB1_RST_Set()               (LATCSET = (1<<2))
#define MB1_RST_Clear()             (LATCCLR = (1<<2))
#define MB1_RST_Toggle()            (LATCINV= (1<<2))
#define MB1_RST_OutputEnable()      (TRISCCLR = (1<<2))
#define MB1_RST_InputEnable()       (TRISCSET = (1<<2))
#define MB1_RST_Get()               ((PORTC >> 2) & 0x1)
#define MB1_RST_GetLatch()          ((LATC >> 2) & 0x1)
#define MB1_RST_PIN                  GPIO_PIN_RC2

/*** Macros for LD_TEMP_IND pin ***/
#define LD_TEMP_IND_Set()               (LATCSET = (1<<11))
#define LD_TEMP_IND_Clear()             (LATCCLR = (1<<11))
#define LD_TEMP_IND_Toggle()            (LATCINV= (1<<11))
#define LD_TEMP_IND_OutputEnable()      (TRISCCLR = (1<<11))
#define LD_TEMP_IND_InputEnable()       (TRISCSET = (1<<11))
#define LD_TEMP_IND_Get()               ((PORTC >> 11) & 0x1)
#define LD_TEMP_IND_GetLatch()          ((LATC >> 11) & 0x1)
#define LD_TEMP_IND_PIN                  GPIO_PIN_RC11

/*** Macros for DRV_TEMP_IND pin ***/
#define DRV_TEMP_IND_Set()               (LATASET = (1<<2))
#define DRV_TEMP_IND_Clear()             (LATACLR = (1<<2))
#define DRV_TEMP_IND_Toggle()            (LATAINV= (1<<2))
#define DRV_TEMP_IND_OutputEnable()      (TRISACLR = (1<<2))
#define DRV_TEMP_IND_InputEnable()       (TRISASET = (1<<2))
#define DRV_TEMP_IND_Get()               ((PORTA >> 2) & 0x1)
#define DRV_TEMP_IND_GetLatch()          ((LATA >> 2) & 0x1)
#define DRV_TEMP_IND_PIN                  GPIO_PIN_RA2

/*** Macros for DRV_V_IND pin ***/
#define DRV_V_IND_Set()               (LATASET = (1<<3))
#define DRV_V_IND_Clear()             (LATACLR = (1<<3))
#define DRV_V_IND_Toggle()            (LATAINV= (1<<3))
#define DRV_V_IND_OutputEnable()      (TRISACLR = (1<<3))
#define DRV_V_IND_InputEnable()       (TRISASET = (1<<3))
#define DRV_V_IND_Get()               ((PORTA >> 3) & 0x1)
#define DRV_V_IND_GetLatch()          ((LATA >> 3) & 0x1)
#define DRV_V_IND_PIN                  GPIO_PIN_RA3

/*** Macros for MOSI pin ***/
#define MOSI_Get()               ((PORTA >> 8) & 0x1)
#define MOSI_GetLatch()          ((LATA >> 8) & 0x1)
#define MOSI_PIN                  GPIO_PIN_RA8

/*** Macros for POWER_IN_V_FB pin ***/
#define POWER_IN_V_FB_Get()               ((PORTB >> 4) & 0x1)
#define POWER_IN_V_FB_GetLatch()          ((LATB >> 4) & 0x1)
#define POWER_IN_V_FB_PIN                  GPIO_PIN_RB4

/*** Macros for U3_STEP pin ***/
#define U3_STEP_Set()               (LATASET = (1<<4))
#define U3_STEP_Clear()             (LATACLR = (1<<4))
#define U3_STEP_Toggle()            (LATAINV= (1<<4))
#define U3_STEP_OutputEnable()      (TRISACLR = (1<<4))
#define U3_STEP_InputEnable()       (TRISASET = (1<<4))
#define U3_STEP_Get()               ((PORTA >> 4) & 0x1)
#define U3_STEP_GetLatch()          ((LATA >> 4) & 0x1)
#define U3_STEP_PIN                  GPIO_PIN_RA4

/*** Macros for MB1_MISO pin ***/
#define MB1_MISO_Get()               ((PORTA >> 9) & 0x1)
#define MB1_MISO_GetLatch()          ((LATA >> 9) & 0x1)
#define MB1_MISO_PIN                  GPIO_PIN_RA9

/*** Macros for VERT_G pin ***/
#define VERT_G_Set()               (LATDSET = (1<<4))
#define VERT_G_Clear()             (LATDCLR = (1<<4))
#define VERT_G_Toggle()            (LATDINV= (1<<4))
#define VERT_G_OutputEnable()      (TRISDCLR = (1<<4))
#define VERT_G_InputEnable()       (TRISDSET = (1<<4))
#define VERT_G_Get()               ((PORTD >> 4) & 0x1)
#define VERT_G_GetLatch()          ((LATD >> 4) & 0x1)
#define VERT_G_PIN                  GPIO_PIN_RD4

/*** Macros for VERT_B pin ***/
#define VERT_B_Set()               (LATDSET = (1<<2))
#define VERT_B_Clear()             (LATDCLR = (1<<2))
#define VERT_B_Toggle()            (LATDINV= (1<<2))
#define VERT_B_OutputEnable()      (TRISDCLR = (1<<2))
#define VERT_B_InputEnable()       (TRISDSET = (1<<2))
#define VERT_B_Get()               ((PORTD >> 2) & 0x1)
#define VERT_B_GetLatch()          ((LATD >> 2) & 0x1)
#define VERT_B_PIN                  GPIO_PIN_RD2

/*** Macros for SW1_8 pin ***/
#define SW1_8_Set()               (LATDSET = (1<<3))
#define SW1_8_Clear()             (LATDCLR = (1<<3))
#define SW1_8_Toggle()            (LATDINV= (1<<3))
#define SW1_8_OutputEnable()      (TRISDCLR = (1<<3))
#define SW1_8_InputEnable()       (TRISDSET = (1<<3))
#define SW1_8_Get()               ((PORTD >> 3) & 0x1)
#define SW1_8_GetLatch()          ((LATD >> 3) & 0x1)
#define SW1_8_PIN                  GPIO_PIN_RD3

/*** Macros for RLY_PUMP pin ***/
#define RLY_PUMP_Set()               (LATDSET = (1<<0))
#define RLY_PUMP_Clear()             (LATDCLR = (1<<0))
#define RLY_PUMP_Toggle()            (LATDINV= (1<<0))
#define RLY_PUMP_OutputEnable()      (TRISDCLR = (1<<0))
#define RLY_PUMP_InputEnable()       (TRISDSET = (1<<0))
#define RLY_PUMP_Get()               ((PORTD >> 0) & 0x1)
#define RLY_PUMP_GetLatch()          ((LATD >> 0) & 0x1)
#define RLY_PUMP_PIN                  GPIO_PIN_RD0

/*** Macros for SW1_1 pin ***/
#define SW1_1_Set()               (LATCSET = (1<<3))
#define SW1_1_Clear()             (LATCCLR = (1<<3))
#define SW1_1_Toggle()            (LATCINV= (1<<3))
#define SW1_1_OutputEnable()      (TRISCCLR = (1<<3))
#define SW1_1_InputEnable()       (TRISCSET = (1<<3))
#define SW1_1_Get()               ((PORTC >> 3) & 0x1)
#define SW1_1_GetLatch()          ((LATC >> 3) & 0x1)
#define SW1_1_PIN                  GPIO_PIN_RC3

/*** Macros for SW1_2 pin ***/
#define SW1_2_Set()               (LATCSET = (1<<4))
#define SW1_2_Clear()             (LATCCLR = (1<<4))
#define SW1_2_Toggle()            (LATCINV= (1<<4))
#define SW1_2_OutputEnable()      (TRISCCLR = (1<<4))
#define SW1_2_InputEnable()       (TRISCSET = (1<<4))
#define SW1_2_Get()               ((PORTC >> 4) & 0x1)
#define SW1_2_GetLatch()          ((LATC >> 4) & 0x1)
#define SW1_2_PIN                  GPIO_PIN_RC4

/*** Macros for SW1_4 pin ***/
#define SW1_4_Set()               (LATCSET = (1<<5))
#define SW1_4_Clear()             (LATCCLR = (1<<5))
#define SW1_4_Toggle()            (LATCINV= (1<<5))
#define SW1_4_OutputEnable()      (TRISCCLR = (1<<5))
#define SW1_4_InputEnable()       (TRISCSET = (1<<5))
#define SW1_4_Get()               ((PORTC >> 5) & 0x1)
#define SW1_4_GetLatch()          ((LATC >> 5) & 0x1)
#define SW1_4_PIN                  GPIO_PIN_RC5

/*** Macros for uC_U1TX pin ***/
#define uC_U1TX_Get()               ((PORTC >> 12) & 0x1)
#define uC_U1TX_GetLatch()          ((LATC >> 12) & 0x1)
#define uC_U1TX_PIN                  GPIO_PIN_RC12

/*** Macros for RLY_FAN pin ***/
#define RLY_FAN_Get()               ((PORTC >> 14) & 0x1)
#define RLY_FAN_GetLatch()          ((LATC >> 14) & 0x1)
#define RLY_FAN_PIN                  GPIO_PIN_RC14

/*** Macros for U4_STEP pin ***/
#define U4_STEP_Get()               ((PORTC >> 15) & 0x1)
#define U4_STEP_GetLatch()          ((LATC >> 15) & 0x1)
#define U4_STEP_PIN                  GPIO_PIN_RC15

/*** Macros for SDA pin ***/
#define SDA_Get()               ((PORTB >> 5) & 0x1)
#define SDA_GetLatch()          ((LATB >> 5) & 0x1)
#define SDA_PIN                  GPIO_PIN_RB5

/*** Macros for U2_CS pin ***/
#define U2_CS_Set()               (LATBSET = (1<<6))
#define U2_CS_Clear()             (LATBCLR = (1<<6))
#define U2_CS_Toggle()            (LATBINV= (1<<6))
#define U2_CS_OutputEnable()      (TRISBCLR = (1<<6))
#define U2_CS_InputEnable()       (TRISBSET = (1<<6))
#define U2_CS_Get()               ((PORTB >> 6) & 0x1)
#define U2_CS_GetLatch()          ((LATB >> 6) & 0x1)
#define U2_CS_PIN                  GPIO_PIN_RB6

/*** Macros for U4_DIR pin ***/
#define U4_DIR_Set()               (LATCSET = (1<<10))
#define U4_DIR_Clear()             (LATCCLR = (1<<10))
#define U4_DIR_Toggle()            (LATCINV= (1<<10))
#define U4_DIR_OutputEnable()      (TRISCCLR = (1<<10))
#define U4_DIR_InputEnable()       (TRISCSET = (1<<10))
#define U4_DIR_Get()               ((PORTC >> 10) & 0x1)
#define U4_DIR_GetLatch()          ((LATC >> 10) & 0x1)
#define U4_DIR_PIN                  GPIO_PIN_RC10

/*** Macros for MB1_SDA pin ***/
#define MB1_SDA_Get()               ((PORTB >> 7) & 0x1)
#define MB1_SDA_GetLatch()          ((LATB >> 7) & 0x1)
#define MB1_SDA_PIN                  GPIO_PIN_RB7

/*** Macros for FOCUS_G pin ***/
#define FOCUS_G_Set()               (LATCSET = (1<<13))
#define FOCUS_G_Clear()             (LATCCLR = (1<<13))
#define FOCUS_G_Toggle()            (LATCINV= (1<<13))
#define FOCUS_G_OutputEnable()      (TRISCCLR = (1<<13))
#define FOCUS_G_InputEnable()       (TRISCSET = (1<<13))
#define FOCUS_G_Get()               ((PORTC >> 13) & 0x1)
#define FOCUS_G_GetLatch()          ((LATC >> 13) & 0x1)
#define FOCUS_G_PIN                  GPIO_PIN_RC13

/*** Macros for MB1_SCK pin ***/
#define MB1_SCK_Get()               ((PORTB >> 8) & 0x1)
#define MB1_SCK_GetLatch()          ((LATB >> 8) & 0x1)
#define MB1_SCK_PIN                  GPIO_PIN_RB8

/*** Macros for FOCUS_B pin ***/
#define FOCUS_B_Set()               (LATBSET = (1<<9))
#define FOCUS_B_Clear()             (LATBCLR = (1<<9))
#define FOCUS_B_Toggle()            (LATBINV= (1<<9))
#define FOCUS_B_OutputEnable()      (TRISBCLR = (1<<9))
#define FOCUS_B_InputEnable()       (TRISBSET = (1<<9))
#define FOCUS_B_Get()               ((PORTB >> 9) & 0x1)
#define FOCUS_B_GetLatch()          ((LATB >> 9) & 0x1)
#define FOCUS_B_PIN                  GPIO_PIN_RB9

/*** Macros for MB1_TX pin ***/
#define MB1_TX_Get()               ((PORTC >> 6) & 0x1)
#define MB1_TX_GetLatch()          ((LATC >> 6) & 0x1)
#define MB1_TX_PIN                  GPIO_PIN_RC6

/*** Macros for MB1_RX pin ***/
#define MB1_RX_Get()               ((PORTC >> 7) & 0x1)
#define MB1_RX_GetLatch()          ((LATC >> 7) & 0x1)
#define MB1_RX_PIN                  GPIO_PIN_RC7

/*** Macros for BLUE_LED pin ***/
#define BLUE_LED_Set()               (LATCSET = (1<<8))
#define BLUE_LED_Clear()             (LATCCLR = (1<<8))
#define BLUE_LED_Toggle()            (LATCINV= (1<<8))
#define BLUE_LED_OutputEnable()      (TRISCCLR = (1<<8))
#define BLUE_LED_InputEnable()       (TRISCSET = (1<<8))
#define BLUE_LED_Get()               ((PORTC >> 8) & 0x1)
#define BLUE_LED_GetLatch()          ((LATC >> 8) & 0x1)
#define BLUE_LED_PIN                  GPIO_PIN_RC8

/*** Macros for UX_EN pin ***/
#define UX_EN_Set()               (LATDSET = (1<<1))
#define UX_EN_Clear()             (LATDCLR = (1<<1))
#define UX_EN_Toggle()            (LATDINV= (1<<1))
#define UX_EN_OutputEnable()      (TRISDCLR = (1<<1))
#define UX_EN_InputEnable()       (TRISDSET = (1<<1))
#define UX_EN_Get()               ((PORTD >> 1) & 0x1)
#define UX_EN_GetLatch()          ((LATD >> 1) & 0x1)
#define UX_EN_PIN                  GPIO_PIN_RD1

/*** Macros for DRV8876_FAULT pin ***/
#define DRV8876_FAULT_Set()               (LATASET = (1<<5))
#define DRV8876_FAULT_Clear()             (LATACLR = (1<<5))
#define DRV8876_FAULT_Toggle()            (LATAINV= (1<<5))
#define DRV8876_FAULT_OutputEnable()      (TRISACLR = (1<<5))
#define DRV8876_FAULT_InputEnable()       (TRISASET = (1<<5))
#define DRV8876_FAULT_Get()               ((PORTA >> 5) & 0x1)
#define DRV8876_FAULT_GetLatch()          ((LATA >> 5) & 0x1)
#define DRV8876_FAULT_PIN                  GPIO_PIN_RA5

/*** Macros for SCL pin ***/
#define SCL_Get()               ((PORTC >> 9) & 0x1)
#define SCL_GetLatch()          ((LATC >> 9) & 0x1)
#define SCL_PIN                  GPIO_PIN_RC9

/*** Macros for POWER_OK pin ***/
#define POWER_OK_Set()               (LATASET = (1<<15))
#define POWER_OK_Clear()             (LATACLR = (1<<15))
#define POWER_OK_Toggle()            (LATAINV= (1<<15))
#define POWER_OK_OutputEnable()      (TRISACLR = (1<<15))
#define POWER_OK_InputEnable()       (TRISASET = (1<<15))
#define POWER_OK_Get()               ((PORTA >> 15) & 0x1)
#define POWER_OK_GetLatch()          ((LATA >> 15) & 0x1)
#define POWER_OK_PIN                  GPIO_PIN_RA15

/*** Macros for UX_SLEEP pin ***/
#define UX_SLEEP_Set()               (LATASET = (1<<14))
#define UX_SLEEP_Clear()             (LATACLR = (1<<14))
#define UX_SLEEP_Toggle()            (LATAINV= (1<<14))
#define UX_SLEEP_OutputEnable()      (TRISACLR = (1<<14))
#define UX_SLEEP_InputEnable()       (TRISASET = (1<<14))
#define UX_SLEEP_Get()               ((PORTA >> 14) & 0x1)
#define UX_SLEEP_GetLatch()          ((LATA >> 14) & 0x1)
#define UX_SLEEP_PIN                  GPIO_PIN_RA14

/*** Macros for U3_CS pin ***/
#define U3_CS_Set()               (LATBSET = (1<<10))
#define U3_CS_Clear()             (LATBCLR = (1<<10))
#define U3_CS_Toggle()            (LATBINV= (1<<10))
#define U3_CS_OutputEnable()      (TRISBCLR = (1<<10))
#define U3_CS_InputEnable()       (TRISBSET = (1<<10))
#define U3_CS_Get()               ((PORTB >> 10) & 0x1)
#define U3_CS_GetLatch()          ((LATB >> 10) & 0x1)
#define U3_CS_PIN                  GPIO_PIN_RB10

/*** Macros for U4_CS pin ***/
#define U4_CS_Set()               (LATBSET = (1<<11))
#define U4_CS_Clear()             (LATBCLR = (1<<11))
#define U4_CS_Toggle()            (LATBINV= (1<<11))
#define U4_CS_OutputEnable()      (TRISBCLR = (1<<11))
#define U4_CS_InputEnable()       (TRISBSET = (1<<11))
#define U4_CS_Get()               ((PORTB >> 11) & 0x1)
#define U4_CS_GetLatch()          ((LATB >> 11) & 0x1)
#define U4_CS_PIN                  GPIO_PIN_RB11

/*** Macros for MB1_SCL pin ***/
#define MB1_SCL_Get()               ((PORTB >> 13) & 0x1)
#define MB1_SCL_GetLatch()          ((LATB >> 13) & 0x1)
#define MB1_SCL_PIN                  GPIO_PIN_RB13


// *****************************************************************************
/* GPIO Port

  Summary:
    Identifies the available GPIO Ports.

  Description:
    This enumeration identifies the available GPIO Ports.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all ports are available on all devices.  Refer to the specific
    device data sheet to determine which ports are supported.
*/
#define    GPIO_PORT_A   (0U)
#define    GPIO_PORT_B   (1U)
#define    GPIO_PORT_C   (2U)
#define    GPIO_PORT_D   (3U)
typedef uint32_t GPIO_PORT;

typedef enum
{
    GPIO_INTERRUPT_ON_MISMATCH,
    GPIO_INTERRUPT_ON_RISING_EDGE,
    GPIO_INTERRUPT_ON_FALLING_EDGE,
    GPIO_INTERRUPT_ON_BOTH_EDGES,
}GPIO_INTERRUPT_STYLE;

// *****************************************************************************
/* GPIO Port Pins

  Summary:
    Identifies the available GPIO port pins.

  Description:
    This enumeration identifies the available GPIO port pins.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all pins are available on all devices.  Refer to the specific
    device data sheet to determine which pins are supported.
*/
#define    GPIO_PIN_RA0   (0U)
#define    GPIO_PIN_RA1   (1U)
#define    GPIO_PIN_RA2   (2U)
#define    GPIO_PIN_RA3   (3U)
#define    GPIO_PIN_RA4   (4U)
#define    GPIO_PIN_RA5   (5U)
#define    GPIO_PIN_RA6   (6U)
#define    GPIO_PIN_RA7   (7U)
#define    GPIO_PIN_RA8   (8U)
#define    GPIO_PIN_RA9   (9U)
#define    GPIO_PIN_RA10   (10U)
#define    GPIO_PIN_RA11   (11U)
#define    GPIO_PIN_RA12   (12U)
#define    GPIO_PIN_RA13   (13U)
#define    GPIO_PIN_RA14   (14U)
#define    GPIO_PIN_RA15   (15U)
#define    GPIO_PIN_RB0   (16U)
#define    GPIO_PIN_RB1   (17U)
#define    GPIO_PIN_RB2   (18U)
#define    GPIO_PIN_RB3   (19U)
#define    GPIO_PIN_RB4   (20U)
#define    GPIO_PIN_RB5   (21U)
#define    GPIO_PIN_RB6   (22U)
#define    GPIO_PIN_RB7   (23U)
#define    GPIO_PIN_RB8   (24U)
#define    GPIO_PIN_RB9   (25U)
#define    GPIO_PIN_RB10   (26U)
#define    GPIO_PIN_RB11   (27U)
#define    GPIO_PIN_RB13   (29U)
#define    GPIO_PIN_RB14   (30U)
#define    GPIO_PIN_RB15   (31U)
#define    GPIO_PIN_RC0   (32U)
#define    GPIO_PIN_RC1   (33U)
#define    GPIO_PIN_RC2   (34U)
#define    GPIO_PIN_RC3   (35U)
#define    GPIO_PIN_RC4   (36U)
#define    GPIO_PIN_RC5   (37U)
#define    GPIO_PIN_RC6   (38U)
#define    GPIO_PIN_RC7   (39U)
#define    GPIO_PIN_RC8   (40U)
#define    GPIO_PIN_RC9   (41U)
#define    GPIO_PIN_RC10   (42U)
#define    GPIO_PIN_RC11   (43U)
#define    GPIO_PIN_RC12   (44U)
#define    GPIO_PIN_RC13   (45U)
#define    GPIO_PIN_RC14   (46U)
#define    GPIO_PIN_RC15   (47U)
#define    GPIO_PIN_RD0   (48U)
#define    GPIO_PIN_RD1   (49U)
#define    GPIO_PIN_RD2   (50U)
#define    GPIO_PIN_RD3   (51U)
#define    GPIO_PIN_RD4   (52U)

    /* This element should not be used in any of the GPIO APIs.
       It will be used by other modules or application to denote that none of the GPIO Pin is used */
#define    GPIO_PIN_NONE   (-1)

typedef uint32_t GPIO_PIN;


void GPIO_Initialize(void);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on multiple pins of a port
// *****************************************************************************
// *****************************************************************************

uint32_t GPIO_PortRead(GPIO_PORT port);

void GPIO_PortWrite(GPIO_PORT port, uint32_t mask, uint32_t value);

uint32_t GPIO_PortLatchRead ( GPIO_PORT port );

void GPIO_PortSet(GPIO_PORT port, uint32_t mask);

void GPIO_PortClear(GPIO_PORT port, uint32_t mask);

void GPIO_PortToggle(GPIO_PORT port, uint32_t mask);

void GPIO_PortInputEnable(GPIO_PORT port, uint32_t mask);

void GPIO_PortOutputEnable(GPIO_PORT port, uint32_t mask);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on one pin at a time
// *****************************************************************************
// *****************************************************************************

static inline void GPIO_PinWrite(GPIO_PIN pin, bool value)
{
    GPIO_PortWrite((GPIO_PORT)(pin>>4), (uint32_t)(0x1) << (pin & 0xFU), (uint32_t)(value) << (pin & 0xFU));
}

static inline bool GPIO_PinRead(GPIO_PIN pin)
{
    return (bool)(((GPIO_PortRead((GPIO_PORT)(pin>>4))) >> (pin & 0xFU)) & 0x1U);
}

static inline bool GPIO_PinLatchRead(GPIO_PIN pin)
{
    return (bool)((GPIO_PortLatchRead((GPIO_PORT)(pin>>4)) >> (pin & 0xFU)) & 0x1U);
}

static inline void GPIO_PinToggle(GPIO_PIN pin)
{
    GPIO_PortToggle((GPIO_PORT)(pin>>4), 0x1UL << (pin & 0xFU));
}

static inline void GPIO_PinSet(GPIO_PIN pin)
{
    GPIO_PortSet((GPIO_PORT)(pin>>4), 0x1UL << (pin & 0xFU));
}

static inline void GPIO_PinClear(GPIO_PIN pin)
{
    GPIO_PortClear((GPIO_PORT)(pin>>4), 0x1UL << (pin & 0xFU));
}

static inline void GPIO_PinInputEnable(GPIO_PIN pin)
{
    GPIO_PortInputEnable((GPIO_PORT)(pin>>4), 0x1UL << (pin & 0xFU));
}

static inline void GPIO_PinOutputEnable(GPIO_PIN pin)
{
    GPIO_PortOutputEnable((GPIO_PORT)(pin>>4), 0x1UL << (pin & 0xFU));
}


// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif
// DOM-IGNORE-END
#endif // PLIB_GPIO_H
