#include "guest.h"
#include "profile.h"
#include "save_guard.h"
#include "travel_runtime.h"
#include "flight.h"
#include "noclip.h"
#include "inventory.h"

uint64_t save_guard_filter(uintptr_t data, uint64_t buttons) {
    if (!(buttons & CONFIRM) || !data ||
        !(travel_holds_saves() || flight_holds_saves() || noclip_holds_saves() || inventory_busy())) return buttons;
    uint32_t type = 0;
    // Native FullScreenUIType defines separate Save and Load values. Save's
    // normal sequence publishes this state before accepting its confirmation.
    // Read only: never alter slot indices, UI state or serialized GameData.
    bool known = ((unsigned (*)(uintptr_t, uint32_t *, uint32_t))game(GAME_read_ui_type))(
        data, &type, 0x578d41e1);
    if (!known || type != 0x42814d01) return buttons; // Murmur3("Save").
    return (buttons & ~(uint64_t)CONFIRM) | BACK;
}
