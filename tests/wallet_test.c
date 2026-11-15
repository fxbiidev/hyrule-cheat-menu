#include "guest.h"
#include "wallet.h"
#include <assert.h>
#include <stdio.h>

uintptr_t main_base = GAME_BASE;
static unsigned epoch = 7, writes, reads;
static int balance = 12, capacity = 200;
static bool reset_in_getter;

unsigned save_epoch(void) { return epoch; }
static int mock_wallet_cap(uintptr_t pouch) {
    assert(pouch == 1); ++reads;
    if (reset_in_getter) { ++epoch; wallet_reset(); reset_in_getter = false; }
    return capacity;
}
static int mock_rupees(uintptr_t pouch, unsigned mode) {
    assert(pouch == 1 && !mode); ++reads; return balance;
}
static void mock_stat_write(uintptr_t data, int value, uintptr_t stats, uint32_t key) {
    assert(data == 20 && stats == 1 + 0x3bc0 && key == 0xc995bbb8 && value == capacity);
    balance = value; ++writes;
}

#include "../src/wallet.c"

int main(void) {
    wallet_update(1, 20, epoch, true, false, true);
    assert(!writes && !reads && balance == 12); // An event cannot authorize an unknown pouch.
    wallet_update(1, 20, epoch, true, true, true);
    assert(writes == 1 && balance == 200);
    balance = 150;
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == 2 && balance == 200); // Retain the safe binding through a shop dialogue.
    capacity = 1000;
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == 3 && balance == 1000); // Detect upgrades even during an event.
    unsigned before = writes;
    balance = 950;
    wallet_update(1, 20, epoch, false, true, true);
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before && balance == 950); // OFF retains currency and revokes authorization.
    wallet_update(1, 20, epoch, true, true, true);
    assert(balance == 1000); before = writes;
    balance = 25;
    wallet_update(1, 20, epoch, true, false, false);
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before && balance == 25); // Loading requires a fresh safe binding afterward.
    wallet_update(1, 20, epoch, true, true, true); before = writes; balance = 50;
    wallet_update(2, 20, epoch, true, false, true);
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before && balance == 50); // Pouch replacement invalidates the previous pair.
    wallet_update(1, 20, epoch, true, true, true); before = writes; balance = 60;
    wallet_update(1, 21, epoch, true, false, true);
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before && balance == 60); // So does a different archive.
    wallet_update(1, 20, epoch, true, true, true); before = writes; balance = 70;
    unsigned previous_epoch = epoch++;
    wallet_update(1, 20, previous_epoch, true, true, true);
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before && balance == 70);
    wallet_update(1, 20, epoch, true, true, true); before = writes; balance = 80;
    reset_in_getter = true;
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before && balance == 80); // A save reset inside a native getter fences the write.
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before);
    wallet_update(1, 20, epoch, true, true, true); before = writes;
    capacity = 10000; balance = 1;
    wallet_update(1, 20, epoch, true, false, true);
    capacity = 0;
    wallet_update(1, 20, epoch, true, false, true);
    capacity = 1000; balance = -1;
    wallet_update(1, 20, epoch, true, false, true);
    balance = 1200;
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before && balance == 1200);
    wallet_update(0, 20, epoch, true, true, true);
    wallet_update(1, 0, epoch, true, true, true);
    wallet_update(1, 20, epoch, true, false, true);
    assert(writes == before);
    puts("Production wallet: safe binding, event spending, upgrades, OFF, loading, object changes and save fences passed.");
}
