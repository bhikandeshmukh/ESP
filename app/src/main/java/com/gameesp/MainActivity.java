package com.gameesp;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.provider.Settings;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import rikka.shizuku.Shizuku;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

    private static final String TAG = "GameESP";
    private static final int SHIZUKU_CODE = 1001;
    private static final int OVERLAY_CODE = 1002;

    private TextView tvStatus;
    private EditText etPackage;
    private Button btnAttach, btnStart, btnStop;
    private SurfaceView surfaceView;

    private int gamePid = -1;
    private boolean isRunning = false;

    // Floating Menu
    private FloatingMenu floatingMenu;

    // Shizuku UserService
    private IRemoteService remoteService = null;
    private boolean serviceConnected = false;

    private final Shizuku.UserServiceArgs userServiceArgs = new Shizuku.UserServiceArgs(
            new ComponentName(BuildConfig.APPLICATION_ID, RemoteService.class.getName()))
            .daemon(false)
            .processNameSuffix("service")
            .debuggable(BuildConfig.DEBUG)
            .version(BuildConfig.VERSION_CODE);

    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.i(TAG, "UserService connected");
            remoteService = IRemoteService.Stub.asInterface(service);
            serviceConnected = true;
            updateStatus("Shizuku Service: Connected");
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.i(TAG, "UserService disconnected");
            remoteService = null;
            serviceConnected = false;
            updateStatus("Shizuku Service: Disconnected");
        }
    };

    private final Shizuku.OnRequestPermissionResultListener permissionListener = (requestCode, grantResult) -> {
        if (requestCode == SHIZUKU_CODE) {
            if (grantResult == PackageManager.PERMISSION_GRANTED) {
                updateStatus("Shizuku: Permission Granted");
                bindUserService();
            } else {
                updateStatus("Shizuku: Permission Denied");
            }
        }
    };

    private final Shizuku.OnBinderReceivedListener binderReceivedListener = () -> {
        Log.i(TAG, "Shizuku binder received");
        checkAndRequestPermission();
    };

    private final Shizuku.OnBinderDeadListener binderDeadListener = () -> {
        Log.i(TAG, "Shizuku binder dead");
        updateStatus("Shizuku: Disconnected");
        serviceConnected = false;
    };

    // Settings callback for floating menu
    private final FloatingMenu.SettingsCallback settingsCallback = new FloatingMenu.SettingsCallback() {
        @Override
        public void onSettingChanged(String key, boolean value) {
            NativeLib.setSetting(key, value);
            Log.i(TAG, "Setting changed: " + key + " = " + value);
        }

        @Override
        public void onSettingChanged(String key, int value) {
            NativeLib.setSettingInt(key, value);
            Log.i(TAG, "Setting changed: " + key + " = " + value);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tvStatus = findViewById(R.id.tvStatus);
        etPackage = findViewById(R.id.etPackage);
        btnAttach = findViewById(R.id.btnAttach);
        btnStart = findViewById(R.id.btnStart);
        btnStop = findViewById(R.id.btnStop);
        surfaceView = findViewById(R.id.surfaceView);

        surfaceView.getHolder().addCallback(this);
        surfaceView.setZOrderOnTop(true);

        // Initialize native
        NativeLib.init();

        setupShizuku();
        setupButtons();
    }

    private void setupShizuku() {
        Shizuku.addRequestPermissionResultListener(permissionListener);
        Shizuku.addBinderReceivedListenerSticky(binderReceivedListener);
        Shizuku.addBinderDeadListener(binderDeadListener);

        if (Shizuku.pingBinder()) {
            checkAndRequestPermission();
        } else {
            updateStatus("Shizuku: Not Running - Start Shizuku app first!");
        }
    }

    private void checkAndRequestPermission() {
        if (Shizuku.isPreV11()) {
            updateStatus("Shizuku: Version too old (need v11+)");
            return;
        }

        // Check if running with ROOT or ADB
        try {
            int uid = Shizuku.getUid();
            if (uid == 2000) {
                // ADB mode - limited permissions
                updateStatus("⚠️ Shizuku: ADB mode (limited) - ROOT recommended");
                toast("ADB mode has limited permissions. For full functionality, use ROOT mode in Shizuku app.");
            } else if (uid == 0) {
                // ROOT mode - full permissions
                updateStatus("✓ Shizuku: ROOT mode (full access)");
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to check Shizuku UID: " + e.getMessage());
        }

        if (Shizuku.checkSelfPermission() == PackageManager.PERMISSION_GRANTED) {
            updateStatus("Shizuku: Ready");
            bindUserService();
        } else if (Shizuku.shouldShowRequestPermissionRationale()) {
            updateStatus("Shizuku: Permission denied permanently");
        } else {
            Shizuku.requestPermission(SHIZUKU_CODE);
        }
    }

    private void bindUserService() {
        try {
            Shizuku.bindUserService(userServiceArgs, serviceConnection);
            Log.i(TAG, "Binding UserService...");
        } catch (Exception e) {
            Log.e(TAG, "Failed to bind UserService: " + e.getMessage());
            updateStatus("Failed to bind service: " + e.getMessage());
        }
    }

    private void unbindUserService() {
        try {
            Shizuku.unbindUserService(userServiceArgs, serviceConnection, true);
        } catch (Exception e) {
            Log.e(TAG, "Failed to unbind: " + e.getMessage());
        }
    }

    private void setupButtons() {
        btnAttach.setOnClickListener(v -> {
            if (!serviceConnected || remoteService == null) {
                toast("Shizuku service not connected");
                return;
            }

            String pkg = etPackage.getText().toString();
            if (pkg.isEmpty()) {
                toast("Enter package name");
                return;
            }

            try {
                gamePid = remoteService.findProcess(pkg);
                if (gamePid > 0) {
                    if (remoteService.attachProcess(gamePid)) {
                        updateStatus("Attached: " + pkg + " (PID: " + gamePid + ")");

                        // Pass service to native
                        NativeLib.setRemoteService(remoteService.asBinder());
                    } else {
                        updateStatus("Attach failed - need ROOT?");
                    }
                } else {
                    updateStatus("Game not found - is it running?");
                }
            } catch (RemoteException e) {
                updateStatus("Error: " + e.getMessage());
            }
        });

        btnStart.setOnClickListener(v -> {
            if (gamePid <= 0) {
                toast("Attach to game first");
                return;
            }

            if (!Settings.canDrawOverlays(this)) {
                Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                        Uri.parse("package:" + getPackageName()));
                startActivityForResult(intent, OVERLAY_CODE);
                return;
            }

            startESP();
        });

        btnStop.setOnClickListener(v -> {
            stopESP();
        });
    }

    private void startESP() {
        if (surfaceView.getHolder().getSurface().isValid()) {
            if (NativeLib.startOverlay(surfaceView.getHolder().getSurface())) {
                isRunning = true;
                updateStatus("ESP Running - Enemies: " + NativeLib.getEnemyCount());

                // Start foreground service to keep running in background
                Intent serviceIntent = new Intent(this, ESPForegroundService.class);
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    startForegroundService(serviceIntent);
                } else {
                    startService(serviceIntent);
                }

                // Show floating menu
                if (floatingMenu == null) {
                    floatingMenu = new FloatingMenu(this, settingsCallback);
                }
                floatingMenu.show();

                toast("ESP Started! Tap 'ESP' button for settings");
            } else {
                updateStatus("Failed to start overlay");
            }
        }
    }

    private void stopESP() {
        NativeLib.stop();
        isRunning = false;
        updateStatus("ESP Stopped");

        // Stop foreground service
        stopService(new Intent(this, ESPForegroundService.class));

        // Hide floating menu
        if (floatingMenu != null) {
            floatingMenu.hide();
        }
    }

    private void updateStatus(String msg) {
        runOnUiThread(() -> tvStatus.setText(msg));
    }

    private void toast(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (isRunning) {
            stopESP();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        stopESP();
        unbindUserService();

        if (floatingMenu != null) {
            floatingMenu.destroy();
            floatingMenu = null;
        }

        Shizuku.removeRequestPermissionResultListener(permissionListener);
        Shizuku.removeBinderReceivedListener(binderReceivedListener);
        Shizuku.removeBinderDeadListener(binderDeadListener);
    }
}
