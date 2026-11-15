#include "guest.h"
#include "profile.h"
#include "travel_runtime.h"

typedef TravelAnchor Location;
enum TravelPhase { IDLE, READY, REQUESTED, RUNNING };
static struct {
    enum TravelPhase phase;
    Location target, source, previous;
    const char *arrival_scene;
    bool returning, changing_age, age_warp, recovery;
    unsigned step;
    uint64_t deadline, mismatch_since;
} trip;
// The constructor's final inline string starts at +0x1a9c and has 128 bytes.
// Its last-byte store is at +0x1b1b; the 8-byte-aligned object occupies 0x1b20.
enum { EVENT_REQUEST_BYTES = 0x1b20 };
static struct {
    _Alignas(16) uint64_t before[2];
    uint8_t bytes[EVENT_REQUEST_BYTES];
    uint64_t after[2];
} event_storage;
#define event_request event_storage.bytes
_Static_assert(0x1a9c + 128 <= EVENT_REQUEST_BYTES, "native inline event-name capacity");
static bool save_hold;
static bool layout_valid = true;
static TravelAnchor prepared_source;
static uint32_t prepared_request;
static unsigned recovery_epoch;
static TravelRecoveryState recovery_state;

TravelRecoveryState travel_recovery_state(unsigned expected_epoch) {
    TravelRecoveryState state = __atomic_load_n(&recovery_state, __ATOMIC_ACQUIRE);
    return expected_epoch == save_epoch() &&
           expected_epoch == __atomic_load_n(&recovery_epoch, __ATOMIC_RELAXED) ?
           state : TRAVEL_RECOVERY_NONE;
}

static bool copy_text(char *out, size_t capacity, const char *in) {
    if (!in) return false;
    size_t i = 0;
    while (in[i] && i + 1 < capacity) { out[i] = in[i]; ++i; }
    out[i] = 0;
    return !in[i];
}

static bool equal_text(const char *left, const char *right) {
    if (!left || !right) return false;
    while (*left && *left == *right) { ++left; ++right; }
    return *left == *right;
}

static uintptr_t character(uintptr_t actor) {
    uintptr_t components = actor ? pointer(actor + 0x270) : 0;
    uintptr_t physics = components ? pointer(components + 0x50) : 0;
    uintptr_t controller = physics ? pointer(physics + 0x20) : 0;
    uintptr_t handle = controller ? pointer(controller + 0x10) : 0;
    return handle ? pointer(handle + 8) : 0;
}

bool player_on_ground(uintptr_t actor) {
    uintptr_t cct = character(actor);
    if (!cct || pointer(cct) != game(0x84c54f10)) return false;
    // The native grounded state also includes underwater floors. They are not
    // dry recovery anchors: ordinary drowning can resume after flight is disabled.
    // QueryPhysicsCctGetWaterDepth verifies this body pointer and wet flag bit.
    uintptr_t body = pointer(cct + 0x38);
    if (!body || !mapped(body + 0x1d0, 2, 1) ||
        (*(const uint16_t *)(body + 0x1d0) & 1)) return false;
    uintptr_t states = pointer(cct + 0x48);
    if (!states || !mapped(states, 0x60, 1)) return false;
    unsigned index = *(const unsigned *)(states + 0x58);
    if (index > 2) return false;
    uintptr_t state = pointer(states + 0x40 + index * 8);
    return state && mapped(state, 0x14, 1) && !*(const unsigned *)(state + 0x10);
}

bool player_in_water(uintptr_t actor) {
    uintptr_t cct = character(actor);
    if (!cct || pointer(cct) != game(0x84c54f10)) return false;
    uintptr_t body = pointer(cct + 0x38);
    return body && mapped(body + 0x1d0, 2, 1) &&
           (*(const uint16_t *)(body + 0x1d0) & 1);
}

static const char *read_text(uintptr_t data, uint32_t key) {
    const char *text = 0;
    return data && ((unsigned (*)(uintptr_t, const char **, uint32_t))game(GAME_read_text))(
        data, &text, key) ? text : 0;
}

static bool known_scene(const char *scene, unsigned age) {
    for (unsigned i = 0; i < AREA_COUNT; ++i)
        if (travel_areas[i].age == age && equal_text(scene, travel_areas[i].scene)) return true;
    return false;
}

static Location current_location(uintptr_t actor, uintptr_t data) {
    Location result = {0};
    if (!actor) return result;
    result.age = !((unsigned (*)(uintptr_t))game(GAME_child_link))(actor);
    const char *scene = read_text(data, 0x1d6189da);
    if (!known_scene(scene, result.age) || !copy_text(result.scene, sizeof result.scene, scene)) return result;
    const float *position = (const float *)(actor + 0x31c);
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (!__builtin_isfinite(position[axis])) return (Location){0};
        result.position[axis] = position[axis];
    }
    result.valid = player_on_ground(actor);
    return result;
}

bool travel_capture(uintptr_t actor, TravelAnchor *anchor) {
    Location location = current_location(actor, singleton(0x84C9E180));
    if (!location.valid) return false;
    *anchor = location;
    return true;
}

bool travel_same_scene(const TravelAnchor *anchor) {
    return anchor->valid && equal_text(anchor->scene, read_text(singleton(0x84C9E180), 0x1d6189da));
}

bool travel_recover(Menu *menu, const TravelAnchor *anchor) {
    unsigned generation = save_epoch();
    if (trip.phase != IDLE || !travel_same_scene(anchor)) return false;
    if (generation != save_epoch()) return false;
    trip.target = *anchor;
    trip.arrival_scene = 0;
    trip.recovery = true;
    trip.changing_age = trip.age_warp = false;
    __atomic_store_n(&save_hold, true, __ATOMIC_RELEASE);
    trip.phase = READY;
    __atomic_store_n(&recovery_epoch, generation, __ATOMIC_RELAXED);
    __atomic_store_n(&recovery_state, TRAVEL_RECOVERY_PENDING, __ATOMIC_RELEASE);
    trip.deadline = monotonic_ms() + 15000;
    menu->open = menu->prompt = menu->travel_prompt = false;
    return true;
}

bool travel_prepare_source(uint32_t request, const TravelAnchor *anchor) {
    if (!request || !anchor->valid || trip.phase != IDLE || !layout_valid ||
        !travel_same_scene(anchor)) return false;
    if (request != UINT32_MAX) {
        int index = destination_index(request);
        if (index < 0 || !destination_valid((unsigned)index) || destination_restriction((unsigned)index)) return false;
    } else if (!trip.previous.valid) return false;
    prepared_source = *anchor;
    prepared_request = request;
    __atomic_store_n(&save_hold, true, __ATOMIC_RELEASE);
    return true;
}

void travel_reset(void) {
    __atomic_store_n(&recovery_state, TRAVEL_RECOVERY_NONE, __ATOMIC_RELEASE);
    destination_set_gerudo_access(false);
    __atomic_store_n(&save_hold, false, __ATOMIC_RELEASE);
    trip = (typeof(trip)){0};
    prepared_request = 0;
    prepared_source.valid = false;
}

bool travel_busy(void) { return trip.phase != IDLE; }
bool travel_healthy(void) { return layout_valid; }
bool travel_holds_saves(void) { return __atomic_load_n(&save_hold, __ATOMIC_ACQUIRE); }

static bool queue_room(uintptr_t data, bool vectors) {
    unsigned text_used = *(const unsigned *)(data + 0x640) & 0xfffff;
    unsigned text_max = *(const unsigned *)(data + 0x630);
    unsigned vector_used = *(const unsigned *)(data + 0x400) & 0xfffff;
    unsigned vector_max = *(const unsigned *)(data + 0x3f0);
    return text_max >= text_used + 2 && (!vectors || vector_max >= vector_used + 2);
}

static bool request_step(uintptr_t data, uintptr_t event, const char *scene,
                          const char *spawn, const float *position, const char *event_name) {
    if (pointer(event + 0x68) || !queue_room(data, !spawn[0])) {
        debug_log("[hyrule] travel dispatch busy: event or reservation capacity\n");
        return false;
    }
    const uint64_t canary = 0x48434d5245515545ull;
    for (unsigned i = 0; i < 2; ++i) event_storage.before[i] = event_storage.after[i] = canary;
    ((void (*)(void *))game(GAME_event_request_init))(event_request);
    for (unsigned i = 0; i < 2; ++i) {
        if (event_storage.before[i] != canary || event_storage.after[i] != canary) {
            layout_valid = false;
            debug_log("[hyrule] event request guard changed; travel disabled\n");
            return false;
        }
    }
    char *event_buffer = (char *)pointer((uintptr_t)event_request + 0x1a90);
    unsigned capacity = *(const unsigned *)(event_request + 0x1a98);
    if (!event_buffer || capacity < 2 || capacity > 128 || !copy_text(event_buffer, capacity, event_name)) return false;
    unsigned native_flags = *(const unsigned *)(event + 0xb8);
    char scope[128] = {0};
    uintptr_t scope_text = pointer(event + 0xc0);
    if (mapped(scope_text, sizeof scope, 1)) copy_text(scope, sizeof scope, (const char *)scope_text);
    bool accepted = ((unsigned (*)(uintptr_t, const void *))game(GAME_event_request))(event, event_request) != 0;
    ((void (*)(void *))game(GAME_event_request_cleanup))(event_request + 0x1968);
    ((void (*)(void *))game(GAME_event_request_cleanup))(event_request + 0x1950);
    if (!accepted) {
        debug_log("[hyrule] native event manager explicitly refused travel\n");
        char message[] = "[hyrule] pre-dispatch flags 00000000\n";
        for (unsigned i = 0; i < 8; ++i)
            message[sizeof message - 10 + i] = "0123456789abcdef"[(native_flags >> (28 - 4 * i)) & 15];
        debug_log(message);
        debug_log("[hyrule] pre-dispatch native event scope follows:\n");
        debug_log(scope[0] ? scope : "(empty)");
        debug_log("\n");
        return false;
    }
    if (trip.age_warp) return true;
    typedef unsigned (*WriteText)(uintptr_t, const char **, uint32_t);
    WriteText write = (WriteText)game(GAME_write_text);
    bool reserved = write(data, &scene, 0xee27bf71) && write(data, &spawn, 0x0b570ab2);
    if (!spawn[0]) {
        float rotation[3] = {0};
        typedef unsigned (*WriteVector)(uintptr_t, const float *, uint32_t);
        WriteVector vector = (WriteVector)game(GAME_write_vector);
        reserved = reserved && vector(data, position, 0x52046b85) && vector(data, rotation, 0x1662143e);
    }
    if (!reserved) debug_log("[hyrule] native event accepted but scene reservation failed\n");
    return reserved;
}

static void cancel(Menu *menu, const char *reason) {
    if (trip.recovery)
        __atomic_store_n(&recovery_state, TRAVEL_RECOVERY_FAILED, __ATOMIC_RELEASE);
    __atomic_store_n(&save_hold, false, __ATOMIC_RELEASE);
    trip.phase = IDLE;
    prepared_request = 0;
    prepared_source.valid = false;
    menu->notice = reason;
    debug_log("[hyrule] travel transaction stopped; inspect menu status\n");
}

void travel_update(Menu *menu, uintptr_t actor, bool safe, bool paused, bool supported) {
    menu->can_return = trip.previous.valid;
    menu->return_age = trip.previous.age;
    if (!supported || !layout_valid) {
        menu->travel_request = 0;
        menu->travel_prompt = false;
        if (trip.phase != IDLE) cancel(menu, "Travel signatures differ. Further requests stopped.");
        else {
            prepared_request = 0;
            prepared_source.valid = false;
            __atomic_store_n(&save_hold, false, __ATOMIC_RELEASE);
            if (menu->tab == MENU_TRAVEL) menu->notice = "Travel signatures differ. Activation disabled.";
        }
        return;
    }
    uintptr_t data = singleton(0x84C9E180), event = singleton(0x84C9E090);
    if (!data || !event) return;
    if (menu->travel_request && trip.phase == IDLE) {
        uint32_t request = menu->travel_request;
        menu->travel_request = 0;
        trip.returning = request == UINT32_MAX;
        trip.recovery = false;
        __atomic_store_n(&recovery_state, TRAVEL_RECOVERY_NONE, __ATOMIC_RELEASE);
        if (trip.returning) {
            if (!trip.previous.valid) { cancel(menu, "No verified return location is available."); return; }
            trip.target = trip.previous;
            trip.arrival_scene = 0;
        } else {
            int index = destination_index(request);
            if (index < 0 || !destination_valid((unsigned)index)) {
                cancel(menu, "Unknown destination. Travel cancelled."); return;
            }
            const char *restriction = destination_restriction((unsigned)index);
            if (restriction) { cancel(menu, restriction); return; }
            const Destination *destination = &destinations[index];
            const TravelArea *area = &travel_areas[destination->area];
            trip.target = (Location){.age = area->age, .valid = true};
            trip.arrival_scene = destination_arrival_scene((unsigned)index);
            copy_text(trip.target.scene, sizeof trip.target.scene, area->scene);
            copy_text(trip.target.spawn, sizeof trip.target.spawn, destination->spawn);
            for (unsigned axis = 0; axis < 3; ++axis) trip.target.position[axis] = destination->position[axis];
        }
        trip.source = prepared_request == request && travel_same_scene(&prepared_source) ?
                      prepared_source : current_location(actor, data);
        prepared_request = 0;
        prepared_source.valid = false;
        // A normal swimming source may depart, but it must not overwrite a dry
        // Return anchor. Clipped and arbitrary airborne sources still need the
        // acknowledged Noclip handoff or grounded recovery first.
        if (!trip.source.valid && !(safe && trip.source.scene[0] && player_in_water(actor))) {
            cancel(menu, "Stand on safe ground before travelling."); return;
        }
        trip.changing_age = trip.source.age != trip.target.age;
        __atomic_store_n(&save_hold, true, __ATOMIC_RELEASE);
        trip.step = 0;
        trip.mismatch_since = 0;
        trip.phase = READY;
        trip.deadline = monotonic_ms() + 15000;
    }
    if (trip.phase == IDLE || paused || menu->alpha) return;
    uint64_t now = monotonic_ms();
    if (now > trip.deadline) { cancel(menu, "Travel timed out. Let the game finish."); return; }
    bool event_running = pointer(event + 0x68) != 0;
    if (trip.phase == READY) {
        if (!safe || event_running) return;
        trip.age_warp = trip.changing_age && trip.step == 1;
        const char *scene = trip.target.scene, *spawn = trip.target.spawn;
        const char *event_name = "DefaultParentEvent";
        if (trip.changing_age && trip.step < 2) {
            scene = trip.step ? (trip.target.age ? "Market_Adult" : "Market_Child")
                              : (trip.source.age ? "Market_Adult" : "Market_Child");
            spawn = "LightPrelude";
            if (trip.age_warp) event_name = trip.target.age ? "Market_Child_Field_ChildToAdult"
                                                          : "Market_Adult_Field_AdultToChild";
        }
        if (!request_step(data, event, scene, spawn, trip.target.position, event_name)) {
            cancel(menu, "The game refused the travel request."); return;
        }
        trip.phase = REQUESTED;
        if (trip.recovery)
            __atomic_store_n(&recovery_state, TRAVEL_RECOVERY_ACCEPTED, __ATOMIC_RELEASE);
        trip.deadline = now + 15000;
        debug_log("[hyrule] native travel event accepted\n");
        return;
    }
    if (trip.phase == REQUESTED) {
        if (event_running) { trip.phase = RUNNING; trip.deadline = now + 180000; }
        return;
    }
    const char *reserved = read_text(data, 0xee27bf71);
    if (event_running || !safe || !actor || !reserved || reserved[0]) {
        trip.mismatch_since = 0;
        return;
    }
    if (trip.changing_age && trip.step < 2) {
        if (trip.age_warp) {
            unsigned age = !((unsigned (*)(uintptr_t))game(GAME_child_link))(actor);
            if (age != trip.target.age) { cancel(menu, "Link age did not change. Travel stopped."); return; }
        }
        ++trip.step;
        trip.phase = READY;
        trip.deadline = now + 15000;
        return;
    }
    const char *arrived = read_text(data, 0x1d6189da);
    if (!equal_text(arrived, trip.arrival_scene ? trip.arrival_scene : trip.target.scene)) {
        // Some original entrance coordinates immediately touch another native
        // exit. Confirm a stable recognized scene after the event/loading ends,
        // then release this transaction instead of hiding the menu for 180 s.
        unsigned age = !((unsigned (*)(uintptr_t))game(GAME_child_link))(actor);
        if (!known_scene(arrived, age)) trip.mismatch_since = 0;
        else if (!trip.mismatch_since) trip.mismatch_since = now;
        else if (now - trip.mismatch_since >= 2000)
            cancel(menu, "Arrival changed scene. Travel stopped.");
        return;
    }
    trip.mismatch_since = 0;
    if (!trip.recovery && trip.source.valid) trip.previous = trip.source;
    trip.phase = IDLE;
    if (trip.recovery)
        __atomic_store_n(&recovery_state, TRAVEL_RECOVERY_NONE, __ATOMIC_RELEASE);
    __atomic_store_n(&save_hold, false, __ATOMIC_RELEASE);
    menu->can_return = trip.previous.valid;
    menu->notice = trip.recovery ? "Returned to the last safe ground position." :
                   trip.source.valid ? "Travel completed. Return location stored." :
                   trip.previous.valid ? "Travel completed. Last safe Return retained." :
                                         "Travel completed. No dry Return captured.";
    debug_log(trip.recovery ? "[hyrule] native flight recovery completed\n" :
                              "[hyrule] native travel completed; return location recorded\n");
}
