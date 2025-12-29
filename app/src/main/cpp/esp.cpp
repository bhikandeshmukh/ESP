#include "esp.h"
#include "scanner.h"
#include <android/log.h>
#include <cmath>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ESP", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ESP", __VA_ARGS__)

// Global scanner
extern Scanner* g_scanner;

ESP::ESP(ShizukuMemory* mem) : m_mem(mem), m_viewWorld(0), m_uWorld(0), m_gNames(0),
    m_viewMatrixBase(0), m_entityCount(0), m_enemyCount(0), m_myTeamId(0),
    m_myObject(0), m_screenWidth(1920), m_screenHeight(1080) {
    memset(&m_viewMatrix, 0, sizeof(D3DMatrix));
    for (int i = 0; i < MAX_ENTITY; i++) {
        m_entities[i] = EntityData();
    }
}

ESP::~ESP() {}

void ESP::setScreenSize(int w, int h) {
    m_screenWidth = w;
    m_screenHeight = h;
}

DWORD ESP::getViewWorld() {
    uint8_t pattern[] = { 0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
                          0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01 };
    
    DWORD result = m_mem->scanPattern(pattern, sizeof(pattern), 0x26000000, 0x30000000);
    
    if (result > 0) {
        DWORD cand = result - 32;
        DWORD rr = get<DWORD>(cand) + 32;
        DWORD tmp = get<DWORD>(rr) + 512;
        
        float t1 = get<float>(tmp + 56);
        float t2 = get<float>(tmp + 40);
        float t3 = get<float>(tmp + 24);
        float t4 = get<float>(tmp + 8);
        
        if (t1 >= 3 && t2 == 0 && t3 == 0 && t4 == 0) {
            LOGI("Found ViewWorld: 0x%X", cand);
            return cand;
        }
    }
    return 0;
}

bool ESP::init() {
    LOGI("ESP::init() - Starting auto-scan...");
    
    // Try auto-scan first
    if (!g_scanner) {
        g_scanner = new Scanner(m_mem);
    }
    
    if (g_scanner->autoScan()) {
        const GameOffsets& offsets = g_scanner->getOffsets();
        m_viewWorld = offsets.GWorld;
        m_gNames = offsets.GNames;
        
        // Update offsets from scanner
        m_offsets.healthOffset = offsets.Health;
        m_offsets.teamIDOffset = offsets.TeamID;
        m_offsets.nameOffset = offsets.PlayerName;
        m_offsets.posOffset = offsets.Position;
        
        LOGI("Auto-scan successful!");
    } else {
        LOGI("Auto-scan partial, trying manual...");
    }
    
    // Fallback to manual scan if auto-scan failed
    if (m_viewWorld == 0) {
        m_viewWorld = getViewWorld();
    }
    
    if (m_viewWorld == 0) {
        LOGE("Failed to find ViewWorld");
        return false;
    }
    
    if (m_uWorld == 0) {
        m_uWorld = m_viewWorld - 4217216;
    }
    if (m_gNames == 0) {
        m_gNames = m_viewWorld - 1638204;
    }
    
    LOGI("ESP Initialized - ViewWorld: 0x%X, UWorld: 0x%X, GNames: 0x%X",
         m_viewWorld, m_uWorld, m_gNames);
    
    return true;
}

std::string ESP::getEntityClassName(DWORD gNames, DWORD id) {
    if (id <= 0 || id >= 2000000) return "";
    
    DWORD gname = get<DWORD>(gNames);
    DWORD page = id / 16384;
    DWORD index = id % 16384;
    
    DWORD secPartAddr = get<DWORD>(gname + page * 4);
    if (secPartAddr <= 0) return "";
    
    DWORD nameAddr = get<DWORD>(secPartAddr + index * 4);
    if (nameAddr <= 0) return "";
    
    char buffer[64] = {0};
    m_mem->readBytes(nameAddr + 8, buffer, 63);
    
    return std::string(buffer);
}

std::string ESP::getPlayerName(DWORD entityAddr) {
    uint8_t nameBuffer[34] = {0};
    DWORD namePtr = get<DWORD>(entityAddr + m_offsets.nameOffset);
    
    if (!m_mem->readBytes(namePtr, nameBuffer, 32)) return "";
    
    std::string name;
    for (int i = 0; i < 32; i++) {
        if (nameBuffer[i] >= 33 && nameBuffer[i] <= 126) {
            name += (char)nameBuffer[i];
        }
        if (i > 0 && i % 2 == 1 && nameBuffer[i + 1] == 0) break;
    }
    return name;
}

void ESP::getViewMatrix() {
    DWORD viewMatrixAddr = get<DWORD>(get<DWORD>(m_viewMatrixBase) + 32) + 512;
    m_viewMatrix = get<D3DMatrix>(viewMatrixAddr);
}

void ESP::scanEntityList() {
    DWORD uWorlds = get<DWORD>(m_uWorld);
    DWORD uLevel = get<DWORD>(uWorlds + 32);
    DWORD gameInstance = get<DWORD>(uWorlds + 36);
    DWORD playerController = get<DWORD>(gameInstance + m_offsets.controllerOffset);
    DWORD playerCarry = get<DWORD>(playerController + 32);
    m_myObject = get<DWORD>(playerCarry + 788);
    m_viewMatrixBase = m_viewWorld;
    
    DWORD entityEntry = get<DWORD>(uLevel + 112);
    m_entityCount = get<DWORD>(uLevel + 116);
    
    if (m_entityCount < 0) m_entityCount = 0;
    if (m_entityCount > MAX_ENTITY) m_entityCount = MAX_ENTITY;
    
    m_enemyCount = 0;
    
    for (int i = 0; i < m_entityCount; i++) {
        DWORD entityAddr = get<DWORD>(entityEntry + i * 4);
        DWORD entityStruct = get<DWORD>(entityAddr + 16);
        
        if (m_entities[i].address != entityAddr) {
            std::string className = getEntityClassName(m_gNames, entityStruct);
            m_entities[i].className = className;
            m_entities[i].itemId = get<int>(get<DWORD>(get<DWORD>(entityAddr + 0xC) + 0xA8) + 0x2F4);
            
            // Detect entity type
            m_entities[i].isPlayer = (className.find("Player") != std::string::npos);
            m_entities[i].isVehicle = (className.find("Vehicle") != std::string::npos);
            m_entities[i].isBox = (className.find("Box") != std::string::npos || 
                                   className.find("Crate") != std::string::npos);
            m_entities[i].isLoot = (className.find("Pickup") != std::string::npos);
        }
        
        // Player specific
        if (m_entities[i].isPlayer) {
            if (m_entities[i].address != entityAddr) {
                m_entities[i].playerWorld = get<DWORD>(entityAddr + 312);
                m_entities[i].name = getPlayerName(entityAddr);
            }
            
            m_entities[i].status = get<int>(m_entities[i].playerWorld + m_offsets.statusOffset);
            
            if (m_entities[i].status != 6) { // Not dead
                m_entities[i].teamId = get<int>(entityAddr + m_offsets.teamIDOffset);
                
                if (entityAddr == m_myObject) {
                    m_myTeamId = m_entities[i].teamId;
                }
                
                if (m_myTeamId != m_entities[i].teamId) {
                    m_enemyCount++;
                }
                
                m_entities[i].isBot = (get<int>(entityAddr + 692) == 0);
                m_entities[i].pose = get<int>(m_entities[i].playerWorld + m_offsets.poseOffset);
                
                // Bone addresses
                DWORD tmpAddr = get<DWORD>(entityAddr + 776);
                m_entities[i].bodyAddr = tmpAddr + 320;
                m_entities[i].boneAddr = get<DWORD>(tmpAddr + 1408) + 48;
                
                // Weapons
                DWORD weaponsPtr = get<DWORD>(entityAddr + 300);
                DWORD wptr1 = get<DWORD>(weaponsPtr);
                DWORD wptr2 = get<DWORD>(weaponsPtr + 4);
                m_entities[i].weapon1Id = get<int>(wptr1 + 728);
                m_entities[i].weapon2Id = get<int>(wptr2 + 728);
            }
        }
        
        m_entities[i].address = entityAddr;
    }
}

void ESP::scanEntityPositions() {
    for (int i = 0; i < m_entityCount; i++) {
        DWORD worldAddr = get<DWORD>(m_entities[i].address + 312);
        
        m_entities[i].position.x = get<float>(worldAddr + m_offsets.posOffset);
        m_entities[i].position.y = get<float>(worldAddr + m_offsets.posOffset + 4);
        m_entities[i].position.z = get<float>(worldAddr + m_offsets.posOffset + 8);
        
        if (m_entities[i].isVehicle) {
            DWORD vehicleDynamics = get<DWORD>(get<DWORD>(m_entities[i].address + 0x54) + 0x4D8);
            m_entities[i].maxHealth = get<float>(vehicleDynamics + 0x108);
            m_entities[i].health = get<float>(vehicleDynamics + 0x10C);
        }
        
        if (m_entities[i].isPlayer) {
            m_entities[i].maxHealth = get<float>(m_entities[i].address + m_offsets.healthOffset);
            m_entities[i].health = get<float>(m_entities[i].address + m_offsets.healthOffset + 4);
        }
    }
}

bool ESP::worldToScreen(Vector3 pos, Vector2& screen, int* distance) {
    float screenW = (m_viewMatrix._14 * pos.x) + (m_viewMatrix._24 * pos.y) + 
                    (m_viewMatrix._34 * pos.z + m_viewMatrix._44);
    
    *distance = (int)(screenW / 100);
    if (screenW < 0.0001f) return false;
    
    screenW = 1.0f / screenW;
    float sightX = m_screenWidth / 2.0f;
    float sightY = m_screenHeight / 2.0f;
    
    screen.x = sightX + (m_viewMatrix._11 * pos.x + m_viewMatrix._21 * pos.y + 
               m_viewMatrix._31 * pos.z + m_viewMatrix._41) * screenW * sightX;
    screen.y = sightY - (m_viewMatrix._12 * pos.x + m_viewMatrix._22 * pos.y + 
               m_viewMatrix._32 * pos.z + m_viewMatrix._42) * screenW * sightY;
    
    return true;
}

bool ESP::worldToScreen(Vector3 pos, Vector3& screen, int* distance) {
    float screenW = (m_viewMatrix._14 * pos.x) + (m_viewMatrix._24 * pos.y) + 
                    (m_viewMatrix._34 * pos.z + m_viewMatrix._44);
    
    screen.z = screenW;
    *distance = (int)(screenW / 100);
    if (screenW < 0.0001f) return false;
    
    screenW = 1.0f / screenW;
    float sightX = m_screenWidth / 2.0f;
    float sightY = m_screenHeight / 2.0f;
    
    screen.x = sightX + (m_viewMatrix._11 * pos.x + m_viewMatrix._21 * pos.y + 
               m_viewMatrix._31 * pos.z + m_viewMatrix._41) * screenW * sightX;
    screen.y = sightY - (m_viewMatrix._12 * pos.x + m_viewMatrix._22 * pos.y + 
               m_viewMatrix._32 * pos.z + m_viewMatrix._42) * screenW * sightY;
    
    return true;
}

bool ESP::worldToScreenPlayer(Vector3 pos, Vector3& screen, int* distance) {
    float screenW = (m_viewMatrix._14 * pos.x) + (m_viewMatrix._24 * pos.y) + 
                    (m_viewMatrix._34 * pos.z + m_viewMatrix._44);
    
    *distance = (int)(screenW / 100);
    if (screenW < 0.0001f) return false;
    
    float screenY = (m_viewMatrix._12 * pos.x) + (m_viewMatrix._22 * pos.y) + 
                    (m_viewMatrix._32 * (pos.z + 85) + m_viewMatrix._42);
    float screenX = (m_viewMatrix._11 * pos.x) + (m_viewMatrix._21 * pos.y) + 
                    (m_viewMatrix._31 * pos.z + m_viewMatrix._41);
    
    screen.y = (m_screenHeight / 2.0f) - (m_screenHeight / 2.0f) * screenY / screenW;
    screen.x = (m_screenWidth / 2.0f) + (m_screenWidth / 2.0f) * screenX / screenW;
    
    float y1 = (m_screenHeight / 2.0f) - (m_viewMatrix._12 * pos.x + m_viewMatrix._22 * pos.y + 
               m_viewMatrix._32 * (pos.z - 95) + m_viewMatrix._42) * (m_screenHeight / 2.0f) / screenW;
    screen.z = y1 - screen.y;
    
    return true;
}

FTTransform2_t ESP::readFTransform2(DWORD addr) {
    return get<FTTransform2_t>(addr);
}

D3DMatrix ESP::toMatrixWithScale(Vector3 trans, Vector3 scale, Vector4 rot) {
    D3DMatrix m;
    m._41 = trans.x;
    m._42 = trans.y;
    m._43 = trans.z;

    float x2 = rot.x + rot.x;
    float y2 = rot.y + rot.y;
    float z2 = rot.z + rot.z;

    float xx2 = rot.x * x2;
    float yy2 = rot.y * y2;
    float zz2 = rot.z * z2;
    m._11 = (1.0f - (yy2 + zz2)) * scale.x;
    m._22 = (1.0f - (xx2 + zz2)) * scale.y;
    m._33 = (1.0f - (xx2 + yy2)) * scale.z;

    float yz2 = rot.y * z2;
    float wx2 = rot.w * x2;
    m._32 = (yz2 - wx2) * scale.z;
    m._23 = (yz2 + wx2) * scale.y;

    float xy2 = rot.x * y2;
    float wz2 = rot.w * z2;
    m._21 = (xy2 - wz2) * scale.y;
    m._12 = (xy2 + wz2) * scale.x;

    float xz2 = rot.x * z2;
    float wy2 = rot.w * y2;
    m._31 = (xz2 + wy2) * scale.z;
    m._13 = (xz2 - wy2) * scale.x;

    m._14 = 0.0f;
    m._24 = 0.0f;
    m._34 = 0.0f;
    m._44 = 1.0f;

    return m;
}

D3DMatrix ESP::matrixMultiply(D3DMatrix m1, D3DMatrix m2) {
    D3DMatrix out;
    out._11 = m1._11*m2._11 + m1._12*m2._21 + m1._13*m2._31 + m1._14*m2._41;
    out._12 = m1._11*m2._12 + m1._12*m2._22 + m1._13*m2._32 + m1._14*m2._42;
    out._13 = m1._11*m2._13 + m1._12*m2._23 + m1._13*m2._33 + m1._14*m2._43;
    out._14 = m1._11*m2._14 + m1._12*m2._24 + m1._13*m2._34 + m1._14*m2._44;
    out._21 = m1._21*m2._11 + m1._22*m2._21 + m1._23*m2._31 + m1._24*m2._41;
    out._22 = m1._21*m2._12 + m1._22*m2._22 + m1._23*m2._32 + m1._24*m2._42;
    out._23 = m1._21*m2._13 + m1._22*m2._23 + m1._23*m2._33 + m1._24*m2._43;
    out._24 = m1._21*m2._14 + m1._22*m2._24 + m1._23*m2._34 + m1._24*m2._44;
    out._31 = m1._31*m2._11 + m1._32*m2._21 + m1._33*m2._31 + m1._34*m2._41;
    out._32 = m1._31*m2._12 + m1._32*m2._22 + m1._33*m2._32 + m1._34*m2._42;
    out._33 = m1._31*m2._13 + m1._32*m2._23 + m1._33*m2._33 + m1._34*m2._43;
    out._34 = m1._31*m2._14 + m1._32*m2._24 + m1._33*m2._34 + m1._34*m2._44;
    out._41 = m1._41*m2._11 + m1._42*m2._21 + m1._43*m2._31 + m1._44*m2._41;
    out._42 = m1._41*m2._12 + m1._42*m2._22 + m1._43*m2._32 + m1._44*m2._42;
    out._43 = m1._41*m2._13 + m1._42*m2._23 + m1._43*m2._33 + m1._44*m2._43;
    out._44 = m1._41*m2._14 + m1._42*m2._24 + m1._43*m2._34 + m1._44*m2._44;
    return out;
}

Vector3 ESP::getBoneWorldPosition(DWORD actorAddr, DWORD boneAddr) {
    FTTransform2_t bone = readFTransform2(boneAddr);
    FTTransform2_t actor = readFTransform2(actorAddr);
    
    D3DMatrix boneMatrix = toMatrixWithScale(bone.Translation, bone.Scale3D, bone.Rotation);
    D3DMatrix actorMatrix = toMatrixWithScale(actor.Translation, actor.Scale3D, actor.Rotation);
    D3DMatrix result = matrixMultiply(boneMatrix, actorMatrix);
    
    return Vector3(result._41, result._42, result._43);
}

// Global ESP instance
ESP* g_esp = nullptr;
