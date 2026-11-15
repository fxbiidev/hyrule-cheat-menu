// Execute the production camera backend against checked fake native objects.
// This is a host fixture, not a display/emulator/controller acceptance test.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "guest.h"
#include "free_camera.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static _Alignas(16) unsigned char manager[0x30], camera[0x80], view[0x90], other_view[0x90];
static uintptr_t global_slot, manager_holder, camera_array;
static unsigned epoch = 1;
static uint64_t now = 1000;
static bool world = true, loading_ready = true, readable = true;
uintptr_t main_base;

static bool contains(uintptr_t start, size_t size, uintptr_t p, size_t n) {
    return p >= start && n <= size && p - start <= size - n;
}

bool mapped(uintptr_t p, size_t n, unsigned permissions) {
    (void)permissions;
    if (!readable) return false;
    return contains((uintptr_t)manager, sizeof manager, p, n) ||
           contains((uintptr_t)camera, sizeof camera, p, n) ||
           contains((uintptr_t)view, sizeof view, p, n) ||
           contains((uintptr_t)other_view, sizeof other_view, p, n) ||
           contains((uintptr_t)&global_slot, sizeof global_slot, p, n) ||
           contains((uintptr_t)&manager_holder, sizeof manager_holder, p, n) ||
           contains((uintptr_t)&camera_array, sizeof camera_array, p, n);
}
unsigned save_epoch(void) { return epoch; }
uint64_t monotonic_ms(void) { return now; }
bool world_safe(void) { return world; }
bool movement_ready(void) { return loading_ready; }
void debug_log(const char *message) { (void)message; }

#include "../src/free_camera.c"

static void put_pointer(void *p, uintptr_t value) { memcpy(p, &value, sizeof value); }
static float *matrix_at(void *v) { return (float *)((unsigned char *)v + 8); }
static void native_matrix(void *v, double x, double y, double z) {
    float m[12] = {1,0,0,(float)-x, 0,1,0,(float)-y, 0,0,1,(float)-z};
    double eye[3] = {x,y,z}, target[3] = {x,y,z - 1};
    memcpy((unsigned char *)v + 8, m, sizeof m);
    memcpy((unsigned char *)v + 0x60, eye, sizeof eye);
    memcpy((unsigned char *)v + 0x78, target, sizeof target);
}
static void reset_fixture(void) {
    memset(manager, 0, sizeof manager); memset(camera, 0, sizeof camera);
    memset(view, 0, sizeof view); memset(other_view, 0, sizeof other_view);
    memset(&observation, 0, sizeof observation); memset(&output, 0, sizeof output);
    memset(&pose, 0, sizeof pose); memset(&applied, 0, sizeof applied);
    memset(&free_camera_evidence, 0, sizeof free_camera_evidence);
    main_base = (uintptr_t)&global_slot - (0x84ca6bc0ull - GAME_BASE);
    global_slot = (uintptr_t)&manager_holder; manager_holder = (uintptr_t)manager;
    *(unsigned *)(manager + 0x20) = 32;
    put_pointer(manager + 0x28, (uintptr_t)&camera_array);
    camera_array = (uintptr_t)camera;
    put_pointer(camera + 0x78, (uintptr_t)view);
    put_pointer(view, game(0x84c18a88ull)); put_pointer(other_view, game(0x84c18a88ull));
    native_matrix(view, 10,20,30); native_matrix(other_view, 40,50,60);
    epoch = 1; now = 1000; world = loading_ready = readable = true;
    free_camera_init();
    assert(free_camera_hook_resume == game(ORIGINAL_free_camera_hook));
    free_camera_update(false, 0, epoch, false, true, 0);
}

static bool update(FreeCameraInput input, bool paused) {
    now += 50;
    return free_camera_update(true, 0x12345678u, epoch, true, paused, &input);
}

static void apply_native(void) {
    native_matrix(view, 10,20,30);
    free_camera_apply((uintptr_t)view);
}

static void normal_motion_and_restoration(void) {
    reset_fixture();
    FreeCameraInput neutral = {0};
    assert(!free_camera_available(epoch, false));
    free_camera_apply((uintptr_t)other_view);
    assert(!free_camera_available(epoch, false));
    apply_native();
    assert(free_camera_available(epoch, false));
    assert(update(neutral, true) && free_camera_active());
    double original_eye[3]; memcpy(original_eye, view + 0x60, sizeof original_eye);
    unsigned char other_before[sizeof other_view]; memcpy(other_before, other_view, sizeof other_before);
    FreeCameraInput forward = {.ly = 32767};
    assert(update(forward, false));
    apply_native();
    assert(fabsf(matrix_at(view)[11] + 29.6f) < 0.001f);
    assert(!memcmp(original_eye, view + 0x60, sizeof original_eye));
    free_camera_apply((uintptr_t)other_view);
    assert(!memcmp(other_before, other_view, sizeof other_before));
    FreeCameraInput strafe = {.lx = 32767};
    assert(update(strafe, false)); apply_native();
    assert(fabsf(matrix_at(view)[3] + 10.4f) < 0.001f);
    FreeCameraInput ascend = {.buttons = 1u << 9};
    assert(update(ascend, false)); apply_native();
    assert(fabsf(matrix_at(view)[7] + 20.4f) < 0.001f);
    FreeCameraInput descend = {.buttons = 1u << 8};
    assert(update(descend, false)); apply_native();
    assert(fabsf(matrix_at(view)[7] + 20.0f) < 0.001f);
    double before_pause[3]; memcpy(before_pause, pose.eye, sizeof before_pause);
    assert(update(forward, true)); assert(!memcmp(before_pause, pose.eye, sizeof before_pause));
    assert(!free_camera_update(false, 0x12345678u, epoch, true, false, &neutral));
    assert(!free_camera_active());
    native_matrix(view, 100,200,300);
    unsigned char native_before[sizeof view]; memcpy(native_before, view, sizeof native_before);
    free_camera_apply((uintptr_t)view);
    assert(!memcmp(native_before, view, sizeof native_before));
}

static void rotation_clamp_and_time(void) {
    reset_fixture(); apply_native();
    FreeCameraInput right = {.rx = 32767};
    assert(update((FreeCameraInput){0}, false));
    assert(update(right, false)); apply_native();
    assert(matrix_at(view)[8] < 0 && matrix_at(view)[0] > 0.9f);
    assert(finite_matrix(matrix_at(view)));
    FreeCameraInput up = {.ry = 32767};
    for (unsigned i = 0; i < 1000; ++i) assert(update(up, false));
    apply_native();
    assert(pose.back[1] >= -0.9851f && pose.back[1] <= -0.98f);
    assert(finite_matrix(matrix_at(view)));
    for (unsigned i = 0; i < 1000; ++i) assert(update((FreeCameraInput){.rx=-32767,.ry=-32767}, false));
    apply_native(); assert(finite_matrix(matrix_at(view)));
    assert(pose.back[1] <= 0.9851f && pose.back[1] >= 0.98f);
    double old[3]; memcpy(old, pose.eye, sizeof old);
    now += 600000;
    assert(update((FreeCameraInput){.ly=32767}, false));
    double dx = pose.eye[0] - old[0], dy = pose.eye[1] - old[1], dz = pose.eye[2] - old[2];
    assert(dx*dx + dy*dy + dz*dz < 0.161);
    memcpy(old, pose.eye, sizeof old);
    assert(update((FreeCameraInput){.lx=4000,.ly=-4000,.rx=4000,.ry=-4000}, false));
    assert(!memcmp(old, pose.eye, sizeof old));
}

static void interruptions_and_layout_guards(void) {
    FreeCameraInput neutral = {0};
    reset_fixture(); apply_native(); assert(update(neutral, false));
    world = false; apply_native(); assert(!free_camera_active());
    world = true; assert(!update(neutral, false)); // Must be explicitly reselected.
    assert(!free_camera_update(false, 0, epoch, true, false, &neutral));
    apply_native(); assert(update(neutral, false));
    loading_ready = false; apply_native(); assert(!free_camera_active());
    loading_ready = true; assert(!update(neutral, false));
    reset_fixture(); apply_native(); assert(update(neutral, false));
    ++epoch; assert(!free_camera_active()); apply_native(); assert(!update(neutral, false));
    reset_fixture(); apply_native(); assert(update(neutral, false));
    assert(!free_camera_update(true, 0x23456789u, epoch, true, false, &neutral));
    reset_fixture(); apply_native(); assert(update(neutral, false));
    put_pointer(camera + 0x78, (uintptr_t)other_view);
    free_camera_apply((uintptr_t)other_view); assert(!free_camera_active());
    reset_fixture(); apply_native(); assert(update(neutral, false));
    free_camera_reset(); assert(!free_camera_active()); assert(!update(neutral, false));
    reset_fixture(); apply_native(); assert(update(neutral, false));
    assert(!free_camera_update(true, 0x12345678u, epoch, false, false, &neutral));
    reset_fixture(); apply_native(); now += 1001;
    assert(!free_camera_available(epoch, false)); assert(free_camera_available(epoch, true));
    readable = false; assert(!free_camera_available(epoch, true));
    reset_fixture(); *(unsigned *)(manager + 0x20) = 65;
    apply_native(); assert(!free_camera_available(epoch, false));
    reset_fixture(); put_pointer(view, 0); apply_native(); assert(!free_camera_available(epoch, false));
    reset_fixture(); matrix_at(view)[8] = NAN;
    free_camera_apply((uintptr_t)view); assert(!free_camera_available(epoch, false));
    reset_fixture(); *(double *)(view + 0x60) = INFINITY;
    free_camera_apply((uintptr_t)view); assert(!free_camera_available(epoch, false));
    reset_fixture(); apply_native(); assert(update(neutral, false));
    pose.eye[0] = 100000;
    assert(!update((FreeCameraInput){.lx=32767}, false)); assert(!free_camera_active());
}

static void degenerate_native_restoration(void) {
    reset_fixture(); apply_native(); assert(update((FreeCameraInput){0}, false));
    assert(update((FreeCameraInput){.ly=32767}, false)); apply_native();
    assert(fabsf(matrix_at(view)[11] + 29.6f) < 0.001f);
    // Native's exact eye==target branch leaves the existing matrix in place.
    memcpy(view + 0x78, view + 0x60, 3 * sizeof(double));
    free_camera_reset();
    free_camera_apply((uintptr_t)view);
    assert(fabsf(matrix_at(view)[11] + 30.0f) < 0.001f);
    assert(!free_camera_active() && !free_camera_available(epoch, false));
    // Changed native bytes are no longer ours and must never be overwritten.
    reset_fixture(); apply_native(); assert(update((FreeCameraInput){0}, false));
    assert(update((FreeCameraInput){.ly=32767}, false)); apply_native();
    memcpy(view + 0x78, view + 0x60, 3 * sizeof(double));
    matrix_at(view)[11] = 123;
    free_camera_reset(); free_camera_apply((uintptr_t)view);
    assert(matrix_at(view)[11] == 123);
    // Reused view pointers in a newer save never authorize old restoration.
    reset_fixture(); apply_native(); assert(update((FreeCameraInput){0}, false));
    assert(update((FreeCameraInput){.ly=32767}, false)); apply_native();
    memcpy(view + 0x78, view + 0x60, 3 * sizeof(double));
    ++epoch; free_camera_apply((uintptr_t)view);
    assert(fabsf(matrix_at(view)[11] + 29.6f) < 0.001f && !free_camera_active());
}

static void active_view_survives_activation_sample_expiry(void) {
    reset_fixture(); apply_native(); assert(update((FreeCameraInput){0}, true));
    // Menu held for a minute; the first unpaused input poll precedes any new
    // view callback. Its original activation observation is now stale.
    now += 60000;
    assert(free_camera_available(epoch, false));
    assert(update((FreeCameraInput){0}, false));
    // Likewise, an in-progress observation copy cannot disable an owned view.
    observation.sequence |= 1;
    assert(free_camera_available(epoch, false));
    assert(update((FreeCameraInput){0}, false));
    observation.sequence += 1;
    // This does not relax activation, identity, epoch or world safety.
    assert(!free_camera_update(false, 0, epoch, true, false, &(FreeCameraInput){0}));
    assert(!free_camera_available(epoch, false));
    reset_fixture(); apply_native(); assert(update((FreeCameraInput){0}, false));
    assert(!free_camera_update(true, 0, epoch, true, false, &(FreeCameraInput){0}));
    assert(free_camera_evidence.reason == CAMERA_REASON_NO_ACTOR);
    free_camera_update(false, 0, epoch, true, false, &(FreeCameraInput){0});
    assert(free_camera_evidence.reason == CAMERA_REASON_NO_ACTOR);
}

int main(void) {
    normal_motion_and_restoration();
    rotation_clamp_and_time();
    interruptions_and_layout_guards();
    degenerate_native_restoration();
    active_view_survives_activation_sample_expiry();
    puts("Production Free Camera: scoped native view, input movement/rotation, pause/time bounds, restoration, invalid layouts and save/scene/cutscene interruption guards passed (host fixture).");
}
