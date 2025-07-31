#include "light.h"

LightVolumeInstance* getLightVolumeInstance(uint32_t instanceIndex, LightVolume* pLightVolume)
{
    return &pLightVolume->pLightVolumeInstance[instanceIndex];
}
