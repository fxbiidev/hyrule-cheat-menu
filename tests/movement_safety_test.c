#include "guest.h"
#include "movement_safety.h"
#include <assert.h>
#include <stdio.h>

static unsigned entries[3][8];
static struct { unsigned count, padding; uintptr_t table; } manager;
static uintptr_t manager_slot;
uintptr_t main_base;
static bool readable = true;
bool mapped(uintptr_t address, size_t bytes, unsigned permissions) {
    assert(permissions == 1);
    return readable && ((address == (uintptr_t)&manager && bytes <= sizeof manager) ||
                        (address == (uintptr_t)entries && bytes <= sizeof entries));
}
#include "../src/movement_safety.c"
int main(void) {
    manager_slot = (uintptr_t)&manager;
    main_base = (uintptr_t)&manager_slot - (0x84eb7e38ull - GAME_BASE);
    manager.count = 3; manager.table = (uintptr_t)entries;
    entries[0][2] = 0xb5b0a658; entries[0][7] = 1; // Menu's own pause is independent.
    entries[1][2] = 0x0eafe200;
    assert(movement_ready());
    entries[1][7] = 1; assert(!movement_ready());
    entries[1][7] = 3; assert(!movement_ready());
    entries[1][7] = 0; assert(movement_ready()); // Selected speeds need not change.
    entries[1][7] = 0x80000000u; assert(!movement_ready());
    entries[1][7] = 0; entries[2][2] = 0x0eafe200; assert(!movement_ready());
    entries[2][2] = 0; entries[1][2] = 0; assert(!movement_ready());
    entries[1][2] = 0x0eafe200; readable = false; assert(!movement_ready());
    readable = true; manager.count = 129; assert(!movement_ready());
    manager.count = 0; assert(!movement_ready());
    manager_slot = 0; assert(!movement_ready());
    puts("Native loading gate: ready, nested pauses, resume, independent menu pause and invalid tables passed.");
}
