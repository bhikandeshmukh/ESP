package com.gameesp;

import android.os.IBinder;
import android.view.Surface;

/**
 * Native library wrapper - All logic is in C++
 */
public class NativeLib {
    
    static {
        System.loadLibrary("gameesp");
    }
    
    // Initialize
    public static native boolean init();
    
    // Set Shizuku remote service binder
    public static native void setRemoteService(IBinder binder);
    
    // Overlay
    public static native boolean startOverlay(Surface surface);
    public static native void stop();
    
    // ESP Info
    public static native int getEnemyCount();
    public static native int getEntityCount();
    
    // Settings - Boolean
    public static native void setSetting(String key, boolean value);
    
    // Settings - Integer
    public static native void setSettingInt(String key, int value);
    
    // Get settings
    public static native boolean getSetting(String key);
    public static native int getSettingInt(String key);
}
