#include "skeleton.h"
#include "esp.h"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Skeleton", __VA_ARGS__)

Skeleton::Skeleton(ShizukuMemory* mem) : m_mem(mem) {
    memset(m_bonePositions, 0, sizeof(m_bonePositions));
    memset(m_screenPositions, 0, sizeof(m_screenPositions));
    memset(m_boneValid, 0, sizeof(m_boneValid));
}

void Skeleton::init() {
    m_connections.clear();
    
    // Head to body
    m_connections.push_back({BONE_HEAD, BONE_NECK});
    m_connections.push_back({BONE_NECK, BONE_CHEST});
    m_connections.push_back({BONE_CHEST, BONE_STOMACH});
    m_connections.push_back({BONE_STOMACH, BONE_PELVIS});
    
    // Left arm
    m_connections.push_back({BONE_NECK, BONE_LEFT_SHOULDER});
    m_connections.push_back({BONE_LEFT_SHOULDER, BONE_LEFT_ELBOW});
    m_connections.push_back({BONE_LEFT_ELBOW, BONE_LEFT_HAND});
    
    // Right arm
    m_connections.push_back({BONE_NECK, BONE_RIGHT_SHOULDER});
    m_connections.push_back({BONE_RIGHT_SHOULDER, BONE_RIGHT_ELBOW});
    m_connections.push_back({BONE_RIGHT_ELBOW, BONE_RIGHT_HAND});
    
    // Left leg
    m_connections.push_back({BONE_PELVIS, BONE_LEFT_HIP});
    m_connections.push_back({BONE_LEFT_HIP, BONE_LEFT_KNEE});
    m_connections.push_back({BONE_LEFT_KNEE, BONE_LEFT_FOOT});
    
    // Right leg
    m_connections.push_back({BONE_PELVIS, BONE_RIGHT_HIP});
    m_connections.push_back({BONE_RIGHT_HIP, BONE_RIGHT_KNEE});
    m_connections.push_back({BONE_RIGHT_KNEE, BONE_RIGHT_FOOT});
}

bool Skeleton::readBones(DWORD meshAddr, DWORD boneArrayAddr) {
    if (meshAddr == 0 || boneArrayAddr == 0) return false;
    
    memset(m_boneValid, 0, sizeof(m_boneValid));
    
    // Read bone transforms
    for (const auto& conn : m_connections) {
        // Read 'from' bone
        if (!m_boneValid[conn.from]) {
            DWORD boneAddr = boneArrayAddr + conn.from * 48;  // FTransform size
            FTTransform2_t transform;
            if (m_mem->readBytes(boneAddr, &transform, sizeof(FTTransform2_t))) {
                m_bonePositions[conn.from] = transform.Translation;
                m_boneValid[conn.from] = true;
            }
        }
        
        // Read 'to' bone
        if (!m_boneValid[conn.to]) {
            DWORD boneAddr = boneArrayAddr + conn.to * 48;
            FTTransform2_t transform;
            if (m_mem->readBytes(boneAddr, &transform, sizeof(FTTransform2_t))) {
                m_bonePositions[conn.to] = transform.Translation;
                m_boneValid[conn.to] = true;
            }
        }
    }
    
    return true;
}

bool Skeleton::bonesToScreen(ESP* esp) {
    if (!esp) return false;
    
    bool anyValid = false;
    
    for (int i = 0; i < 64; i++) {
        if (m_boneValid[i]) {
            int dist;
            Vector2 screen;
            if (esp->worldToScreen(m_bonePositions[i], screen, &dist)) {
                m_screenPositions[i] = screen;
                anyValid = true;
            } else {
                m_boneValid[i] = false;
            }
        }
    }
    
    return anyValid;
}

void Skeleton::draw(Overlay* overlay, Color color, float thickness) {
    if (!overlay) return;
    
    for (const auto& conn : m_connections) {
        if (m_boneValid[conn.from] && m_boneValid[conn.to]) {
            overlay->drawLine(
                m_screenPositions[conn.from].x,
                m_screenPositions[conn.from].y,
                m_screenPositions[conn.to].x,
                m_screenPositions[conn.to].y,
                color,
                thickness
            );
        }
    }
}

bool Skeleton::getBoneScreen(BoneID bone, Vector2& screen) {
    int boneIdx = static_cast<int>(bone);
    if (boneIdx >= 64 || !m_boneValid[boneIdx]) return false;
    screen = m_screenPositions[boneIdx];
    return true;
}

bool Skeleton::getHeadPosition(Vector3& worldPos) {
    if (!m_boneValid[BONE_HEAD]) return false;
    worldPos = m_bonePositions[BONE_HEAD];
    return true;
}
