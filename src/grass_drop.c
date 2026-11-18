#include "grass_drop.h"
#include "guest.h"
#include "profile.h"

typedef struct DropEntry {
    const char *name;
    uint32_t hash;
    uint32_t padding;
    uintptr_t subtable, item;
} DropEntry;

typedef struct DropList {
    int count, capacity;
    DropEntry **entries;
    DropEntry *free_entry;
    uintptr_t reserved;
    DropEntry pool[16];
    DropEntry *slots[16];
    uintptr_t owner, table;
} DropList;

typedef struct DropRequest {
    unsigned char transform[0x40];
    uintptr_t actor;
    uint32_t reason;
    uint8_t special_source, terrain_chance;
} DropRequest;

_Static_assert(sizeof(DropEntry) == 0x20 && offsetof(DropList, table) == 0x2a8,
               "Native drop list layout");
_Static_assert(sizeof(DropRequest) == 0x50 && offsetof(DropRequest, terrain_chance) == 0x4d,
               "Native drop request layout");

static uint64_t luck_publication;
static uint64_t luck_random;

// Read by the isolated emulator fixture alongside the normal spawned actors.
static struct {
    unsigned placed_requests, terrain_requests, green, blue;
    unsigned last_reason, last_match;
} grass_drop_evidence;
static uintptr_t terrain_table_path;

void grass_luck_publish(unsigned epoch, bool enabled) {
    uint64_t state = ((uint64_t)epoch << 1) | enabled;
    __atomic_store_n(&luck_publication, state, __ATOMIC_RELEASE);
}

static unsigned roll_percent(void) {
    // SplitMix64, with a separate sequence from the game's drop lottery.
    uint64_t state = __atomic_load_n(&luck_random, __ATOMIC_RELAXED);
    if (!state) {
        uint64_t seed = monotonic_ms() | 1;
        __atomic_compare_exchange_n(&luck_random, &state, seed, false,
                                    __ATOMIC_RELAXED, __ATOMIC_RELAXED);
    }
    uint32_t sample;
    do {
        uint64_t value = __atomic_add_fetch(&luck_random, 0x9e3779b97f4a7c15ull, __ATOMIC_RELAXED);
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
        sample = (uint32_t)((value ^ (value >> 31)) >> 32);
    } while (sample >= 4294967200u);
    return sample % 100;
}

static const char *rupee_for_roll(unsigned roll) {
    return roll < 85 ? "Obj_Rupee_Green" : "Obj_Rupee_Blue";
}

static bool same_name(uintptr_t address, const char *name, size_t length) {
    if (!mapped(address, length, 1)) return false;
    const char *actual = (const char *)address;
    for (size_t i = 0; i < length; ++i)
        if (actual[i] != name[i]) return false;
    return true;
}

static uintptr_t first_table(uintptr_t source) {
    uintptr_t tables = pointer(source + 0x30);
    return *(const int *)(source + 0x28) > 0 && mapped(tables, sizeof(uintptr_t), 1)
        ? pointer(tables) : 0;
}

static bool terrain_table(uintptr_t table) {
    static const char normal_resource[] = "Drop/DropTable/DropTable_Foliage_00";
    if (!mapped(table, sizeof(uintptr_t), 1)) return false;
    uintptr_t handle = pointer(table);
    if (!mapped(handle, 0x44, 1)) return false;
    uintptr_t address = pointer(handle + 0x38);
    __atomic_store_n(&terrain_table_path, address, __ATOMIC_RELAXED);
    return *(const unsigned *)(handle + 0x40) >= sizeof normal_resource &&
           same_name(address, normal_resource, sizeof normal_resource);
}

static uintptr_t ordinary_terrain(uintptr_t source, const DropRequest *request) {
    if (request->actor || request->reason != 7 || request->special_source != 1 ||
        request->terrain_chance != 1)
        return 0;
    uintptr_t module = pointer(game(FOLIAGE_MODULE));
    if (!mapped(module, 0xf8, 1)) return 0;
    uintptr_t scene = pointer(module + 0xf0);
    if (!mapped(scene, 0x338, 1)) return 0;
    uintptr_t processor = pointer(scene + 0x330);
    if (!processor || source != processor + 0xb8) return 0;
    __atomic_add_fetch(&grass_drop_evidence.terrain_requests, 1, __ATOMIC_RELAXED);
    if (*(const int *)(source + 0x28) != 9) return 0;
    uintptr_t table = first_table(source);
    return terrain_table(table) ? table : 0;
}

static bool ordinary_grass(uintptr_t source, const DropList *result, uintptr_t request) {
    uintptr_t actor = pointer(request + 0x40);
    if (!actor || !mapped(actor, 0x28, 1)) return false;
    uintptr_t name = pointer(actor + 0x20);
    if (!same_name(name, "Obj_Plant_Grass_A_01", sizeof "Obj_Plant_Grass_A_01") &&
        !same_name(name, "Obj_Plant_Grass_A_01_Respawn", sizeof "Obj_Plant_Grass_A_01_Respawn"))
        return false;
    __atomic_add_fetch(&grass_drop_evidence.placed_requests, 1, __ATOMIC_RELAXED);
    unsigned reason = *(const uint32_t *)(request + 0x48);
    __atomic_store_n(&grass_drop_evidence.last_reason, reason, __ATOMIC_RELAXED);
    uintptr_t actor_override = pointer(source + 0x48);
    uintptr_t table_override = pointer(source + 0x98);
    bool default_table = result->table && result->table == first_table(source);
    bool default_actor = same_name(actor_override, "", 1) && same_name(table_override, "", 1);
    // The native death dispatch passes reason 7 for several destruction
    // methods. LifeDepleted / Normal is the path observed for sword cuts.
    bool normal_damage = *(const unsigned *)(source + 0xe8) == 10 &&
                         *(const unsigned *)(source + 0xec) == 1;
    __atomic_store_n(&grass_drop_evidence.last_match, (default_table ? 1u : 0u) |
                     (default_actor ? 2u : 0u) | (normal_damage ? 4u : 0u), __ATOMIC_RELAXED);
    return reason == 7 && default_table && default_actor && normal_damage;
}

static bool luck_current(uint64_t state) {
    return state == __atomic_load_n(&luck_publication, __ATOMIC_ACQUIRE) &&
           (state >> 1) == save_epoch();
}

static bool replace_drop(DropList *result, uint64_t state) {
    // Mirror the native explicit-actor entry in this spawn request's local pool.
    // Its consumer receives one result even if the ordinary table rolled none.
    if (result->capacity != 16 || result->entries != result->slots ||
        result->count < 0 || result->count > result->capacity)
        return false;
    DropEntry *entry = result->count ? result->entries[0] : result->free_entry;
    uintptr_t offset = (uintptr_t)entry - (uintptr_t)result->pool;
    if (offset >= sizeof result->pool || offset % sizeof *entry) return false;
    unsigned roll = roll_percent();
    const char *name = rupee_for_roll(roll);
    unsigned hash = ((unsigned (*)(const char *, unsigned))game(GAME_drop_actor_hash))(name, 0);
    if (!luck_current(state)) return false;
    if (!result->count) {
        result->free_entry = (DropEntry *)entry->name;
        result->entries[0] = entry;
    }
    *entry = (DropEntry){.name = name, .hash = hash};
    result->count = 1;
    unsigned *color = roll < 85 ? &grass_drop_evidence.green : &grass_drop_evidence.blue;
    __atomic_add_fetch(color, 1, __ATOMIC_RELAXED);
    return true;
}

void grass_drop(uintptr_t source, void *output, uintptr_t request) {
    DropList *result = output;
    void (*resolve)(uintptr_t, DropList *, uintptr_t) = (void *)game(GAME_drop_list_resolve);
    uint64_t state = __atomic_load_n(&luck_publication, __ATOMIC_ACQUIRE);
    bool active = (state & 1) && (state >> 1) == save_epoch();
    uintptr_t terrain = active ? ordinary_terrain(source, (const DropRequest *)request) : 0;
    if (terrain) {
        // The native cut queue already admitted this record. Bypass only the
        // throttle and probability gate on the ordinary terrain's own request.
        DropRequest terrain_request = *(const DropRequest *)request;
        terrain_request.terrain_chance = 0;
        resolve(source, result, (uintptr_t)&terrain_request);
        if (result->table == terrain && luck_current(state) && replace_drop(result, state)) return;
        resolve(source, result, request);
        return;
    }
    resolve(source, result, request);
    if (active && luck_current(state) && ordinary_grass(source, result, request))
        replace_drop(result, state);
}
