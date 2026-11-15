#include "recovery.h"
#include <stddef.h>

static bool finite_position(const float p[3]) {
    if (!p) return false;
    for (unsigned i = 0; i < 3; ++i)
        if (!__builtin_isfinite(p[i]) || p[i] < -100000 || p[i] > 100000) return false;
    return true;
}

static int candidate(const RecoveryQueries *q, const float p[3], bool grounded, float out[3]) {
    for (unsigned i = 0; i < 3; ++i) out[i] = p[i];
    if (grounded) {
        int floor = q->floor(q->context, p, &out[1]);
        if (floor <= 0) return floor;
        if (!finite_position(out) || out[1] - p[1] > 0.5f || p[1] - out[1] > 1.5f) return 0;
    }
    return q->clear(q->context, out);
}

RecoveryResult recovery_search(const RecoveryQueries *q, const float origin[3],
                               bool grounded, float result[3]) {
    if (!q || !q->clear || (grounded && !q->floor) || !result || !finite_position(origin))
        return RECOVERY_UNAVAILABLE;
    float best[3];
    int valid = candidate(q, origin, grounded, best);
    if (valid < 0) return RECOVERY_UNAVAILABLE;
    if (valid) {
        for (unsigned i = 0; i < 3; ++i) result[i] = best[i];
        return RECOVERY_CURRENT;
    }
    // Increasing-distance half-metre grid inside a two-metre sphere. This is a
    // bounded local search, followed by clearance refinement, not a global map.
    float best_distance = 1000;
    bool found = false;
    for (int square = 1; square <= 16; ++square) {
        if (!grounded && found) break;
        for (int y = grounded ? 0 : -4; y <= (grounded ? 0 : 4); ++y)
            for (int z = -4; z <= 4; ++z) for (int x = -4; x <= 4; ++x) {
                if (x * x + y * y + z * z != square) continue;
                float p[3] = {origin[0] + x * 0.5f, origin[1] + y * 0.5f, origin[2] + z * 0.5f};
                float checked[3];
                valid = candidate(q, p, grounded, checked);
                if (valid < 0) return RECOVERY_UNAVAILABLE;
                if (!valid) continue;
                float distance = 0;
                for (unsigned i = 0; i < 3; ++i) {
                    float d = checked[i] - origin[i]; distance += d * d;
                }
                if (distance < best_distance && distance <= 4.01f) {
                    for (unsigned i = 0; i < 3; ++i) best[i] = checked[i];
                    best_distance = distance;
                    found = true;
                }
            }
    }
    if (!found) return RECOVERY_NONE;
    float blocked[3];
    for (unsigned i = 0; i < 3; ++i) blocked[i] = origin[i];
    // Every retained endpoint is independently queried. Thin/disjoint obstacles
    // need not be monotonic along the segment for the returned endpoint to be clear.
    for (unsigned pass = 0; pass < 7; ++pass) {
        float p[3], checked[3];
        for (unsigned i = 0; i < 3; ++i) p[i] = (blocked[i] + best[i]) * 0.5f;
        valid = candidate(q, p, grounded, checked);
        if (valid < 0) return RECOVERY_UNAVAILABLE;
        for (unsigned i = 0; i < 3; ++i) {
            if (valid) best[i] = checked[i];
            else blocked[i] = p[i];
        }
    }
    // A moving obstacle may have changed since the first query.
    if (q->clear(q->context, best) != 1) return RECOVERY_UNAVAILABLE;
    for (unsigned i = 0; i < 3; ++i) result[i] = best[i];
    return RECOVERY_NEARBY;
}
