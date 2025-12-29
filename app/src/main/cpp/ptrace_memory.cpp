#include "ptrace_memory.h"
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PtraceMem", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PtraceMem", __VA_ARGS__)

PtraceMemory::PtraceMemory() : m_pid(-1), m_attached(false) {}

PtraceMemory::~PtraceMemory() {
    detach();
}

bool PtraceMemory::attach(int pid) {
    if (m_attached) {
        detach();
    }
    
    m_pid = pid;
    
    // Try ptrace attach (may fail in ADB mode due to SELinux)
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == 0) {
        waitpid(pid, nullptr, 0);
        m_attached = true;
        LOGI("Attached to PID %d via ptrace", pid);
        return true;
    }
    
    // If ptrace fails, we can still use process_vm_readv without attaching
    // This works in ADB mode for reading (not writing)
    LOGI("Ptrace attach failed (errno=%d), using process_vm_readv only", errno);
    m_pid = pid;
    m_attached = false; // Not attached but can still read
    return true;
}

void PtraceMemory::detach() {
    if (m_attached && m_pid > 0) {
        ptrace(PTRACE_DETACH, m_pid, nullptr, nullptr);
        m_attached = false;
    }
    m_pid = -1;
}

bool PtraceMemory::readMemory(uintptr_t address, void* buffer, size_t size) {
    if (m_pid <= 0 || !buffer || size == 0) {
        return false;
    }
    
    // Use process_vm_readv - works in ADB mode!
    struct iovec local[1];
    struct iovec remote[1];
    
    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = size;
    
    ssize_t nread = process_vm_readv(m_pid, local, 1, remote, 1, 0);
    
    if (nread == (ssize_t)size) {
        return true;
    }
    
    if (nread < 0) {
        // LOGE("process_vm_readv failed at 0x%lx: %s", address, strerror(errno));
    }
    
    return false;
}

bool PtraceMemory::writeMemory(uintptr_t address, const void* buffer, size_t size) {
    if (m_pid <= 0 || !buffer || size == 0) {
        return false;
    }
    
    // Use process_vm_writev - may fail in ADB mode
    struct iovec local[1];
    struct iovec remote[1];
    
    local[0].iov_base = (void*)buffer;
    local[0].iov_len = size;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = size;
    
    ssize_t nwritten = process_vm_writev(m_pid, local, 1, remote, 1, 0);
    
    if (nwritten == (ssize_t)size) {
        return true;
    }
    
    if (nwritten < 0) {
        LOGE("process_vm_writev failed at 0x%lx: %s", address, strerror(errno));
    }
    
    return false;
}

int PtraceMemory::readInt(uintptr_t address) {
    int value = 0;
    readMemory(address, &value, sizeof(value));
    return value;
}

long PtraceMemory::readLong(uintptr_t address) {
    long value = 0;
    readMemory(address, &value, sizeof(value));
    return value;
}

float PtraceMemory::readFloat(uintptr_t address) {
    float value = 0;
    readMemory(address, &value, sizeof(value));
    return value;
}

bool PtraceMemory::writeInt(uintptr_t address, int value) {
    return writeMemory(address, &value, sizeof(value));
}

bool PtraceMemory::writeLong(uintptr_t address, long value) {
    return writeMemory(address, &value, sizeof(value));
}

bool PtraceMemory::writeFloat(uintptr_t address, float value) {
    return writeMemory(address, &value, sizeof(value));
}
