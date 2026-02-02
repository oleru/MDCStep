#ifndef TLV493D_H
#define TLV493D_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* =========================================================================
 * Data structure (raw values from TLV493D-A1B6)
 *  - x/y/z: signed 12-bit values sign-extended to int16_t
 *  - temperature: signed 12-bit raw temperature (datasheet "T" in 0.1/?? is device-specific).
 * ========================================================================= */
typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t temperature;

    uint8_t frame;
    uint8_t channel;
    bool    powerDown;
} TLV493D_Data_t;

/* =========================================================================
 * Non-blocking driver API
 * =========================================================================
 * Usage pattern:
 *   - Register TLV493D_I2C_Callback as I2C1 callback
 *   - Call TLV493D_Init(addr) once at boot
 *   - Call TLV493D_Task(now_ms) periodically (e.g. every 50ms)
 *   - Call TLV493D_GetLatest(&data, now_ms, &age_ms) when you want the latest sample
 */
void TLV493D_Init(uint8_t i2c_addr);
void TLV493D_Task(uint32_t now_ms);
bool TLV493D_GetLatest(TLV493D_Data_t *out, uint32_t now_ms, uint32_t *age_ms);

/* Pass this directly to I2C1_CallbackRegister() */
void TLV493D_I2C_Callback(uintptr_t context);

#endif /* TLV493D_H */
