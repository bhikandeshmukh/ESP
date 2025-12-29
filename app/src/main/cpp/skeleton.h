#ifndef SKELETON_H
#define SKELETON_H

#include "types.h"
#include "shizuku_memory.h"
#include "overlay.h"
#include "features.h"
#include <vector>

// Bone connection for drawing skeleton
struct BoneConnection {
    BoneID from;
    BoneID to;
};

class Skeleton {
private:
    ShizukuMemory* m_mem;
    
    // Bone connections to draw
    std::vector<BoneConnection> m_connections;
    
    // Cached bone positions
    Vector3 m_bonePositions[64];
    Vector2 m_screenPositions[64];
    bool m_boneValid[64];

public:
    Skeleton(ShizukuMemory* mem);
    
    // Initialize bone connections
    void init();
    
    // Read all bones for an entity
    bool readBones(DWORD meshAddr, DWORD boneArrayAddr);
    
    // Convert bones to screen coordinates
    bool bonesToScreen(class ESP* esp);
    
    // Draw skeleton
    void draw(Overlay* overlay, Color color, float thickness = 1.5f);
    
    // Get specific bone screen position
    bool getBoneScreen(BoneID bone, Vector2& screen);
    
    // Get head position for aimbot
    bool getHeadPosition(Vector3& worldPos);
};

#endif // SKELETON_H
