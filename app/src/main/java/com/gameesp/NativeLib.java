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
}
