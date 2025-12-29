cmake_minimum_required(VERSION 3.22.1)

project("gameesp")

# ESP App - Pure C++ with Shizuku
add_library(gameesp SHARED
    main.cpp
    esp.cpp
    overlay.cpp
    shizuku_memory.cpp
    scanner.cpp
)

target_link_libraries(gameesp
    android
    log
    EGL
    GLESv2
)
