#pragma once

#include "menu.h"
#include <stddef.h>

#define GAME_BASE 0x80020000ull
extern uintptr_t main_base;

static inline uintptr_t game(uintptr_t analysis_address) {
    return main_base + analysis_address - GAME_BASE;
}

static inline uintptr_t pointer(uintptr_t address) {
    return *(const uintptr_t *)address;
}

static inline uintptr_t singleton(uintptr_t address) {
    uintptr_t slot = pointer(game(address));
    return slot ? pointer(slot) : 0;
}

void debug_log(const char *text);
bool mapped(uintptr_t address, size_t bytes, unsigned permissions);
bool world_safe(void);
unsigned save_epoch(void);
uint64_t monotonic_ms(void);
uintptr_t runtime_symbol(const char *name);

typedef void *(*Lookup)(void *, const char *);
void graphics_load(void *device, Lookup lookup);
bool overlay_ready(void);
void overlay_begin_capture(void);
bool overlay_captured(void);
void overlay_end_capture(void);
void overlay_publish(const Menu *menu, unsigned available, unsigned experimental);
