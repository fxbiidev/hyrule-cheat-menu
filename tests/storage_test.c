#include "guest.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct { uint8_t bytes[PREFERENCES_BYTES]; size_t size; bool exists; } files[2];
static bool fail_create, fail_write, fail_mount;
static unsigned valid_write_mismatch;
static unsigned flushes;
static int path_slot(const char *path) { return strstr(path, "preferences-b") ? 1 : 0; }
static uint32_t mount_sd(const char *name) { assert(!strcmp(name, "hcm")); return fail_mount; }
static uint32_t make_directory(const char *path) { (void)path; return 0; }
static uint32_t create_file(const char *path, int64_t size) {
    if (fail_create) return 1;
    int index = path_slot(path);
    assert(size >= 0 && size <= PREFERENCES_BYTES);
    files[index].size = (size_t)size;
    files[index].exists = true;
    memset(files[index].bytes, 0, sizeof files[index].bytes);
    return 0;
}
static uint32_t remove_file(const char *path) { files[path_slot(path)].exists = false; return 0; }
static uint32_t open_file(uintptr_t *handle, const char *path, int flags) {
    (void)flags;
    int index = path_slot(path);
    if (!files[index].exists) return 1;
    *handle = (uintptr_t)index + 1;
    return 0;
}
static uint32_t file_size(int64_t *size, uintptr_t handle) { *size = files[handle - 1].size; return 0; }
static uint32_t read_file(uintptr_t handle, int64_t offset, void *out, size_t size) {
    assert(!offset && size <= files[handle - 1].size);
    memcpy(out, files[handle - 1].bytes, size);
    return 0;
}
static uint32_t write_file(uintptr_t handle, int64_t offset, const void *bytes, size_t size,
                            const uint32_t *options) {
    assert(!offset && size <= files[handle - 1].size && *options == 1);
    memcpy(files[handle - 1].bytes, bytes, fail_write ? size / 2 : size);
    if (valid_write_mismatch && !fail_write) {
        Preferences different = {0};
        assert(preferences_decode(&different, files[handle - 1].bytes, size));
        if (valid_write_mismatch == 1) different.movement_speed = (different.movement_speed + 1) % 4;
        else if (valid_write_mismatch == 2) {
            assert(different.favorite_count > 0);
            different.favorites[0] = destinations[2].id;
        } else different.damage = (different.damage + 1) % 4;
        assert(preferences_encode(&different, files[handle - 1].bytes) == size);
    }
    return fail_write;
}
static uint32_t flush_file(uintptr_t handle) { (void)handle; ++flushes; return 0; }
static void close_file(uintptr_t handle) { (void)handle; }

uintptr_t runtime_symbol(const char *name) {
    #define EXPORT(symbol, function) if (!strcmp(name, symbol)) return (uintptr_t)&function
    EXPORT("_ZN2nn2fs11MountSdCardEPKc", mount_sd);
    EXPORT("_ZN2nn2fs15CreateDirectoryEPKc", make_directory);
    EXPORT("_ZN2nn2fs10CreateFileEPKcl", create_file);
    EXPORT("_ZN2nn2fs10DeleteFileEPKc", remove_file);
    EXPORT("_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci", open_file);
    EXPORT("_ZN2nn2fs11GetFileSizeEPlNS0_10FileHandleE", file_size);
    EXPORT("_ZN2nn2fs8ReadFileENS0_10FileHandleElPvm", read_file);
    EXPORT("_ZN2nn2fs9WriteFileENS0_10FileHandleElPKvmRKNS0_11WriteOptionE", write_file);
    EXPORT("_ZN2nn2fs9FlushFileENS0_10FileHandleE", flush_file);
    EXPORT("_ZN2nn2fs9CloseFileENS0_10FileHandleE", close_file);
    return 0;
}
void debug_log(const char *message) { (void)message; }

int main(int argc, char **argv) {
    (void)argv;
    Preferences preferences = {.damage = 2}, restored = {0};
    if (argc > 1) {
        fail_mount = true;
        assert(!storage_load(&preferences));
        assert(preferences_toggle_favorite(&preferences, destinations[0].id));
        assert(!storage_save(&preferences) && preferences.favorite_count == 1);
        puts("Unavailable SD preserves session preferences.");
        return 0;
    }
    assert(storage_load(&preferences));
    assert(preferences_toggle_favorite(&preferences, destinations[0].id));
    assert(storage_save(&preferences) && preferences.generation == 1 && flushes == 1);
    assert(preferences_toggle_favorite(&preferences, destinations[1].id));
    assert(storage_save(&preferences) && preferences.generation == 2);
    files[1].bytes[28] ^= 1;
    assert(storage_load(&restored) && restored.generation == 1 && restored.favorite_count == 1);
    fail_write = true;
    assert(!storage_save(&restored) && restored.generation == 1);
    assert(storage_load(&restored) && restored.generation == 1);
    fail_write = false;
    fail_create = true;
    assert(!storage_save(&restored));
    assert(storage_load(&restored) && restored.generation == 1);
    fail_create = false;
    // A valid checksum and matching generation alone do not prove that the
    // requested settings or Favorites reached storage. Reject and invalidate
    // all three kinds of mismatch so a later load still chooses the previous slot.
    for (valid_write_mismatch = 1; valid_write_mismatch <= 3; ++valid_write_mismatch) {
        assert(!storage_save(&restored) && restored.generation == 1);
        Preferences recovered = {0};
        assert(storage_load(&recovered) && recovered.generation == 1);
        assert(recovered.favorite_count == 1 && recovered.favorites[0] == destinations[0].id);
        assert(recovered.movement_speed == restored.movement_speed && recovered.damage == restored.damage);
    }
    valid_write_mismatch = 0;
    for (unsigned i = 1; i < DESTINATION_COUNT; ++i)
        assert(preferences_toggle_favorite(&restored, destinations[i].id));
    assert(storage_save(&restored));
    memset(&restored, 0, sizeof restored);
    assert(storage_load(&restored) && restored.favorite_count == DESTINATION_COUNT);
    puts("Dual-generation recovery, short writes, checksum-valid payload mismatches, create failure, and all-favorite storage passed.");
    return 0;
}
