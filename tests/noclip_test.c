// Exercise the shipped movement/state code against a deterministic native CCT
// fixture. These checks prove contracts, not actual Havok or scene behavior.
#include "guest.h"
#include "travel_runtime.h"
#include "geometry.h"
#include "noclip.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

uintptr_t main_base = GAME_BASE;
static _Alignas(16) unsigned char actor_bytes[0x400], components[0x300], physics[0x60];
static _Alignas(16) unsigned char controller[0x30], handle_bytes[0x10], cct_bytes[0x60];
static _Alignas(16) unsigned char entity[0x100], parameters[0x200];
static float position[3], desired[3], elapsed;
static bool grounded = true, same_scene = true, accept_travel = true;
static bool wet;
static bool flight_hold;
static bool world_ready = true;
static TravelRecoveryState recovery_status;
static unsigned recovery_status_epoch;
static unsigned position_calls, original_calls, recovery_calls, flight_resets;
static unsigned epoch;
static bool load_in_position_query;
static bool load_in_scene_query;
static TravelAnchor prepared;
static uint64_t clock_ms;
static RecoveryResult geometry_result = RECOVERY_UNAVAILABLE;
static float geometry_position[3];
static unsigned geometry_calls;
static bool supported_floor;
static float floor_height;
static bool reset_in_geometry;
static bool reset_in_floor;
static bool loading_ready = true;
static float native_correction[3];
#define ACTOR ((uintptr_t)actor_bytes)

static void put_pointer(void *base, unsigned offset, uintptr_t value) {
    memcpy((unsigned char *)base + offset, &value, sizeof value);
}
static void publish_actor(void) { memcpy(actor_bytes + 0x31c, position, sizeof position); }
static void set_intent(float speed) {
    *(float *)(parameters + 0x1c0) = 1;
    *(float *)(parameters + 0x1d8) = speed;
}
static void mock_position(uintptr_t cct, const float *p) {
    assert(cct == (uintptr_t)cct_bytes);
    ++position_calls;
    memcpy(position, p, sizeof position);
}
static unsigned mock_get_position(uintptr_t cct, float *p) {
    assert(cct == (uintptr_t)cct_bytes);
    memcpy(p, position, sizeof position);
    if (load_in_position_query) { load_in_position_query = false; ++epoch; }
    return 0; // The real native query writes its output even when returning false.
}
static void mock_velocity(uintptr_t cct, const float *v) {
    assert(cct == (uintptr_t)cct_bytes);
    memcpy(entity + 0xe0, v, 3 * sizeof(float));
}
static uintptr_t mock_move(uintptr_t actor, uintptr_t output, uintptr_t context, uintptr_t flags) {
    assert(output == 123 && flags == 456 && context);
    ++original_calls;
    if (actor == ACTOR) {
        float *v = (float *)(entity + 0xe0);
        float x = position[0] + v[0] * elapsed;
        if (position[0] <= 1 && x > 1) x = 1; // Ordinary native wall response.
        position[0] = x;
        position[1] += v[1] * elapsed;
        position[2] += v[2] * elapsed;
        for (unsigned i = 0; i < 3; ++i) position[i] += native_correction[i];
        publish_actor();
    }
    return 789;
}
bool player_on_ground(uintptr_t actor) { assert(actor == ACTOR); return grounded && !wet; }
bool player_in_water(uintptr_t actor) { assert(actor == ACTOR); return wet; }
bool travel_same_scene(const TravelAnchor *anchor) {
    if (load_in_scene_query) { load_in_scene_query = false; ++epoch; }
    return anchor->valid && same_scene;
}
bool travel_capture(uintptr_t actor, TravelAnchor *anchor) {
    assert(actor == ACTOR);
    if (!grounded) return false;
    *anchor = (TravelAnchor){.scene = "Fixture", .valid = true};
    memcpy(anchor->position, position, sizeof position);
    return true;
}
bool travel_recover(Menu *menu, const TravelAnchor *anchor) {
    if (!accept_travel) return false;
    ++recovery_calls;
    prepared = *anchor;
    menu->open = false;
    return true;
}
bool travel_prepare_source(uint32_t request, const TravelAnchor *anchor) {
    assert(request == 42);
    prepared = *anchor;
    return accept_travel;
}
TravelRecoveryState travel_recovery_state(unsigned expected_epoch) {
    return expected_epoch == epoch && expected_epoch == recovery_status_epoch ?
           recovery_status : TRAVEL_RECOVERY_NONE;
}
void flight_reset(void) { ++flight_resets; flight_hold = false; }
bool flight_holds_saves(void) { return flight_hold; }
bool flight_velocity(uintptr_t handle, float out[3]) {
    assert(handle == (uintptr_t)handle_bytes);
    memcpy(out, desired, sizeof desired);
    return true;
}
uint64_t monotonic_ms(void) { return clock_ms += 100; }
bool world_safe(void) { return world_ready && same_scene; }
bool movement_ready(void) { return loading_ready; }
unsigned save_epoch(void) { return epoch; }
RecoveryResult geometry_recover(uintptr_t actor, bool needs_floor, float out[3]) {
    assert(actor == ACTOR); ++geometry_calls;
    (void)needs_floor;
    if (reset_in_geometry) { reset_in_geometry = false; noclip_reset(); }
    if (geometry_result > RECOVERY_NONE) memcpy(out, geometry_position, sizeof geometry_position);
    return geometry_result;
}
int geometry_floor(uintptr_t actor, const float p[3], float *height) {
    assert(actor == ACTOR && p);
    if (reset_in_floor) { reset_in_floor = false; noclip_reset(); }
    *height = floor_height;
    return supported_floor ? 1 : 0;
}

// Compile the actual production implementation with function addresses supplied
// by the test-only profile generated by run_tests.py.
#include "../src/noclip.c"

static Menu start(unsigned enabled) {
    noclip_reset();
    memset(position, 0, sizeof position);
    memset(entity, 0, sizeof entity);
    memset(desired, 0, sizeof desired);
    grounded = same_scene = accept_travel = true;
    wet = false;
    flight_hold = false;
    world_ready = true;
    recovery_status = TRAVEL_RECOVERY_NONE;
    recovery_status_epoch = epoch;
    geometry_result = RECOVERY_UNAVAILABLE;
    supported_floor = false;
    floor_height = 0;
    reset_in_geometry = false;
    reset_in_floor = false;
    loading_ready = true;
    load_in_position_query = false;
    load_in_scene_query = false;
    memset(native_correction, 0, sizeof native_correction);
    publish_actor();
    Menu menu = {.enabled = enabled};
    assert(noclip_available(&menu, ACTOR, true));
    noclip_update(&menu, ACTOR, true, epoch);
    return menu;
}
static void step(float dt) {
    float context[4] = {dt * 60, dt, 0, 0};
    elapsed = dt;
    assert(noclip_move(ACTOR, 123, (uintptr_t)context, 456) == 789);
}
static void near(float actual, float expected) { assert(fabsf(actual - expected) < 0.001f); }

int main(void) {
    put_pointer(actor_bytes, 0x270, (uintptr_t)components);
    put_pointer(components, 0x50, (uintptr_t)physics);
    put_pointer(physics, 0x20, (uintptr_t)controller);
    put_pointer(controller, 0x10, (uintptr_t)handle_bytes);
    put_pointer(handle_bytes, 8, (uintptr_t)cct_bytes);
    put_pointer(cct_bytes, 0, 0x84c54f10);
    put_pointer(cct_bytes, 0x20, (uintptr_t)entity);
    put_pointer(cct_bytes, 0x28, (uintptr_t)parameters);
    Menu menu = start(0);
    flight_hold = true;
    position[1] = 1.4f; publish_actor(); // Native contact flag has not cleared yet.
    assert(noclip_available(&menu, ACTOR, true));
    near(ground.position[1], 0);
    flight_hold = false;
    position[1] = 0.25f; publish_actor();
    assert(noclip_available(&menu, ACTOR, true));
    near(ground.position[1], 0.25f);
    for (unsigned index = 0; index < 4; ++index) {
        menu = start(1u << NOCLIP);
        menu.speed = index;
        noclip_update(&menu, ACTOR, true, epoch);
        near(noclip_native_ground_factor(ACTOR, index), 1);
        for (unsigned action = 0; action < 2; ++action) {
            float requested = action ? 6.74146f : 5.4f;
            float before = position[0];
            set_intent(requested * noclip_native_ground_factor(ACTOR, index));
            step(1.0f / 60);
            near(position[0] - before, requested * movement_speed_factor(index) / 60);
        }
        wet = true;
        near(noclip_native_ground_factor(ACTOR, index), movement_speed_factor(index));
        wet = false;
        menu.enabled |= 1u << FLY;
        noclip_update(&menu, ACTOR, true, epoch);
        near(noclip_native_ground_factor(ACTOR, index), movement_speed_factor(index));
    }
    near(movement_speed_factor(4), 1);
    menu = start(1u << NOCLIP);
    menu.speed = 3;
    noclip_update(&menu, ACTOR, true, epoch);
    set_intent(0);
    step(1.0f / 60);
    wet = true; grounded = false; supported_floor = wall_seen = true;
    set_intent(5.4f * noclip_native_ground_factor(ACTOR, menu.speed));
    step(1.0f / 60);
    near(position[0], 5.4f * 3 / 60); // Native water intent was already scaled.
    menu = start((1u << NOCLIP) | (1u << FLY));
    menu.speed = 3;
    noclip_update(&menu, ACTOR, true, epoch);
    desired[0] = 6;
    step(1.0f / 60);
    near(position[0], 6.0f / 60); // Normal speed never multiplies flight.
    menu = start(0);
    grounded = false;
    assert(noclip_available(&menu, 0, true));
    noclip_update(&menu, 0, true, epoch);
    assert(ground.valid && noclip_available(&menu, ACTOR, true));
    assert(!noclip_holds_saves());
    menu.enabled = 1u << NOCLIP;
    noclip_update(&menu, ACTOR, true, epoch);
    assert(noclip_holds_saves());
    noclip_update(&menu, 0, false, epoch);
    assert(!ground.valid && !(menu.enabled & (1u << NOCLIP)));
    menu = start(0);
    same_scene = false;
    assert(!noclip_available(&menu, 0, true));
    same_scene = true;
    menu = start(1u << NOCLIP);
    same_scene = false;
    assert(!noclip_available(&menu, 0, true));
    assert(!(menu.enabled & (1u << NOCLIP)) && !ground.valid && !noclip_holds_saves());
    noclip_update(&menu, 0, true, epoch);
    assert(!owner);
    menu = start(1u << NOCLIP);
    same_scene = false;
    (void)noclip_available(&menu, ACTOR, true);
    assert(!(menu.enabled & (1u << NOCLIP)));
    menu = start(0);
    unsigned calls = position_calls;
    step(1.0f / 60);
    assert(position_calls == calls && !noclip_holds_saves());
    float context[4] = {1, 1.0f / 60, 0, 0};
    assert(noclip_move(1, 123, (uintptr_t)context, 456) == 789);
    assert(position_calls == calls); // Unrelated actor remains untouched.

    for (unsigned fps = 30; fps <= 60; fps += 30) {
        menu = start(1u << NOCLIP);
        set_intent(4);
        for (unsigned i = 0; i < fps; ++i) step(1.0f / fps);
        near(position[0], 4); near(position[1], 0);
        assert(noclip_holds_saves());
    }
    // Loss of floor after a wall bypass restores the collision-valid start.
    grounded = false;
    *(float *)(entity + 0xe4) = -1;
    for (unsigned i = 0; i < 4; ++i) step(0.05f);
    near(position[0], 0); near(position[1], 0);
    grounded = true;
    for (unsigned i = 0; i < 3; ++i) step(0.05f);
    near(position[0], 0); // Held unsafe direction is blocked.
    set_intent(0); step(0.05f); set_intent(4); step(0.05f);
    assert(position[0] > 0);

    menu = start(1u << NOCLIP); set_intent(4); step(0.05f);
    loading_ready = false; calls = position_calls;
    for (unsigned i = 0; i < 5; ++i) step(0.05f);
    assert(position_calls == calls && menu.enabled == (1u << NOCLIP));
    float native_position = position[0];
    loading_ready = true; step(0.05f);
    assert(position[0] >= native_position); // Resumes from the native pose, without snapping back.

    menu = start(1u << NOCLIP); set_intent(4);
    for (unsigned i = 0; i < 6; ++i) step(0.05f);
    grounded = false; supported_floor = true;
    *(float *)(entity + 0xe4) = -1;
    for (unsigned i = 0; i < 8; ++i) step(0.05f);
    assert(position[0] > 2 && fabsf(position[1]) < 0.001f);
    supported_floor = false;
    for (unsigned i = 0; i < 6; ++i) step(0.05f);
    near(position[0], 0); // Loss of the verified floor still restores the clear point.

    menu = start(1u << NOCLIP); set_intent(4); step(0.05f);
    grounded = false;
    *(float *)(entity + 0xe4) = 3;
    for (unsigned i = 0; i < 10; ++i) step(0.05f);
    assert(position[1] > 1); // A native jump is not a missing-floor recovery.

    menu = start((1u << GOD) | (1u << FLY) | (1u << NOCLIP));
    desired[0] = 4; desired[1] = -4;
    for (unsigned i = 0; i < 20; ++i) step(0.05f);
    near(position[0], 4); near(position[1], -4);
    menu.open = true; noclip_update(&menu, ACTOR, true, epoch);
    for (unsigned i = 0; i < 20; ++i) step(0.05f);
    near(position[0], 4); near(position[1], -4);
    menu.enabled &= ~(1u << NOCLIP);
    noclip_update(&menu, ACTOR, true, epoch);
    assert(!menu.open && menu.enabled == ((1u << GOD) | (1u << FLY)));
    assert(noclip_busy());
    noclip_update(&menu, 0, true, epoch);
    assert(noclip_busy()); // Retain scope until the first post-pause job runs.
    step(0.05f); near(position[0], 0); near(position[1], 0);
    noclip_update(&menu, ACTOR, true, epoch); assert(!noclip_busy());

    menu = start((1u << GOD) | (1u << NOCLIP)); set_intent(4); step(0.05f);
    menu.enabled &= ~(1u << NOCLIP); noclip_update(&menu, ACTOR, true, epoch);
    assert(noclip_busy() && noclip_holds_saves());
    step(0.05f);
    assert(noclip_holds_saves()); // Hold remains until the native reload owns it.
    noclip_update(&menu, ACTOR, true, epoch);
    assert(recovery_calls == 1 && menu.enabled == (1u << GOD));
    near(prepared.position[0], 0);

    menu = start((1u << GOD) | (1u << FLY) | (1u << NOCLIP));
    desired[0] = 4; desired[1] = -4;
    for (unsigned i = 0; i < 20; ++i) step(0.05f);
    geometry_result = RECOVERY_NEARBY;
    geometry_position[0] = 3.5f; geometry_position[1] = -3.5f; geometry_position[2] = 0;
    menu.enabled &= ~(1u << NOCLIP); noclip_update(&menu, ACTOR, true, epoch);
    step(0.05f); noclip_update(&menu, ACTOR, true, epoch);
    near(position[0], 3.5f); near(position[1], -3.5f);
    assert(!noclip_busy() && recovery_calls == 1 && menu.enabled == ((1u << GOD) | (1u << FLY)));

    menu = start((1u << GOD) | (1u << NOCLIP)); set_intent(4); step(0.05f);
    geometry_result = RECOVERY_NEARBY;
    geometry_position[0] = 0.25f; geometry_position[1] = geometry_position[2] = 0;
    menu.enabled &= ~(1u << NOCLIP); noclip_update(&menu, ACTOR, true, epoch);
    step(0.05f); noclip_update(&menu, ACTOR, true, epoch);
    near(position[0], 0.25f);
    assert(!noclip_busy() && !noclip_holds_saves() && recovery_calls == 1);

    // Reproduced native floor settling must acknowledge a checked endpoint,
    // including lagging contact flags. Horizontal displacement, large drops,
    // missing floors and changed floor heights remain unacknowledged.
    for (unsigned correction = 0; correction < 6; ++correction) {
        menu = start((1u << GOD) | (1u << NOCLIP)); set_intent(4); step(0.05f);
        geometry_result = RECOVERY_NEARBY;
        geometry_position[0] = 0.25f; geometry_position[1] = geometry_position[2] = 0;
        menu.enabled &= ~(1u << NOCLIP); noclip_update(&menu, ACTOR, true, epoch);
        supported_floor = correction != 4;
        if (correction == 5) floor_height = 1;
        native_correction[1] = correction == 2 ? -0.08f : -0.02f;
        if (correction == 1) native_correction[0] = 0.02f;
        if (correction == 3) grounded = false;
        step(0.05f); noclip_update(&menu, ACTOR, true, epoch);
        assert(noclip_busy() == (correction != 0 && correction != 3));
        assert(noclip_holds_saves() == (correction != 0 && correction != 3));
        assert(recovery_calls == 1); // No forced reload for a native settling adjustment.
    }

    menu = start(1u << NOCLIP); set_intent(4); step(0.05f);
    geometry_result = RECOVERY_NEARBY; geometry_position[0] = 0.25f;
    geometry_position[1] = geometry_position[2] = 0;
    menu.enabled = 0; noclip_update(&menu, ACTOR, true, epoch);
    supported_floor = reset_in_floor = true; native_correction[1] = -0.02f;
    unsigned completed_before_reset = restore_complete;
    step(0.05f);
    assert(!noclip_busy() && !noclip_holds_saves() && restore_complete == completed_before_reset);

    menu = start((1u << FLY) | (1u << NOCLIP)); desired[0] = 4; step(0.05f);
    geometry_result = RECOVERY_NEARBY; reset_in_geometry = true;
    menu.enabled &= ~(1u << NOCLIP); noclip_update(&menu, ACTOR, true, epoch);
    calls = position_calls;
    step(0.05f);
    assert(position_calls == calls && !noclip_holds_saves() && !noclip_busy());
    // A reset inside a native query cannot publish a stale recovery hold/pose.

    menu = start((1u << GOD) | (1u << FLY) | (1u << NOCLIP));
    desired[1] = -4; step(0.05f);
    menu.travel_request = 42; noclip_update(&menu, ACTOR, true, epoch);
    assert(noclip_busy() && !menu.travel_request && noclip_holds_saves());
    step(0.05f); near(position[1], 0);
    for (unsigned i = 0; i < 4 && noclip_busy(); ++i) noclip_update(&menu, ACTOR, true, epoch);
    assert(!noclip_busy() && menu.enabled == (1u << GOD) && menu.travel_request == 42 && flight_resets == 1);
    near(prepared.position[1], 0);
    calls = position_calls; step(0.05f); assert(position_calls == calls);
    assert(!noclip_holds_saves()); // Native travel now owns the save hold.

    // Save generation changes synchronously in the native load hook, before
    // pad_read resets these publications. No stale CCT pose may be applied.
    menu = start(1u << NOCLIP); set_intent(4); step(0.05f);
    calls = position_calls; unsigned stale_epoch = epoch; ++epoch;
    near(noclip_native_ground_factor(ACTOR, 3), 3);
    noclip_update(&menu, ACTOR, true, stale_epoch);
    step(0.05f);
    assert(position_calls == calls);
    // Also reject a load that starts during an in-flight native position query.
    menu = start(1u << NOCLIP); set_intent(4); step(0.05f);
    calls = position_calls; load_in_position_query = true; step(0.05f);
    assert(position_calls == calls);
    // An old pad update must not stamp its actor with the new save generation
    // when a native getter begins loading before publication completes.
    menu = start(1u << NOCLIP); set_intent(4); step(0.05f);
    load_in_scene_query = true; noclip_update(&menu, ACTOR, true, epoch);
    calls = position_calls; step(0.05f);
    assert(position_calls == calls);
    near(noclip_native_ground_factor(ACTOR, 3), 3);

    menu = start(1u << NOCLIP); menu.travel_request = 42; accept_travel = false;
    noclip_update(&menu, ACTOR, true, epoch);
    step(0.05f);
    for (unsigned i = 0; i < 60 && noclip_busy(); ++i) noclip_update(&menu, ACTOR, true, epoch);
    assert(menu.enabled == (1u << NOCLIP) && !menu.travel_request && menu.open);
    assert(noclip_holds_saves());
    same_scene = false; noclip_update(&menu, ACTOR, true, epoch);
    assert(!menu.enabled && !noclip_holds_saves());
    menu = start(1u << NOCLIP); noclip_update(&menu, 0, true, epoch);
    assert(noclip_holds_saves()); // Post-pause identification gap retains scope.
    noclip_update(&menu, 0, false, epoch);
    assert(!menu.enabled && !noclip_holds_saves());
    // Travel selected before the first movement callback must still recover.
    menu = start(1u << NOCLIP); menu.travel_request = 42;
    noclip_update(&menu, ACTOR, true, epoch); step(0.05f);
    for (unsigned i = 0; i < 4 && noclip_busy(); ++i) noclip_update(&menu, ACTOR, true, epoch);
    assert(!noclip_busy() && menu.travel_request == 42);
    noclip_reset(); assert(!noclip_holds_saves());
    // Fly recovery can be queued and then refused after Noclip has already
    // restored a full-shape-clear point below a floor. Keep its checked anchor
    // while only the travel transaction makes "safe" false, so manual Noclip
    // escape remains available when Fly is restored after that refusal.
    menu = start((1u << FLY) | (1u << NOCLIP));
    desired[1] = -4;
    for (unsigned i = 0; i < 20; ++i) step(0.05f);
    geometry_result = RECOVERY_NEARBY;
    geometry_position[0] = geometry_position[2] = 0; geometry_position[1] = -3.5f;
    menu.enabled &= ~(1u << NOCLIP);
    noclip_update(&menu, ACTOR, true, epoch); step(0.05f);
    noclip_update(&menu, ACTOR, true, epoch);
    assert(!noclip_busy() && ground.valid);
    near(position[1], -3.5f);
    grounded = false; flight_hold = true; menu.enabled &= ~(1u << FLY);
    recovery_status = TRAVEL_RECOVERY_PENDING;
    noclip_update(&menu, ACTOR, false, epoch);
    assert(ground.valid && !owner && !noclip_holds_saves());
    recovery_status = TRAVEL_RECOVERY_FAILED; menu.enabled |= 1u << FLY;
    assert(noclip_available(&menu, ACTOR, true));
    menu.enabled |= 1u << NOCLIP; noclip_update(&menu, ACTOR, true, epoch);
    desired[1] = 4; step(0.05f);
    assert(noclip_holds_saves() && position[1] > -3.5f);

    for (unsigned condition = 0; condition < 7; ++condition) {
        menu = start(0); flight_hold = true;
        recovery_status = condition == 0 ? TRAVEL_RECOVERY_PENDING : TRAVEL_RECOVERY_ACCEPTED;
        if (condition == 2) world_ready = false;
        if (condition == 3) same_scene = false;
        if (condition == 4) loading_ready = false;
        if (condition == 5) recovery_status_epoch = epoch - 1;
        if (condition == 6) recovery_status = TRAVEL_RECOVERY_NONE;
        noclip_update(&menu, ACTOR, false, epoch);
        assert(ground.valid == (condition < 2));
        assert(!owner && !noclip_holds_saves());
    }
    assert(original_calls > 100);
    assert(geometry_calls >= 4);
    puts("Noclip scope, floor guard, Fly recovery refusal/manual escape, scene/save fences, nearby recovery and travel handoff passed (native fixture).");
}
