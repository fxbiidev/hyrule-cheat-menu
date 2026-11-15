#include "guest.h"
#include "travel_runtime.h"
#include "flight.h"
#include "noclip.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base = GAME_BASE;
static bool clipping, grounded = true, same_scene = true, ready = true;
static bool wet, recovery_accepted;
static TravelRecoveryState recovery_result;
static unsigned recovery_result_epoch;
static unsigned captures;
static unsigned recovery_attempts;
static unsigned epoch;
static bool load_in_scene_query;
static bool load_in_ready_query;
static _Alignas(16) unsigned char actor_bytes[0x400], components[0x58];
static _Alignas(16) unsigned char physics[0x28], controller[0x18], handle_bytes[0x10];
static float position[3];
static uint64_t clock_ms;
#define ACTOR ((uintptr_t)actor_bytes)

bool noclip_holds_saves(void) { return clipping; }
bool player_on_ground(uintptr_t actor) { assert(actor == ACTOR); return grounded; }
bool player_in_water(uintptr_t actor) { assert(actor == ACTOR); return wet; }
bool travel_same_scene(const TravelAnchor *anchor) {
    if (load_in_scene_query) { load_in_scene_query = false; ++epoch; }
    return same_scene && anchor->valid;
}
bool travel_capture(uintptr_t actor, TravelAnchor *anchor) {
    assert(actor == ACTOR); ++captures;
    if (!grounded) return false;
    *anchor = (TravelAnchor){.scene = "Fixture", .valid = true};
    memcpy(anchor->position, position, sizeof position);
    return true;
}
bool travel_recover(Menu *menu, const TravelAnchor *anchor) {
    ++recovery_attempts;
    if (recovery_accepted) {
        recovery_result = TRAVEL_RECOVERY_PENDING;
        recovery_result_epoch = epoch;
    }
    (void)menu; (void)anchor; return recovery_accepted;
}
TravelRecoveryState travel_recovery_state(unsigned expected_epoch) {
    return expected_epoch == recovery_result_epoch ? recovery_result : TRAVEL_RECOVERY_NONE;
}
bool world_safe(void) { return same_scene; }
bool movement_ready(void) {
    if (load_in_ready_query) { load_in_ready_query = false; ++epoch; }
    return ready;
}
unsigned save_epoch(void) { return epoch; }
bool mapped(uintptr_t address, size_t size, unsigned permissions) {
    return address == ACTOR + 0x31c && size == 3 * sizeof(float) && permissions == 1;
}
uint64_t monotonic_ms(void) { return clock_ms += 100; }
void debug_log(const char *text) { (void)text; }

#include "../src/flight.c"

static Menu start_flight(float height, unsigned enabled) {
    flight_reset(); same_scene = grounded = ready = true;
    clipping = wet = recovery_accepted = false;
    recovery_result = TRAVEL_RECOVERY_NONE;
    recovery_result_epoch = epoch;
    memset(position, 0, sizeof position);
    memcpy(actor_bytes + 0x31c, position, sizeof position);
    Menu menu = {.enabled = enabled | (1u << FLY), .open = true};
    assert(flight_available(&menu, ACTOR, true));
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(active);
    position[1] = height;
    memcpy(actor_bytes + 0x31c, position, sizeof position);
    grounded = false;
    recovery_attempts = 0;
    return menu;
}

static void stop_flight(Menu *menu) {
    menu->enabled &= ~(1u << FLY);
    flight_update(menu, ACTOR, true, true, 0, 0, 0, epoch);
}

static void test_native_landing(void) {
    float result[3];
    for (unsigned test = 0; test < 4; ++test) {
        Menu menu = start_flight(test == 0 ? 2 : test == 1 ? 30 : test == 2 ? 80 : 200,
                                 test == 3 ? 1u << GOD : 0);
        stop_flight(&menu);
        assert(!(menu.enabled & (1u << FLY)) && !active && !recovery_attempts);
        assert(!flight_velocity((uintptr_t)handle_bytes, result));
        assert(flight_holds_saves());
        assert(!flight_consume_landing_protection(ACTOR + 8));
        assert(flight_consume_landing_protection(ACTOR));
        assert(!flight_consume_landing_protection(ACTOR) && !flight_holds_saves());
    }
    // Extreme/invalid poses keep the checked-ground fallback, without arming
    // a fall token. A busy recovery leaves flight enabled for another attempt.
    Menu menu = start_flight(81, 0); stop_flight(&menu);
    assert(recovery_attempts == 1 && (menu.enabled & (1u << FLY)));
    assert(!flight_consume_landing_protection(ACTOR));
    menu = start_flight(NAN, 1u << GOD); recovery_accepted = true; stop_flight(&menu);
    assert(recovery_attempts == 1 && !(menu.enabled & (1u << FLY)) && !active);
    assert(!flight_consume_landing_protection(ACTOR));
    menu = start_flight(-81, 1u << GOD); stop_flight(&menu);
    assert(recovery_attempts == 1);

    // Ground/water contact can precede the native landing dispatch. Keep a
    // short fixed tail, then expire it without extending a later departure.
    for (unsigned water = 0; water < 2; ++water) {
        menu = start_flight(10, 0); stop_flight(&menu);
        grounded = !water; wet = water;
        // A frozen menu must not retire protection before native landing runs.
        for (unsigned i = 0; i < 8; ++i)
            flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
        assert(flight_holds_saves());
        menu.open = false;
        flight_update(&menu, ACTOR, true, false, 0, 0, 0, epoch);
        assert(flight_holds_saves());
        grounded = wet = false;
        for (unsigned i = 0; i < 4; ++i)
            flight_update(&menu, ACTOR, true, false, 0, 0, 0, epoch);
        assert(!flight_holds_saves() && !flight_consume_landing_protection(ACTOR));
    }
    // The exact landing dispatch can consume the tail before its expiry.
    menu = start_flight(10, 0); stop_flight(&menu); grounded = true;
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(flight_consume_landing_protection(ACTOR));
    // Noclip retains its own collision/floor recovery; flight OFF does not
    // mutate its selection or force a scene reload at ordinary heights.
    menu = start_flight(5, 1u << NOCLIP); clipping = true; stop_flight(&menu);
    assert(menu.enabled == (1u << NOCLIP) && !recovery_attempts);
    assert(flight_consume_landing_protection(ACTOR));

    // A full-shape-clear point recovered after Noclip OFF can still be below
    // terrain. Retain flight provenance and recover instead of falling to void.
    menu = start_flight(-11, 1u << NOCLIP);
    menu.enabled &= ~(1u << NOCLIP);
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    recovery_accepted = true; stop_flight(&menu);
    assert(recovery_attempts == 1 && !active && !(menu.enabled & (1u << FLY)));
    assert(!flight_consume_landing_protection(ACTOR));
    // The provenance check does not force ordinary above-ground drops to reload.
    menu = start_flight(20, 1u << NOCLIP);
    menu.enabled &= ~(1u << NOCLIP);
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    stop_flight(&menu);
    assert(!recovery_attempts && flight_consume_landing_protection(ACTOR));

    // No-ground cases have a bounded watchdog, and paused menu time never
    // spends its descent allowance. Busy recovery restores visible Fly control.
    for (unsigned accepted = 0; accepted < 2; ++accepted) {
        menu = start_flight(10, 0); stop_flight(&menu);
        clock_ms += 60000;
        flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
        assert(!recovery_attempts && flight_holds_saves());
        menu.open = false;
        flight_update(&menu, ACTOR, true, false, 0, 0, 0, epoch);
        assert(!recovery_attempts);
        recovery_accepted = accepted;
        clock_ms += 31000;
        flight_update(&menu, ACTOR, true, false, 0, 0, 0, epoch);
        assert(recovery_attempts == 1 && active == !accepted);
        assert(!flight_consume_landing_protection(ACTOR));
        if (!accepted) assert(menu.open && (menu.enabled & (1u << FLY)));
        else {
            assert(flight_holds_saves());
            recovery_result = TRAVEL_RECOVERY_NONE;
            flight_update(&menu, ACTOR, true, false, 0, 0, 0, epoch);
            assert(!flight_holds_saves());
        }
    }

    menu = start_flight(10, 0); stop_flight(&menu); ++epoch;
    assert(!flight_consume_landing_protection(ACTOR));
    flight_reset(); assert(!flight_holds_saves());
    menu = start_flight(10, 0); stop_flight(&menu);
    same_scene = false; (void)flight_available(&menu, ACTOR, true);
    assert(!flight_consume_landing_protection(ACTOR) && !flight_holds_saves());
    menu = start_flight(10, 0); stop_flight(&menu);
    menu.enabled |= 1u << FLY;
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(!flight_consume_landing_protection(ACTOR));
    menu = start_flight(10, 0); stop_flight(&menu);
    menu.travel_request = 42;
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(!menu.travel_request && menu.open);
    assert(!flight_velocity((uintptr_t)handle_bytes, result));
    assert(flight_consume_landing_protection(ACTOR));
    menu = start_flight(10, 0); stop_flight(&menu);
    grounded = true; menu.travel_request = 42;
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(!flight_consume_landing_protection(ACTOR) && !flight_holds_saves());
}

static void test_recovery_acknowledgement(void) {
    float result[3];
    for (unsigned after_acceptance = 0; after_acceptance < 2; ++after_acceptance) {
        Menu menu = start_flight(100, 0);
        recovery_accepted = true; stop_flight(&menu);
        assert(!active && recovery_attempts == 1 && flight_holds_saves());
        assert(flight_velocity((uintptr_t)handle_bytes, result));
        assert(!result[0] && !result[1] && !result[2]);
        // A safe first post-pause pad poll can precede Link identification.
        // Keep the already-scoped zero hover until identification resumes.
        flight_update(&menu, 0, false, true, 0, 0, 0, epoch);
        assert(flight_holds_saves());
        assert(flight_velocity((uintptr_t)handle_bytes, result));
        assert(!result[0] && !result[1] && !result[2]);
        // Local queue acceptance can precede a refusal or a later native
        // timeout. Neither path may release gravity into an unprotected fall.
        if (after_acceptance) {
            recovery_result = TRAVEL_RECOVERY_ACCEPTED;
            ready = false;
            flight_update(&menu, 0, false, true, 0, 0, 0, epoch);
            assert(flight_holds_saves() && !active);
            assert(!flight_velocity((uintptr_t)handle_bytes, result));
            ready = true;
        }
        recovery_result = TRAVEL_RECOVERY_FAILED;
        flight_update(&menu, ACTOR, true, false, 0, 0, 0, epoch);
        assert(active && menu.open && (menu.enabled & (1u << FLY)));
        assert(recovery_attempts == 1 && flight_holds_saves());
        assert(flight_velocity((uintptr_t)handle_bytes, result));
        assert(!result[0] && !result[1] && !result[2]);
        assert(!flight_consume_landing_protection(ACTOR));
    }
    Menu menu = start_flight(100, 0);
    recovery_accepted = true; stop_flight(&menu);
    recovery_result = TRAVEL_RECOVERY_ACCEPTED;
    flight_update(&menu, ACTOR, false, false, 0, 0, 0, epoch);
    assert(flight_velocity((uintptr_t)handle_bytes, result) && flight_holds_saves());
    recovery_result = TRAVEL_RECOVERY_NONE; grounded = true;
    flight_update(&menu, ACTOR, true, false, 0, 0, 0, epoch);
    assert(!active && !(menu.enabled & (1u << FLY)) && !flight_holds_saves());
    assert(!flight_velocity((uintptr_t)handle_bytes, result));
    menu = start_flight(100, 0); recovery_accepted = true; stop_flight(&menu);
    ++epoch;
    assert(!flight_velocity((uintptr_t)handle_bytes, result));
    flight_reset(); assert(!flight_holds_saves());
    menu = start_flight(100, 0); recovery_accepted = true; stop_flight(&menu);
    same_scene = false; (void)flight_available(&menu, ACTOR, true);
    assert(!active && !flight_holds_saves());
}

int main(void) {
    *(uintptr_t *)(actor_bytes + 0x270) = (uintptr_t)components;
    *(uintptr_t *)(components + 0x50) = (uintptr_t)physics;
    *(uintptr_t *)(physics + 0x20) = (uintptr_t)controller;
    *(uintptr_t *)(controller + 0x10) = (uintptr_t)handle_bytes;
    Menu menu = {0};
    assert(flight_available(&menu, ACTOR, true));
    assert(ground.valid && ground.position[1] == 0 && captures == 1);
    clipping = true;
    position[1] = 1.4f; // Contact can remain true after a bypass moved the body.
    assert(flight_available(&menu, ACTOR, true));
    assert(ground.position[1] == 0 && captures == 1);
    clipping = false;
    position[1] = 0.25f;
    assert(flight_available(&menu, ACTOR, true));
    assert(ground.position[1] == 0.25f && captures == 2);
    flight_reset(); clipping = true;
    assert(!flight_available(&menu, ACTOR, true));
    assert(!ground.valid && captures == 2);
    clipping = false;
    assert(flight_available(&menu, ACTOR, true));
    same_scene = false; clipping = true; menu.enabled = 1u << FLY;
    active = menu.open = true;
    float held[3] = {0}; publish(0x2000, held, epoch);
    assert(!flight_available(&menu, 0, true));
    assert(!ground.valid && !(menu.enabled & (1u << FLY)));
    assert(!flight_holds_saves());
    float velocity[3] = {6, 2, -3}, result[3];
    publish(0x2000, velocity, epoch);
    assert(flight_velocity(0x2000, result));
    assert(result[0] == 6 && result[1] == 2 && result[2] == -3);
    ready = false;
    assert(flight_velocity(0x2000, result));
    assert(result[0] == 0 && result[1] == 0 && result[2] == 0);
    ready = true;
    assert(flight_velocity(0x2000, result) && result[0] == 6);
    assert(!flight_velocity(0x3000, result));
    // A native save-load hook changes generation before the next pad poll can
    // reset the menu. Its old velocity must already be ineligible in that gap.
    unsigned stale_epoch = epoch; ++epoch;
    assert(!flight_velocity(0x2000, result));
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, stale_epoch);
    assert(!flight_velocity(0x2000, result));
    publish(0x2000, velocity, epoch);
    assert(flight_velocity(0x2000, result) && result[0] == 6);
    ready = false; load_in_ready_query = true;
    assert(!flight_velocity(0x2000, result));
    ready = true;
    flight_reset();
    assert(!flight_holds_saves() && !flight_velocity(0x2000, result));
    same_scene = true; clipping = false;
    assert(flight_available(&menu, ACTOR, true));
    menu.open = true; menu.enabled = 1u << FLY;
    // A getter can begin a save during an already-running pad update. Stamping
    // its old actor with save_epoch() at the end would revive stale ownership.
    load_in_scene_query = true;
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(!flight_velocity((uintptr_t)handle_bytes, result));

    // Turning flight off at an ordinary airborne position must release native
    // gravity immediately, without re-enabling flight or reloading the scene.
    flight_reset(); same_scene = grounded = true; clipping = false;
    assert(flight_available(&menu, ACTOR, true));
    menu.enabled = 1u << FLY; menu.open = true;
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(active && flight_holds_saves());
    grounded = false; menu.enabled = 0; recovery_attempts = 0;
    flight_update(&menu, ACTOR, true, true, 0, 0, 0, epoch);
    assert(!(menu.enabled & (1u << FLY)) && !active && !recovery_attempts);
    assert(!flight_velocity((uintptr_t)handle_bytes, result));
    test_native_landing();
    test_recovery_acknowledgement();
    puts("Flight: native OFF descent, one landing guard, recovery acknowledgements/refusals, Noclip provenance, watchdog and save/scene fences passed.");
    return 0;
}
