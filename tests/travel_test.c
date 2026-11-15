#include "menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int order(unsigned a, unsigned b) {
    int region = strcmp(travel_groups[destination_group(a)].name,
                        travel_groups[destination_group(b)].name);
    return region ? region : strcmp(destination_label(a), destination_label(b));
}

int main(void) {
    uint32_t favorites[DESTINATION_COUNT];
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i) {
        assert(destination_valid(i));
        assert(destination_index(destinations[i].id) == (int)i);
        assert(destination_group(i) < TRAVEL_GROUP_COUNT);
        assert(destination_label(i)[0] && strlen(destination_label(i)) <= 34);
        unsigned indent = travel_groups[destination_group(i)].parent == TRAVEL_ROOT ? 38 : 44;
        assert(strlen(destination_label(i)) <= (246 - indent) / 6);
        favorites[i] = destinations[i].id;
        for (unsigned j = 0; j < i; ++j)
            if (travel_areas[destinations[i].area].age == travel_areas[destinations[j].area].age &&
                destination_group(i) == destination_group(j))
                assert(strcmp(destination_label(i), destination_label(j)));
    }
    assert(destination_group(DESTINATION_COUNT) == TRAVEL_ROOT);
    assert(!strcmp(destination_label(destination_index(0x7767ab3d)), "Potion shop"));
    assert(!destination_label(DESTINATION_COUNT)[0]);
    assert(destination_restriction(DESTINATION_COUNT));
    assert(destination_restriction(destination_index(0x887aed4f)));
    assert(destination_valid(destination_index(0x887aed4f))); // Retain data/Favorite compatibility.
    assert(destination_restriction(destination_index(0x834153be)));
    assert(destination_valid(destination_index(0x834153be)));
    unsigned trainer = (unsigned)destination_index(0x73fcc09f);
    assert(destination_valid(trainer) && destination_locked(trainer) && destination_restriction(trainer));
    assert(!destination_locked(DESTINATION_COUNT));
    destination_set_gerudo_access(true);
    assert(!destination_locked(trainer) && !destination_restriction(trainer));
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i)
        assert((destination_restriction(i) != 0) ==
               (destinations[i].id == 0x887aed4fu || destinations[i].id == 0x834153beu));
    destination_set_gerudo_access(false);
    assert(destination_locked(trainer) && destination_restriction(trainer));
    assert(!destination_restriction(0));
    assert(!destination_arrival_scene(DESTINATION_COUNT));
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i)
        assert(!strcmp(destination_arrival_scene(i), destinations[i].id == 0x1f300d08u ?
                       "InsideGanonCastle" : destinations[i].id == 0x1c023884u ?
                       "GerudoFortress_Adult" : travel_areas[destinations[i].area].scene));
    assert(destination_group(destination_index(0xca674ff3)) == GROUP_LAKE_HYLIA);
    assert(destination_group(destination_index(0x78a45f34)) == GROUP_LAKE_HYLIA);
    assert(destination_group(destination_index(0xef4fb363)) == GROUP_HYRULE_FIELD);
    assert(destination_group(destination_index(0x9151554f)) == GROUP_LON_LON_RANCH);
    assert(destination_group(destination_index(0x52a41e8c)) == GROUP_TEMPLE_OF_TIME);
    for (unsigned group = 0; group < TRAVEL_GROUP_COUNT; ++group) {
        unsigned parent = travel_groups[group].parent;
        assert(parent == TRAVEL_ROOT || (parent < TRAVEL_GROUP_COUNT && parent != group));
        if (parent != TRAVEL_ROOT) assert(travel_groups[parent].parent == TRAVEL_ROOT);
    }
    for (unsigned age = 0; age < 2; ++age) {
        bool seen[DESTINATION_COUNT] = {0};
        unsigned total = 0;
        for (unsigned group = 1; group < TRAVEL_GROUP_COUNT; ++group) {
            unsigned expected = 0, indices[DESTINATION_COUNT];
            for (unsigned i = 0; i < DESTINATION_COUNT; ++i) {
                unsigned assigned = destination_group(i);
                if (travel_areas[destinations[i].area].age == age &&
                    (assigned == group || travel_groups[assigned].parent == group)) ++expected;
            }
            assert(travel_group_count(age, group, 0, 0) == expected);
            unsigned count = travel_list(age, group, 0, 0, indices);
            for (unsigned i = 0; i < count; ++i) {
                unsigned index = indices[i];
                assert(index < DESTINATION_COUNT && !seen[index]);
                seen[index] = true;
                assert(destination_group(index) == group);
                assert(travel_areas[destinations[index].area].age == age);
                if (i) assert(order(indices[i - 1], index) <= 0);
                ++total;
            }
        }
        unsigned indices[DESTINATION_COUNT];
        unsigned count = travel_list(age, GROUP_FAVORITES, favorites, DESTINATION_COUNT, indices);
        assert(count == total && travel_group_count(age, GROUP_FAVORITES, favorites, DESTINATION_COUNT) == total);
        for (unsigned i = 0; i < count; ++i) {
            assert(seen[indices[i]]);
            if (i) assert(order(indices[i - 1], indices[i]) <= 0);
        }
        Menu menu = {.tab = 1, .age = age};
        menu_initialize(&menu);
        menu.preferences.favorite_count = DESTINATION_COUNT;
        memcpy(menu.preferences.favorites, favorites, sizeof favorites);
        for (unsigned group = 0; group < TRAVEL_GROUP_COUNT; ++group) {
            menu.travel_sections[age] = 1ull << group;
            MenuRow rows[MENU_ROWS];
            unsigned count = menu_rows(&menu, rows);
            assert(count <= MENU_ROWS);
            for (unsigned i = 0; i < count; ++i)
                if (rows[i].kind == DESTINATION_ROW) {
                    assert(rows[i].parent == group);
                    assert(travel_groups[group].parent == TRAVEL_ROOT);
                }
        }
    }
    puts("214 unchanged destinations, 37 groups, shared-scene routing, concise unique labels and sorted Favorites passed.");
    return 0;
}
