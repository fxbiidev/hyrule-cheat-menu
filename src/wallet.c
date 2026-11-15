#include "guest.h"
#include "profile.h"
#include "wallet.h"

static uintptr_t wallet_owner, wallet_data;
static unsigned wallet_generation;

void wallet_reset(void) {
    __atomic_store_n(&wallet_owner, 0, __ATOMIC_RELEASE);
}

void wallet_update(uintptr_t pouch, uintptr_t data, unsigned generation,
                   bool enabled, bool authenticate, bool loaded) {
    if (!enabled || !loaded || !pouch || !data || generation != save_epoch()) {
        wallet_reset();
        return;
    }
    // Only a safe identified-player frame can authorize these native objects.
    // An ordinary dialogue may then retain the binding, without opening HCM or
    // continuing actor/physics operations there. Loading invalidates it.
    if (authenticate) {
        wallet_data = data;
        wallet_generation = generation;
        __atomic_store_n(&wallet_owner, pouch, __ATOMIC_RELEASE);
    }
    if (__atomic_load_n(&wallet_owner, __ATOMIC_ACQUIRE) != pouch ||
        wallet_data != data || wallet_generation != generation) {
        wallet_reset();
        return;
    }
    int cap = ((int (*)(uintptr_t))game(GAME_wallet_cap))(pouch);
    int amount = ((int (*)(uintptr_t, unsigned))game(GAME_read_rupees))(pouch, 0);
    // Native getters can cross a save boundary. Recheck before the only write.
    if (cap > 0 && cap <= 9999 && amount >= 0 && amount < cap &&
        generation == save_epoch() &&
        __atomic_load_n(&wallet_owner, __ATOMIC_ACQUIRE) == pouch)
        ((void (*)(uintptr_t, int, uintptr_t, uint32_t))game(GAME_write_struct_int))(
            data, cap, pouch + 0x3bc0, 0xc995bbb8);
}
