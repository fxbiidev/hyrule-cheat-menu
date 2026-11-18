#pragma once

#include <stdbool.h>
#include <stdint.h>

void grass_luck_publish(unsigned epoch, bool enabled);
void grass_drop(uintptr_t source, void *result, uintptr_t request);
