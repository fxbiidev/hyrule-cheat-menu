#include "guest.h"
#include "inventory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base = GAME_BASE;
static unsigned epoch = 7, writes, count_calls;
static bool child = true, ready = true, safe = true, readable = true;
static bool reset_count, reset_capacity, reset_age, reset_write, fail_write, partial_write;
static bool reset_equipped, unsafe_equipped, defer_unequip;
static bool equipped[INVENTORY_COUNT];
static bool defer_write;
static bool defer_bag, bag_queued;
static unsigned bag_gives;
static int queued_index = -1, queued_quantity;
static uint64_t now;
static int amounts[INVENTORY_COUNT], capacities[INVENTORY_COUNT];
unsigned save_epoch(void) { return epoch; }
uint64_t monotonic_ms(void) { return now; }
bool world_safe(void) { return safe; }
bool mapped(uintptr_t address, size_t size, unsigned permissions) {
    assert(permissions == 1);
    return readable && ((address == 1 && size == 0x3bc8) || (address == 2 && size == 0x278));
}
static int index_of(const char *name) {
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i)
        if (!strcmp(name, inventory_catalog[i].native_id)) return (int)i;
    assert(!"Unverified inventory item was requested");
    return -1;
}
static int mock_count(uintptr_t pouch, const char **name, unsigned alt, unsigned flags) {
    assert(pouch == 1 && !alt && !flags);
    ++count_calls;
    if (reset_count) { ++epoch; reset_count = false; }
    return amounts[index_of(*name)];
}
static int mock_capacity(uintptr_t pouch, const char **name, unsigned alt, uintptr_t flags) {
    assert(pouch == 1 && !alt && !flags);
    assert(inventory_catalog[index_of(*name)].quantity_editable);
    if (reset_capacity) { ++epoch; reset_capacity = false; }
    return capacities[index_of(*name)];
}
static unsigned mock_child(uintptr_t actor) {
    assert(actor == 2);
    if (reset_age) { ++epoch; reset_age = false; }
    return child;
}
static unsigned mock_equipped(uintptr_t pouch, const char **name) {
    assert(pouch == 1);
    if (reset_equipped) { ++epoch; reset_equipped = false; }
    if (unsafe_equipped) { safe = false; unsafe_equipped = false; }
    return equipped[index_of(*name)];
}
static unsigned mock_give(uintptr_t pouch, const char **name, unsigned alt, int delta,
    unsigned a, unsigned b, unsigned c, unsigned d, unsigned e) {
    assert(pouch == 1 && !alt && !a && !b && !c && !d && !e);
    if (!strcmp(*name, "Item_BulletBag_Small")) {
        assert(inventory_busy() && delta == 1 && amounts[INV_SLINGSHOT] == 1);
        assert(!amounts[INV_DEKU_SEEDS] && !capacities[INV_DEKU_SEEDS]);
        ++writes; ++bag_gives;
        if (defer_bag) bag_queued = true;
        else if (!fail_write) {
            capacities[INV_DEKU_SEEDS] = 30;
            amounts[INV_DEKU_SEEDS] = partial_write ? 29 : 30;
        }
        if (reset_write) { ++epoch; reset_write = false; }
        return !fail_write;
    }
    int i = index_of(*name);
    assert(inventory_busy()); // Save/travel hold is published before native dispatch.
    assert(delta > 0 && amounts[i] + delta <= capacities[i]);
    ++writes;
    if (defer_write) { queued_index = i; queued_quantity = amounts[i] + delta; }
    else if (!fail_write) amounts[i] += partial_write ? delta - 1 : delta;
    if (reset_write) { ++epoch; reset_write = false; }
    return !fail_write;
}
static void mock_consume(uintptr_t pouch, const char **name, unsigned delta, unsigned alt,
    unsigned a, unsigned b) {
    assert(pouch == 1 && !alt && !a && !b);
    int i = index_of(*name);
    assert(inventory_busy());
    assert(delta > 0 && delta <= (unsigned)amounts[i]);
    ++writes;
    if (defer_write) { queued_index = i; queued_quantity = amounts[i] - (int)delta; }
    else if (!fail_write) amounts[i] -= (int)delta;
    if (!fail_write && !defer_write && !amounts[i] && !defer_unequip) equipped[i] = false;
    if (reset_write) { ++epoch; reset_write = false; }
}
static uintptr_t mock_singleton(uintptr_t address) {
    if (address == 0x84c9e0d0) return ready ? 1 : 0;
    assert(address == 0x84c9e2a8);
    return ready;
}
static uintptr_t mock_pointer(uintptr_t address) {
    assert(address == 0x84eb6888);
    return ready;
}
#define singleton mock_singleton
#define pointer mock_pointer
#include "../src/inventory.c"

static enum InventoryResult apply(InventorySnapshot *snapshot, unsigned index,
    enum InventoryAction action, int quantity) {
    return inventory_apply(1, 2, epoch, snapshot, index, action, quantity, true, false);
}

static void stack_edit_tests(void) {
    // These fixtures supply capacities to exercise the production policy. They
    // do not establish native item parameters, planting or save persistence.
    const struct { unsigned index; int capacity; } stacks[] = {
        {INV_DEKU_SEEDS, 30}, {INV_DEKU_SEEDS, 40}, {INV_DEKU_SEEDS, 50},
        {INV_MAGIC_BEANS, 10}
    };
    InventorySnapshot snapshot;
    child = true;
    for (unsigned s = 0; s < sizeof stacks / sizeof *stacks; ++s) {
        unsigned index = stacks[s].index;
        capacities[index] = stacks[s].capacity;
        amounts[INV_SLINGSHOT] = index == INV_DEKU_SEEDS;
        int original[INVENTORY_COUNT];
        memcpy(original, amounts, sizeof original);
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(!snapshot.entries[index].owned && snapshot.entries[index].can_set);
        unsigned before = writes;
        assert(inventory_apply(1, 2, epoch, &snapshot, index, INVENTORY_SET, 1, false, false) ==
            INV_CONFIRM_REQUIRED);
        assert(inventory_apply(1, 2, epoch, &snapshot, index, INVENTORY_SET, 1, true, true) ==
            INV_INFINITE_ITEMS);
        assert(apply(&snapshot, index, INVENTORY_SET, -1) == INV_CAPACITY);
        assert(apply(&snapshot, index, INVENTORY_SET, capacities[index] + 1) == INV_CAPACITY);
        assert(writes == before && !memcmp(original, amounts, sizeof original));
        assert(apply(&snapshot, index, INVENTORY_SET, 1) == INV_OK);
        assert(writes == ++before && amounts[index] == 1);
        assert(inventory_read(1, 2, epoch, &snapshot) && snapshot.entries[index].owned);
        assert(apply(&snapshot, index, INVENTORY_REFILL, 0) == INV_OK);
        assert(writes == ++before && amounts[index] == capacities[index]);
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, index, INVENTORY_REFILL, 0) == INV_UNCHANGED && writes == before);
        assert(apply(&snapshot, index, INVENTORY_SET, capacities[index] - 1) == INV_OK);
        assert(writes == ++before && amounts[index] == capacities[index] - 1);
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, index, INVENTORY_REMOVE, 0) == INV_OK);
        assert(writes == ++before && !amounts[index]);
        assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[index].owned);
        assert(apply(&snapshot, index, INVENTORY_SET, 0) == INV_UNCHANGED && writes == before);
        assert(!memcmp(original, amounts, sizeof original)); // Only the selected stack changed.
        child = false;
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, index, INVENTORY_SET, 1) == INV_AGE_RESTRICTED);
        assert(writes == before && !memcmp(original, amounts, sizeof original));
        child = true;
    }
    amounts[INV_SLINGSHOT] = 0;
    capacities[INV_DEKU_SEEDS] = 30;
}

static void invalid_stack_capacity_tests(void) {
    const struct { unsigned index; int capacity; } invalid[] = {
        {INV_MAGIC_BEANS, 999}, {INV_MAGIC_BEANS, 0},
        {INV_DEKU_SEEDS, 31}, {INV_DEKU_SEEDS, 999}
    };
    InventorySnapshot snapshot;
    child = true;
    amounts[INV_SLINGSHOT] = 1;
    for (unsigned i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        unsigned index = invalid[i].index;
        int valid_capacity = capacities[index];
        unsigned before = writes;
        capacities[index] = invalid[i].capacity;
        assert(inventory_read(1, 2, epoch, &snapshot));
        const InventoryEntry *entry = &snapshot.entries[index];
        assert(!entry->valid && !entry->can_set && !entry->can_refill && !entry->can_remove);
        assert(apply(&snapshot, index, INVENTORY_SET, 1) == INV_STALE);
        assert(apply(&snapshot, index, INVENTORY_REFILL, 0) == INV_STALE);
        assert(apply(&snapshot, index, INVENTORY_REMOVE, 0) == INV_STALE);
        assert(writes == before && !inventory_busy() && !amounts[index]);

        // The same validation must also refuse a changed native layout during
        // acknowledgement, without retrying the queued grant or allowing edits.
        capacities[index] = valid_capacity;
        assert(inventory_read(1, 2, epoch, &snapshot));
        defer_write = true;
        assert(apply(&snapshot, index, INVENTORY_SET, 1) == INV_PENDING);
        capacities[index] = invalid[i].capacity;
        assert(inventory_poll(1, 2, epoch) == INV_NATIVE_FAILED && !inventory_busy());
        assert(writes == before + 1 && !amounts[index]);
        capacities[index] = valid_capacity;
        assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[index].can_set);
        assert(apply(&snapshot, index, INVENTORY_SET, 1) == INV_NATIVE_FAILED);
        assert(writes == before + 1);
        defer_write = false; queued_index = -1;
        ++epoch; inventory_reset();
    }
    amounts[INV_SLINGSHOT] = 0;
}

static void first_seed_tests(void) {
    InventorySnapshot snapshot;
    child = true;
    amounts[INV_SLINGSHOT] = 1;
    capacities[INV_DEKU_SEEDS] = 0;
    unsigned before = writes, prior_bags = bag_gives;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(snapshot.entries[INV_DEKU_SEEDS].needs_seed_bag);
    assert(snapshot.entries[INV_DEKU_SEEDS].capacity == 30 && !capacities[INV_DEKU_SEEDS]);
    assert(inventory_apply(1, 2, epoch, &snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 1, false, false) ==
        INV_CONFIRM_REQUIRED);
    assert(inventory_apply(1, 2, epoch, &snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 1, true, true) ==
        INV_INFINITE_ITEMS);
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 31) == INV_CAPACITY);
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_REMOVE, 0) == INV_UNCHANGED);
    assert(writes == before && bag_gives == prior_bags && !capacities[INV_DEKU_SEEDS]);
    // The same displayed 30 can refer to an acquired or prospective pouch.
    // Confirmation must not silently change meaning between those states.
    capacities[INV_DEKU_SEEDS] = 30;
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 1) == INV_STALE);
    capacities[INV_DEKU_SEEDS] = 0;
    assert(writes == before);

    // Native bag acquisition fills to 30; a target of one must consume 29 once,
    // without releasing the save/travel hold between its two native operations.
    defer_bag = defer_write = true;
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 1) == INV_PENDING);
    assert(bag_queued && inventory_busy() && writes == before + 1 && bag_gives == prior_bags + 1);
    for (unsigned i = 0; i < 5; ++i) {
        assert(inventory_poll(1, 2, epoch) == INV_PENDING && writes == before + 1);
    }
    capacities[INV_DEKU_SEEDS] = 30;
    assert(inventory_poll(1, 2, epoch) == INV_PENDING && writes == before + 1);
    amounts[INV_DEKU_SEEDS] = 30; bag_queued = false;
    assert(inventory_poll(1, 2, epoch) == INV_PENDING && writes == before + 2);
    assert(queued_index == INV_DEKU_SEEDS && queued_quantity == 1 && inventory_busy());
    for (unsigned i = 0; i < 5; ++i) {
        assert(inventory_poll(1, 2, epoch) == INV_PENDING && writes == before + 2);
    }
    amounts[INV_DEKU_SEEDS] = queued_quantity; queued_index = -1;
    assert(inventory_poll(1, 2, epoch) == INV_OK && !inventory_busy());
    assert(amounts[INV_SLINGSHOT] == 1 && amounts[INV_DEKU_SEEDS] == 1);
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[INV_DEKU_SEEDS].needs_seed_bag);
    defer_bag = defer_write = false;
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_REMOVE, 0) == INV_OK);

    // A confirmed refill only acquires the native base bag; its real 30/30
    // acknowledgement needs no compensating seed write.
    capacities[INV_DEKU_SEEDS] = 0;
    assert(inventory_read(1, 2, epoch, &snapshot));
    before = writes;
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_REFILL, 0) == INV_OK);
    assert(writes == before + 1 && amounts[INV_DEKU_SEEDS] == 30 && capacities[INV_DEKU_SEEDS] == 30);
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_REMOVE, 0) == INV_OK);

    // Unexpected bag results never dispatch the second writer, and a reset in
    // the same save cannot authorize another native bag acquisition.
    for (unsigned fault = 0; fault < 5; ++fault) {
        capacities[INV_DEKU_SEEDS] = 0; amounts[INV_DEKU_SEEDS] = 0;
        assert(inventory_read(1, 2, epoch, &snapshot));
        defer_bag = true;
        before = writes;
        assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 1) == INV_PENDING);
        if (fault == 0) { capacities[INV_DEKU_SEEDS] = 30; amounts[INV_DEKU_SEEDS] = 29; }
        if (fault == 1) capacities[INV_DEKU_SEEDS] = 40;
        if (fault == 2) amounts[INV_SLINGSHOT] = 0;
        if (fault == 3) safe = false;
        if (fault == 4) now += 2000;
        enum InventoryResult expected = fault == 4 ? INV_TIMEOUT : INV_NATIVE_FAILED;
        assert(inventory_poll(1, 2, epoch) == expected && !inventory_busy());
        assert(writes == before + 1);
        safe = true; amounts[INV_SLINGSHOT] = 1;
        inventory_reset();
        assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 1) == expected && writes == before + 1);
        ++epoch; inventory_reset(); bag_queued = false; defer_bag = false;
    }
    // A save epoch change revokes the bag stage before its second writer.
    capacities[INV_DEKU_SEEDS] = 0; amounts[INV_DEKU_SEEDS] = 0;
    assert(inventory_read(1, 2, epoch, &snapshot));
    defer_bag = true; before = writes;
    assert(apply(&snapshot, INV_DEKU_SEEDS, INVENTORY_SET, 1) == INV_PENDING);
    ++epoch; inventory_reset();
    capacities[INV_DEKU_SEEDS] = 30; amounts[INV_DEKU_SEEDS] = 30;
    assert(inventory_poll(1, 2, epoch) == INV_UNCHANGED && writes == before + 1);
    defer_bag = bag_queued = false;
    amounts[INV_SLINGSHOT] = amounts[INV_DEKU_SEEDS] = 0;
}

int main(void) {
    InventorySnapshot snapshot = {0};
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i) capacities[i] = i < INV_DEKU_STICKS ? 1 : 30;
    capacities[INV_MAGIC_BEANS] = 10;
    stack_edit_tests();
    invalid_stack_capacity_tests();
    first_seed_tests();
    assert(inventory_read(1, 2, epoch, &snapshot) && snapshot.valid && snapshot.child);
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i) {
        assert(!snapshot.entries[i].owned);
        assert(snapshot.entries[i].can_set == snapshot.entries[i].age_allowed);
    }
    // Every allowed ID can be granted from zero without acquisition evidence,
    // then removed. Arrows/seeds retain their corresponding native tool.
    for (unsigned age = 0; age < 2; ++age) {
        child = age == 0;
        for (unsigned i = 0; i < INVENTORY_COUNT; ++i) {
            assert(inventory_read(1, 2, epoch, &snapshot));
            if (!snapshot.entries[i].age_allowed) {
                assert(apply(&snapshot, i, INVENTORY_SET, 1) == INV_AGE_RESTRICTED);
                continue;
            }
            if (i == INV_ARROWS) amounts[INV_BOW] = 1;
            if (i == INV_DEKU_SEEDS) amounts[INV_SLINGSHOT] = 1;
            assert(inventory_read(1, 2, epoch, &snapshot));
            assert(apply(&snapshot, i, INVENTORY_SET, 1) == INV_OK && amounts[i] == 1);
            assert(inventory_read(1, 2, epoch, &snapshot) && snapshot.entries[i].owned);
            equipped[i] = !inventory_catalog[i].quantity_editable;
            assert(apply(&snapshot, i, INVENTORY_REMOVE, 0) == INV_OK && !amounts[i] && !equipped[i]);
            assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[i].owned);
            if (i == INV_ARROWS) amounts[INV_BOW] = 0;
            if (i == INV_DEKU_SEEDS) amounts[INV_SLINGSHOT] = 0;
        }
    }
    // Shared-slot and tool/ammunition dependencies are checked on the live
    // snapshot, before any native dispatch. They need no gameplay acquisition.
    unsigned dependency_writes = writes;
    assert(inventory_read(1, 2, epoch, &snapshot));
    amounts[INV_LONGSHOT] = 1; // Other slot owner appeared after confirmation.
    assert(apply(&snapshot, INV_HOOKSHOT, INVENTORY_SET, 1) == INV_SLOT_CONFLICT);
    assert(writes == dependency_writes && !inventory_busy());
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_LONGSHOT, INVENTORY_REMOVE, 0) == INV_OK);
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_HOOKSHOT, INVENTORY_SET, 1) == INV_OK);
    assert(inventory_read(1, 2, epoch, &snapshot));
    dependency_writes = writes;
    assert(apply(&snapshot, INV_LONGSHOT, INVENTORY_REFILL, 0) == INV_SLOT_CONFLICT);
    assert(writes == dependency_writes);
    assert(apply(&snapshot, INV_HOOKSHOT, INVENTORY_REMOVE, 0) == INV_OK);
    amounts[INV_LONGSHOT] = -1;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_HOOKSHOT, INVENTORY_SET, 1) == INV_UNAVAILABLE);
    amounts[INV_LONGSHOT] = 0;
    for (unsigned age = 0; age < 2; ++age) {
        child = age != 0;
        unsigned tool = child ? INV_SLINGSHOT : INV_BOW;
        unsigned ammo = child ? INV_DEKU_SEEDS : INV_ARROWS;
        assert(inventory_read(1, 2, epoch, &snapshot));
        dependency_writes = writes;
        assert(apply(&snapshot, ammo, INVENTORY_SET, 1) == INV_ADD_TOOL_FIRST);
        assert(apply(&snapshot, ammo, INVENTORY_REFILL, 0) == INV_ADD_TOOL_FIRST);
        assert(writes == dependency_writes && !inventory_busy());
        capacities[ammo] = 0;
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, ammo, INVENTORY_SET, 1) == INV_ADD_TOOL_FIRST);
        capacities[ammo] = 30;
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, tool, INVENTORY_SET, 1) == INV_OK);
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, ammo, INVENTORY_REFILL, 0) == INV_OK);
        assert(inventory_read(1, 2, epoch, &snapshot));
        dependency_writes = writes;
        assert(apply(&snapshot, tool, INVENTORY_REMOVE, 0) == INV_REMOVE_AMMO_FIRST);
        assert(writes == dependency_writes && amounts[tool] == 1);
        assert(apply(&snapshot, ammo, INVENTORY_REMOVE, 0) == INV_OK);
        assert(inventory_read(1, 2, epoch, &snapshot));
        amounts[ammo] = 1; // Stock changed after the removal confirmation.
        assert(apply(&snapshot, tool, INVENTORY_REMOVE, 0) == INV_REMOVE_AMMO_FIRST);
        amounts[ammo] = -1;
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, tool, INVENTORY_REMOVE, 0) == INV_UNAVAILABLE);
        amounts[ammo] = 0;
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, tool, INVENTORY_REMOVE, 0) == INV_OK);
        assert(inventory_read(1, 2, epoch, &snapshot));
        amounts[ammo] = 3; // An existing orphan can still be emptied safely.
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, ammo, INVENTORY_SET, 4) == INV_ADD_TOOL_FIRST);
        assert(apply(&snapshot, ammo, INVENTORY_SET, 2) == INV_OK);
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, ammo, INVENTORY_REMOVE, 0) == INV_OK);
        amounts[tool] = 2;
        assert(inventory_read(1, 2, epoch, &snapshot));
        assert(apply(&snapshot, ammo, INVENTORY_SET, 1) == INV_UNAVAILABLE);
        amounts[tool] = 0;
    }
    assert(strstr(inventory_result_text(INV_SLOT_CONFLICT), "share an inventory slot"));
    assert(strstr(inventory_result_text(INV_REMOVE_AMMO_FIRST), "ammunition"));
    assert(strstr(inventory_result_text(INV_ADD_TOOL_FIRST), "Item Manager"));
    child = false;
    // A removed equipped tunic cannot be acknowledged while a native reference
    // still points to it, even if its count has already reached zero.
    amounts[INV_GORON_TUNIC] = 1; equipped[INV_GORON_TUNIC] = true; defer_unequip = true;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_GORON_TUNIC, INVENTORY_REMOVE, 0) == INV_PENDING);
    assert(inventory_busy() && !amounts[INV_GORON_TUNIC]);
    equipped[INV_GORON_TUNIC] = false;
    assert(inventory_poll(1, 2, epoch) == INV_OK && !inventory_busy());
    amounts[INV_GORON_TUNIC] = 1; equipped[INV_GORON_TUNIC] = true;
    assert(inventory_read(1, 2, epoch, &snapshot));
    reset_equipped = true;
    assert(apply(&snapshot, INV_GORON_TUNIC, INVENTORY_REMOVE, 0) == INV_STALE && !inventory_busy());
    equipped[INV_GORON_TUNIC] = false; defer_unequip = false;
    // A same-epoch scene/world invalidation inside the final equipment getter
    // cannot acknowledge success or authorize another edit without a reload.
    amounts[INV_GORON_TUNIC] = 1;
    assert(inventory_read(1, 2, epoch, &snapshot));
    unsafe_equipped = true;
    assert(apply(&snapshot, INV_GORON_TUNIC, INVENTORY_REMOVE, 0) == INV_NATIVE_FAILED);
    assert(!inventory_busy());
    safe = true;
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[INV_GORON_TUNIC].can_set);
    ++epoch; inventory_reset();
    child = true;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(!snapshot.entries[INV_KOKIRI_SWORD].owned && snapshot.entries[INV_KOKIRI_SWORD].can_set);
    assert(snapshot.entries[INV_BOMBS].can_refill && snapshot.entries[INV_DEKU_STICKS].can_set);
    assert(snapshot.entries[INV_DEKU_SEEDS].can_set && !snapshot.entries[INV_ARROWS].age_allowed);
    unsigned initial_writes = writes;
    assert(apply(&snapshot, INV_ARROWS, INVENTORY_SET, 1) == INV_AGE_RESTRICTED && writes == initial_writes);
    assert(inventory_apply(1, 2, epoch, &snapshot, INV_BOMBS, INVENTORY_SET, 1, false, false) == INV_CONFIRM_REQUIRED);
    assert(inventory_apply(1, 2, epoch, &snapshot, INV_BOMBS, INVENTORY_SET, 1, true, true) == INV_INFINITE_ITEMS);
    assert(writes == initial_writes);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_SET, 31) == INV_CAPACITY);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_SET, -1) == INV_CAPACITY && writes == initial_writes);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_SET, 7) == INV_OK && amounts[INV_BOMBS] == 7 && writes == initial_writes + 1);
    // Confirmation tied to an old count cannot stack or replay a previous edit.
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_SET, 7) == INV_STALE && writes == initial_writes + 1);
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_SET, 7) == INV_UNCHANGED && writes == initial_writes + 1);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_SET, 3) == INV_OK && amounts[INV_BOMBS] == 3);
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REMOVE, 999) == INV_OK && !amounts[INV_BOMBS]);
    assert(inventory_read(1, 2, epoch, &snapshot));
    capacities[INV_BOMBS] = 40;
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_STALE);
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_OK && amounts[INV_BOMBS] == 40);
    // First Bombchu acquisition works even after a fresh save epoch.
    amounts[INV_BOMBCHUS] = 1;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBCHUS, INVENTORY_REMOVE, 0) == INV_OK);
    assert(inventory_read(1, 2, epoch, &snapshot) && snapshot.entries[INV_BOMBCHUS].can_set);
    assert(apply(&snapshot, INV_BOMBCHUS, INVENTORY_SET, 2) == INV_OK);
    amounts[INV_BOMBCHUS] = 0; ++epoch;
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[INV_BOMBCHUS].owned);
    assert(inventory_read(1, 2, epoch, &snapshot) && snapshot.entries[INV_BOMBCHUS].can_refill);
    assert(apply(&snapshot, INV_BOMBCHUS, INVENTORY_REFILL, 0) == INV_OK);
    child = false;
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REMOVE, 0) == INV_STALE);
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.child);
    assert(!snapshot.entries[INV_DEKU_STICKS].can_set && !snapshot.entries[INV_DEKU_SEEDS].can_set);
    assert(snapshot.entries[INV_ARROWS].can_set);
    amounts[INV_BOW] = 1;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_ARROWS, INVENTORY_REFILL, 0) == INV_OK);
    unsigned before = writes;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(inventory_apply(1, 2, epoch - 1, &snapshot, INV_BOMBS, INVENTORY_REMOVE, 0, true, false) == INV_STALE);
    assert(inventory_apply(1, 2, epoch, &snapshot, INVENTORY_COUNT, INVENTORY_SET, 1, true, false) == INV_UNAVAILABLE);
    assert(apply(&snapshot, INV_BOMBS, (enum InventoryAction)-1, 0) == INV_UNAVAILABLE);
    amounts[INV_DEKU_NUTS] = -1;
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[INV_DEKU_NUTS].valid);
    amounts[INV_DEKU_NUTS] = 0; capacities[INV_DEKU_NUTS] = 1000;
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[INV_DEKU_NUTS].valid);
    assert(writes == before);
    ready = false; assert(!inventory_read(1, 2, epoch, &snapshot) && !snapshot.valid); ready = true;
    safe = false; assert(!inventory_read(1, 2, epoch, &snapshot) && !snapshot.valid); safe = true;
    readable = false; assert(!inventory_read(1, 2, epoch, &snapshot) && !snapshot.valid); readable = true;
    assert(!inventory_read(9, 2, epoch, &snapshot) && !snapshot.valid);
    reset_age = true; assert(!inventory_read(1, 2, epoch, &snapshot) && !snapshot.valid);
    reset_count = true; assert(!inventory_read(1, 2, epoch, &snapshot) && !snapshot.valid);
    reset_capacity = true; assert(!inventory_read(1, 2, epoch, &snapshot) && !snapshot.valid);
    assert(inventory_read(1, 2, epoch, &snapshot));
    fail_write = true;
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REMOVE, 0) == INV_PENDING && writes == before + 1);
    assert(inventory_busy());
    now += 1999;
    assert(inventory_poll(1, 2, epoch) == INV_PENDING);
    ++now;
    assert(inventory_poll(1, 2, epoch) == INV_TIMEOUT && !inventory_busy());
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REMOVE, 0) == INV_TIMEOUT && writes == before + 1);
    inventory_reset(); // Same-save reset cannot authorize a late write's replay.
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REMOVE, 0) == INV_TIMEOUT && writes == before + 1);
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[INV_BOMBS].can_set);
    ++epoch; inventory_reset();
    fail_write = false; partial_write = true;
    amounts[INV_BOMBS] = 0;
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_NATIVE_FAILED && writes == before + 2);
    assert(!inventory_busy() && amounts[INV_BOMBS] == capacities[INV_BOMBS] - 1);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_NATIVE_FAILED && writes == before + 2);
    partial_write = false;
    ++epoch; inventory_reset();
    assert(inventory_read(1, 2, epoch, &snapshot));
    reset_write = true;
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REMOVE, 0) == INV_STALE && writes == before + 3);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_STALE && writes == before + 3);
    assert(!inventory_busy());
    // Reproduce the native delay: the writer queues once, and twelve subsequent
    // polls still see the old count. Only acknowledgement can report success.
    assert(inventory_read(1, 2, epoch, &snapshot));
    amounts[INV_BOMBS] = 10;
    assert(inventory_read(1, 2, epoch, &snapshot));
    defer_write = true;
    before = writes;
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_SET, 7) == INV_PENDING);
    assert(inventory_busy() && amounts[INV_BOMBS] == 10 && writes == before + 1);
    assert(inventory_poll(1, 2, epoch - 1) == INV_STALE && inventory_busy());
    for (unsigned i = 0; i < 12; ++i) {
        now += 16;
        assert(inventory_poll(1, 2, epoch) == INV_PENDING && writes == before + 1);
    }
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_BUSY && writes == before + 1);
    assert(queued_index == INV_BOMBS && queued_quantity == 7);
    amounts[queued_index] = queued_quantity; queued_index = -1;
    assert(inventory_poll(1, 2, epoch) == INV_OK && !inventory_busy());
    assert(inventory_poll(1, 2, epoch) == INV_UNCHANGED && writes == before + 1);
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_PENDING && inventory_busy());
    amounts[queued_index] = queued_quantity; queued_index = -1;
    assert(inventory_poll(1, 2, epoch) == INV_OK && amounts[INV_BOMBS] == 40);
    // Unexpected intermediate quantities fail closed and remain read-only even
    // if the queued operation later reaches the requested target.
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REMOVE, 0) == INV_PENDING);
    amounts[INV_BOMBS] = 15;
    assert(inventory_poll(1, 2, epoch) == INV_NATIVE_FAILED && !inventory_busy());
    before = writes;
    amounts[INV_BOMBS] = 0; queued_index = -1;
    assert(inventory_read(1, 2, epoch, &snapshot) && !snapshot.entries[INV_BOMBS].can_set);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_NATIVE_FAILED && writes == before);
    // Save reload advances the epoch and revokes queued state/acquisition.
    ++epoch; inventory_reset();
    assert(inventory_read(1, 2, epoch, &snapshot) && snapshot.entries[INV_BOMBS].can_set);
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_PENDING);
    ++capacities[INV_BOMBS];
    assert(inventory_poll(1, 2, epoch) == INV_NATIVE_FAILED && !inventory_busy());
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_NATIVE_FAILED);
    --capacities[INV_BOMBS]; queued_index = -1;
    ++epoch; inventory_reset();
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_PENDING);
    child = !child;
    assert(inventory_poll(1, 2, epoch) == INV_NATIVE_FAILED && !inventory_busy());
    child = !child; queued_index = -1;
    ++epoch; inventory_reset();
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_PENDING);
    safe = false;
    assert(inventory_poll(1, 2, epoch) == INV_NATIVE_FAILED && !inventory_busy());
    safe = true; queued_index = -1;
    ++epoch; inventory_reset();
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_PENDING);
    before = count_calls;
    ++epoch; inventory_reset(); queued_index = -1;
    assert(!inventory_busy() && inventory_poll(1, 2, epoch) == INV_UNCHANGED && count_calls == before);
    assert(inventory_read(1, 2, epoch, &snapshot));
    assert(apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_PENDING);
    inventory_reset(); queued_index = -1;
    assert(!inventory_busy() && apply(&snapshot, INV_BOMBS, INVENTORY_REFILL, 0) == INV_NATIVE_FAILED);
    assert(strstr(inventory_result_text(INV_NATIVE_FAILED), "Reload a save"));
    assert(count_calls && !strcmp(inventory_result_text(INV_OK), "Inventory change verified."));
    puts("Inventory: all catalog IDs granted from zero/removed, seed/bean stack limits and write isolation, shared-slot/tool/ammo dependencies, equipped removal fence, age/capacity/confirmation checks, deferred acknowledgement and save-epoch isolation passed (portable fixture).");
}
