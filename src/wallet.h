#pragma once
#include <stdbool.h>
#include <stdint.h>

void wallet_reset(void);
void wallet_update(uintptr_t pouch, uintptr_t data, unsigned generation,
                   bool enabled, bool authenticate, bool loaded);
