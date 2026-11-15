#pragma once

#include <stdbool.h>
#include <stdint.h>

enum InventoryCategory { INV_WEAPONS, INV_SHIELDS, INV_EQUIPMENT, INV_CONSUMABLES, INV_AMMUNITION };
enum InventoryId {
    INV_KOKIRI_SWORD, INV_BOW, INV_SLINGSHOT, INV_BOOMERANG, INV_HOOKSHOT, INV_LONGSHOT, INV_HAMMER,
    INV_DEKU_SHIELD, INV_HYLIAN_SHIELD, INV_MIRROR_SHIELD,
    INV_GORON_TUNIC, INV_ZORA_TUNIC, INV_IRON_BOOTS, INV_HOVER_BOOTS,
    INV_DEKU_STICKS, INV_DEKU_NUTS, INV_ARROWS, INV_BOMBS, INV_DEKU_SEEDS, INV_BOMBCHUS,
    INV_MAGIC_BEANS,
    INVENTORY_COUNT
};
enum InventoryAge { INV_ADULT = 1, INV_CHILD = 2, INV_BOTH_AGES = 3 };
typedef struct {
    const char *name, *native_id;
    unsigned category, ages;
    bool quantity_editable;
} InventoryItem;
extern const InventoryItem inventory_catalog[INVENTORY_COUNT];

typedef struct {
    int count, capacity;
    bool valid, owned, age_allowed, can_set, can_refill, can_remove;
    // A first seed grant initializes the verified base pouch through native
    // acquisition. The displayed capacity is prospective until acknowledged.
    bool needs_seed_bag;
} InventoryEntry;
typedef struct {
    unsigned epoch;
    uintptr_t pouch, actor;
    bool valid, child;
    InventoryEntry entries[INVENTORY_COUNT];
} InventorySnapshot;

enum InventoryAction { INVENTORY_SET, INVENTORY_REFILL, INVENTORY_REMOVE };
enum InventoryResult {
    INV_OK, INV_UNCHANGED, INV_UNAVAILABLE, INV_STALE, INV_READ_ONLY, INV_AGE_RESTRICTED,
    INV_NOT_OWNED, INV_CAPACITY, INV_CONFIRM_REQUIRED, INV_INFINITE_ITEMS, INV_NATIVE_FAILED,
    INV_PENDING, INV_TIMEOUT, INV_BUSY,
    INV_SLOT_CONFLICT, INV_REMOVE_AMMO_FIRST, INV_ADD_TOOL_FIRST
};

// Read real native stock/ownership, including absent items. On failure the output
// is invalidated, never partly published. No normal-acquisition evidence is needed.
bool inventory_read(uintptr_t pouch, uintptr_t actor, unsigned epoch, InventorySnapshot *snapshot);
// All mutations require a separate confirmation and a still-current snapshot.
// Disable Infinite Items before editing so its refill cannot undo a confirmed edit.
enum InventoryResult inventory_apply(uintptr_t pouch, uintptr_t actor, unsigned epoch,
    const InventorySnapshot *snapshot, unsigned index, enum InventoryAction action,
    int quantity, bool confirmed, bool infinite_items);
// Native pouch mutations may commit on later frames. Poll the one pending
// operation; do not resubmit it. Timeout or ambiguous acknowledgement makes this
// save epoch read-only. Reset after a full save load; a same-epoch cancellation
// cannot clear uncertainty or authorize replay of an already queued mutation.
enum InventoryResult inventory_poll(uintptr_t pouch, uintptr_t actor, unsigned epoch);
bool inventory_busy(void);
void inventory_reset(void);
const char *inventory_result_text(enum InventoryResult result);
