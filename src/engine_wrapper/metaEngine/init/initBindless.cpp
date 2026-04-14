#include "initBindless.hpp"

extern "C"
{
#include "bindless.h"
#include "perPassLayouts.h"
}

// Initial capacities. Actual caps are clamped against device
// maxDescriptorSetUpdateAfterBind* limits inside bindlessInit.
// 1024 textures / 256 buffers covers the current scene with headroom for
// a material system; grow when a real asset pipeline lands.
static constexpr uint32_t BINDLESS_MAX_TEXTURES = 1024;
static constexpr uint32_t BINDLESS_MAX_BUFFERS  = 256;

void metaInitBindless(MetaInitContext &ctx)
{
    LogicalDevice *ld = ctx.ld;
    bindlessInit(&ld->bindless, ld->device, *ctx.pd->gpu,
                 BINDLESS_MAX_TEXTURES, BINDLESS_MAX_BUFFERS);
    perPassLayoutsCreate(&ld->perPassLayouts, ld->device);
}
