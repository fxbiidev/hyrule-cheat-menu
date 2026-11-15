#include "travel.h"

#include "destinations.inc"

typedef struct { unsigned group; const char *label; } DestinationView;
#include "travel_view.inc"
_Static_assert(TRAVEL_GROUP_COUNT < 64, "travel expansion mask capacity");
static unsigned gerudo_access;

void destination_set_gerudo_access(bool owned) {
    __atomic_store_n(&gerudo_access, owned, __ATOMIC_RELEASE);
}

bool destination_locked(unsigned index) {
    // The trainer captures unrecognized visitors at this exact entrance.
    return index < DESTINATION_COUNT && destinations[index].id == 0x73fcc09fu &&
           !__atomic_load_n(&gerudo_access, __ATOMIC_ACQUIRE);
}

unsigned destination_group(unsigned index) {
    return index < DESTINATION_COUNT ? destination_views[index].group : TRAVEL_ROOT;
}

const char *destination_label(unsigned index) {
    if (index >= DESTINATION_COUNT) return "";
    return destination_views[index].label ? destination_views[index].label : destinations[index].name;
}

int destination_index(uint32_t id) {
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i)
        if (destinations[i].id == id) return (int)i;
    return -1;
}

bool destination_valid(unsigned index) {
    if (index >= DESTINATION_COUNT) return false;
    const Destination *destination = &destinations[index];
    if (!destination->id || destination->area >= AREA_COUNT) return false;
    for (unsigned axis = 0; axis < 3; ++axis)
        if (!__builtin_isfinite(destination->position[axis])) return false;
    return true;
}

const char *destination_restriction(unsigned index) {
    if (index >= DESTINATION_COUNT) return "Unknown destination.";
    // Two real Young-Link attempts on 2026-11-12 entered this native exit
    // trigger and immediately returned to Lake Hylia. Keep the original record
    // and Favorite ID, but require a verified stable route before enabling it.
    if (destinations[index].id == 0x887aed4fu)
        return "River exit returns to Lake Hylia.";
    // Two independent Adult-Link arrivals also auto-exited the valley into
    // Hyrule Field. Do not present that transient river trigger as a stable warp.
    if (destinations[index].id == 0x834153beu)
        return "River exit immediately leaves the valley.";
    if (destination_locked(index)) return "Requires Gerudo training-ground access.";
    return 0;
}

const char *destination_arrival_scene(unsigned index) {
    if (index >= DESTINATION_COUNT) return 0;
    // Two real attempts touched the exterior door's native exit trigger and
    // continued into the castle. Keep the original request coordinates; only
    // recognize the verified final scene and describe it in the display label.
    if (destinations[index].id == 0x1f300d08u) return "InsideGanonCastle";
    // The Adult wasteland-edge coordinates likewise touch the native fortress
    // exit trigger. Two real attempts settled at its gate without further motion.
    if (destinations[index].id == 0x1c023884u) return "GerudoFortress_Adult";
    return travel_areas[destinations[index].area].scene;
}

static bool is_favorite(uint32_t id, const uint32_t *favorites, unsigned count) {
    for (unsigned i = 0; i < count; ++i)
        if (favorites[i] == id) return true;
    return false;
}

unsigned travel_group_count(unsigned age, unsigned group, const uint32_t *favorites, unsigned favorite_count) {
    if (age > 1 || group >= TRAVEL_GROUP_COUNT || favorite_count > DESTINATION_COUNT) return 0;
    if (group != GROUP_FAVORITES) return group_counts[age][group];
    unsigned count = 0;
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i)
        if (travel_areas[destinations[i].area].age == age &&
            is_favorite(destinations[i].id, favorites, favorite_count)) ++count;
    return count;
}

unsigned travel_list(unsigned age, unsigned group, const uint32_t *favorites,
                     unsigned favorite_count, unsigned *indices) {
    if (age > 1 || group >= TRAVEL_GROUP_COUNT || favorite_count > DESTINATION_COUNT) return 0;
    unsigned count = 0;
    for (unsigned position = 0; position < DESTINATION_COUNT; ++position) {
        unsigned i = destination_order[position];
        const TravelArea *area = &travel_areas[destinations[i].area];
        bool selected = group == GROUP_FAVORITES ? is_favorite(destinations[i].id, favorites, favorite_count)
                                                : destination_group(i) == group;
        if (area->age != age || !selected || !destination_valid(i)) continue;
        indices[count++] = i;
    }
    return count;
}
