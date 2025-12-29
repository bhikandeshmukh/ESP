#ifndef PTRACE_MEMORY_H
#define PTRACE_MEMORY_H

#include <cstdint>
#include <cstddef>

/**
 * Non-root memory access using process_vm_readv/writev
 * Works in ADB mode without root!
 */
class PtraceMemory {
private:
    int m_pid;
    bool m_attached;

public:
    PtraceMemory();
    ~PtraceMemory();
    
    bool attach(int pid);
    void detach();
    
    bool readMemory(uintptr_t address, void* buffer, size_t size);
    bool writeMemory(uintptr_t address, const void* buffer, size_t size);
    
    // Convenience methods
    int readInt(uintptr_t address);
    long readLong(uintptr_t address);
    float readFloat(uintptr_t address);
    
    bool writeInt(uintptr_t address, int value);
    bool writeLong(uintptr_t address, long value);
    bool writeFloat(uintptr_t address, float value);
    
    template<typename T>
    T read(uintptr_t address) {
        T value{};
        readMemory(address, &value, sizeof(T));
        return value;
    }
    
    template<typename T>
    bool write(uintptr_t address, T value) {
        return writeMemory(address, &value, sizeof(T));
    }
    
    bool isAttached() const { return m_pid > 0; }
    int getPid() const { return m_pid; }
};

extern PtraceMemory* g_ptraceMem;

#endif // PTRACE_MEMORY_H
