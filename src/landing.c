#include "guest.h"
#include "profile.h"
#include "flight.h"
#include "landing.h"

void flight_fall_damage(uintptr_t state, uintptr_t definition, void *request) {
    unsigned generation = save_epoch();
    // This bridge is confined to the native airborne-to-ground fall calculator.
    // Its private request stores the amount at +0 and Fall cause 4 at +0x14.
    // Enemy, blast, fire and other generic damage dispatch callers are unhooked.
    if (mapped(state, 0x10, 1) && mapped((uintptr_t)request, 0x28, 1) &&
        *(const int *)request > 0 && *(const unsigned *)((uintptr_t)request + 0x14) == 4) {
        uintptr_t actor = pointer(state + 8);
        if (actor && generation == save_epoch() && flight_consume_landing_protection(actor) &&
            generation == save_epoch()) {
            debug_log("[hyrule] one Fly landing fall-damage request suppressed\n");
            return;
        }
    }
    ((void (*)(uintptr_t, uintptr_t, void *))game(GAME_fall_damage_dispatch))(
        state, definition, request);
}
