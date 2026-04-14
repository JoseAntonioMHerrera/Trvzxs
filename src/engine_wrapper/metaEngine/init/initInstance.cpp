#include "initInstance.hpp"

#include <cstdlib>

extern "C"
{
#include "main_vulkan.h"
}

void metaInitInstance(MetaInitContext &ctx, SDL_Window *window)
{
    DeviceManager *dm = ctx.dm;

    dm->lDevice    = (LogicalDevice **)malloc(sizeof(*dm->lDevice));
    (*dm->lDevice) = (LogicalDevice *)malloc(sizeof(**dm->lDevice) * 1);
    dm->pDevice    = (PhysicalDevice *)malloc(sizeof(dm->pDevice));
    dm->gpuIndex    = 0;
    dm->deviceIndex = 0;

    createInstance(&dm->instance, window);
}
