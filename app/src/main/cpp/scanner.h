#ifndef SCANNER_H
#define SCANNER_H

#include "types.h"
#include "shizuku_memory.h"
#include <vector>
#include <string>

/**
 * Auto-detect game offsets using signature scanning
 * Patterns remain same across updates, offsets change
 */

struct ScanResult {
    std::string name;
    DWORD address;
    DWORD offset;
    bool found;
};

struct GameOffsets {
    // UE4 Core
    DWORD GWorld;
    DWORD GNames;
    DWORD GObjects;
    
    // Player
    DWORD PlayerController;
    DWORD LocalPlayer;
    DWORD PlayerCameraManager;
    
    // Entity
    DWORD RootComponent;
    DWORD PlayerState;
    DWORD Health;
    DWORD MaxHealth;
    DWORD TeamID;
    DWORD PlayerName;
    DWORD Position;
    DWORD Mesh;
    DWORD BoneArray;
    
    // Weapon
    DWORD CurrentWeapon;
    DWORD WeaponID;
    DWORD AmmoCount;
    
    // Vehicle
    DWORD VehicleHealth;
    DWORD VehicleFuel;
    
    bool isValid;
};

class Scanner {
private:
    ShizukuMemory* m_mem;
    std::vector<ScanResult> m_results;
    GameOffsets m_offsets;
    
    // Known patterns for UE4 games (PUBG Mobile style)
    struct Pattern {
        const char* name;
        std::vector<uint8_t> bytes;
        std::vector<bool> mask;  // true = match, false = wildcard
        int resultOffset;        // offset from pattern start to actual value
    };
    
    std::vector<Pattern> m_patterns;
    
    void initPatterns();
    DWORD scanPattern(const Pattern& pattern, DWORD start, DWORD end);
    DWORD scanInModule(const Pattern& pattern, const char* moduleName);
    
    // Pointer chain resolver
    DWORD resolvePointerChain(DWORD base, const std::vector<DWORD>& offsets);
    
    // Validation
    bool validateGWorld(DWORD addr);
    bool validateGNames(DWORD addr);
    bool validateEntity(DWORD addr);

public:
    Scanner(ShizukuMemory* mem);
    ~Scanner();
    
    // Auto scan all offsets
    bool autoScan();
    
    // Manual scan specific offset
    bool scanGWorld();
    bool scanGNames();
    bool scanPlayerOffsets();
    bool scanEntityOffsets();
    
    // Get results
    const GameOffsets& getOffsets() const { return m_offsets; }
    const std::vector<ScanResult>& getResults() const { return m_results; }
    
    // Dump offsets to log
    void dumpOffsets();
    
    // Export/Import offsets (for caching)
    std::string exportOffsets();
    bool importOffsets(const std::string& data);
};

#endif // SCANNER_H
