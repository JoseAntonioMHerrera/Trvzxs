#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum DeviceExtensionBits
{
    DEVICE_EXTENSION_MAINTENANCE9 = 1u << 0,
} DeviceExtensionBits;

typedef uint32_t DeviceExtensions;

static inline bool deviceExtensionsHas(DeviceExtensions gpuExtensionsEnabled, DeviceExtensionBits bit)
{
    return (gpuExtensionsEnabled & bit) != 0;
}
