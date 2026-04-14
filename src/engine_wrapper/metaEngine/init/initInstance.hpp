#pragma once

#include "initContext.hpp"

#include <SDL2/SDL.h>

// Phase 7 — initInstance
//
// Completes the DeviceManager bootstrap for bulkInitVulkanWithScene:
// allocates the lDevice / pDevice slots, zeros gpuIndex/deviceIndex, and
// calls createInstance.
//
// TODO (Phase 8): collapse the redundancy with MetaEngine::initVkInstance —
// currently both functions allocate dm and call createInstance, which is
// wasteful at best and a double-init at worst. Phase 8 will merge them.
//
// Inputs: ctx.dm must be set (caller passes the DeviceManager from
//         this->deviceManager.getStruct()).
// Outputs: populates dm->lDevice / dm->pDevice / dm->instance.
void metaInitInstance(MetaInitContext &ctx, SDL_Window *window);
