"""Hook sites and signatures for the local 1.1.1 executable, in analysis addresses."""

import hashlib
import struct

BASE = 0x80020000
TITLE_ID = 0x0400486024DEC000
BUILD_ID = "E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348"
TEXT_HASH = "f71eb7b75b11dcc8d955f3f6e987a098573288d9ce41aa22a96272541e858c5f"
CAVE = BASE + 0x3C46280
CAVE_END = BASE + 0x3C46400
EXTRA_CAVES = [(BASE + 0x3C46FB0, BASE + 0x3C47000), (BASE + 0x3C461D0, BASE + 0x3C46200),
               (BASE + 0x3C46400, BASE + 0x3C46500)]
TABLE = BASE + 0x4E9F800
FOLIAGE_MODULE = 0x84E42540

# Site, four original words, handler, and original function (None for function entry).
HOOKS = [
    (0x834894F8, "be641f9448008052f4031f2af7031f2a", "pad_dual", 0x83C627F0),
    (0x83489524, "bb641f9468008052fc031f2af7031f2a", "pad_left", 0x83C62810),
    (0x83489550, "b4641f948800805237008052fc031f2a", "pad_right", 0x83C62820),
    (0x8348966C, "65641f94fc031f2af4031f2af7031f2a", "pad_full", 0x83C62800),
    (0x83489698, "66641f9428008052fc031f2af4031f2a", "pad_handheld", 0x83C62830),
    (0x8348BFF0, "a8c8789708c100d0e043009108f545f9", "graphics_load", 0x812BE290),
    (0x8348C0D4, "6fc8789717c100b0e2430091e00319aa", "graphics_load", 0x812BE290),
    (0x832E1DB0, "7f2303d5fd7bbda9f50b00f9f44f02a9", "save_load", None),
    (0x832F8500, "7f2303d5fd7bbea9f30b00f9fd030091", "save_ready", None),
    (0x832DBCE0, "7f2303d5fd7bbea9f30b00f9fd030091", "save_from_game_over", None),
    (0x804BD8C0, "7f2303d5ffc306d1e8a300fdfd7b15a9", "life_update", None),
    (0x82FAE760, "292040290a1040b94a0100370a1040b9", "consume_magic", None),
    (0x80954990, "58020094880e40f9091c02f0083941f9", "ground_motion", 0x809552F0),
    (0x8282FB44, "43282094e923446deb2b436dec1340fd", "swim_motion", 0x83039C50),
    (0x827ECF40, "a8f82894e00316aa96104394688e40b9", "climb_motion", 0x8322B1E0),
    (0x827F07E4, "7fea2894e00313aa6d024394dcfeff17", "climb_motion", 0x8322B1E0),
    (0x80BC7730, "7f2303d5ff0301d1fd7b01a9f65702a9", "autosave_gate", None),
    (0x807490C0, "7f2303d5ff4302d1ef3b016ded33026d", "flight_step", None),
    (0x80150180, "7f2303d5ef3bb66ded33016deb2b026d", "flight_job", None),
    (0x82C415E0, "7f2303d5ea0f18fce923016dfd7b02a9", "noclip_move", None),
    # Ordinary player-ammunition use only. Generic event/quest removals remain stock.
    # The second site is a tail branch; preserve B versus BL when redirecting it.
    (0x82802D64, "8f082f94689240b908011e3203000014", "ammo_use", 0x833C4FA0),
    (0x828171A4, "7fb72e14f44f41a9fd7bc2a8ff0f5fd6", "ammo_use", 0x833C4FA0),
    (0x82E19CD8, "b2ac1694e00315aae10313aa08008012", "ammo_use", 0x833C4FA0),
    (0x82E1CE58, "52a01694a8035ff8093941f9690000b4", "ammo_use", 0x833C4FA0),
    (0x82E20708, "26921694a84301d160a2019163e55597", "ammo_use", 0x833C4FA0),
    # Nuts use the shared event command even in ordinary gameplay. Restrict this
    # bridge to single nuts, zero flags and the same safe-world/Link policy.
    (0x828D5168, "8ebf2b94d9ffff172800001400000000", "ammo_nut_use", 0x833C4FA0),
    # Only the airborne-to-ground calculator's final Fall (cause 4) dispatch.
    # Other callers of the shared damage queue retain their native instructions.
    (0x804A2A54, "9f0b00946effff170820417929008052", "flight_fall_damage", 0x804A58D0),
    # Accepted external physical hits, after native attack acceptance.
    (0x804A99FC, "280128b888265c291f01096b0a090054", "combat_hook", None),
    # Gameplay camera view matrix has just been built; source camera remains stock.
    (0x804E5068, "681240f9685800b415a940f9550300b4", "free_camera_hook", None),
    # Resolve the drop list used by the native item-spawn job.
    (0x81217A60, "e0f27394e80240b9f31f01f9685b0235", "grass_drop", 0x82F145E0),
]
EXTRA_HANDLERS = {"autosave_gate", "flight_step", "flight_job", "noclip_move", "ammo_use", "ammo_nut_use",
                  "flight_fall_damage", "combat_hook", "free_camera_hook", "grass_drop"}

FUNCTIONS = {
    "drop_list_resolve": (0x82F145E0, "7f2303d5eb2bb86de923016dfd7b02a9"),
    "drop_request_owner": (0x807A3704, "080c40f9282000f9083941f9081941f9"),
    "drop_override_resolve": (0x82F14888, "de747397080340f9ec0316aaf69f00f9"),
    "drop_default_table": (0x82F155E8, "68a20a916a5601f957ec00b0e88f00f9"),
    "drop_list_allocate": (0x82F14A34, "740a40f9740000b4880240f9680a00f9"),
    "drop_list_name": (0x82F14A8C, "082740f9880200f9095380b90801098b"),
    "drop_list_consume": (0x81217FC0, "390140f9280b40b9882c0234280340f9"),
    "drop_actor_death_dispatch": (0x81217834, "6202088b22000094686a763808011e32"),
    "drop_actor_death_cause": (0x812177F4, "686a763875ca1ab974ce1ab948021837"),
    "drop_actor_hash": (0x801939E0, "0c0040396c0800342aaa855268b28652"),
    "foliage_scene": (0x80ED03D0, "140140f988c2403928010037957a40f9"),
    "foliage_queue": (0x80224CAC, "009841f9210040f927010094689e41f9"),
    "foliage_drop_source": (0x80225398, "62e20291ffc302f8ff4303f8ff9b0079"),
    "foliage_request_flags": (0x802253D0, "e89b0079e85302d0083142f9000140f9"),
    "foliage_chance_gate": (0x82F14790, "c8364139fa0300aae10316aa1f050071"),
    "foliage_table_path": (0x82F1493C, "281f40f9294380b9f6031aaa0801098b"),
    "combat_contact_channel": (0x804A77A8, "a94311b885100094d54000b4a80240f9"),
    "combat_contact_source": (0x804AA8D0, "898201914201899a8102088b8dfeff97"),
    "combat_source_generation": (0x804A8B08, "693240b93f050031e0110054681640f9"),
    "combat_source_handle": (0x804A8B14, "681640f90a4940b95f01096b61110054"),
    "combat_source_actor": (0x804A8B24, "082140f9281100b4082d40b9081d0051"),
    "combat_packet_cap": (0x804A87AC, "6a7240395f050071c10000546a0240b9"),
    "combat_packet_cap_amount": (0x804A87BC, "6b1a40b95f010b6b4ab18b1a6a0200b9"),
    "combat_receiver_layout": (0x804A9140, "880640f9083941f9884300b4085d41f9"),
    "combat_receiver_health": (0x804A9150, "484300b408094ef9084300b4080940b9"),
    "combat_external_offset": (0x804A5230, "604208916102158b5a07009460824df9"),
    "combat_life_backref": (0x804BD938, "740e40f9883a41f9a80000b4095941f9"),
    "combat_native_health_clamp": (0x804BDCCC, "0a0940b96b0e4ef94b0000b46b0940b9"),
    "combat_native_queue_flags": (0x804A99D0, "680240b9691640b98a1240b92a010037"),

    "camera_view_source_layout": (0x804E4E58, "8006066d820e076d8416086d3240621e"),
    "camera_view_matrix_layout": (0x804E4FE0, "901e012d8512052dd208251ef5c0221e"),
    "fall_damage_dispatch": (0x804A58D0, "7f2303d5fd7bbda9f65701a9f44f02a9"),
    "fall_ground_transition": (0x804A27F4, "89b655398ab2553988b615398a000037"),
    "fall_start_height_reference": (0x804A28E4, "e00315aaaae70d94000040bd96000037"),
    "fall_high_threshold_reference": (0x804A2904, "e00315aa32adab94080040bde00315aa"),
    "fall_instant_kill_reference": (0x804A2910, "e00315aa5fadab94010040bd806645bd"),
    "fall_cause_reference": (0x804A2930, "09808052e80b00b9e92b00b996000034"),
    "fall_high_rate_reference": (0x804A2B10, "0838281ec7acab94000040bd0008281e"),
    "request_pause": (0x80B1B980, "e91c0290e803002a201d47f9400500b4"),
    "release_pause": (0x80B1BA70, "e91c0290e803002a201d47f9400600b4"),
    "pause_profile_reference": (0x830510F8, "206b00f000e42991e1031f2a370a4597"),
    "read_bool": (0x8022FD90, "620300347f2303d5ffc300d1fd7b01a9"),
    "challenge_manager_reference": (0x831FE414, "48e200f008412591092040b90a0140f9"),
    "challenge_step_layout": (0x831FE600, "4ae101914d0549b8bf01096ba0000054"),
    "challenge_order_layout": (0x831FE620, "087d40b909010b2a490df8371f010b6b"),
    "challenge_busy_reference": (0x82D154A0, "094080b92800805208d0013949010034"),
    "read_rupees": (0x80A87C30, "7f2303d5ff8300d1fd7b01a9fd430091"),
    "wallet_cap": (0x80A876A0, "7f2303d5ffc300d1fd7b02a9fd830091"),
    "event_running": (0x80BEC100, "093440f9690000b5e0031f2ac0035fd6"),
    "nvn_present_slot": (0x812BEA78, "68cf01f0615001d02120149108f546f9"),
    "nvn_window_textures_slot": (0x812BEAE8, "68cf01f0215101f021d00991080547f9"),
    "nvn_window_init_slot": (0x812BECC4, "68cf01f0c15a01d0216c2391084947f9"),
    "nvn_window_fini_slot": (0x812BECE0, "68cf01f0c15001d021200591084d47f9"),
    "read_struct_int": (0x8019B3B0, "7f2303d5ffc300d1fd7b02a9fd830091"),
    "write_struct_int": (0x809D39E0, "7f2303d5ffc300d1fd7b02a9fd830091"),
    "magic_component": (0x80964E50, "083841f9680000b400ad41f9c0035fd6"),
    "heal_magic": (0x82EA0F80, "7f2303d5ffc300d1fd7b01a9f44f02a9"),
    "ammo_count": (0x805B63E0, "7f2303d5ffc305d1fd7b11a9fc6f12a9"),
    "ammo_capacity": (0x805B5CF0, "7f2303d5ff8302d1fd7b07a9f65708a9"),
    "action_guide_opened": (0x80A96B10, "7f2303d5ffc300d1fd7b01a9f31300f9"),
    "give_ammo": (0x8112D570, "7f2303d5fd7bbaa9fc6f01a9fa6702a9"),
    "consume_ammo": (0x833C4FA0, "7f2303d5fd7bbaa9fc6f01a9fa6702a9"),
    "inventory_equipped": (0x80A91080, "7f2303d5ffc302d1fd7b06a9f93b00f9"),
    "inventory_unequip": (0x833C6A40, "7f2303d5ffc302d1fd7b07a9f85f08a9"),
    "child_link": (0x804D6DB0, "7f2303d5ff8300d1fd7b01a9fd430091"),
    "ground_velocity": (0x809552F0, "7f2303d5ffc300d1fd7b02a9fd830091"),
    "swim_velocity": (0x83039C50, "7f2303d5ff0303d1ee2300fded33056d"),
    "climb_velocity": (0x8322B1E0, "7f2303d5fd7bbda9f50b00f9f44f02a9"),
    "climb_vector_reader": (0x8323205C, "025860bd006460bd016860bd16084091"),
    "event_request_init": (0x80D99F10, "7f2303d5fd7bbda9f50b00f9f44f02a9"),
    "event_request_tail": (0x80D9A2C4, "89722a91759a1ab99f6e2c39694a0df9"),
    "event_scope_reference": (0x80D9A9F8, "086040f909c880b9f30301aaf40300aa"),
    "event_request": (0x80D9A820, "08b840b9080858377f2303d5ff0301d1"),
    "event_request_cleanup": (0x8022B6F0, "7f2303d5fd7bbca9f70b00f9f65702a9"),
    "read_text": (0x80CD0EA0, "620800347f2303d5ffc300d1fd7b01a9"),
    "write_text": (0x80D44E50, "7f2303d5ff0301d1fd7b01a9f65702a9"),
    "write_vector": (0x80EA56D0, "420700347f2303d5ff0301d1fd7b02a9"),
    "ground_state_reference": (0x82ACBA20, "080440f9080540f908f940f9083941f9"),
    "water_body_reference": (0x8084FC2C, "080540f9081d40f9280200b47f2303d5"),
    "water_flags_reference": (0x8084FC4C, "09a1437949040037081040f968010837"),
    "flight_camera_reference": (0x829F7B5C, "08d146f9080140f900e97abd01f97abd"),
    "flight_job_member": (0x838B0A80, "7f2303d5ff0304d1fd7b0ca9f76b00f9"),
    "set_linear_velocity": (0x809F8440, "001040f9030000140000000000000000"),
    "noclip_position": (0x806D2530, "7f2303d5fd7bbda9fc5701a9f44f02a9"),
    "noclip_get_position": (0x838B3BD0, "7f2303d5fd7bbda9fc5701a9f44f02a9"),
    "noclip_velocity_layout": (0x809F84FC, "880240f9890a40b969ea00b9687200f9"),
    "noclip_speed_layout": (0x82AC9E90, "081540f900d941bd081040f908010837"),
    "noclip_direction_layout": (0x80559E20, "081440f9210040bd00c141bd09010791"),
    "geometry_body": (0x80E86F20, "083841f9680000b4002940f9c0035fd6"),
    "geometry_bounds": (0x83B9B340, "080040f9c80c00b47f2303d5ff0301d1"),
    "geometry_layer": (0x83B9B560, "080040f9080200b47f2303d5fd7bbfa9"),
    "geometry_filter": (0x83B9B750, "090040f9090200b4201140f9c00100b4"),
    "geometry_query_init": (0x8288A1F0, "7f2303d5ffc301d1fd7b03a9f85f04a9"),
    "geometry_sphere_init": (0x803BD550, "7f2303d5ff8302d1e83300fdfd7b07a9"),
    "geometry_filter_bits": (0x803F3BF0, "484502d0297c7d93088147f9080140f9"),
    "geometry_filter_set": (0x803F3C60, "083c40f9291040390a0940b93f010071"),
    "geometry_overlap": (0x803C2FA0, "7f2303d5ff0301d1fd7b01a9f51300f9"),
    "geometry_query_fini": (0x82729680, "7f2303d5fd7bbea9f30b00f9fd030091"),
    "geometry_ray": (0x80E86730, "08604439080618377f2303d5fd7bbea9"),
    "geometry_ray_filter": (0x80E86AC0, "083040f9291040390a0940b93f010071"),
    "geometry_shape_type": (0x8024C3F0, "c0008052c0035fd60000000000000000"),
    "loading_counter_reference": (0x80B1BA08, "0971009133012ab8010940b9088c44f8"),
    "loading_pause_reference": (0x832A77AC, "010940b9088c44f8e2031f2a080140f9"),
    "read_ui_type": (0x808073D0, "c20700347f2303d5ffc300d1fd7b01a9"),
    "ui_type_write_reference": (0x803FAF10, "223c8852e103002ae00314aaa2f1aa72"),
}

CAS_SITE = 0x804BDCF8
CAS_SIGNATURE = "ac7dab887f010a6ba1720054680a4ef9"
CHEAT_SIGNATURES = {
    "grass_drop": "INCREASED_LUCK", "drop_list_resolve": "INCREASED_LUCK",
    "drop_request_owner": "INCREASED_LUCK", "drop_override_resolve": "INCREASED_LUCK",
    "drop_default_table": "INCREASED_LUCK", "drop_list_allocate": "INCREASED_LUCK",
    "drop_list_name": "INCREASED_LUCK", "drop_list_consume": "INCREASED_LUCK",
    "drop_actor_death_dispatch": "INCREASED_LUCK", "drop_actor_hash": "INCREASED_LUCK",
    "drop_actor_death_cause": "INCREASED_LUCK",
    "foliage_scene": "INCREASED_LUCK", "foliage_queue": "INCREASED_LUCK",
    "foliage_drop_source": "INCREASED_LUCK", "foliage_request_flags": "INCREASED_LUCK",
    "foliage_chance_gate": "INCREASED_LUCK", "foliage_table_path": "INCREASED_LUCK",
    "combat_contact_channel": "ONE_HIT DAMAGE",
    "combat_contact_source": "ONE_HIT DAMAGE",
    "combat_source_generation": "ONE_HIT DAMAGE",
    "combat_source_handle": "ONE_HIT DAMAGE",
    "combat_source_actor": "ONE_HIT DAMAGE",
    "combat_packet_cap": "ONE_HIT DAMAGE",
    "combat_packet_cap_amount": "ONE_HIT DAMAGE",
    "combat_receiver_layout": "ONE_HIT DAMAGE",
    "combat_receiver_health": "ONE_HIT DAMAGE",
    "combat_external_offset": "ONE_HIT DAMAGE",
    "combat_life_backref": "ONE_HIT DAMAGE",
    "combat_native_health_clamp": "ONE_HIT DAMAGE",
    "combat_native_queue_flags": "ONE_HIT DAMAGE",

    "combat_hook": "ONE_HIT DAMAGE",
    "free_camera_hook": "FREE_CAMERA",
    "camera_view_source_layout": "FREE_CAMERA",
    "camera_view_matrix_layout": "FREE_CAMERA",
    "flight_fall_damage": "FLY FLIGHT_SPEED", "fall_damage_dispatch": "FLY FLIGHT_SPEED",
    "fall_ground_transition": "FLY FLIGHT_SPEED", "fall_start_height_reference": "FLY FLIGHT_SPEED",
    "fall_high_threshold_reference": "FLY FLIGHT_SPEED", "fall_instant_kill_reference": "FLY FLIGHT_SPEED",
    "fall_cause_reference": "FLY FLIGHT_SPEED", "fall_high_rate_reference": "FLY FLIGHT_SPEED",
    "challenge_manager_reference": "TRAVEL", "challenge_step_layout": "TRAVEL",
    "challenge_order_layout": "TRAVEL", "challenge_busy_reference": "TRAVEL",
    "read_rupees": "RUPEES", "wallet_cap": "RUPEES",
    "read_struct_int": "MAGIC", "write_struct_int": "RUPEES MAGIC",
    "magic_component": "MAGIC", "heal_magic": "MAGIC", "consume_magic": "MAGIC",
    "ammo_count": "ITEMS INVENTORY", "ammo_capacity": "ITEMS INVENTORY",
    "give_ammo": "ITEMS INVENTORY", "child_link": "ITEMS TRAVEL INVENTORY",
    "action_guide_opened": "ITEMS INVENTORY",
    "ammo_use": "ITEMS", "ammo_nut_use": "ITEMS", "consume_ammo": "ITEMS INVENTORY",
    "ground_motion": "SPEED", "swim_motion": "SPEED", "climb_motion": "SPEED",
    "ground_velocity": "SPEED", "swim_velocity": "SPEED", "climb_velocity": "SPEED",
    "climb_vector_reader": "SPEED",
    "event_request_init": "TRAVEL", "event_request": "TRAVEL", "event_request_cleanup": "TRAVEL",
    "event_request_tail": "TRAVEL",
    "event_scope_reference": "TRAVEL",
    "read_text": "TRAVEL FREE_CAMERA ONE_HIT DAMAGE", "write_text": "TRAVEL", "write_vector": "TRAVEL",
    "ground_state_reference": "TRAVEL",
    "water_body_reference": "TRAVEL", "water_flags_reference": "TRAVEL",
    "autosave_gate": "TRAVEL INVENTORY",
    "read_ui_type": "TRAVEL INVENTORY", "ui_type_write_reference": "TRAVEL INVENTORY",
    "flight_step": "FLY FLIGHT_SPEED",
    "flight_job": "FLY FLIGHT_SPEED", "flight_job_member": "FLY FLIGHT_SPEED",
    "set_linear_velocity": "FLY FLIGHT_SPEED NOCLIP",
    "flight_camera_reference": "FLY FLIGHT_SPEED",
    "noclip_move": "NOCLIP", "noclip_position": "NOCLIP", "noclip_get_position": "NOCLIP",
    "noclip_velocity_layout": "NOCLIP", "noclip_speed_layout": "NOCLIP",
    "noclip_direction_layout": "NOCLIP",
    "loading_counter_reference": "FLY FLIGHT_SPEED NOCLIP",
    "loading_pause_reference": "FLY FLIGHT_SPEED NOCLIP",

    "geometry_body": "NOCLIP",
    "geometry_bounds": "NOCLIP",
    "geometry_layer": "NOCLIP",
    "geometry_filter": "NOCLIP",
    "geometry_query_init": "NOCLIP",
    "geometry_sphere_init": "NOCLIP",
    "geometry_filter_bits": "NOCLIP",
    "geometry_filter_set": "NOCLIP",
    "geometry_overlap": "NOCLIP",
    "geometry_query_fini": "NOCLIP",
    "geometry_ray": "NOCLIP",
    "geometry_ray_filter": "NOCLIP",
    "geometry_shape_type": "NOCLIP",

}


def branch(site, target, link=False):
    delta = target - site
    if delta % 4 or not -(1 << 27) <= delta < (1 << 27):
        raise ValueError("branch target out of range")
    return (0x94000000 if link else 0x14000000) | ((delta // 4) & 0x3ffffff)


def adrp(site, target, rd):
    delta = (target >> 12) - (site >> 12)
    if not -(1 << 20) <= delta < (1 << 20):
        raise ValueError("adrp target out of range")
    return 0x90000000 | (delta & 3) << 29 | ((delta >> 2) & 0x7ffff) << 5 | rd


def ldr_pointer(target, rn, rt):
    return 0xF9400000 | ((target % 4096) // 8) << 10 | rn << 5 | rt


def make_patch(image=None):
    # The checked-in profile is sufficient to reproduce the known 1.1.1 patch.
    # An optional private dump additionally verifies every source byte before building.
    flat = None
    if image is not None:
        if image[:4] != b"NSO0" or image[0x40:0x60] != bytes.fromhex(BUILD_ID).ljust(32, b"\0"):
            raise ValueError("expected the Build 130 uncompressed NSO dump of game 1.1.1")
        flat = image[0x100:]
        if hashlib.sha256(flat[:0x3C461D0]).hexdigest() != TEXT_HASH:
            raise ValueError("main text hash differs from the original 1.1.1 executable")
        if any(flat[CAVE - BASE:CAVE_END - BASE]) or any(
                any(flat[start - BASE:end - BASE]) for start, end in EXTRA_CAVES):
            raise ValueError("expected unused zero-filled text padding")

    records = {}
    features = {}
    declarations = ["/* Generated from the verified profile; optional private dump checked by builder. */"]
    table_slots = {}
    cave = CAVE
    extra_index = 0
    extra_cave = EXTRA_CAVES[0][0]

    def check(site, signature):
        original = bytes.fromhex(signature)
        if flat is not None and flat[site - BASE:site - BASE + len(original)] != original:
            raise ValueError(f"signature mismatch at main+{site - BASE:#x}")
        return struct.unpack("<4I", original)

    for site, signature, handler, target in HOOKS:
        words = check(site, signature)
        slot = table_slots.setdefault(handler, TABLE + len(table_slots) * 8)
        entry = target is None
        primary_cave = cave
        extra = handler in EXTRA_HANDLERS
        if extra:
            stub_bytes = 24 if entry else 20
            if extra_cave + stub_bytes > EXTRA_CAVES[extra_index][1]:
                extra_index += 1
                if extra_index >= len(EXTRA_CAVES):
                    raise ValueError("additional hook bridges exceed verified padding")
                extra_cave = EXTRA_CAVES[extra_index][0]
            cave = extra_cave
        fallback = [words[0], branch(cave + 20, site + 4)] if entry else [branch(cave + 16, target)]
        stub = [adrp(cave, slot, 16), ldr_pointer(slot, 16, 16), 0xB4000050, 0xD61F0200, *fallback]
        replacement = branch(site, cave, words[0] >> 26 == 0b100101)
        records[site] = struct.pack("<I", replacement)
        records[cave] = struct.pack(f"<{len(stub)}I", *stub)
        features[site] = features[cave] = CHEAT_SIGNATURES.get(handler, "")
        declarations.append(f"#define SLOT_{handler} 0x{slot:X}ull")
        if entry:
            declarations.append(f"#define ORIGINAL_{handler} 0x{cave + 16:X}ull")
        cave += len(stub) * 4
        if extra:
            extra_cave = cave
            cave = primary_cave

    # At the life CAS, x13 is the destination and w11/w12 hold old/new health.
    # The dispatch slot contains the address of Link's current-life field only while protected.
    # Preserve x16/x17 across the inline hook. The following stock CMP replaces NZCV.
    original = check(CAS_SITE, CAS_SIGNATURE)
    life_slot = TABLE + len(table_slots) * 8
    life_stub = [
        0xA9BF47F0,  # stp x16,x17,[sp,#-16]!
        adrp(cave + 4, life_slot, 16), ldr_pointer(life_slot, 16, 16),
        0xEB0D021F,  # cmp x16,x13
        0x54000081,  # b.ne restore
        0x7100017F,  # cmp w11,#0
        0x7A4CC164,  # ccmp w11,w12,#4,gt
        0x1A8CC16C,  # csel w12,w11,w12,gt
        0xA8C147F0,  # ldp x16,x17,[sp],#16
        original[0], branch(cave + 40, CAS_SITE + 4),
    ]
    records[CAS_SITE] = struct.pack("<I", branch(CAS_SITE, cave))
    records[cave] = struct.pack("<11I", *life_stub)
    features[CAS_SITE] = features[cave] = "GOD"
    if cave + len(life_stub) * 4 > CAVE_END:
        raise ValueError("hook bridges exceed the reserved text padding")
    declarations.append(f"#define LIFE_SLOT 0x{life_slot:X}ull")
    declarations.append(f"#define TABLE 0x{TABLE:X}ull")
    declarations.append(f"#define FOLIAGE_MODULE 0x{FOLIAGE_MODULE:X}ull")

    checks = []
    for site, sig, handler, _ in HOOKS:
        checks.append((site, records[site] + bytes.fromhex(sig)[4:]))
    checks.append((CAS_SITE, records[CAS_SITE] + bytes.fromhex(CAS_SIGNATURE)[4:]))
    for name, (site, sig) in FUNCTIONS.items():
        check(site, sig)
        declarations.append(f"#define GAME_{name} 0x{site:X}ull")
        checks.append((site, bytes.fromhex(sig)))
        features[site] = CHEAT_SIGNATURES.get(name, "")

    declarations = list(dict.fromkeys(declarations))
    declarations.append("#ifdef HCM_PROFILE_CHECKS\nstatic const struct Signature signatures[] = {")
    caves = [(CAVE, CAVE_END), *EXTRA_CAVES]
    for site, data in checks + [(site, data) for site, data in records.items()
                               if any(start <= site < end for start, end in caves)]:
        words = ", ".join(f"0x{word:08x}" for word, in struct.iter_unpack("<I", data))
        mask = " | ".join(f"(1u << {feature})" for feature in features[site].split()) or "0"
        declarations.append(f"    {{0x{site:X}ull, {len(data)//4}, {mask}, {{{words}}}}},")
    declarations.append("};\n#endif\n")

    patch = bytearray(b"IPS32")
    for site, data in sorted(records.items()):
        patch.extend(struct.pack(">IH", site - BASE + 0x100, len(data)))
        patch.extend(data)
    return patch + b"EEOF", "\n".join(declarations)
