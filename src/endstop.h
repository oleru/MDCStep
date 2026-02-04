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

typedef void (*endstop_callback_t)(endstop_id_t id, bool state);

void ENDSTOP_Init(void);
void ENDSTOP_RegisterCallback(endstop_callback_t cb);

#endif
