#ifndef FEATURES_H
#define FEATURES_H

#include "types.h"
#include <atomic>

// Feature toggles
struct ESPConfig {
    // ESP Features
    std::atomic<bool> enableESP{true};
    std::atomic<bool> enableBox{true};
    std::atomic<bool> enableSkeleton{true};
    std::atomic<bool> enableHealth{true};
    std::atomic<bool> enableDistance{true};
    std::atomic<bool> enableName{true};
    std::atomic<bool> enableWeapon{true};
    std::atomic<bool> enableSnapline{true};
    
    // Loot ESP
    std::atomic<bool> enableLoot{false};
    std::atomic<bool> lootWeapons{true};
    std::atomic<bool> lootArmor{true};
    std::atomic<bool> lootMeds{true};
    std::atomic<bool> lootAmmo{false};
    std::atomic<bool> lootScopes{true};
    std::atomic<int> lootMaxDistance{100};  // meters
    
    // Vehicle ESP
    std::atomic<bool> enableVehicle{true};
    
    // Airdrop
    std::atomic<bool> enableAirdrop{true};
    
    // Visual Settings
    std::atomic<int> maxDistance{500};  // meters
    std::atomic<float> boxThickness{2.0f};
    std::atomic<float> skeletonThickness{1.5f};
    
    // Colors (RGBA as int: 0xRRGGBBAA)
    std::atomic<uint32_t> enemyColor{0xFF0000FF};    // Red
    std::atomic<uint32_t> teamColor{0x00FF00FF};     // Green
    std::atomic<uint32_t> botColor{0xFFFF00FF};      // Yellow
    std::atomic<uint32_t> lootColor{0x00FFFFFF};     // Cyan
    std::atomic<uint32_t> vehicleColor{0xFF00FFFF}; // Magenta
    
    // Anti-Detection
    std::atomic<bool> enableAntiDetect{true};
    std::atomic<int> readDelayMin{1};   // ms
    std::atomic<int> readDelayMax{5};   // ms
    std::atomic<bool> randomizeReads{true};
};

// Bone IDs for skeleton
enum BoneID {
    BONE_HEAD = 6,
    BONE_NECK = 5,
    BONE_CHEST = 4,
    BONE_STOMACH = 3,
    BONE_PELVIS = 2,
    
    BONE_LEFT_SHOULDER = 11,
    BONE_LEFT_ELBOW = 12,
    BONE_LEFT_HAND = 13,
    
    BONE_RIGHT_SHOULDER = 32,
    BONE_RIGHT_ELBOW = 33,
    BONE_RIGHT_HAND = 34,
    
    BONE_LEFT_HIP = 51,
    BONE_LEFT_KNEE = 52,
    BONE_LEFT_FOOT = 53,
    
    BONE_RIGHT_HIP = 56,
    BONE_RIGHT_KNEE = 57,
    BONE_RIGHT_FOOT = 58
};

// Loot item types
enum LootType {
    LOOT_UNKNOWN = 0,
    LOOT_WEAPON_AR,
    LOOT_WEAPON_SMG,
    LOOT_WEAPON_SNIPER,
    LOOT_WEAPON_SHOTGUN,
    LOOT_WEAPON_PISTOL,
    LOOT_ARMOR_HELMET,
    LOOT_ARMOR_VEST,
    LOOT_ARMOR_BAG,
    LOOT_MED_FIRSTAID,
    LOOT_MED_MEDKIT,
    LOOT_MED_BANDAGE,
    LOOT_MED_ENERGY,
    LOOT_AMMO,
    LOOT_SCOPE,
    LOOT_ATTACHMENT,
    LOOT_GRENADE,
    LOOT_AIRDROP
};

struct LootItem {
    DWORD address;
    Vector3 position;
    LootType type;
    int itemId;
    std::string name;
    float distance;
};

// Global config
extern ESPConfig g_config;

#endif // FEATURES_H
