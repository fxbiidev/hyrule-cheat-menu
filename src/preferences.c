#include "preferences.h"

static uint32_t read_word(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static void write_word(uint8_t *bytes, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (uint8_t)(value >> (i * 8));
}

static uint32_t checksum(const uint8_t *bytes, size_t length) {
    uint32_t value = 2166136261u;
    for (size_t i = 0; i < length; ++i) value = (value ^ bytes[i]) * 16777619u;
    return value;
}

bool preferences_favorite(const Preferences *preferences, uint32_t id) {
    for (unsigned i = 0; i < preferences->favorite_count; ++i)
        if (preferences->favorites[i] == id) return true;
    return false;
}

bool preferences_toggle_favorite(Preferences *preferences, uint32_t id) {
    if (destination_index(id) < 0) return false;
    for (unsigned i = 0; i < preferences->favorite_count; ++i) {
        if (preferences->favorites[i] != id) continue;
        for (unsigned j = i + 1; j < preferences->favorite_count; ++j)
            preferences->favorites[j - 1] = preferences->favorites[j];
        --preferences->favorite_count;
        return true;
    }
    if (preferences->favorite_count >= DESTINATION_COUNT) return false;
    preferences->favorites[preferences->favorite_count++] = id;
    return true;
}

size_t preferences_encode(const Preferences *preferences, uint8_t *bytes) {
    if (preferences->favorite_count > DESTINATION_COUNT || preferences->movement_speed > 3 ||
        preferences->flight_speed > 5 || preferences->damage > 3) return 0;
    size_t length = 32 + preferences->favorite_count * 4;
    write_word(bytes, 0x504d4348u); // HCMP
    write_word(bytes + 4, 2);
    write_word(bytes + 8, preferences->generation);
    write_word(bytes + 12, preferences->movement_speed);
    write_word(bytes + 16, preferences->flight_speed);
    write_word(bytes + 20, preferences->favorite_count);
    write_word(bytes + 24, preferences->damage);
    for (unsigned i = 0; i < preferences->favorite_count; ++i)
        write_word(bytes + 28 + i * 4, preferences->favorites[i]);
    write_word(bytes + length - 4, checksum(bytes, length - 4));
    return length;
}

bool preferences_decode(Preferences *preferences, const uint8_t *bytes, size_t length) {
    if (length < 28 || length > PREFERENCES_BYTES || read_word(bytes) != 0x504d4348u ||
        (read_word(bytes + 4) != 1 && read_word(bytes + 4) != 2) || checksum(bytes, length - 4) != read_word(bytes + length - 4))
        return false;
    unsigned version = read_word(bytes + 4);
    unsigned header = version == 1 ? 24 : 28;
    if (length < header + 4) return false;
    unsigned count = read_word(bytes + 20);
    unsigned movement = read_word(bytes + 12), flight = read_word(bytes + 16);
    unsigned damage = version == 2 ? read_word(bytes + 24) : 0;
    if (count > DESTINATION_COUNT || length != header + 4 + count * 4 || movement > 3 || flight > 5 || damage > 3)
        return false;
    Preferences decoded = { .generation = read_word(bytes + 8),
                            .movement_speed = movement, .flight_speed = flight, .damage = damage };
    for (unsigned i = 0; i < count; ++i) {
        uint32_t id = read_word(bytes + header + i * 4);
        if (destination_index(id) >= 0 && !preferences_favorite(&decoded, id))
            decoded.favorites[decoded.favorite_count++] = id;
    }
    *preferences = decoded;
    return true;
}
