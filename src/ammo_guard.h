#pragma once
#include <stdint.h>

void ammo_set_owner(uintptr_t actor, unsigned generation);
void ammo_use(uintptr_t pouch, const char **name, unsigned amount,
              unsigned alternate, unsigned flag4, unsigned flag5);
void ammo_nut_use(uintptr_t pouch, const char **name, unsigned amount,
                  unsigned alternate, unsigned flag4, unsigned flag5);
