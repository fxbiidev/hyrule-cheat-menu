#include "guest.h"
#include "preferences.h"

typedef uintptr_t File;
static struct {
    uint32_t (*mount)(const char *);
    uint32_t (*mkdir)(const char *);
    uint32_t (*create)(const char *, int64_t);
    uint32_t (*remove)(const char *);
    uint32_t (*open)(File *, const char *, int);
    uint32_t (*size)(int64_t *, File);
    uint32_t (*read)(File, int64_t, void *, size_t);
    uint32_t (*write)(File, int64_t, const void *, size_t, const uint32_t *);
    uint32_t (*flush)(File);
    void (*close)(File);
} fs;
static bool attempted, mounted;
static int current_slot = -1;
static const char *status = "Preferences have not been loaded.";
static const char *const paths[] = {
    "hcm:/Hyrule-Cheat-Menu/preferences-a.bin", "hcm:/Hyrule-Cheat-Menu/preferences-b.bin"
};

static bool initialize(void) {
    if (attempted) return mounted;
    attempted = true;
    #define RESOLVE(member, name) *(uintptr_t *)&fs.member = runtime_symbol(name)
    RESOLVE(mount, "_ZN2nn2fs11MountSdCardEPKc");
    RESOLVE(mkdir, "_ZN2nn2fs15CreateDirectoryEPKc");
    RESOLVE(create, "_ZN2nn2fs10CreateFileEPKcl");
    RESOLVE(remove, "_ZN2nn2fs10DeleteFileEPKc");
    RESOLVE(open, "_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci");
    RESOLVE(size, "_ZN2nn2fs11GetFileSizeEPlNS0_10FileHandleE");
    RESOLVE(read, "_ZN2nn2fs8ReadFileENS0_10FileHandleElPvm");
    RESOLVE(write, "_ZN2nn2fs9WriteFileENS0_10FileHandleElPKvmRKNS0_11WriteOptionE");
    RESOLVE(flush, "_ZN2nn2fs9FlushFileENS0_10FileHandleE");
    RESOLVE(close, "_ZN2nn2fs9CloseFileENS0_10FileHandleE");
    #undef RESOLVE
    if (!fs.mount || !fs.mkdir || !fs.create || !fs.remove || !fs.open || !fs.size ||
        !fs.read || !fs.write || !fs.flush || !fs.close) {
        status = "No storage API. Favorites are session-only.";
        debug_log("[hyrule] filesystem exports unavailable; session preferences only\n");
        return false;
    }
    if (fs.mount("hcm")) {
        status = "No SD storage. Favorites are session-only.";
        debug_log("[hyrule] SD mount refused; session preferences only\n");
        return false;
    }
    // Already-existing directories are harmless. Open/Create report other failures below.
    fs.mkdir("hcm:/Hyrule-Cheat-Menu");
    mounted = true;
    return true;
}

static bool read_slot(unsigned slot, Preferences *preferences) {
    File file = 0;
    if (fs.open(&file, paths[slot], 1)) return false;
    int64_t size = 0;
    uint8_t bytes[PREFERENCES_BYTES];
    bool valid = !fs.size(&size, file) && size >= 28 && size <= PREFERENCES_BYTES &&
                 !fs.read(file, 0, bytes, (size_t)size);
    fs.close(file);
    return valid && preferences_decode(preferences, bytes, (size_t)size);
}

static bool same_preferences(const Preferences *left, const Preferences *right) {
    if (left->generation != right->generation || left->movement_speed != right->movement_speed ||
        left->flight_speed != right->flight_speed || left->favorite_count != right->favorite_count)
        return false;
    for (unsigned i = 0; i < left->favorite_count; ++i)
        if (left->favorites[i] != right->favorites[i]) return false;
    return true;
}

bool storage_load(Preferences *preferences) {
    if (!initialize()) return false;
    Preferences candidate[2] = {{0}};
    bool valid[2] = {read_slot(0, &candidate[0]), read_slot(1, &candidate[1])};
    if (!valid[0] && !valid[1]) {
        status = "Favorites save when the menu closes.";
        return true;
    }
    current_slot = valid[1] && (!valid[0] || (int32_t)(candidate[1].generation - candidate[0].generation) > 0);
    *preferences = candidate[current_slot];
    status = "Favorites and preferences loaded.";
    debug_log("[hyrule] valid preference generation loaded from SD\n");
    return true;
}

bool storage_save(Preferences *preferences) {
    if (!initialize()) return false;
    unsigned slot = current_slot == 0 ? 1 : 0;
    Preferences pending = *preferences;
    ++pending.generation;
    uint8_t bytes[PREFERENCES_BYTES];
    size_t length = preferences_encode(&pending, bytes);
    if (!length) return false;
    // Write the inactive generation. A short write or crash leaves the previous file valid.
    fs.remove(paths[slot]);
    if (fs.create(paths[slot], (int64_t)length)) {
        status = "Cannot save preferences. Previous file kept.";
        return false;
    }
    File file = 0;
    if (fs.open(&file, paths[slot], 2)) {
        status = "Cannot open preferences for writing.";
        return false;
    }
    uint32_t options = 1;
    bool written = !fs.write(file, 0, bytes, length, &options) && !fs.flush(file);
    fs.close(file);
    Preferences checked = {0};
    if (!written || !read_slot(slot, &checked) || !same_preferences(&checked, &pending)) {
        // A checksum-valid but different payload must not become the newest
        // generation on the next launch. The active slot remains untouched.
        fs.remove(paths[slot]);
        status = "Save verification failed. Previous file kept.";
        return false;
    }
    *preferences = pending;
    current_slot = (int)slot;
    status = "Favorites and preferences saved.";
    debug_log("[hyrule] preference generation flushed and verified\n");
    return true;
}

const char *storage_status(void) { return status; }
