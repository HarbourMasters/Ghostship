#pragma once

#include <cstdint>
#include <Resource.h>

struct PaintingData {
    int16_t id;
    int8_t imageCount;
    int8_t textureType;
    int8_t lastFloor;
    int8_t currFloor;
    int8_t floorEntered;
    int8_t state;

    float pitch;
    float yaw;

    float posX;
    float posY;
    float posZ;

    float currRippleMag;
    float passiveRippleMag;
    float entryRippleMag;

    float rippleDecay;
    float passiveRippleDecay;
    float entryRippleDecay;

    float currRippleRate;
    float passiveRippleRate;
    float entryRippleRate;

    float dispersionFactor;
    float passiveDispersionFactor;
    float entryDispersionFactor;

    float rippleTimer;

    float rippleX;
    float rippleY;

    void* normalDisplayList;
    int16_t* textureMaps;
    uint8_t* textureArray;

    int16_t textureWidth;
    int16_t textureHeight;

    void* rippleDisplayList;

    int8_t rippleTrigger;
    uint8_t alpha;

    int8_t marioWasUnder;
    int8_t marioIsUnder;
    int8_t marioWentUnder;
    
    float size;
};

namespace SM64 {

class Painting : public Ship::Resource<PaintingData> {
  public:
    using Resource::Resource;

    Painting() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {}

    PaintingData* GetPointer();
    size_t GetPointerSize();

    PaintingData mData;
};
}