#pragma once
#include "recovery.h"

#define GEOMETRY_FLOOR_CLEARANCE 0.05f
#include <stdint.h>

// Called only by the identified Link job in a safe, unchanged save generation.
RecoveryResult geometry_recover(uintptr_t actor, bool grounded, float result[3]);
// Native downward ray only; solid walls are intentionally allowed for Noclip.
int geometry_floor(uintptr_t actor, const float position[3], float *root_height);
