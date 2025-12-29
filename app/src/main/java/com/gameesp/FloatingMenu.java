package com.gameesp;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.Build;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

/**
 * Floating Menu for in-game settings
 */
public class FloatingMenu {
    
    private Context context;
    private WindowManager windowManager;
    private View menuView;
    private View iconView;
    private boolean isMenuOpen = false;
    
    private WindowManager.LayoutParams iconParams;
    private WindowManager.LayoutParams menuParams;
    
    // Settings callbacks
    public interface SettingsCallback {
        void onSettingChanged(String key, boolean value);
        void onSettingChanged(String key, int value);
    }
    
    private SettingsCallback callback;
    
    public FloatingMenu(Context context, SettingsCallback callback) {
        this.context = context;
        this.callback = callback;
        this.windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        
        createIconView();
        createMenuView();
    }
    
    private void createIconView() {
        Button icon = new Button(context);
        icon.setText("ESP");
        icon.setTextColor(Color.WHITE);
        icon.setBackgroundColor(Color.argb(200, 233, 69, 96));
        icon.setPadding(20, 10, 20, 10);
        icon.setTextSize(12);
        
        icon.setOnClickListener(v -> toggleMenu());
        
        // Make draggable
        icon.setOnTouchListener(new View.OnTouchListener() {
            private int initialX, initialY;
            private float initialTouchX, initialTouchY;
            private boolean isDragging = false;
            
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = iconParams.x;
                        initialY = iconParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        isDragging = false;
                        return true;
                        
                    case MotionEvent.ACTION_MOVE:
                        int dx = (int) (event.getRawX() - initialTouchX);
                        int dy = (int) (event.getRawY() - initialTouchY);
                        
                        if (Math.abs(dx) > 10 || Math.abs(dy) > 10) {
                            isDragging = true;
                        }
                        
                        if (isDragging) {
                            iconParams.x = initialX + dx;
                            iconParams.y = initialY + dy;
                            windowManager.updateViewLayout(iconView, iconParams);
                        }
                        return true;
                        
                    case MotionEvent.ACTION_UP:
                        if (!isDragging) {
                            v.performClick();
                        }
                        return true;
                }
                return false;
            }
        });
        
        iconView = icon;
        
        iconParams = new WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ?
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                WindowManager.LayoutParams.TYPE_PHONE,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        iconParams.gravity = Gravity.TOP | Gravity.START;
        iconParams.x = 0;
        iconParams.y = 200;
    }
    
    private void createMenuView() {
        LinearLayout mainLayout = new LinearLayout(context);
        mainLayout.setOrientation(LinearLayout.VERTICAL);
        mainLayout.setBackgroundColor(Color.argb(230, 26, 26, 46));
        mainLayout.setPadding(20, 20, 20, 20);
        
        // Title
        TextView title = new TextView(context);
        title.setText("ESP Settings");
        title.setTextColor(Color.parseColor("#e94560"));
        title.setTextSize(18);
        title.setPadding(0, 0, 0, 20);
        mainLayout.addView(title);
        
        // Scrollable content
        ScrollView scrollView = new ScrollView(context);
        LinearLayout content = new LinearLayout(context);
        content.setOrientation(LinearLayout.VERTICAL);
        
        // ESP Section
        addSectionTitle(content, "ESP Features");
        addCheckbox(content, "Enable ESP", "enableESP", true);
        addCheckbox(content, "Box ESP", "enableBox", true);
        addCheckbox(content, "Skeleton", "enableSkeleton", true);
        addCheckbox(content, "Health Bar", "enableHealth", true);
        addCheckbox(content, "Distance", "enableDistance", true);
        addCheckbox(content, "Name", "enableName", true);
        addCheckbox(content, "Snapline", "enableSnapline", true);
        
        // Loot Section
        addSectionTitle(content, "Loot ESP");
        addCheckbox(content, "Enable Loot", "enableLoot", false);
        addCheckbox(content, "Weapons", "lootWeapons", true);
        addCheckbox(content, "Armor", "lootArmor", true);
        addCheckbox(content, "Meds", "lootMeds", true);
        addCheckbox(content, "Scopes", "lootScopes", true);
        
        // Vehicle Section
        addSectionTitle(content, "Other");
        addCheckbox(content, "Vehicles", "enableVehicle", true);
        addCheckbox(content, "Airdrops", "enableAirdrop", true);
        
        // Distance slider
        addSectionTitle(content, "Max Distance");
        addSlider(content, "maxDistance", 100, 500, 300);
        
        // Anti-Detection
        addSectionTitle(content, "Anti-Detection");
        addCheckbox(content, "Enable", "enableAntiDetect", true);
        addCheckbox(content, "Random Delays", "randomizeReads", true);
        
        scrollView.addView(content);
        mainLayout.addView(scrollView);
        
        // Close button
        Button closeBtn = new Button(context);
        closeBtn.setText("Close");
        closeBtn.setBackgroundColor(Color.parseColor("#0f3460"));
        closeBtn.setTextColor(Color.WHITE);
        closeBtn.setOnClickListener(v -> toggleMenu());
        mainLayout.addView(closeBtn);
        
        menuView = mainLayout;
        
        menuParams = new WindowManager.LayoutParams(
            400,
            600,
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ?
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                WindowManager.LayoutParams.TYPE_PHONE,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        menuParams.gravity = Gravity.CENTER;
    }
    
    private void addSectionTitle(LinearLayout parent, String title) {
        TextView tv = new TextView(context);
        tv.setText(title);
        tv.setTextColor(Color.parseColor("#e94560"));
        tv.setTextSize(14);
        tv.setPadding(0, 20, 0, 10);
        parent.addView(tv);
    }
    
    private void addCheckbox(LinearLayout parent, String label, String key, boolean defaultValue) {
        CheckBox cb = new CheckBox(context);
        cb.setText(label);
        cb.setTextColor(Color.WHITE);
        cb.setChecked(defaultValue);
        cb.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (callback != null) {
                callback.onSettingChanged(key, isChecked);
            }
        });
        parent.addView(cb);
    }
    
    private void addSlider(LinearLayout parent, String key, int min, int max, int defaultValue) {
        LinearLayout row = new LinearLayout(context);
        row.setOrientation(LinearLayout.HORIZONTAL);
        
        SeekBar seekBar = new SeekBar(context);
        seekBar.setMax(max - min);
        seekBar.setProgress(defaultValue - min);
        
        TextView valueText = new TextView(context);
        valueText.setText(String.valueOf(defaultValue));
        valueText.setTextColor(Color.WHITE);
        valueText.setPadding(20, 0, 0, 0);
        
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int value = progress + min;
                valueText.setText(String.valueOf(value));
                if (callback != null && fromUser) {
                    callback.onSettingChanged(key, value);
                }
            }
            
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}
            
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        LinearLayout.LayoutParams seekParams = new LinearLayout.LayoutParams(
            0, LinearLayout.LayoutParams.WRAP_CONTENT, 1);
        seekBar.setLayoutParams(seekParams);
        
        row.addView(seekBar);
        row.addView(valueText);
        parent.addView(row);
    }
    
    public void show() {
        if (iconView.getParent() == null) {
            windowManager.addView(iconView, iconParams);
        }
    }
    
    public void hide() {
        if (iconView.getParent() != null) {
            windowManager.removeView(iconView);
        }
        if (isMenuOpen && menuView.getParent() != null) {
            windowManager.removeView(menuView);
            isMenuOpen = false;
        }
    }
    
    private void toggleMenu() {
        if (isMenuOpen) {
            windowManager.removeView(menuView);
            isMenuOpen = false;
        } else {
            windowManager.addView(menuView, menuParams);
            isMenuOpen = true;
        }
    }
    
    public void destroy() {
        hide();
    }
}
