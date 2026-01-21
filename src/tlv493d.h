#ifndef TLV493D_H
#define TLV493D_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * TLV493D I2C ADDRESS
 *
 * Sensoren starter alltid på 0x1F etter reset.
 * Hvis IICAddr-bitene i MOD1 bevares, forblir adressen 0x1F.
 * ========================================================================= */
#define TLV493D_I2C_ADDR   0x1F   /* 7-bit adresse */

#define TLV493D_ADDR_FACTORY_WRITE  0x3E   // 0x1F << 1
#define TLV493D_ADDR_FACTORY_READ   0x3F

#define TLV493D_ADDR_RUN_WRITE      0x7E   // 0x3F << 1
#define TLV493D_ADDR_RUN_READ       0x7F
/* =========================================================================
 * Data structure
 * ========================================================================= */
typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
    int8_t  temperature;

    uint8_t frame;
    uint8_t channel;
    bool    powerDown;
} TLV493D_Data_t;

/* =========================================================================
 * API
 * ========================================================================= */
bool TLV493D_Init(void);
bool TLV493D_Read(TLV493D_Data_t *data);

#endif /* TLV493D_H */
