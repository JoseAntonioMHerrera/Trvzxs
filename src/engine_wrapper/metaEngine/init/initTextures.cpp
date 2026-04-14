#include "initTextures.hpp"

extern "C"
{
#include "image.h"
#include "textureManager.h"
}

void metaInitTextures(MetaInitContext &ctx)
{
    PhysicalDevice           *pd    = ctx.pd;
    LogicalDevice            *ld    = ctx.ld;
    DisplayManager           *dispM = ctx.dispM;
    GraphicPipelineToSurface *gdp   = ctx.gdp;

    createTextureImage(pd, ld, gdp, &gdp->texture);
    createTextureImageView(dispM->graphicPipeline->swapchain,
                           ld->device, gdp->texture,
                           gdp->texture->mipLevels);
}
