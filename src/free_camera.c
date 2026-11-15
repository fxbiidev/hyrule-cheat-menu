#include "guest.h"
#include "profile.h"
#include "movement_safety.h"
#include "free_camera.h"

// Signature-verified native view construction uses these manager/view layouts.
// Keep all camera-owned state in this module, never in game save/body memory.
enum { CAMERA_MANAGER_SLOT = 0x84ca6bc0ull, CAMERA_VIEW_VTABLE = 0x84c18a88ull };
uintptr_t free_camera_hook_resume;
FreeCameraEvidence free_camera_evidence;

static unsigned active, interrupted, owner_epoch;
static uintptr_t owner_view;
static struct {
    unsigned sequence, epoch;
    uintptr_t view;
    uint64_t seen, eye[3];
    uint32_t back[3];
} observation;
static struct { unsigned sequence; uint32_t matrix[12]; } output;
// Only the native view hook accesses this fallback. A degenerate native
// eye==target path skips matrix construction; restore only bytes we still own
// on the currently identified view in the same save, never a cached raw pointer.
static struct {
    uintptr_t view;
    unsigned epoch;
    bool valid;
    float native[12], written[12];
} applied;
// Only the input thread accesses the integration state. Reset/save hooks only
// invalidate atomic publications; they never race a floating-point pose write.
static struct {
    bool valid;
    uintptr_t actor, view;
    unsigned epoch;
    uint64_t last;
    double eye[3];
    float back[3];
} pose;

static uintptr_t gameplay_view(void) {
    uintptr_t slot = game(CAMERA_MANAGER_SLOT);
    if (!mapped(slot, sizeof(uintptr_t), 1)) return 0;
    uintptr_t holder = pointer(slot);
    if (!mapped(holder, sizeof(uintptr_t), 1)) return 0;
    uintptr_t manager = pointer(holder);
    if (!mapped(manager, 0x30, 1)) return 0;
    unsigned count = *(const unsigned *)(manager + 0x20);
    uintptr_t cameras = pointer(manager + 0x28);
    if (!count || count > 64 || !mapped(cameras, sizeof(uintptr_t), 1)) return 0;
    uintptr_t camera = pointer(cameras);
    if (!mapped(camera, 0x80, 1)) return 0;
    uintptr_t view = pointer(camera + 0x78);
    if (!mapped(view, 0x90, 1) || !mapped(view + 8, 48, 3) ||
        pointer(view) != game(CAMERA_VIEW_VTABLE)) return 0;
    return view;
}

static bool finite_eye(const double eye[3]) {
    for (unsigned i = 0; i < 3; ++i)
        if (!__builtin_isfinite(eye[i]) || eye[i] < -100000 || eye[i] > 100000) return false;
    return true;
}

static bool finite_matrix(const float matrix[12]) {
    for (unsigned i = 0; i < 12; ++i)
        if (!__builtin_isfinite(matrix[i])) return false;
    for (unsigned row = 0; row < 3; ++row) {
        const float *axis = matrix + row * 4;
        float length = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
        if (length < 0.8f || length > 1.2f) return false;
        for (unsigned other = row + 1; other < 3; ++other) {
            const float *b = matrix + other * 4;
            float dot = axis[0] * b[0] + axis[1] * b[1] + axis[2] * b[2];
            if (dot < -0.1f || dot > 0.1f) return false;
        }
    }
    return true;
}

static bool native_source_valid(uintptr_t view) {
    const double *eye = (const double *)(view + 0x60);
    const double *target = (const double *)(view + 0x78);
    if (!finite_eye(eye) || !finite_eye(target)) return false;
    return eye[0] != target[0] || eye[1] != target[1] || eye[2] != target[2];
}

static void restore_owned_degenerate(uintptr_t view, unsigned epoch) {
    if (applied.valid && applied.view == view && applied.epoch == epoch) {
        const float *matrix = (const float *)(view + 8);
        bool owned = true;
        for (unsigned i = 0; i < 12; ++i) if (matrix[i] != applied.written[i]) owned = false;
        if (owned && epoch == save_epoch())
            for (unsigned i = 0; i < 12; ++i) ((float *)(view + 8))[i] = applied.native[i];
    }
    applied.valid = false;
}

static void evidence(enum FreeCameraReason reason, unsigned stage, uintptr_t actor,
                     unsigned epoch, uintptr_t view) {
    unsigned expected = CAMERA_REASON_NONE;
    if (!__atomic_compare_exchange_n(&free_camera_evidence.reason, &expected, reason,
            false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) return;
    __atomic_store_n(&free_camera_evidence.stage, stage, __ATOMIC_RELAXED);
    __atomic_store_n(&free_camera_evidence.actor, actor, __ATOMIC_RELAXED);
    __atomic_store_n(&free_camera_evidence.view, view, __ATOMIC_RELAXED);
    __atomic_store_n(&free_camera_evidence.expected_epoch, epoch, __ATOMIC_RELAXED);
    __atomic_store_n(&free_camera_evidence.current_epoch, save_epoch(), __ATOMIC_RELAXED);
    __atomic_store_n(&free_camera_evidence.when, monotonic_ms(), __ATOMIC_RELEASE);
}

static void stop(bool latch) {
    if (__atomic_exchange_n(&active, 0, __ATOMIC_ACQ_REL)) {
        __atomic_add_fetch(&free_camera_evidence.stops, 1, __ATOMIC_RELAXED);
        debug_log("[hyrule] Free Camera released; native view resumes\n");
    }
    if (latch) __atomic_store_n(&interrupted, 1, __ATOMIC_RELEASE);
}

void free_camera_reset(void) {
    if (__atomic_load_n(&active, __ATOMIC_ACQUIRE))
        evidence(CAMERA_REASON_RESET, 0, 0, save_epoch(), 0);
    stop(true);
}

void free_camera_init(void) {
    free_camera_hook_resume = game(ORIGINAL_free_camera_hook);
    __atomic_store_n(&active, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&interrupted, 0, __ATOMIC_RELEASE);
}

bool free_camera_active(void) {
    return __atomic_load_n(&active, __ATOMIC_ACQUIRE) &&
           !__atomic_load_n(&interrupted, __ATOMIC_ACQUIRE) &&
           __atomic_load_n(&owner_epoch, __ATOMIC_RELAXED) == save_epoch();
}

typedef struct { uintptr_t view; unsigned epoch; uint64_t seen; double eye[3]; float back[3]; } NativePose;

static bool read_observation(NativePose *native) {
    unsigned before = __atomic_load_n(&observation.sequence, __ATOMIC_ACQUIRE);
    if (before & 1) return false;
    native->view = __atomic_load_n(&observation.view, __ATOMIC_RELAXED);
    native->epoch = __atomic_load_n(&observation.epoch, __ATOMIC_RELAXED);
    native->seen = __atomic_load_n(&observation.seen, __ATOMIC_RELAXED);
    for (unsigned i = 0; i < 3; ++i) {
        union { uint64_t bits; double value; } d;
        union { uint32_t bits; float value; } f;
        d.bits = __atomic_load_n(&observation.eye[i], __ATOMIC_RELAXED);
        f.bits = __atomic_load_n(&observation.back[i], __ATOMIC_RELAXED);
        native->eye[i] = d.value; native->back[i] = f.value;
    }
    return before == __atomic_load_n(&observation.sequence, __ATOMIC_ACQUIRE) && native->view;
}

static bool usable_observation(unsigned epoch, bool paused, NativePose *native) {
    if (epoch != save_epoch() || !world_safe() || !movement_ready() ||
        !read_observation(native) || native->epoch != epoch ||
        native->view != gameplay_view() || !native_source_valid(native->view) ||
        !finite_eye(native->eye)) return false;
    uint64_t now = monotonic_ms();
    return native->seen && now >= native->seen && (paused || now - native->seen <= 1000) &&
           epoch == save_epoch();
}

bool free_camera_available(unsigned epoch, bool paused) {
    // Fresh observation is an activation precondition, not an ongoing lease.
    // A paused menu can outlive that observation, and its first resumed input
    // poll may precede native view construction. An already-owned valid view
    // must not disappear because that activation sample is old or being copied.
    if (__atomic_load_n(&active, __ATOMIC_ACQUIRE)) {
        enum FreeCameraReason reason = CAMERA_REASON_NONE;
        uintptr_t view = gameplay_view();
        if (epoch != save_epoch() || __atomic_load_n(&owner_epoch, __ATOMIC_RELAXED) != epoch)
            reason = CAMERA_REASON_SAVE;
        else if (__atomic_load_n(&interrupted, __ATOMIC_ACQUIRE)) reason = CAMERA_REASON_INTERRUPTED;
        else if (!world_safe()) reason = CAMERA_REASON_WORLD;
        else if (!movement_ready()) reason = CAMERA_REASON_LOADING;
        else if (!view || view != __atomic_load_n(&owner_view, __ATOMIC_RELAXED)) reason = CAMERA_REASON_VIEW;
        else if (!native_source_valid(view)) reason = CAMERA_REASON_NATIVE_SOURCE;
        if (reason != CAMERA_REASON_NONE) evidence(reason, 1, 0, epoch, view);
        return reason == CAMERA_REASON_NONE && epoch == save_epoch();
    }
    NativePose native;
    return usable_observation(epoch, paused, &native);
}

static float stick(int32_t input) {
    float x = (float)input / 32767.0f;
    if (x > 1) x = 1;
    if (x < -1) x = -1;
    float a = __builtin_fabsf(x);
    if (a <= 0.15f) return 0;
    return (x < 0 ? -1 : 1) * ((a - 0.15f) / 0.85f);
}

static void axes(const float back[3], float right[3], float up[3]) {
    float flat = __builtin_sqrtf(back[0] * back[0] + back[2] * back[2]);
    right[0] = back[2] / flat; right[1] = 0; right[2] = -back[0] / flat;
    up[0] = -back[1] * back[0] / flat; up[1] = flat; up[2] = -back[1] * back[2] / flat;
}

// Angles are bounded to 0.12 rad per step. These small-angle polynomials avoid
// external math-library dependencies; normalisation removes accumulated drift.
static void rotation(float angle, float *sine, float *cosine) {
    float q = angle * angle;
    *sine = angle * (1 - q / 6 + q * q / 120);
    *cosine = 1 - q / 2 + q * q / 24;
}

static bool advance(float dt, const FreeCameraInput *input) {
    float sy, cy, sp, cp;
    rotation(-stick(input->rx) * 2.4f * dt, &sy, &cy);
    rotation(stick(input->ry) * 2.4f * dt, &sp, &cp);
    float bx = cy * pose.back[0] + sy * pose.back[2];
    float bz = -sy * pose.back[0] + cy * pose.back[2];
    float flat = __builtin_sqrtf(bx * bx + bz * bz);
    float by = pose.back[1] * cp - flat * sp;
    if (by > 0.985f) by = 0.985f;
    if (by < -0.985f) by = -0.985f;
    if (!__builtin_isfinite(flat) || flat < 0.0001f) return false;
    float ratio = __builtin_sqrtf(1 - by * by) / flat;
    pose.back[0] = bx * ratio; pose.back[1] = by; pose.back[2] = bz * ratio;
    float right[3], up[3];
    axes(pose.back, right, up);
    float x = stick(input->lx), y = stick(input->ly);
    float radius = __builtin_sqrtf(x * x + y * y);
    if (radius > 1) { x /= radius; y /= radius; }
    float rise = (input->buttons & (1u << 9) ? 1.0f : 0) -
                 (input->buttons & (1u << 8) ? 1.0f : 0);
    for (unsigned i = 0; i < 3; ++i)
        pose.eye[i] += (double)(right[i] * x - pose.back[i] * y + (i == 1 ? rise : 0)) * 8 * dt;
    return finite_eye(pose.eye);
}

static void publish_matrix(void) {
    float right[3], up[3], matrix[12];
    axes(pose.back, right, up);
    const float *basis[] = {right, up, pose.back};
    for (unsigned row = 0; row < 3; ++row) {
        double translation = 0;
        for (unsigned i = 0; i < 3; ++i) {
            matrix[row * 4 + i] = basis[row][i];
            translation -= (double)basis[row][i] * pose.eye[i];
        }
        matrix[row * 4 + 3] = (float)translation;
    }
    __atomic_add_fetch(&output.sequence, 1, __ATOMIC_ACQ_REL);
    for (unsigned i = 0; i < 12; ++i) {
        union { float value; uint32_t bits; } f = {.value = matrix[i]};
        __atomic_store_n(&output.matrix[i], f.bits, __ATOMIC_RELAXED);
    }
    __atomic_add_fetch(&output.sequence, 1, __ATOMIC_RELEASE);
}

bool free_camera_update(bool requested, uintptr_t actor, unsigned epoch,
                        bool allowed, bool paused, const FreeCameraInput *input) {
    if (!requested) {
        if (__atomic_load_n(&active, __ATOMIC_ACQUIRE))
            evidence(CAMERA_REASON_OFF, 2, actor, epoch, 0);
        stop(false);
        __atomic_store_n(&interrupted, 0, __ATOMIC_RELEASE);
        pose.valid = false; pose.last = 0;
        return false;
    }
    enum FreeCameraReason reason = CAMERA_REASON_NONE;
    if (epoch != save_epoch()) reason = CAMERA_REASON_SAVE;
    else if (!actor) reason = CAMERA_REASON_NO_ACTOR;
    else if (!input) reason = CAMERA_REASON_NO_INPUT;
    else if (__atomic_load_n(&interrupted, __ATOMIC_ACQUIRE)) reason = CAMERA_REASON_INTERRUPTED;
    else if (!allowed) reason = CAMERA_REASON_NOT_ALLOWED;
    else if (!world_safe()) reason = CAMERA_REASON_WORLD;
    else if (!movement_ready()) reason = CAMERA_REASON_LOADING;
    if (reason != CAMERA_REASON_NONE) {
        evidence(reason, 2, actor, epoch, 0);
        stop(true); pose.valid = false; pose.last = 0;
        return false;
    }
    uintptr_t current_view = gameplay_view();
    if (pose.valid && (pose.epoch != epoch || pose.actor != actor || pose.view != current_view)) {
        evidence(pose.epoch != epoch ? CAMERA_REASON_SAVE : CAMERA_REASON_VIEW, 2, actor, epoch, current_view);
        stop(true); pose.valid = false; pose.last = 0;
        return false;
    }
    if (!pose.valid) {
        NativePose native;
        if (!usable_observation(epoch, paused, &native)) {
            evidence(CAMERA_REASON_OBSERVATION, 2, actor, epoch, current_view);
            return false;
        }
        float length = 0;
        for (unsigned i = 0; i < 3; ++i) length += native.back[i] * native.back[i];
        if (!__builtin_isfinite(length) || length < 0.8f || length > 1.2f) {
            evidence(CAMERA_REASON_POSE, 2, actor, epoch, current_view);
            return false;
        }
        float inverse = 1.0f / __builtin_sqrtf(length);
        for (unsigned i = 0; i < 3; ++i) {
            pose.eye[i] = native.eye[i]; pose.back[i] = native.back[i] * inverse;
        }
        // Keep a stable no-roll basis even when the native view is almost vertical.
        if (__builtin_fabsf(pose.back[1]) > 0.985f) {
            float flat = __builtin_sqrtf(pose.back[0] * pose.back[0] + pose.back[2] * pose.back[2]);
            if (flat < 0.0001f) { pose.back[0] = 0; pose.back[2] = 1; flat = 1; }
            pose.back[1] = pose.back[1] < 0 ? -0.985f : 0.985f;
            float scale = __builtin_sqrtf(1 - pose.back[1] * pose.back[1]) / flat;
            pose.back[0] *= scale; pose.back[2] *= scale;
        }
        pose.actor = actor; pose.view = native.view; pose.epoch = epoch;
        pose.valid = true; pose.last = 0;
    }
    uint64_t now = monotonic_ms();
    float dt = pose.last && now >= pose.last ? (float)(now - pose.last) * 0.001f : 0;
    pose.last = now;
    if (dt > 0.05f) dt = 0.05f;
    if (!paused && !advance(dt, input)) {
        evidence(CAMERA_REASON_POSE, 2, actor, epoch, current_view);
        stop(true); pose.valid = false; return false;
    }
    publish_matrix();
    if (epoch != save_epoch() || __atomic_load_n(&interrupted, __ATOMIC_ACQUIRE)) {
        evidence(epoch != save_epoch() ? CAMERA_REASON_SAVE : CAMERA_REASON_INTERRUPTED, 2, actor, epoch, current_view);
        stop(true); pose.valid = false; return false;
    }
    __atomic_store_n(&owner_view, pose.view, __ATOMIC_RELAXED);
    __atomic_store_n(&owner_epoch, epoch, __ATOMIC_RELAXED);
    if (!__atomic_load_n(&active, __ATOMIC_ACQUIRE))
        __atomic_store_n(&free_camera_evidence.reason, CAMERA_REASON_NONE, __ATOMIC_RELEASE);
    if (!__atomic_exchange_n(&active, 1, __ATOMIC_RELEASE))
        debug_log("[hyrule] Free Camera active; gameplay view matrix scoped\n");
    return free_camera_active();
}

void free_camera_apply(uintptr_t view) {
    unsigned epoch = save_epoch();
    uintptr_t current_view = gameplay_view();
    bool selected = __atomic_load_n(&active, __ATOMIC_ACQUIRE) != 0;
    if (selected && (__atomic_load_n(&owner_epoch, __ATOMIC_RELAXED) != epoch ||
                     __atomic_load_n(&owner_view, __ATOMIC_RELAXED) != current_view)) {
        evidence(__atomic_load_n(&owner_epoch, __ATOMIC_RELAXED) != epoch ? CAMERA_REASON_SAVE : CAMERA_REASON_VIEW,
            3, 0, epoch, current_view);
        stop(true);
    }
    if (!current_view || view != current_view) return;
    const float *native_matrix = (const float *)(view + 8);
    const double *native_eye = (const double *)(view + 0x60);
    if (!native_source_valid(view)) {
        if (selected) { evidence(CAMERA_REASON_NATIVE_SOURCE, 3, 0, epoch, view); stop(true); }
        restore_owned_degenerate(view, epoch);
        return;
    }
    if (!finite_eye(native_eye) || !finite_matrix(native_matrix)) {
        if (selected) { evidence(CAMERA_REASON_NATIVE_MATRIX, 3, 0, epoch, view); stop(true); }
        return;
    }
    // Observe the native camera before our override. Eye/target/source state is
    // untouched, so native camera construction continues normally underneath.
    __atomic_add_fetch(&observation.sequence, 1, __ATOMIC_ACQ_REL);
    __atomic_store_n(&observation.view, view, __ATOMIC_RELAXED);
    __atomic_store_n(&observation.epoch, epoch, __ATOMIC_RELAXED);
    __atomic_store_n(&observation.seen, monotonic_ms(), __ATOMIC_RELAXED);
    for (unsigned i = 0; i < 3; ++i) {
        union { double value; uint64_t bits; } d = {.value = native_eye[i]};
        union { float value; uint32_t bits; } f = {.value = native_matrix[8 + i]};
        __atomic_store_n(&observation.eye[i], d.bits, __ATOMIC_RELAXED);
        __atomic_store_n(&observation.back[i], f.bits, __ATOMIC_RELAXED);
    }
    __atomic_add_fetch(&observation.sequence, 1, __ATOMIC_RELEASE);
    if (!free_camera_active()) { applied.valid = false; return; }
    bool ready_world = world_safe();
    if (!ready_world || !movement_ready()) {
        evidence(ready_world ? CAMERA_REASON_LOADING : CAMERA_REASON_WORLD, 3, 0, epoch, view);
        stop(true); applied.valid = false; return;
    }
    unsigned before = __atomic_load_n(&output.sequence, __ATOMIC_ACQUIRE);
    if (before & 1) return;
    float matrix[12];
    for (unsigned i = 0; i < 12; ++i) {
        union { uint32_t bits; float value; } f;
        f.bits = __atomic_load_n(&output.matrix[i], __ATOMIC_RELAXED); matrix[i] = f.value;
    }
    if (before != __atomic_load_n(&output.sequence, __ATOMIC_ACQUIRE) ||
        !free_camera_active() || epoch != save_epoch()) return;
    for (unsigned i = 0; i < 12; ++i) {
        applied.native[i] = native_matrix[i]; applied.written[i] = matrix[i];
    }
    applied.view = view; applied.epoch = epoch; applied.valid = true;
    for (unsigned i = 0; i < 12; ++i) ((float *)(view + 8))[i] = matrix[i];
}

