// Shared by the ARM64 integration and portable resource regression fixture.
static uintptr_t magic_owner;
// Actor address and selected step share one atomic publication.
static uintptr_t speed_owner;
static unsigned magic_generation, speed_generation, life_generation;

static void clear_life_guard(void) {
    __atomic_store_n((uintptr_t *)game(LIFE_SLOT), 0, __ATOMIC_RELEASE);
}

static void publish_life_guard(uintptr_t current, unsigned generation) {
    if (generation != save_epoch()) return;
    __atomic_store_n(&life_generation, generation, __ATOMIC_RELAXED);
    __atomic_store_n((uintptr_t *)game(LIFE_SLOT), current ? current + 8 : 0, __ATOMIC_RELEASE);
    // The native CAS bridge reads the pointer slot directly. If a reset crossed
    // this publication, revoke it before this input invocation can continue.
    if (generation != save_epoch()) clear_life_guard();
}

static void refill_life(uintptr_t life, unsigned generation) {
    if (generation != save_epoch()) return;
    uintptr_t current = pointer(life + 0x1c10);
    uintptr_t maximum = pointer(life + 0x1c18);
    if (!current || !maximum) return;
    unsigned cap = __atomic_load_n((unsigned *)(maximum + 8), __ATOMIC_RELAXED);
    unsigned *value = (unsigned *)(current + 8);
    unsigned before = __atomic_load_n(value, __ATOMIC_RELAXED);
    if (generation == save_epoch() && before && before < cap && cap <= 80)
        __atomic_compare_exchange_n(value, &before, cap, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}

static void publish_speed(uintptr_t state, unsigned generation) {
    if (generation != save_epoch()) return;
    __atomic_store_n(&speed_generation, generation, __ATOMIC_RELAXED);
    __atomic_store_n(&speed_owner, state, __ATOMIC_RELEASE);
    if (generation != save_epoch()) __atomic_store_n(&speed_owner, 0, __ATOMIC_RELEASE);
}

static uintptr_t current_speed(unsigned *generation) {
    uintptr_t state = __atomic_load_n(&speed_owner, __ATOMIC_ACQUIRE);
    *generation = __atomic_load_n(&speed_generation, __ATOMIC_RELAXED);
    return *generation == save_epoch() ? state : 0;
}

static bool speed_current(uintptr_t state, unsigned generation) {
    return generation == save_epoch() && state == __atomic_load_n(&speed_owner, __ATOMIC_ACQUIRE);
}

static uintptr_t character_handle(uintptr_t actor) {
    uintptr_t components = pointer(actor + 0x270);
    uintptr_t physics = components ? pointer(components + 0x50) : 0;
    uintptr_t controller = physics ? pointer(physics + 0x20) : 0;
    return controller ? pointer(controller + 0x10) : 0;
}

static float movement_multiplier(uintptr_t state) {
    return movement_speed_factor(state & 3);
}

static void ground_motion(uintptr_t component, const float direction[3], float step_distance) {
    unsigned generation;
    uintptr_t state = current_speed(&generation);
    if (state && pointer(component + 0x18) == (state & ~(uintptr_t)3)) {
        float factor = noclip_native_ground_factor(state & ~(uintptr_t)3, state & 3);
        if (speed_current(state, generation)) step_distance *= factor;
    }
    ((void (*)(uintptr_t, const float *, float))game(GAME_ground_velocity))(
        component, direction, step_distance);
}

static void swim_motion(uintptr_t actor, void *history, unsigned mode, unsigned special,
                        float target, float step) {
    unsigned generation;
    uintptr_t state = current_speed(&generation);
    if (state && actor == (state & ~(uintptr_t)3) && speed_current(state, generation))
        target *= movement_multiplier(state);
    ((void (*)(uintptr_t, void *, unsigned, unsigned, float, float))game(GAME_swim_velocity))(
        actor, history, mode, special, target, step);
}

static void climb_motion(uintptr_t controller, void *request) {
    void (*submit)(uintptr_t, const void *) = (void (*)(uintptr_t, const void *))game(GAME_climb_velocity);
    unsigned generation;
    uintptr_t state = current_speed(&generation);
    if (!state) {
        submit(controller, request);
        return;
    }
    uintptr_t owner = pointer((uintptr_t)request);
    uintptr_t handle = character_handle(state & ~(uintptr_t)3);
    unsigned mode = *(const unsigned *)((uintptr_t)request + 0x18);
    if (handle != owner || !handle || (mode != 0 && mode != 1 && mode != 4) ||
        !speed_current(state, generation)) {
        submit(controller, request);
        return;
    }
    // The request is a private stack buffer, copied synchronously by the controller.
    // Scale the input translation at +0x1c. Leave time, mode and contact parameters intact.
    float *velocity = (float *)((uintptr_t)request + 0x1c);
    float original[3] = {velocity[0], velocity[1], velocity[2]};
    float factor = movement_multiplier(state);
    for (unsigned i = 0; i < 3; ++i) velocity[i] *= factor;
    submit(controller, request);
    for (unsigned i = 0; i < 3; ++i) velocity[i] = original[i];
}

static void write_pouch_stat(uintptr_t pouch, uintptr_t data, int value, uint32_t field) {
    ((void (*)(uintptr_t, int, uintptr_t, uint32_t))game(GAME_write_struct_int))(
        data, value, pouch + 0x3bc0, field);
}

static bool magic_levels(uintptr_t pouch, uintptr_t data, int *amount, int *cap,
                         unsigned generation) {
    if (generation != save_epoch() || !pouch || !data) return false;
    typedef unsigned (*Read)(uintptr_t, int *, uintptr_t, uint32_t);
    Read read_int = (Read)game(GAME_read_struct_int);
    uintptr_t stats = pouch + 0x3bc0;
    return read_int(data, cap, stats, 0x39e8a20d) && generation == save_epoch() &&
           *cap > 0 && *cap <= 480 && read_int(data, amount, stats, 0xcbca2f4d) &&
           generation == save_epoch() && *amount >= 0 && *amount <= *cap;
}

static void consume_magic(uintptr_t counter, const int *cost) {
    uintptr_t owner = __atomic_load_n(&magic_owner, __ATOMIC_ACQUIRE);
    unsigned generation = __atomic_load_n(&magic_generation, __ATOMIC_RELAXED);
    if (owner && counter == owner && generation == save_epoch() && cost[0] > 0 &&
        owner == __atomic_load_n(&magic_owner, __ATOMIC_ACQUIRE) && generation == save_epoch()) return;
    ((void (*)(uintptr_t, const int *))game(ORIGINAL_consume_magic))(counter, cost);
}

static void fill_magic(uintptr_t life, uintptr_t pouch, uintptr_t data, int amount, int cap,
                       unsigned generation) {
    if (generation != save_epoch()) return;
    uintptr_t physics = pointer(life + 0x18);
    uintptr_t component = ((uintptr_t (*)(uintptr_t))game(GAME_magic_component))(physics);
    if (generation != save_epoch()) return;
    if (!mapped(component, 0x1bc0, READ) || pointer(component) != game(0x84B6DFC0) ||
        pointer(component + 0x18) != physics) {
        __atomic_store_n(&magic_owner, 0, __ATOMIC_RELEASE);
        return;
    }
    if (generation != save_epoch()) return;
    __atomic_store_n(&magic_generation, generation, __ATOMIC_RELAXED);
    __atomic_store_n(&magic_owner, component + 0x1b78, __ATOMIC_RELEASE);
    // The game keeps a live meter and mirrors it to GameData. Queue a native heal for the
    // live meter once, and update the GameData balance while the actor is paused.
    int live = __atomic_load_n((const int *)(component + 0x1bbc), __ATOMIC_RELAXED);
    int pending = __atomic_load_n((const int *)(component + 0x1b90), __ATOMIC_RELAXED);
    if (generation != save_epoch()) return;
    if (live >= 0 && live < cap && pending <= 0)
        ((void (*)(uintptr_t))game(GAME_heal_magic))(component);
    if (generation == save_epoch() && amount < cap)
        write_pouch_stat(pouch, data, cap, 0xcbca2f4d);
}

