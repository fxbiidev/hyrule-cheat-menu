#include "guest.h"
#include "save_guard.h"
#include <assert.h>
#include <stdio.h>

uintptr_t main_base = GAME_BASE;
static unsigned holds, native_calls;
static uint32_t ui_type;
static bool readable = true;
bool travel_holds_saves(void) { return holds & 1; }
bool flight_holds_saves(void) { return holds & 2; }
bool noclip_holds_saves(void) { return holds & 4; }
static unsigned mock_ui_type(uintptr_t data, uint32_t *out, uint32_t key) {
    assert(data == 123 && key == 0x578d41e1);
    ++native_calls;
    *out = ui_type;
    return readable;
}
#include "../src/save_guard.c"

int main(void) {
    const uint32_t types[] = {0xb6eede09, 0x42814d01, 0x3b849e33, 0x76e3453e};
    const uint64_t unrelated = (1ull << 40) | UP | R3;
    for (holds = 0; holds < 8; ++holds) {
        for (unsigned i = 0; i < sizeof types / sizeof *types; ++i) {
            ui_type = types[i];
            uint64_t before = unrelated | CONFIRM;
            uint64_t expected = holds && i == 1 ? unrelated | BACK : before;
            assert(save_guard_filter(123, before) == expected);
            unsigned calls = native_calls;
            assert(save_guard_filter(123, unrelated) == unrelated && native_calls == calls);
            assert(save_guard_filter(0, before) == before && native_calls == calls);
        }
    }
    holds = 7;
    ui_type = 0x42814d01;
    readable = false;
    assert(save_guard_filter(123, CONFIRM | unrelated) == (CONFIRM | unrelated));
    puts("Production Save input guard: Save-only cancellation, Load/stock-menu isolation, all unsafe holds, original input bits and unavailable UI state passed (native fixture).");
    return 0;
}
