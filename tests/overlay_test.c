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

int main(void) {
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
            Menu menu = {.consent = true, .enabled = 175, .speed = 3, .flight_speed = 5};
            menu_initialize(&menu);
            for (unsigned state = 0; state < 4; ++state) {
                menu.tab = state != 0;
                menu.age = state & 1;
                menu.travel_sections[menu.age] = UINT64_MAX;
                menu.cursor = state ? 12 : 9;
                menu.scroll = state ? 8 : 1;
                menu.prompt = state == 2;
                menu.travel_prompt = state == 3;
                menu.travel_target = destinations[114].id;
                menu.notice = state == 1 ? "A long status remains clipped inside the panel's text boundary, with no buffer overrun." : 0;
                overlay_publish(&menu, 511, 511);
                draw_panel(&canvas, 175);
                guards(allocation, count);
                blend_panel(&canvas, scene, 0);
                for (size_t i = 0; i < count; i += 127)
                    require(canvas.pixels[i] == scene[i], "Fade zero did not preserve the background");
                draw_panel(&canvas, 175);
                blend_panel(&canvas, scene, 175);
                // An empty background pixel should be ~85% dark green over white.
                size_t sample = 2 * scale * (WIDTH * scale) + 2 * scale;
                unsigned max = formats[f] == RGB10A2 ? 1023 : 255;
                unsigned red = (canvas.pixels[sample] & max) * 255 / max;
                if (red < 48 || red > 51) {
                    fprintf(stderr, "Panel opacity: format %x scale %u red=%u; expected about 50\n", formats[f], scale, red);
                    return 1;
                }
                guards(allocation, count);
                draw_panel(&canvas, 175);
                uint32_t ink = canvas.pixels[0];
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
    puts("Production software panel: all six scales, four formats, menu states, buffer guards and 85% background blending passed (not a display test).");
}
