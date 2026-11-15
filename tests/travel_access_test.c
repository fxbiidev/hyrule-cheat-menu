#include "guest.h"
#include "travel_access.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base;
static uintptr_t manager_slot;
static _Alignas(8) unsigned char manager[0x78], records[0x2c0], steps[0x240];
static unsigned epoch = 5, calls;
static uint8_t received;
static bool readable = true, world = true, reset_in_bool, reset_in_count, lose_manager;
static int count;
unsigned save_epoch(void) { return epoch; }
bool world_safe(void) { return world; }
bool mapped(uintptr_t address, size_t bytes, unsigned permissions) {
    assert(permissions == 1);
    return readable && ((address == (uintptr_t)manager && bytes <= sizeof manager) ||
                        (address == (uintptr_t)records && bytes <= sizeof records) ||
                        (address == (uintptr_t)steps && bytes <= sizeof steps));
}
static void put32(unsigned char *p, unsigned value) { memcpy(p, &value, 4); }
static void put_pointer(unsigned char *p, uintptr_t value) { memcpy(p, &value, sizeof value); }
static unsigned mock_access_bool(uintptr_t data, uint8_t *out, uint32_t key) {
    assert(data == 20 && key == 0x0e6264c7u);
    *out = received;
    if (reset_in_bool) ++epoch;
    if (lose_manager) manager_slot = 0;
    return readable;
}
static int mock_access_count(uintptr_t pouch, const char **name, unsigned alt, unsigned flags) {
    assert(pouch == 1 && !alt && !flags && !strcmp(*name, "Item_Token_Gerudo"));
    ++calls;
    if (reset_in_count) ++epoch;
    return count;
}
#include "../src/travel_access.c"
int main(void) {
    manager_slot = (uintptr_t)manager;
    main_base = (uintptr_t)&manager_slot - (0x84e49950ull - GAME_BASE);
    put32(manager + 0x40, 1); put_pointer(manager + 0x48, (uintptr_t)records);
    put32(records + 8, 0x6371358du); put32(records + 0x80, 4);
    put_pointer(records + 0x88, (uintptr_t)steps);
    put32(steps + 2 * 0x90 + 0x78, 0xeb62add9u);
    count = received = 1;
    // Live Ready and Step01 stay locked even if another mod changed archived quest enums.
    for (unsigned current = 0; current <= 2; ++current) {
        put32(records + 0x7c, current); assert(!gerudo_admission(1, 20, epoch) && !calls);
    }
    put32(records + 0x7c, 3); assert(gerudo_admission(1, 20, epoch));
    received = 0; assert(!gerudo_admission(1, 20, epoch));
    received = 2; assert(!gerudo_admission(1, 20, epoch)); received = 1;
    count = 0; assert(!gerudo_admission(1, 20, epoch));
    count = 2; assert(!gerudo_admission(1, 20, epoch));
    count = -1; assert(!gerudo_admission(1, 20, epoch)); count = 1;
    readable = false; assert(!gerudo_admission(1, 20, epoch)); readable = true;
    manager[0x74] = 1; assert(!gerudo_admission(1, 20, epoch)); manager[0x74] = 0;
    world = false; assert(!gerudo_admission(1, 20, epoch)); world = true;
    put32(records + 0x7c, 4); assert(!gerudo_admission(1, 20, epoch)); put32(records + 0x7c, 3);
    put32(records + 0x80, 129); assert(!gerudo_admission(1, 20, epoch)); put32(records + 0x80, 4);
    put32(manager + 0x40, 513); assert(!gerudo_admission(1, 20, epoch)); put32(manager + 0x40, 1);
    put32(steps + 0x78, 0xeb62add9u); assert(!gerudo_admission(1, 20, epoch)); put32(steps + 0x78, 0);
    memcpy(records + 0x160, records, 0x160); put32(manager + 0x40, 2);
    assert(!gerudo_admission(1, 20, epoch)); put32(manager + 0x40, 1);
    reset_in_bool = true; assert(!gerudo_admission(1, 20, epoch)); reset_in_bool = false;
    reset_in_count = true; assert(!gerudo_admission(1, 20, epoch)); reset_in_count = false;
    lose_manager = true; assert(!gerudo_admission(1, 20, epoch)); lose_manager = false;
    manager_slot = (uintptr_t)manager;
    assert(!gerudo_admission(1, 20, epoch - 1));
    assert(!gerudo_admission(0, 20, epoch) && !gerudo_admission(1, 0, epoch));
    puts("Gerudo admission: live guard quest order, stale archive isolation, card/receipt, invalid layouts and save fences passed.");
}
