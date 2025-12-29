#include "loot.h"
#include <android/log.h>
#include <cmath>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Loot", __VA_ARGS__)

extern ESPConfig g_config;

LootManager::LootManager(ShizukuMemory* mem) : m_mem(mem) {
    initItemDatabase();
}

void LootManager::initItemDatabase() {
    // Weapons - AR
    m_itemTypeMap[101001] = LOOT_WEAPON_AR;  // AKM
    m_itemTypeMap[101002] = LOOT_WEAPON_AR;  // M416
    m_itemTypeMap[101003] = LOOT_WEAPON_AR;  // SCAR-L
    m_itemTypeMap[101004] = LOOT_WEAPON_AR;  // M16A4
    m_itemTypeMap[101005] = LOOT_WEAPON_AR;  // Groza
    m_itemTypeMap[101006] = LOOT_WEAPON_AR;  // AUG
    m_itemTypeMap[101007] = LOOT_WEAPON_AR;  // Beryl M762
    m_itemTypeMap[101008] = LOOT_WEAPON_AR;  // Mk47 Mutant
    m_itemTypeMap[101009] = LOOT_WEAPON_AR;  // G36C
    
    // Weapons - SMG
    m_itemTypeMap[102001] = LOOT_WEAPON_SMG;  // UMP45
    m_itemTypeMap[102002] = LOOT_WEAPON_SMG;  // Vector
    m_itemTypeMap[102003] = LOOT_WEAPON_SMG;  // UZI
    m_itemTypeMap[102004] = LOOT_WEAPON_SMG;  // Tommy Gun
    m_itemTypeMap[102005] = LOOT_WEAPON_SMG;  // PP-19
    m_itemTypeMap[102006] = LOOT_WEAPON_SMG;  // MP5K
    
    // Weapons - Sniper
    m_itemTypeMap[103001] = LOOT_WEAPON_SNIPER;  // Kar98k
    m_itemTypeMap[103002] = LOOT_WEAPON_SNIPER;  // M24
    m_itemTypeMap[103003] = LOOT_WEAPON_SNIPER;  // AWM
    m_itemTypeMap[103004] = LOOT_WEAPON_SNIPER;  // SKS
    m_itemTypeMap[103005] = LOOT_WEAPON_SNIPER;  // Mini14
    m_itemTypeMap[103006] = LOOT_WEAPON_SNIPER;  // SLR
    m_itemTypeMap[103007] = LOOT_WEAPON_SNIPER;  // VSS
    m_itemTypeMap[103008] = LOOT_WEAPON_SNIPER;  // Win94
    m_itemTypeMap[103009] = LOOT_WEAPON_SNIPER;  // MK14
    m_itemTypeMap[103010] = LOOT_WEAPON_SNIPER;  // QBU
    
    // Weapons - Shotgun
    m_itemTypeMap[104001] = LOOT_WEAPON_SHOTGUN;  // S686
    m_itemTypeMap[104002] = LOOT_WEAPON_SHOTGUN;  // S1897
    m_itemTypeMap[104003] = LOOT_WEAPON_SHOTGUN;  // S12K
    m_itemTypeMap[104004] = LOOT_WEAPON_SHOTGUN;  // DBS
    
    // Armor - Helmet
    m_itemTypeMap[201001] = LOOT_ARMOR_HELMET;  // Level 1
    m_itemTypeMap[201002] = LOOT_ARMOR_HELMET;  // Level 2
    m_itemTypeMap[201003] = LOOT_ARMOR_HELMET;  // Level 3
    
    // Armor - Vest
    m_itemTypeMap[202001] = LOOT_ARMOR_VEST;  // Level 1
    m_itemTypeMap[202002] = LOOT_ARMOR_VEST;  // Level 2
    m_itemTypeMap[202003] = LOOT_ARMOR_VEST;  // Level 3
    
    // Armor - Backpack
    m_itemTypeMap[203001] = LOOT_ARMOR_BAG;  // Level 1
    m_itemTypeMap[203002] = LOOT_ARMOR_BAG;  // Level 2
    m_itemTypeMap[203003] = LOOT_ARMOR_BAG;  // Level 3
    
    // Meds
    m_itemTypeMap[301001] = LOOT_MED_BANDAGE;
    m_itemTypeMap[301002] = LOOT_MED_FIRSTAID;
    m_itemTypeMap[301003] = LOOT_MED_MEDKIT;
    m_itemTypeMap[301004] = LOOT_MED_ENERGY;  // Energy drink
    m_itemTypeMap[301005] = LOOT_MED_ENERGY;  // Painkiller
    m_itemTypeMap[301006] = LOOT_MED_ENERGY;  // Adrenaline
    
    // Scopes
    m_itemTypeMap[401001] = LOOT_SCOPE;  // Red dot
    m_itemTypeMap[401002] = LOOT_SCOPE;  // Holo
    m_itemTypeMap[401003] = LOOT_SCOPE;  // 2x
    m_itemTypeMap[401004] = LOOT_SCOPE;  // 3x
    m_itemTypeMap[401005] = LOOT_SCOPE;  // 4x
    m_itemTypeMap[401006] = LOOT_SCOPE;  // 6x
    m_itemTypeMap[401007] = LOOT_SCOPE;  // 8x
    
    // Item names
    m_itemNameMap[101001] = "AKM";
    m_itemNameMap[101002] = "M416";
    m_itemNameMap[101003] = "SCAR-L";
    m_itemNameMap[101004] = "M16A4";
    m_itemNameMap[101005] = "Groza";
    m_itemNameMap[101006] = "AUG";
    m_itemNameMap[103001] = "Kar98k";
    m_itemNameMap[103002] = "M24";
    m_itemNameMap[103003] = "AWM";
    m_itemNameMap[103009] = "MK14";
    m_itemNameMap[201003] = "Helmet L3";
    m_itemNameMap[202003] = "Vest L3";
    m_itemNameMap[203003] = "Bag L3";
    m_itemNameMap[401005] = "4x Scope";
    m_itemNameMap[401006] = "6x Scope";
    m_itemNameMap[401007] = "8x Scope";
}

LootType LootManager::getItemType(int itemId) {
    auto it = m_itemTypeMap.find(itemId);
    if (it != m_itemTypeMap.end()) {
        return it->second;
    }
    return LOOT_UNKNOWN;
}

std::string LootManager::getItemName(int itemId) {
    auto it = m_itemNameMap.find(itemId);
    if (it != m_itemNameMap.end()) {
        return it->second;
    }
    return "Item";
}

void LootManager::scanLoot(DWORD levelAddr, Vector3 playerPos) {
    m_items.clear();
    
    if (levelAddr == 0) return;
    
    // Read entity list from level
    DWORD entityEntry = m_mem->read<DWORD>(levelAddr + 112);
    int entityCount = m_mem->read<int>(levelAddr + 116);
    
    if (entityCount <= 0 || entityCount > 2000) return;
    
    int maxDist = g_config.lootMaxDistance.load();
    
    for (int i = 0; i < entityCount; i++) {
        DWORD entityAddr = m_mem->read<DWORD>(entityEntry + i * 4);
        if (entityAddr == 0) continue;
        
        // Check if it's a pickup item
        DWORD entityStruct = m_mem->read<DWORD>(entityAddr + 16);
        if (entityStruct == 0) continue;
        
        // Get item ID
        int itemId = m_mem->read<int>(m_mem->read<DWORD>(m_mem->read<DWORD>(entityAddr + 0xC) + 0xA8) + 0x2F4);
        if (itemId <= 0) continue;
        
        LootType type = getItemType(itemId);
        if (type == LOOT_UNKNOWN) continue;
        
        // Get position
        DWORD rootComp = m_mem->read<DWORD>(entityAddr + 312);
        if (rootComp == 0) continue;
        
        Vector3 pos;
        pos.x = m_mem->read<float>(rootComp + 336);
        pos.y = m_mem->read<float>(rootComp + 340);
        pos.z = m_mem->read<float>(rootComp + 344);
        
        // Calculate distance
        float dx = pos.x - playerPos.x;
        float dy = pos.y - playerPos.y;
        float dz = pos.z - playerPos.z;
        float distance = sqrt(dx*dx + dy*dy + dz*dz) / 100.0f;  // Convert to meters
        
        if (distance > maxDist) continue;
        
        LootItem item;
        item.address = entityAddr;
        item.position = pos;
        item.type = type;
        item.itemId = itemId;
        item.name = getItemName(itemId);
        item.distance = distance;
        
        m_items.push_back(item);
    }
}

void LootManager::filterItems() {
    std::vector<LootItem> filtered;
    
    for (const auto& item : m_items) {
        bool keep = false;
        
        switch (item.type) {
            case LOOT_WEAPON_AR:
            case LOOT_WEAPON_SMG:
            case LOOT_WEAPON_SNIPER:
            case LOOT_WEAPON_SHOTGUN:
            case LOOT_WEAPON_PISTOL:
                keep = g_config.lootWeapons.load();
                break;
                
            case LOOT_ARMOR_HELMET:
            case LOOT_ARMOR_VEST:
            case LOOT_ARMOR_BAG:
                keep = g_config.lootArmor.load();
                break;
                
            case LOOT_MED_FIRSTAID:
            case LOOT_MED_MEDKIT:
            case LOOT_MED_BANDAGE:
            case LOOT_MED_ENERGY:
                keep = g_config.lootMeds.load();
                break;
                
            case LOOT_AMMO:
                keep = g_config.lootAmmo.load();
                break;
                
            case LOOT_SCOPE:
                keep = g_config.lootScopes.load();
                break;
                
            case LOOT_AIRDROP:
                keep = g_config.enableAirdrop.load();
                break;
                
            default:
                break;
        }
        
        if (keep) {
            filtered.push_back(item);
        }
    }
    
    m_items = filtered;
}

Color LootManager::getLootColor(LootType type) {
    switch (type) {
        case LOOT_WEAPON_AR:
        case LOOT_WEAPON_SMG:
        case LOOT_WEAPON_SNIPER:
        case LOOT_WEAPON_SHOTGUN:
            return Color(1.0f, 0.5f, 0.0f, 1.0f);  // Orange
            
        case LOOT_ARMOR_HELMET:
        case LOOT_ARMOR_VEST:
        case LOOT_ARMOR_BAG:
            return Color(0.0f, 0.5f, 1.0f, 1.0f);  // Blue
            
        case LOOT_MED_FIRSTAID:
        case LOOT_MED_MEDKIT:
        case LOOT_MED_BANDAGE:
        case LOOT_MED_ENERGY:
            return Color(0.0f, 1.0f, 0.0f, 1.0f);  // Green
            
        case LOOT_SCOPE:
            return Color(1.0f, 0.0f, 1.0f, 1.0f);  // Magenta
            
        case LOOT_AIRDROP:
            return Color(1.0f, 0.0f, 0.0f, 1.0f);  // Red
            
        default:
            return Color(1.0f, 1.0f, 1.0f, 1.0f);  // White
    }
}

const char* LootManager::getLootSymbol(LootType type) {
    switch (type) {
        case LOOT_WEAPON_AR: return "[AR]";
        case LOOT_WEAPON_SMG: return "[SMG]";
        case LOOT_WEAPON_SNIPER: return "[SR]";
        case LOOT_WEAPON_SHOTGUN: return "[SG]";
        case LOOT_ARMOR_HELMET: return "[H]";
        case LOOT_ARMOR_VEST: return "[V]";
        case LOOT_ARMOR_BAG: return "[B]";
        case LOOT_MED_FIRSTAID: return "[+]";
        case LOOT_MED_MEDKIT: return "[++]";
        case LOOT_SCOPE: return "[S]";
        case LOOT_AIRDROP: return "[AIR]";
        default: return "[?]";
    }
}
