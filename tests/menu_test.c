#include "menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const enum Support support[CHEAT_COUNT] = {
    EXPERIMENTAL, EXPERIMENTAL, EXPERIMENTAL, EXPERIMENTAL, EXPERIMENTAL,
    UNSUPPORTED, UNSUPPORTED, UNSUPPORTED
};
static const enum Support supported[CHEAT_COUNT] = {
    VERIFIED, VERIFIED, VERIFIED, VERIFIED, VERIFIED, VERIFIED,
    VERIFIED, VERIFIED, VERIFIED, VERIFIED, VERIFIED, VERIFIED
};
enum { ALL_AVAILABLE = (1u << (TRAVEL + 1)) - 1 };
static unsigned tick;
static void input(Menu *menu, uint32_t buttons) {
    tick += 20;
    menu_input(menu, tick, true, buttons, true, ((1u << FLY) - 1) | (1u << TRAVEL), support);
}
static void press(Menu *menu, uint32_t buttons) { input(menu, 0); input(menu, buttons); }
static void input_all(Menu *menu, uint32_t buttons) {
    tick += 20;
    menu_input(menu, tick, true, buttons, true, ALL_AVAILABLE, supported);
}
static void press_all(Menu *menu, uint32_t buttons) { input_all(menu, 0); input_all(menu, buttons); }
static InventorySnapshot inventory_fixture(bool child) {
    InventorySnapshot snapshot = {.valid = true, .epoch = 7, .pouch = 1, .actor = 2, .child = child};
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i) {
        InventoryEntry *entry = &snapshot.entries[i];
        bool editable = inventory_catalog[i].quantity_editable;
        entry->valid = entry->owned = true;
        entry->age_allowed = (inventory_catalog[i].ages & (child ? INV_CHILD : INV_ADULT)) != 0;
        entry->count = editable ? 3 : 1;
        entry->capacity = editable ? 30 : 1;
        entry->can_set = entry->can_remove = entry->age_allowed;
        entry->can_refill = editable && entry->age_allowed;
    }
    return snapshot;
}
static unsigned row_index(Menu *menu, enum RowKind kind, unsigned value) {
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(menu, rows);
    for (unsigned i = 0; i < count; ++i)
        if (rows[i].kind == kind && rows[i].value == value) return i;
    fprintf(stderr, "Required row absent: kind=%u value=%u age=%u\n", kind, value, menu->age);
    exit(1);
    return 0;
}

static void ownership_requests(void) {
    // Every supported catalog entry can request real ownership changes from
    // either initial state. Native validation belongs to inventory_test.c.
    for (unsigned index = 0; index < INVENTORY_COUNT; ++index) {
        bool child = (inventory_catalog[index].ages & INV_ADULT) == 0;
        Menu menu = {.tab = MENU_ITEMS, .open = true, .consent = true};
        input_all(&menu, 0);
        menu.item_sections = (1u << ITEM_CATEGORY_COUNT) - 1;
        menu.inventory = inventory_fixture(child);
        menu.cursor = row_index(&menu, ITEM_ROW, index);
        InventorySnapshot original = menu.inventory;
        press_all(&menu, CONFIRM);
        assert(menu.item_prompt && !menu.item_request && menu.item_action == INVENTORY_REMOVE);
        assert(menu.item_prompt_index == index && menu.item_prompt_quantity == 0);
        input_all(&menu, CONFIRM);
        assert(menu.item_prompt && !menu.item_request);
        press_all(&menu, BACK);
        assert(!menu.item_prompt && !menu.item_request && menu.open);
        assert(!memcmp(&original, &menu.inventory, sizeof original));

        InventoryEntry *entry = &menu.inventory.entries[index];
        entry->owned = false; entry->count = 0; entry->can_remove = false;
        menu.item_edit_index = INVENTORY_COUNT;
        press_all(&menu, CONFIRM);
        assert(menu.item_prompt && !menu.item_request && menu.item_action == INVENTORY_SET);
        assert(menu.item_prompt_index == index && menu.item_prompt_quantity == 1);
        input_all(&menu, CONFIRM);
        assert(menu.item_prompt && !menu.item_request);
        press_all(&menu, CONFIRM);
        assert(!menu.item_prompt && menu.item_request);
        assert(entry->count == 0 && !entry->owned);
        // The guest consumes the request; holding the same A cannot resubmit it.
        menu.item_request = false;
        input_all(&menu, CONFIRM);
        assert(!menu.item_prompt && !menu.item_request);

        menu.enabled |= 1u << ITEMS;
        press_all(&menu, CONFIRM);
        assert(!menu.item_prompt && !menu.item_request && menu.notice);
        menu.enabled &= ~(1u << ITEMS);
        entry->age_allowed = false;
        press_all(&menu, CONFIRM);
        assert(!menu.item_prompt && !menu.item_request && menu.notice);
        entry->age_allowed = true; entry->can_set = false;
        press_all(&menu, CONFIRM);
        assert(!menu.item_prompt && !menu.item_request && menu.notice);
    }
}

int main(void) {
    ownership_requests();
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
    bytes[4] = 3;
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

    Menu integrated = {.open = true, .consent = true};
    input_all(&integrated, 0);
    integrated.cheat_sections = (1u << CHEAT_SECTION_COUNT) - 1;
    count = menu_rows(&integrated, rows);
    assert(count == CHEAT_SECTION_COUNT + CHEAT_COUNT - 1);
    unsigned combat = row_index(&integrated, CHEAT_SECTION, COMBAT_CHEATS);
    assert(rows[combat + 1].kind == CHEAT_ROW && rows[combat + 1].value == ONE_HIT);
    assert(rows[combat + 2].kind == CHEAT_ROW && rows[combat + 2].value == DAMAGE);
    integrated.cursor = combat + 1;
    press_all(&integrated, CONFIRM);
    assert(integrated.enabled == (1u << ONE_HIT));
    integrated.cursor = combat + 2;
    for (unsigned i = 0; i < 5; ++i) press_all(&integrated, RIGHT);
    assert(integrated.damage == 3 && integrated.preferences.damage == 3 && integrated.preferences_dirty);
    input_all(&integrated, RIGHT);
    assert(integrated.damage == 3);
    integrated.cursor = combat + 1;
    press_all(&integrated, CONFIRM);
    assert(!integrated.enabled && integrated.damage == 3 && integrated.preferences.damage == 3);
    integrated.cursor = row_index(&integrated, CHEAT_ROW, FREE_CAMERA);
    press_all(&integrated, CONFIRM);
    assert(integrated.enabled == (1u << FREE_CAMERA));
    unsigned cheat_cursor = integrated.cursor;
    integrated.preferences_dirty = false;
    Preferences untouched = integrated.preferences;
    unsigned old_age = integrated.age;
    unsigned old_age_cursor = integrated.age_cursor[old_age];
    integrated.inventory = inventory_fixture(true);
    InventorySnapshot unchanged_inventory = integrated.inventory;
    // LB and RB cycle in opposite directions, including both tab-strip ends.
    press_all(&integrated, TAB_LEFT);
    assert(integrated.tab == MENU_ITEMS && !integrated.cursor);
    input_all(&integrated, TAB_LEFT);
    assert(integrated.tab == MENU_ITEMS);
    count = menu_rows(&integrated, rows);
    assert(count == ITEM_CATEGORY_COUNT + 7 && rows[0].kind == ITEM_SECTION);
    integrated.cursor = row_index(&integrated, ITEM_ROW, INV_KOKIRI_SWORD);
    press_all(&integrated, CONFIRM);
    assert(integrated.item_prompt && !integrated.item_request && integrated.item_action == INVENTORY_REMOVE);
    assert(integrated.item_prompt_quantity == 0 && integrated.item_prompt_index == INV_KOKIRI_SWORD);
    press_all(&integrated, BACK);
    press_all(&integrated, AGE);
    assert(!integrated.item_prompt);
    press_all(&integrated, FAVORITE);
    assert(integrated.item_prompt && integrated.item_action == INVENTORY_REMOVE);
    press_all(&integrated, BACK);
    assert(integrated.age == old_age && integrated.age_cursor[old_age] == old_age_cursor);
    assert(!integrated.travel_prompt && !integrated.travel_target && !integrated.travel_request);
    assert(!memcmp(&untouched, &integrated.preferences, sizeof untouched) && !integrated.preferences_dirty);
    press_all(&integrated, BACK);
    assert(!integrated.cursor && !(integrated.item_sections & 1) && integrated.open);
    press_all(&integrated, RIGHT);
    assert(integrated.item_sections & 1);
    integrated.cursor = row_index(&integrated, ITEM_SECTION, INV_AMMUNITION);
    press_all(&integrated, CONFIRM);
    count = menu_rows(&integrated, rows);
    assert(rows[count - 1].kind == ITEM_ROW && rows[count - 1].value == INV_BOMBCHUS);
    integrated.cursor = row_index(&integrated, ITEM_ROW, INV_BOMBS);
    input_all(&integrated, 0);
    assert(integrated.item_quantity == 3);
    press_all(&integrated, RIGHT);
    assert(integrated.item_quantity == 4 && !integrated.item_prompt && !integrated.item_request);
    press_all(&integrated, CONFIRM);
    assert(integrated.item_prompt && !integrated.item_request && integrated.item_action == INVENTORY_SET);
    assert(integrated.item_prompt_quantity == 4 && integrated.item_prompt_index == INV_BOMBS);
    input_all(&integrated, CONFIRM);
    assert(integrated.item_prompt && !integrated.item_request);
    press_all(&integrated, BACK);
    assert(!integrated.item_prompt && !integrated.item_request && integrated.open);
    press_all(&integrated, CONFIRM);
    press_all(&integrated, CONFIRM);
    assert(!integrated.item_prompt && integrated.item_request);
    assert(!memcmp(&integrated.inventory, &unchanged_inventory, sizeof unchanged_inventory));
    integrated.item_request = false; // Native guest dispatch acknowledges requests outside menu_input.
    press_all(&integrated, AGE);
    assert(integrated.item_prompt && integrated.item_action == INVENTORY_REFILL && integrated.item_prompt_quantity == 30);
    assert(integrated.age == old_age && !integrated.travel_request && !integrated.preferences_dirty);
    press_all(&integrated, BACK);
    press_all(&integrated, FAVORITE);
    assert(integrated.item_prompt && integrated.item_action == INVENTORY_REMOVE && !integrated.item_prompt_quantity);
    press_all(&integrated, BACK);
    integrated.enabled |= 1u << ITEMS;
    press_all(&integrated, AGE);
    assert(!integrated.item_prompt && !integrated.item_request && integrated.notice);
    integrated.enabled &= ~(1u << ITEMS);
    integrated.inventory.entries[INV_BOMBS].age_allowed = false;
    press_all(&integrated, CONFIRM);
    assert(!integrated.item_prompt && integrated.notice);
    integrated.inventory.entries[INV_BOMBS].age_allowed = true;
    press_all(&integrated, AGE);
    assert(integrated.item_prompt);
    menu_input(&integrated, tick += 20, true, 0, true, ALL_AVAILABLE & ~(1u << INVENTORY), supported);
    assert(!integrated.item_prompt && !integrated.item_request);
    integrated.inventory.entries[INV_BOMBS].can_refill = integrated.inventory.entries[INV_BOMBS].can_remove = false;
    press_all(&integrated, AGE); press_all(&integrated, FAVORITE);
    assert(!integrated.item_prompt && !integrated.item_request);
    integrated.inventory.entries[INV_BOMBS].can_refill = integrated.inventory.entries[INV_BOMBS].can_remove = true;
    integrated.item_quantity = integrated.inventory.entries[INV_BOMBS].capacity;
    press_all(&integrated, RIGHT);
    assert(integrated.item_quantity == 30);
    integrated.item_quantity = 0;
    press_all(&integrated, LEFT);
    assert(!integrated.item_quantity);
    unsigned item_cursor = integrated.cursor, item_scroll = integrated.scroll;
    press_all(&integrated, TAB_RIGHT);
    assert(integrated.tab == MENU_CHEATS && integrated.cursor == cheat_cursor);
    press_all(&integrated, TAB_RIGHT);
    assert(integrated.tab == MENU_TRAVEL);
    integrated.travel_sections[old_age] = UINT64_MAX;
    integrated.cursor = row_index(&integrated, DESTINATION_ROW, 26);
    input_all(&integrated, 0);
    unsigned travel_cursor = integrated.cursor, travel_scroll = integrated.scroll;
    press_all(&integrated, TAB_RIGHT);
    assert(integrated.tab == MENU_ITEMS && integrated.cursor == item_cursor && integrated.scroll == item_scroll);
    assert(integrated.age_cursor[old_age] == travel_cursor && integrated.age_scroll[old_age] == travel_scroll);
    press_all(&integrated, TAB_LEFT);
    assert(integrated.tab == MENU_TRAVEL && integrated.cursor == travel_cursor && integrated.scroll == travel_scroll);
    press_all(&integrated, TAB_LEFT);
    assert(integrated.tab == MENU_CHEATS && integrated.cursor == cheat_cursor);
    press_all(&integrated, TAB_LEFT);
    assert(integrated.tab == MENU_ITEMS && integrated.cursor == item_cursor && integrated.scroll == item_scroll);
    press_all(&integrated, R3);
    assert(!integrated.open && integrated.tab == MENU_ITEMS);
    press_all(&integrated, R3);
    assert(integrated.open && integrated.tab == MENU_ITEMS && integrated.cursor == item_cursor);
    press_all(&integrated, AGE);
    assert(integrated.item_prompt);
    press_all(&integrated, R3);
    assert(!integrated.open && !integrated.item_prompt && !integrated.item_request);
    press_all(&integrated, R3);
    press_all(&integrated, BACK);
    menu_rows(&integrated, rows);
    assert(integrated.open && rows[integrated.cursor].kind == ITEM_SECTION &&
           rows[integrated.cursor].value == INV_AMMUNITION);
    assert(!(integrated.item_sections & (1u << INV_AMMUNITION)));
    press_all(&integrated, BACK);
    assert(!integrated.open && integrated.tab == MENU_ITEMS);
    press_all(&integrated, R3);
    assert(integrated.open && integrated.tab == MENU_ITEMS);
    integrated.cursor = row_index(&integrated, ITEM_SECTION, INV_WEAPONS);
    press_all(&integrated, BACK);
    assert(integrated.open && !integrated.cursor && !(integrated.item_sections & 1));
    menu_reset(&integrated);
    assert(integrated.tab == MENU_ITEMS && !integrated.open && !integrated.enabled && !integrated.damage);
    assert(!integrated.inventory.valid && !integrated.item_request && !integrated.item_prompt);
    assert(!integrated.preferences_dirty && integrated.preferences.damage == 3);
    assert(!memcmp(&untouched, &integrated.preferences, sizeof untouched));

    puts("Menu, 214 destinations, all favorites, transitions, corrupt preferences, three tabs, combat settings and confirmed add/remove ownership requests for all 20 items passed.");
    return 0;
}
