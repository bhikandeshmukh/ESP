cmake_minimum_required(VERSION 3.22.1)

project("gameesp")

# ESP App - Pure C++ with Shizuku and all features
add_library(gameesp SHARED
    main.cpp
    esp.cpp
    overlay.cpp
    shizuku_memory.cpp
    ptrace_memory.cpp
    remote_jni.cpp
    scanner.cpp
    skeleton.cpp
    loot.cpp
    antidetect.cpp
)

target_link_libraries(gameesp
    android
    log
    EGL
    GLESv2
)
