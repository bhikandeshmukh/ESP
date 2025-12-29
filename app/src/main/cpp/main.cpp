#include <jni.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <thread>
#include <atomic>

#include "types.h"
#include "shizuku_memory.h"
#include "scanner.h"
#include "esp.h"
#include "overlay.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "GameESP", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GameESP", __VA_ARGS__)

// Global instances
extern ShizukuMemory* g_shizukuMem;
extern Overlay* g_overlay;
extern Scanner* g_scanner;

static ESP* g_esp = nullptr;
static std::atomic<bool> g_running(false);
static std::thread g_espThread;

// ESP main loop
void espLoop() {
    LOGI("ESP loop started");
    
    while (g_running) {
        if (!g_esp || !g_overlay) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // Update ESP data
        g_esp->getViewMatrix();
        g_esp->scanEntityList();
        g_esp->scanEntityPositions();
        
        // Draw overlay
        g_overlay->beginFrame();
        
        EntityData* entities = g_esp->getEntities();
        int count = g_esp->getEntityCount();
        int myTeam = g_esp->getMyTeamId();
        
        for (int i = 0; i < count; i++) {
            EntityData& ent = entities[i];
            
            if (!ent.isPlayer) continue;
            if (ent.teamId == myTeam) continue; // Skip teammates
            if (ent.status == 6) continue; // Skip dead
            
            Vector3 screenPos;
            int distance;
            
            if (g_esp->worldToScreenPlayer(ent.position, screenPos, &distance)) {
                // Box dimensions based on distance
                float boxHeight = screenPos.z;
                float boxWidth = boxHeight * 0.6f;
                float boxX = screenPos.x - boxWidth / 2;
                float boxY = screenPos.y - boxHeight;
                
                // Enemy color
                Color boxColor = ent.isBot ? Color::Yellow() : Color::Red();
                
                // Draw ESP box
                g_overlay->drawBox(boxX, boxY, boxWidth, boxHeight, boxColor);
                
                // Health bar
                g_overlay->drawHealthBar(boxX - 6, boxY, 4, boxHeight, 
                                         ent.health, ent.maxHealth);
                
                // Snap line
                g_overlay->drawSnapLine(screenPos.x, screenPos.y, Color(1, 1, 0, 0.5f));
            }
        }
        
        g_overlay->endFrame();
        
        // ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    LOGI("ESP loop stopped");
}

extern "C" {

// Initialize native library
JNIEXPORT jboolean JNICALL
Java_com_gameesp_NativeLib_init(JNIEnv* env, jobject thiz) {
    LOGI("Initializing GameESP Native");
    
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

} // extern "C"
