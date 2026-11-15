#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "preferences.h"
#include "inventory.h"

enum Cheat { GOD, RUPEES, ITEMS, MAGIC, SPEED, FLY, FLIGHT_SPEED, NOCLIP,
             ONE_HIT, DAMAGE, FREE_CAMERA, INVENTORY, CHEAT_COUNT };
enum { TRAVEL = CHEAT_COUNT };
enum MenuTab { MENU_CHEATS, MENU_TRAVEL, MENU_ITEMS, MENU_TAB_COUNT };

static inline float movement_speed_factor(unsigned index) {
    static const float factors[] = {1.0f, 1.5f, 2.0f, 3.0f};
    return index < 4 ? factors[index] : 1.0f;
}
enum Support { UNSUPPORTED, EXPERIMENTAL, VERIFIED };
enum Button {
    CONFIRM = 1u << 0, BACK = 1u << 1, R3 = 1u << 5,
    AGE = 1u << 2, FAVORITE = 1u << 3, TAB_LEFT = 1u << 6, TAB_RIGHT = 1u << 7,
    LEFT = 1u << 12, UP = 1u << 13, RIGHT = 1u << 14, DOWN = 1u << 15,
};

enum RowKind {
    CHEAT_ROW, CHEAT_SECTION, TRAVEL_SECTION, DESTINATION_ROW, RETURN_ROW,
    ITEM_SECTION, ITEM_ROW
};
enum CheatSection {
    PLAYER_CHEATS, MOVEMENT_CHEATS, COMBAT_CHEATS, CAMERA_WORLD,
    CHEAT_SECTION_COUNT
};
enum { ITEM_CATEGORY_COUNT = 5 };
extern const char *const cheat_section_names[CHEAT_SECTION_COUNT];
extern const char *const item_category_names[ITEM_CATEGORY_COUNT];
typedef struct { enum RowKind kind; unsigned value, parent; } MenuRow;
enum { MENU_ROWS = 2 * DESTINATION_COUNT + TRAVEL_GROUP_COUNT + 1, VISIBLE_ROWS = 9 };

typedef struct {
    uint32_t previous;
    uint64_t last, repeat;
    unsigned row, cursor, speed, flight_speed, enabled, alpha, tab, age;
    unsigned scroll, tab_cursor[MENU_TAB_COUNT], tab_scroll[MENU_TAB_COUNT], age_cursor[2], age_scroll[2];
    unsigned cheat_sections;
    unsigned damage, item_sections;
    unsigned item_edit_index, item_prompt_index;
    int item_quantity, item_prompt_quantity;
    enum InventoryAction item_action;
    InventorySnapshot inventory;
    uint64_t travel_sections[2];
    unsigned prompt_cheat, prompt_value;
    unsigned return_age;
    uint32_t travel_target, travel_request;
    bool initialized, open, connected, consent, prompt, travel_prompt, can_return;
    bool preferences_loaded, preferences_dirty, item_prompt, item_request;
    const char *notice;
    Preferences preferences;
} Menu;

// Time is monotonic milliseconds. 'available' is a bit mask of the currently safe rows.
void menu_input(Menu *menu, uint64_t now, bool connected, uint32_t buttons,
                bool can_open, unsigned available, const enum Support support[CHEAT_COUNT]);
void menu_reset(Menu *menu);
void menu_initialize(Menu *menu);
unsigned menu_rows(const Menu *menu, MenuRow *rows);
