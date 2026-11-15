#include "guest.h"

enum {
    WIDTH = 420, HEIGHT = 288, MAX_SCALE = 6, FRAME_COUNT = 3,
    BYTES_PER_FRAME = ((WIDTH * HEIGHT * MAX_SCALE * MAX_SCALE * 4 + 4095) / 4096) * 4096,
    COMMAND_BYTES = 4096, FRAME_STRIDE = BYTES_PER_FRAME + COMMAND_BYTES, MAX_TEXTURES = 8,
    MAX_WIDTH = 5120, MAX_HEIGHT = 2160, BACKGROUND_OFFSET = FRAME_COUNT * FRAME_STRIDE,
    READBACK_OFFSET = BACKGROUND_OFFSET + MAX_WIDTH * MAX_HEIGHT * 4,
    BACKING_BYTES = READBACK_OFFSET + BYTES_PER_FRAME,
    RGBA8 = 0x25, RGBX8_SRGB = 0x37, RGBA8_SRGB = 0x38, RGB10A2 = 0x3d,
    TEXTURE_2D = 1,
};

typedef struct { int x, y, z, width, height, depth; } CopyRegion;
typedef struct { int x, y, width, height; } Rect;
static struct {
    void (*present)(void *, void *, int);
    void (*set_textures)(void *, int, void *const *);
    unsigned (*window_init)(void *, const void *);
    void (*window_fini)(void *);
    void (*window_crop)(const void *, Rect *);
    int (*width)(const void *), (*height)(const void *), (*format)(const void *);
    int (*target)(const void *), (*samples)(const void *);
    void (*builder_defaults)(void *);
    void (*builder_device)(void *, void *);
    void (*builder_storage)(void *, void *, size_t);
    void (*builder_flags)(void *, int);
    unsigned (*pool_init)(void *, const void *);
    uint64_t (*pool_address)(const void *);
    void *(*pool_map)(const void *);
    void (*flush)(const void *, ptrdiff_t, size_t);
    void (*invalidate)(const void *, ptrdiff_t, size_t);
    unsigned (*sync_init)(void *, void *);
    unsigned (*sync_wait)(const void *, uint64_t);
    void (*fence)(void *, void *, int, int);
    unsigned (*command_init)(void *, void *);
    void (*add_command_memory)(void *, const void *, ptrdiff_t, size_t);
    void (*add_control_memory)(void *, void *, size_t);
    void (*begin)(void *);
    uint64_t (*end)(void *);
    void (*submit)(void *, int, const uint64_t *);
    void (*set_stride)(void *, ptrdiff_t);
    void (*copy)(void *, uint64_t, void *, const void *, const CopyRegion *, int);
    void (*capture)(void *, const void *, const void *, const CopyRegion *, uint64_t, int);
    void (*barrier)(void *, int);
} nvn;

static uint8_t *backing;
static _Alignas(16) uint8_t pool[512], syncs[FRAME_COUNT][128], readback_sync[128];
static _Alignas(16) uint8_t commands[FRAME_COUNT][512], controls[FRAME_COUNT][COMMAND_BYTES];
static unsigned next_frame;
static bool submitted[FRAME_COUNT];
static uint32_t last_flags[FRAME_COUNT];
static unsigned last_capture[FRAME_COUNT];
static int last_scale[FRAME_COUNT], last_format[FRAME_COUNT];
static uint64_t gpu_address;
static uint8_t *cpu_address;
static uint64_t last_surface;
static uint32_t published;
static unsigned panel_lock, panel_available, panel_experimental;
static Menu panel_menu;
static unsigned capture_request, capture_done, capture_serial;
static struct {
    void *queue;
    Rect crop;
    int format;
    bool checked, readable;
} background;
static bool surface_reported;
static void *overlay_device;
static struct {
    const void *builder;
    void *window;
    void *pending[MAX_TEXTURES], *textures[MAX_TEXTURES];
    int pending_count, count;
} surface;

void overlay_publish(const Menu *menu, unsigned available, unsigned experimental) {
    if (__atomic_exchange_n(&panel_lock, 1, __ATOMIC_ACQUIRE)) return;
    panel_menu = *menu;
    panel_available = available;
    panel_experimental = experimental;
    uint32_t state[] = {menu->cursor, menu->speed, menu->flight_speed, menu->enabled,
                        menu->tab, menu->age, menu->cheat_sections, menu->scroll, menu->return_age,
                        (uint32_t)menu->travel_sections[menu->age],
                        (uint32_t)(menu->travel_sections[menu->age] >> 32),
                        menu->prompt, menu->travel_prompt, menu->travel_target, menu->can_return,
                        available, experimental, (uint32_t)(uintptr_t)menu->notice,
                        menu->damage, menu->item_sections, menu->inventory.valid,
                        menu->item_edit_index, (uint32_t)menu->item_quantity, menu->item_prompt,
                        menu->item_prompt_index, (uint32_t)menu->item_prompt_quantity};
    uint32_t revision = 2166136261u;
    for (unsigned i = 0; i < sizeof state / sizeof *state; ++i)
        revision = (revision ^ state[i]) * 16777619u;
    for (unsigned i = 0; i < menu->preferences.favorite_count; ++i)
        revision = (revision ^ menu->preferences.favorites[i]) * 16777619u;
    for (unsigned i = 0; i < INVENTORY_COUNT; ++i) {
        const InventoryEntry *entry = &menu->inventory.entries[i];
        revision = (revision ^ (unsigned)entry->count) * 16777619u;
        revision = (revision ^ (unsigned)entry->capacity) * 16777619u;
        revision = (revision ^ (entry->valid | entry->owned << 1 | entry->age_allowed << 2 |
                    entry->can_set << 3 | entry->can_refill << 4 | entry->can_remove << 5 |
                    entry->needs_seed_bag << 6)) * 16777619u;
    }
    unsigned packed = menu->alpha | revision << 8;
    __atomic_store_n(&panel_lock, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&published, packed, __ATOMIC_RELEASE);
}

bool overlay_ready(void) {
    uint64_t frame = __atomic_load_n(&last_surface, __ATOMIC_ACQUIRE);
    return frame && monotonic_ms() - frame < 1000;
}

void overlay_begin_capture(void) {
    if (++capture_serial == 0) ++capture_serial;
    __atomic_store_n(&capture_request, capture_serial, __ATOMIC_RELEASE);
}

bool overlay_captured(void) {
    unsigned done = __atomic_load_n(&capture_done, __ATOMIC_ACQUIRE);
    return done && done == __atomic_load_n(&capture_request, __ATOMIC_ACQUIRE);
}

void overlay_end_capture(void) {
    __atomic_store_n(&capture_request, 0, __ATOMIC_RELEASE);
}

// Original 5-by-7 lettering, stored as row bitmasks.
static const uint8_t alphabet[][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4}, {17,17,17,21,21,27,17}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
};
static const uint8_t numerals[][7] = {
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14},
};
static const uint8_t footer_lowercase[26][7] = {
    ['a' - 'a'] = {0,0,14,1,15,17,15}, ['b' - 'a'] = {16,16,22,25,17,17,30},
    ['c' - 'a'] = {0,0,14,17,16,17,14},
    ['d' - 'a'] = {1,1,13,19,17,17,15}, ['e' - 'a'] = {0,0,14,17,31,16,15},
    ['f' - 'a'] = {6,9,8,28,8,8,8}, ['i' - 'a'] = {4,0,12,4,4,4,14},
    ['g' - 'a'] = {0,0,15,17,15,1,14}, ['h' - 'a'] = {16,16,22,25,17,17,17},
    ['j' - 'a'] = {2,0,6,2,2,18,12}, ['k' - 'a'] = {16,16,18,20,24,20,18},
    ['l' - 'a'] = {12,4,4,4,4,4,14}, ['m' - 'a'] = {0,0,26,21,21,21,21},
    ['n' - 'a'] = {0,0,22,25,17,17,17}, ['o' - 'a'] = {0,0,14,17,17,17,14},
    ['p' - 'a'] = {0,0,30,17,30,16,16}, ['q' - 'a'] = {0,0,15,17,15,1,1},
    ['r' - 'a'] = {0,0,22,25,16,16,16}, ['t' - 'a'] = {8,8,28,8,8,9,6},
    ['s' - 'a'] = {0,0,15,16,14,1,30}, ['u' - 'a'] = {0,0,17,17,17,19,13},
    ['v' - 'a'] = {0,0,17,17,17,10,4}, ['x' - 'a'] = {0,0,17,10,4,10,17},
    ['y' - 'a'] = {0,0,17,17,15,1,14},
    ['w' - 'a'] = {0,0,17,17,21,21,10}, ['z' - 'a'] = {0,0,31,2,4,8,31},
};
static const uint8_t dot[7] = {0,0,0,0,0,6,6};
static const uint8_t hyphen[7] = {0,0,0,14,0,0,0};
static const uint8_t slash[7] = {1,1,2,4,8,16,16};
static const uint8_t apostrophe[7] = {4,4,8,0,0,0,0};
static const uint8_t question[7] = {14,17,1,2,4,0,4};
static const uint8_t colon[7] = {0,4,4,0,4,4,0};
static const uint8_t left_paren[7] = {2,4,8,8,8,4,2};
static const uint8_t right_paren[7] = {8,4,2,2,2,4,8};
static const uint8_t ampersand[7] = {12,18,20,8,21,18,13};

typedef struct { uint32_t *pixels; int scale, format; } Canvas;

static uint32_t pixel(Canvas *c, unsigned rgb, unsigned alpha) {
    (void)c;
    unsigned r = rgb >> 16, g = (rgb >> 8) & 255, b = rgb & 255;
    // Keep software coverage at eight bits even for a two-bit-alpha swapchain.
    // Quantizing 85% to RGB10A2 first would round it to completely opaque.
    return r | g << 8 | b << 16 | alpha << 24;
}

static void rect(Canvas *c, int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > WIDTH) w = WIDTH - x;
    if (y + h > HEIGHT) h = HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    int scale = c->scale;
    int stride = WIDTH * scale;
    for (int line = y * scale; line < (y + h) * scale; ++line) {
        uint32_t *row = c->pixels + line * stride + x * scale;
        for (int col = 0; col < w * scale; ++col) row[col] = color;
    }
}

static void label(Canvas *c, int x, int y, const char *text, uint32_t color) {
    for (; *text && x <= WIDTH - 5; ++text, x += 6) {
        unsigned ch = (unsigned char)*text;
        const uint8_t *glyph = 0;
        if (ch >= 'A' && ch <= 'Z') glyph = alphabet[ch - 'A'];
        else if (ch >= 'a' && ch <= 'z') glyph = footer_lowercase[ch - 'a'];
        else if (ch >= '0' && ch <= '9') glyph = numerals[ch - '0'];
        else if (ch == '.') glyph = dot;
        else if (ch == '-') glyph = hyphen;
        else if (ch == '/') glyph = slash;
        else if (ch == '\'') glyph = apostrophe;
        else if (ch == '?') glyph = question;
        else if (ch == ':') glyph = colon;
        else if (ch == '(') glyph = left_paren;
        else if (ch == ')') glyph = right_paren;
        else if (ch == '&') glyph = ampersand;
        if (!glyph) continue;
        for (unsigned row = 0; row < 7; ++row)
            for (unsigned col = 0; col < 5; ++col)
                if (glyph[row] & (16u >> col)) rect(c, x + (int)col, y + (int)row, 1, 1, color);
    }
}

static unsigned append_text(char *out, unsigned used, unsigned capacity, const char *text) {
    while (*text && used + 1 < capacity) out[used++] = *text++;
    out[used] = 0;
    return used;
}

static void group_name(char *out, unsigned capacity, const Menu *menu, unsigned group) {
    unsigned used = append_text(out, 0, capacity, travel_groups[group].name);
    unsigned count = travel_group_count(menu->age, group, menu->preferences.favorites,
                                        menu->preferences.favorite_count);
    used = append_text(out, used, capacity, " (");
    char number[4];
    unsigned length = 0;
    if (count >= 100) number[length++] = '0' + count / 100;
    if (count >= 10) number[length++] = '0' + count / 10 % 10;
    number[length++] = '0' + count % 10;
    number[length] = 0;
    used = append_text(out, used, capacity, number);
    append_text(out, used, capacity, ")");
}

static int paragraph(Canvas *c, int x, int y, const char *text, unsigned lines, uint32_t color) {
    while (*text && lines--) {
        while (*text == ' ') ++text;
        unsigned length = 0, space = 0;
        while (text[length] && length < 18) {
            if (text[length] == ' ') space = length;
            ++length;
        }
        if (text[length] && space) length = space;
        char line[19];
        for (unsigned i = 0; i < length; ++i) line[i] = text[i];
        line[length] = 0;
        label(c, x, y, line, color);
        text += length;
        y += 11;
    }
    return y;
}

static void title(Canvas *c, int x, int y, const char *text, uint32_t color) {
    for (; *text; ++text, x += 12) {
        if (*text < 'A' || *text > 'Z') continue;
        const uint8_t *glyph = alphabet[*text - 'A'];
        for (unsigned row = 0; row < 7; ++row)
            for (unsigned col = 0; col < 5; ++col)
                if (glyph[row] & (16u >> col)) rect(c, x + col * 2, y + row * 2, 2, 2, color);
    }
}

static void number_text(char *out, unsigned value) {
    char reverse[11];
    unsigned length = 0;
    do { reverse[length++] = '0' + value % 10; value /= 10; } while (value && length < sizeof reverse);
    unsigned used = 0;
    while (length) out[used++] = reverse[--length];
    out[used] = 0;
}

static void quantity_text(char *out, unsigned capacity, int count, int maximum) {
    char number[12];
    number_text(number, count >= 0 ? (unsigned)count : 0);
    unsigned used = append_text(out, 0, capacity, number);
    used = append_text(out, used, capacity, "/");
    number_text(number, maximum >= 0 ? (unsigned)maximum : 0);
    append_text(out, used, capacity, number);
}

static void draw_panel(Canvas *c, uint32_t flags) {
    (void)flags;
    while (__atomic_exchange_n(&panel_lock, 1, __ATOMIC_ACQUIRE)) { }
    Menu menu = panel_menu;
    unsigned available = panel_available, experimental = panel_experimental;
    __atomic_store_n(&panel_lock, 0, __ATOMIC_RELEASE);
    static const char *const names[CHEAT_COUNT] = {
        "God Mode", "Infinite Rupees", "Infinite Items", "Infinite Magic", "Movement Speed",
        "Fly Mode", "Flight Speed", "Noclip", "One Hit Kill", "Damage Multiplier", "Free Camera", "Item Manager"
    };
    static const char *const speeds[] = {"1x", "1.5x", "2x", "3x", "5x", "10x"};
    static const char *const detail[CHEAT_COUNT][2] = {
        {"Refills your existing hearts.", "Protects ordinary life damage."},
        {"Fills the current wallet.", "Respects unlocked capacity."},
        {"Refills eligible owned ammunition.", "Respects age and capacity upgrades."},
        {"Refills an unlocked magic meter.", "Requires the fairy magic upgrade."},
        {"Scales ordinary movement.", "Walking, running, swimming and climbing."},
        {"Left stick moves. RT up, LT down.", "OFF lands naturally and protects one fall."},
        {"Independent flight movement speed.", "Six settings from 1x to 10x."},
        {"Pass walls on foot. Add Fly for full clip.", "OFF restores checked collision."},
        {"One valid hit defeats supported ordinary enemies.", "Bosses and scripted actors stay protected."},
        {"Scales supported attacks from Link.", "One Hit Kill overrides; your multiplier stays selected."},
        {"Left stick moves, right stick rotates. RT up, LT down.", "R3 and B together exit at any time."},
        {"Safe inventory tools.", "Confirmed ownership and quantity changes."}
    };
    uint32_t gold = pixel(c, 0xd8ba78, 255), bright = pixel(c, 0xf0ebd8, 255);
    uint32_t muted = pixel(c, 0x97b3a5, 255), green = pixel(c, 0x91dfb2, 255);
    uint32_t trim = pixel(c, 0x466c58, 255);
    for (int y = 0; y < HEIGHT; ++y) {
        unsigned r = 10 - 3 * y / HEIGHT, g = 37 - 9 * y / HEIGHT, b = 30 - 5 * y / HEIGHT;
        rect(c, 0, y, WIDTH, 1, pixel(c, r << 16 | g << 8 | b, 217));
    }
    rect(c, 0, 0, WIDTH, 1, gold); rect(c, 0, HEIGHT - 1, WIDTH, 1, gold);
    rect(c, 0, 1, 1, HEIGHT - 2, trim); rect(c, WIDTH - 1, 1, 1, HEIGHT - 2, trim);
    rect(c, 5, 5, WIDTH - 10, 1, trim); rect(c, 5, HEIGHT - 6, WIDTH - 10, 1, trim);
    for (int i = 0; i < 7; ++i) {
        rect(c, 26 - i, 13 + i, 1 + i * 2, 1, gold);
        rect(c, 18 - i, 22 + i, 1 + i * 2, 1, gold);
        rect(c, 34 - i, 22 + i, 1 + i * 2, 1, gold);
    }
    title(c, 52, 13, "HYRULE CHEAT MENU", bright);
    label(c, 52, 35, "STANDALONE CHEAT MENU FOR OOTR", gold);
    rect(c, 12, 50, WIDTH - 24, 1, trim);
    unsigned active_tab = menu.tab < MENU_TAB_COUNT ? menu.tab : MENU_CHEATS;
    rect(c, 12 + active_tab * 134, 54, 128, 20, pixel(c, 0x224e3a, 244));
    rect(c, 12 + active_tab * 134, 72, 128, 2, gold);
    label(c, 58, 60, "CHEATS", menu.tab == MENU_CHEATS ? bright : muted);
    label(c, 177, 60, "FAST TRAVEL", menu.tab == MENU_TRAVEL ? bright : muted);
    label(c, 308, 60, "ITEM MANAGER", menu.tab == MENU_ITEMS ? bright : muted);
    unsigned age = menu.travel_prompt && menu.travel_target == UINT32_MAX ? menu.return_age : menu.age;
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(&menu, rows);
    unsigned cursor = menu.cursor < count ? menu.cursor : count - 1;
    unsigned start = menu.scroll <= cursor ? menu.scroll : cursor;
    MenuRow selected = rows[cursor];
    rect(c, 280, 81, 128, 151, pixel(c, 0x09271f, 235)); rect(c, 280, 81, 128, 1, trim);
    label(c, 290, 90, menu.tab == MENU_ITEMS ? "INVENTORY" :
          menu.tab == MENU_TRAVEL ? (age ? "ADULT LINK" : "YOUNG LINK") : "CONTEXT", gold);
    rect(c, 290, 103, 108, 1, trim);
    for (unsigned line = 0; line < VISIBLE_ROWS && start + line < count; ++line) {
        unsigned index = start + line;
        MenuRow row = rows[index];
        int y = 86 + (int)line * 16;
        if (index == cursor) {
            rect(c, 12, y - 4, 252, 15, pixel(c, 0x28523b, 246)); rect(c, 12, y - 4, 2, 15, gold);
        }
        const char *name = "", *state = "";
        char row_label[160], state_label[24];
        int name_x = 32;
        uint32_t color = bright;
        if (row.kind == CHEAT_SECTION || row.kind == TRAVEL_SECTION || row.kind == ITEM_SECTION) {
            bool expanded;
            if (row.kind == CHEAT_SECTION) {
                name = cheat_section_names[row.value]; expanded = menu.cheat_sections & (1u << row.value);
            } else if (row.kind == ITEM_SECTION) {
                name = item_category_names[row.value]; expanded = menu.item_sections & (1u << row.value);
            } else {
                group_name(row_label, sizeof row_label, &menu, row.value); name = row_label;
                expanded = menu.travel_sections[menu.age] & (1ull << row.value);
                if (row.parent != TRAVEL_ROOT) name_x += 6;
            }
            for (int i = 0; i < 3; ++i) {
                if (expanded) rect(c, name_x - 11 + i, y + 1 + i, 5 - i * 2, 1, gold);
                else rect(c, name_x - 11 + i, y + 1 + i, 1, 5 - i * 2, gold);
            }
            color = gold;
        } else if (row.kind == CHEAT_ROW) {
            unsigned mask = 1u << row.value; name = names[row.value];
            color = available & mask ? bright : muted;
            if (!(available & mask)) state = "--";
            else if (row.value == SPEED) state = speeds[menu.speed < 4 ? menu.speed : 0];
            else if (row.value == FLIGHT_SPEED) state = speeds[menu.flight_speed < 6 ? menu.flight_speed : 0];
            else if (row.value == DAMAGE) state = speeds[menu.damage < 4 ? menu.damage : 0];
            else state = menu.enabled & mask ? "ON" : "OFF";
        } else if (row.kind == ITEM_ROW) {
            const InventoryItem *item = &inventory_catalog[row.value];
            const InventoryEntry *entry = &menu.inventory.entries[row.value];
            name = item->name;
            bool valid = menu.inventory.valid && entry->valid && (available & (1u << INVENTORY));
            if (!valid) { state = "--"; color = muted; }
            else if (item->quantity_editable) {
                int quantity = index == cursor && menu.item_edit_index == row.value ? menu.item_quantity : entry->count;
                quantity_text(state_label, sizeof state_label, quantity, entry->capacity);
                state = state_label;
                if (!entry->age_allowed || !entry->owned) color = muted;
            } else { state = entry->owned ? "OWNED" : "NOT OWNED"; if (!entry->owned) color = muted; }
        } else if (row.kind == DESTINATION_ROW) {
            name_x = 38; name = destination_label(row.value);
            if (row.parent == GROUP_FAVORITES) {
                unsigned used = append_text(row_label, 0, sizeof row_label, travel_groups[destination_group(row.value)].name);
                used = append_text(row_label, used, sizeof row_label, ": "); append_text(row_label, used, sizeof row_label, name);
                name = row_label;
            } else if (travel_groups[row.parent].parent != TRAVEL_ROOT) name_x += 6;
            if (!(available & (1u << TRAVEL))) color = muted;
            if (destination_restriction(row.value)) { color = muted; state = destination_locked(row.value) ? "LOCKED" : "UNSUP"; }
            else if (preferences_favorite(&menu.preferences, destinations[row.value].id)) state = "FAV";
        } else {
            name = "Return to Previous Location"; name_x = 22;
            if (!menu.can_return || !(available & (1u << TRAVEL))) color = muted;
        }
        bool ownership_badge = row.kind == ITEM_ROW && !inventory_catalog[row.value].quantity_editable;
        char clipped[48]; unsigned length = 0, maximum = ((ownership_badge ? 198 : state[0] ? 216 : 260) - name_x) / 6;
        while (name[length] && length < maximum && length < sizeof clipped - 1) { clipped[length] = name[length]; ++length; }
        if (name[length] && length >= 3) clipped[length - 1] = clipped[length - 2] = clipped[length - 3] = '.';
        clipped[length] = 0; label(c, name_x, y, clipped, color);
        if (state[0]) {
            bool on = state[0] == 'O' && (state[1] == 'N' || state[1] == 'W'); unsigned length_state = 0;
            while (state[length_state]) ++length_state;
            rect(c, ownership_badge ? 201 : 219, y - 2, ownership_badge ? 62 : 44, 11,
                pixel(c, on ? 0x2a5a3d : 0x122f26, 250));
            label(c, (ownership_badge ? 232 : 241) - (int)length_state * 3, y, state, on ? green : muted);
        }
    }
    if (count > VISIBLE_ROWS) {
        unsigned h = 143 * VISIBLE_ROWS / count; if (h < 4) h = 4;
        rect(c, 270, 82, 1, 143, trim); rect(c, 269, 82 + start * (143 - h) / (count - VISIBLE_ROWS), 3, h, gold);
    }
    const char *first = "Expand a category to browse its controls.", *second = "D-pad selects. A opens or collapses.";
    const char *hint = "A Expand / LB-RB Tabs / B Close", *tag = "GAMEPLAY PAUSED";
    char item_detail[128];
    if (menu.item_prompt && menu.item_prompt_index < INVENTORY_COUNT) {
        first = inventory_catalog[menu.item_prompt_index].name;
        unsigned used = append_text(item_detail, 0, sizeof item_detail,
            menu.item_prompt_quantity == 0 ? "REMOVE ITEM. Quantity: " :
            !inventory_catalog[menu.item_prompt_index].quantity_editable ? "ADD ITEM. Quantity: " : "SET ITEM QUANTITY: ");
        char number[12];
        number_text(number, (unsigned)menu.item_prompt_quantity);
        used = append_text(item_detail, used, sizeof item_detail, number);
        append_text(item_detail, used, sizeof item_detail,
            menu.inventory.entries[menu.item_prompt_index].needs_seed_bag ?
            ". Initializes the native 30-seed bag." : ". This changes the current inventory.");
        second = item_detail;
        tag = "CONFIRM ITEM CHANGE";
        hint = "A Confirm Change / B Cancel / R3 Close";
    } else if (menu.travel_prompt) {
        int index = destination_index(menu.travel_target);
        first = index >= 0 ? travel_groups[destination_group(index)].name : "Return to Previous Location";
        second = index >= 0 ? destination_label(index) : "Restore the recorded safe position.";
        tag = "CONFIRM TRAVEL"; hint = "A Confirm Travel / B Cancel / R3 Close";
    } else if (menu.prompt) {
        first = "Experimental functionality."; second = "A accepts for this game session.";
        tag = "CONFIRM ACTION"; hint = "A Accept / B Close";
    } else if (selected.kind == CHEAT_ROW) {
        first = detail[selected.value][0]; second = detail[selected.value][1];
        if (!(available & (1u << selected.value))) {
            first = "Currently unavailable.";
            second = selected.value == MAGIC ? "Requires an unlocked magic meter." :
                selected.value == FREE_CAMERA ? "Wait for a supported gameplay camera." :
                selected.value == ONE_HIT || selected.value == DAMAGE ? "Unsupported attack integration." :
                selected.value >= FLY && selected.value <= NOCLIP ? "Requires supported travel and safe ground." : "Safe integration required.";
        }
        if (selected.value == DAMAGE && (menu.enabled & (1u << ONE_HIT)))
            second = "One Hit Kill overrides. Your multiplier resumes when OFF.";
        hint = selected.value == SPEED || selected.value == FLIGHT_SPEED || selected.value == DAMAGE ?
            "Left-Right Adjust / LB-RB Tabs / B Close" : "A Toggle / LB-RB Tabs / B Close";
    } else if (selected.kind == ITEM_ROW) {
        const InventoryItem *item = &inventory_catalog[selected.value];
        const InventoryEntry *entry = &menu.inventory.entries[selected.value];
        first = item->name;
        if (!menu.inventory.valid || !entry->valid) second = "Inventory unavailable. Wait for safe gameplay.";
        else if (!entry->age_allowed) second = "This item is restricted to the other age.";
        else if (!entry->can_set) second = "No verified safe operation. Check capacity or reload after an uncertain update.";
        else if (menu.enabled & (1u << ITEMS)) second = "Disable Infinite Items before editing quantities.";
        else if (entry->needs_seed_bag) second = "First grant initializes the native 30-seed bag. Left-Right drafts quantity.";
        else second = item->quantity_editable ?
            "Add or remove stock within native capacity. Left-Right drafts quantity; X refills." :
            "Edit real equipment ownership. Removal restores valid native equipment.";
        tag = entry->owned ? "OWNED / REMOVE ITEM" : "NOT OWNED / ADD ITEM";
        if (menu.item_edit_index == selected.value && menu.item_quantity != entry->count) tag = "DRAFT / NOT APPLIED";
        hint = !entry->can_set ? "LB-RB Tabs / B Category / R3 Close" :
            item->quantity_editable && menu.item_edit_index == selected.value && menu.item_quantity != entry->count ?
            "Left-Right Qty / A Apply / X Fill / Y Remove" :
            entry->owned ? "A REMOVE ITEM / LB-RB Tabs / B Category" : "A ADD ITEM / LB-RB Tabs / B Category";
    } else if (selected.kind == ITEM_SECTION) {
        first = "Browse equipment and ammunition.";
        second = "Add or remove supported items. Every change requires confirmation.";
        tag = "SAFE ITEM MANAGER";
        hint = "A Expand / LB-RB Tabs / B Back";
    } else if (selected.kind == DESTINATION_ROW) {
        first = destination_label(selected.value); second = destination_restriction(selected.value);
        if (!second) second = travel_groups[destination_group(selected.value)].name;
        hint = "A Travel / Y Favorite / X Age / B Region";
    } else if (selected.kind == RETURN_ROW) {
        first = menu.can_return ? "Return to the last safe position." : "No safe return location stored.";
        second = "Cleared when a save is loaded."; hint = "A Select / X Age / LB-RB Tabs / B Close";
    } else if (menu.tab == MENU_TRAVEL) {
        first = selected.value == GROUP_FAVORITES && !travel_group_count(menu.age, GROUP_FAVORITES,
            menu.preferences.favorites, menu.preferences.favorite_count) ? "No Favorites yet. Use Y on a destination." :
            "Open a region, then choose an entrance.";
        second = "Age lists and Favorites are remembered."; hint = "A Open-Close / X Age / B Back / R3 Close";
    }
    if (experimental && selected.kind == CHEAT_ROW && (experimental & (1u << selected.value))) tag = "EXPERIMENTAL";
    int next = paragraph(c, 290, 112, menu.notice ? menu.notice : first, 4, bright);
    paragraph(c, 290, next + 8, second, 5, muted);
    rect(c, 12, 233, WIDTH - 24, 1, trim); label(c, 16, 242, hint, gold);
    label(c, 16, 256, tag, muted); label(c, 294, 256, "R3 OPEN / CLOSE", muted);
    rect(c, 12, 268, WIDTH - 24, 1, trim);
    label(c, 16, 272, "Created by fxbii.dev", muted); label(c, 354, 272, "V1.1.3", gold);
}

static void blend_panel(Canvas *canvas, const uint32_t *scene, unsigned fade) {
    unsigned bits = canvas->format == RGB10A2 ? 10 : 8;
    unsigned mask = (1u << bits) - 1;
    unsigned alpha_bits = 32 - 3 * bits;
    unsigned alpha_max = (1u << alpha_bits) - 1;
    unsigned denom = 175 * 255;
    int pixels = WIDTH * HEIGHT * canvas->scale * canvas->scale;
    for (int i = 0; i < pixels; ++i) {
        uint32_t ink = canvas->pixels[i], base = scene[i];
        unsigned alpha = (ink >> 24) * fade;
        uint32_t result = alpha_max << (3 * bits);
        for (unsigned channel = 0; channel < 3; ++channel) {
            unsigned shift = channel * bits;
            unsigned src = ((ink >> (channel * 8)) & 255) * mask / 255;
            unsigned dst = (base >> shift) & mask;
            result |= ((src * alpha + dst * (denom - alpha) + denom / 2) / denom) << shift;
        }
        canvas->pixels[i] = result;
    }
}

static void encode_panel(Canvas *canvas) {
    if (canvas->format != RGB10A2) return;
    int count = WIDTH * HEIGHT * canvas->scale * canvas->scale;
    for (int i = 0; i < count; ++i) {
        uint32_t ink = canvas->pixels[i];
        canvas->pixels[i] = ((ink & 255) * 1023 / 255) |
            ((((ink >> 8) & 255) * 1023 / 255) << 10) |
            ((((ink >> 16) & 255) * 1023 / 255) << 20) | 0xc0000000u;
    }
}

static void draw_overlay(void *queue, void *texture, Rect crop, int format, uint32_t flags,
                         unsigned request) {
    unsigned alpha = flags & 255;
    bool capture = request && request != __atomic_load_n(&capture_done, __ATOMIC_RELAXED);
    bool restore = request && !capture && background.queue == queue && background.format == format &&
                   background.crop.x == crop.x && background.crop.y == crop.y &&
                   background.crop.width == crop.width && background.crop.height == crop.height;
    int scale = crop.height / 360;
    if (scale > crop.width / 640) scale = crop.width / 640;
    if (scale > MAX_SCALE) scale = MAX_SCALE;
    if (scale < 1) return;
    unsigned frame = next_frame;
    if (submitted[frame] && nvn.sync_wait(syncs[frame], 0) > 1) return;
    next_frame = (frame + 1) % FRAME_COUNT;
    size_t offset = (size_t)frame * FRAME_STRIDE;
    int width = WIDTH * scale, height = HEIGHT * scale;
    CopyRegion panel = {crop.x + (crop.width - width) / 2, crop.y + (crop.height - height) / 2,
                        0, width, height, 1};
    const uint32_t *scene = (const uint32_t *)(cpu_address + READBACK_OFFSET);
    size_t panel_bytes = (size_t)width * height * 4;
    if (restore && !background.checked && nvn.sync_wait(readback_sync, 0) <= 1) {
        nvn.invalidate(pool, READBACK_OFFSET, panel_bytes);
        for (int i = 0; i < width * height; i += 64) {
            if (scene[i] & (format == RGB10A2 ? 0x3fffffffu : 0xffffffu)) {
                background.readable = true;
                break;
            }
        }
        background.checked = true;
        if (background.readable) debug_log("[hyrule] pause frame available for panel blending\n");
    }
    unsigned blend_capture = restore && background.readable ? request : 0;
    uint32_t panel_flags = blend_capture ? flags : flags & ~255u;
    if (alpha && (last_flags[frame] != panel_flags || last_scale[frame] != scale ||
                  last_format[frame] != format || last_capture[frame] != blend_capture)) {
        Canvas canvas = {(uint32_t *)(cpu_address + offset), scale, format};
        draw_panel(&canvas, flags);
        if (blend_capture) blend_panel(&canvas, scene, alpha);
        else encode_panel(&canvas);
        nvn.flush(pool, (ptrdiff_t)offset, panel_bytes);
        last_flags[frame] = panel_flags;
        last_scale[frame] = scale;
        last_format[frame] = format;
        last_capture[frame] = blend_capture;
    }
    void *cmd = commands[frame];
    nvn.add_command_memory(cmd, pool, (ptrdiff_t)(offset + BYTES_PER_FRAME), COMMAND_BYTES);
    nvn.add_control_memory(cmd, controls[frame], COMMAND_BYTES);
    nvn.begin(cmd);
    nvn.barrier(cmd, 3);
    if (capture || restore) {
        CopyRegion whole = {crop.x, crop.y, 0, crop.width, crop.height, 1};
        nvn.set_stride(cmd, ((crop.width + 15) & ~15) * 4);
        if (capture) {
            nvn.capture(cmd, texture, 0, &whole, gpu_address + BACKGROUND_OFFSET, 0);
            nvn.set_stride(cmd, width * 4);
            nvn.capture(cmd, texture, 0, &panel, gpu_address + READBACK_OFFSET, 0);
            nvn.fence(cmd, readback_sync, 0, 0);
        } else nvn.copy(cmd, gpu_address + BACKGROUND_OFFSET, texture, 0, &whole, 0);
        nvn.barrier(cmd, 3);
    }
    if (alpha) {
        // A reversible center reveal is the fallback if CPU readback is unavailable.
        int trim = blend_capture ? 0 : (int)((175 - alpha) * height / 350);
        panel.y += trim;
        panel.height -= trim * 2;
        nvn.set_stride(cmd, width * 4);
        nvn.copy(cmd, gpu_address + offset + (uint64_t)trim * width * 4, texture, 0, &panel, 0);
    }
    nvn.barrier(cmd, 3);
    nvn.fence(cmd, syncs[frame], 0, 0);
    uint64_t command = nvn.end(cmd);
    if (!command) return;
    nvn.submit(queue, 1, &command);
    submitted[frame] = true;
    if (capture) {
        background.queue = queue;
        background.crop = crop;
        background.format = format;
        background.checked = false;
        background.readable = false;
        __atomic_store_n(&capture_done, request, __ATOMIC_RELEASE);
    }
}

static void present(void *queue, void *window, int index) {
    if (window == surface.window && index >= 0 && index < surface.count && surface.textures[index]) {
        void *texture = surface.textures[index];
        int w = nvn.width(texture), h = nvn.height(texture), format = nvn.format(texture);
        if (w >= 640 && h >= 360 && w <= MAX_WIDTH && h <= MAX_HEIGHT && w >= h &&
            nvn.target(texture) == TEXTURE_2D && nvn.samples(texture) <= 1 &&
            (format == RGBA8 || format == RGBA8_SRGB || format == RGBX8_SRGB || format == RGB10A2)) {
            __atomic_store_n(&last_surface, monotonic_ms(), __ATOMIC_RELEASE);
            if (!surface_reported) {
                surface_reported = true;
                debug_log("[hyrule] swapchain target detected\n");
            }
            uint32_t flags = __atomic_load_n(&published, __ATOMIC_ACQUIRE);
            unsigned request = __atomic_load_n(&capture_request, __ATOMIC_ACQUIRE);
            if ((flags & 255) || request) {
                Rect crop = {0, 0, w, h};
                nvn.window_crop(window, &crop);
                if (crop.x < 0 || crop.y < 0 || crop.x >= w || crop.y >= h ||
                    crop.width <= 0 || crop.height <= 0 || crop.width > w - crop.x || crop.height > h - crop.y)
                    crop = (Rect){0, 0, w, h};
                draw_overlay(queue, texture, crop, format, flags, request);
            }
        }
    }
    nvn.present(queue, window, index);
}

static void set_textures(void *builder, int count, void *const *textures) {
    nvn.set_textures(builder, count, textures);
    surface.builder = builder;
    surface.pending_count = textures && count > 0 && count <= MAX_TEXTURES ? count : 0;
    for (int i = 0; i < surface.pending_count; ++i) surface.pending[i] = textures[i];
}

static unsigned window_init(void *window, const void *builder) {
    unsigned result = nvn.window_init(window, builder);
    if (result && builder == surface.builder && surface.pending_count > 0) {
        surface.window = window;
        surface.count = surface.pending_count;
        for (int i = 0; i < surface.count; ++i) surface.textures[i] = surface.pending[i];
        surface.builder = 0;
        surface.pending_count = 0;
    }
    return result;
}

static void window_fini(void *window) {
    if (surface.window == window) {
        surface.window = 0;
        surface.count = 0;
        __atomic_store_n(&last_surface, 0, __ATOMIC_RELEASE);
    }
    nvn.window_fini(window);
}

void graphics_load(void *device, Lookup lookup) {
    ((void (*)(void *, Lookup))game(0x812BE290))(device, lookup);
    if (!device || !lookup || overlay_device) return;
    #define RESOLVE(field, name) do { \
        nvn.field = (void *)lookup(device, "nvn" name); \
        if (!nvn.field) { debug_log("[hyrule] NVN function unavailable: " name "\n"); return; } \
    } while (0)
    RESOLVE(present, "QueuePresentTexture");
    RESOLVE(set_textures, "WindowBuilderSetTextures");
    RESOLVE(window_init, "WindowInitialize");
    RESOLVE(window_fini, "WindowFinalize");
    RESOLVE(window_crop, "WindowGetCrop");
    RESOLVE(width, "TextureGetWidth");
    RESOLVE(height, "TextureGetHeight");
    RESOLVE(format, "TextureGetFormat");
    RESOLVE(target, "TextureGetTarget");
    RESOLVE(samples, "TextureGetSamples");
    RESOLVE(builder_defaults, "MemoryPoolBuilderSetDefaults");
    RESOLVE(builder_device, "MemoryPoolBuilderSetDevice");
    RESOLVE(builder_storage, "MemoryPoolBuilderSetStorage");
    RESOLVE(builder_flags, "MemoryPoolBuilderSetFlags");
    RESOLVE(pool_init, "MemoryPoolInitialize");
    RESOLVE(pool_address, "MemoryPoolGetBufferAddress");
    RESOLVE(pool_map, "MemoryPoolMap");
    RESOLVE(flush, "MemoryPoolFlushMappedRange");
    RESOLVE(invalidate, "MemoryPoolInvalidateMappedRange");
    RESOLVE(sync_init, "SyncInitialize");
    RESOLVE(sync_wait, "SyncWait");
    RESOLVE(fence, "CommandBufferFenceSync");
    RESOLVE(command_init, "CommandBufferInitialize");
    RESOLVE(add_command_memory, "CommandBufferAddCommandMemory");
    RESOLVE(add_control_memory, "CommandBufferAddControlMemory");
    RESOLVE(begin, "CommandBufferBeginRecording");
    RESOLVE(end, "CommandBufferEndRecording");
    RESOLVE(submit, "QueueSubmitCommands");
    RESOLVE(set_stride, "CommandBufferSetCopyRowStride");
    RESOLVE(copy, "CommandBufferCopyBufferToTexture");
    RESOLVE(capture, "CommandBufferCopyTextureToBuffer");
    RESOLVE(barrier, "CommandBufferBarrier");
    #undef RESOLVE
    const uintptr_t dispatch[] = {0x84CADDE8, 0x84CADE08, 0x84CADE90, 0x84CADE98};
    const uintptr_t originals[] = {(uintptr_t)nvn.present, (uintptr_t)nvn.set_textures,
                                   (uintptr_t)nvn.window_init, (uintptr_t)nvn.window_fini};
    const uintptr_t hooks[] = {(uintptr_t)present, (uintptr_t)set_textures,
                               (uintptr_t)window_init, (uintptr_t)window_fini};
    for (unsigned i = 0; i < sizeof dispatch / sizeof *dispatch; ++i) {
        uintptr_t slot = pointer(game(dispatch[i]));
        if (!mapped(slot, sizeof(uintptr_t), 3) || pointer(slot) != originals[i]) {
            debug_log("[hyrule] swapchain dispatch slot mismatch\n");
            return;
        }
    }
    _Alignas(16) uint8_t builder[512];
    uint32_t (*allocate)(uintptr_t *, size_t) = (void *)runtime_symbol("_ZN2nn2os14AllocateMemoryEPmm");
    uintptr_t memory = 0;
    if (!allocate || allocate(&memory, BACKING_BYTES) || !memory || (memory & 4095) ||
        !mapped(memory, BACKING_BYTES, 3)) {
        debug_log("[hyrule] overlay memory allocation unavailable\n");
        return;
    }
    backing = (uint8_t *)memory;
    nvn.builder_defaults(builder);
    nvn.builder_device(builder, device);
    nvn.builder_storage(builder, backing, BACKING_BYTES);
    nvn.builder_flags(builder, 0x24); // CPU cached, GPU cached
    if (!nvn.pool_init(pool, builder)) {
        debug_log("[hyrule] NVN rejected the overlay staging pool\n");
        return;
    }
    overlay_device = device;
    cpu_address = nvn.pool_map(pool);
    gpu_address = nvn.pool_address(pool);
    if (!cpu_address || !gpu_address) return;
    if (!nvn.sync_init(readback_sync, device)) return;
    for (unsigned i = 0; i < FRAME_COUNT; ++i) {
        if (!nvn.sync_init(syncs[i], device) || !nvn.command_init(commands[i], device)) return;
    }
    for (unsigned i = 0; i < sizeof dispatch / sizeof *dispatch; ++i)
        __atomic_store_n((uintptr_t *)pointer(game(dispatch[i])), hooks[i], __ATOMIC_RELEASE);
    debug_log("[hyrule] NVN overlay staging initialized\n");
}
