#include "guest.h"
#include "movement_safety.h"

bool movement_ready(void) {
    // Native RequestPause reads this manager/table and atomically increments
    // record+1c. LoadingPauseMgr uses key Murmur3("LoadingPause"). Both paths
    // were independently traced and the live table/class identity checked.
    uintptr_t manager = pointer(game(0x84eb7e38));
    if (!mapped(manager, 0x10, 1)) return false;
    unsigned count = *(const unsigned *)manager;
    uintptr_t table = pointer(manager + 8);
    if (!count || count > 128 || !mapped(table, count * 0x20, 1)) return false;
    bool found = false;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned *entry = (const unsigned *)(table + i * 0x20);
        if (entry[2] != 0x0eafe200u) continue;
        if (found || __atomic_load_n(entry + 7, __ATOMIC_ACQUIRE)) return false;
        found = true;
    }
    return found;
}
