#include "inventory.h"

const InventoryItem inventory_catalog[INVENTORY_COUNT] = {
    {"Kokiri Sword", "Weapon_Sword_Kokiri", INV_WEAPONS, INV_CHILD, false},
    {"Fairy Bow", "Weapon_Range_Bow", INV_WEAPONS, INV_ADULT, false},
    {"Fairy Slingshot", "Weapon_Range_SlingShot", INV_WEAPONS, INV_CHILD, false},
    {"Boomerang", "Item_Boomerang", INV_WEAPONS, INV_CHILD, false},
    {"Hookshot", "Item_Hookshot", INV_WEAPONS, INV_ADULT, false},
    {"Longshot", "Item_Longshot", INV_WEAPONS, INV_ADULT, false},
    {"Megaton Hammer", "Item_Hammer", INV_WEAPONS, INV_ADULT, false},
    {"Deku Shield", "Weapon_Shield_Deku", INV_SHIELDS, INV_CHILD, false},
    {"Hylian Shield", "Weapon_Shield_Hylian", INV_SHIELDS, INV_BOTH_AGES, false},
    {"Mirror Shield", "Weapon_Shield_Mirror", INV_SHIELDS, INV_ADULT, false},
    {"Goron Tunic", "Armor_Adult_Goron", INV_EQUIPMENT, INV_ADULT, false},
    {"Zora Tunic", "Armor_Adult_Zora", INV_EQUIPMENT, INV_ADULT, false},
    {"Iron Boots", "Armor_Adult_HeavyBoot", INV_EQUIPMENT, INV_ADULT, false},
    {"Hover Boots", "Armor_Adult_HoverBoot", INV_EQUIPMENT, INV_ADULT, false},
    {"Deku Sticks", "Item_DekuStick", INV_CONSUMABLES, INV_CHILD, true},
    {"Deku Nuts", "Item_DekuNut", INV_CONSUMABLES, INV_BOTH_AGES, true},
    {"Arrows", "Item_Arrow_Normal", INV_AMMUNITION, INV_ADULT, true},
    {"Bombs", "Item_Bomb", INV_AMMUNITION, INV_BOTH_AGES, true},
    {"Deku Seeds", "Item_DekuSeed", INV_AMMUNITION, INV_CHILD, true},
    {"Bombchus", "Item_Bombchu", INV_AMMUNITION, INV_BOTH_AGES, true},
    {"Magic Beans", "Item_MagicBean", INV_CONSUMABLES, INV_CHILD, true}
};
