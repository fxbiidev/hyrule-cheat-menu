// Software renderer checks. No emulator, game assets or GPU/display claims.
#include "guest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uintptr_t main_base;
bool mapped(uintptr_t a, size_t n, unsigned p) { (void)a; (void)n; (void)p; return false; }
uintptr_t runtime_symbol(const char *s) { (void)s; return 0; }
void debug_log(const char *s) { (void)s; }
const char *storage_status(void) { return "Software fixture; no storage access."; }
#include "../src/overlay.c"

static void require(bool okay, const char *message) {
    if (!okay) { fprintf(stderr, "%s\n", message); exit(1); }
}
static void guards(const uint32_t *allocation, size_t count) {
    for (unsigned i = 0; i < 16; ++i)
        require(allocation[i] == 0xdeadbeef && allocation[count + 16 + i] == 0xdeadbeef,
                "Panel renderer wrote outside its pixel buffer");
}

static unsigned find_row(Menu *menu, enum RowKind kind, unsigned value) {
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(menu, rows);
    for (unsigned i = 0; i < count; ++i)
        if (rows[i].kind == kind && rows[i].value == value) return i;
    require(false, "Required renderer state row is missing");
    return 0;
}

enum { ALL_AVAILABLE = (1u << (TRAVEL + 1)) - 1, FIXTURE_STATES = 49 };

static Menu fixture_menu(unsigned state) {
    Menu menu = {.consent = true, .enabled = 175, .speed = 3, .flight_speed = 5,
                 .alpha = 175, .open = true};
    menu_initialize(&menu);
    menu.cheat_sections = (1u << CHEAT_SECTION_COUNT) - 1;
    menu.inventory = (InventorySnapshot){.valid = true, .epoch = 7, .pouch = 1, .actor = 2, .child = true};
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i) {
        InventoryEntry *entry = &menu.inventory.entries[i];
        bool quantity = inventory_catalog[i].quantity_editable;
        entry->valid = true; entry->owned = i != INV_MIRROR_SHIELD;
        entry->age_allowed = (inventory_catalog[i].ages & INV_CHILD) != 0;
        entry->count = entry->owned ? quantity ? 3 : 1 : 0;
        entry->capacity = i == INV_MAGIC_BEANS ? 10 : quantity ? 30 : 1;
        entry->can_set = entry->age_allowed;
        entry->can_refill = quantity && entry->age_allowed && entry->count < entry->capacity;
        entry->can_remove = entry->age_allowed && entry->owned;
    }
    if (state == 0) menu.cursor = 1;
    else if (state == 1) menu.cursor = find_row(&menu, CHEAT_ROW, NOCLIP);
    else if (state == 2) { menu.cheat_sections = 0; menu.cursor = CHEAT_SECTION_COUNT - 1; }
    else if (state >= 3 && state <= 7) {
        if (state & 1) menu.enabled |= 1u << ONE_HIT;
        menu.damage = state >= 4 ? state - 4 : 0;
        menu.cursor = find_row(&menu, CHEAT_ROW, state == 3 ? ONE_HIT : DAMAGE);
    } else if (state == 8) {
        menu.enabled |= 1u << FREE_CAMERA;
        menu.cursor = find_row(&menu, CHEAT_ROW, FREE_CAMERA);
    } else if ((state >= 9 && state <= 12) || state >= 22) {
        menu.tab = MENU_ITEMS;
        menu.enabled = 0;
        menu.item_sections = state == 11 ? 0 : (1u << ITEM_CATEGORY_COUNT) - 1;
        menu.cursor = state == 11 ? ITEM_CATEGORY_COUNT - 1 :
            find_row(&menu, ITEM_ROW, state == 9 ? INV_KOKIRI_SWORD : INV_BOMBCHUS);
        if (state == 12) menu.notice = "Native change failed. No automatic retry.";
        if (state == 22) menu.notice = "AReallyLongUnbrokenNoticeThatMustWrapAndNeverWriteOutsideThePanelBoundaryEvenAtTheLargestScale";
        if (state == 23 || state == 24) {
            menu.item_edit_index = INV_BOMBCHUS; menu.item_quantity = 15;
            if (state == 24) {
                menu.item_prompt = true; menu.item_prompt_index = INV_BOMBCHUS;
                menu.item_prompt_quantity = 15; menu.item_action = INVENTORY_SET;
            }
        } else if (state == 25) {
            menu.cursor = find_row(&menu, ITEM_ROW, INV_ARROWS);
        } else if (state == 26) menu.inventory.valid = false;
        else if (state == 27) menu.enabled = 1u << ITEMS;
        else if (state == 28) {
            menu.inventory.entries[INV_BOMBCHUS].owned = false;
            menu.inventory.entries[INV_BOMBCHUS].count = 0;
            menu.inventory.entries[INV_BOMBCHUS].can_set = true;
            menu.inventory.entries[INV_BOMBCHUS].can_refill = true;
            menu.inventory.entries[INV_BOMBCHUS].can_remove = false;
        } else if (state == 29) {
            menu.inventory.entries[INV_BOMBCHUS].capacity = 999;
            menu.inventory.entries[INV_BOMBCHUS].count = 999;
        } else if (state == 30) {
            menu.inventory.entries[INV_BOMBCHUS].valid = false;
            menu.inventory.entries[INV_BOMBCHUS].count = -1;
        } else if (state >= 31 && state <= 35) {
            menu.cursor = find_row(&menu, ITEM_ROW, INV_GORON_TUNIC);
            InventoryEntry *entry = &menu.inventory.entries[INV_GORON_TUNIC];
            menu.inventory.child = state == 35;
            entry->age_allowed = entry->can_set = state != 35;
            entry->owned = state != 32 && state != 33;
            entry->count = entry->owned ? 1 : 0;
            entry->can_remove = entry->owned && entry->age_allowed;
            if (state == 33 || state == 34) {
                menu.item_prompt = true; menu.item_prompt_index = INV_GORON_TUNIC;
                menu.item_prompt_quantity = state == 33 ? 1 : 0;
                menu.item_action = state == 33 ? INVENTORY_SET : INVENTORY_REMOVE;
            }
        } else if (state == 36) {
            InventoryEntry *entry = &menu.inventory.entries[INV_BOMBCHUS];
            entry->count = entry->capacity = 0;
            entry->owned = entry->can_set = entry->can_refill = entry->can_remove = false;
        } else if (state == 37) {
            menu.inventory.entries[INV_BOMBCHUS].can_set = false;
            menu.notice = "Inventory uncertain. Reload a save before further edits.";
        } else if (state == 38) {
            menu.notice = "Applying native inventory change...";
        } else if (state >= 39 && state <= 46) {
            unsigned index = state < 43 ? INV_MAGIC_BEANS : INV_DEKU_SEEDS;
            unsigned variant = (state - 39) % 4;
            menu.cursor = find_row(&menu, ITEM_ROW, index);
            InventoryEntry *entry = &menu.inventory.entries[index];
            entry->count = variant == 0 ? 0 : entry->capacity;
            entry->owned = entry->can_remove = entry->count > 0;
            entry->can_refill = entry->count < entry->capacity;
            if (variant == 2) {
                menu.item_prompt = true; menu.item_prompt_index = index;
                menu.item_prompt_quantity = 0; menu.item_action = INVENTORY_REMOVE;
            } else if (variant == 3) {
                menu.inventory.child = false;
                entry->age_allowed = entry->can_set = entry->can_remove = entry->can_refill = false;
            }
        } else if (state == 47 || state == 48) {
            menu.cursor = find_row(&menu, ITEM_ROW, INV_DEKU_SEEDS);
            InventoryEntry *entry = &menu.inventory.entries[INV_DEKU_SEEDS];
            entry->count = 0; entry->capacity = 30;
            entry->owned = entry->can_remove = false;
            entry->can_set = entry->can_refill = entry->needs_seed_bag = true;
            if (state == 48) {
                menu.item_prompt = true; menu.item_prompt_index = INV_DEKU_SEEDS;
                menu.item_prompt_quantity = 1; menu.item_action = INVENTORY_SET;
            }
        }
    } else if (state >= 13 && state <= 17) {
        menu.tab = MENU_TRAVEL;
        menu.age = state & 1;
        menu.travel_sections[menu.age] = UINT64_MAX;
        preferences_toggle_favorite(&menu.preferences, destinations[114].id);
        MenuRow rows[MENU_ROWS];
        unsigned count = menu_rows(&menu, rows);
        menu.cursor = state == 14 ? count - 1 : 12;
        menu.travel_prompt = state >= 16;
        menu.travel_target = state == 17 ? UINT32_MAX : destinations[114].id;
        menu.return_age = !menu.age;
        menu.can_return = true;
    } else if (state == 18) {
        menu.prompt = true;
        menu.cursor = find_row(&menu, CHEAT_ROW, GOD);
    } else if (state == 19) {
        menu.notice = "A long status remains clipped inside the panel's text boundary, with no buffer overrun. This text exercises all context lines.";
    } else if (state == 20) {
        menu.cursor = find_row(&menu, CHEAT_ROW, MAGIC);
    } else if (state == 21) {
        menu.tab = MENU_TRAVEL;
        menu.cursor = find_row(&menu, TRAVEL_SECTION, GROUP_FAVORITES);
    }
    menu.scroll = menu.cursor >= VISIBLE_ROWS ? menu.cursor - VISIBLE_ROWS + 1 : 0;
    return menu;
}

static void cache_revision_checks(void) {
    Menu menu = fixture_menu(0);
    overlay_publish(&menu, ALL_AVAILABLE, 0);
    uint32_t baseline = published;
    overlay_publish(&menu, ALL_AVAILABLE, 0);
    require(published == baseline, "Unchanged menu invalidated its cached panel");
    #define CHECK_CHANGE(member, value) do { \
        Menu before = menu; menu.member = (value); \
        overlay_publish(&menu, ALL_AVAILABLE, 0); \
        require(published != baseline, "Visible state change failed to invalidate cached frames: " #member); \
        menu = before; overlay_publish(&menu, ALL_AVAILABLE, 0); \
        require(published == baseline, "Restored state failed to restore cache identity: " #member); \
    } while (0)
    CHECK_CHANGE(damage, 1);
    CHECK_CHANGE(enabled, menu.enabled ^ (1u << ONE_HIT));
    CHECK_CHANGE(tab, MENU_ITEMS);
    CHECK_CHANGE(item_sections, menu.item_sections ^ 1);
    CHECK_CHANGE(inventory.valid, false);
    CHECK_CHANGE(item_edit_index, INV_BOMBCHUS);
    CHECK_CHANGE(item_quantity, 1);
    CHECK_CHANGE(item_prompt, true);
    CHECK_CHANGE(item_prompt_index, INV_BOMBCHUS);
    CHECK_CHANGE(item_prompt_quantity, 1);
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i) {
        CHECK_CHANGE(inventory.entries[i].count, menu.inventory.entries[i].count + 1);
        CHECK_CHANGE(inventory.entries[i].capacity, menu.inventory.entries[i].capacity + 1);
        CHECK_CHANGE(inventory.entries[i].valid, !menu.inventory.entries[i].valid);
        CHECK_CHANGE(inventory.entries[i].owned, !menu.inventory.entries[i].owned);
        CHECK_CHANGE(inventory.entries[i].age_allowed, !menu.inventory.entries[i].age_allowed);
        CHECK_CHANGE(inventory.entries[i].can_set, !menu.inventory.entries[i].can_set);
        CHECK_CHANGE(inventory.entries[i].can_refill, !menu.inventory.entries[i].can_refill);
        CHECK_CHANGE(inventory.entries[i].can_remove, !menu.inventory.entries[i].can_remove);
        CHECK_CHANGE(inventory.entries[i].needs_seed_bag, !menu.inventory.entries[i].needs_seed_bag);
    }
    #undef CHECK_CHANGE
}

int main(void) {
    cache_revision_checks();
    uint32_t ownership_frames[4] = {0};
    for (unsigned scale = 1; scale <= MAX_SCALE; ++scale) {
        size_t count = WIDTH * HEIGHT * scale * scale;
        uint32_t *allocation = malloc((count + 32) * sizeof(uint32_t));
        uint32_t *scene = malloc(count * sizeof(uint32_t));
        require(allocation && scene, "Fixture allocation failed");
        for (size_t i = 0; i < count + 32; ++i) allocation[i] = 0xdeadbeef;
        for (size_t i = 0; i < count; ++i) scene[i] = 0xffffffff;
        const int formats[] = {RGBA8, RGBX8_SRGB, RGBA8_SRGB, RGB10A2};
        for (unsigned f = 0; f < sizeof formats / sizeof *formats; ++f) {
            Canvas canvas = {allocation + 16, (int)scale, formats[f]};
            for (unsigned state = 0; state < FIXTURE_STATES; ++state) {
                Menu menu = fixture_menu(state);
                overlay_publish(&menu, state == 20 ? 0 : ALL_AVAILABLE, state == 18 ? ALL_AVAILABLE : 0);
                draw_panel(&canvas, 175);
                guards(allocation, count);
                if (scale == 1 && f == 0 && state >= 31 && state <= 34) {
                    uint32_t hash = 2166136261u;
                    for (size_t i = 0; i < count; ++i) hash = (hash ^ canvas.pixels[i]) * 16777619u;
                    ownership_frames[state - 31] = hash;
                }
                blend_panel(&canvas, scene, 0);
                for (size_t i = 0; i < count; i += 127)
                    require(canvas.pixels[i] == scene[i], "Fade zero did not preserve the background");
                draw_panel(&canvas, 175);
                // The empty gradient pixel retains 217/255 (~85%) software coverage.
                size_t sample = 2 * scale * (WIDTH * scale) + 2 * scale;
                uint32_t ink = canvas.pixels[sample];
                unsigned alpha = ink >> 24;
                require(alpha == 217, "Panel background no longer retains 85% software coverage");
                blend_panel(&canvas, scene, 175);
                unsigned bits = formats[f] == RGB10A2 ? 10 : 8;
                unsigned maximum = (1u << bits) - 1;
                for (unsigned channel = 0; channel < 3; ++channel) {
                    unsigned source = ((ink >> (channel * 8)) & 255) * maximum / 255;
                    unsigned background_channel = (scene[sample] >> (channel * bits)) & maximum;
                    unsigned expected = (source * alpha + background_channel * (255 - alpha) + 127) / 255;
                    unsigned actual = (canvas.pixels[sample] >> (channel * bits)) & maximum;
                    require(actual == expected, "Panel opacity did not match native-format alpha/background arithmetic");
                    require(actual != source, "Background contribution was lost during transparency conversion");
                }
                guards(allocation, count);
                draw_panel(&canvas, 175);
                ink = canvas.pixels[0];
                encode_panel(&canvas);
                uint32_t encoded = canvas.pixels[0];
                if (formats[f] == RGB10A2) {
                    require((encoded >> 30) == 3, "Fallback native alpha conversion failed");
                    for (unsigned channel = 0; channel < 3; ++channel)
                        require(((encoded >> (channel * 10)) & 1023) ==
                                (((ink >> (channel * 8)) & 255) * 1023 / 255),
                                "Fallback native channel conversion failed");
                } else require(encoded == ink, "Eight-bit fallback unexpectedly changed");
                guards(allocation, count);
            }
        }
        free(scene); free(allocation);
    }
    require(ownership_frames[0] != ownership_frames[1], "Owned and unowned equipment produced identical frames");
    require(ownership_frames[2] != ownership_frames[3], "Add and remove equipment confirmations produced identical frames");
    puts("Production software panel: six scales, four formats, 49 states including first-seed-bag context/confirmation, ownership/draft/confirmation cache invalidation, buffer guards and exact 85% blending passed (not a display test).");
}
