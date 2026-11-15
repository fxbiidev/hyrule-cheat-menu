#include "recovery.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static unsigned clears, floors;
static bool wall, solid, void_floor, unavailable;
static int clear(void *context, const float p[3]) {
    assert(context == &clears); ++clears;
    if (unavailable) return -1;
    return !solid && (!wall || fabsf(p[0]) > 0.21f);
}
static int floor_at(void *context, const float p[3], float *height) {
    assert(context == &clears); ++floors;
    if (unavailable) return -1;
    if (void_floor) return 0;
    *height = p[0] > 1 ? 50 : 0.05f; // Reject an implausible upper surface.
    return 1;
}
int main(void) {
    RecoveryQueries q = {&clears, clear, floor_at};
    float origin[3] = {0}, out[3] = {99, 98, 97};
    assert(recovery_search(&q, origin, false, out) == RECOVERY_CURRENT);
    assert(!out[0] && !out[1] && !out[2] && clears == 1 && !floors);
    wall = true; clears = 0;
    assert(recovery_search(&q, origin, false, out) == RECOVERY_NEARBY);
    assert(fabsf(out[0]) > 0.21f && fabsf(out[0]) < 0.22f && !out[1] && !out[2]);
    assert(clears < 300);
    clears = floors = 0;
    assert(recovery_search(&q, origin, true, out) == RECOVERY_NEARBY);
    assert(fabsf(out[0]) > 0.21f && fabsf(out[0]) < 0.22f && out[1] == 0.05f);
    assert(clears < 100 && floors < 100);
    void_floor = true; out[0] = 99;
    assert(recovery_search(&q, origin, true, out) == RECOVERY_NONE && out[0] == 99);
    void_floor = false; solid = true; clears = 0;
    assert(recovery_search(&q, origin, false, out) == RECOVERY_NONE && clears < 300);
    unavailable = true;
    assert(recovery_search(&q, origin, false, out) == RECOVERY_UNAVAILABLE && out[0] == 99);
    unavailable = solid = wall = false;
    assert(recovery_search(&q, origin, true, out) == RECOVERY_CURRENT && out[1] == 0.05f);
    unsigned before = clears;
    origin[1] = NAN;
    assert(recovery_search(&q, origin, false, out) == RECOVERY_UNAVAILABLE && clears == before);
    assert(recovery_search(NULL, origin, false, out) == RECOVERY_UNAVAILABLE);
    puts("Recovery search: current clearance, nearest grid/refinement, ground, void, query failure and finite bounds passed.");
}
