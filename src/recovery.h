#pragma once
#include <stdbool.h>

typedef struct {
    void *context;
    // -1: query unavailable, 0: blocked/no floor, 1: clear/floor found.
    int (*clear)(void *, const float position[3]);
    int (*floor)(void *, const float position[3], float *root_height);
} RecoveryQueries;

typedef enum { RECOVERY_UNAVAILABLE = -1, RECOVERY_NONE, RECOVERY_CURRENT, RECOVERY_NEARBY } RecoveryResult;
RecoveryResult recovery_search(const RecoveryQueries *queries, const float origin[3],
                               bool grounded, float result[3]);
