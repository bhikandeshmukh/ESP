#ifndef ESP_H
#define ESP_H

#include "types.h"
#include "shizuku_memory.h"

#define MAX_ENTITY 1024

class ESP {
private:
    ShizukuMemory* m_mem;
    Offsets m_offsets;
    
    // Game addresses
    DWORD m_viewWorld;
    DWORD m_uWorld;
    DWORD m_gNames;
    DWORD m_viewMatrixBase;
    
    // Entity data
    EntityData m_entities[MAX_ENTITY];
    int m_entityCount;
    int m_enemyCount;
    int m_myTeamId;
    DWORD m_myObject;
    
    // View matrix
    D3DMatrix m_viewMatrix;
    
    // Screen
    int m_screenWidth;
    int m_screenHeight;

public:
    ESP(ShizukuMemory* mem);
    ~ESP();
    
    // Initialize
    bool init();
    void setScreenSize(int w, int h);
    
    // Core functions from ESP.cpp
    DWORD getViewWorld();
    void getViewMatrix();
    void scanEntityList();
    void scanEntityPositions();
    
    // Entity functions
    std::string getEntityClassName(DWORD gNames, DWORD id);
    std::string getPlayerName(DWORD entityAddr);
    
    // World to Screen
    bool worldToScreen(Vector3 pos, Vector2& screen, int* distance);
    bool worldToScreen(Vector3 pos, Vector3& screen, int* distance);
    bool worldToScreenPlayer(Vector3 pos, Vector3& screen, int* distance);
    
    // Bone functions
    FTTransform2_t readFTransform2(DWORD addr);
    Vector3 getBoneWorldPosition(DWORD actorAddr, DWORD boneAddr);
    D3DMatrix toMatrixWithScale(Vector3 trans, Vector3 scale, Vector4 rot);
    D3DMatrix matrixMultiply(D3DMatrix m1, D3DMatrix m2);
    
    // Getters
    EntityData* getEntities() { return m_entities; }
    int getEntityCount() const { return m_entityCount; }
    int getEnemyCount() const { return m_enemyCount; }
    int getMyTeamId() const { return m_myTeamId; }
    D3DMatrix& viewMatrix() { return m_viewMatrix; }
    
    // Template read helper
    template<typename T>
    T get(DWORD addr) {
        return m_mem->read<T>(addr);
    }
};

#endif // ESP_H
