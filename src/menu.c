#include "menu.h"

enum { FADE_MS = 175, REPEAT_DELAY_MS = 285, REPEAT_MS = 90 };

const char *const cheat_section_names[CHEAT_SECTION_COUNT] = {
    "Player Cheats", "Movement Cheats", "Combat Cheats", "Camera & World"
};
const char *const item_category_names[ITEM_CATEGORY_COUNT] = {
    "Weapons", "Shields", "Equipment", "Consumables", "Ammunition"
};

void menu_initialize(Menu *menu) {
    if (menu->initialized) return;
    menu->initialized = true;
    menu->cheat_sections = 3;
    menu->item_sections = 1;
    menu->item_edit_index = INVENTORY_COUNT;
    menu->cursor = menu->tab == MENU_ITEMS ? 0 : 1;
    menu->tab_cursor[MENU_CHEATS] = menu->tab_cursor[MENU_TRAVEL] = 1;
    menu->tab_cursor[MENU_ITEMS] = 0;
    menu->age_cursor[0] = menu->age_cursor[1] = 1;
}

void menu_reset(Menu *menu) {
    menu->open = false;
    menu->prompt = menu->travel_prompt = false;
    menu->travel_request = menu->travel_target = 0;
    menu->can_return = false;
    menu->notice = 0;
    menu->enabled = 0;
    menu->speed = 0;
    menu->alpha = 0;
    menu->repeat = 0;
    menu->damage = 0;
    menu->item_prompt = menu->item_request = false;
    menu->item_edit_index = INVENTORY_COUNT;
    menu->inventory.valid = false;
}

static void group_rows(const Menu *menu, unsigned group, MenuRow *rows, unsigned *count) {
    if (group != GROUP_FAVORITES && !travel_group_count(menu->age, group,
            menu->preferences.favorites, menu->preferences.favorite_count)) return;
    rows[(*count)++] = (MenuRow){TRAVEL_SECTION, group, travel_groups[group].parent};
    if (!(menu->travel_sections[menu->age] & (1ull << group))) return;
    bool children = false;
    for (unsigned i = 0; i < TRAVEL_GROUP_COUNT; ++i) {
        if (travel_groups[i].parent != group) continue;
        children = true;
        group_rows(menu, i, rows, count);
    }
    if (children) return;
    unsigned indices[DESTINATION_COUNT];
    unsigned found = travel_list(menu->age, group, menu->preferences.favorites,
                                 menu->preferences.favorite_count, indices);
    for (unsigned i = 0; i < found; ++i)
        rows[(*count)++] = (MenuRow){DESTINATION_ROW, indices[i], group};
}

unsigned menu_rows(const Menu *menu, MenuRow *rows) {
    unsigned count = 0;
    if (menu->tab == MENU_ITEMS) {
        for (unsigned category = 0; category < ITEM_CATEGORY_COUNT; ++category) {
            rows[count++] = (MenuRow){ITEM_SECTION, category, TRAVEL_ROOT};
            if (!(menu->item_sections & (1u << category))) continue;
            for (unsigned item = 0; item < INVENTORY_COUNT; ++item)
                if (inventory_catalog[item].category == category)
                    rows[count++] = (MenuRow){ITEM_ROW, item, category};
        }
        return count;
    }
    if (menu->tab == MENU_CHEATS) {
        for (unsigned section = 0; section < CHEAT_SECTION_COUNT; ++section) {
            rows[count++] = (MenuRow){CHEAT_SECTION, section, TRAVEL_ROOT};
            if (!(menu->cheat_sections & (1u << section))) continue;
            if (section <= MOVEMENT_CHEATS) {
                unsigned first = section ? SPEED : GOD, end = section ? NOCLIP + 1 : SPEED;
                for (unsigned cheat = first; cheat < end; ++cheat)
                    rows[count++] = (MenuRow){CHEAT_ROW, cheat, section};
            } else if (section == COMBAT_CHEATS) {
                rows[count++] = (MenuRow){CHEAT_ROW, ONE_HIT, section};
                rows[count++] = (MenuRow){CHEAT_ROW, DAMAGE, section};
            } else {
                rows[count++] = (MenuRow){CHEAT_ROW, FREE_CAMERA, section};
            }
        }
        return count;
    }
    rows[count++] = (MenuRow){RETURN_ROW, 0, TRAVEL_ROOT};
    for (unsigned group = 0; group < TRAVEL_GROUP_COUNT; ++group)
        if (travel_groups[group].parent == TRAVEL_ROOT) group_rows(menu, group, rows, &count);
    return count;
}

static void remember_position(Menu *menu) {
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(menu, rows);
    if (menu->cursor >= count) menu->cursor = count - 1;
    if (menu->scroll > menu->cursor) menu->scroll = menu->cursor;
    if (menu->cursor >= menu->scroll + VISIBLE_ROWS) menu->scroll = menu->cursor - VISIBLE_ROWS + 1;
    unsigned maximum = count > VISIBLE_ROWS ? count - VISIBLE_ROWS : 0;
    if (menu->scroll > maximum) menu->scroll = maximum;
    menu->tab_cursor[menu->tab] = menu->cursor;
    menu->tab_scroll[menu->tab] = menu->scroll;
    if (menu->tab == MENU_TRAVEL) {
        menu->age_cursor[menu->age] = menu->cursor;
        menu->age_scroll[menu->age] = menu->scroll;
    }
}

static void focus_row(Menu *menu, enum RowKind kind, unsigned value, unsigned parent) {
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(menu, rows);
    for (unsigned i = 0; i < count; ++i) {
        if (rows[i].kind == kind && rows[i].value == value && rows[i].parent == parent) {
            menu->cursor = i;
            return;
        }
    }
}

static bool collapse_group(Menu *menu, MenuRow selected) {
    unsigned group = selected.kind == TRAVEL_SECTION ? selected.value : selected.parent;
    if (group >= TRAVEL_GROUP_COUNT) return false;
    if (selected.kind == TRAVEL_SECTION && !(menu->travel_sections[menu->age] & (1ull << group)))
        group = travel_groups[group].parent;
    if (group >= TRAVEL_GROUP_COUNT) return false;
    menu->travel_sections[menu->age] &= ~(1ull << group);
    focus_row(menu, TRAVEL_SECTION, group, travel_groups[group].parent);
    return true;
}

static void travel_input(Menu *menu, MenuRow selected, uint32_t pressed, bool supported) {
    if (pressed & AGE) {
        menu->age ^= 1;
        menu->cursor = menu->age_cursor[menu->age];
        menu->scroll = menu->age_scroll[menu->age];
        return;
    }
    if (pressed & LEFT) { collapse_group(menu, selected); return; }
    if (selected.kind == TRAVEL_SECTION && (pressed & (CONFIRM | RIGHT))) {
        uint64_t mask = 1ull << selected.value;
        if (pressed & CONFIRM) menu->travel_sections[menu->age] ^= mask;
        else menu->travel_sections[menu->age] |= mask;
        return;
    }
    if (selected.kind == DESTINATION_ROW && (pressed & FAVORITE)) {
        uint32_t id = destinations[selected.value].id;
        bool was_favorite = preferences_favorite(&menu->preferences, id);
        menu->preferences_dirty |= preferences_toggle_favorite(&menu->preferences, id);
        menu->notice = was_favorite ? "Removed from Favorites." : "Added to Favorites.";
        MenuRow rows[MENU_ROWS];
        unsigned count = menu_rows(menu, rows);
        if (selected.parent == GROUP_FAVORITES && was_favorite) {
            if (menu->cursor >= count || rows[menu->cursor].kind != DESTINATION_ROW ||
                rows[menu->cursor].parent != GROUP_FAVORITES)
                focus_row(menu, TRAVEL_SECTION, GROUP_FAVORITES, TRAVEL_ROOT);
        } else {
            focus_row(menu, selected.kind, selected.value, selected.parent);
        }
        return;
    }
    if (!(pressed & CONFIRM) || !supported) return;
    if (selected.kind == DESTINATION_ROW) {
        const char *restriction = destination_restriction(selected.value);
        if (restriction) { menu->notice = restriction; return; }
        menu->travel_target = destinations[selected.value].id;
    }
    else if (selected.kind == RETURN_ROW && menu->can_return) menu->travel_target = UINT32_MAX;
    else return;
    menu->travel_prompt = true;
}

static void cheat_input(Menu *menu, unsigned available, const enum Support *support,
                        MenuRow selected, uint32_t pressed) {
    if (selected.kind == CHEAT_SECTION) {
        if (pressed & CONFIRM) menu->cheat_sections ^= 1u << selected.value;
        else if (pressed & RIGHT) menu->cheat_sections |= 1u << selected.value;
        else if (pressed & LEFT) menu->cheat_sections &= ~(1u << selected.value);
        return;
    }
    if ((pressed & LEFT) && selected.kind == CHEAT_ROW &&
            selected.value != SPEED && selected.value != FLIGHT_SPEED && selected.value != DAMAGE) {
        menu->cheat_sections &= ~(1u << selected.parent);
        focus_row(menu, CHEAT_SECTION, selected.parent, TRAVEL_ROOT);
        return;
    }
    menu->row = selected.value;
    unsigned mask = 1u << selected.value;
    if (!(available & mask) || support[selected.value] == UNSUPPORTED) return;
    int delta = (pressed & RIGHT ? 1 : 0) - (pressed & LEFT ? 1 : 0);
    if (selected.value == SPEED || selected.value == FLIGHT_SPEED || selected.value == DAMAGE) {
        unsigned *setting = selected.value == SPEED ? &menu->speed :
                            selected.value == DAMAGE ? &menu->damage : &menu->flight_speed;
        unsigned maximum = selected.value == FLIGHT_SPEED ? 5 : 3;
        int next = (int)*setting + delta;
        if (!delta || next < 0 || (unsigned)next > maximum) return;
        if (support[selected.value] == EXPERIMENTAL && !menu->consent) {
            menu->prompt_cheat = selected.value;
            menu->prompt_value = (unsigned)next;
            menu->prompt = true;
            return;
        }
        *setting = (unsigned)next;
        if (selected.value == SPEED) menu->preferences.movement_speed = menu->speed;
        else if (selected.value == FLIGHT_SPEED) menu->preferences.flight_speed = menu->flight_speed;
        else menu->preferences.damage = menu->damage;
        menu->preferences_dirty = true;
        return;
    }
    if (!(pressed & CONFIRM)) return;
    if (support[selected.value] == EXPERIMENTAL && !menu->consent) {
        menu->prompt_cheat = selected.value;
        menu->prompt = true;
        return;
    }
    menu->enabled ^= mask;
}

static void item_input(Menu *menu, MenuRow selected, uint32_t pressed, unsigned available) {
    if (selected.kind == ITEM_SECTION) {
        if (pressed & CONFIRM) menu->item_sections ^= 1u << selected.value;
        else if (pressed & RIGHT) menu->item_sections |= 1u << selected.value;
        else if (pressed & LEFT) menu->item_sections &= ~(1u << selected.value);
        return;
    }
    unsigned index = selected.value;
    if (index >= INVENTORY_COUNT) return;
    const InventoryEntry *entry = &menu->inventory.entries[index];
    if (menu->item_edit_index != index) {
        menu->item_edit_index = index;
        menu->item_quantity = entry->valid ? entry->count : 0;
    }
    if (!(pressed & (LEFT | RIGHT | CONFIRM | AGE | FAVORITE))) return;
    if (!(available & (1u << INVENTORY)) || !menu->inventory.valid || !entry->valid) {
        menu->notice = "Inventory unavailable. Wait for safe gameplay.";
        return;
    }
    if (!entry->age_allowed) { menu->notice = "This item is restricted to the other age."; return; }
    if (!entry->can_set) { menu->notice = "No verified safe inventory operation is available."; return; }
    if (menu->enabled & (1u << ITEMS)) { menu->notice = "Disable Infinite Items before editing."; return; }
    int delta = (pressed & RIGHT ? 1 : 0) - (pressed & LEFT ? 1 : 0);
    if (delta) {
        if (!inventory_catalog[index].quantity_editable) return;
        int next = menu->item_quantity + delta;
        if (next >= 0 && next <= entry->capacity) menu->item_quantity = next;
        return;
    }
    enum InventoryAction action = INVENTORY_SET;
    int quantity = menu->item_quantity;
    if (pressed & AGE) {
        if (!entry->can_refill) return;
        action = INVENTORY_REFILL;
        quantity = entry->capacity;
    } else if (pressed & FAVORITE) {
        if (!entry->can_remove) return;
        action = INVENTORY_REMOVE;
        quantity = 0;
    } else if (!(pressed & CONFIRM)) return;
    else if (!inventory_catalog[index].quantity_editable || quantity == entry->count) {
        action = entry->owned ? INVENTORY_REMOVE : INVENTORY_SET;
        quantity = entry->owned ? 0 : 1;
    }
    menu->item_prompt_index = index;
    menu->item_prompt_quantity = quantity;
    menu->item_action = action;
    menu->item_prompt = true;
}

static bool collapse_item_category(Menu *menu, MenuRow selected) {
    unsigned category = selected.kind == ITEM_SECTION ? selected.value : selected.parent;
    if (category >= ITEM_CATEGORY_COUNT || !(menu->item_sections & (1u << category))) return false;
    menu->item_sections &= ~(1u << category);
    focus_row(menu, ITEM_SECTION, category, TRAVEL_ROOT);
    return true;
}

void menu_input(Menu *menu, uint64_t now, bool connected, uint32_t buttons,
                bool can_open, unsigned available, const enum Support support[CHEAT_COUNT]) {
    menu_initialize(menu);
    uint64_t elapsed = now - menu->last;
    menu->last = now;
    unsigned step = elapsed >= FADE_MS ? FADE_MS : (unsigned)elapsed;
    if (menu->open) {
        menu->alpha += step;
        if (menu->alpha > FADE_MS) menu->alpha = FADE_MS;
    } else menu->alpha = menu->alpha > step ? menu->alpha - step : 0;
    if (!connected) { menu->connected = false; menu->repeat = 0; return; }
    if (!menu->connected) { menu->connected = true; menu->previous = buttons; }
    uint32_t pressed = buttons & ~menu->previous;
    menu->previous = buttons;
    if ((pressed & R3) && (menu->open || can_open)) {
        menu->open = !menu->open;
        menu->prompt = menu->travel_prompt = menu->item_prompt = false;
        menu->item_request = false;
        menu->repeat = 0;
        remember_position(menu);
        return;
    }
    if (!menu->open) return;
    if (pressed & ~R3) menu->notice = 0;
    if (pressed & BACK) {
        if (menu->item_prompt) menu->item_prompt = false;
        else if (menu->travel_prompt) menu->travel_prompt = false;
        else if (!menu->prompt && menu->tab == MENU_ITEMS) {
            MenuRow rows[MENU_ROWS];
            unsigned count = menu_rows(menu, rows);
            if (!collapse_item_category(menu, rows[menu->cursor < count ? menu->cursor : count - 1])) menu->open = false;
        } else if (!menu->prompt && menu->tab == MENU_TRAVEL) {
            MenuRow rows[MENU_ROWS];
            unsigned count = menu_rows(menu, rows);
            if (!collapse_group(menu, rows[menu->cursor < count ? menu->cursor : count - 1])) menu->open = false;
        } else menu->open = false;
        menu->prompt = false;
        menu->repeat = 0;
        remember_position(menu);
        return;
    }
    if (menu->item_prompt) {
        if (!(available & (1u << INVENTORY)) || !menu->inventory.valid ||
            menu->item_prompt_index >= INVENTORY_COUNT) menu->item_prompt = false;
        else if (pressed & CONFIRM) { menu->item_request = true; menu->item_prompt = false; }
        return;
    }
    if (menu->travel_prompt) {
        if (!(available & (1u << TRAVEL))) {
            menu->travel_prompt = false;
        } else if (pressed & CONFIRM) {
            menu->consent = true;
            menu->travel_request = menu->travel_target;
            menu->travel_prompt = menu->open = false;
        }
        return;
    }
    if (menu->prompt) {
        unsigned cheat = menu->prompt_cheat;
        if (cheat >= CHEAT_COUNT || !(available & (1u << cheat)) || support[cheat] == UNSUPPORTED) {
            menu->prompt = false;
        } else if (pressed & CONFIRM) {
            menu->consent = true;
            menu->prompt = false;
            if (cheat == SPEED || cheat == FLIGHT_SPEED || cheat == DAMAGE) {
                if (cheat == SPEED) menu->speed = menu->preferences.movement_speed = menu->prompt_value;
                else if (cheat == FLIGHT_SPEED) menu->flight_speed = menu->preferences.flight_speed = menu->prompt_value;
                else menu->damage = menu->preferences.damage = menu->prompt_value;
                menu->preferences_dirty = true;
            } else menu->enabled ^= 1u << cheat;
        }
        return;
    }
    if (pressed & (TAB_LEFT | TAB_RIGHT)) {
        remember_position(menu);
        menu->tab = (menu->tab + (pressed & TAB_RIGHT ? 1 : MENU_TAB_COUNT - 1)) % MENU_TAB_COUNT;
        menu->cursor = menu->tab_cursor[menu->tab];
        menu->scroll = menu->tab_scroll[menu->tab];
        menu->prompt = false;
        menu->repeat = 0;
        remember_position(menu);
        return;
    }
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(menu, rows);
    if (menu->cursor >= count) menu->cursor = count - 1;
    uint32_t vertical = buttons & (UP | DOWN);
    if (vertical == UP || vertical == DOWN) {
        if ((pressed & vertical) || (menu->repeat && now >= menu->repeat)) {
            menu->cursor = (menu->cursor + (vertical == UP ? count - 1 : 1)) % count;
            menu->repeat = now + ((pressed & vertical) ? REPEAT_DELAY_MS : REPEAT_MS);
            menu->prompt = false;
        }
    } else menu->repeat = 0;
    if (menu->tab == MENU_TRAVEL) travel_input(menu, rows[menu->cursor], pressed, available & (1u << TRAVEL));
    else if (menu->tab == MENU_ITEMS) item_input(menu, rows[menu->cursor], pressed, available);
    else cheat_input(menu, available, support, rows[menu->cursor], pressed);
    remember_position(menu);
}
