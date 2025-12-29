#ifndef SHIZUKU_MEMORY_H
#define SHIZUKU_MEMORY_H

#include <jni.h>
#include "types.h"

/**
 * Memory access through Shizuku UserService
 * Uses IRemoteService binder for privileged memory operations
 */
class ShizukuMemory {
private:
    JavaVM* m_jvm;
    jobject m_remoteBinder;  // IRemoteService binder
    jclass m_binderClass;
    
    // Cached method IDs
    jmethodID m_readInt;
    jmethodID m_readLong;
    jmethodID m_readFloat;
    jmethodID m_readBytes;
    jmethodID m_writeInt;
    jmethodID m_writeLong;
    jmethodID m_writeFloat;
    jmethodID m_writeBytes;
    jmethodID m_getModuleBase;
    jmethodID m_scanPattern;
    
    bool m_initialized;
    
    JNIEnv* getEnv();

public:
    ShizukuMemory();
    ~ShizukuMemory();
    
    bool init(JNIEnv* env, jobject binder);
    void cleanup();
    bool isInitialized() const { return m_initialized; }
    
    // Memory operations via Shizuku
    int readInt(DWORD address);
    long readLong(DWORD address);
    float readFloat(DWORD address);
    bool readBytes(DWORD address, void* buffer, size_t size);
    
    bool writeInt(DWORD address, int value);
    bool writeLong(DWORD address, long value);
    bool writeFloat(DWORD address, float value);
    bool writeBytes(DWORD address, const void* buffer, size_t size);
    
    DWORD getModuleBase(const char* moduleName);
    DWORD scanPattern(const uint8_t* pattern, size_t size, DWORD start, DWORD end);
    
    // Template helpers
    template<typename T>
    T read(DWORD address) {
        T value{};
        readBytes(address, &value, sizeof(T));
        return value;
    }
    
    template<typename T>
    bool write(DWORD address, T value) {
        return writeBytes(address, &value, sizeof(T));
    }
};

extern ShizukuMemory* g_shizukuMem;

#endif // SHIZUKU_MEMORY_H
