#include "guest.h"
#include "profile.h"
#include "items.h"

static int count(uintptr_t pouch, const char *name) {
    return ((int (*)(uintptr_t, const char **, unsigned, unsigned))game(GAME_ammo_count))(
        pouch, &name, 0, 0);
}

enum { ADULT = 1, CHILD = 2, BOTH_AGES = ADULT | CHILD };
static const struct { const char *name; unsigned ages; const char *tools[3]; } ammo[] = {
        {"Item_Arrow_Normal", ADULT, {"Weapon_Range_Bow"}},
        {"Item_Bomb", BOTH_AGES, {"Item_BombBag_Small", "Item_BombBag_Medium", "Item_BombBag_Large"}},
        {"Item_DekuSeed", CHILD, {"Weapon_Range_SlingShot"}},
        // Native item parameters grant these bags when the corresponding item
        // is first acquired. Their ownership survives an empty consumable stack.
        {"Item_DekuNut", BOTH_AGES, {"Item_NutBag_Small", "Item_NutBag_Medium", "Item_NutBag_Large"}},
        {"Item_DekuStick", CHILD, {"Item_StickBag_Small", "Item_StickBag_Medium", "Item_StickBag_Large"}},
        // Bombchus share bomb capacity. A Bomb Bag cannot establish ownership.
        {"Item_Bombchu", BOTH_AGES, {0}},
};

bool items_supported_for_age(uintptr_t actor, const char *name) {
    for (unsigned i = 0; i < sizeof ammo / sizeof *ammo; ++i) {
        const char *a = name, *b = ammo[i].name;
        while (*a && *a == *b) { ++a; ++b; }
        if (*a != *b) continue;
        unsigned age = ((unsigned (*)(uintptr_t))game(GAME_child_link))(actor) ? CHILD : ADULT;
        return (ammo[i].ages & age) != 0;
    }
    return false;
}

void items_update(uintptr_t pouch, uintptr_t actor, unsigned observed[2], bool refill,
                  unsigned generation) {
    if (generation != save_epoch() || !pouch || !singleton(0x84C9E2A8) ||
        !pointer(game(0x84EB6888))) return;
    bool child = ((unsigned (*)(uintptr_t))game(GAME_child_link))(actor) != 0;
    if (generation != save_epoch()) return;
    unsigned age = child ? CHILD : ADULT;
    // Native inventory getters can trigger a reload. Keep acquisition evidence
    // local until this invocation still belongs to the caller's save generation.
    unsigned owned = observed[child];
    for (unsigned i = 0; i < sizeof ammo / sizeof *ammo; ++i) {
        // Observe Bombchus even with the cheat OFF. Otherwise using the last
        // one before enabling Items loses acquisition evidence for this session.
        // Other stacks have independent owning tools and need no OFF polling.
        if (!refill && i != 5) continue;
        if (!(ammo[i].ages & age)) continue;
        const char *name = ammo[i].name;
        int amount = count(pouch, name);
        if (generation != save_epoch()) return;
        if (amount < 0) continue;
        unsigned mask = 1u << i;
        // These tools can be removed by Item Manager. Cached ammunition
        // evidence must never recreate arrows/seeds after their tool is gone.
        if (i == 0 || i == 2) {
            int tool = count(pouch, ammo[i].tools[0]);
            if (generation != save_epoch()) return;
            if (tool != 1) { owned &= ~mask; continue; }
        }
        if (amount) owned |= mask;
        if (!refill) continue;
        // Ask the native getter for independent persisted acquisition/use
        // evidence. The recorded depleted-save case sets it after native use.
        // Resolve the game's own mapping instead of hard-coding save keys or
        // archive layouts. False/missing evidence never grants an item.
        if (!(owned & mask) && i == 5 && pointer(game(0x84EA42A8)) &&
            ((unsigned (*)(uintptr_t, unsigned))game(GAME_action_guide_opened))(0, 36))
            owned |= mask; // Verified ActionGuideType::Bombchu = 36 in this build.
        if (generation != save_epoch()) return;
        if (!(owned & mask)) for (unsigned j = 0; j < 3 && ammo[i].tools[j]; ++j) {
            if (count(pouch, ammo[i].tools[j]) > 0) owned |= mask;
            if (generation != save_epoch()) return;
        }
        if (!(owned & mask)) continue;
        int cap = ((int (*)(uintptr_t, const char **, unsigned, uintptr_t))game(GAME_ammo_capacity))(
            pouch, &name, 0, 0);
        if (generation != save_epoch()) return;
        if (cap <= amount || cap > 999) continue;
        typedef unsigned (*Give)(uintptr_t, const char **, unsigned, int, unsigned,
                                 unsigned, unsigned, unsigned, unsigned);
        ((Give)game(GAME_give_ammo))(pouch, &name, 0, cap - amount, 0, 0, 0, 0, 0);
        if (generation != save_epoch()) return;
    }
    if (generation == save_epoch()) observed[child] = owned;
}
