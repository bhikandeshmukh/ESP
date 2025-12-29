package com.gameesp;

interface IRemoteService {
    void destroy() = 16777114;
    void exit() = 1;
    
    int findProcess(String packageName) = 2;
    boolean attachProcess(int pid) = 3;
    void detachProcess() = 4;
    
    int readInt(long address) = 10;
    long readLong(long address) = 11;
    float readFloat(long address) = 12;
    byte[] readBytes(long address, int size) = 13;
    
    boolean writeInt(long address, int value) = 20;
    boolean writeLong(long address, long value) = 21;
    boolean writeFloat(long address, float value) = 22;
    boolean writeBytes(long address, in byte[] data) = 23;
    
    long getModuleBase(String moduleName) = 30;
    long scanPattern(in byte[] pattern, long startAddr, long endAddr) = 31;
}
