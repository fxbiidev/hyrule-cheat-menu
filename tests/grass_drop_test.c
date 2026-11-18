// Production grass-drop policy with mock native calls. No emulator gameplay claim.
#include "guest.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base;

static struct {
    _Alignas(8) unsigned char processor[0xb8], source[0xf0], request[0x50], actor[0x28];
    _Alignas(8) unsigned char foliage[0xf8], scene[0x338], table_handle[0x48];
    uintptr_t tables[9], foliage_slot, foliage_table;
    char name[40], actor_override[40], table_override[40];
    char table_name[96];
} fixture;
static unsigned epoch, resolve_calls, hash_calls;
static int stock_count;
static uintptr_t stock_table;
static bool disable_during_hash, disable_during_resolve;
static bool expect_request_copy, expect_stock_retry;

struct DropList;
static void mock_drop_resolve(uintptr_t source, struct DropList *result, uintptr_t request);
static unsigned mock_drop_hash(const char *name, unsigned salt);

unsigned save_epoch(void) { return epoch; }
uint64_t monotonic_ms(void) { return 1234; }
bool mapped(uintptr_t address, size_t size, unsigned permission) {
    uintptr_t start = (uintptr_t)&fixture, end = start + sizeof fixture;
    return permission == 1 && address >= start && address <= end && size <= end - address;
}

#include "../src/grass_drop.c"

static void set_pointer(unsigned char *object, size_t offset, const void *address) {
    uintptr_t value = (uintptr_t)address;
    memcpy(object + offset, &value, sizeof value);
}

static void set_u32(unsigned char *object, size_t offset, uint32_t value) {
    memcpy(object + offset, &value, sizeof value);
}

static void setup(void) {
    memset(&fixture, 0, sizeof fixture);
    epoch = 11;
    main_base = (uintptr_t)&fixture.foliage_slot - (FOLIAGE_MODULE - GAME_BASE);
    stock_count = 1;
    stock_table = fixture.tables[0] = 0x1000;
    fixture.tables[1] = 0x2000;
    disable_during_hash = disable_during_resolve = false;
    expect_request_copy = expect_stock_retry = false;
    resolve_calls = hash_calls = 0;
    strcpy(fixture.name, "Obj_Plant_Grass_A_01");
    set_pointer(fixture.actor, 0x20, fixture.name);
    set_pointer(fixture.source, 0, fixture.actor);
    set_u32(fixture.source, 0x28, 2);
    set_pointer(fixture.source, 0x30, fixture.tables);
    set_pointer(fixture.source, 0x48, fixture.actor_override);
    set_pointer(fixture.source, 0x98, fixture.table_override);
    set_u32(fixture.source, 0xe8, 10);
    set_u32(fixture.source, 0xec, 1);
    set_pointer(fixture.request, 0x40, fixture.actor);
    set_u32(fixture.request, 0x48, 7);
    grass_luck_publish(epoch, true);
}

static void set_terrain_table(const char *name) {
    strcpy(fixture.table_name, name);
    set_pointer(fixture.table_handle, 0x38, fixture.table_name);
    set_u32(fixture.table_handle, 0x40, (uint32_t)strlen(name) + 1);
}

static void setup_terrain(void) {
    setup();
    fixture.foliage_slot = (uintptr_t)fixture.foliage;
    set_pointer(fixture.foliage, 0xf0, fixture.scene);
    set_pointer(fixture.scene, 0x330, fixture.processor);
    assert((uintptr_t)fixture.source == (uintptr_t)fixture.processor + 0xb8);
    set_pointer(fixture.request, 0x40, NULL);
    fixture.request[0x4c] = fixture.request[0x4d] = 1;
    set_u32(fixture.source, 0x28, 9);
    set_terrain_table("Drop/DropTable/DropTable_Foliage_00");
    fixture.foliage_table = (uintptr_t)fixture.table_handle;
    stock_table = fixture.tables[0] = (uintptr_t)&fixture.foliage_table;
}

static void stock_output(DropList *result) {
    memset(result, 0, sizeof *result);
    result->count = stock_count;
    result->capacity = 16;
    result->entries = result->slots;
    result->table = stock_table;
    result->owner = pointer((uintptr_t)fixture.request + 0x40);
    for (int i = 0; i < stock_count; ++i) {
        result->slots[i] = &result->pool[i];
        result->pool[i] = (DropEntry){"Stock_Item", 0x123u, 0, 0x2000, 0x3000};
    }
    if (stock_count < result->capacity) result->free_entry = &result->pool[stock_count];
    for (int i = stock_count; i + 1 < result->capacity; ++i)
        result->pool[i].name = (const char *)&result->pool[i + 1];
}

static void mock_drop_resolve(uintptr_t source, struct DropList *result, uintptr_t request) {
    assert(source == (uintptr_t)fixture.source);
    if (expect_request_copy) {
        assert(request != (uintptr_t)fixture.request);
        unsigned char expected[sizeof fixture.request];
        memcpy(expected, fixture.request, sizeof expected);
        expected[0x4d] = 0;
        assert(!memcmp((void *)request, expected, sizeof expected));
        if (disable_during_resolve) grass_luck_publish(epoch, false);
    } else assert(request == (uintptr_t)fixture.request);
    ++resolve_calls;
    stock_output(result);
    if (expect_request_copy && expect_stock_retry) result->reserved = 0xfeed;
    expect_request_copy = false;
}

enum { GREEN_HASH = 0x12345678, BLUE_HASH = 0x42345678 };

static unsigned mock_drop_hash(const char *name, unsigned salt) {
    assert(!salt);
    ++hash_calls;
    if (disable_during_hash) grass_luck_publish(epoch, false);
    if (!strcmp(name, "Obj_Rupee_Green")) return GREEN_HASH;
    assert(!strcmp(name, "Obj_Rupee_Blue"));
    return BLUE_HASH;
}

static void resolve(DropList *result) {
    unsigned char before[sizeof fixture];
    memcpy(before, &fixture, sizeof before);
    unsigned previous_calls = resolve_calls;
    grass_drop((uintptr_t)fixture.source, result, (uintptr_t)fixture.request);
    assert(resolve_calls == previous_calls + 1 + expect_stock_retry);
    assert(!memcmp(before, &fixture, sizeof before));
}

static void expect_stock(unsigned expected_hash_calls) {
    DropList result;
    unsigned char before[sizeof result];
    stock_output(&result);
    memcpy(before, &result, sizeof before);
    unsigned previous_calls = hash_calls;
    resolve(&result);
    assert(hash_calls == previous_calls + expected_hash_calls);
    assert(!memcmp(before, &result, sizeof before));
}

static void expect_guaranteed(void) {
    DropList result;
    unsigned previous_calls = hash_calls;
    resolve(&result);
    assert(hash_calls == previous_calls + 1);
    assert(result.count == 1 && result.capacity == 16 && result.entries == result.slots);
    assert(result.entries[0] == &result.pool[0]);
    assert(result.table == stock_table && result.owner == pointer((uintptr_t)fixture.request + 0x40));
    DropEntry *entry = result.entries[0];
    unsigned expected = !strcmp(entry->name, "Obj_Rupee_Green") ? GREEN_HASH : BLUE_HASH;
    assert(expected == GREEN_HASH || !strcmp(entry->name, "Obj_Rupee_Blue"));
    assert(entry->hash == expected && !entry->subtable && !entry->item);
    int consumed = stock_count ? stock_count : 1;
    assert(result.free_entry == (consumed == 16 ? NULL : &result.pool[consumed]));
}

static void distribution(void) {
    unsigned green = 0, blue = 0;
    for (unsigned roll = 0; roll < 100; ++roll) {
        const char *rupee = rupee_for_roll(roll);
        green += !strcmp(rupee, "Obj_Rupee_Green");
        blue += !strcmp(rupee, "Obj_Rupee_Blue");
    }
    assert(green == 85 && blue == 15);
    assert(!strcmp(rupee_for_roll(84), "Obj_Rupee_Green"));
    assert(!strcmp(rupee_for_roll(85), "Obj_Rupee_Blue"));

    unsigned rolls[16];
    luck_random = 0;
    for (unsigned i = 0; i < sizeof rolls / sizeof *rolls; ++i) rolls[i] = roll_percent();
    luck_random = 0;
    for (unsigned i = 0; i < sizeof rolls / sizeof *rolls; ++i) assert(roll_percent() == rolls[i]);
    green = 0;
    for (unsigned i = 0; i < 100000; ++i) {
        unsigned roll = roll_percent();
        assert(roll < 100);
        green += roll < 85;
    }
    assert(green >= 84500 && green <= 85500);
}

int main(void) {
    distribution();
    const int counts[] = {0, 1, 4, 16};
    for (unsigned i = 0; i < sizeof counts / sizeof *counts; ++i) {
        setup();
        stock_count = counts[i];
        expect_guaranteed();
    }
    strcpy(fixture.name, "Obj_Plant_Grass_A_01_Respawn");
    expect_guaranteed();

    grass_luck_publish(epoch, false);
    expect_stock(0);
    grass_luck_publish(epoch, true);
    ++epoch;
    expect_stock(0);
    grass_luck_publish(epoch, true);
    expect_guaranteed();
    disable_during_hash = true;
    expect_stock(1);
    expect_stock(0);

    setup(); strcpy(fixture.actor_override, "Obj_Quest_Item"); expect_stock(0);
    setup(); strcpy(fixture.table_override, "Special"); expect_stock(0);
    setup(); stock_table = fixture.tables[1]; expect_stock(0);
    setup(); set_u32(fixture.request, 0x48, 6); expect_stock(0);
    const unsigned excluded_causes[][2] = {{4, 1}, {6, 3}, {5, 8}, {10, 3}};
    for (unsigned i = 0; i < sizeof excluded_causes / sizeof *excluded_causes; ++i) {
        setup();
        set_u32(fixture.source, 0xe8, excluded_causes[i][0]);
        set_u32(fixture.source, 0xec, excluded_causes[i][1]);
        expect_stock(0);
    }
    const char *excluded[] = {"Obj_Plant_Grass_A_01_Special", "Obj_Pot_A_01", "Enemy_Keese", "TreasureBox"};
    for (unsigned i = 0; i < sizeof excluded / sizeof *excluded; ++i) {
        setup(); strcpy(fixture.name, excluded[i]); expect_stock(0);
    }

    setup_terrain(); expect_request_copy = true; stock_count = 0; expect_guaranteed();
    setup_terrain(); expect_request_copy = true; stock_count = 4; expect_guaranteed();
    setup_terrain(); grass_luck_publish(epoch, false); expect_stock(0);
    setup_terrain(); ++epoch; expect_stock(0);
    setup_terrain(); fixture.request[0x4c] = 0; expect_stock(0);
    setup_terrain(); fixture.request[0x4d] = 0; expect_stock(0);
    setup_terrain(); set_pointer(fixture.scene, 0x330, NULL); expect_stock(0);
    const char *excluded_tables[] = {
        "Drop/DropTable/DropTable_Quest", "Drop/DropTable/DropTable_Foliage_00Extra",
        "Drop/DropTable/DropTable_Foliage_00.game__drop__DropTable.bgyml",
        "Quest/DropTable/DropTable_Foliage_00",
    };
    for (unsigned i = 0; i < sizeof excluded_tables / sizeof *excluded_tables; ++i) {
        setup_terrain(); set_terrain_table(excluded_tables[i]); expect_stock(0);
    }
    setup_terrain(); expect_request_copy = expect_stock_retry = true;
    stock_table = fixture.tables[1]; expect_stock(0);
    setup_terrain(); expect_request_copy = expect_stock_retry = disable_during_resolve = true; expect_stock(0);
    setup_terrain(); expect_request_copy = expect_stock_retry = disable_during_hash = true; expect_stock(1);
    puts("Grass drop fixture passed exact 85/15 boundaries, reproducible sampling, single-entry results and stock fallbacks.");
}
