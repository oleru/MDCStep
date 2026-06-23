#ifndef ENDSTOP_H
#define ENDSTOP_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ENDSTOP_VERT_G = 0,
    ENDSTOP_VERT_B,
    ENDSTOP_FOCUS_G,
    ENDSTOP_FOCUS_B
} endstop_id_t;

/**
 * Initialize endstop driver.
 * - Samples current pin levels and initializes debounce state.
 * - Updates MBS_HoldRegisters[MBS_SL_STATUS] immediately (incl. fault bits),
 *   which is important to detect missing sensor-board at boot (both low).
 */
void ENDSTOP_Init(void);

/**
 * Call from main loop at a fixed 1ms cadence to debounce inputs, detect changes
 * and keep Modbus status bits updated.
 */
void ENDSTOP_Task_1ms(void);

/** Returns debounced raw GPIO level (true = pin high). */
bool ENDSTOP_GetRaw(endstop_id_t id);

/** Returns logical active state (true = endstop active). Active-low handled internally. */
bool ENDSTOP_IsActive(endstop_id_t id);

#endif
