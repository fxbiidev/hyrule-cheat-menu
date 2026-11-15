// Execute the production arrival state machine with native event/text fixtures.
// This proves bounded failure handling, not actual destination geometry.
#include "guest.h"
#include "profile.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base = GAME_BASE;
static _Alignas(16) unsigned char event_bytes[0x100], data_bytes[0x800];
static const char *scene, *reserved;
static uint64_t clock_ms;
static unsigned epoch = 7, native_requests;
static bool accept_request = true;
unsigned save_epoch(void) { return epoch; }
static unsigned mock_child(uintptr_t actor) { assert(actor == 1); return 0; }
static unsigned mock_text(uintptr_t data, const char **out, uint32_t key) {
    assert(data == (uintptr_t)data_bytes);
    *out = key == 0x1d6189da ? scene : reserved;
    return *out != 0;
}
static void mock_request_init(void *request) {
    memset(request, 0, 0x1b20);
    *(uintptr_t *)((uintptr_t)request + 0x1a90) = (uintptr_t)request + 0x1a9c;
    *(unsigned *)((uintptr_t)request + 0x1a98) = 128;
}
static unsigned mock_request(uintptr_t event, const void *request) {
    assert(event == (uintptr_t)event_bytes);
    assert(!strcmp((const char *)request + 0x1a9c, "DefaultParentEvent"));
    ++native_requests;
    return accept_request;
}
static void mock_cleanup(void *request) { assert(request); }
static unsigned mock_write_text(uintptr_t data, const char **value, uint32_t key) {
    assert(data == (uintptr_t)data_bytes && value && *value);
    assert(key == 0xee27bf71 || key == 0x0b570ab2);
    if (key == 0xee27bf71) reserved = *value;
    return 1;
}
static unsigned mock_write_vector(uintptr_t data, const float value[3], uint32_t key) {
    assert(data == (uintptr_t)data_bytes && value);
    assert(key == 0x52046b85 || key == 0x1662143e);
    return 1;
}
static uintptr_t fixture_game(uintptr_t address) {
    if (address == GAME_read_text) return (uintptr_t)mock_text;
    if (address == GAME_child_link) return (uintptr_t)mock_child;
    if (address == GAME_event_request_init) return (uintptr_t)mock_request_init;
    if (address == GAME_event_request) return (uintptr_t)mock_request;
    if (address == GAME_event_request_cleanup) return (uintptr_t)mock_cleanup;
    if (address == GAME_write_text) return (uintptr_t)mock_write_text;
    if (address == GAME_write_vector) return (uintptr_t)mock_write_vector;
    assert(!"Unexpected native call");
    return 0;
}
static uintptr_t fixture_singleton(uintptr_t address) {
    if (address == 0x84C9E180) return (uintptr_t)data_bytes;
    assert(address == 0x84C9E090);
    return (uintptr_t)event_bytes;
}
uint64_t monotonic_ms(void) { return clock_ms; }
void debug_log(const char *text) { assert(text); }
bool mapped(uintptr_t address, size_t bytes, unsigned permissions) {
    return address && bytes && permissions;
}

#define game fixture_game
#define singleton fixture_singleton
#include "../src/travel_runtime.c"

static Menu start(void) {
    travel_reset();
    memset(event_bytes, 0, sizeof event_bytes);
    memset(data_bytes, 0, sizeof data_bytes);
    *(unsigned *)(data_bytes + 0x630) = *(unsigned *)(data_bytes + 0x3f0) = 16;
    accept_request = true;
    clock_ms = 10000;
    scene = "InsideGanonCastle";
    reserved = "";
    trip.phase = RUNNING;
    trip.deadline = clock_ms + 180000;
    trip.target = (Location){.scene = "GanonCastle", .age = 1, .valid = true};
    trip.source = (Location){.scene = "KakarikoVillage_Adult", .age = 1, .valid = true};
    __atomic_store_n(&save_hold, true, __ATOMIC_RELEASE);
    return (Menu){.enabled = 15, .speed = 3};
}
static void tick(Menu *menu) { travel_update(menu, 1, true, false, true); }

static void recovery_ack_tests(void) {
    TravelAnchor anchor = {.scene = "InsideGanonCastle", .age = 1, .valid = true};
    Menu menu = start(); trip.phase = IDLE;
    assert(travel_recover(&menu, &anchor));
    assert(travel_recovery_state(epoch) == TRAVEL_RECOVERY_PENDING);
    unsigned before = native_requests;
    travel_update(&menu, 1, true, true, true);
    assert(native_requests == before && travel_recovery_state(epoch) == TRAVEL_RECOVERY_PENDING);
    // Native refusal is different from queue acceptance. The caller must get
    // an explicit failure rather than mistake IDLE for a completed recovery.
    accept_request = false; tick(&menu);
    assert(native_requests == before + 1 && !travel_busy() && !travel_holds_saves());
    assert(travel_recovery_state(epoch) == TRAVEL_RECOVERY_FAILED);
    assert(!strcmp(menu.notice, "The game refused the travel request."));
    unsigned previous = epoch++;
    assert(travel_recovery_state(previous) == TRAVEL_RECOVERY_NONE);
    assert(travel_recovery_state(epoch) == TRAVEL_RECOVERY_NONE);

    menu = start(); trip.phase = IDLE;
    assert(travel_recover(&menu, &anchor)); tick(&menu);
    assert(travel_busy() && travel_recovery_state(epoch) == TRAVEL_RECOVERY_ACCEPTED);
    *(uintptr_t *)(event_bytes + 0x68) = 1; tick(&menu);
    assert(trip.phase == RUNNING && travel_recovery_state(epoch) == TRAVEL_RECOVERY_ACCEPTED);
    *(uintptr_t *)(event_bytes + 0x68) = 0; reserved = ""; tick(&menu);
    assert(!travel_busy() && travel_recovery_state(epoch) == TRAVEL_RECOVERY_NONE);
    assert(!strcmp(menu.notice, "Returned to the last safe ground position."));

    menu = start(); trip.phase = IDLE;
    assert(travel_recover(&menu, &anchor)); tick(&menu);
    assert(travel_recovery_state(epoch) == TRAVEL_RECOVERY_ACCEPTED);
    clock_ms += 15001; tick(&menu);
    assert(!travel_busy() && travel_recovery_state(epoch) == TRAVEL_RECOVERY_FAILED);
    assert(!strcmp(menu.notice, "Travel timed out. Let the game finish."));
    travel_reset(); assert(travel_recovery_state(epoch) == TRAVEL_RECOVERY_NONE);

    menu = start(); trip.phase = IDLE;
    assert(travel_recover(&menu, &anchor));
    travel_update(&menu, 1, true, false, false);
    assert(!travel_busy() && travel_recovery_state(epoch) == TRAVEL_RECOVERY_FAILED);
    travel_reset();
}

int main(void) {
    const uint32_t blocked[] = {0x887aed4f, 0x834153be, 0x73fcc09f};
    for (unsigned i = 0; i < sizeof blocked / sizeof *blocked; ++i) {
        destination_set_gerudo_access(true);
        Menu rejected = start(); // Reset must discard cached key-item ownership.
        assert(destination_locked((unsigned)destination_index(0x73fcc09f)));
        trip.phase = IDLE;
        __atomic_store_n(&save_hold, false, __ATOMIC_RELEASE);
        TravelAnchor source = {.scene = "InsideGanonCastle", .age = 1, .valid = true};
        assert(!travel_prepare_source(blocked[i], &source));
        rejected.travel_request = blocked[i];
        tick(&rejected);
        assert(!travel_busy() && !travel_holds_saves() && !rejected.travel_request);
        assert(rejected.notice && rejected.enabled == 15 && rejected.speed == 3);
    }
    Menu menu = start();
    tick(&menu);
    assert(trip.mismatch_since == 10000 && travel_busy() && travel_holds_saves());
    clock_ms += 1999;
    tick(&menu);
    assert(travel_busy());
    ++clock_ms;
    tick(&menu);
    assert(!travel_busy() && !travel_holds_saves());
    assert(menu.enabled == 15 && menu.speed == 3);
    assert(!strcmp(menu.notice, "Arrival changed scene. Travel stopped."));

    menu = start();
    tick(&menu);
    clock_ms += 1000;
    scene = "GanonCastle";
    tick(&menu);
    assert(!travel_busy() && !travel_holds_saves() && !trip.mismatch_since);
    assert(trip.previous.valid && menu.can_return);
    assert(!strcmp(menu.notice, "Travel completed. Return location stored."));

    menu = start();
    trip.arrival_scene = "InsideGanonCastle";
    tick(&menu);
    assert(!travel_busy() && !travel_holds_saves() && !trip.mismatch_since);
    assert(!strcmp(trip.target.scene, "GanonCastle")); // Request parameters stay original.
    assert(trip.previous.valid);

    menu = start();
    tick(&menu);
    scene = "UnknownScene";
    clock_ms += 5000;
    tick(&menu);
    assert(travel_busy() && !trip.mismatch_since);
    scene = 0;
    tick(&menu);
    assert(travel_busy() && !trip.mismatch_since);

    menu = start();
    tick(&menu);
    reserved = "GanonCastle";
    clock_ms += 5000;
    tick(&menu);
    assert(travel_busy() && !trip.mismatch_since);
    reserved = "";
    tick(&menu);
    assert(trip.mismatch_since == clock_ms);
    *(uintptr_t *)(event_bytes + 0x68) = 1;
    clock_ms += 5000;
    tick(&menu);
    assert(travel_busy() && !trip.mismatch_since);
    *(uintptr_t *)(event_bytes + 0x68) = 0;
    tick(&menu);
    clock_ms += 5000;
    travel_update(&menu, 0, true, false, true);
    assert(travel_busy() && !trip.mismatch_since);
    tick(&menu);
    clock_ms += 5000;
    travel_update(&menu, 1, false, false, true);
    assert(travel_busy() && !trip.mismatch_since);

    menu = start();
    trip.recovery = true;
    scene = "GanonCastle";
    tick(&menu);
    assert(!travel_busy() && !travel_holds_saves() && !trip.previous.valid);
    assert(!strcmp(menu.notice, "Returned to the last safe ground position."));
    recovery_ack_tests();
    puts("Production arrival state: stable scene mismatch, delayed completion, event/loading interruptions and recovery passed (native fixture).");
    puts("Recovery acknowledgement: queued/paused, explicit refusal, accepted/completed, timeout, unsupported and save-epoch isolation passed.");
    return 0;
}
