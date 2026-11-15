#include "guest.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base = GAME_BASE;
static unsigned epoch = 7;
unsigned save_epoch(void) { return epoch; }
static uintptr_t life_slot, reset_pointer_address;
static _Alignas(8) unsigned char life[0x1c20], magic_component[0x1bc0];
static _Alignas(8) unsigned char actor[0x300], components[0x60], physics[0x30], controller[0x20];
static uint64_t current_life[2], maximum_life[2];
static const uintptr_t native_handle = 0x12345678;
static unsigned healed, written, consumed, reads;
static int magic_amount = 20, magic_cap = 240;
static bool reset_component, reset_heal, reset_ground;
static uint32_t reset_read;
static float ground_step, swim_target, climb_translation[3];

static void put_pointer(void *bytes, size_t offset, uintptr_t value) {
    memcpy((unsigned char *)bytes + offset, &value, sizeof value);
}
static uintptr_t mock_pointer(uintptr_t address) {
    if (address == reset_pointer_address) { ++epoch; reset_pointer_address = 0; }
    return *(const uintptr_t *)address;
}
static uintptr_t mock_component(uintptr_t body) {
    assert(body == (uintptr_t)actor);
    if (reset_component) { ++epoch; reset_component = false; }
    return (uintptr_t)magic_component;
}
static bool mock_mapped(uintptr_t address, size_t size, unsigned permissions) {
    assert(address == (uintptr_t)magic_component && size == sizeof magic_component && permissions == 1);
    return true;
}
static unsigned mock_read(uintptr_t data, int *value, uintptr_t stats, uint32_t field) {
    assert(data == 20 && stats == 1 + 0x3bc0);
    assert(field == 0x39e8a20d || field == 0xcbca2f4d);
    ++reads;
    if (reset_read == field) { ++epoch; reset_read = 0; }
    *value = field == 0x39e8a20d ? magic_cap : magic_amount;
    return 1;
}
static void mock_heal(uintptr_t component) {
    assert(component == (uintptr_t)magic_component);
    ++healed;
    if (reset_heal) { ++epoch; reset_heal = false; }
}
static void mock_write(uintptr_t data, int value, uintptr_t stats, uint32_t field) {
    assert(data == 20 && value == magic_cap && stats == 1 + 0x3bc0 && field == 0xcbca2f4d);
    ++written;
}
static void mock_consume(uintptr_t counter, const int *cost) {
    assert(counter && cost); ++consumed;
}
float noclip_native_ground_factor(uintptr_t owner, unsigned speed) {
    assert(owner == (uintptr_t)actor);
    if (reset_ground) { ++epoch; reset_ground = false; }
    return movement_speed_factor(speed);
}
static void mock_ground(uintptr_t component, const float direction[3], float step) {
    assert(component == (uintptr_t)components && direction[0] == 1);
    ground_step = step;
}
static void mock_swim(uintptr_t owner, void *history, unsigned mode, unsigned special,
                      float target, float step) {
    assert(owner == (uintptr_t)actor && history == (void *)20 && mode == 3 && special == 4 && step == 0.5f);
    swim_target = target;
}
static void mock_climb(uintptr_t owner, const void *request) {
    assert(owner == (uintptr_t)controller);
    memcpy(climb_translation, (const unsigned char *)request + 0x1c, sizeof climb_translation);
}

#define READ 1
#define pointer mock_pointer
#define mapped mock_mapped
#define LIFE_SLOT ((uintptr_t)&life_slot)
#define GAME_ground_velocity ((uintptr_t)mock_ground)
#define GAME_swim_velocity ((uintptr_t)mock_swim)
#define GAME_climb_velocity ((uintptr_t)mock_climb)
#define GAME_write_struct_int ((uintptr_t)mock_write)
#define GAME_read_struct_int ((uintptr_t)mock_read)
#define GAME_magic_component ((uintptr_t)mock_component)
#define GAME_heal_magic ((uintptr_t)mock_heal)
#define ORIGINAL_consume_magic ((uintptr_t)mock_consume)
#include "../src/guest_resources.inc"

static void test_life(void) {
    unsigned *health = (unsigned *)&current_life[1];
    unsigned *capacity = (unsigned *)&maximum_life[1];
    *health = 12; *capacity = 40;
    publish_life_guard((uintptr_t)current_life, epoch);
    assert(life_slot == (uintptr_t)health);
    refill_life((uintptr_t)life, epoch); assert(*health == 40);
    *health = 0; refill_life((uintptr_t)life, epoch); assert(!*health); // Never resurrect.
    *health = 12; *capacity = 81;
    refill_life((uintptr_t)life, epoch); assert(*health == 12);
    *capacity = 40;
    reset_pointer_address = (uintptr_t)life + 0x1c18;
    unsigned previous = epoch;
    refill_life((uintptr_t)life, previous); assert(*health == 12 && epoch != previous);
    clear_life_guard();
    publish_life_guard((uintptr_t)current_life, previous); assert(!life_slot);
    refill_life((uintptr_t)life, previous); assert(*health == 12);
}

static void test_magic(void) {
    int amount = -1, cap = -1;
    assert(magic_levels(1, 20, &amount, &cap, epoch) && amount == 20 && cap == 240);
    magic_cap = 0; assert(!magic_levels(1, 20, &amount, &cap, epoch));
    magic_cap = 240; magic_amount = 241;
    assert(!magic_levels(1, 20, &amount, &cap, epoch));
    magic_amount = 20;
    reset_read = 0x39e8a20d; unsigned before_reads = reads;
    assert(!magic_levels(1, 20, &amount, &cap, epoch) && reads == before_reads + 1);
    reset_read = 0xcbca2f4d;
    assert(!magic_levels(1, 20, &amount, &cap, epoch));

    fill_magic((uintptr_t)life, 1, 20, 20, 240, epoch);
    assert(healed == 1 && written == 1);
    int cost = 60;
    consume_magic((uintptr_t)magic_component + 0x1b78, &cost); assert(!consumed);
    cost = 0; consume_magic((uintptr_t)magic_component + 0x1b78, &cost); assert(consumed == 1);
    cost = 60; consume_magic(100, &cost); assert(consumed == 2);
    unsigned previous = epoch++;
    // Even if the pointer has not yet been cleared, the old epoch cannot guard costs.
    consume_magic((uintptr_t)magic_component + 0x1b78, &cost); assert(consumed == 3);
    fill_magic((uintptr_t)life, 1, 20, 20, 240, previous);
    assert(healed == 1 && written == 1);

    reset_component = true;
    fill_magic((uintptr_t)life, 1, 20, 20, 240, epoch);
    assert(healed == 1 && written == 1);
    // A reset triggered by native healing cannot be followed by an old-save stat write.
    reset_heal = true;
    fill_magic((uintptr_t)life, 1, 20, 20, 240, epoch);
    assert(healed == 2 && written == 1);
    consume_magic((uintptr_t)magic_component + 0x1b78, &cost); assert(consumed == 4);
    fill_magic((uintptr_t)life, 1, 20, 20, 240, epoch);
    assert(healed == 3 && written == 2);
}

static void test_speed(void) {
    float direction[3] = {1, 0, 0};
    _Alignas(8) unsigned char request[0x40] = {0};
    put_pointer(request, 0, native_handle);
    *(unsigned *)(request + 0x18) = 4;
    const float original[3] = {2, 4, 6};
    memcpy(request + 0x1c, original, sizeof original);
    for (unsigned speed = 0; speed < 4; ++speed) {
        publish_speed((uintptr_t)actor | speed, epoch);
        float factor = movement_speed_factor(speed);
        ground_motion((uintptr_t)components, direction, 2); assert(ground_step == 2 * factor);
        swim_motion((uintptr_t)actor, (void *)20, 3, 4, 2, 0.5f); assert(swim_target == 2 * factor);
        climb_motion((uintptr_t)controller, request);
        for (unsigned axis = 0; axis < 3; ++axis)
            assert(climb_translation[axis] == original[axis] * factor);
        assert(!memcmp(request + 0x1c, original, sizeof original));
    }
    reset_ground = true;
    ground_motion((uintptr_t)components, direction, 2); assert(ground_step == 2);
    swim_motion((uintptr_t)actor, (void *)20, 3, 4, 2, 0.5f); assert(swim_target == 2);
    climb_motion((uintptr_t)controller, request); assert(climb_translation[0] == 2);
    unsigned previous = epoch - 1;
    publish_speed((uintptr_t)actor | 3, previous);
    ground_motion((uintptr_t)components, direction, 2); assert(ground_step == 2);
    publish_speed((uintptr_t)actor | 3, epoch);
    reset_pointer_address = (uintptr_t)components + 0x50;
    climb_motion((uintptr_t)controller, request); assert(climb_translation[0] == 2);
    assert(!memcmp(request + 0x1c, original, sizeof original));
    publish_speed(0, epoch);
}

int main(void) {
    put_pointer(life, 0x18, (uintptr_t)actor);
    put_pointer(life, 0x1c10, (uintptr_t)current_life);
    put_pointer(life, 0x1c18, (uintptr_t)maximum_life);
    put_pointer(magic_component, 0, game(0x84B6DFC0));
    put_pointer(magic_component, 0x18, (uintptr_t)actor);
    *(int *)(magic_component + 0x1bbc) = 20;
    put_pointer(actor, 0x270, (uintptr_t)components);
    put_pointer(components, 0x18, (uintptr_t)actor);
    put_pointer(components, 0x50, (uintptr_t)physics);
    put_pointer(physics, 0x20, (uintptr_t)controller);
    put_pointer(controller, 0x10, native_handle);
    test_life(); test_magic(); test_speed();
    puts("Production resource hooks: life/magic bounds, native ABI, all speed factors, getter resets and stale publications passed.");
}
