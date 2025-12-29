/**
 * JNI bindings for RemoteService native methods
 * Uses process_vm_readv/writev for non-root memory access
 */

#include "ptrace_memory.h"
#include <android/log.h>
#include <jni.h>


#define LOGI(...)                                                              \
  __android_log_print(ANDROID_LOG_INFO, "RemoteJNI", __VA_ARGS__)
#define LOGE(...)                                                              \
  __android_log_print(ANDROID_LOG_ERROR, "RemoteJNI", __VA_ARGS__)

// Use global from ptrace_memory.h (extern PtraceMemory* g_ptraceMem)

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_gameesp_RemoteService_nativeAttach(
    JNIEnv *env, jobject thiz, jint pid) {
  LOGI("nativeAttach called for PID: %d", pid);

  if (!g_ptraceMem) {
    g_ptraceMem = new PtraceMemory();
  }

  bool result = g_ptraceMem->attach(pid);
  LOGI("Attach result: %s", result ? "success" : "failed");
  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gameesp_RemoteService_nativeDetach(JNIEnv *env, jobject thiz) {
  LOGI("nativeDetach called");

  if (g_ptraceMem) {
    g_ptraceMem->detach();
  }
}

JNIEXPORT jbyteArray JNICALL Java_com_gameesp_RemoteService_nativeReadBytes(
    JNIEnv *env, jobject thiz, jlong address, jint size) {
  if (!g_ptraceMem || size <= 0 || size > 1024 * 1024) {
    return nullptr;
  }

  // Allocate temporary buffer
  uint8_t *buffer = new uint8_t[size];

  bool success = g_ptraceMem->readMemory((uintptr_t)address, buffer, size);

  if (!success) {
    delete[] buffer;
    return nullptr;
  }

  // Create Java byte array
  jbyteArray result = env->NewByteArray(size);
  if (result) {
    env->SetByteArrayRegion(result, 0, size, (jbyte *)buffer);
  }

  delete[] buffer;
  return result;
}

JNIEXPORT jboolean JNICALL Java_com_gameesp_RemoteService_nativeWriteBytes(
    JNIEnv *env, jobject thiz, jlong address, jbyteArray data) {
  if (!g_ptraceMem || !data) {
    return JNI_FALSE;
  }

  jsize size = env->GetArrayLength(data);
  if (size <= 0) {
    return JNI_FALSE;
  }

  // Get data from Java array
  jbyte *buffer = env->GetByteArrayElements(data, nullptr);
  if (!buffer) {
    return JNI_FALSE;
  }

  bool success = g_ptraceMem->writeMemory((uintptr_t)address, buffer, size);

  env->ReleaseByteArrayElements(data, buffer, JNI_ABORT);

  return success ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
