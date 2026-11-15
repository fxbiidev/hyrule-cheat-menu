#include "combat.h"
#include "guest.h"
#include <limits.h>

enum { READ_PERMISSION = 1, MAX_ORDINARY_LIFE = 1000000 };
static unsigned publication, active_epoch, active_one_hit, active_damage;
static uintptr_t active_link;
CombatEvidence combat_evidence;
CombatEvidence combat_last_player;
unsigned combat_reason_counts[COMBAT_UNCHANGED + 1];
unsigned combat_player_cause_counts[32];

static const char *const ordinary_enemies[] = {
    "Enemy_Tektite_Red", "Enemy_Tektite_Blue",
    "Enemy_Keese", "Enemy_Keese_Fire", "Enemy_Keese_Ice",
    "Enemy_Stalchild", "Enemy_Stalchild_Big", "Enemy_Octorok"
};

void combat_publish(uintptr_t link_actor, unsigned epoch, bool one_hit,
                    unsigned damage_index, bool safe) {
    __atomic_add_fetch(&publication, 1, __ATOMIC_ACQ_REL);
    __atomic_store_n(&active_link, safe ? link_actor : 0, __ATOMIC_RELAXED);
    __atomic_store_n(&active_epoch, epoch, __ATOMIC_RELAXED);
    __atomic_store_n(&active_one_hit, safe && one_hit, __ATOMIC_RELAXED);
    __atomic_store_n(&active_damage, safe && damage_index < 4 ? damage_index : 0, __ATOMIC_RELAXED);
    __atomic_add_fetch(&publication, 1, __ATOMIC_RELEASE);
}

void combat_reset(void) {
    combat_publish(0, save_epoch(), false, 0, false);
}

static bool readable(uintptr_t address, size_t bytes) {
    return address && address <= UINTPTR_MAX - bytes && mapped(address, bytes, READ_PERMISSION);
}

static unsigned word(uintptr_t address) {
    return *(const unsigned *)address;
}

static bool actor(uintptr_t address) {
    if (!readable(address, 0x278) || pointer(address) != game(0x84B45E18)) return false;
    // The native weak-handle resolver excludes these engine actor states too.
    unsigned type = word(address + 0x2c);
    return type < 7 || type > 11;
}

static unsigned enemy_index(uintptr_t target) {
    uintptr_t name = pointer(target + 0x20);
    // Read each bounded candidate independently, including its terminating NUL.
    for (unsigned i = 0; i < sizeof ordinary_enemies / sizeof *ordinary_enemies; ++i) {
        const char *expected = ordinary_enemies[i];
        size_t length = 0;
        while (expected[length]) ++length;
        if (!readable(name, length + 1)) continue;
        const char *actual = (const char *)name;
        size_t j = 0;
        while (j <= length && actual[j] == expected[j]) ++j;
        if (j == length + 1) return i + 1;
    }
    return 0;
}

static uintptr_t attacker(uintptr_t request) {
    // Native 804A8B14-38: checked handle + generation -> actor, not a name guess.
    unsigned generation = word(request + 0x30);
    uintptr_t handle = pointer(request + 0x28);
    if (generation == UINT_MAX || !readable(handle, 0x4c) ||
        word(handle + 0x48) != generation) return 0;
    uintptr_t result = pointer(handle + 0x40);
    return actor(result) ? result : 0;
}

static int evidence(int original, int result, unsigned reason) {
    __atomic_store_n(&combat_evidence.reason, reason, __ATOMIC_RELAXED);
    __atomic_store_n(&combat_evidence.applied_amount, result, __ATOMIC_RELAXED);
    if (reason <= COMBAT_UNCHANGED)
        __atomic_add_fetch(&combat_reason_counts[reason], 1, __ATOMIC_RELAXED);
    if (result != original) __atomic_add_fetch(&combat_evidence.modified, 1, __ATOMIC_RELAXED);
    return result;
}

static int player_evidence(int original, int result, unsigned reason) {
    int value = evidence(original, result, reason);
    combat_last_player = combat_evidence;
    return value;
}

int combat_damage(uintptr_t state, uintptr_t request, int native_amount) {
    unsigned revision = __atomic_load_n(&publication, __ATOMIC_ACQUIRE);
    uintptr_t link = __atomic_load_n(&active_link, __ATOMIC_RELAXED);
    unsigned epoch = __atomic_load_n(&active_epoch, __ATOMIC_RELAXED);
    unsigned one_hit = __atomic_load_n(&active_one_hit, __ATOMIC_RELAXED);
    unsigned factor = __atomic_load_n(&active_damage, __ATOMIC_RELAXED);
    if ((revision & 1) || !link || (!one_hit && !factor)) return native_amount;
    __atomic_add_fetch(&combat_evidence.inspected, 1, __ATOMIC_RELAXED);
    combat_evidence.native_amount = native_amount;
    combat_evidence.request = request;
    combat_evidence.target = combat_evidence.source = 0;
    combat_evidence.damage_type = combat_evidence.target_index = 0;
    combat_evidence.current_life = combat_evidence.queued_before = 0;
    if (epoch != save_epoch() || !world_safe())
        return evidence(native_amount, native_amount, COMBAT_UNSAFE);
    if (native_amount <= 0 || native_amount > MAX_ORDINARY_LIFE)
        return evidence(native_amount, native_amount, COMBAT_BAD_AMOUNT);
    if (!readable(state, 0xc0) || !readable(request, 0xe8))
        return evidence(native_amount, native_amount, COMBAT_BAD_STATE);
    unsigned cause = word(request + 0x14);
    combat_evidence.damage_type = cause;
    uintptr_t target = pointer(state + 8);
    combat_evidence.target = target;
    if (target == link || !actor(target) || !actor(link))
        return evidence(native_amount, native_amount, COMBAT_UNSUPPORTED_TARGET);
    uintptr_t source = attacker(request);
    combat_evidence.source = source;
    if (source != link) return evidence(native_amount, native_amount, COMBAT_BAD_SOURCE);
    if (cause < 32) __atomic_add_fetch(&combat_player_cause_counts[cause], 1, __ATOMIC_RELAXED);
    unsigned supported = enemy_index(target);
    combat_evidence.target_index = supported;
    if (!supported) return player_evidence(native_amount, native_amount, COMBAT_UNSUPPORTED_TARGET);
    uintptr_t components = pointer(target + 0x270);
    if (!readable(components, 0x2c0))
        return player_evidence(native_amount, native_amount, COMBAT_BAD_STATE);
    uintptr_t life = pointer(components + 0x2b8);
    if (state < 0x210 || life != state - 0x210 || !readable(life, 0x1c20) ||
        pointer(life + 0x18) != target || (word(state + 0x10) & 3))
        return player_evidence(native_amount, native_amount, COMBAT_BAD_STATE);
    uintptr_t current = pointer(life + 0x1c10), maximum = pointer(life + 0x1c18);
    if (!readable(current, 12) || !readable(maximum, 12))
        return player_evidence(native_amount, native_amount, COMBAT_BAD_STATE);
    int health = *(const int *)(current + 8), capacity = *(const int *)(maximum + 8);
    combat_evidence.current_life = health;
    if (health <= 0 || capacity < health || capacity > MAX_ORDINARY_LIFE)
        return player_evidence(native_amount, native_amount, COMBAT_BAD_AMOUNT);
    // Actual Master Sword and Bow impacts both verified native contact channel 2
    // with Link's checked source handle. Every other channel stays stock.
    if (cause != 2) return player_evidence(native_amount, native_amount, COMBAT_OTHER_CAUSE);
    int64_t adjusted = native_amount;
    if (one_hit) {
        if (adjusted < health) adjusted = health;
    } else if (factor == 1) adjusted = ((int64_t)native_amount * 3 + 1) / 2;
    else if (factor == 2) adjusted = (int64_t)native_amount * 2;
    else if (factor == 3) adjusted = (int64_t)native_amount * 3;
    // Preserve the request's own native maximum (804A87AC-C8), including OHK.
    if (*(const uint8_t *)(request + 0x1c) == 1) {
        int cap = *(const int *)(request + 0x18);
        if (cap < native_amount) return player_evidence(native_amount, native_amount, COMBAT_NATIVE_CAP);
        if (adjusted > cap) adjusted = cap;
    }
    int queued = *(const int *)(state + 0x18);
    combat_evidence.queued_before = queued;
    if (queued < 0 || adjusted > INT_MAX || adjusted > INT_MAX - queued)
        return player_evidence(native_amount, native_amount, COMBAT_BAD_AMOUNT);
    if (epoch != save_epoch() || !world_safe() || epoch != save_epoch() ||
        revision != __atomic_load_n(&publication, __ATOMIC_ACQUIRE))
        return player_evidence(native_amount, native_amount, COMBAT_UNSAFE);
    return player_evidence(native_amount, (int)adjusted,
                    adjusted != native_amount ? COMBAT_APPLIED : COMBAT_UNCHANGED);
}
