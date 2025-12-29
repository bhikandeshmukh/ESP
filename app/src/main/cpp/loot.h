#ifndef LOOT_H
#define LOOT_H

#include "types.h"
#include "features.h"
#include "shizuku_memory.h"
#include "overlay.h"
#include <vector>
#include <unordered_map>

class LootManager {
private:
    ShizukuMemory* m_mem;
    std::vector<LootItem> m_items;
    std::unordered_map<int, LootType> m_itemTypeMap;
    std::unordered_map<int, std::string> m_itemNameMap;
    
    void initItemDatabase();
    LootType getItemType(int itemId);
    std::string getItemName(int itemId);

public:
    LootManager(ShizukuMemory* mem);
    
    // Scan for loot items
    void scanLoot(DWORD levelAddr, Vector3 playerPos);
    
    // Filter items based on config
    void filterItems();
    
    // Get filtered items
    const std::vector<LootItem>& getItems() const { return m_items; }
    
    // Clear items
    void clear() { m_items.clear(); }
    
    // Get color for loot type
    static Color getLootColor(LootType type);
    
    // Get icon/symbol for loot type
    static const char* getLootSymbol(LootType type);
};

#endif // LOOT_H
