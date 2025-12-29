#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <vector>

typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef void* PVOID;
typedef size_t* PSIZE_T;

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct Vector4 {
    float x, y, z, w;
    Vector4() : x(0), y(0), z(0), w(0) {}
};

struct D3DMatrix {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
};

struct FTTransform2_t {
    Vector4 Rotation;
    Vector3 Translation;
    Vector3 Scale3D;
};

struct EntityData {
    DWORD address;
    DWORD entityStruct;
    Vector3 position;
    float health;
    float maxHealth;
    int teamId;
    int status;
    int pose;
    std::string name;
    std::string className;
    bool isPlayer;
    bool isBot;
    bool isVehicle;
    bool isBox;
    bool isLoot;
    DWORD boneAddr;
    DWORD bodyAddr;
    DWORD playerWorld;
    int itemId;
    
    // Weapons
    int weapon1Id;
    int weapon2Id;
    int weapon1Ammo;
    int weapon2Ammo;
    
    EntityData() : address(0), entityStruct(0), health(0), maxHealth(100),
                   teamId(0), status(0), pose(0), isPlayer(false), isBot(false),
                   isVehicle(false), isBox(false), isLoot(false), boneAddr(0),
                   bodyAddr(0), playerWorld(0), itemId(0), weapon1Id(0),
                   weapon2Id(0), weapon1Ammo(0), weapon2Ammo(0) {}
};

// ESP Offsets from ESP.cpp
struct Offsets {
    DWORD controllerOffset = 96;
    DWORD posOffset = 336;
    DWORD healthOffset = 1912;
    DWORD nameOffset = 1512;
    DWORD teamIDOffset = 1552;
    DWORD statusOffset = 868;
    DWORD poseOffset = 288;
};

#endif // TYPES_H
