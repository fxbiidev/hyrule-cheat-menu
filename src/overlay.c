#include "guest.h"

enum {
    WIDTH = 300, HEIGHT = 238, MAX_SCALE = 6, FRAME_COUNT = 3,
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
                        available, experimental, (uint32_t)(uintptr_t)menu->notice};
    uint32_t revision = 2166136261u;
    for (unsigned i = 0; i < sizeof state / sizeof *state; ++i)
        revision = (revision ^ state[i]) * 16777619u;
    for (unsigned i = 0; i < menu->preferences.favorite_count; ++i)
        revision = (revision ^ menu->preferences.favorites[i]) * 16777619u;
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

typedef struct { uint32_t *pixels; int scale, format; } Canvas;

static uint32_t pixel(Canvas *c, unsigned rgb, unsigned alpha) {
    (void)c;
    unsigned r = rgb >> 16, g = (rgb >> 8) & 255, b = rgb & 255;
    // Keep software coverage at eight bits even for a two-bit-alpha swapchain.
    // Quantizing 85% to RGB10A2 first would round it to completely opaque.
    return r | g << 8 | b << 16 | alpha << 24;
}

static void rect(Canvas *c, int x, int y, int w, int h, uint32_t color) {
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

static void draw_panel(Canvas *c, uint32_t flags) {
    (void)flags;
    while (__atomic_exchange_n(&panel_lock, 1, __ATOMIC_ACQUIRE)) { }
    Menu menu = panel_menu;
    unsigned available = panel_available, experimental = panel_experimental;
    __atomic_store_n(&panel_lock, 0, __ATOMIC_RELEASE);
    static const char *const names[CHEAT_COUNT] = {
        "God Mode", "Infinite Rupees", "Infinite Items", "Infinite Magic", "Movement Speed",
        "Fly Mode", "Flight Speed", "Noclip"
    };
    static const char *const speeds[] = {"1X", "1.5X", "2X", "3X", "5X", "10X"};
    static const char *const detail[CHEAT_COUNT][2] = {
        {"Refills your existing hearts.", "Protects ordinary life damage."},
        {"Fills the current wallet.", "Respects unlocked wallet capacity."},
        {"Refills eligible owned ammunition.", "Respects age and capacity upgrades."},
        {"Refills an unlocked magic meter.", "Requires the fairy magic upgrade."},
        {"Scales ordinary travel movement.", "Walking, running, swimming, climbing."},
        {"Left stick fly / RT ascend / LT descend.", "OFF lands naturally; protects one fall."},
        {"Independent horizontal and vertical speed.", "1x, 1.5x, 2x, 3x, 5x and 10x."},
        {"Grounded walls; add Fly for full clip.", "OFF recovers the last clear position."}
    };
    uint32_t gold = pixel(c, 0xddb760, 255), bright = pixel(c, 0xf8e7ba, 255);
    uint32_t muted = pixel(c, 0x9caa92, 255), green = pixel(c, 0x92d8a5, 255);
    uint32_t trim = pixel(c, 0x8d7849, 255);
    rect(c, 0, 0, WIDTH, HEIGHT, pixel(c, 0x0e251b, 217));
    rect(c, 0, 0, WIDTH, 1, gold);
    rect(c, 0, HEIGHT - 1, WIDTH, 1, gold);
    rect(c, 0, 1, 1, HEIGHT - 2, gold);
    rect(c, WIDTH - 1, 1, 1, HEIGHT - 2, gold);
    label(c, 12, 9, "HYRULE CHEAT MENU", gold);
    rect(c, 12, 22, WIDTH - 24, 1, trim);
    label(c, 12, 29, "CHEATS", menu.tab ? muted : gold);
    label(c, 72, 29, "FAST TRAVEL", menu.tab ? gold : muted);
    if (menu.tab && (experimental & (1u << TRAVEL))) label(c, 163, 29, "EXP", gold);
    unsigned display_age = menu.travel_prompt && menu.travel_target == UINT32_MAX ? menu.return_age : menu.age;
    if (menu.tab) label(c, 204, 29, display_age ? "ADULT LINK" : "YOUNG LINK", bright);
    MenuRow rows[MENU_ROWS];
    unsigned count = menu_rows(&menu, rows);
    unsigned cursor = menu.cursor < count ? menu.cursor : count - 1;
    unsigned start = menu.scroll <= cursor ? menu.scroll : cursor;
    MenuRow selected = rows[cursor];
    for (unsigned line = 0; line < VISIBLE_ROWS && start + line < count; ++line) {
        unsigned index = start + line;
        MenuRow row = rows[index];
        int y = 46 + (int)line * 14;
        if (index == cursor) {
            rect(c, 7, y - 3, WIDTH - 14, 13, pixel(c, 0x39412a, 232));
            for (int i = 0; i < 4; ++i) rect(c, 12 + i, y + i, 1, 7 - 2 * i, gold);
        }
        const char *name = "", *state = "";
        char row_label[160];
        int name_x = 23;
        uint32_t color = bright;
        if (row.kind == CHEAT_SECTION || row.kind == TRAVEL_SECTION) {
            bool expanded;
            if (row.kind == CHEAT_SECTION) {
                name = row.value ? "Movement Cheats" : "Player Cheats";
                expanded = menu.cheat_sections & (1u << row.value);
            } else {
                group_name(row_label, sizeof row_label, &menu, row.value);
                name = row_label;
                expanded = menu.travel_sections[menu.age] & (1ull << row.value);
                int indicator = row.parent == TRAVEL_ROOT ? 23 : 29;
                name_x = indicator + 9;
                for (int i = 0; i < 4; ++i) {
                    if (expanded) rect(c, indicator + i, y + i, 7 - i * 2, 1, gold);
                    else rect(c, indicator + i, y + i, 1, 7 - i * 2, gold);
                }
            }
            state = expanded ? "OPEN" : "CLOSED";
            color = gold;
        } else if (row.kind == CHEAT_ROW) {
            unsigned mask = 1u << row.value;
            name = names[row.value];
            bool active = available & mask;
            color = active ? bright : muted;
            if (experimental & mask) label(c, 215, y, "EXP", gold);
            if (!active) state = "--";
            else if (row.value == SPEED) state = speeds[menu.speed];
            else if (row.value == FLIGHT_SPEED) state = speeds[menu.flight_speed];
            else state = menu.enabled & mask ? "ON" : "OFF";
        } else if (row.kind == DESTINATION_ROW) {
            name_x = 38;
            name = destination_label(row.value);
            if (row.parent == GROUP_FAVORITES) {
                unsigned used = append_text(row_label, 0, sizeof row_label,
                                            travel_groups[destination_group(row.value)].name);
                used = append_text(row_label, used, sizeof row_label, ": ");
                append_text(row_label, used, sizeof row_label, name);
                name = row_label;
            } else if (travel_groups[row.parent].parent != TRAVEL_ROOT) name_x += 6;
            if (!(available & (1u << TRAVEL))) color = muted;
            if (destination_restriction(row.value)) {
                color = muted;
                state = destination_locked(row.value) ? "LOCKED" : "UNSUP";
            }
            else if (preferences_favorite(&menu.preferences, destinations[row.value].id)) state = "FAV";
        } else {
            name = "Return to Previous Location";
            if (!menu.can_return || !(available & (1u << TRAVEL))) color = muted;
        }
        char clipped[39];
        unsigned length = 0;
        unsigned maximum = (246 - name_x) / 6;
        while (name[length] && length < maximum && length < sizeof clipped - 1) {
            clipped[length] = name[length];
            ++length;
        }
        if (name[length] && length >= 3) clipped[length - 1] = clipped[length - 2] = clipped[length - 3] = '.';
        clipped[length] = 0;
        label(c, name_x, y, clipped, color);
        label(c, 253, y, state, state[0] == 'O' && state[1] == 'N' ? green : color);
    }
    if (count > VISIBLE_ROWS) {
        unsigned height = 124 * VISIBLE_ROWS / count;
        if (height < 4) height = 4;
        rect(c, 294, 46 + start * (124 - height) / (count - VISIBLE_ROWS), 2, height, trim);
    }
    rect(c, 12, 174, WIDTH - 24, 1, trim);
    if (menu.travel_prompt) {
        int index = destination_index(menu.travel_target);
        label(c, 12, 182, index >= 0 ? travel_groups[destination_group(index)].name : "Return to Previous Location", gold);
        label(c, 12, 193, index >= 0 ? destination_label(index) : "Restore the recorded safe position.", bright);
        label(c, 12, 211, menu.consent ? "A Travel / B Cancel" : "A Accept EXP and Travel / B Cancel", gold);
    } else if (menu.prompt) {
        label(c, 12, 182, "Experimental functionality", gold);
        label(c, 12, 193, "A accepts for this game session.", bright);
        label(c, 12, 211, "A Accept / B Close", gold);
    } else {
        const char *first = "A expands or collapses this section.";
        const char *second = menu.tab ? storage_status() : "Both sections start open on restart.";
        const char *hint = "A Expand / L-R Tabs / B Close";
        if (selected.kind == CHEAT_ROW) {
            first = detail[selected.value][0]; second = detail[selected.value][1];
            if (!(available & (1u << selected.value)) && selected.value != MAGIC) {
                first = "Unavailable. Safe integration required.";
                second = selected.value == FLY || selected.value == FLIGHT_SPEED || selected.value == NOCLIP ?
                         "Requires supported travel and safe ground." : detail[selected.value][0];
            }
            hint = selected.value == SPEED || selected.value == FLIGHT_SPEED ?
                   "Left-Right Adjust / LB-RB Tabs / B Close" : "A Toggle / LB-RB Tabs / B Close";
        } else if (selected.kind == DESTINATION_ROW) {
            first = destination_label(selected.value);
            second = destination_restriction(selected.value);
            if (!second) second = travel_groups[destination_group(selected.value)].name;
            hint = "A Travel / Y Favorite / B Region / X Age";
        } else if (selected.kind == RETURN_ROW) {
            first = menu.can_return ? "Return to the last recorded safe position." : "No safe return location is stored.";
            second = "Cleared when an actual save is loaded.";
            hint = "A Select / X Age / LB-RB Tabs / B Close";
        } else if (menu.tab) {
            first = selected.value == GROUP_FAVORITES && !travel_group_count(menu.age, GROUP_FAVORITES,
                    menu.preferences.favorites, menu.preferences.favorite_count) ?
                    "No Favorites yet. Use Y on a destination." : "Open a region, then choose an entrance.";
            hint = "A Open-Close / X Age / B Back / R3 Close";
        }
        label(c, 12, 182, menu.notice ? menu.notice : first, bright);
        label(c, 12, 193, second, muted);
        label(c, 12, 211, hint, gold);
    }
    rect(c, 12, 222, WIDTH - 24, 1, trim);
    label(c, 12, 228, "Created by fxbii.dev", muted);
    label(c, 220, 228, "V1.0.0", gold);
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
