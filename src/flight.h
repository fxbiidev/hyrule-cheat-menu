#pragma once
#include "menu.h"

bool flight_available(Menu *menu, uintptr_t actor, bool safe);
void flight_reset(void);
bool flight_holds_saves(void);
// Consulted only by the verified native landing-damage callsite. Consumes one
// Fly-deactivation fall; ordinary enemy/environment damage never uses this.
bool flight_consume_landing_protection(uintptr_t actor);
void flight_update(Menu *menu, uintptr_t actor, bool safe, bool paused,
                   int32_t horizontal, int32_t vertical, uint32_t buttons, unsigned expected_epoch);
void flight_step(uintptr_t states, uintptr_t unused, float *output, uintptr_t context);
void flight_job(uintptr_t job);
bool flight_velocity(uintptr_t context_handle, float velocity[3]);
