#include "guest.h"
#include "profile.h"
#include "flight.h"
#include "travel_runtime.h"
#include "movement_safety.h"
#include "noclip.h"

static TravelAnchor ground;
static bool active;
static bool clipped_flight;
static uint64_t next_ground_check;
static float forward_x, forward_z = -1;
static unsigned sequence;
static unsigned owner_epoch;
static unsigned recovery_hold;
static uintptr_t owner;
static uint32_t velocity_bits[3];
// One native landing after Fly OFF may use its accumulated fall distance.
// This actor-scoped token is read only by the verified fall-damage callsite.
static uintptr_t landing_owner;
static unsigned landing_epoch;
static uint64_t landing_expires, landing_deadline;

static void clear_landing(void) {
    __atomic_store_n(&landing_owner, 0, __ATOMIC_RELEASE);
    landing_expires = 0;
    landing_deadline = 0;
}

static void protect_landing(uintptr_t actor, unsigned expected_epoch) {
    landing_expires = 0;
    landing_deadline = monotonic_ms() + 30000;
    __atomic_store_n(&landing_epoch, expected_epoch, __ATOMIC_RELAXED);
    __atomic_store_n(&landing_owner, actor, __ATOMIC_RELEASE);
}

bool flight_consume_landing_protection(uintptr_t actor) {
    uintptr_t expected = __atomic_load_n(&landing_owner, __ATOMIC_ACQUIRE);
    unsigned epoch = __atomic_load_n(&landing_epoch, __ATOMIC_RELAXED);
    return actor && expected == actor && epoch == save_epoch() &&
           __atomic_compare_exchange_n(&landing_owner, &expected, 0, false,
                                       __ATOMIC_ACQ_REL, __ATOMIC_RELAXED) &&
           epoch == save_epoch();
}

static bool natural_landing_pose(uintptr_t actor, bool god, bool clipping) {
    if (!mapped(actor + 0x31c, 3 * sizeof(float), 1)) return false;
    const float *p = (const float *)(actor + 0x31c);
    for (unsigned i = 0; i < 3; ++i)
        if (!__builtin_isfinite(p[i]) || p[i] < -100000 || p[i] > 100000) return false;
    // The checked ground anchor is a conservative height reference, not a ray
    // to the landing surface. God Mode permits higher finite native descents.
    float height = p[1] - ground.position[1];
    // Noclip's full-shape recovery can find clear air underneath a floor. Once
    // collision is restored, do not treat that known bypass history as a safe
    // native descent. Noclip still ON retains its separate floor policy.
    if (clipped_flight && !clipping && height < -0.15f) return false;
    return height >= -80 && (god || height <= 80);
}

static uintptr_t handle_for(uintptr_t actor) {
    uintptr_t components = actor ? pointer(actor + 0x270) : 0;
    uintptr_t physics = components ? pointer(components + 0x50) : 0;
    uintptr_t controller = physics ? pointer(physics + 0x20) : 0;
    return controller ? pointer(controller + 0x10) : 0;
}

static void publish(uintptr_t handle, const float velocity[3], unsigned expected_epoch) {
    __atomic_add_fetch(&sequence, 1, __ATOMIC_ACQ_REL);
    __atomic_store_n(&owner, handle, __ATOMIC_RELAXED);
    __atomic_store_n(&owner_epoch, expected_epoch, __ATOMIC_RELAXED);
    for (unsigned i = 0; i < 3; ++i) {
        union { float value; uint32_t bits; } converted = {.value = velocity ? velocity[i] : 0};
        __atomic_store_n(&velocity_bits[i], converted.bits, __ATOMIC_RELAXED);
    }
    __atomic_add_fetch(&sequence, 1, __ATOMIC_RELEASE);
}

static bool request_recovery(Menu *menu, uintptr_t actor, unsigned expected_epoch) {
    if (!travel_recover(menu, &ground)) return false;
    __atomic_store_n(&recovery_hold, 1, __ATOMIC_RELEASE);
    // Queue acceptance is not native acceptance. Keep a scoped hover until the
    // transaction acknowledges completion or hands control back after failure.
    publish(handle_for(actor), 0, expected_epoch);
    return true;
}

static bool update_recovery(Menu *menu, uintptr_t actor, unsigned expected_epoch) {
    if (!__atomic_load_n(&recovery_hold, __ATOMIC_ACQUIRE)) return false;
    TravelRecoveryState state = travel_recovery_state(expected_epoch);
    if (state == TRAVEL_RECOVERY_NONE) {
        __atomic_store_n(&recovery_hold, 0, __ATOMIC_RELEASE);
        publish(0, 0, expected_epoch);
        active = false;
        debug_log("[hyrule] safe-ground recovery acknowledged; flight hold released\n");
    } else if (state == TRAVEL_RECOVERY_FAILED && actor && world_safe() && movement_ready()) {
        __atomic_store_n(&recovery_hold, 0, __ATOMIC_RELEASE);
        menu->enabled |= 1u << FLY;
        menu->open = true;
        menu->notice = "Safe recovery refused. Fly restored; choose safe ground.";
        active = true;
        publish(handle_for(actor), 0, expected_epoch);
        debug_log("[hyrule] safe-ground recovery failed; Fly control restored\n");
    } else {
        // A safe first post-pause poll can precede actor identification. Retain
        // the existing scoped zero hover in that gap, as ordinary Fly does.
        // Actual loading clears the old handle before the actor is replaced.
        bool ready = world_safe() && movement_ready();
        if (actor || !ready)
            publish(actor && ready ? handle_for(actor) : 0, 0, expected_epoch);
        active = false;
        menu->enabled &= ~(1u << FLY);
    }
    return true;
}

void flight_reset(void) {
    publish(0, 0, save_epoch());
    __atomic_store_n(&recovery_hold, 0, __ATOMIC_RELEASE);
    clear_landing();
    active = false;
    clipped_flight = false;
    ground.valid = false;
    next_ground_check = 0;
}

bool flight_holds_saves(void) {
    return __atomic_load_n(&owner, __ATOMIC_ACQUIRE) ||
           __atomic_load_n(&recovery_hold, __ATOMIC_ACQUIRE) ||
           __atomic_load_n(&landing_owner, __ATOMIC_ACQUIRE);
}

bool flight_available(Menu *menu, uintptr_t actor, bool safe) {
    if (!safe) return active && menu->open;
    if (ground.valid && !travel_same_scene(&ground)) {
        flight_reset();
        menu->enabled &= ~(1u << FLY);
    }
    if (!actor) return active && menu->open;
    uint64_t now = monotonic_ms();
    // A grounded Noclip pose may already be inside a wall. Retain the last
    // ordinary-collision anchor through bypass and its recovery handoff.
    if (!active && !__atomic_load_n(&landing_owner, __ATOMIC_ACQUIRE) &&
        !__atomic_load_n(&recovery_hold, __ATOMIC_ACQUIRE) &&
        !noclip_holds_saves() && now >= next_ground_check) {
        travel_capture(actor, &ground);
        next_ground_check = now + 100;
    }
    return ground.valid;
}

void flight_update(Menu *menu, uintptr_t actor, bool safe, bool paused,
                   int32_t horizontal, int32_t vertical, uint32_t buttons, unsigned expected_epoch) {
    if (expected_epoch != save_epoch()) return;
    if (update_recovery(menu, actor, expected_epoch)) return;
    if ((active || __atomic_load_n(&landing_owner, __ATOMIC_ACQUIRE)) &&
        menu->travel_request && actor && !player_on_ground(actor)) {
        menu->travel_request = 0;
        menu->open = true;
        menu->notice = active ? "Land or disable Fly Mode before travelling." : "Land before travelling.";
        if (active) publish(handle_for(actor), 0, expected_epoch);
        return;
    }
    // Input may resume one frame before Link's first post-pause life update. Keep the
    // zero-velocity scoped publication until identification resumes; no actor is read.
    if (safe && !actor && (active || __atomic_load_n(&landing_owner, __ATOMIC_ACQUIRE)) &&
        !menu->travel_request) return;
    if (!safe || !actor || menu->travel_request || !ground.valid || !travel_same_scene(&ground)) {
        if (active) {
            if (!safe) debug_log("[hyrule] flight cleared: world transition\n");
            else if (!actor) debug_log("[hyrule] flight cleared: actor unavailable\n");
            else if (menu->travel_request) debug_log("[hyrule] flight cleared: travel selected\n");
            else debug_log("[hyrule] flight cleared: recovery anchor unavailable\n");
        }
        menu->enabled &= ~(1u << FLY);
        publish(0, 0, expected_epoch);
        clear_landing();
        active = false;
        clipped_flight = false;
        if (!safe || !actor) ground.valid = false;
        return;
    }
    bool selected = menu->enabled & (1u << FLY);
    if (!active && selected) clipped_flight = false;
    if ((active || selected) && (menu->enabled & (1u << NOCLIP))) clipped_flight = true;
    if (active && !selected) {
        if (natural_landing_pose(actor, (menu->enabled & (1u << GOD)) != 0,
                                 (menu->enabled & (1u << NOCLIP)) != 0)) {
            // Release only our velocity override. Native gravity and collision
            // handle the descent; one exact landing-damage call may be skipped.
            // This also covers the native contact flag's first-ascent-frame lag.
            protect_landing(actor, expected_epoch);
            debug_log("[hyrule] flight OFF released native gravity with one landing guard\n");
        } else if (!request_recovery(menu, actor, expected_epoch)) {
            menu->enabled |= 1u << FLY;
            menu->notice = "Safe recovery is busy. Fly Mode remains on.";
            selected = true;
        } else debug_log("[hyrule] flight OFF queued checked-ground recovery\n");
    }
    if (selected) clear_landing();
    else if (__atomic_load_n(&landing_owner, __ATOMIC_ACQUIRE)) {
        // Allow the native landing-damage dispatch to run before retiring an
        // undamaged landing. A later departure never extends this grace period.
        // Menu time cannot expire a landing whose native jobs are still paused.
        uint64_t now = monotonic_ms();
        if (paused || menu->open) {
            landing_expires = 0;
            landing_deadline = now + 30000;
        }
        else {
            if (!landing_expires && (player_on_ground(actor) || player_in_water(actor)))
                landing_expires = now + 250;
            if (landing_expires && now >= landing_expires) clear_landing();
            else if (now >= landing_deadline &&
                     __atomic_load_n(&landing_owner, __ATOMIC_ACQUIRE)) {
                // A missing landing must not leave an invisible, permanent
                // autosave hold. Prefer the checked native recovery; if busy,
                // return explicit hover/menu control so the player can recover.
                if (request_recovery(menu, actor, expected_epoch))
                    menu->notice = "Landing unconfirmed. Safe-ground recovery requested.";
                else {
                    selected = true;
                    menu->enabled |= 1u << FLY;
                    menu->open = true;
                    menu->notice = "Landing unconfirmed. Fly restored; choose safe ground.";
                }
                clear_landing();
            }
        }
    }
    if (!active && selected) debug_log("[hyrule] flight enabled with scoped collision velocity\n");
    active = selected;
    if (__atomic_load_n(&recovery_hold, __ATOMIC_ACQUIRE)) { active = false; return; }
    if (!active) { publish(0, 0, expected_epoch); return; }
    float velocity[3] = {0};
    if (!paused && !menu->open && movement_ready()) {
        uintptr_t camera = singleton(0x84c9dda0);
        if (mapped(camera, 0x3b0c, 1)) {
            float x = -*(const float *)(camera + 0x3ae8);
            float z = -*(const float *)(camera + 0x3b08);
            float length = x * x + z * z;
            if (__builtin_isfinite(length) && length > 0.0001f) {
                float inverse = 1.0f / __builtin_sqrtf(length);
                forward_x = x * inverse;
                forward_z = z * inverse;
            }
        }
        float x = (float)horizontal / 32767.0f, y = (float)vertical / 32767.0f;
        float radius = __builtin_sqrtf(x * x + y * y);
        if (radius <= 0.15f) x = y = 0;
        else {
            float response = radius < 1 ? (radius - 0.15f) / 0.85f : 1;
            x *= response / radius;
            y *= response / radius;
        }
        static const float factors[] = {1, 1.5f, 2, 3, 5, 10};
        float speed = 4 * factors[menu->flight_speed < 6 ? menu->flight_speed : 0];
        velocity[0] = (-forward_z * x + forward_x * y) * speed;
        velocity[2] = (forward_x * x + forward_z * y) * speed;
        velocity[1] = ((buttons & (1u << 9) ? 1 : 0) - (buttons & (1u << 8) ? 1 : 0)) * speed;
    }
    // Bind to the caller's original save, even if a native getter above began
    // another load. Never assign a stale actor the generation observed later.
    publish(handle_for(actor), velocity, expected_epoch);
}

bool flight_velocity(uintptr_t context_handle, float velocity[3]) {
    unsigned before = __atomic_load_n(&sequence, __ATOMIC_ACQUIRE);
    uintptr_t handle = __atomic_load_n(&owner, __ATOMIC_RELAXED);
    unsigned epoch = __atomic_load_n(&owner_epoch, __ATOMIC_RELAXED);
    // Save hooks invalidate motion immediately; pad_read may not have reset
    // the menu/publication yet when the next native physics job executes.
    if (!handle || context_handle != handle || epoch != save_epoch()) return false;
    if (!movement_ready()) {
        for (unsigned i = 0; i < 3; ++i) velocity[i] = 0;
    } else for (unsigned i = 0; i < 3; ++i) {
        union { float value; uint32_t bits; } converted;
        converted.bits = __atomic_load_n(&velocity_bits[i], __ATOMIC_RELAXED);
        velocity[i] = converted.value;
    }
    unsigned after = __atomic_load_n(&sequence, __ATOMIC_ACQUIRE);
    if (epoch != save_epoch()) return false;
    bool coherent = !(before & 1) && before == after;
    if (!coherent) for (unsigned i = 0; i < 3; ++i) velocity[i] = 0;
    return true;
}

void flight_step(uintptr_t states, uintptr_t unused, float *output, uintptr_t context) {
    ((void (*)(uintptr_t, uintptr_t, float *, uintptr_t))game(ORIGINAL_flight_step))(
        states, unused, output, context);
    float velocity[3];
    if (flight_velocity(pointer(context + 0x58), velocity)) {
        static bool traced;
        if (!traced) {
            traced = true;
            char message[] = "[hyrule] flight velocity caller 0000000000000000\n";
            uintptr_t caller = (uintptr_t)__builtin_return_address(0) - main_base + GAME_BASE;
            char *digits = message + sizeof message - 18;
            for (unsigned i = 0; i < 16; ++i) digits[i] = "0123456789abcdef"[(caller >> (60 - i * 4)) & 15];
            debug_log(message);
        }
        for (unsigned i = 0; i < 3; ++i) output[i] = velocity[i];
    }
}

void flight_job(uintptr_t job) {
    ((void (*)(uintptr_t))game(ORIGINAL_flight_job))(job);
    uintptr_t wanted = __atomic_load_n(&owner, __ATOMIC_ACQUIRE);
    if (!wanted) return;
    int64_t adjustment = *(const int64_t *)(job + 0x18);
    uintptr_t handle = pointer(job + 8) + (adjustment >> 1);
    if (handle != wanted) return;
    uintptr_t member = pointer(job + 0x10);
    if (adjustment & 1) {
        if (member > 4096 || (member & 7)) return;
        member = pointer(pointer(handle) + member);
    }
    // The native job dispatcher inlines this member's modifier loop. Other job
    // phases retain their normal behavior, including the collision solver.
    if (member != game(GAME_flight_job_member)) return;
    uintptr_t controller = pointer(handle + 8);
    if (!controller || pointer(controller) != game(0x84c54f10)) return;
    float velocity[3];
    if (!flight_velocity(handle, velocity)) return;
    ((void (*)(uintptr_t, const float *))game(GAME_set_linear_velocity))(controller, velocity);
    static bool reported;
    if (!reported) {
        reported = true;
        debug_log("[hyrule] flight applied after native velocity modifiers\n");
    }
}
