#pragma once
#include <stdint.h>
#include <stdbool.h>

void items_update(uintptr_t pouch, uintptr_t actor, unsigned observed[2], bool refill,
                  unsigned generation);
// Caller supplies a readable, terminated name and the identified current Link actor.
bool items_supported_for_age(uintptr_t actor, const char *name);
