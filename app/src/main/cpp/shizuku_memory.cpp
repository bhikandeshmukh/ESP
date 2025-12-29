#include "shizuku_memory.h"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ShizukuMem", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ShizukuMem", __VA_ARGS__)

ShizukuMemory* g_shizukuMem = nullptr;

ShizukuMemory::ShizukuMemory() : m_jvm(nullptr), m_remoteBinder(nullptr),
    m_binderClass(nullptr), m_initialized(false) {}

ShizukuMemory::~ShizukuMemory() {
    cleanup();
}

JNIEnv* ShizukuMemory::getEnv() {
    if (!m_jvm) return nullptr;
    
    JNIEnv* env = nullptr;
    int status = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    
    if (status == JNI_EDETACHED) {
        m_jvm->AttachCurrentThread(&env, nullptr);
    }
    
    return env;
}

bool ShizukuMemory::init(JNIEnv* env, jobject binder) {
    if (!env || !binder) {
        LOGE("Invalid parameters");
        return false;
    }
    
    env->GetJavaVM(&m_jvm);
    
    // Get IRemoteService.Stub.asInterface
    jclass stubClass = env->FindClass("com/gameesp/IRemoteService$Stub");
    if (!stubClass) {
        LOGE("IRemoteService.Stub not found");
        return false;
    }
    
    jmethodID asInterface = env->GetStaticMethodID(stubClass, "asInterface",
        "(Landroid/os/IBinder;)Lcom/gameesp/IRemoteService;");
    if (!asInterface) {
        LOGE("asInterface method not found");
        return false;
    }
    
    // Get IRemoteService instance
    jobject service = env->CallStaticObjectMethod(stubClass, asInterface, binder);
    if (!service) {
        LOGE("Failed to get IRemoteService");
        return false;
    }
    
    m_remoteBinder = env->NewGlobalRef(service);
    m_binderClass = (jclass)env->NewGlobalRef(env->GetObjectClass(m_remoteBinder));
    
    // Cache method IDs
    m_readInt = env->GetMethodID(m_binderClass, "readInt", "(J)I");
    m_readLong = env->GetMethodID(m_binderClass, "readLong", "(J)J");
    m_readFloat = env->GetMethodID(m_binderClass, "readFloat", "(J)F");
    m_readBytes = env->GetMethodID(m_binderClass, "readBytes", "(JI)[B");
    m_writeInt = env->GetMethodID(m_binderClass, "writeInt", "(JI)Z");
    m_writeLong = env->GetMethodID(m_binderClass, "writeLong", "(JJ)Z");
    m_writeFloat = env->GetMethodID(m_binderClass, "writeFloat", "(JF)Z");
    m_writeBytes = env->GetMethodID(m_binderClass, "writeBytes", "(J[B)Z");
    m_getModuleBase = env->GetMethodID(m_binderClass, "getModuleBase", "(Ljava/lang/String;)J");
    m_scanPattern = env->GetMethodID(m_binderClass, "scanPattern", "([BJJ)J");
    
    if (!m_readInt || !m_readBytes || !m_writeInt) {
        LOGE("Failed to get method IDs");
        return false;
    }
    
    m_initialized = true;
    LOGI("ShizukuMemory initialized");
    return true;
}

void ShizukuMemory::cleanup() {
    JNIEnv* env = getEnv();
    if (env) {
        if (m_remoteBinder) {
            env->DeleteGlobalRef(m_remoteBinder);
            m_remoteBinder = nullptr;
        }
        if (m_binderClass) {
            env->DeleteGlobalRef(m_binderClass);
            m_binderClass = nullptr;
        }
    }
    m_initialized = false;
}

int ShizukuMemory::readInt(DWORD address) {
    if (!m_initialized) return 0;
    
    JNIEnv* env = getEnv();
    if (!env) return 0;
    
    return env->CallIntMethod(m_remoteBinder, m_readInt, (jlong)address);
}

long ShizukuMemory::readLong(DWORD address) {
    if (!m_initialized) return 0;
    
    JNIEnv* env = getEnv();
    if (!env) return 0;
    
    return env->CallLongMethod(m_remoteBinder, m_readLong, (jlong)address);
}

float ShizukuMemory::readFloat(DWORD address) {
    if (!m_initialized) return 0;
    
    JNIEnv* env = getEnv();
    if (!env) return 0;
    
    return env->CallFloatMethod(m_remoteBinder, m_readFloat, (jlong)address);
}

bool ShizukuMemory::readBytes(DWORD address, void* buffer, size_t size) {
    if (!m_initialized || !buffer || size == 0) return false;
    
    JNIEnv* env = getEnv();
    if (!env) return false;
    
    jbyteArray result = (jbyteArray)env->CallObjectMethod(m_remoteBinder, 
        m_readBytes, (jlong)address, (jint)size);
    
    if (!result) return false;
    
    jsize len = env->GetArrayLength(result);
    if (len != (jsize)size) {
        env->DeleteLocalRef(result);
        return false;
    }
    
    env->GetByteArrayRegion(result, 0, len, (jbyte*)buffer);
    env->DeleteLocalRef(result);
    
    return true;
}

bool ShizukuMemory::writeInt(DWORD address, int value) {
    if (!m_initialized) return false;
    
    JNIEnv* env = getEnv();
    if (!env) return false;
    
    return env->CallBooleanMethod(m_remoteBinder, m_writeInt, (jlong)address, value);
}

bool ShizukuMemory::writeLong(DWORD address, long value) {
    if (!m_initialized) return false;
    
    JNIEnv* env = getEnv();
    if (!env) return false;
    
    return env->CallBooleanMethod(m_remoteBinder, m_writeLong, (jlong)address, value);
}

bool ShizukuMemory::writeFloat(DWORD address, float value) {
    if (!m_initialized) return false;
    
    JNIEnv* env = getEnv();
    if (!env) return false;
    
    return env->CallBooleanMethod(m_remoteBinder, m_writeFloat, (jlong)address, value);
}

bool ShizukuMemory::writeBytes(DWORD address, const void* buffer, size_t size) {
    if (!m_initialized || !buffer || size == 0) return false;
    
    JNIEnv* env = getEnv();
    if (!env) return false;
    
    jbyteArray data = env->NewByteArray(size);
    env->SetByteArrayRegion(data, 0, size, (const jbyte*)buffer);
    
    bool result = env->CallBooleanMethod(m_remoteBinder, m_writeBytes, (jlong)address, data);
    env->DeleteLocalRef(data);
    
    return result;
}

DWORD ShizukuMemory::getModuleBase(const char* moduleName) {
    if (!m_initialized || !moduleName) return 0;
    
    JNIEnv* env = getEnv();
    if (!env) return 0;
    
    jstring name = env->NewStringUTF(moduleName);
    jlong result = env->CallLongMethod(m_remoteBinder, m_getModuleBase, name);
    env->DeleteLocalRef(name);
    
    return (DWORD)result;
}

DWORD ShizukuMemory::scanPattern(const uint8_t* pattern, size_t size, DWORD start, DWORD end) {
    if (!m_initialized || !pattern || size == 0) return 0;
    
    JNIEnv* env = getEnv();
    if (!env) return 0;
    
    jbyteArray patternArray = env->NewByteArray(size);
    env->SetByteArrayRegion(patternArray, 0, size, (const jbyte*)pattern);
    
    jlong result = env->CallLongMethod(m_remoteBinder, m_scanPattern, 
        patternArray, (jlong)start, (jlong)end);
    env->DeleteLocalRef(patternArray);
    
    return (DWORD)result;
}
