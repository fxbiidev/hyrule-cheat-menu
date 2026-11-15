#include "guest.h"
#include "profile.h"
#include "travel_access.h"

static bool guard_released(void) {
    // The placed trainer's EnableSecurityGuard plug is Before(Step01) OR
    // Equal(Step01), using HoldChallengeCurrentStepCheckOrder. That command
    // reads live quest records, not GameData's separately cached enum value.
    uintptr_t manager = pointer(game(0x84e49950));
    if (!mapped(manager, 0x78, 1) || *(const uint8_t *)(manager + 0x74)) return false;
    uintptr_t quest = 0;
    unsigned total = 0;
    for (unsigned offset = 0x40; offset <= 0x60; offset += 0x10) {
        unsigned count = *(const unsigned *)(manager + offset);
        if (count > 512 || (total += count) > 1024) return false;
        if (!count) continue;
        uintptr_t records = pointer(manager + offset + 8);
        if (!mapped(records, (size_t)count * 0x160, 1)) return false;
        for (unsigned i = 0; i < count; ++i) {
            uintptr_t record = records + i * 0x160;
            if (*(const uint32_t *)(record + 8) != 0x6371358du) continue; // SageOfSpirit
            if (quest) return false;
            quest = record;
        }
    }
    if (!quest) return false;
    unsigned current = *(const unsigned *)(quest + 0x7c);
    unsigned count = *(const unsigned *)(quest + 0x80);
    uintptr_t steps = pointer(quest + 0x88);
    if (!count || count > 128 || current >= count || !mapped(steps, (size_t)count * 0x90, 1)) return false;
    unsigned reference = count;
    for (unsigned i = 0; i < count; ++i) {
        if (*(const uint32_t *)(steps + i * 0x90 + 0x78) != 0xeb62add9u) continue; // Step01
        if (reference != count) return false;
        reference = i;
    }
    return reference < count && current > reference &&
           !*(const uint8_t *)(manager + 0x74) && pointer(game(0x84e49950)) == manager;
}

bool gerudo_admission(uintptr_t pouch, uintptr_t data, unsigned epoch) {
    if (!pouch || !data || epoch != save_epoch() || !world_safe() || !guard_released()) return false;
    uint8_t received = 0;
    if (!((unsigned (*)(uintptr_t, uint8_t *, uint32_t))game(GAME_read_bool))(
            data, &received, 0x0e6264c7u) || received != 1 || epoch != save_epoch()) return false;
    const char *name = "Item_Token_Gerudo";
    int count = ((int (*)(uintptr_t, const char **, unsigned, unsigned))game(GAME_ammo_count))(
        pouch, &name, 0, 0);
    return count == 1 && epoch == save_epoch() && world_safe() && guard_released();
}
