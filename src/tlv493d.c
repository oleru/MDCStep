#include "tlv493d.h"

/* ====== EKSTERNE I2C-FUNKSJONER (Harmony 3) ============================== */
extern bool I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len);
extern bool I2C1_Read(uint8_t addr, uint8_t *data, uint8_t len);
extern bool I2C1_WriteRead(uint8_t addr,
                           uint8_t *wdata, uint8_t wlen,
                           uint8_t *rdata, uint8_t rlen);
extern bool I2C1_IsBusy(void);

/* ====== Factory bytes ==================================================== */
static uint8_t fact7;
static uint8_t fact8;
static uint8_t fact9;

/* ======================================================================== */
/* General Call Reset (0x00)                                                 */
/* ======================================================================== */
static void tlv493d_reset(void)
{
    uint8_t dummy = 0x00;
    I2C1_Write(0x00, &dummy, 1);
    while (I2C1_IsBusy());
}

/* ======================================================================== */
/* Read factory registers (0x07..0x09)                                       */
/* ======================================================================== */
static bool tlv493d_read_factory(void)
{
    uint8_t buf[10];

    if (!I2C1_Read(TLV493D_ADDR_FACTORY_READ, buf, 10))
        return false;

    fact7 = buf[7];
    fact8 = buf[8];
    fact9 = buf[9];

    return true;
}

/* ======================================================================== */
/* Write configuration                                                       */
/* CRITICAL: IICAddr bits (MOD1[6:5]) MUST be preserved                      */
/* ======================================================================== */
//static bool tlv493d_write_config(void)
//{
//    uint8_t tx[4];
//
//    /* ---- MOD1 ----------------------------------------------------------- */
//    uint8_t mod1 = fact7;
//
//    mod1 &= ~(1 << 2);   // INT = 0
//    mod1 &= ~(1 << 1);   // FAST = 0
//    mod1 |=  (1 << 0);   // LOW = 1
//    mod1 |=  (1 << 7);   // P-bit (parity) = 1
//
//    /* DO NOT TOUCH bits 6:5 (IICAddr) */
//
//    /* ---- MOD2 ----------------------------------------------------------- */
//    uint8_t mod2 = fact9;
//
//    mod2 &= ~(1 << 7);        /* T  = 0 (temperature enabled) */
//    mod2 |=  (1 << 6);        /* LP = 1 (12 ms period) */
//
//    // ---- Write sequence ------------------------------------------------- */
//    tx[0] = 0x00;     /* write register pointer */
//    tx[1] = mod1;     /* MOD1 */
//    tx[2] = fact8;    /* reserved ? MUST be written back */
//    tx[3] = mod2;     /* MOD2 */
//
//    return I2C1_Write(TLV493D_I2C_ADDR, tx, 4);
//}
static bool tlv493d_write_config(void)
{
    uint8_t tx[4];

    uint8_t mod1 = fact7;
    mod1 &= ~(1 << 2);   // INT = 0
    mod1 &= ~(1 << 1);   // FAST = 0
    mod1 |=  (1 << 0);   // LOW = 1
    // bits 6:5 (IICAddr) beholdes

    uint8_t mod2 = fact9;
    mod2 &= ~(1 << 7);   // T = 0
    mod2 |=  (1 << 6);   // LP = 1 (12 ms)

    tx[0] = 0x00;   // <<<<<< REGISTER POINTER ? KRITISK
    tx[1] = mod1;
    tx[2] = fact8;
    tx[3] = mod2;

    return I2C1_Write(TLV493D_ADDR_FACTORY_WRITE, tx, 4);

}

/* ======================================================================== */
/* Public Init                                                               */
/* ======================================================================== */
bool TLV493D_Init(void)
{
    tlv493d_reset();

    /* Datasheet: wait >14 µs, we are conservative */
    for (volatile uint32_t i = 0; i < 50000; i++);

    if (!tlv493d_read_factory())
        return false;
 
    for (volatile uint32_t i = 0; i < 50000; i++);

    if (!tlv493d_write_config())
        return false;

    return true;
}

/* ======================================================================== */
/* Read measurement                                                          */
/* ======================================================================== */
bool TLV493D_Read(TLV493D_Data_t *d)
{
    uint8_t buf[7];
    
uint8_t tx[4] = { 0xAA, 0x55, 0xCC, 0x33 };
I2C1_Write(0x1F, tx, 4);

return true;

    if (!I2C1_Read(TLV493D_ADDR_RUN_READ, buf, 7))
        return false;

    /* X */
    d->x = ((buf[0] << 4) | (buf[4] >> 4)) & 0x0FFF;
    if (d->x & 0x800) d->x -= 0x1000;

    /* Y */
    d->y = ((buf[1] << 4) | (buf[4] & 0x0F)) & 0x0FFF;
    if (d->y & 0x800) d->y -= 0x1000;

    /* Z */
    d->z = ((buf[2] << 4) | (buf[5] & 0x0F)) & 0x0FFF;
    if (d->z & 0x800) d->z -= 0x1000;

    /* Temperature */
    d->temperature = buf[3];

    /* Status */
    d->frame     = (buf[3] >> 2) & 0x03;
    d->channel   =  buf[3]       & 0x03;
    d->powerDown = (buf[5] >> 4) & 0x01;

    return true;
}
