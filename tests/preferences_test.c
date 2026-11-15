#include "preferences.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Frozen schema fixtures generated independently of preferences_encode. The v1
// Favorite bytes begin at 24; v2 adds damage at 24 and moves Favorites to 28.
static const uint8_t legacy_v1[] = {
    0x48,0x43,0x4d,0x50,0x01,0x00,0x00,0x00,0x2a,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x4f,0xed,0x7a,0x88,0x9f,0xc0,0xfc,0x73,0xe6,0x38,0xb7,0x54
};
static const uint8_t current_v2[] = {
    0x48,0x43,0x4d,0x50,0x02,0x00,0x00,0x00,0x2a,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x03,0x00,0x00,0x00,0x4f,0xed,0x7a,0x88,0x9f,0xc0,0xfc,0x73,
    0x16,0xb7,0x48,0xb7
};

static void word(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}
static void sign_fixture(uint8_t *bytes, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size - 4; ++i) hash = (hash ^ bytes[i]) * 16777619u;
    word(bytes + size - 4, hash);
}
static void reject_unchanged(Preferences *out, const uint8_t *bytes, size_t size) {
    Preferences before = *out;
    assert(!preferences_decode(out, bytes, size));
    assert(!memcmp(out, &before, sizeof before));
}

int main(void) {
    Preferences preferences = {.damage = 3};
    uint8_t bytes[PREFERENCES_BYTES];
    assert(preferences_decode(&preferences, legacy_v1, sizeof legacy_v1));
    assert(preferences.generation == 42 && preferences.movement_speed == 3 && preferences.flight_speed == 5);
    assert(!preferences.damage && preferences.favorite_count == 2);
    assert(preferences.favorites[0] == 0x887aed4fu && preferences.favorites[1] == 0x73fcc09fu);
    assert(preferences_encode(&preferences, bytes) == sizeof current_v2);
    assert(bytes[4] == 2 && !bytes[24]);
    assert(!memcmp(bytes + 28, legacy_v1 + 24, 8));
    assert(preferences_decode(&preferences, current_v2, sizeof current_v2));
    assert(preferences.damage == 3 && preferences.favorite_count == 2);
    assert(preferences_encode(&preferences, bytes) == sizeof current_v2);
    assert(!memcmp(bytes, current_v2, sizeof current_v2));
    for (unsigned damage = 0; damage < 4; ++damage) {
        preferences.damage = damage;
        size_t length = preferences_encode(&preferences, bytes);
        Preferences decoded = {0};
        assert(length == sizeof current_v2 && preferences_decode(&decoded, bytes, length));
        assert(decoded.damage == damage && decoded.favorite_count == 2);
    }
    for (size_t i = 0; i < sizeof legacy_v1; ++i) {
        memcpy(bytes, legacy_v1, sizeof legacy_v1); bytes[i] ^= 1;
        reject_unchanged(&preferences, bytes, sizeof legacy_v1);
    }
    for (size_t i = 0; i < sizeof current_v2; ++i) {
        memcpy(bytes, current_v2, sizeof current_v2); bytes[i] ^= 1;
        reject_unchanged(&preferences, bytes, sizeof current_v2);
    }
    for (size_t length = 0; length < sizeof legacy_v1; ++length)
        reject_unchanged(&preferences, legacy_v1, length);
    for (size_t length = 0; length < sizeof current_v2; ++length)
        reject_unchanged(&preferences, current_v2, length);
    memcpy(bytes, current_v2, sizeof current_v2);
    word(bytes + 24, 4); sign_fixture(bytes, sizeof current_v2);
    reject_unchanged(&preferences, bytes, sizeof current_v2);
    word(bytes + 24, 0); word(bytes + 4, 3); sign_fixture(bytes, sizeof current_v2);
    reject_unchanged(&preferences, bytes, sizeof current_v2);
    word(bytes + 4, 2); word(bytes + 20, 3); sign_fixture(bytes, sizeof current_v2);
    reject_unchanged(&preferences, bytes, sizeof current_v2);
    // A minimal v2 envelope cannot reuse v1's shorter minimum header.
    memset(bytes, 0, 28); word(bytes, 0x504d4348u); word(bytes + 4, 2); sign_fixture(bytes, 28);
    reject_unchanged(&preferences, bytes, 28);
    // The old maximum of all 214 Favorites still migrates without losing an ID.
    memset(bytes, 0, sizeof bytes); word(bytes, 0x504d4348u); word(bytes + 4, 1);
    word(bytes + 8, 0xffffffffu); word(bytes + 12, 3); word(bytes + 16, 5);
    word(bytes + 20, DESTINATION_COUNT);
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i) word(bytes + 24 + i * 4, destinations[i].id);
    size_t legacy_size = 28 + DESTINATION_COUNT * 4;
    sign_fixture(bytes, legacy_size);
    assert(preferences_decode(&preferences, bytes, legacy_size));
    assert(preferences.favorite_count == DESTINATION_COUNT && !preferences.damage && preferences.generation == 0xffffffffu);
    assert(preferences_encode(&preferences, bytes) == PREFERENCES_BYTES);
    Preferences migrated = {0};
    assert(preferences_decode(&migrated, bytes, sizeof bytes) && migrated.favorite_count == DESTINATION_COUNT);
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i) assert(migrated.favorites[i] == destinations[i].id);
    preferences.damage = 4;
    memset(bytes, 0x5a, sizeof bytes);
    assert(!preferences_encode(&preferences, bytes) && bytes[0] == 0x5a);
    puts("Preferences migration: frozen v1/v2 bytes, all 214 legacy Favorites, four damage settings, checksum-valid invalid values and rejection atomicity passed.");
}
