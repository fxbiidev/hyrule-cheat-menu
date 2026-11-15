#pragma once
#include "menu.h"

typedef struct {
    char scene[80], spawn[80];
    float position[3];
    unsigned age;
    bool valid;
} TravelAnchor;

bool travel_capture(uintptr_t actor, TravelAnchor *anchor);
bool travel_same_scene(const TravelAnchor *anchor);
bool travel_recover(Menu *menu, const TravelAnchor *anchor);
typedef enum {
    TRAVEL_RECOVERY_NONE, TRAVEL_RECOVERY_PENDING,
    TRAVEL_RECOVERY_ACCEPTED, TRAVEL_RECOVERY_FAILED
} TravelRecoveryState;
// Recovery is queued first; callers retain their safe hold until acceptance.
// Results from another save epoch are never exposed to the current player.
TravelRecoveryState travel_recovery_state(unsigned expected_epoch);
bool travel_prepare_source(uint32_t request, const TravelAnchor *anchor);

void travel_reset(void);
bool travel_busy(void);
bool travel_holds_saves(void);
bool travel_healthy(void);
bool player_on_ground(uintptr_t actor);
bool player_in_water(uintptr_t actor);
void travel_update(Menu *menu, uintptr_t actor, bool safe, bool paused, bool supported);
