// Production combat policy at the verified accepted-damage bridge. No native gameplay claim.
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "combat.h"
#include "guest.h"

uintptr_t main_base = 0x10000000;
typedef struct {
    _Alignas(16) unsigned char link[0x500], target[0x500], other[0x500];
    _Alignas(16) unsigned char components[0x300], life[0x1c20];
    _Alignas(16) unsigned char handle[0x60], request[0xe8], current[16], maximum[16];
    char name[64];
} Fixture;
static Fixture f;
static unsigned generation = 11, world_calls, invalidate_on_world, revoke_on_map;
static bool safe_world = true;

static void ptr(void *base, unsigned offset, uintptr_t value) {
    memcpy((unsigned char *)base + offset, &value, sizeof value);
}
static void integer(void *base, unsigned offset, int value) {
    memcpy((unsigned char *)base + offset, &value, sizeof value);
}
bool mapped(uintptr_t address, size_t length, unsigned permissions) {
    if (revoke_on_map && address == (uintptr_t)f.handle) {
        revoke_on_map = 0;
        combat_publish(0, generation, false, 0, false);
    }
    uintptr_t first = (uintptr_t)&f, end = first + sizeof f;
    return permissions == 1 && address >= first && address <= end && length <= end - address;
}
unsigned save_epoch(void) { return generation; }
bool world_safe(void) {
    ++world_calls;
    if (invalidate_on_world && world_calls == invalidate_on_world) ++generation;
    return safe_world;
}

#include "../src/combat.c"

static void fixture(void) {
    memset(&f, 0, sizeof f);
    generation = 11;
    world_calls = invalidate_on_world = revoke_on_map = 0;
    safe_world = true;
    strcpy(f.name, "Enemy_Tektite_Red");
    ptr(f.link, 0, game(0x84B45E18));
    ptr(f.target, 0, game(0x84B45E18));
    ptr(f.other, 0, game(0x84B45E18));
    ptr(f.target, 0x20, (uintptr_t)f.name);
    ptr(f.target, 0x270, (uintptr_t)f.components);
    ptr(f.components, 0x2b8, (uintptr_t)f.life);
    ptr(f.life, 0x18, (uintptr_t)f.target);
    ptr(f.life, 0x218, (uintptr_t)f.target);
    ptr(f.life, 0x1c10, (uintptr_t)f.current);
    ptr(f.life, 0x1c18, (uintptr_t)f.maximum);
    integer(f.current, 8, 40);
    integer(f.maximum, 8, 40);
    integer(f.request, 0x14, 2);
    ptr(f.request, 0x28, (uintptr_t)f.handle);
    integer(f.request, 0x30, 72);
    ptr(f.handle, 0x40, (uintptr_t)f.link);
    integer(f.handle, 0x48, 72);
    combat_publish((uintptr_t)f.link, generation, false, 2, true);
}
static int damage(int amount) {
    Fixture before = f;
    int result = combat_damage((uintptr_t)f.life + 0x210, (uintptr_t)f.request, amount);
    // The policy only returns an operand. Native request, health and actor bytes stay exact.
    assert(!memcmp(&before, &f, sizeof f));
    return result;
}
static void rejected(void) {
    static unsigned check;
    ++check;
    int actual = damage(3);
    if (actual != 3) fprintf(stderr, "Rejection %u returned %d, reason %u\n", check, actual, combat_evidence.reason);
    assert(actual == 3);
}

int main(void) {
    fixture();
    const int expected[][5] = {{1,2,3,5,10}, {2,3,5,8,15}, {2,4,6,10,20}, {3,6,9,15,30}};
    const int amounts[] = {1,2,3,5,10};
    for (unsigned setting = 0; setting < 4; ++setting) {
        combat_publish((uintptr_t)f.link, generation, false, setting, true);
        for (unsigned i = 0; i < 5; ++i) assert(damage(amounts[i]) == expected[setting][i]);
    }
    combat_publish((uintptr_t)f.link, generation, true, 3, true);
    assert(damage(2) == 40); // OHK takes priority; not 40*3 or 2*3.
    assert(damage(60) == 60); // Existing larger native damage is not reduced.
    combat_publish((uintptr_t)f.link, generation, false, 3, true);
    assert(damage(2) == 6); // The selected multiplier resumes immediately.
    combat_publish((uintptr_t)f.link, generation, false, 0, true);
    assert(damage(2) == 2);
    combat_publish((uintptr_t)f.link, generation, false, 100, true);
    assert(damage(2) == 2);
    combat_publish((uintptr_t)f.link, generation, true, 100, true);
    assert(damage(2) == 40);
    combat_reset();
    assert(damage(2) == 2);

    const char *allowed[] = {"Enemy_Tektite_Red", "Enemy_Tektite_Blue", "Enemy_Keese",
        "Enemy_Keese_Fire", "Enemy_Keese_Ice", "Enemy_Stalchild", "Enemy_Stalchild_Big", "Enemy_Octorok"};
    for (unsigned i = 0; i < sizeof allowed / sizeof *allowed; ++i) {
        fixture(); strcpy(f.name, allowed[i]); assert(damage(2) == 4);
    }
    const char *excluded[] = {"Player", "Npc_Zelda", "Npc_Kokiri", "Animal_Chicken_A",
        "Enemy_DungeonBoss_Gohma", "Enemy_DungeonBoss_Ganondorf", "Enemy_MiddleBoss_DarkLink",
        "Enemy_Skulltula_Gold", "Enemy_Stalfos", "Enemy_Tektite_Red_Dictionary",
        "Enemy_Tektite_Red_Dictionary_Undefeated", "Enemy_Keese_Truth", "Enemy_Tektite_RedX", ""};
    for (unsigned i = 0; i < sizeof excluded / sizeof *excluded; ++i) {
        fixture(); strcpy(f.name, excluded[i]); rejected();
        combat_publish((uintptr_t)f.link, generation, true, 3, true); rejected();
    }
    for (unsigned type = 0; type < 20; ++type) {
        fixture(); integer(f.request, 0x14, (int)type);
        assert(damage(3) == (type == 2 ? 6 : 3));
    }
    fixture(); ptr(f.handle, 0x40, (uintptr_t)f.other); rejected();
    fixture(); ptr(f.handle, 0x40, (uintptr_t)f.target); rejected();
    fixture(); ptr(f.life, 0x218, (uintptr_t)f.link); rejected(); // Damage received never changes.
    fixture(); integer(f.handle, 0x48, 73); rejected();
    fixture(); integer(f.request, 0x30, -1); rejected();
    fixture(); ptr(f.request, 0x28, 1); rejected();
    fixture(); ptr(f.target, 0, 1); rejected();
    fixture(); ptr(f.link, 0, 1); rejected();
    fixture(); ptr(f.target, 0x270, 1); rejected();
    fixture(); ptr(f.components, 0x2b8, (uintptr_t)f.life + 1); rejected();
    fixture(); ptr(f.life, 0x18, (uintptr_t)f.other); rejected();
    fixture(); ptr(f.life, 0x1c10, 1); rejected();
    fixture(); ptr(f.life, 0x1c18, 1); rejected();
    for (unsigned type = 7; type <= 11; ++type) {
        fixture(); integer(f.target, 0x2c, (int)type); rejected();
    }
    for (unsigned flags = 1; flags < 4; ++flags) {
        fixture(); integer(f.life, 0x220, (int)flags); rejected();
    }
    fixture(); integer(f.current, 8, 0); rejected();
    fixture(); integer(f.current, 8, -1); rejected();
    fixture(); integer(f.current, 8, 41); rejected();
    fixture(); integer(f.maximum, 8, 1000001); rejected();
    fixture(); assert(damage(0) == 0 && damage(-3) == -3 && damage(INT_MIN) == INT_MIN);
    assert(damage(INT_MAX) == INT_MAX && damage(1000001) == 1000001);
    fixture(); integer(f.life, 0x228, INT_MAX - 4); rejected();
    fixture(); integer(f.life, 0x228, -1); rejected();
    fixture(); integer(f.life, 0x228, INT_MAX - 6); assert(damage(3) == 6);

    fixture(); f.request[0x1c] = 1; integer(f.request, 0x18, 5);
    assert(damage(3) == 5);
    combat_publish((uintptr_t)f.link, generation, true, 3, true);
    assert(damage(3) == 5);
    integer(f.request, 0x18, 2); rejected();
    integer(f.request, 0x18, -1); rejected();
    fixture(); safe_world = false; rejected();
    fixture(); ++generation; rejected();
    fixture(); invalidate_on_world = 2; rejected();
    fixture(); revoke_on_map = 1; rejected();
    fixture(); combat_publish((uintptr_t)f.link, generation, true, 3, false); rejected();
    fixture(); assert(combat_damage(1, (uintptr_t)f.request, 3) == 3);
    assert(combat_damage((uintptr_t)f.life + 0x210, 1, 3) == 3);
    // Each fresh accepted operand is scaled once. No queued-byte mutation causes stacking.
    fixture(); assert(damage(3) == 6 && damage(3) == 6 && damage(3) == 6);
    assert(combat_evidence.modified && combat_evidence.reason == COMBAT_APPLIED);
    // Retain checked player evidence when later enemy/environment packets arrive.
    fixture(); integer(f.request, 0x14, 6);
    unsigned previous_cause = combat_player_cause_counts[6];
    rejected();
    assert(combat_player_cause_counts[6] == previous_cause + 1);
    assert(combat_last_player.source == (uintptr_t)f.link && combat_last_player.target == (uintptr_t)f.target);
    assert(combat_last_player.damage_type == 6 && combat_last_player.reason == COMBAT_OTHER_CAUSE);
    ptr(f.handle, 0x40, (uintptr_t)f.other); rejected();
    assert(combat_last_player.source == (uintptr_t)f.link && combat_last_player.damage_type == 6);
    assert(combat_reason_counts[COMBAT_APPLIED] && combat_reason_counts[COMBAT_OTHER_CAUSE] &&
           combat_reason_counts[COMBAT_BAD_SOURCE]);
    puts("Combat: all factors/rounding, OHK priority, native caps, provenance, target exclusions, epochs and read-only policy passed.");
    return 0;
}
