#include "guest.h"
#include "profile.h"
#include "geometry.h"
#include "movement_safety.h"

typedef struct {
    uintptr_t actor, physics, set, handle, shape;
    unsigned epoch, layer;
    uint64_t filter;
    float rotation[9], bottom;
    bool intact;
} Geometry;

enum { READ_MEMORY = 1 };
static const uint64_t guard_a = 0x123456789abcdef0ull, guard_b = 0xfedcba9876543210ull;
extern void geometry_filter_native(const uintptr_t *, unsigned, void *, uintptr_t);

static bool current(Geometry *g) {
    if (!g->intact || save_epoch() != g->epoch || !world_safe() || !movement_ready()) return false;
    uintptr_t components = pointer(g->actor + 0x270);
    return pointer(g->handle + 0x28) == g->shape &&
           pointer(g->physics + 0x20) == g->set && pointer(g->set + 0x190) == g->handle &&
           components && pointer(components + 0x50) == g->physics;
}

static bool begin(uintptr_t actor, Geometry *g) {
    if (!actor || !world_safe() || !movement_ready() || !mapped(actor, 0x328, READ_MEMORY)) return false;
    if (pointer(game(0x84c9f610)) != game(0x849c53b0) ||
        pointer(game(0x84cd7580)) != game(0x84c51460)) return false;
    g->actor = actor;
    g->epoch = save_epoch();
    uintptr_t components = pointer(actor + 0x270);
    if (!mapped(components, 0x58, READ_MEMORY)) return false;
    g->physics = ((uintptr_t (*)(uintptr_t))game(GAME_geometry_body))(actor);
    if (!mapped(g->physics, 0x28, READ_MEMORY)) return false;
    uintptr_t set = pointer(g->physics + 0x20);
    if (!mapped(set, 0x198, READ_MEMORY)) return false;
    g->set = set;
    g->handle = pointer(set + 0x190);
    if (!mapped(g->handle, 0xb0, READ_MEMORY)) return false;
    g->shape = pointer(g->handle + 0x28);
    // This player shape was identified in the actual executable and runtime.
    // Other shape classes fall back to the existing recovery anchor.
    if (!mapped(g->shape, 0xf0, READ_MEMORY) || pointer(g->shape) != game(0x84c51a28) ||
        pointer(game(0x84c51a38)) != game(GAME_geometry_shape_type)) return false;
    float bounds[6];
    if (!((unsigned (*)(const uintptr_t *, float *, unsigned))game(GAME_geometry_bounds))(
            &g->physics, bounds, 0)) return false;
    for (unsigned i = 0; i < 3; ++i) {
        if (!__builtin_isfinite(bounds[i]) || !__builtin_isfinite(bounds[i + 3]) ||
            bounds[i + 3] - bounds[i] < 0.05f || bounds[i + 3] - bounds[i] > 4) return false;
    }
    g->bottom = bounds[1] - *(const float *)(g->handle + 0x9c);
    if (!__builtin_isfinite(g->bottom) || g->bottom < -0.5f || g->bottom > 0.5f) return false;
    for (unsigned y = 0; y < 3; ++y) for (unsigned x = 0; x < 3; ++x) {
        float v = ((const float *)(g->handle + 0x80))[y * 4 + x];
        if (!__builtin_isfinite(v) || v < -1.01f || v > 1.01f) return false;
        g->rotation[y * 3 + x] = v;
    }
    g->layer = ((unsigned (*)(const uintptr_t *, unsigned))game(GAME_geometry_layer))(&g->physics, 0);
    g->filter = 0;
    geometry_filter_native(&g->physics, 0, &g->filter, game(GAME_geometry_filter));
    g->intact = true;
    return current(g);
}

static int clear(void *context, const float p[3]) {
    Geometry *g = context;
    if (!current(g)) return -1;
    // Layout/lifetime independently traced from native SafeDrop's sphere query.
    // The temporary sphere supplies owned scratch storage; q+8 borrows Link's
    // full collision shape. The borrowed shape is never destroyed by this code.
    struct {
        volatile uint64_t head[2];
        _Alignas(16) unsigned char bytes[0xb00];
        volatile uint64_t tail[2];
    } scratch = {.head = {guard_a, guard_b}, .tail = {guard_a, guard_b}};
    uintptr_t q = (uintptr_t)scratch.bytes;
    ((void (*)(uintptr_t, uintptr_t, uintptr_t, unsigned, uintptr_t, unsigned, uintptr_t))
        game(GAME_geometry_query_init))(q, 0, 0, 14, 0, 1, 0);
    *(uintptr_t *)q = pointer(game(0x84c9f610)) + 0x10;
    ((void (*)(uintptr_t, float))game(GAME_geometry_sphere_init))(q + 0x6d0, 0.2f);
    uintptr_t owned = pointer(q + 0x6d8);
    *(uintptr_t *)(q + 8) = g->shape;
    for (unsigned i = 0; i < 3; ++i) ((float *)(q + 0x10))[i] = p[i];
    for (unsigned i = 0; i < 9; ++i) ((float *)(q + 0x28))[i] = g->rotation[i];
    *(uint16_t *)(q + 0x110) |= 4;
    ((void (*)(uintptr_t, unsigned))game(GAME_geometry_filter_bits))(q + 0x120, 32);
    *(uint64_t *)(q + 0x130) &= ~0xc000ull;
    ((void (*)(uintptr_t, const void *))game(GAME_geometry_filter_set))(q, &g->filter);
    *(unsigned *)(q + 0x128) = (*(unsigned *)(q + 0x128) & ~63u) | (g->layer & 63);
    unsigned hit = ((unsigned (*)(uintptr_t))game(GAME_geometry_overlap))(q);
    *(uintptr_t *)(q + 8) = owned;
    ((void (*)(uintptr_t))game(GAME_geometry_query_fini))(q);
    g->intact = scratch.head[0] == guard_a && scratch.head[1] == guard_b &&
                scratch.tail[0] == guard_a && scratch.tail[1] == guard_b;
    return current(g) ? !(hit & 1) : -1;
}

static int floor_at(void *context, const float p[3], float *height) {
    Geometry *g = context;
    if (!current(g)) return -1;
    // Owned, fixed-size native ray object. Its native destructor is a no-op.
    // Field initialization and filter binding match the traced native caller.
    struct {
        volatile uint64_t head[2];
        _Alignas(16) unsigned char bytes[0x180];
        volatile uint64_t tail[2];
    } scratch = {.head = {guard_a, guard_b}, .tail = {guard_a, guard_b}};
    uintptr_t q = (uintptr_t)scratch.bytes;
    *(uintptr_t *)q = game(0x8499fba0);
    *(float *)(q + 0x3c) = -1;
    *(unsigned *)(q + 0x90) = 1;
    *(unsigned *)(q + 0x114) = 1;
    *(uintptr_t *)(q + 0x60) = q + 0x128;
    *(uintptr_t *)(q + 0x128) = pointer(game(0x84cd7580)) + 0x10;
    *(unsigned *)(q + 0x130) = 14;
    *(uint64_t *)(q + 0x138) = *(uint64_t *)(q + 0x148) = 0x3fffff;
    *(uint64_t *)(q + 0x140) = *(uint64_t *)(q + 0x150) = 0x07ffffffeeffffffull;
    *(uint64_t *)(q + 0x158) = 1;
    ((void (*)(uintptr_t, unsigned))game(GAME_geometry_filter_bits))(q + 0x128, 32);
    *(uint64_t *)(q + 0x138) &= ~0xc000ull;
    ((void (*)(uintptr_t, const void *))game(GAME_geometry_ray_filter))(q, &g->filter);
    *(unsigned *)(q + 0x130) = (*(unsigned *)(q + 0x130) & ~63u) | (g->layer & 63);
    for (unsigned i = 0; i < 3; ++i) ((float *)(q + 8))[i] = ((float *)(q + 0x14))[i] = p[i];
    *(float *)(q + 0xc) += 0.4f;
    *(float *)(q + 0x18) -= 1.5f;
    unsigned hit = ((unsigned (*)(uintptr_t))game(GAME_geometry_ray))(q);
    g->intact = scratch.head[0] == guard_a && scratch.head[1] == guard_b &&
                scratch.tail[0] == guard_a && scratch.tail[1] == guard_b;
    if (!current(g)) return -1;
    if (!(hit & 1)) return 0;
    const float *point = (const float *)(q + 0x24), *normal = (const float *)(q + 0x30);
    float norm = 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (!__builtin_isfinite(point[i]) || !__builtin_isfinite(normal[i])) return -1;
        norm += normal[i] * normal[i];
    }
    if (normal[1] < 0.65f || norm < 0.5f || norm > 1.5f ||
        point[1] > p[1] + 0.401f || point[1] < p[1] - 1.501f) return 0;
    *height = point[1] + GEOMETRY_FLOOR_CLEARANCE - g->bottom;
    return 1;
}

RecoveryResult geometry_recover(uintptr_t actor, bool grounded, float result[3]) {
    Geometry g = {0};
    if (!begin(actor, &g)) return RECOVERY_UNAVAILABLE;
    float origin[3], found[3];
    for (unsigned i = 0; i < 3; ++i) origin[i] = ((const float *)(actor + 0x31c))[i];
    RecoveryQueries queries = {&g, clear, floor_at};
    RecoveryResult status = recovery_search(&queries, origin, grounded, found);
    if (status <= RECOVERY_NONE || !current(&g)) return RECOVERY_UNAVAILABLE;
    for (unsigned i = 0; i < 3; ++i) result[i] = found[i];
    return status;
}

int geometry_floor(uintptr_t actor, const float position[3], float *root_height) {
    Geometry g = {0};
    if (!position || !root_height || !begin(actor, &g)) return -1;
    for (unsigned i = 0; i < 3; ++i)
        if (!__builtin_isfinite(position[i]) || position[i] < -100000 || position[i] > 100000) return -1;
    return floor_at(&g, position, root_height);
}
