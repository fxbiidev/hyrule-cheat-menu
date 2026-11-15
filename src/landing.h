#pragma once
#include <stdint.h>

// Called only from the verified native fall-damage dispatch site.
void flight_fall_damage(uintptr_t state, uintptr_t definition, void *request);
