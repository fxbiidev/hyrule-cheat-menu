#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t lx, ly, rx, ry;
    uint32_t buttons;
} FreeCameraInput;

// Call before installing free_camera_hook into the verified dispatch slot.
void free_camera_init(void);
// Immediate invalidation; never dereferences an old view or changes Link.
void free_camera_reset(void);
bool free_camera_available(unsigned epoch, bool paused);
// Return the actual state. The caller clears its toggle after any refusal.
// allowed must exclude travel, scene changes, cutscenes and unidentified Link.
// paused includes the menu transition/native pause; camera inputs then do not move.
bool free_camera_update(bool requested, uintptr_t actor, unsigned epoch,
                        bool allowed, bool paused, const FreeCameraInput *input);
bool free_camera_active(void);

enum FreeCameraReason {
    CAMERA_REASON_NONE, CAMERA_REASON_OFF, CAMERA_REASON_NOT_ALLOWED,
    CAMERA_REASON_NO_ACTOR, CAMERA_REASON_NO_INPUT, CAMERA_REASON_SAVE,
    CAMERA_REASON_INTERRUPTED, CAMERA_REASON_WORLD, CAMERA_REASON_LOADING,
    CAMERA_REASON_VIEW, CAMERA_REASON_OBSERVATION, CAMERA_REASON_POSE,
    CAMERA_REASON_NATIVE_SOURCE, CAMERA_REASON_NATIVE_MATRIX, CAMERA_REASON_RESET
};
// First reason in the current activation is retained after deactivation, for
// bounded native QA. Stage: 0 external reset, 1 availability, 2 input, 3 view hook.
typedef struct {
    unsigned reason, stage, stops, expected_epoch, current_epoch;
    uintptr_t actor, view;
    uint64_t when;
} FreeCameraEvidence;
extern FreeCameraEvidence free_camera_evidence;

// Native inline bridge: x20 is the just-built view. Only matrix +8..+37 changes.
void free_camera_apply(uintptr_t view);
void free_camera_hook(void);

