#include "endstop.h"
#include "definitions.h"
#include "ModbusSlave.h"

#ifndef ENDSTOP_DEBOUNCE_MS
// Hall sensors normally do not need debounce, but we keep a small default to
// suppress short glitches/noise. Set to 0 to disable.
#define ENDSTOP_DEBOUNCE_MS  0u
#endif

static uint8_t s_raw_last = 0;
static uint8_t s_stable   = 0;
static uint8_t s_db_cnt[4] = {0,0,0,0};

static inline uint8_t read_raw_bits(void)
{
    uint8_t s = 0;

    if (VERT_G_Get())   s |= (1u << ENDSTOP_VERT_G);
    if (VERT_B_Get())   s |= (1u << ENDSTOP_VERT_B);
    if (FOCUS_G_Get())  s |= (1u << ENDSTOP_FOCUS_G);
    if (FOCUS_B_Get())  s |= (1u << ENDSTOP_FOCUS_B);

    return s;
}

// CC6201ST: output is normally HIGH, goes LOW when active
static inline bool is_active_low(bool pin_high)
{
    return !pin_high;
}

static void update_modbus_status(void)
{
    volatile uint16_t *reg = &MBS_HoldRegisters[MBS_SL_STATUS];

    const bool vert_g_active  = is_active_low(((s_stable >> ENDSTOP_VERT_G)  & 1u) != 0u);
    const bool vert_b_active  = is_active_low(((s_stable >> ENDSTOP_VERT_B)  & 1u) != 0u);
    const bool focus_g_active = is_active_low(((s_stable >> ENDSTOP_FOCUS_G) & 1u) != 0u);
    const bool focus_b_active = is_active_low(((s_stable >> ENDSTOP_FOCUS_B) & 1u) != 0u);

    const uint16_t clear_mask =
        (uint16_t)(MBS_SL_STATUS_END_STOP_VERT_B |
                   MBS_SL_STATUS_END_STOP_VERT_Y |
                   MBS_SL_STATUS_END_STOP_FOCUS_B |
                   MBS_SL_STATUS_END_STOP_FOCUS_Y |
                   MBS_SL_STATUS_VERT_SENSOR_FAULT |
                   MBS_SL_STATUS_FOCUS_SENSOR_FAULT);

    // Clear relevant bits first (leave other status bits untouched)
    MBS_RegClearBits(reg, clear_mask);

    // Set endstop bits
    if (vert_b_active)  MBS_RegSetBits(reg, MBS_SL_STATUS_END_STOP_VERT_B);
    if (vert_g_active)  MBS_RegSetBits(reg, MBS_SL_STATUS_END_STOP_VERT_Y);
    if (focus_b_active) MBS_RegSetBits(reg, MBS_SL_STATUS_END_STOP_FOCUS_B);
    if (focus_g_active) MBS_RegSetBits(reg, MBS_SL_STATUS_END_STOP_FOCUS_Y);

    // Fault bits: both sensors active simultaneously on same axis
    if (vert_b_active && vert_g_active)    MBS_RegSetBits(reg, MBS_SL_STATUS_VERT_SENSOR_FAULT);
    if (focus_b_active && focus_g_active)  MBS_RegSetBits(reg, MBS_SL_STATUS_FOCUS_SENSOR_FAULT);
}

void ENDSTOP_Init(void)
{
    s_raw_last = read_raw_bits();
    s_stable   = s_raw_last;

    for (unsigned i = 0; i < 4u; i++)
        s_db_cnt[i] = 0u;

    // Initial sync (important for missing board case)
    update_modbus_status();
}

void ENDSTOP_Task_1ms(void)
{
    const uint8_t raw_now = read_raw_bits();
    bool changed_any = false;

    for (endstop_id_t id = ENDSTOP_VERT_G; id <= ENDSTOP_FOCUS_B; id++)
    {
        const uint8_t mask = (uint8_t)(1u << (uint8_t)id);
        const bool raw_bit_now  = (raw_now & mask) != 0u;
        const bool raw_bit_last = (s_raw_last & mask) != 0u;
        const bool stable_bit   = (s_stable & mask) != 0u;

        if (raw_bit_now != raw_bit_last)
        {
            // Input changed since last sample -> restart debounce timer
            if (raw_bit_now) s_raw_last |= mask;
            else             s_raw_last &= (uint8_t)~mask;

            s_db_cnt[(uint8_t)id] = 0u;
            continue;
        }

#if (ENDSTOP_DEBOUNCE_MS == 0u)
        if (raw_bit_now != stable_bit)
        {
            if (raw_bit_now) s_stable |= mask;
            else             s_stable &= (uint8_t)~mask;

            changed_any = true;
        }
#else
        if (s_db_cnt[(uint8_t)id] < ENDSTOP_DEBOUNCE_MS)
        {
            s_db_cnt[(uint8_t)id]++;
            continue;
        }

        if (raw_bit_now != stable_bit)
        {
            if (raw_bit_now) s_stable |= mask;
            else             s_stable &= (uint8_t)~mask;

            changed_any = true;
        }
#endif
    }

    if (changed_any)
    {
        update_modbus_status();
    }
}

bool ENDSTOP_GetRaw(endstop_id_t id)
{
    return ((s_stable >> (uint8_t)id) & 1u) != 0u;
}

bool ENDSTOP_IsActive(endstop_id_t id)
{
    return !ENDSTOP_GetRaw(id);
}
