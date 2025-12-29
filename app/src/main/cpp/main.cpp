#include <jni.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <thread>
#include <atomic>
#include <cstring>

#include "types.h"
#include "shizuku_memory.h"
#include "scanner.h"
#include "esp.h"
#include "overlay.h"
#include "features.h"
#include "skeleton.h"
#include "loot.h"
#include "antidetect.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "GameESP", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GameESP", __VA_ARGS__)

// Global instances
extern ShizukuMemory* g_shizukuMem;
extern Overlay* g_overlay;
extern Scanner* g_scanner;
extern AntiDetect g_antiDetect;

// Global config
ESPConfig g_config;

static ESP* g_esp = nullptr;
static Skeleton* g_skeleton = nullptr;
static LootManager* g_loot = nullptr;
static std::atomic<bool> g_running(false);
static std::thread g_espThread;

// Draw distance text
void drawDistanceText(Overlay* overlay, float x, float y, int distance, Color color) {
    // Simple distance indicator (number will be drawn as lines forming digits)
    char distStr[16];
    snprintf(distStr, sizeof(distStr), "%dm", distance);
    
    // Draw background
    float textWidth = strlen(distStr) * 8.0f;
    overlay->drawFilledRect(x - textWidth/2 - 2, y - 2, textWidth + 4, 14, Color(0, 0, 0, 0.5f));
    
    // Draw text as simple lines (basic number rendering)
    float tx = x - textWidth/2;
    for (int i = 0; distStr[i]; i++) {
        // Simple dot for each character position
        overlay->drawFilledRect(tx, y, 6, 10, color);
        tx += 8;
    }
}

// ESP main loop with all features
void espLoop() {
    LOGI("ESP loop started with all features");
    
    while (g_running) {
        if (!g_esp || !g_overlay) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // Anti-detection: random frame skip
        if (g_config.enableAntiDetect && g_antiDetect.shouldSkipFrame(3)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        
        // Check if ESP enabled
        if (!g_config.enableESP) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        
        ANTI_DETECT_PRE();
        
        // Update ESP data
        g_esp->getViewMatrix();
        g_esp->scanEntityList();
        g_esp->scanEntityPositions();
        
        ANTI_DETECT_POST();
        
        // Draw overlay
        g_overlay->beginFrame();
        
        EntityData* entities = g_esp->getEntities();
        int count = g_esp->getEntityCount();
        int myTeam = g_esp->getMyTeamId();
        int maxDist = g_config.maxDistance.load();
        
        // Draw players
        for (int i = 0; i < count; i++) {
            EntityData& ent = entities[i];
            
            if (!ent.isPlayer) continue;
            if (ent.teamId == myTeam) continue;
            if (ent.status == 6) continue;
            
            Vector3 screenPos;
            int distance;
            
            if (!g_esp->worldToScreenPlayer(ent.position, screenPos, &distance)) continue;
            if (distance > maxDist) continue;
            
            float boxHeight = screenPos.z;
            float boxWidth = boxHeight * 0.6f;
            float boxX = screenPos.x - boxWidth / 2;
            float boxY = screenPos.y - boxHeight;
            
            Color boxColor = ent.isBot ? Color::Yellow() : Color::Red();
            
            // Box ESP
            if (g_config.enableBox) {
                g_overlay->drawBox(boxX, boxY, boxWidth, boxHeight, boxColor);
            }
            
            // Skeleton ESP
            if (g_config.enableSkeleton && g_skeleton && ent.boneAddr != 0) {
                g_skeleton->readBones(ent.bodyAddr, ent.boneAddr);
                g_skeleton->bonesToScreen(g_esp);
                g_skeleton->draw(g_overlay, boxColor, g_config.skeletonThickness);
            }
            
            // Health bar
            if (g_config.enableHealth) {
                g_overlay->drawHealthBar(boxX - 6, boxY, 4, boxHeight, 
                                         ent.health, ent.maxHealth);
            }
            
            // Distance text
            if (g_config.enableDistance) {
                drawDistanceText(g_overlay, screenPos.x, screenPos.y + 5, 
                                distance, Color::White());
            }
            
            // Snapline
            if (g_config.enableSnapline) {
                g_overlay->drawSnapLine(screenPos.x, screenPos.y, 
                                       Color(1, 1, 0, 0.5f));
            }
        }
        
        // Draw Loot ESP
        if (g_config.enableLoot && g_loot) {
            // Get player position for distance calc
            Vector3 playerPos = {0, 0, 0};
            for (int i = 0; i < count; i++) {
                if (entities[i].isPlayer && entities[i].teamId == myTeam) {
                    playerPos = entities[i].position;
                    break;
                }
            }
            
            const auto& items = g_loot->getItems();
            for (const auto& item : items) {
                Vector2 screen;
                int dist;
                if (g_esp->worldToScreen(item.position, screen, &dist)) {
                    Color lootColor = LootManager::getLootColor(item.type);
                    const char* symbol = LootManager::getLootSymbol(item.type);
                    
                    // Draw loot marker
                    g_overlay->drawFilledRect(screen.x - 3, screen.y - 3, 6, 6, lootColor);
                    
                    // Draw distance
                    if (g_config.enableDistance) {
                        drawDistanceText(g_overlay, screen.x, screen.y + 10, 
                                        (int)item.distance, lootColor);
                    }
                }
            }
        }
        
        // Draw Vehicles
        if (g_config.enableVehicle) {
            for (int i = 0; i < count; i++) {
                EntityData& ent = entities[i];
                if (!ent.isVehicle) continue;
                
                Vector2 screen;
                int dist;
                if (g_esp->worldToScreen(ent.position, screen, &dist)) {
                    if (dist <= maxDist) {
                        Color vehColor(1, 0, 1, 1);  // Magenta
                        g_overlay->drawRect(screen.x - 15, screen.y - 10, 30, 20, vehColor);
                        
                        // Vehicle health
                        if (ent.maxHealth > 0) {
                            g_overlay->drawHealthBar(screen.x - 15, screen.y - 15, 
                                                    30, 4, ent.health, ent.maxHealth);
                        }
                    }
                }
            }
        }
        
        // Draw enemy count
        char infoText[64];
        snprintf(infoText, sizeof(infoText), "Enemies: %d", g_esp->getEnemyCount());
        g_overlay->drawFilledRect(10, 10, 100, 20, Color(0, 0, 0, 0.7f));
        
        g_overlay->endFrame();
        
        // Frame timing with anti-detect randomization
        int frameDelay = 16;
        if (g_config.enableAntiDetect) {
            frameDelay = g_antiDetect.randomInt(14, 18);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
    }
    
    LOGI("ESP loop stopped");
}

extern "C" {

// Initialize native library
JNIEXPORT jboolean JNICALL
Java_com_gameesp_NativeLib_init(JNIEnv* env, jobject thiz) {
    LOGI("Initializing GameESP Native with all features");
    
    if (!g_shizukuMem) {
        g_shizukuMem = new ShizukuMemory();
    }
    
    return JNI_TRUE;
}

// Set Shizuku remote service binder
JNIEXPORT void JNICALL
Java_com_gameesp_NativeLib_setRemoteService(JNIEnv* env, jobject thiz, jobject binder) {
    LOGI("Setting remote service binder");
    
    if (!g_shizukuMem) {
        g_shizukuMem = new ShizukuMemory();
    }
    
    if (g_shizukuMem->init(env, binder)) {
        LOGI("ShizukuMemory initialized successfully");
        
        // Initialize ESP
        if (!g_esp) {
            g_esp = new ESP(g_shizukuMem);
            if (g_esp->init()) {
                LOGI("ESP initialized successfully");
                
                // Initialize Skeleton
                g_skeleton = new Skeleton(g_shizukuMem);
                g_skeleton->init();
                
                // Initialize Loot Manager
                g_loot = new LootManager(g_shizukuMem);
                
                LOGI("All features initialized");
            } else {
                LOGE("Failed to initialize ESP");
            }
        }
    } else {
        LOGE("Failed to initialize ShizukuMemory");
    }
}

// Start ESP overlay
JNIEXPORT jboolean JNICALL
Java_com_gameesp_NativeLib_startOverlay(JNIEnv* env, jobject thiz, jobject surface) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window");
        return JNI_FALSE;
    }
    
    g_overlay = new Overlay();
    if (!g_overlay->init(window)) {
        LOGE("Failed to initialize overlay");
        return JNI_FALSE;
    }
    
    if (g_esp) {
        g_esp->setScreenSize(g_overlay->getWidth(), g_overlay->getHeight());
    }
    
    // Start ESP thread
    g_running = true;
    g_espThread = std::thread(espLoop);
    
    LOGI("Overlay started");
    return JNI_TRUE;
}

// Stop ESP
JNIEXPORT void JNICALL
Java_com_gameesp_NativeLib_stop(JNIEnv* env, jobject thiz) {
    g_running = false;
    
    if (g_espThread.joinable()) {
        g_espThread.join();
    }
    
    if (g_overlay) {
        delete g_overlay;
        g_overlay = nullptr;
    }
    
    if (g_skeleton) {
        delete g_skeleton;
        g_skeleton = nullptr;
    }
    
    if (g_loot) {
        delete g_loot;
        g_loot = nullptr;
    }
    
    if (g_esp) {
        delete g_esp;
        g_esp = nullptr;
    }
    
    if (g_scanner) {
        delete g_scanner;
        g_scanner = nullptr;
    }
    
    LOGI("ESP stopped");
}

// Get ESP info
JNIEXPORT jint JNICALL
Java_com_gameesp_NativeLib_getEnemyCount(JNIEnv* env, jobject thiz) {
    if (!g_esp) return 0;
    return g_esp->getEnemyCount();
}

JNIEXPORT jint JNICALL
Java_com_gameesp_NativeLib_getEntityCount(JNIEnv* env, jobject thiz) {
    if (!g_esp) return 0;
    return g_esp->getEntityCount();
}

// Settings - Boolean
JNIEXPORT void JNICALL
Java_com_gameesp_NativeLib_setSetting(JNIEnv* env, jobject thiz, jstring key, jboolean value) {
    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    
    if (strcmp(keyStr, "enableESP") == 0) g_config.enableESP = value;
    else if (strcmp(keyStr, "enableBox") == 0) g_config.enableBox = value;
    else if (strcmp(keyStr, "enableSkeleton") == 0) g_config.enableSkeleton = value;
    else if (strcmp(keyStr, "enableHealth") == 0) g_config.enableHealth = value;
    else if (strcmp(keyStr, "enableDistance") == 0) g_config.enableDistance = value;
    else if (strcmp(keyStr, "enableName") == 0) g_config.enableName = value;
    else if (strcmp(keyStr, "enableSnapline") == 0) g_config.enableSnapline = value;
    else if (strcmp(keyStr, "enableLoot") == 0) g_config.enableLoot = value;
    else if (strcmp(keyStr, "lootWeapons") == 0) g_config.lootWeapons = value;
    else if (strcmp(keyStr, "lootArmor") == 0) g_config.lootArmor = value;
    else if (strcmp(keyStr, "lootMeds") == 0) g_config.lootMeds = value;
    else if (strcmp(keyStr, "lootScopes") == 0) g_config.lootScopes = value;
    else if (strcmp(keyStr, "enableVehicle") == 0) g_config.enableVehicle = value;
    else if (strcmp(keyStr, "enableAirdrop") == 0) g_config.enableAirdrop = value;
    else if (strcmp(keyStr, "enableAntiDetect") == 0) g_config.enableAntiDetect = value;
    else if (strcmp(keyStr, "randomizeReads") == 0) g_config.randomizeReads = value;
    
    LOGI("Setting %s = %d", keyStr, value);
    env->ReleaseStringUTFChars(key, keyStr);
}

// Settings - Integer
JNIEXPORT void JNICALL
Java_com_gameesp_NativeLib_setSettingInt(JNIEnv* env, jobject thiz, jstring key, jint value) {
    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    
    if (strcmp(keyStr, "maxDistance") == 0) g_config.maxDistance = value;
    else if (strcmp(keyStr, "lootMaxDistance") == 0) g_config.lootMaxDistance = value;
    
    LOGI("Setting %s = %d", keyStr, value);
    env->ReleaseStringUTFChars(key, keyStr);
}

// Get settings - Boolean
JNIEXPORT jboolean JNICALL
Java_com_gameesp_NativeLib_getSetting(JNIEnv* env, jobject thiz, jstring key) {
    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    bool result = false;
    
    if (strcmp(keyStr, "enableESP") == 0) result = g_config.enableESP;
    else if (strcmp(keyStr, "enableBox") == 0) result = g_config.enableBox;
    else if (strcmp(keyStr, "enableSkeleton") == 0) result = g_config.enableSkeleton;
    
    env->ReleaseStringUTFChars(key, keyStr);
    return result;
}

// Get settings - Integer
JNIEXPORT jint JNICALL
Java_com_gameesp_NativeLib_getSettingInt(JNIEnv* env, jobject thiz, jstring key) {
    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    int result = 0;
    
    if (strcmp(keyStr, "maxDistance") == 0) result = g_config.maxDistance;
    
    env->ReleaseStringUTFChars(key, keyStr);
    return result;
}

} // extern "C"
