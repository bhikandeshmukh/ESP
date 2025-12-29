#include "scanner.h"
#include <android/log.h>
#include <sstream>
#include <iomanip>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Scanner", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Scanner", __VA_ARGS__)

Scanner::Scanner(ShizukuMemory* mem) : m_mem(mem) {
    memset(&m_offsets, 0, sizeof(GameOffsets));
    m_offsets.isValid = false;
    initPatterns();
}

Scanner::~Scanner() {}

void Scanner::initPatterns() {
    // UE4 GWorld pattern - common across versions
    // This pattern looks for GWorld pointer structure
    m_patterns.push_back({
        "GWorld",
        {0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
         0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01},
        {true, true, true, true, true, true, true, true,
         true, true, true, true, true, true, true, true, true},
        -32  // GWorld is 32 bytes before pattern
    });
    
    // GNames pattern
    m_patterns.push_back({
        "GNames",
        {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00},
        {false, false, false, false, true, true, true, true},
        0
    });
    
    // PlayerController pattern
    m_patterns.push_back({
        "PlayerController",
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x80, 0x3F},  // 1.0f in float
        {false, false, false, false, false, false, false, false,
         true, true, true, true},
        0
    });
}

DWORD Scanner::scanPattern(const Pattern& pattern, DWORD start, DWORD end) {
    const size_t CHUNK_SIZE = 0x10000;  // 64KB chunks
    const size_t patternSize = pattern.bytes.size();
    
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    
    for (DWORD addr = start; addr < end; addr += CHUNK_SIZE - patternSize) {
        if (!m_mem->readBytes(addr, buffer.data(), CHUNK_SIZE)) {
            continue;
        }
        
        for (size_t i = 0; i < CHUNK_SIZE - patternSize; i++) {
            bool found = true;
            
            for (size_t j = 0; j < patternSize && found; j++) {
                if (pattern.mask[j] && buffer[i + j] != pattern.bytes[j]) {
                    found = false;
                }
            }
            
            if (found) {
                DWORD result = addr + i + pattern.resultOffset;
                LOGI("Pattern '%s' found at 0x%lX (result: 0x%lX)", 
                     pattern.name, (unsigned long)(addr + i), (unsigned long)result);
                return result;
            }
        }
    }
    
    return 0;
}

DWORD Scanner::scanInModule(const Pattern& pattern, const char* moduleName) {
    DWORD base = m_mem->getModuleBase(moduleName);
    if (base == 0) {
        LOGE("Module '%s' not found", moduleName);
        return 0;
    }
    
    // Scan module range (assume 64MB max)
    return scanPattern(pattern, base, base + 0x4000000);
}

DWORD Scanner::resolvePointerChain(DWORD base, const std::vector<DWORD>& offsets) {
    DWORD addr = base;
    
    for (size_t i = 0; i < offsets.size(); i++) {
        addr = m_mem->read<DWORD>(addr);
        if (addr == 0) return 0;
        
        if (i < offsets.size() - 1) {
            addr += offsets[i];
        }
    }
    
    return addr + offsets.back();
}

bool Scanner::validateGWorld(DWORD addr) {
    if (addr == 0) return false;
    
    // GWorld should point to valid UWorld
    DWORD uworld = m_mem->read<DWORD>(addr);
    if (uworld < 0x10000000 || uworld > 0x80000000) return false;
    
    // UWorld->PersistentLevel should be valid
    DWORD level = m_mem->read<DWORD>(uworld + 32);
    if (level < 0x10000000 || level > 0x80000000) return false;
    
    return true;
}

bool Scanner::validateGNames(DWORD addr) {
    if (addr == 0) return false;
    
    // GNames should have valid structure
    DWORD names = m_mem->read<DWORD>(addr);
    if (names < 0x10000000) return false;
    
    // First entry should be "None"
    DWORD firstEntry = m_mem->read<DWORD>(names);
    if (firstEntry == 0) return false;
    
    char buffer[8] = {0};
    m_mem->readBytes(firstEntry + 8, buffer, 4);
    
    return (strcmp(buffer, "None") == 0);
}

bool Scanner::validateEntity(DWORD addr) {
    if (addr == 0 || addr < 0x10000000) return false;
    
    // Entity should have valid VTable
    DWORD vtable = m_mem->read<DWORD>(addr);
    return (vtable > 0x10000000 && vtable < 0x80000000);
}

bool Scanner::scanGWorld() {
    LOGI("Scanning for GWorld...");
    
    // Method 1: Pattern scan
    for (const auto& pattern : m_patterns) {
        if (strcmp(pattern.name, "GWorld") == 0) {
            DWORD result = scanPattern(pattern, 0x20000000, 0x40000000);
            if (result && validateGWorld(result)) {
                m_offsets.GWorld = result;
                m_results.push_back({"GWorld", result, 0, true});
                LOGI("GWorld found: 0x%X", result);
                return true;
            }
        }
    }
    
    // Method 2: Search in libUE4.so
    DWORD base = m_mem->getModuleBase("libUE4.so");
    if (base == 0) {
        base = m_mem->getModuleBase("libtersafe.so");
    }
    
    if (base > 0) {
        // Common GWorld offset patterns
        std::vector<DWORD> commonOffsets = {
            0x8B8B8B8, 0x8C8C8C8, 0x8D8D8D8,  // Example offsets
            0x9000000, 0x9100000, 0x9200000
        };
        
        for (DWORD off : commonOffsets) {
            DWORD addr = base + off;
            if (validateGWorld(addr)) {
                m_offsets.GWorld = addr;
                m_results.push_back({"GWorld", addr, off, true});
                LOGI("GWorld found at base+0x%X: 0x%X", off, addr);
                return true;
            }
        }
    }
    
    m_results.push_back({"GWorld", 0, 0, false});
    LOGE("GWorld not found");
    return false;
}

bool Scanner::scanGNames() {
    LOGI("Scanning for GNames...");
    
    if (m_offsets.GWorld == 0) {
        LOGE("Need GWorld first");
        return false;
    }
    
    // GNames is usually at fixed offset from GWorld
    std::vector<int> gnamesOffsets = {
        -1638204,   // From ESP.cpp
        -0x190000,
        -0x180000,
        -0x1A0000
    };
    
    for (int off : gnamesOffsets) {
        DWORD addr = m_offsets.GWorld + off;
        if (validateGNames(addr)) {
            m_offsets.GNames = addr;
            m_results.push_back({"GNames", addr, (DWORD)off, true});
            LOGI("GNames found: 0x%X (offset: %d)", addr, off);
            return true;
        }
    }
    
    m_results.push_back({"GNames", 0, 0, false});
    LOGE("GNames not found");
    return false;
}

bool Scanner::scanPlayerOffsets() {
    LOGI("Scanning player offsets...");
    
    if (m_offsets.GWorld == 0) return false;
    
    DWORD uworld = m_mem->read<DWORD>(m_offsets.GWorld);
    if (uworld == 0) return false;
    
    // Scan for GameInstance offset
    for (DWORD off = 32; off < 256; off += 4) {
        DWORD gameInstance = m_mem->read<DWORD>(uworld + off);
        if (gameInstance > 0x10000000 && gameInstance < 0x80000000) {
            // Check if this looks like GameInstance
            DWORD localPlayers = m_mem->read<DWORD>(gameInstance + 56);
            if (localPlayers > 0x10000000) {
                LOGI("GameInstance offset: %d", off);
                
                // Scan for PlayerController offset
                for (DWORD pcOff = 64; pcOff < 256; pcOff += 4) {
                    DWORD pc = m_mem->read<DWORD>(gameInstance + pcOff);
                    if (validateEntity(pc)) {
                        m_offsets.PlayerController = pcOff;
                        m_results.push_back({"PlayerController", pc, pcOff, true});
                        LOGI("PlayerController offset: %d", pcOff);
                        break;
                    }
                }
                break;
            }
        }
    }
    
    return m_offsets.PlayerController != 0;
}

bool Scanner::scanEntityOffsets() {
    LOGI("Scanning entity offsets...");
    
    // Common UE4 entity offsets to try
    struct OffsetTest {
        const char* name;
        DWORD* target;
        std::vector<DWORD> candidates;
    };
    
    std::vector<OffsetTest> tests = {
        {"Health", &m_offsets.Health, {1912, 0x778, 0x780, 0x788, 0x790}},
        {"MaxHealth", &m_offsets.MaxHealth, {1916, 0x77C, 0x784, 0x78C, 0x794}},
        {"TeamID", &m_offsets.TeamID, {1552, 0x610, 0x618, 0x620, 0x628}},
        {"PlayerName", &m_offsets.PlayerName, {1512, 0x5E8, 0x5F0, 0x5F8, 0x600}},
        {"Position", &m_offsets.Position, {336, 0x150, 0x158, 0x160, 0x168}},
        {"RootComponent", &m_offsets.RootComponent, {312, 0x138, 0x140, 0x148, 0x150}},
        {"Mesh", &m_offsets.Mesh, {776, 0x308, 0x310, 0x318, 0x320}},
    };
    
    // Use default offsets from ESP.cpp initially
    m_offsets.Health = 1912;
    m_offsets.MaxHealth = 1916;
    m_offsets.TeamID = 1552;
    m_offsets.PlayerName = 1512;
    m_offsets.Position = 336;
    m_offsets.RootComponent = 312;
    m_offsets.Mesh = 776;
    
    for (auto& test : tests) {
        m_results.push_back({test.name, 0, test.candidates[0], true});
        *test.target = test.candidates[0];
    }
    
    return true;
}

bool Scanner::autoScan() {
    LOGI("=== Starting Auto Scan ===");
    m_results.clear();
    
    bool success = true;
    
    success &= scanGWorld();
    success &= scanGNames();
    success &= scanPlayerOffsets();
    success &= scanEntityOffsets();
    
    m_offsets.isValid = success;
    
    LOGI("=== Auto Scan Complete: %s ===", success ? "SUCCESS" : "PARTIAL");
    dumpOffsets();
    
    return success;
}

void Scanner::dumpOffsets() {
    LOGI("=== Offset Dump ===");
    LOGI("GWorld: 0x%X", m_offsets.GWorld);
    LOGI("GNames: 0x%X", m_offsets.GNames);
    LOGI("PlayerController: 0x%X", m_offsets.PlayerController);
    LOGI("Health: %d (0x%X)", m_offsets.Health, m_offsets.Health);
    LOGI("TeamID: %d (0x%X)", m_offsets.TeamID, m_offsets.TeamID);
    LOGI("Position: %d (0x%X)", m_offsets.Position, m_offsets.Position);
    LOGI("PlayerName: %d (0x%X)", m_offsets.PlayerName, m_offsets.PlayerName);
    LOGI("==================");
}

std::string Scanner::exportOffsets() {
    std::stringstream ss;
    ss << "GWorld=" << std::hex << m_offsets.GWorld << "\n";
    ss << "GNames=" << std::hex << m_offsets.GNames << "\n";
    ss << "Health=" << std::dec << m_offsets.Health << "\n";
    ss << "MaxHealth=" << m_offsets.MaxHealth << "\n";
    ss << "TeamID=" << m_offsets.TeamID << "\n";
    ss << "PlayerName=" << m_offsets.PlayerName << "\n";
    ss << "Position=" << m_offsets.Position << "\n";
    ss << "RootComponent=" << m_offsets.RootComponent << "\n";
    ss << "Mesh=" << m_offsets.Mesh << "\n";
    return ss.str();
}

bool Scanner::importOffsets(const std::string& data) {
    // Parse key=value format
    std::istringstream iss(data);
    std::string line;
    
    while (std::getline(iss, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        
        DWORD value = 0;
        if (val.find("0x") == 0 || key == "GWorld" || key == "GNames") {
            value = std::stoul(val, nullptr, 16);
        } else {
            value = std::stoul(val);
        }
        
        if (key == "GWorld") m_offsets.GWorld = value;
        else if (key == "GNames") m_offsets.GNames = value;
        else if (key == "Health") m_offsets.Health = value;
        else if (key == "MaxHealth") m_offsets.MaxHealth = value;
        else if (key == "TeamID") m_offsets.TeamID = value;
        else if (key == "PlayerName") m_offsets.PlayerName = value;
        else if (key == "Position") m_offsets.Position = value;
        else if (key == "RootComponent") m_offsets.RootComponent = value;
        else if (key == "Mesh") m_offsets.Mesh = value;
    }
    
    m_offsets.isValid = (m_offsets.GWorld != 0);
    return m_offsets.isValid;
}

// Global scanner instance
Scanner* g_scanner = nullptr;
