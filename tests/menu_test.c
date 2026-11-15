#include "menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const enum Support support[CHEAT_COUNT] = {
    EXPERIMENTAL, EXPERIMENTAL, EXPERIMENTAL, EXPERIMENTAL, EXPERIMENTAL,
    UNSUPPORTED, UNSUPPORTED, UNSUPPORTED
};
static unsigned tick;
static void input(Menu *menu, uint32_t buttons) {
    tick += 20;
    menu_input(menu, tick, true, buttons, true, ((1u << FLY) - 1) | (1u << TRAVEL), support);
}
static void press(Menu *menu, uint32_t buttons) { input(menu, 0); input(menu, buttons); }
static unsigned row_index(Menu *menu, enum RowKind kind, unsigned value) {
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(menu, rows);
    for (unsigned i = 0; i < count; ++i)
        if (rows[i].kind == kind && rows[i].value == value) return i;
    fprintf(stderr, "Required row absent: kind=%u value=%u age=%u\n", kind, value, menu->age);
    exit(1);
    return 0;
}

int main(void) {
    Menu fade = {0};
    menu_input(&fade, 0, true, 0, true, 31, support);
    menu_input(&fade, 20, true, R3, true, 31, support);
    menu_input(&fade, 100, true, 0, true, 31, support);
    assert(fade.open && fade.alpha == 80);
    menu_input(&fade, 120, true, R3, true, 31, support);
    menu_input(&fade, 160, true, 0, true, 31, support);
    assert(!fade.open && fade.alpha == 60);
    menu_input(&fade, 180, true, R3, true, 31, support);
    menu_input(&fade, 220, true, 0, true, 31, support);
    assert(fade.open && fade.alpha == 80);
    menu_input(&fade, 400, true, 0, true, 31, support);
    assert(fade.alpha == 175);
    menu_input(&fade, 420, true, R3, true, 31, support);
    menu_input(&fade, 700, true, 0, true, 31, support);
    assert(!fade.open && !fade.alpha);
    Menu menu = {0};
    input(&menu, 0);
    assert(menu.cheat_sections == 3 && menu.cursor == 1);
    const uint32_t restricted_ids[] = {0x887aed4f, 0x834153be};
    for (unsigned age = 0; age < 2; ++age) {
        Menu restricted = {.tab = 1, .consent = true};
        input(&restricted, 0); press(&restricted, R3);
        restricted.age = age;
        restricted.travel_sections[age] = UINT64_MAX;
        restricted.cursor = row_index(&restricted, DESTINATION_ROW, destination_index(restricted_ids[age]));
        press(&restricted, CONFIRM);
        assert(restricted.open && !restricted.travel_prompt && !restricted.travel_request && restricted.notice);
        press(&restricted, FAVORITE);
        assert(preferences_favorite(&restricted.preferences, restricted_ids[age]));
    }
    Menu trainer = {.tab = 1, .consent = true};
    input(&trainer, 0); press(&trainer, R3);
    trainer.age = 1;
    trainer.travel_sections[1] = UINT64_MAX;
    trainer.cursor = row_index(&trainer, DESTINATION_ROW, destination_index(0x73fcc09f));
    press(&trainer, CONFIRM);
    assert(trainer.open && !trainer.travel_prompt && !trainer.travel_request && trainer.notice);
    press(&trainer, FAVORITE);
    assert(preferences_favorite(&trainer.preferences, 0x73fcc09f));
    destination_set_gerudo_access(true);
    press(&trainer, CONFIRM);
    assert(trainer.travel_prompt && !trainer.travel_request && trainer.travel_target == 0x73fcc09f);
    press(&trainer, BACK);
    destination_set_gerudo_access(false);
    press(&trainer, CONFIRM);
    assert(trainer.open && !trainer.travel_prompt && !trainer.travel_request);
    press(&menu, R3);
    assert(menu.open);
    press(&menu, CONFIRM);
    assert(menu.prompt && !menu.enabled);
    input(&menu, CONFIRM);
    assert(!menu.enabled);
    press(&menu, CONFIRM);
    assert(menu.consent && menu.enabled == (1u << GOD));
    press(&menu, CONFIRM);
    assert(!menu.enabled);

    Menu fresh = {0};
    input(&fresh, 0);
    press(&fresh, R3);
    fresh.cursor = row_index(&fresh, CHEAT_ROW, SPEED);
    press(&fresh, RIGHT);
    assert(fresh.prompt && !fresh.speed && !fresh.consent);
    input(&fresh, RIGHT);
    assert(!fresh.speed);
    press(&fresh, CONFIRM);
    assert(fresh.consent && fresh.speed == 1 && !fresh.prompt);

    menu.cursor = row_index(&menu, CHEAT_ROW, SPEED);
    press(&menu, RIGHT);
    assert(menu.speed == 1);
    input(&menu, RIGHT);
    assert(menu.speed == 1);
    for (unsigned i = 0; i < 5; ++i) press(&menu, RIGHT);
    assert(menu.speed == 3 && menu.preferences.movement_speed == 3);
    for (unsigned i = 0; i < 5; ++i) press(&menu, LEFT);
    assert(!menu.speed);
    menu.cursor = row_index(&menu, CHEAT_ROW, FLY);
    press(&menu, CONFIRM);
    assert(!(menu.enabled & (1u << FLY)));

    menu.cursor = 0;
    press(&menu, CONFIRM);
    assert(menu.cheat_sections == 2 && menu.cursor == 0);
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(&menu, rows);
    press(&menu, UP);
    assert(menu.cursor == count - 1);
    press(&menu, DOWN);
    assert(!menu.cursor);
    input(&menu, 0);
    input(&menu, DOWN);
    unsigned cursor = menu.cursor;
    menu_input(&menu, tick + 280, true, DOWN, true, 31, support);
    assert(menu.cursor == cursor);
    tick += 300;
    menu_input(&menu, tick, true, DOWN, true, 31, support);
    assert(menu.cursor == cursor + 1);

    press(&menu, TAB_RIGHT);
    assert(menu.tab == 1);
    unsigned remembered_cheat = menu.tab_cursor[0];
    press(&menu, TAB_LEFT);
    assert(!menu.tab && menu.cursor == remembered_cheat);
    press(&menu, TAB_RIGHT);
    menu.age = 1;
    menu.travel_sections[1] = UINT64_MAX;
    menu.cursor = row_index(&menu, DESTINATION_ROW, 144);
    menu_input(&menu, tick += 20, true, 0, true, 0, support);
    menu_input(&menu, tick += 20, true, CONFIRM, true, 0, support);
    assert(!menu.travel_prompt && !menu.travel_request);
    menu.notice = "previous status";
    press(&menu, DOWN);
    assert(!menu.notice);
    menu.cursor = row_index(&menu, DESTINATION_ROW, 144);
    press(&menu, FAVORITE);
    assert(preferences_favorite(&menu.preferences, destinations[144].id));
    press(&menu, CONFIRM);
    assert(menu.travel_prompt && !menu.travel_request);
    input(&menu, CONFIRM);
    assert(menu.travel_prompt && !menu.travel_request);
    press(&menu, BACK);
    assert(menu.open && !menu.travel_prompt);
    press(&menu, CONFIRM);
    press(&menu, CONFIRM);
    assert(!menu.open && menu.travel_request == destinations[144].id);
    uint32_t request = menu.travel_request;
    input(&menu, CONFIRM);
    assert(menu.travel_request == request);
    press(&menu, R3);
    press(&menu, AGE);
    assert(menu.age == 0 && menu.cursor == 1);

    menu.enabled = 15;
    menu.speed = 3;
    menu.can_return = true;
    menu_reset(&menu);
    assert(!menu.enabled && !menu.speed && !menu.can_return && !menu.travel_request);
    assert(menu.consent && menu.preferences.favorite_count == 1);
    press(&menu, R3);
    menu_input(&menu, tick + 10, false, 0, false, 0, support);
    tick += 20;
    input(&menu, BACK);
    assert(menu.open);
    press(&menu, BACK);
    assert(!menu.open);
    input(&menu, 0);
    press(&menu, R3);
    assert(menu.open);

    enum Support flight_support[CHEAT_COUNT];
    for (unsigned i = 0; i < CHEAT_COUNT; ++i) flight_support[i] = EXPERIMENTAL;
    Menu flight = {.consent = true};
    menu_input(&flight, 0, true, 0, true, 255, flight_support);
    menu_input(&flight, 20, true, R3, true, 255, flight_support);
    flight.cursor = row_index(&flight, CHEAT_ROW, FLY);
    menu_input(&flight, 40, true, 0, true, 255, flight_support);
    menu_input(&flight, 60, true, CONFIRM, true, 255, flight_support);
    assert(flight.enabled == (1u << FLY));
    flight.cursor = row_index(&flight, CHEAT_ROW, FLIGHT_SPEED);
    for (unsigned i = 0; i < 8; ++i) {
        menu_input(&flight, 80 + i * 40, true, 0, true, 255, flight_support);
        menu_input(&flight, 100 + i * 40, true, RIGHT, true, 255, flight_support);
        menu_input(&flight, 110 + i * 40, true, RIGHT, true, 255, flight_support);
        assert(flight.flight_speed == (i < 5 ? i + 1 : 5));
    }
    flight.cursor = row_index(&flight, CHEAT_ROW, NOCLIP);
    menu_input(&flight, 420, true, 0, true, 255, flight_support);
    menu_input(&flight, 440, true, CONFIRM, true, 255, flight_support);
    assert(flight.enabled == ((1u << FLY) | (1u << NOCLIP)));
    menu_input(&flight, 460, true, 0, true, 255, flight_support);
    menu_input(&flight, 480, true, CONFIRM, true, 255, flight_support);
    assert(flight.enabled == (1u << FLY));
    menu_reset(&flight);
    assert(!flight.enabled && !flight.speed && flight.flight_speed == 5);

    Preferences preferences = {.movement_speed = 3, .flight_speed = 5, .generation = 42};
    for (unsigned i = 0; i < DESTINATION_COUNT; ++i) {
        assert(destination_valid(i));
        assert(destination_index(destinations[i].id) == (int)i);
        assert(preferences_toggle_favorite(&preferences, destinations[i].id));
    }
    assert(preferences.favorite_count == DESTINATION_COUNT);
    uint8_t bytes[PREFERENCES_BYTES];
    size_t length = preferences_encode(&preferences, bytes);
    Preferences decoded = {0};
    assert(preferences_decode(&decoded, bytes, length));
    assert(decoded.generation == 42 && decoded.movement_speed == 3 && decoded.flight_speed == 5);
    assert(decoded.favorite_count == DESTINATION_COUNT);
    for (size_t i = 0; i < length; ++i) {
        bytes[i] ^= 1;
        assert(!preferences_decode(&decoded, bytes, length));
        assert(decoded.favorite_count == DESTINATION_COUNT);
        bytes[i] ^= 1;
    }
    for (size_t size = 0; size < length; ++size)
        assert(!preferences_decode(&decoded, bytes, size));
    Preferences migrated = {.favorite_count = 3,
                            .favorites = {0, destinations[0].id, destinations[0].id}};
    size_t migration_length = preferences_encode(&migrated, bytes);
    assert(preferences_decode(&migrated, bytes, migration_length));
    assert(migrated.favorite_count == 1 && migrated.favorites[0] == destinations[0].id);
    bytes[4] = 2;
    uint32_t checksum = 2166136261u;
    for (size_t i = 0; i < migration_length - 4; ++i) checksum = (checksum ^ bytes[i]) * 16777619u;
    for (unsigned i = 0; i < 4; ++i) bytes[migration_length - 4 + i] = (uint8_t)(checksum >> (8 * i));
    assert(!preferences_decode(&migrated, bytes, migration_length));
    assert(migrated.favorite_count == 1);
    menu.preferences = preferences;
    menu.tab = 1;
    menu.travel_sections[0] = menu.travel_sections[1] = UINT64_MAX;
    for (unsigned age = 0; age < 2; ++age) {
        menu.age = age;
        count = menu_rows(&menu, rows);
        assert(count <= MENU_ROWS);
        unsigned list[DESTINATION_COUNT];
        unsigned favorites = travel_list(age, GROUP_FAVORITES, preferences.favorites, preferences.favorite_count, list);
        unsigned headers = 1;
        for (unsigned group = 1; group < TRAVEL_GROUP_COUNT; ++group)
            headers += travel_group_count(age, group, 0, 0) > 0;
        assert(count == favorites * 2 + headers + 1);
    }

    Menu regions = {.tab = 1, .open = true, .consent = true};
    input(&regions, 0);
    assert(!regions.travel_sections[0] && !regions.travel_sections[1]);
    regions.cursor = row_index(&regions, TRAVEL_SECTION, GROUP_HYRULE_FIELD);
    unsigned header = regions.cursor;
    press(&regions, CONFIRM);
    assert(regions.cursor == header && (regions.travel_sections[0] & (1ull << GROUP_HYRULE_FIELD)));
    regions.travel_sections[0] |= 1ull << GROUP_FAVORITES;
    regions.cursor = row_index(&regions, DESTINATION_ROW, 26);
    press(&regions, FAVORITE);
    count = menu_rows(&regions, rows);
    assert(rows[regions.cursor].kind == DESTINATION_ROW && rows[regions.cursor].value == 26);
    assert(rows[regions.cursor].parent == GROUP_HYRULE_FIELD);
    assert(regions.notice && !strcmp(regions.notice, "Added to Favorites."));
    unsigned remembered = regions.cursor, scroll = regions.scroll;
    press(&regions, TAB_LEFT);
    press(&regions, TAB_RIGHT);
    assert(regions.cursor == remembered && regions.scroll == scroll);
    press(&regions, AGE);
    assert(regions.age == 1 && regions.cursor == 1 && !regions.travel_sections[1]);
    regions.cursor = row_index(&regions, TRAVEL_SECTION, GROUP_LAKE_HYLIA);
    press(&regions, RIGHT);
    assert(regions.travel_sections[1] == (1ull << GROUP_LAKE_HYLIA));
    unsigned adult_cursor = regions.cursor;
    press(&regions, AGE);
    assert(!regions.age && regions.cursor == remembered && regions.scroll == scroll);
    press(&regions, BACK);
    count = menu_rows(&regions, rows);
    assert(regions.open && rows[regions.cursor].kind == TRAVEL_SECTION &&
           rows[regions.cursor].value == GROUP_HYRULE_FIELD);
    assert(!(regions.travel_sections[0] & (1ull << GROUP_HYRULE_FIELD)));
    for (unsigned i = 0; i < 100; ++i) {
        unsigned previous = regions.cursor;
        press(&regions, CONFIRM);
        assert(regions.cursor == previous && regions.open);
    }
    press(&regions, AGE);
    assert(regions.age == 1 && regions.cursor == adult_cursor);
    regions.cursor = row_index(&regions, TRAVEL_SECTION, GROUP_DUNGEONS);
    press(&regions, CONFIRM);
    regions.cursor = row_index(&regions, TRAVEL_SECTION, GROUP_FOREST_TEMPLE);
    press(&regions, CONFIRM);
    regions.cursor = row_index(&regions, DESTINATION_ROW, 114);
    press(&regions, BACK);
    menu_rows(&regions, rows);
    assert(rows[regions.cursor].kind == TRAVEL_SECTION && rows[regions.cursor].value == GROUP_FOREST_TEMPLE);
    press(&regions, BACK);
    menu_rows(&regions, rows);
    assert(rows[regions.cursor].kind == TRAVEL_SECTION && rows[regions.cursor].value == GROUP_DUNGEONS);

    puts("Menu, 214 destinations, all favorites, transitions, and corrupt preferences passed.");
    return 0;
}
