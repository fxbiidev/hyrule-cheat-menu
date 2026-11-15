#pragma once

#include "travel.h"

enum { PREFERENCES_BYTES = 32 + DESTINATION_COUNT * 4 };

typedef struct {
    uint32_t generation;
    unsigned movement_speed, flight_speed, damage, favorite_count;
    uint32_t favorites[DESTINATION_COUNT];
} Preferences;

bool preferences_favorite(const Preferences *preferences, uint32_t id);
bool preferences_toggle_favorite(Preferences *preferences, uint32_t id);
size_t preferences_encode(const Preferences *preferences, uint8_t *bytes);
bool preferences_decode(Preferences *preferences, const uint8_t *bytes, size_t length);

bool storage_load(Preferences *preferences);
bool storage_save(Preferences *preferences);
const char *storage_status(void);
