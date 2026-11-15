/* Presentation only. All IDs, scenes, ages, spawn names and positions remain in destinations.inc.
 * Shared HyruleField/Market scene exceptions were assigned by exact registry ID and
 * named entrance (or its reference vector label); no runtime prefix categorization. */
const TravelGroupInfo travel_groups[TRAVEL_GROUP_COUNT] = {
    {"Favorites", TRAVEL_ROOT},
    {"Hyrule Field", TRAVEL_ROOT},
    {"Lake Hylia", TRAVEL_ROOT},
    {"Lon Lon Ranch", TRAVEL_ROOT},
    {"Market", TRAVEL_ROOT},
    {"Temple of Time", TRAVEL_ROOT},
    {"Kakariko Village", TRAVEL_ROOT},
    {"Graveyard", TRAVEL_ROOT},
    {"Kokiri Forest", TRAVEL_ROOT},
    {"Lost Woods", TRAVEL_ROOT},
    {"Sacred Forest Meadow", TRAVEL_ROOT},
    {"Death Mountain Trail", TRAVEL_ROOT},
    {"Goron City", TRAVEL_ROOT},
    {"Death Mountain Crater", TRAVEL_ROOT},
    {"Zora's River", TRAVEL_ROOT},
    {"Zora's Domain", TRAVEL_ROOT},
    {"Zora's Fountain", TRAVEL_ROOT},
    {"Gerudo Valley", TRAVEL_ROOT},
    {"Gerudo's Fortress", TRAVEL_ROOT},
    {"Haunted Wasteland", TRAVEL_ROOT},
    {"Desert Colossus", TRAVEL_ROOT},
    {"Dungeons", TRAVEL_ROOT},
    {"Inside the Deku Tree", GROUP_DUNGEONS},
    {"Dodongo's Cavern", GROUP_DUNGEONS},
    {"Jabu-Jabu's Belly", GROUP_DUNGEONS},
    {"Forest Temple", GROUP_DUNGEONS},
    {"Fire Temple", GROUP_DUNGEONS},
    {"Water Temple", GROUP_DUNGEONS},
    {"Shadow Temple", GROUP_DUNGEONS},
    {"Spirit Temple", GROUP_DUNGEONS},
    {"Bottom of the Well", GROUP_DUNGEONS},
    {"Ice Cavern", GROUP_DUNGEONS},
    {"Gerudo Training Ground", GROUP_DUNGEONS},
    {"Inside Ganon's Castle", GROUP_DUNGEONS},
    {"Special Locations", TRAVEL_ROOT},
    {"Hyrule Castle", GROUP_SPECIAL},
    {"Ganon's Castle (outside)", GROUP_SPECIAL},
};
static const DestinationView destination_views[DESTINATION_COUNT] = {
    {GROUP_KOKIRI_FOREST, 0}, /* 2dff0b6a */
    {GROUP_KOKIRI_FOREST, 0}, /* e829bc4e */
    {GROUP_KOKIRI_FOREST, 0}, /* 2c15fea0 */
    {GROUP_KOKIRI_FOREST, 0}, /* 967a3397 */
    {GROUP_KOKIRI_FOREST, 0}, /* 2b4d360d */
    {GROUP_KOKIRI_FOREST, 0}, /* d9d83b29 */
    {GROUP_DEKU_TREE, 0}, /* c8f5b17b */
    {GROUP_DEKU_TREE, 0}, /* 9f00b32a */
    {GROUP_LOST_WOODS, 0}, /* de6033dd */
    {GROUP_LOST_WOODS, 0}, /* 8332cc1f */
    {GROUP_LOST_WOODS, 0}, /* 6feb8bda */
    {GROUP_LOST_WOODS, 0}, /* 2853681a */
    {GROUP_LOST_WOODS, 0}, /* 55a0fcf5 */
    {GROUP_LOST_WOODS, 0}, /* a56818f5 */
    {GROUP_LOST_WOODS, 0}, /* b4dcb614 */
    {GROUP_LOST_WOODS, 0}, /* 5ec04948 */
    {GROUP_SACRED_MEADOW, 0}, /* 0658abe2 */
    {GROUP_SACRED_MEADOW, 0}, /* 3b98dffa */
    {GROUP_SACRED_MEADOW, 0}, /* b05b9e63 */
    {GROUP_HYRULE_FIELD, 0}, /* 2c16603f */
    {GROUP_HYRULE_FIELD, 0}, /* 7c98d21d */
    {GROUP_HYRULE_FIELD, 0}, /* 6f7d5ff8 */
    {GROUP_HYRULE_FIELD, 0}, /* 1f80a3d0 */
    {GROUP_HYRULE_FIELD, 0}, /* 8b48c269 */
    {GROUP_LAKE_HYLIA, "From Gerudo river"}, /* ca674ff3 */
    {GROUP_LAKE_HYLIA, "From Zora's Domain"}, /* 78a45f34 */
    {GROUP_HYRULE_FIELD, "From Lake Hylia"}, /* ef4fb363 */
    {GROUP_LAKE_HYLIA, "Fishing pond"}, /* 8f610d13 */
    {GROUP_LAKE_HYLIA, "Serenade warp"}, /* 95353765 */
    {GROUP_LON_LON_RANCH, "Cucco game start"}, /* 9eafcf9d */
    {GROUP_HYRULE_FIELD, 0}, /* dd2597a2 */
    {GROUP_LAKE_HYLIA, "Dream return spot"}, /* 8fe6b8a1 */
    {GROUP_MARKET, 0}, /* 8bf2e2bb */
    {GROUP_MARKET, 0}, /* b6e80e74 */
    {GROUP_TEMPLE_OF_TIME, "Prelude of Light warp"}, /* c53ba43d */
    {GROUP_MARKET, 0}, /* d55aa666 */
    {GROUP_MARKET, "Potion shop"}, /* 7767ab3d: native bottle-tutorial spawn is inside the shop. */
    {GROUP_MARKET, 0}, /* a0697240 */
    {GROUP_MARKET, 0}, /* cde9dac0 */
    {GROUP_MARKET, 0}, /* 6253315b */
    {GROUP_HYRULE_CASTLE, 0}, /* 16725fd8 */
    {GROUP_KAKARIKO, 0}, /* 5d31c87d */
    {GROUP_KAKARIKO, 0}, /* e7375de3 */
    {GROUP_KAKARIKO, 0}, /* 9b0c9643 */
    {GROUP_KAKARIKO, 0}, /* 31ab4fd8 */
    {GROUP_KAKARIKO, 0}, /* da93dd3e */
    {GROUP_KAKARIKO, 0}, /* c3193e25 */
    {GROUP_GRAVEYARD, 0}, /* 9237e0cf */
    {GROUP_GRAVEYARD, 0}, /* 679582f5 */
    {GROUP_GRAVEYARD, 0}, /* 1fb87815 */
    {GROUP_GRAVEYARD, 0}, /* d3e3f9ec */
    {GROUP_WELL, 0}, /* 265327c7 */
    {GROUP_WELL, 0}, /* e489a9b0 */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 2544c6bf */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 92d3152f */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 4f7d849c */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 369d73c7 */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* df3295e9 */
    {GROUP_DODONGO, 0}, /* f162396a */
    {GROUP_DODONGO, 0}, /* 3c42d128 */
    {GROUP_GORON_CITY, 0}, /* 7a9c2f2f */
    {GROUP_GORON_CITY, 0}, /* 4a8370d7 */
    {GROUP_GORON_CITY, 0}, /* 48482537 */
    {GROUP_GORON_CITY, 0}, /* fdfd939b */
    {GROUP_MOUNTAIN_CRATER, 0}, /* d674c0fe */
    {GROUP_MOUNTAIN_CRATER, 0}, /* d0e1ed05 */
    {GROUP_MOUNTAIN_CRATER, 0}, /* ef50c787 */
    {GROUP_MOUNTAIN_CRATER, 0}, /* 237c1c6f */
    {GROUP_ZORA_RIVER, 0}, /* 72da9fdb */
    {GROUP_ZORA_RIVER, 0}, /* 219e26c5 */
    {GROUP_ZORA_RIVER, 0}, /* 7e04a6a5 */
    {GROUP_ZORA_RIVER, 0}, /* 1e578075 */
    {GROUP_ZORA_DOMAIN, 0}, /* 4f4b3ecf */
    {GROUP_ZORA_DOMAIN, 0}, /* 0cefb513 */
    {GROUP_ZORA_DOMAIN, 0}, /* 5bae1fa5 */
    {GROUP_ZORA_DOMAIN, 0}, /* 9eae2ba9 */
    {GROUP_ZORA_FOUNTAIN, 0}, /* 5b0a1a73 */
    {GROUP_ZORA_FOUNTAIN, 0}, /* 06d5fcf3 */
    {GROUP_ZORA_FOUNTAIN, 0}, /* 2d587ffb */
    {GROUP_JABU, 0}, /* 48816f22 */
    {GROUP_JABU, 0}, /* c16e6a1b */
    {GROUP_JABU, 0}, /* 333027ab */
    {GROUP_GERUDO_VALLEY, 0}, /* e13c125d */
    {GROUP_GERUDO_VALLEY, 0}, /* 887aed4f */
    {GROUP_GERUDO_VALLEY, 0}, /* 9b67412e */
    {GROUP_WASTELAND, 0}, /* d4400905 */
    {GROUP_WASTELAND, 0}, /* 4c5caeef */
    {GROUP_DESERT_COLOSSUS, 0}, /* 6ebefa0b */
    {GROUP_DESERT_COLOSSUS, 0}, /* 9ee41a48 */
    {GROUP_DESERT_COLOSSUS, 0}, /* 1357f449 */
    {GROUP_DESERT_COLOSSUS, 0}, /* 95e27ed3 */
    {GROUP_DESERT_COLOSSUS, 0}, /* 8d06fb93 */
    {GROUP_DESERT_COLOSSUS, 0}, /* 4e069b81 */
    {GROUP_SPIRIT_TEMPLE, 0}, /* 37958a46 */
    {GROUP_SPIRIT_TEMPLE, 0}, /* 300c9fbf */
    {GROUP_SPIRIT_TEMPLE, 0}, /* c526c7d4 */
    {GROUP_SPIRIT_TEMPLE, 0}, /* de5a60fa */
    {GROUP_KOKIRI_FOREST, 0}, /* 643f698f */
    {GROUP_KOKIRI_FOREST, 0}, /* 328970ef */
    {GROUP_KOKIRI_FOREST, 0}, /* e36bb30e */
    {GROUP_KOKIRI_FOREST, 0}, /* 49c78fa9 */
    {GROUP_DEKU_TREE, 0}, /* 1e6d5dc2 */
    {GROUP_DEKU_TREE, 0}, /* 1bfa9e4b */
    {GROUP_LOST_WOODS, 0}, /* 98e911fa */
    {GROUP_LOST_WOODS, 0}, /* 089aafee */
    {GROUP_LOST_WOODS, 0}, /* deec6647 */
    {GROUP_LOST_WOODS, 0}, /* 30f217a1 */
    {GROUP_LOST_WOODS, 0}, /* 826b0364 */
    {GROUP_LOST_WOODS, 0}, /* d4bb7ee4 */
    {GROUP_LOST_WOODS, 0}, /* a97affa5 */
    {GROUP_LOST_WOODS, 0}, /* 33174065 */
    {GROUP_SACRED_MEADOW, 0}, /* 38030399 */
    {GROUP_SACRED_MEADOW, 0}, /* f9323f5f */
    {GROUP_SACRED_MEADOW, 0}, /* 7539df62 */
    {GROUP_FOREST_TEMPLE, 0}, /* 685f32ec */
    {GROUP_FOREST_TEMPLE, 0}, /* 145ee6ac */
    {GROUP_FOREST_TEMPLE, 0}, /* 50051b53 */
    {GROUP_HYRULE_FIELD, 0}, /* 389f81f6 */
    {GROUP_HYRULE_FIELD, 0}, /* 1995aba2 */
    {GROUP_HYRULE_FIELD, 0}, /* 3461e37f */
    {GROUP_HYRULE_FIELD, 0}, /* a3d27115 */
    {GROUP_HYRULE_FIELD, 0}, /* 826e97e4 */
    {GROUP_LAKE_HYLIA, "From Gerudo river"}, /* fa4eb004 */
    {GROUP_LAKE_HYLIA, "Water Temple door"}, /* b023f3c0 */
    {GROUP_LAKE_HYLIA, "From Zora's Domain"}, /* 7ccb4a4f */
    {GROUP_HYRULE_FIELD, "From Lake Hylia"}, /* 627846be */
    {GROUP_LAKE_HYLIA, "Fishing pond"}, /* 01bf0d68 */
    {GROUP_LAKE_HYLIA, "Serenade warp"}, /* 0cc57c98 */
    {GROUP_LON_LON_RANCH, "Epona race start"}, /* 9151554f */
    {GROUP_LON_LON_RANCH, "House"}, /* ea560ac5 */
    {GROUP_LAKE_HYLIA, "Lakeside lab"}, /* a493bd96 */
    {GROUP_WATER_TEMPLE, 0}, /* 20305c21 */
    {GROUP_WATER_TEMPLE, 0}, /* 55086f18 */
    {GROUP_WATER_TEMPLE, 0}, /* b0cc217b */
    {GROUP_WATER_TEMPLE, 0}, /* 934f67ad */
    {GROUP_MARKET, 0}, /* 6974f0fc */
    {GROUP_MARKET, 0}, /* b61cb3ed */
    {GROUP_TEMPLE_OF_TIME, "Prelude of Light warp"}, /* 52a41e8c */
    {GROUP_GANON_EXTERIOR, 0}, /* 9be2ca47 */
    {GROUP_GANON_EXTERIOR, 0}, /* cc8bdff6 */
    {GROUP_GANON_EXTERIOR, "Castle door (enters castle)"}, /* 1f300d08 */
    {GROUP_GANON_INTERIOR, 0}, /* 3eab8b8f */
    {GROUP_GANON_INTERIOR, 0}, /* 58f1d107 */
    {GROUP_GANON_INTERIOR, 0}, /* 8f3bdf7d */
    {GROUP_KAKARIKO, 0}, /* 5a2a27b6 */
    {GROUP_KAKARIKO, 0}, /* b524fb34 */
    {GROUP_KAKARIKO, 0}, /* 68f41048 */
    {GROUP_KAKARIKO, 0}, /* c9da6f6d */
    {GROUP_KAKARIKO, 0}, /* b237a128 */
    {GROUP_KAKARIKO, 0}, /* 34c35060 */
    {GROUP_KAKARIKO, 0}, /* c00fac69 */
    {GROUP_KAKARIKO, 0}, /* 1b7650e6 */
    {GROUP_GRAVEYARD, 0}, /* 2b4c8550 */
    {GROUP_GRAVEYARD, 0}, /* 09846632 */
    {GROUP_GRAVEYARD, 0}, /* ee746320 */
    {GROUP_GRAVEYARD, 0}, /* e4568c4a */
    {GROUP_SHADOW_TEMPLE, 0}, /* 66a8be29 */
    {GROUP_SHADOW_TEMPLE, 0}, /* 1a87d691 */
    {GROUP_SHADOW_TEMPLE, 0}, /* 97dd47f1 */
    {GROUP_SHADOW_TEMPLE, 0}, /* b3b976d7 */
    {GROUP_SHADOW_TEMPLE, 0}, /* 2e0bc3c7 */
    {GROUP_WELL, 0}, /* 1b278ce4 */
    {GROUP_WELL, 0}, /* bcfcd091 */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 1804e066 */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 3b84c264 */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* ccf739c7 */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 7a316cf2 */
    {GROUP_MOUNTAIN_TRAIL, 0}, /* 70fe2d70 */
    {GROUP_DODONGO, 0}, /* 914a5971 */
    {GROUP_DODONGO, 0}, /* cdad7405 */
    {GROUP_GORON_CITY, 0}, /* 247d6694 */
    {GROUP_GORON_CITY, 0}, /* 157a1736 */
    {GROUP_GORON_CITY, 0}, /* 648e0086 */
    {GROUP_GORON_CITY, 0}, /* 82d7cdf4 */
    {GROUP_MOUNTAIN_CRATER, 0}, /* d73f072d */
    {GROUP_MOUNTAIN_CRATER, 0}, /* d2fec0b2 */
    {GROUP_MOUNTAIN_CRATER, 0}, /* 9218f5e2 */
    {GROUP_MOUNTAIN_CRATER, 0}, /* e65dbc4c */
    {GROUP_FIRE_TEMPLE, 0}, /* f49ec916 */
    {GROUP_FIRE_TEMPLE, 0}, /* 45ca04e3 */
    {GROUP_ZORA_RIVER, 0}, /* 410a4cec */
    {GROUP_ZORA_RIVER, 0}, /* 6dc197a2 */
    {GROUP_ZORA_RIVER, 0}, /* 31cd1806 */
    {GROUP_ZORA_RIVER, 0}, /* 2d6657fe */
    {GROUP_ZORA_DOMAIN, 0}, /* 3fb41b86 */
    {GROUP_ZORA_DOMAIN, 0}, /* 6fbe7378 */
    {GROUP_ZORA_DOMAIN, 0}, /* 23d90664 */
    {GROUP_ZORA_FOUNTAIN, 0}, /* ca963968 */
    {GROUP_ZORA_FOUNTAIN, 0}, /* 948ac7ee */
    {GROUP_ZORA_FOUNTAIN, 0}, /* 56ead0a4 */
    {GROUP_ICE_CAVERN, 0}, /* c3f2f95f */
    {GROUP_ICE_CAVERN, 0}, /* 7009d0bb */
    {GROUP_GERUDO_VALLEY, 0}, /* 5dfbbebe */
    {GROUP_GERUDO_VALLEY, 0}, /* 8a23ea3a */
    {GROUP_GERUDO_VALLEY, 0}, /* 834153be */
    {GROUP_GERUDO_VALLEY, 0}, /* 6196facb */
    {GROUP_GERUDO_FORTRESS, 0}, /* a8444241 */
    {GROUP_GERUDO_FORTRESS, 0}, /* 93a742d8 */
    {GROUP_GERUDO_FORTRESS, 0}, /* 73fcc09f */
    {GROUP_TRAINING_GROUND, 0}, /* bd060269 */
    {GROUP_TRAINING_GROUND, 0}, /* 25d8fece */
    {GROUP_WASTELAND, 0}, /* ee069924 */
    {GROUP_WASTELAND, 0}, /* e8c180ee */
    {GROUP_WASTELAND, "Fortress gate (exits wasteland)"}, /* 1c023884 */
    {GROUP_DESERT_COLOSSUS, 0}, /* feed4674 */
    {GROUP_DESERT_COLOSSUS, 0}, /* 7d10e995 */
    {GROUP_DESERT_COLOSSUS, 0}, /* 77843d42 */
    {GROUP_DESERT_COLOSSUS, 0}, /* c5286c6c */
    {GROUP_DESERT_COLOSSUS, 0}, /* b76942e4 */
    {GROUP_DESERT_COLOSSUS, 0}, /* 9c0b1ddc */
    {GROUP_SPIRIT_TEMPLE, 0}, /* 9233a3af */
    {GROUP_SPIRIT_TEMPLE, 0}, /* 01fe6904 */
    {GROUP_SPIRIT_TEMPLE, 0}, /* 8fe1373d */
    {GROUP_SPIRIT_TEMPLE, 0}, /* 64f2b563 */
};
static const uint8_t group_counts[2][TRAVEL_GROUP_COUNT] = {
    {0, 7, 5, 1, 7, 1, 6, 4, 6, 8, 3, 5, 4, 4, 4, 4, 3, 3, 0, 2, 6, 13, 2, 2, 3, 0, 0, 0, 0, 4, 2, 0, 0, 0, 1, 1, 0},
    {0, 6, 6, 2, 2, 1, 8, 4, 4, 8, 3, 5, 4, 4, 4, 3, 3, 4, 3, 3, 6, 31, 2, 2, 0, 3, 2, 4, 5, 4, 2, 2, 2, 3, 3, 0, 3},
};
static const uint8_t destination_order[DESTINATION_COUNT] = {
    162, 52, 161, 51, 67, 177, 64, 174, 65, 175, 176, 66, 167, 57, 164, 54, 55, 165, 56, 166, 163, 53, 90, 207, 91, 208, 87, 204, 89, 206, 205, 88, 92, 209, 59, 169, 168, 58, 179, 178, 116, 114, 115, 140, 138, 139, 199, 200, 84, 192, 193, 82, 195, 194, 83, 196, 198, 197, 173, 63, 171, 61, 62, 172, 170, 60, 49, 155, 152, 47, 153, 48, 50, 154, 203, 86, 85, 202, 201, 40, 22, 120, 119, 21, 121, 23, 125, 26, 118, 20, 19, 117, 30, 190, 191, 143, 141, 142, 102, 7, 101, 6, 81, 79, 80, 150, 145, 42, 144, 41, 44, 147, 146, 43, 151, 46, 149, 45, 148, 3, 100, 4, 5, 2, 98, 0, 97, 1, 99, 31, 126, 27, 24, 122, 25, 124, 130, 127, 28, 123, 29, 128, 129, 12, 107, 13, 108, 109, 14, 110, 15, 11, 106, 103, 8, 10, 105, 104, 9, 37, 39, 136, 33, 135, 32, 36, 35, 38, 16, 111, 17, 112, 113, 18, 158, 157, 159, 156, 160, 93, 210, 212, 95, 211, 94, 213, 96, 137, 34, 133, 131, 134, 132, 75, 186, 74, 73, 185, 184, 72, 78, 77, 189, 76, 187, 188, 182, 70, 69, 181, 180, 68, 71, 183
};
