#include "guest.h"
#include "profile.h"
#include "inventory.h"

enum { INVENTORY_ACK_MS = 2000 };
static struct {
    uintptr_t pouch, actor;
    unsigned epoch, index;
    int before, capacity, target;
    bool child;
    uint64_t started;
} pending;
static unsigned pending_active;
static unsigned uncertain_active, uncertain_epoch;
static enum InventoryResult uncertain_result;

static void clear_pending(void) {
    __atomic_store_n(&pending_active, 0, __ATOMIC_RELEASE);
}

static bool is_uncertain(unsigned epoch) {
    return __atomic_load_n(&uncertain_active, __ATOMIC_ACQUIRE) && uncertain_epoch == epoch;
}

static enum InventoryResult uncertain(enum InventoryResult result) {
    if (pending.epoch == save_epoch()) {
        uncertain_epoch = pending.epoch;
        uncertain_result = result;
        __atomic_store_n(&uncertain_active, 1, __ATOMIC_RELEASE);
    }
    clear_pending();
    return result;
}

bool inventory_busy(void) {
    return __atomic_load_n(&pending_active, __ATOMIC_ACQUIRE) != 0;
}

void inventory_reset(void) {
    // A save hook can revoke the request while a native getter is in flight.
    // Leave its immutable details intact; only the input thread writes them.
    if (inventory_busy() && pending.epoch == save_epoch()) uncertain(INV_NATIVE_FAILED);
    else clear_pending();
    if (__atomic_load_n(&uncertain_active, __ATOMIC_ACQUIRE) && uncertain_epoch != save_epoch())
        __atomic_store_n(&uncertain_active, 0, __ATOMIC_RELEASE);
}

static bool current(uintptr_t pouch, uintptr_t actor, unsigned epoch) {
    return epoch == save_epoch() && pouch && actor &&
        mapped(pouch, 0x3bc8, 1) && mapped(actor, 0x278, 1) &&
        pouch == singleton(0x84C9E0D0) && singleton(0x84C9E2A8) &&
        pointer(game(0x84EB6888)) && world_safe() && epoch == save_epoch();
}

static int item_count(uintptr_t pouch, const char *name) {
    return ((int (*)(uintptr_t, const char **, unsigned, unsigned))game(GAME_ammo_count))(
        pouch, &name, 0, 0);
}

static enum InventoryResult dependency(const InventorySnapshot *snapshot, unsigned index,
    enum InventoryAction action, int target) {
    const InventoryEntry *entry = &snapshot->entries[index];
    if ((index == INV_HOOKSHOT || index == INV_LONGSHOT) && target > entry->count) {
        unsigned peer = index == INV_HOOKSHOT ? INV_LONGSHOT : INV_HOOKSHOT;
        if (!snapshot->entries[peer].valid) return INV_UNAVAILABLE;
        if (snapshot->entries[peer].count) return INV_SLOT_CONFLICT;
    }
    if ((index == INV_BOW || index == INV_SLINGSHOT) && !target && entry->count) {
        unsigned ammo = index == INV_BOW ? INV_ARROWS : INV_DEKU_SEEDS;
        if (!snapshot->entries[ammo].valid) return INV_UNAVAILABLE;
        if (snapshot->entries[ammo].count) return INV_REMOVE_AMMO_FIRST;
    }
    if ((index == INV_ARROWS || index == INV_DEKU_SEEDS) &&
        (target > entry->count || (action == INVENTORY_REFILL && !entry->count))) {
        unsigned tool = index == INV_ARROWS ? INV_BOW : INV_SLINGSHOT;
        if (!snapshot->entries[tool].valid) return INV_UNAVAILABLE;
        if (!snapshot->entries[tool].count) return INV_ADD_TOOL_FIRST;
    }
    return INV_OK;
}

enum InventoryResult inventory_poll(uintptr_t pouch, uintptr_t actor, unsigned epoch) {
    if (!inventory_busy()) return INV_UNCHANGED;
    if (pending.epoch != save_epoch()) {
        clear_pending();
        return INV_STALE;
    }
    // A stale caller cannot release the hold for a current pending operation.
    if (pending.epoch != epoch) return INV_STALE;
    if (pending.pouch != pouch || pending.actor != actor) return uncertain(INV_NATIVE_FAILED);
    if (!current(pouch, actor, epoch)) {
        if (epoch == save_epoch()) return uncertain(INV_NATIVE_FAILED);
        clear_pending();
        return INV_STALE;
    }
    bool child = ((unsigned (*)(uintptr_t))game(GAME_child_link))(actor) != 0;
    if (!inventory_busy() || epoch != save_epoch()) {
        clear_pending();
        return INV_STALE;
    }
    if (child != pending.child) return uncertain(INV_NATIVE_FAILED);
    const char *name = inventory_catalog[pending.index].native_id;
    int count = item_count(pouch, name);
    if (!inventory_busy() || epoch != save_epoch()) {
        clear_pending();
        return INV_STALE;
    }
    int capacity = 1;
    if (inventory_catalog[pending.index].quantity_editable)
        capacity = ((int (*)(uintptr_t, const char **, unsigned, uintptr_t))
            game(GAME_ammo_capacity))(pouch, &name, 0, 0);
    if (!inventory_busy() || epoch != save_epoch()) {
        clear_pending();
        return INV_STALE;
    }
    if (capacity != pending.capacity || count < 0 || count > capacity) {
        return uncertain(INV_NATIVE_FAILED);
    }
    if (!current(pouch, actor, epoch) || !inventory_busy()) {
        if (epoch == save_epoch()) return uncertain(INV_NATIVE_FAILED);
        clear_pending();
        return INV_STALE;
    }
    if (count == pending.target) {
        // Native consumption also clears equipment references and schedules the
        // engine's normal fallback. Keep Save/travel held until that is visible.
        if (!pending.target && !inventory_catalog[pending.index].quantity_editable) {
            bool equipped = ((unsigned (*)(uintptr_t, const char **))
                game(GAME_inventory_equipped))(pouch, &name) & 1;
            if (epoch != save_epoch() || !inventory_busy()) {
                clear_pending();
                return INV_STALE;
            }
            if (!current(pouch, actor, epoch)) {
                if (epoch == save_epoch()) return uncertain(INV_NATIVE_FAILED);
                clear_pending();
                return INV_STALE;
            }
            if (equipped) {
                if (monotonic_ms() - pending.started >= INVENTORY_ACK_MS) return uncertain(INV_TIMEOUT);
                return INV_PENDING;
            }
        }
        clear_pending();
        return INV_OK;
    }
    if (count != pending.before) {
        return uncertain(INV_NATIVE_FAILED);
    }
    if (monotonic_ms() - pending.started >= INVENTORY_ACK_MS) {
        return uncertain(INV_TIMEOUT);
    }
    return INV_PENDING;
}

bool inventory_read(uintptr_t pouch, uintptr_t actor, unsigned epoch, InventorySnapshot *snapshot) {
    if (!snapshot) return false;
    *snapshot = (InventorySnapshot){0};
    if (!current(pouch, actor, epoch)) return false;
    InventorySnapshot next = {.epoch = epoch, .pouch = pouch, .actor = actor};
    next.child = ((unsigned (*)(uintptr_t))game(GAME_child_link))(actor) != 0;
    if (epoch != save_epoch()) return false;
    unsigned age = next.child ? INV_CHILD : INV_ADULT;
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i) {
        const InventoryItem *item = &inventory_catalog[i];
        InventoryEntry *entry = &next.entries[i];
        entry->count = item_count(pouch, item->native_id);
        if (epoch != save_epoch()) return false;
        entry->capacity = 1;
        if (item->quantity_editable) {
            const char *name = item->native_id;
            entry->capacity = ((int (*)(uintptr_t, const char **, unsigned, uintptr_t))
                game(GAME_ammo_capacity))(pouch, &name, 0, 0);
            if (epoch != save_epoch()) return false;
        }
        entry->valid = entry->count >= 0 && entry->capacity >= 0 && entry->capacity <= 999 &&
            entry->count <= entry->capacity;
        entry->age_allowed = (item->ages & age) != 0;
        entry->owned = entry->valid && entry->count > 0;
        entry->can_set = !is_uncertain(epoch) && entry->valid &&
            entry->age_allowed && entry->capacity > 0;
        entry->can_refill = entry->can_set && entry->count < entry->capacity;
        entry->can_remove = entry->can_set && entry->count > 0;
    }
    if (!current(pouch, actor, epoch)) return false;
    next.valid = true;
    *snapshot = next;
    return true;
}

enum InventoryResult inventory_apply(uintptr_t pouch, uintptr_t actor, unsigned epoch,
    const InventorySnapshot *snapshot, unsigned index, enum InventoryAction action,
    int quantity, bool confirmed, bool infinite_items) {
    if (inventory_busy()) return INV_BUSY;
    if (epoch == save_epoch() && is_uncertain(epoch)) return uncertain_result;
    if (!snapshot || !snapshot->valid || index >= INVENTORY_COUNT ||
        (unsigned)action > INVENTORY_REMOVE) return INV_UNAVAILABLE;
    if (snapshot->epoch != epoch || epoch != save_epoch() || snapshot->pouch != pouch ||
        snapshot->actor != actor) return INV_STALE;
    if (!confirmed) return INV_CONFIRM_REQUIRED;
    if (infinite_items) return INV_INFINITE_ITEMS;
    InventorySnapshot live = *snapshot;
    if (!inventory_read(pouch, actor, epoch, &live))
        return epoch == save_epoch() ? INV_UNAVAILABLE : INV_STALE;
    const InventoryEntry *entry = &live.entries[index], *expected = &snapshot->entries[index];
    if (live.child != snapshot->child || !expected->valid || entry->count != expected->count ||
        entry->capacity != expected->capacity) return INV_STALE;
    if (!entry->valid) return INV_UNAVAILABLE;
    if (!entry->age_allowed) return INV_AGE_RESTRICTED;
    int target = action == INVENTORY_REMOVE ? 0 : action == INVENTORY_REFILL ? entry->capacity : quantity;
    enum InventoryResult required = dependency(&live, index, action, target);
    if (required != INV_OK) return required;
    if (!entry->can_set) return INV_CAPACITY;
    if (target < 0 || target > entry->capacity) return INV_CAPACITY;
    if (target == entry->count) return INV_UNCHANGED;
    if (!current(pouch, actor, epoch)) return epoch == save_epoch() ? INV_UNAVAILABLE : INV_STALE;
    pending.pouch = pouch;
    pending.actor = actor;
    pending.epoch = epoch;
    pending.index = index;
    pending.before = entry->count;
    pending.capacity = entry->capacity;
    pending.target = target;
    pending.child = live.child;
    pending.started = monotonic_ms();
    __atomic_store_n(&pending_active, 1, __ATOMIC_RELEASE);
    // Publish the save/travel hold before entering the native writer. A reset
    // crossing publication must revoke it before any mutation is dispatched.
    if (!current(pouch, actor, epoch) || !inventory_busy()) {
        clear_pending();
        return epoch == save_epoch() ? INV_UNAVAILABLE : INV_STALE;
    }
    const char *name = inventory_catalog[index].native_id;
    if (target > entry->count) {
        typedef unsigned (*Give)(uintptr_t, const char **, unsigned, int, unsigned,
                                  unsigned, unsigned, unsigned, unsigned);
        ((Give)game(GAME_give_ammo))(pouch, &name, 0, target - entry->count, 0, 0, 0, 0, 0);
    } else {
        ((void (*)(uintptr_t, const char **, unsigned, unsigned, unsigned, unsigned))
            game(GAME_consume_ammo))(pouch, &name, (unsigned)(entry->count - target), 0, 0, 0);
    }
    if (epoch != save_epoch() || !inventory_busy()) {
        clear_pending();
        return INV_STALE;
    }
    // The game can queue a valid update until a later frame. An unchanged
    // immediate count is pending acknowledgement, not evidence of failure.
    return inventory_poll(pouch, actor, epoch);
}

const char *inventory_result_text(enum InventoryResult result) {
    static const char *const text[] = {
        "Inventory change verified.", "Quantity is already correct.", "Inventory is unavailable.",
        "Inventory changed. Review and confirm again.", "No verified safe operation for this item.",
        "Unavailable for Link's current age.", "This item has no stock.",
        "Quantity exceeds the unlocked capacity.", "Confirm this inventory change first.",
        "Turn Infinite Items OFF before editing.", "Inventory uncertain. Reload a save before further edits.",
        "Applying native inventory change...", "Update timed out. Reload a save before further edits.",
        "Wait for the pending inventory change.",
        "Remove the other Hookshot or Longshot first; they share an inventory slot.",
        "Remove this tool's ammunition in Item Manager first.",
        "Add the matching Bow or Slingshot in Item Manager first."
    };
    return (unsigned)result < sizeof text / sizeof *text ? text[result] : text[INV_UNAVAILABLE];
}
