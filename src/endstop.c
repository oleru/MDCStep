#include "endstop.h"

static endstop_callback_t s_cb = 0;

//static inline bool read_vert_g(void)  { return GPIO_PinRead(GPIO_PIN_RD4); }
//static inline bool read_vert_b(void)  { return GPIO_PinRead(GPIO_PIN_RD2); }
//static inline bool read_focus_g(void) { return GPIO_PinRead(GPIO_PIN_RC13); }
//static inline bool read_focus_b(void) { return GPIO_PinRead(GPIO_PIN_RB9); }

/* Harmony callback-signatur: void (*)(GPIO_PIN pin, uintptr_t context) */
//static void endstop_pin_cb(GPIO_PIN pin, uintptr_t context)
//{
//    (void)context;

//    if (!s_cb) return;

//    if (pin == GPIO_PIN_RD4)  s_cb(ENDSTOP_VERT_G,  read_vert_g());
//    else if (pin == GPIO_PIN_RD2)  s_cb(ENDSTOP_VERT_B,  read_vert_b());
//    else if (pin == GPIO_PIN_RC13) s_cb(ENDSTOP_FOCUS_G, read_focus_g());
//    else if (pin == GPIO_PIN_RB9)  s_cb(ENDSTOP_FOCUS_B, read_focus_b());
//}

void ENDSTOP_RegisterCallback(endstop_callback_t cb)
{
    s_cb = cb;
}

void ENDSTOP_Init(void)
{

}
