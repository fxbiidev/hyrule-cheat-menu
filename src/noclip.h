#pragma once
#include "menu.h"

bool noclip_available(Menu *menu, uintptr_t actor, bool safe);
void noclip_reset(void);
bool noclip_holds_saves(void);
bool noclip_busy(void);
// Ground bypass applies the shared multiplier itself. Other native movement
// paths retain their existing scaling, including swimming and climbing.
float noclip_native_ground_factor(uintptr_t actor, unsigned speed);
void noclip_update(Menu *menu, uintptr_t actor, bool safe, unsigned expected_epoch);
uintptr_t noclip_move(uintptr_t actor, uintptr_t output, uintptr_t context, uintptr_t flags);
