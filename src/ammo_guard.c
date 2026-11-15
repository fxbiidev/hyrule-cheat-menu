#include "guest.h"
#include "profile.h"
#include "items.h"
#include "ammo_guard.h"

static uintptr_t ammo_owner;
static unsigned ammo_generation;

void ammo_set_owner(uintptr_t actor, unsigned generation) {
    if (generation != save_epoch()) actor = 0;
    __atomic_store_n(&ammo_generation, generation, __ATOMIC_RELAXED);
    __atomic_store_n(&ammo_owner, actor, __ATOMIC_RELEASE);
}

static void use(uintptr_t pouch, const char **name, unsigned amount,
                unsigned alternate, unsigned flag4, unsigned flag5, bool nuts_only) {
    uintptr_t actor = __atomic_load_n(&ammo_owner, __ATOMIC_ACQUIRE);
    unsigned generation = __atomic_load_n(&ammo_generation, __ATOMIC_RELAXED);
    // These five callers consume a single standard player projectile/stick.
    // Alternate inventories and all unknown flags keep their native behavior.
    // The shared event caller is restricted further to nuts. Its other resource
    // removals, non-unit quantities, dialogs and cutscenes remain native.
    if (actor && generation == save_epoch() && amount == 1 && !alternate && !flag4 && !flag5 &&
        pouch == singleton(0x84C9E0D0) && mapped((uintptr_t)name, sizeof *name, 1)) {
        const char *text = *name;
        if (text && mapped((uintptr_t)text, 32, 1)) {
            unsigned length = 0;
            while (length < 32 && text[length]) ++length;
            bool eligible = !nuts_only;
            if (nuts_only && length == 12) {
                static const char nut[] = "Item_DekuNut";
                eligible = true;
                for (unsigned i = 0; i < length; ++i) if (text[i] != nut[i]) eligible = false;
            }
            if (eligible && length < 32 && world_safe() && items_supported_for_age(actor, text) &&
                generation == save_epoch() &&
                actor == __atomic_load_n(&ammo_owner, __ATOMIC_ACQUIRE)) {
                static bool reported;
                if (!__atomic_exchange_n(&reported, true, __ATOMIC_RELAXED))
                    debug_log("[hyrule] ordinary ammunition consumption protected\n");
                return;
            }
        }
    }
    ((void (*)(uintptr_t, const char **, unsigned, unsigned, unsigned, unsigned))
        game(GAME_consume_ammo))(pouch, name, amount, alternate, flag4, flag5);
}

void ammo_use(uintptr_t pouch, const char **name, unsigned amount,
              unsigned alternate, unsigned flag4, unsigned flag5) {
    use(pouch, name, amount, alternate, flag4, flag5, false);
}

void ammo_nut_use(uintptr_t pouch, const char **name, unsigned amount,
                  unsigned alternate, unsigned flag4, unsigned flag5) {
    use(pouch, name, amount, alternate, flag4, flag5, true);
}
