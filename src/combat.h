#pragma once

#include <stdbool.h>
#include <stdint.h>

// Settings are published only for the current, identified Link and save epoch.
// A stored preference must not be published as an active cheat after a save load.
void combat_publish(uintptr_t link_actor, unsigned epoch, bool one_hit,
                    unsigned damage_index, bool safe);
void combat_reset(void);

// Called only at the verified native accepted-external-damage LDADD bridge.
// Returns a replacement operand; never writes actor health, requests or saves.
int combat_damage(uintptr_t state, uintptr_t request, int native_amount);
void combat_hook(void);

enum CombatReason {
    COMBAT_DISABLED, COMBAT_UNSAFE, COMBAT_BAD_STATE, COMBAT_UNSUPPORTED_TARGET,
    COMBAT_BAD_SOURCE, COMBAT_OTHER_CAUSE, COMBAT_BAD_AMOUNT, COMBAT_NATIVE_CAP,
    COMBAT_APPLIED, COMBAT_UNCHANGED
};

// Bounded in-memory QA evidence. No file, network, inventory or save writes.
typedef struct {
    unsigned inspected, modified, reason, damage_type, target_index;
    int native_amount, applied_amount, current_life, queued_before;
    uintptr_t target, source, request;
} CombatEvidence;
extern CombatEvidence combat_evidence;
extern CombatEvidence combat_last_player;
extern unsigned combat_reason_counts[COMBAT_UNCHANGED + 1];
extern unsigned combat_player_cause_counts[32];
