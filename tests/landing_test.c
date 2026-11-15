#include "guest.h"
#include "landing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uintptr_t main_base = GAME_BASE;
static unsigned epoch = 7, forwarded, protected, queried;
static uintptr_t state_bytes[2], unreadable;
static const uintptr_t link_actor = 0x2000, other_actor = 0x3000;
static bool grace, reset_in_mapping, reset_in_consume;
static uintptr_t forwarded_state, forwarded_definition;
static void *forwarded_request;
static unsigned char request_before[0x28];
unsigned save_epoch(void) { return epoch; }
bool mapped(uintptr_t address, size_t size, unsigned permissions) {
    assert((size == 0x10 || size == 0x28) && permissions == 1);
    if (reset_in_mapping) { ++epoch; reset_in_mapping = false; }
    return address && address != unreadable;
}
bool flight_consume_landing_protection(uintptr_t actor) {
    ++queried;
    if (reset_in_consume) { ++epoch; reset_in_consume = false; }
    if (!grace || actor != link_actor) return false;
    grace = false;
    return true;
}
void debug_log(const char *message) {
    assert(!strcmp(message, "[hyrule] one Fly landing fall-damage request suppressed\n"));
    ++protected;
}
static void mock_dispatch(uintptr_t state, uintptr_t definition, void *request) {
    ++forwarded; forwarded_state = state; forwarded_definition = definition; forwarded_request = request;
    assert(!memcmp(request, request_before, sizeof request_before));
}
#define GAME_fall_damage_dispatch ((uintptr_t)mock_dispatch)
#include "../src/landing.c"

static void dispatch(unsigned char request[0x28]) {
    memcpy(request_before, request, sizeof request_before);
    flight_fall_damage((uintptr_t)state_bytes, 0x1234, request);
}
static void passthrough(unsigned char request[0x28]) {
    unsigned before = forwarded;
    dispatch(request);
    assert(forwarded == before + 1 && forwarded_state == (uintptr_t)state_bytes &&
           forwarded_definition == 0x1234 && forwarded_request == request);
}
int main(void) {
    _Alignas(8) unsigned char request[0x28] = {0};
    *(int *)request = 8;
    *(unsigned *)(request + 0x14) = 4;
    state_bytes[1] = link_actor;
    passthrough(request); // Fly OFF without a matching landing shield is native.
    grace = true;
    dispatch(request); assert(protected == 1 && !grace);
    passthrough(request); assert(protected == 1); // Exactly one dispatch was protected.
    grace = true; state_bytes[1] = other_actor;
    passthrough(request); assert(grace);
    state_bytes[1] = 0; unsigned before = queried;
    passthrough(request); assert(queried == before && grace);
    state_bytes[1] = link_actor;
    // Every other native cause passes through unchanged without querying grace.
    for (unsigned cause = 0; cause < 32; ++cause) if (cause != 4) {
        *(unsigned *)(request + 0x14) = cause; before = queried;
        passthrough(request); assert(queried == before && grace);
    }
    *(unsigned *)(request + 0x14) = ~0u; passthrough(request);
    *(unsigned *)(request + 0x14) = 4;
    *(int *)request = 0; before = queried;
    passthrough(request); assert(queried == before && grace);
    *(int *)request = -8; passthrough(request); assert(queried == before);
    *(int *)request = 8;
    unreadable = (uintptr_t)state_bytes; passthrough(request); assert(grace);
    unreadable = (uintptr_t)request; passthrough(request); assert(grace);
    unreadable = 0; reset_in_mapping = true; before = queried;
    passthrough(request); assert(queried == before && grace);
    reset_in_consume = true;
    passthrough(request); assert(protected == 1); // A reset inside the shield query fences suppression.
    grace = true; dispatch(request); assert(protected == 2 && !grace);
    puts("Fall-only landing guard: one-shot Link scope, all other causes, exact passthrough ABI, invalid pointers and reload fences passed.");
}
