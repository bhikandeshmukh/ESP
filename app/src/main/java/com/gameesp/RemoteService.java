package com.gameesp;

import android.os.RemoteException;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Shizuku UserService - Runs with ROOT/ADB privileges
 * This service runs in a separate process with elevated permissions
 */
public class RemoteService extends IRemoteService.Stub {
    
    private static final String TAG = "RemoteService";
    
    private int mPid = -1;
    private RandomAccessFile mMemFile = null;
    
    // Constructor with Context (Shizuku v13+)
    public RemoteService(android.content.Context context) {
        Log.i(TAG, "RemoteService created with context");
    }
    
    // Default constructor (older Shizuku)
    public RemoteService() {
        Log.i(TAG, "RemoteService created");
    }
    
    @Override
    public void destroy() throws RemoteException {
        Log.i(TAG, "RemoteService destroy called");
        detachProcess();
        System.exit(0);
    }
    
    @Override
    public void exit() throws RemoteException {
        destroy();
    }
    
    @Override
    public int findProcess(String packageName) throws RemoteException {
        try {
            File procDir = new File("/proc");
            File[] files = procDir.listFiles();
            
            if (files == null) return -1;
            
            for (File f : files) {
                if (!f.isDirectory()) continue;
                
                String name = f.getName();
                boolean isNum = true;
                for (char c : name.toCharArray()) {
                    if (c < '0' || c > '9') {
                        isNum = false;
                        break;
                    }
                }
                if (!isNum) continue;
                
                int pid = Integer.parseInt(name);
                String cmdline = readCmdline(pid);
                
                if (cmdline != null && cmdline.contains(packageName)) {
                    Log.i(TAG, "Found process: " + packageName + " PID: " + pid);
                    return pid;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "findProcess error: " + e.getMessage());
        }
        return -1;
    }
    
    private String readCmdline(int pid) {
        try {
            File cmdlineFile = new File("/proc/" + pid + "/cmdline");
            if (!cmdlineFile.exists()) return null;
            
            FileInputStream fis = new FileInputStream(cmdlineFile);
            byte[] buffer = new byte[256];
            int len = fis.read(buffer);
            fis.close();
            
            if (len > 0) {
                // cmdline is null-terminated
                int end = 0;
                for (int i = 0; i < len; i++) {
                    if (buffer[i] == 0) {
                        end = i;
                        break;
                    }
                }
                return new String(buffer, 0, end);
            }
        } catch (Exception e) {
            // Ignore
        }
        return null;
    }
    
    @Override
    public boolean attachProcess(int pid) throws RemoteException {
        try {
            detachProcess();
            
            String memPath = "/proc/" + pid + "/mem";
            File memFile = new File(memPath);
            
            if (!memFile.exists()) {
                Log.e(TAG, "Memory file not found: " + memPath);
                return false;
            }
            
            mMemFile = new RandomAccessFile(memPath, "rw");
            mPid = pid;
            
            Log.i(TAG, "Attached to PID: " + pid);
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "attachProcess error: " + e.getMessage());
            
            // Try read-only mode
            try {
                String memPath = "/proc/" + pid + "/mem";
                mMemFile = new RandomAccessFile(memPath, "r");
                mPid = pid;
                Log.i(TAG, "Attached to PID (read-only): " + pid);
                return true;
            } catch (Exception e2) {
                Log.e(TAG, "attachProcess read-only error: " + e2.getMessage());
            }
        }
        return false;
    }
    
    @Override
    public void detachProcess() throws RemoteException {
        try {
            if (mMemFile != null) {
                mMemFile.close();
                mMemFile = null;
            }
            mPid = -1;
        } catch (Exception e) {
            Log.e(TAG, "detachProcess error: " + e.getMessage());
        }
    }
    
    @Override
    public int readInt(long address) throws RemoteException {
        byte[] data = readBytes(address, 4);
        if (data == null) return 0;
        return ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).getInt();
    }
    
    @Override
    public long readLong(long address) throws RemoteException {
        byte[] data = readBytes(address, 8);
        if (data == null) return 0;
        return ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).getLong();
    }
    
    @Override
    public float readFloat(long address) throws RemoteException {
        byte[] data = readBytes(address, 4);
        if (data == null) return 0;
        return ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).getFloat();
    }
    
    @Override
    public byte[] readBytes(long address, int size) throws RemoteException {
        if (mMemFile == null || size <= 0) return null;
        
        try {
            byte[] buffer = new byte[size];
            mMemFile.seek(address);
            int read = mMemFile.read(buffer);
            
            if (read == size) {
                return buffer;
            }
        } catch (Exception e) {
            // Log.e(TAG, "readBytes error at 0x" + Long.toHexString(address));
        }
        return null;
    }
    
    @Override
    public boolean writeInt(long address, int value) throws RemoteException {
        byte[] data = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(value).array();
        return writeBytes(address, data);
    }
    
    @Override
    public boolean writeLong(long address, long value) throws RemoteException {
        byte[] data = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(value).array();
        return writeBytes(address, data);
    }
    
    @Override
    public boolean writeFloat(long address, float value) throws RemoteException {
        byte[] data = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putFloat(value).array();
        return writeBytes(address, data);
    }
    
    @Override
    public boolean writeBytes(long address, byte[] data) throws RemoteException {
        if (mMemFile == null || data == null) return false;
        
        try {
            mMemFile.seek(address);
            mMemFile.write(data);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "writeBytes error at 0x" + Long.toHexString(address));
        }
        return false;
    }
    
    @Override
    public long getModuleBase(String moduleName) throws RemoteException {
        if (mPid <= 0) return 0;
        
        try {
            File mapsFile = new File("/proc/" + mPid + "/maps");
            BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream(mapsFile)));
            
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.contains(moduleName)) {
                    // Format: address-address perms offset dev inode pathname
                    String[] parts = line.split("-");
                    if (parts.length > 0) {
                        long base = Long.parseLong(parts[0], 16);
                        reader.close();
                        Log.i(TAG, "Module " + moduleName + " base: 0x" + Long.toHexString(base));
                        return base;
                    }
                }
            }
            reader.close();
        } catch (Exception e) {
            Log.e(TAG, "getModuleBase error: " + e.getMessage());
        }
        return 0;
    }
    
    @Override
    public long scanPattern(byte[] pattern, long startAddr, long endAddr) throws RemoteException {
        if (mMemFile == null || pattern == null || pattern.length == 0) return 0;
        
        final int CHUNK_SIZE = 4096;
        byte[] buffer = new byte[CHUNK_SIZE];
        
        try {
            for (long addr = startAddr; addr < endAddr; addr += CHUNK_SIZE - pattern.length) {
                mMemFile.seek(addr);
                int read = mMemFile.read(buffer);
                if (read <= 0) continue;
                
                for (int i = 0; i < read - pattern.length; i++) {
                    boolean found = true;
                    for (int j = 0; j < pattern.length; j++) {
                        if (buffer[i + j] != pattern[j]) {
                            found = false;
                            break;
                        }
                    }
                    if (found) {
                        return addr + i;
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "scanPattern error: " + e.getMessage());
        }
        return 0;
    }
}
