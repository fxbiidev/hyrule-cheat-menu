#include "guest.h"
#include "profile.h"
#include "noclip.h"
#include "flight.h"
#include "geometry.h"
#include "movement_safety.h"
#include "travel_runtime.h"

enum { CLIP = 1, FLYING = 2, PAUSED = 4 };
// Pad-thread state. Recovery anchors are captured only with ordinary collision.
static TravelAnchor ground;
static bool active;
static uint64_t next_ground_check;
static uint32_t pending_travel;
static bool pending_off;
static unsigned pending_ticket;
static uint64_t pending_deadline, settled_at;
// Publications to Link's native movement job. Never change another actor's CCT.
static uintptr_t owner, handle;
static unsigned mode, generation, owner_epoch, recovery_hold, floor_warning, travel_hold, normal_speed;
static unsigned restore_request, restore_complete, restore_generation, restore_ground;
static unsigned restore_fallback, restore_nearby;
static unsigned fallback_hold;
static uint32_t restore_bits[3];
// Movement-job state; the pad thread accesses only the atomic publications above.
static uintptr_t previous_actor;
static unsigned previous_generation;
static unsigned worker_restore;
static bool engaged, recovering, wall_seen, native_jump, blocked, loading_hold;
static float target[4], clear[4], air_time, blocked_x, blocked_z;

static bool current_job(uintptr_t actor, unsigned expected_generation) {
    return __atomic_load_n(&owner, __ATOMIC_ACQUIRE) == actor &&
           __atomic_load_n(&generation, __ATOMIC_ACQUIRE) == expected_generation &&
           __atomic_load_n(&owner_epoch, __ATOMIC_RELAXED) == save_epoch() &&
           world_safe() && movement_ready();
}

static void unpublish(void) {
    __atomic_store_n(&owner, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&mode, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&recovery_hold, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&floor_warning, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&fallback_hold, 0, __ATOMIC_RELEASE);
    __atomic_add_fetch(&generation, 1, __ATOMIC_RELEASE);
}

void noclip_reset(void) {
    unpublish();
    __atomic_store_n(&normal_speed, 0, __ATOMIC_RELEASE);
    active = false;
    ground.valid = false;
    next_ground_check = 0;
    pending_travel = 0;
    pending_off = false;
    settled_at = 0;
    __atomic_store_n(&travel_hold, 0, __ATOMIC_RELEASE);
}

bool noclip_busy(void) { return pending_travel != 0 || pending_off; }

bool noclip_holds_saves(void) {
    return (__atomic_load_n(&mode, __ATOMIC_ACQUIRE) & CLIP) ||
           __atomic_load_n(&recovery_hold, __ATOMIC_ACQUIRE) ||
           __atomic_load_n(&fallback_hold, __ATOMIC_ACQUIRE) ||
           __atomic_load_n(&travel_hold, __ATOMIC_ACQUIRE);
}

float noclip_native_ground_factor(uintptr_t actor, unsigned speed) {
    unsigned flags = __atomic_load_n(&mode, __ATOMIC_ACQUIRE);
    bool bypass = actor && actor == __atomic_load_n(&owner, __ATOMIC_ACQUIRE) &&
                  __atomic_load_n(&owner_epoch, __ATOMIC_RELAXED) == save_epoch() &&
                  (flags & CLIP) && !(flags & FLYING) && !player_in_water(actor);
    return bypass ? 1.0f : movement_speed_factor(speed);
}

bool noclip_available(Menu *menu, uintptr_t actor, bool safe) {
    if (!safe) return active || pending_off;
    // Loading can finish before the new Link life is identified. A changed
    // scene must clear the old publication and selection even in that gap.
    if (ground.valid && !travel_same_scene(&ground)) {
        noclip_reset();
        menu->enabled &= ~(1u << NOCLIP);
    }
    // A native pause can temporarily hide Link's life update even when Noclip
    // is OFF. Keep the checked same-scene anchor available for enabling it
    // during flight; a real world/save transition still invalidates it.
    if (!actor) return active || pending_off ||
                       (ground.valid && travel_same_scene(&ground));
    uint64_t now = monotonic_ms();
    // Native ground contact can lag the first ascent frame. Never replace the
    // pre-flight anchor with a hovering position, even if that flag is still set.
    if (!active && !pending_off && !flight_holds_saves() && now >= next_ground_check) {
        travel_capture(actor, &ground);
        next_ground_check = now + 100;
    }
    return ground.valid;
}

static uintptr_t handle_for(uintptr_t actor) {
    uintptr_t c = actor ? pointer(actor + 0x270) : 0;
    c = c ? pointer(c + 0x50) : 0;
    c = c ? pointer(c + 0x20) : 0;
    return c ? pointer(c + 0x10) : 0;
}

static void request_restore(bool to_ground, unsigned flags) {
    if (to_ground) for (unsigned i = 0; i < 3; ++i) {
        union { float value; uint32_t bits; } v = {.value = ground.position[i]};
        __atomic_store_n(&restore_bits[i], v.bits, __ATOMIC_RELAXED);
    }
    // Keep autosaves held through the handoff, including walking Noclip OFF.
    __atomic_store_n(&recovery_hold, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&mode, flags, __ATOMIC_RELEASE);
    __atomic_store_n(&restore_ground, to_ground ? 1u : 0u, __ATOMIC_RELAXED);
    __atomic_store_n(&restore_generation, __atomic_load_n(&generation, __ATOMIC_ACQUIRE), __ATOMIC_RELAXED);
    pending_ticket = __atomic_add_fetch(&restore_request, 1, __ATOMIC_RELEASE);
    pending_deadline = monotonic_ms() + 5000;
}

void noclip_update(Menu *menu, uintptr_t actor, bool safe, unsigned expected_epoch) {
    if (expected_epoch != save_epoch()) return;
    bool selected = menu->enabled & (1u << NOCLIP);
    // Retain both the anchor and publication across safe identification gaps.
    // OFF has no custom movement publication, but still needs its ground anchor
    // if the player enables Noclip later while Fly is already airborne.
    if (safe && !actor && !menu->travel_request) return;
    if (!safe && !selected && !active && !pending_off && !pending_travel &&
        ground.valid && flight_holds_saves()) {
        TravelRecoveryState recovery = travel_recovery_state(expected_epoch);
        if ((recovery == TRAVEL_RECOVERY_PENDING || recovery == TRAVEL_RECOVERY_ACCEPTED) &&
            world_safe() && movement_ready() && travel_same_scene(&ground) &&
            expected_epoch == save_epoch()) {
            // A queued Fly recovery makes safe=false before any world change.
            // Revoke movement, but retain the checked OFF anchor so a native
            // refusal cannot prevent Noclip from being re-enabled to escape.
            unpublish();
            return;
        }
    }
    if (!safe || !actor || !ground.valid || !travel_same_scene(&ground)) {
        menu->enabled &= ~(1u << NOCLIP);
        if (pending_travel) {
            menu->enabled &= ~(1u << FLY);
            flight_reset();
            menu->notice = "Travel cancelled by a world transition.";
        }
        noclip_reset();
        return;
    }
    if (pending_off) {
        if (__atomic_load_n(&restore_fallback, __ATOMIC_ACQUIRE) == pending_ticket &&
            !(menu->enabled & (1u << FLY))) {
            if (travel_recover(menu, &ground)) { noclip_reset(); return; }
            pending_off = false;
            active = selected = true;
            menu->enabled |= 1u << NOCLIP;
            menu->open = true;
            menu->notice = "Ground recovery is busy. Noclip remains ON.";
        } else if (__atomic_load_n(&restore_complete, __ATOMIC_ACQUIRE) == pending_ticket) {
            pending_off = false;
            menu->notice = __atomic_load_n(&restore_nearby, __ATOMIC_ACQUIRE) == pending_ticket ?
                "Collision restored at a checked nearby position." :
                "Noclip OFF returned to the last clear position.";
        } else if (monotonic_ms() <= pending_deadline) return;
        else {
            pending_off = false;
            active = selected = true;
            menu->enabled |= 1u << NOCLIP;
            menu->open = true;
            menu->notice = "Recovery unconfirmed. Noclip remains ON.";
        }
    }
    if (pending_travel) {
        uint64_t now = monotonic_ms();
        bool restored = __atomic_load_n(&restore_complete, __ATOMIC_ACQUIRE) == pending_ticket;
        if (restored && player_on_ground(actor)) {
            if (!settled_at) settled_at = now + 150;
            if (now >= settled_at && travel_prepare_source(pending_travel, &ground)) {
                menu->travel_request = pending_travel;
                menu->enabled &= ~((1u << NOCLIP) | (1u << FLY));
                flight_reset();
                noclip_reset();
                return;
            }
        } else settled_at = 0;
        if (now > pending_deadline) {
            pending_travel = 0;
            menu->enabled |= 1u << NOCLIP;
            menu->open = true;
            menu->notice = "Ground recovery unconfirmed. Travel cancelled.";
            __atomic_store_n(&travel_hold, 0, __ATOMIC_RELEASE);
            selected = true;
        } else return;
    }
    if (active && menu->travel_request) {
        // Restoring before native event dispatch is essential: dispatching from
        // below terrain can race the game's void recovery and refuse the event.
        pending_travel = menu->travel_request;
        menu->travel_request = 0;
        settled_at = 0;
        __atomic_store_n(&travel_hold, 1, __ATOMIC_RELEASE);
        menu->enabled &= ~(1u << NOCLIP);
        // The actor stays scoped until its movement job acknowledges recovery.
        request_restore(true, menu->enabled & (1u << FLY) ? FLYING : 0);
        return;
    }
    if (active && !selected) {
        // The Link movement job checks native shape overlap and nearby ground.
        // Closing releases the native world pause after fading so that job runs.
        menu->open = false;
        menu->notice = "Checking a safe position to restore collision.";
        active = false;
        pending_off = true;
        request_restore(false, (menu->enabled & (1u << FLY) ? FLYING : 0) |
                               (menu->alpha ? PAUSED : 0));
        return;
    }
    if (menu->open && __atomic_load_n(&floor_warning, __ATOMIC_ACQUIRE))
        menu->notice = "Floor safety: release or turn away.";
    active = selected;
    unsigned flags = (selected ? CLIP : 0) | (menu->enabled & (1u << FLY) ? FLYING : 0) |
                     (menu->open || menu->alpha ? PAUSED : 0);
    __atomic_store_n(&normal_speed, menu->speed, __ATOMIC_RELAXED);
    __atomic_store_n(&handle, handle_for(actor), __ATOMIC_RELEASE);
    __atomic_store_n(&mode, flags, __ATOMIC_RELEASE);
    // Native getters can begin loading while this pad update is in progress.
    // Bind its publication to the caller's original generation, never the new one.
    __atomic_store_n(&owner_epoch, expected_epoch, __ATOMIC_RELAXED);
    __atomic_store_n(&owner, actor, __ATOMIC_RELEASE);
}

uintptr_t noclip_move(uintptr_t actor, uintptr_t output, uintptr_t context, uintptr_t flags) {
    bool scoped = actor && actor == __atomic_load_n(&owner, __ATOMIC_ACQUIRE) &&
                  __atomic_load_n(&owner_epoch, __ATOMIC_RELAXED) == save_epoch();
    unsigned job_generation = __atomic_load_n(&generation, __ATOMIC_ACQUIRE);
    unsigned state = __atomic_load_n(&mode, __ATOMIC_ACQUIRE);
    uintptr_t h = scoped ? handle_for(actor) : 0;
    uintptr_t cct = h ? pointer(h + 8) : 0;
    scoped = cct && pointer(cct) == game(0x84c54f10);
    if (scoped && !movement_ready()) { loading_hold = true; scoped = false; }
    float velocity[3] = {0};
    if (scoped) {
        uintptr_t entity = pointer(cct + 0x20);
        float native[4] = {0};
        ((unsigned (*)(uintptr_t, float *))game(GAME_noclip_get_position))(cct, native);
        for (unsigned i = 0; i < 3; ++i) velocity[i] = ((float *)(entity + 0xe0))[i];
        if ((state & (CLIP | FLYING)) == (CLIP | FLYING)) {
            if (!flight_velocity(__atomic_load_n(&handle, __ATOMIC_ACQUIRE), velocity))
                for (unsigned i = 0; i < 3; ++i) velocity[i] = 0;
        } else if (state & CLIP) {
            // Native requested direction and metres/second, before wall resolution.
            // The ground hook leaves this input unscaled while bypass owns it,
            // so walking and A-actions receive the same multiplier exactly once.
            uintptr_t parameters = pointer(cct + 0x28);
            float factor = player_in_water(actor) ? 1.0f :
                movement_speed_factor(__atomic_load_n(&normal_speed, __ATOMIC_ACQUIRE));
            float speed = *(float *)(parameters + 0x1d8) * factor;
            velocity[0] = *(float *)(parameters + 0x1c0) * speed;
            velocity[2] = *(float *)(parameters + 0x1c8) * speed;
        }
        unsigned current_generation = job_generation;
        if (previous_actor != actor || previous_generation != current_generation) {
            previous_actor = actor;
            previous_generation = current_generation;
            engaged = recovering = wall_seen = native_jump = blocked = false;
            air_time = 0;
            worker_restore = __atomic_load_n(&restore_complete, __ATOMIC_ACQUIRE);
            for (unsigned i = 0; i < 3; ++i) clear[i] = ((float *)(actor + 0x31c))[i];
        }
        if (loading_hold) {
            for (unsigned i = 0; i < 3; ++i) target[i] = ((float *)(actor + 0x31c))[i];
            loading_hold = false;
        }
        if ((state & CLIP) && !engaged) {
            for (unsigned i = 0; i < 3; ++i) target[i] = clear[i] = ((float *)(actor + 0x31c))[i];
            engaged = true;
            recovering = wall_seen = native_jump = blocked = false;
            air_time = 0;
        } else if (engaged && !(state & CLIP)) {
            engaged = false;
            recovering = true;
        }
        unsigned request = __atomic_load_n(&restore_request, __ATOMIC_ACQUIRE);
        if (worker_restore != request) {
            worker_restore = request;
            if (__atomic_load_n(&restore_generation, __ATOMIC_RELAXED) == current_generation) {
                if (__atomic_load_n(&restore_ground, __ATOMIC_RELAXED)) for (unsigned i = 0; i < 3; ++i) {
                    union { uint32_t bits; float value; } v;
                    v.bits = __atomic_load_n(&restore_bits[i], __ATOMIC_RELAXED);
                    clear[i] = v.value;
                }
                else {
                    float nearby[3];
                    RecoveryResult found = geometry_recover(actor, !(state & FLYING), nearby);
                    if (!current_job(actor, job_generation)) scoped = false;
                    else if (found > RECOVERY_NONE) {
                        for (unsigned i = 0; i < 3; ++i) clear[i] = nearby[i];
                        __atomic_store_n(&restore_nearby, request, __ATOMIC_RELEASE);
                    } else {
                        if (!(state & FLYING)) __atomic_store_n(&fallback_hold, 1, __ATOMIC_RELEASE);
                        __atomic_store_n(&restore_fallback, request, __ATOMIC_RELEASE);
                    }
                }
                engaged = false;
                recovering = true;
            }
        }
        bool override = false, supported_floor = false;
        float dt = ((const float *)context)[1]; // Verified native elapsed seconds, not FPS.
        if (engaged && !(state & PAUSED) && __builtin_isfinite(dt) && dt > 0) {
            // Bound a single bypass step after a host hitch. This is not a world
            // streaming query and does not establish arbitrary 10x boundary safety.
            if (dt > 0.1f) dt = 0.1f;
            bool grounded = player_on_ground(actor);
            if (!(state & FLYING) && !grounded) {
                // Preserve native gravity/jumps/water. After wall traversal, loss
                // of floor without a native jump conservatively restores clear.
                native_jump = native_jump || velocity[1] > 0.1f;
                air_time += dt;
                float next[3] = {target[0] + velocity[0] * dt, target[1], target[2] + velocity[2] * dt};
                float floor = 0;
                bool support = wall_seen && !native_jump &&
                    geometry_floor(actor, next, &floor) == 1 &&
                    floor - target[1] <= 0.4f && target[1] - floor <= 0.5f;
                if (support) {
                    // A wall can hide floor contact from the ordinary CCT. Use
                    // an actual downward hit with an upward normal, never an
                    // assumed height, to continue horizontal wall traversal.
                    target[0] = next[0]; target[1] = floor; target[2] = next[2];
                    velocity[1] = 0;
                    supported_floor = true;
                    override = true;
                    air_time = 0;
                } else if (wall_seen && !native_jump && air_time > 0.15f) {
                    recovering = blocked = true;
                    blocked_x = velocity[0]; blocked_z = velocity[2];
                    __atomic_store_n(&floor_warning, 1, __ATOMIC_RELEASE);
                }
                if (!support) for (unsigned i = 0; i < 3; ++i) target[i] = native[i];
            } else {
                air_time = 0; native_jump = false;
                float ax = ((float *)(entity + 0xe0))[0] - velocity[0];
                float az = ((float *)(entity + 0xe0))[2] - velocity[2];
                if (ax * ax + az * az > 0.0025f) wall_seen = true;
                if (blocked) {
                    float intent = velocity[0] * blocked_x + velocity[2] * blocked_z;
                    if (intent <= 0 || velocity[0] * velocity[0] + velocity[2] * velocity[2] < 0.0001f) {
                        blocked = false;
                        __atomic_store_n(&floor_warning, 0, __ATOMIC_RELEASE);
                    } else velocity[0] = velocity[2] = 0;
                }
                for (unsigned i = 0; i < 3; ++i) {
                    float next = target[i] + velocity[i] * dt;
                    if (__builtin_isfinite(next)) target[i] = next;
                }
                if (!(state & FLYING)) target[1] = native[1];
                override = true;
            }
        } else if (engaged && (state & PAUSED)) {
            for (unsigned i = 0; i < 3; ++i) velocity[i] = 0;
            override = true;
        }
        if (recovering) {
            for (unsigned i = 0; i < 3; ++i) { target[i] = clear[i]; velocity[i] = 0; }
            override = true;
        }
        if (!current_job(actor, job_generation)) scoped = false;
        if (override && scoped) {
            ((void (*)(uintptr_t, const float *))game(GAME_noclip_position))(cct, target);
            float vertical[3] = {0, (state & FLYING) || recovering || supported_floor ? 0 : velocity[1], 0};
            ((void (*)(uintptr_t, const float *))game(GAME_set_linear_velocity))(cct, vertical);
        }
    }
    uintptr_t result = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))game(ORIGINAL_noclip_move))(
        actor, output, context, flags);
    if (scoped && current_job(actor, job_generation)) {
        if (recovering) {
            float distance = 0, horizontal = 0;
            float floor_correction = clear[1] - ((float *)(actor + 0x31c))[1];
            for (unsigned i = 0; i < 3; ++i) {
                float d = ((float *)(actor + 0x31c))[i] - clear[i];
                distance += d * d;
                if (i != 1) horizontal += d * d;
            }
            // The real CCT can settle a checked walking endpoint below the
            // query's floor clearance (observed 1.57 cm at Lon Lon Ranch).
            // Accept only that downward correction; keep the exact horizontal
            // check and require this request's verified nearby hit. Grounded
            // state can lag a forced CCT pose, so confirm the actual floor with
            // a fresh native ray instead of waiting indefinitely for that flag.
            bool settled_floor = false;
            if (!(state & FLYING) &&
                __atomic_load_n(&restore_nearby, __ATOMIC_ACQUIRE) == worker_restore &&
                horizontal < 0.0001f && floor_correction >= 0 &&
                floor_correction <= GEOMETRY_FLOOR_CLEARANCE + 0.01f) {
                float floor = 0;
                const float *actual = (const float *)(actor + 0x31c);
                settled_floor = geometry_floor(actor, actual, &floor) == 1 &&
                    __builtin_fabsf(floor - actual[1]) <= GEOMETRY_FLOOR_CLEARANCE + 0.01f;
                if (!current_job(actor, job_generation)) return result;
            }
            if (distance < 0.0001f || settled_floor) {
                recovering = wall_seen = false; air_time = 0;
                __atomic_store_n(&restore_complete, worker_restore, __ATOMIC_RELEASE);
            }
        }
        if (!engaged && !recovering)
            for (unsigned i = 0; i < 3; ++i) clear[i] = ((float *)(actor + 0x31c))[i];
        if (engaged && !(state & PAUSED) && ((state & FLYING) || player_on_ground(actor)))
            ((void (*)(uintptr_t, const float *))game(GAME_set_linear_velocity))(cct, velocity);
        __atomic_store_n(&recovery_hold, recovering ? 1u : 0u, __ATOMIC_RELEASE);
    }
    return result;
}
