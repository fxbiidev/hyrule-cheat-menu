#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum TravelCategory { FAVORITES, REGIONS, TOWNS, DUNGEONS, SPECIAL, TRAVEL_CATEGORY_COUNT };
enum { DESTINATION_COUNT = 214, AREA_COUNT = 51 };
enum TravelGroup {
    GROUP_FAVORITES, GROUP_HYRULE_FIELD, GROUP_LAKE_HYLIA, GROUP_LON_LON_RANCH,
    GROUP_MARKET, GROUP_TEMPLE_OF_TIME, GROUP_KAKARIKO, GROUP_GRAVEYARD,
    GROUP_KOKIRI_FOREST, GROUP_LOST_WOODS, GROUP_SACRED_MEADOW, GROUP_MOUNTAIN_TRAIL,
    GROUP_GORON_CITY, GROUP_MOUNTAIN_CRATER, GROUP_ZORA_RIVER, GROUP_ZORA_DOMAIN,
    GROUP_ZORA_FOUNTAIN, GROUP_GERUDO_VALLEY, GROUP_GERUDO_FORTRESS, GROUP_WASTELAND,
    GROUP_DESERT_COLOSSUS, GROUP_DUNGEONS, GROUP_DEKU_TREE, GROUP_DODONGO, GROUP_JABU,
    GROUP_FOREST_TEMPLE, GROUP_FIRE_TEMPLE, GROUP_WATER_TEMPLE, GROUP_SHADOW_TEMPLE,
    GROUP_SPIRIT_TEMPLE, GROUP_WELL, GROUP_ICE_CAVERN, GROUP_TRAINING_GROUND,
    GROUP_GANON_INTERIOR, GROUP_SPECIAL, GROUP_HYRULE_CASTLE, GROUP_GANON_EXTERIOR,
    TRAVEL_GROUP_COUNT
};
#define TRAVEL_ROOT UINT32_MAX
typedef struct { const char *name; unsigned parent; } TravelGroupInfo;

typedef struct {
    const char *scene, *name;
    unsigned age, category;
} TravelArea;

typedef struct {
    uint32_t id;
    unsigned area;
    const char *name, *spawn;
    float position[3];
} Destination;

extern const TravelArea travel_areas[AREA_COUNT];
extern const Destination destinations[DESTINATION_COUNT];
extern const TravelGroupInfo travel_groups[TRAVEL_GROUP_COUNT];

int destination_index(uint32_t id);
bool destination_valid(unsigned index);
void destination_set_gerudo_access(bool owned);
bool destination_locked(unsigned index);
const char *destination_restriction(unsigned index);
const char *destination_arrival_scene(unsigned index);
unsigned destination_group(unsigned index);
const char *destination_label(unsigned index);
unsigned travel_group_count(unsigned age, unsigned group, const uint32_t *favorites, unsigned favorite_count);
unsigned travel_list(unsigned age, unsigned group, const uint32_t *favorites,
                     unsigned favorite_count, unsigned *indices);
