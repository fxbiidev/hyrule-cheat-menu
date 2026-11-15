#include "guest.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base = GAME_BASE;
static bool child = true, ready = true;
static int quantities[6], capacities[6] = {50, 40, 50, 40, 30, 50};
static int bags[11];
static unsigned given;
static bool bombchu_used, archive_ready = true;
static unsigned epoch = 7;
static bool reset_in_count, reset_in_capacity, reset_in_guide, reset_in_age;
unsigned save_epoch(void) { return epoch; }
static const char *const names[] = {
    "Item_Arrow_Normal", "Item_Bomb", "Item_DekuSeed", "Item_DekuNut", "Item_DekuStick", "Item_Bombchu",
};
static const char *const bag_names[] = {
    "Weapon_Range_Bow", "Item_BombBag_Small", "Item_BombBag_Medium", "Item_BombBag_Large",
    "Weapon_Range_SlingShot", "Item_NutBag_Small", "Item_NutBag_Medium", "Item_NutBag_Large",
    "Item_StickBag_Small", "Item_StickBag_Medium", "Item_StickBag_Large",
};
static int index_of(const char *name) {
    for (unsigned i = 0; i < 6; ++i) if (!strcmp(name, names[i])) return (int)i;
    assert(!"Unsupported consumable was requested");
    return -1;
}
static int mock_count(uintptr_t pouch, const char **name, unsigned a, unsigned b) {
    assert(pouch == 1 && !a && !b);
    if (reset_in_count) { ++epoch; reset_in_count = false; }
    for (unsigned i = 0; i < 6; ++i) if (!strcmp(*name, names[i])) return quantities[i];
    for (unsigned i = 0; i < 11; ++i) if (!strcmp(*name, bag_names[i])) return bags[i];
    assert(!"An unverified owning tool was requested");
    return -1;
}
static int mock_capacity(uintptr_t pouch, const char **name, unsigned a, uintptr_t b) {
    assert(pouch == 1 && !a && !b);
    if (reset_in_capacity) { ++epoch; reset_in_capacity = false; }
    return capacities[index_of(*name)];
}
static unsigned mock_give(uintptr_t pouch, const char **name, unsigned a, int delta,
                          unsigned b, unsigned c, unsigned d, unsigned e, unsigned f) {
    assert(pouch == 1 && !a && !b && !c && !d && !e && !f);
    int i = index_of(*name);
    assert(delta > 0 && quantities[i] + delta == capacities[i]);
    quantities[i] += delta;
    ++given;
    return 1;
}
static unsigned mock_child(uintptr_t actor) {
    assert(actor == 2);
    if (reset_in_age) { ++epoch; reset_in_age = false; }
    return child;
}
static uintptr_t mock_singleton(uintptr_t address) {
    if (address == 0x84c9e0d0) return 1;
    assert(address == 0x84c9e2a8); return ready;
}
static uintptr_t mock_pointer(uintptr_t address) {
    assert(address == 0x84eb6888 || address == 0x84ea42a8);
    return address == 0x84ea42a8 ? archive_ready : ready;
}
static unsigned mock_guide(uintptr_t unused, unsigned type) {
    assert(!unused && type == 36);
    if (reset_in_guide) { ++epoch; reset_in_guide = false; }
    return bombchu_used;
}
#define singleton mock_singleton
#define pointer mock_pointer
#include "../src/items.c"

static bool safe_world = true;
static uintptr_t unreadable;
static unsigned consumed, forwarded[4];
static uintptr_t forwarded_pouch;
static const char **forwarded_name;
bool world_safe(void) { return safe_world; }
void debug_log(const char *text) { assert(strstr(text, "ammunition")); }
static bool mock_mapped(uintptr_t address, size_t size, unsigned permissions) {
    assert(permissions == 1 && (size == sizeof(char *) || size == 32));
    return address && address != unreadable;
}
static void mock_consume(uintptr_t pouch, const char **name, unsigned amount,
                         unsigned alternate, unsigned flag4, unsigned flag5) {
    ++consumed; forwarded_pouch = pouch; forwarded_name = name;
    forwarded[0] = amount; forwarded[1] = alternate; forwarded[2] = flag4; forwarded[3] = flag5;
}
#define mapped mock_mapped
#include "../src/ammo_guard.c"

static void guard_tests(void) {
    const char *name = names[1];
    ammo_set_owner(0, epoch);
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == 1);
    ammo_set_owner(2, epoch);
    unsigned before = consumed;
    for (unsigned age = 0; age < 2; ++age) {
        child = age;
        for (unsigned i = 0; i < 6; ++i) {
            name = names[i];
            bool eligible = child ? i != 0 : i != 2 && i != 4;
            unsigned expected = consumed + !eligible;
            ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == expected);
        }
    }
    assert(consumed == before + 3); // The other nine age/item combinations were protected.
    before = consumed;
    for (unsigned i = 0; i < 6; ++i) {
        name = names[i]; unsigned expected = consumed + (i != 3);
        ammo_nut_use(1, &name, 1, 0, 0, 0); assert(consumed == expected);
    }
    assert(consumed == before + 5); // Shared events protect only the single-nut path.
    name = names[3]; before = consumed;
    ammo_nut_use(1, &name, 2, 0, 0, 0); assert(consumed == ++before);
    ammo_nut_use(1, &name, 1, 1, 0, 0); assert(consumed == ++before);
    safe_world = false;
    ammo_nut_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    safe_world = true;
    const char *excluded[] = {"Item_Bean", "Item_MagicBean", "Item_Bottle", "Item_SmallKey",
                              "Item_DekuStickExtra", "Item_Token_Gerudo", "", "Item_BombBag_Small"};
    for (unsigned i = 0; i < sizeof excluded / sizeof *excluded; ++i) {
        name = excluded[i]; before = consumed;
        ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == before + 1);
    }
    name = names[1];
    const unsigned cases[][4] = {{0,0,0,0}, {2,0,0,0}, {~0u,0,0,0}, {1,1,0,0},
                                 {1,2,0,0}, {1,0,1,0}, {1,0,0,1}, {9,3,4,5}};
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; ++i) {
        before = consumed;
        ammo_use(1, &name, cases[i][0], cases[i][1], cases[i][2], cases[i][3]);
        assert(consumed == before + 1 && forwarded_pouch == 1 && forwarded_name == &name);
        assert(!memcmp(forwarded, cases[i], sizeof forwarded)); // Preserve the complete native ABI.
    }
    before = consumed;
    ammo_use(9, &name, 1, 0, 0, 0); assert(consumed == ++before && forwarded_pouch == 9);
    safe_world = false;
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    safe_world = true; unreadable = (uintptr_t)&name;
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    unreadable = (uintptr_t)name;
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    unreadable = 0; name = 0;
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    char unterminated[32]; memset(unterminated, 'x', sizeof unterminated); name = unterminated;
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    name = names[1]; ammo_set_owner(0, epoch);
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    // A save can begin after the scoped owner was read but inside an age getter.
    ammo_set_owner(2, epoch); child = true; reset_in_age = true;
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    // The old publication stays revoked until an identified frame republishes it.
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    ammo_set_owner(2, epoch);
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == before);
    ammo_set_owner(2, epoch - 1);
    ammo_use(1, &name, 1, 0, 0, 0); assert(consumed == ++before);
    ammo_set_owner(0, epoch);
    puts("Production consumption guard: both ages, six items, exclusions, flags, quantities, pointers, transitions and OFF/reset passed.");
}

int main(void) {
    unsigned observed[2] = {0};
    items_update(1, 2, observed, true, epoch);
    assert(!given && !observed[0] && !observed[1]); // Positive capacity alone never unlocks.
    bags[0] = bags[3] = bags[4] = bags[5] = bags[10] = 1;
    items_update(1, 2, observed, true, epoch);
    assert(!quantities[0] && quantities[1] == 40 && quantities[2] == 50);
    assert(quantities[3] == 40 && quantities[4] == 30 && !quantities[5]);
    // Adult inventory does not receive child sticks or slingshot seeds.
    child = false; quantities[2] = quantities[4] = 0;
    items_update(1, 2, observed, true, epoch);
    assert(quantities[0] == 50 && !quantities[2] && !quantities[4]);
    // Capacity changes are read live; no fixed ammunition maximum is assumed.
    capacities[3] = 50; items_update(1, 2, observed, true, epoch); assert(quantities[3] == 50);
    // OFF polling only observes existing Bombchus; it never grants anything.
    unsigned before_off = given;
    quantities[5] = 1;
    items_update(1, 2, observed, false, epoch);
    assert(given == before_off && quantities[5] == 1);
    quantities[5] = 0;
    items_update(1, 2, observed, false, epoch);
    assert(given == before_off && !quantities[5]);
    items_update(1, 2, observed, true, epoch);
    assert(quantities[5] == 50);
    // An acquired Bombchu stack can refill even after later reaching zero.
    quantities[5] = 1; items_update(1, 2, observed, true, epoch); assert(quantities[5] == 50);
    quantities[5] = 0; items_update(1, 2, observed, true, epoch); assert(quantities[5] == 50);
    // After a real save reset, a Bomb Bag still does not prove Bombchu ownership.
    observed[0] = observed[1] = 0; quantities[5] = 0;
    items_update(1, 2, observed, true, epoch); assert(!quantities[5]);
    // A saved first-use flag proves depleted Bombchu ownership after a reload.
    bombchu_used = true; archive_ready = false;
    items_update(1, 2, observed, true, epoch); assert(!quantities[5]);
    archive_ready = true;
    items_update(1, 2, observed, true, epoch); assert(quantities[5] == 50);
    // A different save with neither evidence starts empty and stays empty.
    bombchu_used = false; observed[0] = observed[1] = 0; quantities[5] = 0;
    items_update(1, 2, observed, true, epoch); assert(!quantities[5]);
    // Persisted owning bags recover empty nuts/sticks without session observations.
    child = true; quantities[3] = quantities[4] = 0;
    observed[0] = observed[1] = 0;
    items_update(1, 2, observed, true, epoch); assert(quantities[3] == 50 && quantities[4] == 30);
    unsigned before = given;
    capacities[3] = 1000; quantities[3] = 0; quantities[4] = -1;
    items_update(1, 2, observed, true, epoch); assert(given == before);
    ready = false; capacities[3] = 50;
    items_update(1, 2, observed, true, epoch); assert(given == before);
    items_update(0, 2, observed, true, epoch); assert(given == before);
    ready = true; child = true;
    memset(quantities, 0, sizeof quantities); memset(bags, 0, sizeof bags);
    observed[0] = observed[1] = 0; bags[3] = 1;
    // A native getter crossing a reload must not refill the stale inventory.
    reset_in_capacity = true;
    items_update(1, 2, observed, true, epoch); assert(given == before);
    assert(!quantities[1]);
    // OFF observation must not publish old-save Bombchu evidence, either.
    observed[0] = observed[1] = 0; quantities[5] = 1; reset_in_count = true;
    items_update(1, 2, observed, false, epoch); assert(!observed[0] && !observed[1]);
    // A saved-use getter can cross the same boundary before reporting ownership.
    quantities[5] = 0; bombchu_used = true; reset_in_guide = true;
    memset(bags, 0, sizeof bags);
    items_update(1, 2, observed, true, epoch);
    assert(given == before && !observed[0] && !observed[1]);
    // A stale caller after the hook must not authenticate the new save, either.
    items_update(1, 2, observed, true, epoch - 1);
    assert(given == before && !observed[0] && !observed[1]);
    // Removing Bow/Slingshot revokes cached arrow/seed refill evidence. Neither
    // the old session bit nor orphan positive stock substitutes for the tool.
    bombchu_used = false;
    for (unsigned age = 0; age < 2; ++age) {
        child = age != 0;
        unsigned ammunition = child ? 2 : 0, bag = child ? 4 : 0;
        unsigned mask = 1u << ammunition;
        memset(quantities, 0, sizeof quantities); memset(bags, 0, sizeof bags);
        observed[0] = observed[1] = 0; bags[bag] = 1;
        items_update(1, 2, observed, true, epoch);
        assert(quantities[ammunition] == capacities[ammunition] && (observed[child] & mask));
        unsigned prior = given;
        quantities[ammunition] = 0; bags[bag] = 0;
        items_update(1, 2, observed, true, epoch);
        assert(!quantities[ammunition] && given == prior && !(observed[child] & mask));
        quantities[ammunition] = 1;
        items_update(1, 2, observed, true, epoch);
        assert(quantities[ammunition] == 1 && given == prior && !(observed[child] & mask));
        bags[bag] = 2; // Reject an invalid tool count as ownership evidence.
        items_update(1, 2, observed, true, epoch);
        assert(quantities[ammunition] == 1 && given == prior);
        bags[bag] = 1;
        items_update(1, 2, observed, true, epoch);
        assert(quantities[ammunition] == capacities[ammunition] && given == prior + 1);
    }
    guard_tests();
    puts("Production ammo policy: age, empty owned bags, capacity upgrades, invalid counts, readiness and never-unlock checks passed (native fixture).");
}
