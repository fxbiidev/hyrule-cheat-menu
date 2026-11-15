#include "guest.h"
#include "travel_runtime.h"
#include "flight.h"
#include "landing.h"
#include "noclip.h"
#include "items.h"
#include "ammo_guard.h"
#include "wallet.h"
#include "save_guard.h"
#include "travel_access.h"

struct Signature { uintptr_t address; unsigned count, features; uint32_t words[11]; };
#define HCM_PROFILE_CHECKS
#include "profile.h"

enum { READ = 1, WRITE = 2, EXECUTE = 4 };
static const uint64_t title_id = 0x0400486024DEC000ull;
// The stock profile pauses actor updates, physics and timers without pausing BGM.
static const uint32_t game_pause = 0xb5b0a658; // Murmur3("GameOverPause")
uintptr_t main_base;
static Menu menu;
static const enum Support support[CHEAT_COUNT] = {
    VERIFIED, VERIFIED, VERIFIED, VERIFIED,
    VERIFIED, VERIFIED, VERIFIED, VERIFIED
};
static unsigned experimental = (1u << (TRAVEL + 1)) - 1;
static uintptr_t last_life, held_pause;
static uint64_t opening_deadline;
static uint64_t last_link_ms;
static unsigned save_generation, active_save_generation;
static bool drained_buttons;
static unsigned observed_ammo[2];
static uint64_t next_ammo_check;
static uint64_t next_travel_access_check;

typedef struct {
    uintptr_t address, size;
    uint32_t state, attributes, permissions, ipc, devices, reserved;
} MemoryInfo;

static bool query_memory(uintptr_t address, MemoryInfo *out) {
    register uintptr_t x0 __asm__("x0") = (uintptr_t)out;
    register uintptr_t x1 __asm__("x1") = 0;
    register uintptr_t x2 __asm__("x2") = address;
    __asm__ volatile("svc #6" : "+r"(x0), "+r"(x1) : "r"(x2) : "memory");
    return x0 == 0;
}

bool mapped(uintptr_t address, size_t bytes, unsigned permissions) {
    MemoryInfo info;
    return address && query_memory(address, &info) && (info.permissions & permissions) == permissions &&
           address >= info.address && address - info.address <= info.size &&
           bytes <= info.size - (address - info.address);
}

static uintptr_t module_symbol(uintptr_t base, const char *name) {
    uint32_t mod_offset = *(const uint32_t *)(base + 4);
    if (mod_offset > 0x8000000 || !mapped(base + mod_offset, 8, READ) ||
        *(const uint32_t *)(base + mod_offset) != 0x30444f4d) return 0;
    uintptr_t dynamic = base + mod_offset + *(const int32_t *)(base + mod_offset + 4);
    uintptr_t strings = 0, symbols = 0;
    size_t string_bytes = 0, symbol_bytes = 0;
    for (unsigned i = 0; i < 128; ++i) {
        if (!mapped(dynamic + i * 16, 16, READ)) return 0;
        const uintptr_t *entry = (const uintptr_t *)(dynamic + i * 16);
        if (!entry[0]) break;
        if (entry[0] == 5) strings = base + entry[1];
        if (entry[0] == 6) symbols = base + entry[1];
        if (entry[0] == 10) string_bytes = entry[1];
        if (entry[0] == 11) symbol_bytes = entry[1];
    }
    if (!strings || !symbols || symbol_bytes != 24 || strings <= symbols ||
        strings - symbols > 65536 * 24 || !mapped(symbols, strings - symbols, READ) ||
        !mapped(strings, string_bytes, READ)) return 0;
    size_t name_length = 0;
    while (name[name_length]) ++name_length;
    for (uintptr_t entry = symbols; entry + 24 <= strings; entry += 24) {
        uint32_t offset = *(const uint32_t *)entry;
        uintptr_t value = *(const uintptr_t *)(entry + 8);
        if (!value || offset >= string_bytes || name_length >= string_bytes - offset) continue;
        const char *candidate = (const char *)(strings + offset);
        size_t i = 0;
        while (i < name_length && candidate[i] == name[i]) ++i;
        if (i == name_length && !candidate[i] && mapped(base + value, 4, READ | EXECUTE))
            return base + value;
    }
    return 0;
}

uintptr_t runtime_symbol(const char *name) {
    static uintptr_t sdk_base;
    if (sdk_base) return module_symbol(sdk_base, name);
    uintptr_t address = main_base;
    for (unsigned i = 0; i < 256; ++i) {
        MemoryInfo info;
        if (!query_memory(address, &info) || !info.size || info.address + info.size <= address) break;
        if (info.permissions == (READ | EXECUTE) && info.size >= 8) {
            uintptr_t symbol = module_symbol(info.address, name);
            if (symbol) { sdk_base = info.address; return symbol; }
        }
        address = info.address + info.size;
    }
    return 0;
}

static uint64_t process_program_id(void) {
    register uintptr_t x0 __asm__("x0") = 0;
    register uintptr_t x1 __asm__("x1") = 18; // InfoType::ProgramId
    register uintptr_t x2 __asm__("x2") = 0xffff8001; // CurrentProcess
    register uintptr_t x3 __asm__("x3") = 0;
    __asm__ volatile("svc #0x29" : "+r"(x0), "+r"(x1) : "r"(x2), "r"(x3) : "memory");
    return x0 == 0 ? x1 : 0;
}

void debug_log(const char *text) {
    static unsigned remaining = 65536;
    size_t length = 0;
    while (text[length]) ++length;
    unsigned budget = __atomic_load_n(&remaining, __ATOMIC_RELAXED);
    do {
        if (length > budget) return;
    } while (!__atomic_compare_exchange_n(&remaining, &budget, budget - (unsigned)length,
                                          false, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    register const char *x0 __asm__("x0") = text;
    register size_t x1 __asm__("x1") = length;
    __asm__ volatile("svc #0x27" : "+r"(x0), "+r"(x1) :: "memory");
}

uint64_t monotonic_ms(void) {
    uint64_t counter, frequency;
    __asm__ volatile("mrs %0, cntpct_el0\n mrs %1, cntfrq_el0" : "=r"(counter), "=r"(frequency));
    return frequency >= 1000 ? counter / (frequency / 1000) : 0;
}

static uintptr_t find_main(void) {
    uintptr_t address = (uintptr_t)&find_main;
    for (unsigned i = 0; address && i < 64; ++i) {
        MemoryInfo info;
        if (!query_memory(address, &info)) break;
        if (info.permissions == (READ | EXECUTE) && info.size == 0x3c47000 &&
            *(const uint32_t *)(info.address + 4) == 0x3c4705c) return info.address;
        if (!info.address) break;
        address = info.address - 1;
    }
    return 0;
}

static bool pause_active(uintptr_t manager) {
    const uint32_t *masks = (const uint32_t *)(manager + 0x30);
    return masks[0] || masks[1] || masks[2] || masks[3];
}

static const uint32_t *pause_entry(uintptr_t manager) {
    unsigned count = *(const unsigned *)manager;
    uintptr_t table = pointer(manager + 8);
    if (!count || count > 128 || !mapped(table, count * 0x20, READ)) return 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint32_t *entry = (const uint32_t *)(table + i * 0x20 + 8);
        if (entry[0] == game_pause) return entry;
    }
    return 0;
}

static bool set_paused(uintptr_t manager, bool pause) {
    if (pause) {
        const uint32_t *entry = manager ? pause_entry(manager) : 0;
        if (!entry || entry[1] != 0x303a0105 || entry[2] != 0x109440a4 ||
            entry[3] != 0x1008d || entry[4] || entry[5]) return false;
        if (!((unsigned (*)(uint32_t))game(GAME_request_pause))(game_pause)) {
            ((unsigned (*)(uint32_t))game(GAME_release_pause))(game_pause);
            return false;
        }
        held_pause = manager;
        debug_log("[hyrule] menu opened, world pause requested\n");
    } else {
        if (held_pause && manager == held_pause)
            ((unsigned (*)(uint32_t))game(GAME_release_pause))(game_pause);
        held_pause = 0;
        debug_log("[hyrule] menu closed, world pause released\n");
    }
    return true;
}

static bool not_loading(uintptr_t data) {
    uint8_t loading = 1;
    return data && ((unsigned (*)(uintptr_t, uint8_t *, uint32_t))game(GAME_read_bool))(
               data, &loading, 0xef2b60c2) && !loading;
}

bool world_safe(void) {
    uintptr_t event = singleton(0x84C9E090), data = singleton(0x84C9E180);
    return event && !((unsigned (*)(uintptr_t, unsigned))game(GAME_event_running))(event, 0) &&
           not_loading(data);
}

unsigned save_epoch(void) {
    return __atomic_load_n(&save_generation, __ATOMIC_ACQUIRE);
}

#include "guest_resources.inc"

static bool is_link_life(uintptr_t life) {
    uintptr_t physics = pointer(life + 0x18);
    if (!physics || pointer(physics) != game(0x84B45E18)) return false;
    uintptr_t components = pointer(physics + 0x270);
    uintptr_t movement = components ? pointer(components + 0x320) : 0;
    if (!movement || pointer(movement + 0x18) != physics) return false;
    uintptr_t player = movement - 0x1d50;
    if (pointer(player) != game(0x84B6C460)) return false;
    uintptr_t name = pointer(player + 0x10);
    if (!name) return false;
    const char *a = (const char *)(name + 0x58);
    return a[0] == 'P' && a[1] == 'l' && a[2] == 'a' && a[3] == 'y' &&
           a[4] == 'e' && a[5] == 'r' && a[6] == '\0';
}

static void life_update(uintptr_t life, uintptr_t update_context) {
    unsigned generation = save_epoch();
    if (is_link_life(life) && generation == save_epoch()) {
        static bool reported;
        if (!__atomic_exchange_n(&reported, true, __ATOMIC_RELAXED))
            debug_log("[hyrule] Link life component identified\n");
        __atomic_store_n(&last_life, life, __ATOMIC_RELAXED);
        __atomic_store_n(&last_link_ms, monotonic_ms(), __ATOMIC_RELEASE);
        uintptr_t guard = __atomic_load_n((uintptr_t *)game(LIFE_SLOT), __ATOMIC_ACQUIRE);
        if (guard && pointer(life + 0x1c10) + 8 == guard &&
            generation == __atomic_load_n(&life_generation, __ATOMIC_RELAXED))
            refill_life(life, generation);
        if (generation != save_epoch())
            __atomic_store_n(&last_link_ms, 0, __ATOMIC_RELEASE);
    }
    ((void (*)(uintptr_t, uintptr_t))game(ORIGINAL_life_update))(life, update_context);
}

static void reset_for_save(void) {
    // Invalidate in-flight native getters/jobs before clearing their publications.
    unsigned generation = __atomic_add_fetch(&save_generation, 1, __ATOMIC_ACQ_REL);
    wallet_reset();
    ammo_set_owner(0, generation);
    clear_life_guard();
    __atomic_store_n(&magic_owner, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&speed_owner, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&last_link_ms, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&last_life, 0, __ATOMIC_RELEASE);
}

static void save_load(uintptr_t command) {
    reset_for_save();
    ((void (*)(uintptr_t))game(ORIGINAL_save_load))(command);
}

static void save_ready(uintptr_t command) {
    reset_for_save();
    ((void (*)(uintptr_t))game(ORIGINAL_save_ready))(command);
}

static void save_from_game_over(uintptr_t command) {
    reset_for_save();
    ((void (*)(uintptr_t))game(ORIGINAL_save_from_game_over))(command);
}

typedef struct {
    uint64_t sample, buttons;
    int32_t lx, ly, rx, ry;
    uint32_t attributes, padding;
} Pad;

_Static_assert(sizeof(Pad) == 0x28, "Npad ABI");

static int pad_read(uintptr_t original, Pad *states, int count, const uint32_t *id) {
    int result = ((int (*)(Pad *, int, const uint32_t *))game(original))(states, count, id);
    // This title is single player. Use player one, with handheld as the fallback.
    static bool primary;
    bool connected = result > 0 && (states[0].attributes & 1);
    if (*id == 0) primary = connected;
    else if (*id != 0x20 || primary) return result;
    uint64_t now = monotonic_ms();
    unsigned generation = __atomic_load_n(&save_generation, __ATOMIC_ACQUIRE);
    uintptr_t manager = pointer(game(0x84EB7E38));
    if (generation != active_save_generation) {
        if (held_pause) set_paused(manager, false);
        overlay_end_capture();
        opening_deadline = 0;
        menu_reset(&menu);
        travel_reset();
        flight_reset();
        noclip_reset();
        observed_ammo[0] = observed_ammo[1] = 0;
        next_ammo_check = 0;
        next_travel_access_check = 0;
        active_save_generation = generation;
    }
    uint64_t last_seen = __atomic_load_n(&last_link_ms, __ATOMIC_ACQUIRE);
    bool was_open = menu.open;
    if ((experimental & (1u << TRAVEL)) && !travel_healthy()) {
        experimental &= ~((1u << TRAVEL) | (1u << FLY) | (1u << FLIGHT_SPEED) | (1u << NOCLIP));
        menu.enabled &= ~((1u << FLY) | (1u << NOCLIP));
        flight_reset();
        noclip_reset();
    }
    bool world_ready = world_safe();
    bool safe = world_ready && !travel_busy();
    bool gameplay_paused = manager && pause_active(manager);
    bool recent_link = last_seen && now - last_seen < 450;
    bool can_open = recent_link && safe && !noclip_busy() && manager &&
                    !gameplay_paused && overlay_ready();
    uintptr_t live = __atomic_load_n(&last_life, __ATOMIC_RELAXED);
    bool identified = (recent_link || held_pause) && mapped(live, 0x1c20, READ) && is_link_life(live);
    uintptr_t actor = identified ? pointer(live + 0x18) : 0;
    if (actor && world_ready && !menu.preferences_loaded) {
        menu_initialize(&menu);
        storage_load(&menu.preferences);
        menu.flight_speed = menu.preferences.flight_speed;
        menu.age = !((unsigned (*)(uintptr_t))game(GAME_child_link))(actor);
        menu.preferences_loaded = true;
    }
    uintptr_t pouch = pointer(game(0x84E98E28));
    uintptr_t data = singleton(0x84C9E180);
    // Read existing membership and the guard's quest condition without grants.
    // Both the pouch getter and live quest-order layout must be verified.
    if (safe && (recent_link || held_pause) && pouch &&
        (experimental & (1u << ITEMS)) && (experimental & (1u << TRAVEL))) {
        if (now >= next_travel_access_check) {
            destination_set_gerudo_access(gerudo_admission(pouch, data, active_save_generation));
            next_travel_access_check = now + 80;
        }
    } else {
        destination_set_gerudo_access(false);
        next_travel_access_check = 0;
    }
    int magic = 0, magic_cap = 0;
    bool magic_unlocked = (experimental & (1u << MAGIC)) && safe && (recent_link || held_pause) &&
                          magic_levels(pouch, data, &magic, &magic_cap, generation);
    unsigned available = magic_unlocked ? experimental : experimental & ~(1u << MAGIC);
    bool flight_ready = (experimental & (1u << TRAVEL)) &&
                        flight_available(&menu, actor, safe);
    if (!flight_ready) available &= ~((1u << FLY) | (1u << FLIGHT_SPEED));
    if (!(experimental & (1u << NOCLIP)) || !noclip_available(&menu, actor, safe))
        available &= ~(1u << NOCLIP);
    // Native availability getters may have entered a save load. The following
    // menu and movement operations must keep the original invocation's epoch.
    if (generation != save_epoch()) return result;
    uint32_t buttons = connected ? (uint32_t)states[0].buttons : 0;
    if ((buttons & R3) && !(menu.previous & R3) && !can_open && !held_pause) {
        if (!recent_link) debug_log("[hyrule] R3 blocked: Link life unavailable\n");
        if (!safe) debug_log("[hyrule] R3 blocked: world unsafe\n");
        if (!manager || pause_active(manager)) debug_log("[hyrule] R3 blocked: pause manager busy\n");
        if (!overlay_ready()) debug_log("[hyrule] R3 blocked: UI target unavailable\n");
    }
    menu_input(&menu, now, connected, buttons, can_open || held_pause, available, support);
    // Safety recovery may reopen the menu after the previous input poll's
    // capture branch. Capture every unpaused opening, including that path.
    if (menu.open && !held_pause && !opening_deadline) {
        overlay_begin_capture();
        opening_deadline = now + 450;
    }
    bool captured = opening_deadline && overlay_captured();
    if (opening_deadline && (!menu.open || now >= opening_deadline || captured)) {
        if (menu.open && (!captured || !can_open || !set_paused(manager, true))) menu.open = false;
        if (!menu.open) overlay_end_capture();
        opening_deadline = 0;
    }
    if (held_pause && !menu.open && !menu.alpha) {
        set_paused(manager, false);
        overlay_end_capture();
    }
    // Re-read after releasing our pause so flight triggers remain consumed on
    // the very first resumed frame, while stock paused menus keep their inputs.
    gameplay_paused = manager && pause_active(manager);
    if (generation != save_epoch()) return result;
    if (experimental & (1u << NOCLIP)) noclip_update(&menu, actor, safe, generation);
    if (experimental & (1u << FLY))
        flight_update(&menu, actor, safe, gameplay_paused || noclip_busy(), connected ? states[0].lx : 0,
                      connected ? states[0].ly : 0, buttons, generation);
    // A movement getter can begin a load while queuing native recovery.
    if (generation != save_epoch()) return result;
    travel_update(&menu, actor, world_ready, held_pause != 0, experimental & (1u << TRAVEL));
    if (was_open && !menu.open && menu.preferences_dirty) {
        // Keep failed writes pending so the next menu close retries them.
        menu.preferences_dirty = !storage_save(&menu.preferences);
    }
    uint64_t native_buttons = connected ? states[0].buttons : 0;
    bool cancel_save = save_guard_filter(data, native_buttons) != native_buttons;
    if (cancel_save) menu.notice = "Save cancelled during Fly, Noclip or travel.";
    overlay_publish(&menu, available, 0);

    bool wallet_enabled = (available & menu.enabled & (1u << RUPEES)) != 0;
    bool wallet_loaded = wallet_enabled && !travel_busy() && (world_ready || not_loading(data));
    wallet_update(pouch, data, active_save_generation, wallet_enabled,
                  safe && identified, wallet_loaded);
    if (generation == save_epoch() && safe && (recent_link || held_pause)) {
        uintptr_t life = __atomic_load_n(&last_life, __ATOMIC_RELAXED);
        bool apply_speed = menu.speed && (available & (1u << SPEED)) && !menu.open &&
                           !(menu.enabled & (1u << FLY)) &&
                           manager && !pause_active(manager);
        bool check_items = (available & (1u << ITEMS)) && now >= next_ammo_check;
        bool protect_items = (available & menu.enabled & (1u << ITEMS)) != 0;
        bool valid_life = (apply_speed || check_items || protect_items || (menu.enabled & ((1u << GOD) | (1u << MAGIC)))) &&
                          mapped(life, 0x1c20, READ) && is_link_life(life);
        uintptr_t actor = valid_life ? pointer(life + 0x18) : 0;
        ammo_set_owner(protect_items ? actor : 0, active_save_generation);
        publish_speed(apply_speed && actor && !(actor & 3) ? actor | menu.speed : 0, generation);
        if (check_items && valid_life) {
            items_update(pouch, actor, observed_ammo, (menu.enabled & (1u << ITEMS)) != 0,
                         active_save_generation);
            next_ammo_check = now + 80;
        }
        if (valid_life && magic_unlocked && (menu.enabled & (1u << MAGIC)))
            fill_magic(life, pouch, data, magic, magic_cap, generation);
        else __atomic_store_n(&magic_owner, 0, __ATOMIC_RELEASE);
        if (valid_life && (menu.enabled & (1u << GOD))) {
            uintptr_t current = pointer(life + 0x1c10);
            publish_life_guard(current, generation);
            if (held_pause) refill_life(life, generation);
        } else clear_life_guard();
    } else {
        ammo_set_owner(0, active_save_generation);
        clear_life_guard();
        __atomic_store_n(&magic_owner, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&speed_owner, 0, __ATOMIC_RELEASE);
    }

    bool suppress = was_open || menu.open || menu.alpha || held_pause || noclip_busy();
    if (suppress) drained_buttons = true;
    if (!suppress && drained_buttons && !buttons) drained_buttons = false;
    for (int i = 0; i < result && i < count && i < 16; ++i) {
        if (cancel_save && (states[i].buttons & CONFIRM))
            states[i].buttons = (states[i].buttons & ~(uint64_t)CONFIRM) | BACK;
        if ((menu.enabled & (1u << FLY)) && !gameplay_paused) {
            states[i].buttons &= ~((uint64_t)(1u << 8) | (1u << 9));
            states[i].lx = states[i].ly = 0;
        }
        if (suppress || drained_buttons) {
            states[i].buttons = 0;
            states[i].lx = states[i].ly = states[i].rx = states[i].ry = 0;
        }
    }
    return result;
}

static int pad_dual(Pad *s, int n, const uint32_t *id) { return pad_read(0x83C627F0, s, n, id); }
static int pad_full(Pad *s, int n, const uint32_t *id) { return pad_read(0x83C62800, s, n, id); }
static int pad_left(Pad *s, int n, const uint32_t *id) { return pad_read(0x83C62810, s, n, id); }
static int pad_right(Pad *s, int n, const uint32_t *id) { return pad_read(0x83C62820, s, n, id); }
static int pad_handheld(Pad *s, int n, const uint32_t *id) { return pad_read(0x83C62830, s, n, id); }

static unsigned autosave_gate(uintptr_t manager, unsigned reason, uintptr_t context) {
    if (travel_holds_saves() || flight_holds_saves() || noclip_holds_saves()) return 0;
    return ((unsigned (*)(uintptr_t, unsigned, uintptr_t))game(ORIGINAL_autosave_gate))(
        manager, reason, context);
}

void hyrule_init(void) {
    if (process_program_id() != title_id || !(main_base = find_main())) {
        debug_log("[hyrule] unsupported title or main layout\n");
        return;
    }
    for (unsigned i = 0; i < sizeof signatures / sizeof *signatures; ++i) {
        const struct Signature *s = &signatures[i];
        const uint32_t *words = (const uint32_t *)game(s->address);
        for (unsigned j = 0; j < s->count; ++j) {
            if (words[j] != s->words[j]) {
                if (!s->features) {
                    debug_log("[hyrule] core signature mismatch; hooks inactive\n");
                    return;
                }
                experimental &= ~s->features;
                debug_log("[hyrule] cheat signature mismatch; affected row disabled\n");
                break;
            }
        }
    }
    if (!(experimental & (1u << TRAVEL)))
        experimental &= ~((1u << FLY) | (1u << FLIGHT_SPEED) | (1u << NOCLIP));
    if (!mapped(game(TABLE), LIFE_SLOT - TABLE + 8, READ | WRITE)) return;
    for (uintptr_t slot = TABLE; slot <= LIFE_SLOT; slot += 8) {
        if (pointer(game(slot))) {
            debug_log("[hyrule] bridge table occupied; hooks inactive\n");
            return;
        }
    }
    #define INSTALL(name) __atomic_store_n((uintptr_t *)game(SLOT_##name), (uintptr_t)&name, __ATOMIC_RELEASE)
    INSTALL(graphics_load);
    INSTALL(save_load);
    INSTALL(save_ready);
    INSTALL(save_from_game_over);
    INSTALL(life_update);
    if (experimental & (1u << TRAVEL)) INSTALL(autosave_gate);
    if (experimental & (1u << FLY)) {
        INSTALL(flight_step); INSTALL(flight_job); INSTALL(flight_fall_damage);
    }
    if (experimental & (1u << NOCLIP)) INSTALL(noclip_move);
    if (experimental & (1u << MAGIC)) INSTALL(consume_magic);
    if (experimental & (1u << ITEMS)) { INSTALL(ammo_use); INSTALL(ammo_nut_use); }
    if (experimental & (1u << SPEED)) {
        INSTALL(ground_motion);
        INSTALL(swim_motion);
        INSTALL(climb_motion);
    }
    INSTALL(pad_dual);
    INSTALL(pad_full);
    INSTALL(pad_left);
    INSTALL(pad_right);
    INSTALL(pad_handheld);
    debug_log("[hyrule] 1.1.1 signatures matched; controller and renderer hooks armed\n");
}
