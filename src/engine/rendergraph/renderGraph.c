#include "renderGraph.h"
#include "physicalDevice.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void rgInit(RenderGraph* rg, VkDevice device, PhysicalDevice* pd,
            uint32_t swapchainWidth, uint32_t swapchainHeight,
            uint32_t swapchainImageCount)
{
    memset(rg, 0, sizeof(RenderGraph));
    rg->device              = device;
    rg->physicalDevice      = pd;
    rg->swapchainWidth      = swapchainWidth;
    rg->swapchainHeight     = swapchainHeight;
    rg->swapchainImageCount = swapchainImageCount;
    rg->isCompiled          = false;
}

void rgDestroy(RenderGraph* rg)
{
    for (uint32_t p = 0; p < rg->passCount; p++)
    {
        RGPass* pass = &rg->passes[p];
        if (pass->type == RG_PASS_GRAPHICS)
        {
            if (pass->framebuffers)
            {
                for (uint32_t i = 0; i < rg->swapchainImageCount; i++)
                {
                    if (pass->framebuffers[i] != VK_NULL_HANDLE)
                        vkDestroyFramebuffer(rg->device, pass->framebuffers[i], NULL);
                }
                free(pass->framebuffers);
                pass->framebuffers = NULL;
            }
            if (pass->renderPass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(rg->device, pass->renderPass, NULL);
                pass->renderPass = VK_NULL_HANDLE;
            }
        }
    }

    // Destroy transient resources
    for (uint32_t r = 0; r < rg->resourceCount; r++)
    {
        RGResource* res = &rg->resources[r];
        if (!res->isActive || res->isExternal || res->type != RG_IMAGE)
            continue;
        if (res->imageView != VK_NULL_HANDLE)
            vkDestroyImageView(rg->device, res->imageView, NULL);
        if (res->image != VK_NULL_HANDLE)
            vkDestroyImage(rg->device, res->image, NULL);
        if (res->memory != VK_NULL_HANDLE)
            vkFreeMemory(rg->device, res->memory, NULL);
    }
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

uint32_t rgAddResource(RenderGraph* rg, RGResource desc)
{
    assert(rg->resourceCount < RG_MAX_RESOURCES);
    uint32_t index = rg->resourceCount++;

    rg->resources[index]                 = desc;
    rg->resources[index].isActive        = true;
    rg->resources[index].lastWriterPass  = UINT32_MAX;
    rg->resources[index].currentLayout   = VK_IMAGE_LAYOUT_UNDEFINED;

    return index;
}

uint32_t rgAddExternalResource(RenderGraph* rg, RGResource desc)
{
    assert(desc.isExternal);
    assert(desc.type != RG_IMAGE || (desc.image != VK_NULL_HANDLE && desc.imageView != VK_NULL_HANDLE));
    assert(desc.type != RG_BUFFER || desc.buffer != VK_NULL_HANDLE);

    return rgAddResource(rg, desc);
}

uint32_t rgAddPass(RenderGraph* rg, RGPassType type)
{
    assert(rg->passCount < RG_MAX_PASSES);
    uint32_t index = rg->passCount++;

    memset(&rg->passes[index], 0, sizeof(RGPass));
    rg->passes[index].type     = type;
    rg->passes[index].isActive = true;

    return index;
}

void rgPassAddInput(RenderGraph* rg, uint32_t passIndex, RGResourceUsage usage)
{
    RGPass* pass = &rg->passes[passIndex];
    assert(pass->inputCount < RG_MAX_RESOURCE_USAGES);
    pass->inputs[pass->inputCount++] = usage;
}

void rgPassAddOutput(RenderGraph* rg, uint32_t passIndex, RGResourceUsage usage)
{
    RGPass* pass = &rg->passes[passIndex];
    assert(pass->outputCount < RG_MAX_RESOURCE_USAGES);
    pass->outputs[pass->outputCount++] = usage;
}

void rgPassSetCallback(RenderGraph* rg, uint32_t passIndex, RGPassExecuteFn fn, void* userData)
{
    rg->passes[passIndex].execute  = fn;
    rg->passes[passIndex].userData = userData;
}

// ---------------------------------------------------------------------------
// Topological sort (Kahn's algorithm)
// ---------------------------------------------------------------------------

static void rgTopologicalSort(RenderGraph* rg)
{
    // For each resource, find which pass writes it
    uint32_t writerOf[RG_MAX_RESOURCES];
    memset(writerOf, 0xFF, sizeof(writerOf));

    for (uint32_t p = 0; p < rg->passCount; p++)
    {
        for (uint32_t o = 0; o < rg->passes[p].outputCount; o++)
        {
            uint32_t resIdx  = rg->passes[p].outputs[o].resourceIndex;
            writerOf[resIdx] = p;
        }
    }

    // Build in-degree and adjacency
    uint32_t inDegree[RG_MAX_PASSES];
    memset(inDegree, 0, sizeof(inDegree));

    uint32_t adj[RG_MAX_PASSES][RG_MAX_PASSES];
    uint32_t adjCount[RG_MAX_PASSES];
    memset(adjCount, 0, sizeof(adjCount));

    for (uint32_t p = 0; p < rg->passCount; p++)
    {
        for (uint32_t i = 0; i < rg->passes[p].inputCount; i++)
        {
            uint32_t resIdx = rg->passes[p].inputs[i].resourceIndex;
            uint32_t writer = writerOf[resIdx];

            if (writer != UINT32_MAX && writer != p)
            {
                // Check for duplicate edges
                bool duplicate = false;
                for (uint32_t a = 0; a < adjCount[writer]; a++)
                {
                    if (adj[writer][a] == p)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                {
                    adj[writer][adjCount[writer]++] = p;
                    inDegree[p]++;
                }
            }
        }
    }

    // BFS from passes with no incoming edges
    uint32_t queue[RG_MAX_PASSES];
    uint32_t qHead = 0, qTail = 0;

    for (uint32_t p = 0; p < rg->passCount; p++)
    {
        if (inDegree[p] == 0)
            queue[qTail++] = p;
    }

    rg->executionOrderCount = 0;

    while (qHead < qTail)
    {
        uint32_t current                                 = queue[qHead++];
        rg->executionOrder[rg->executionOrderCount]      = current;
        rg->passes[current].executionOrder               = rg->executionOrderCount;
        rg->executionOrderCount++;

        for (uint32_t a = 0; a < adjCount[current]; a++)
        {
            uint32_t next = adj[current][a];
            inDegree[next]--;
            if (inDegree[next] == 0)
                queue[qTail++] = next;
        }
    }

    assert(rg->executionOrderCount == rg->passCount && "Render graph has a dependency cycle!");
}

// ---------------------------------------------------------------------------
// Barrier generation
// ---------------------------------------------------------------------------

static void rgComputeBarriers(RenderGraph* rg)
{
    // Reset resource tracking
    for (uint32_t r = 0; r < rg->resourceCount; r++)
    {
        rg->resources[r].currentLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        rg->resources[r].lastWriterPass = UINT32_MAX;
    }

    for (uint32_t orderIdx = 0; orderIdx < rg->executionOrderCount; orderIdx++)
    {
        uint32_t passIdx = rg->executionOrder[orderIdx];
        RGPass*  pass    = &rg->passes[passIdx];

        pass->imageBarrierCount   = 0;
        pass->bufferBarrierCount  = 0;
        pass->barrierSrcStageMask = 0;
        pass->barrierDstStageMask = 0;

        // Check each input: does the resource need a layout transition?
        for (uint32_t i = 0; i < pass->inputCount; i++)
        {
            RGResourceUsage* usage = &pass->inputs[i];
            RGResource*      res   = &rg->resources[usage->resourceIndex];

            if (res->type == RG_IMAGE && res->currentLayout != usage->layout)
            {
                VkImageMemoryBarrier barrier = {
                    .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .oldLayout           = res->currentLayout,
                    .newLayout           = usage->layout,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image               = res->image,
                    .subresourceRange    = {
                        .aspectMask     = res->aspectMask,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                };

                if (res->lastWriterPass != UINT32_MAX)
                {
                    RGPass* writerPass = &rg->passes[res->lastWriterPass];
                    for (uint32_t o = 0; o < writerPass->outputCount; o++)
                    {
                        if (writerPass->outputs[o].resourceIndex == usage->resourceIndex)
                        {
                            barrier.srcAccessMask = writerPass->outputs[o].accessMask;
                            pass->barrierSrcStageMask |= writerPass->outputs[o].stageMask;
                            break;
                        }
                    }
                }
                else
                {
                    barrier.srcAccessMask = 0;
                    pass->barrierSrcStageMask |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                }

                barrier.dstAccessMask = usage->accessMask;
                pass->barrierDstStageMask |= usage->stageMask;

                assert(pass->imageBarrierCount < RG_MAX_BARRIERS_PER_PASS);
                pass->imageBarriers[pass->imageBarrierCount++] = barrier;

                res->currentLayout = usage->layout;
            }
        }

        // Update tracking for outputs
        for (uint32_t o = 0; o < pass->outputCount; o++)
        {
            RGResourceUsage* usage = &pass->outputs[o];
            RGResource*      res   = &rg->resources[usage->resourceIndex];

            // Snapshot current layout before this pass overwrites it —
            // used by render pass creation to derive loadOp and initialLayout
            usage->resolvedInitialLayout = res->currentLayout;

            res->currentLayout  = usage->layout;
            res->lastWriterPass = passIdx;
        }
    }
}

// ---------------------------------------------------------------------------
// Render pass creation (per graphics pass)
// ---------------------------------------------------------------------------

static void rgCreateRenderPassForPass(RenderGraph* rg, RGPass* pass)
{
    VkAttachmentDescription2 attachments[RG_MAX_RESOURCE_USAGES * 2];
    VkAttachmentReference2   colorRefs[RG_MAX_RESOURCE_USAGES];
    VkAttachmentReference2   inputRefs[RG_MAX_RESOURCE_USAGES];
    VkAttachmentReference2   depthRef;
    uint32_t                 attachmentCount = 0;
    uint32_t                 colorRefCount   = 0;
    uint32_t                 inputRefCount   = 0;
    bool                     hasDepth        = false;

    // Map resource index → attachment index
    uint32_t resIdxToAttIdx[RG_MAX_RESOURCES];
    memset(resIdxToAttIdx, 0xFF, sizeof(resIdxToAttIdx));

    // Process outputs first (they define attachment descriptions)
    for (uint32_t o = 0; o < pass->outputCount; o++)
    {
        RGResourceUsage* usage = &pass->outputs[o];
        RGResource*      res   = &rg->resources[usage->resourceIndex];
        if (res->type != RG_IMAGE)
            continue;

        uint32_t attIdx                      = attachmentCount++;
        resIdxToAttIdx[usage->resourceIndex] = attIdx;

        bool isDepth = (res->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;

        bool               firstWriteThisFrame = (usage->resolvedInitialLayout == VK_IMAGE_LAYOUT_UNDEFINED);
        VkAttachmentLoadOp loadOp     = firstWriteThisFrame ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        VkImageLayout      initLayout = firstWriteThisFrame ? VK_IMAGE_LAYOUT_UNDEFINED : usage->resolvedInitialLayout;

        attachments[attIdx] = (VkAttachmentDescription2){
            .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
            .format         = res->format,
            .samples        = res->samples,
            .loadOp         = loadOp,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = initLayout,
            .finalLayout    = usage->layout,
        };

        if (isDepth)
        {
            hasDepth = true;
            depthRef = (VkAttachmentReference2){
                .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = attIdx,
                .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            };
        }
        else
        {
            colorRefs[colorRefCount++] = (VkAttachmentReference2){
                .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = attIdx,
                .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };
        }
    }

    // Process inputs (input attachments for deferred reads)
    for (uint32_t i = 0; i < pass->inputCount; i++)
    {
        RGResourceUsage* usage = &pass->inputs[i];
        RGResource*      res   = &rg->resources[usage->resourceIndex];
        if (res->type != RG_IMAGE)
            continue;

        if (resIdxToAttIdx[usage->resourceIndex] == UINT32_MAX)
        {
            uint32_t attIdx                      = attachmentCount++;
            resIdxToAttIdx[usage->resourceIndex] = attIdx;

            attachments[attIdx] = (VkAttachmentDescription2){
                .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                .format         = res->format,
                .samples        = res->samples,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = usage->layout,
                .finalLayout    = usage->layout,
            };
        }

        if (usage->accessMask & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)
        {
            inputRefs[inputRefCount++] = (VkAttachmentReference2){
                .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = resIdxToAttIdx[usage->resourceIndex],
                .layout     = usage->layout,
                .aspectMask = res->aspectMask,
            };
        }
    }

    VkSubpassDescription2 subpass = {
        .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = colorRefCount,
        .pColorAttachments       = colorRefs,
        .inputAttachmentCount    = inputRefCount,
        .pInputAttachments       = inputRefCount > 0 ? inputRefs : NULL,
        .pDepthStencilAttachment = hasDepth ? &depthRef : NULL,
    };

    VkSubpassDependency2 dependency = {
        .sType         = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    };

    VkRenderPassCreateInfo2 rpInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .attachmentCount = attachmentCount,
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };

    VkResult result = vkCreateRenderPass2(rg->device, &rpInfo, NULL, &pass->renderPass);
    assert(result == VK_SUCCESS);
}

// ---------------------------------------------------------------------------
// Framebuffer creation (per graphics pass)
// ---------------------------------------------------------------------------

static void rgCreateFramebuffersForPass(RenderGraph* rg, RGPass* pass)
{
    VkImageView imageViews[RG_MAX_RESOURCE_USAGES * 2];
    uint32_t    imageViewCount = 0;

    // Outputs first (same order as rgCreateRenderPassForPass)
    for (uint32_t o = 0; o < pass->outputCount; o++)
    {
        RGResource* res = &rg->resources[pass->outputs[o].resourceIndex];
        if (res->type == RG_IMAGE)
            imageViews[imageViewCount++] = res->imageView;
    }

    // Then inputs not already in outputs
    for (uint32_t i = 0; i < pass->inputCount; i++)
    {
        RGResource* res = &rg->resources[pass->inputs[i].resourceIndex];
        if (res->type != RG_IMAGE)
            continue;

        bool alreadyAdded = false;
        for (uint32_t o = 0; o < pass->outputCount; o++)
        {
            if (pass->outputs[o].resourceIndex == pass->inputs[i].resourceIndex)
            {
                alreadyAdded = true;
                break;
            }
        }
        if (!alreadyAdded)
            imageViews[imageViewCount++] = res->imageView;
    }

    pass->framebuffers = malloc(sizeof(VkFramebuffer) * rg->swapchainImageCount);

    for (uint32_t i = 0; i < rg->swapchainImageCount; i++)
    {
        VkFramebufferCreateInfo fbInfo = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = pass->renderPass,
            .attachmentCount = imageViewCount,
            .pAttachments    = imageViews,
            .width           = rg->swapchainWidth,
            .height          = rg->swapchainHeight,
            .layers          = 1,
        };

        VkResult result = vkCreateFramebuffer(rg->device, &fbInfo, NULL, &pass->framebuffers[i]);
        assert(result == VK_SUCCESS);
    }
}

// ---------------------------------------------------------------------------
// Compile
// ---------------------------------------------------------------------------

void rgCompileSortAndBarriers(RenderGraph* rg)
{
    rgTopologicalSort(rg);
    rgComputeBarriers(rg);
}

void rgCompile(RenderGraph* rg)
{
    // Step 1: Create transient image resources (not used in Phase 2 — all external)
    for (uint32_t r = 0; r < rg->resourceCount; r++)
    {
        RGResource* res = &rg->resources[r];
        if (!res->isActive || res->isExternal || res->type != RG_IMAGE)
            continue;
        if (res->image != VK_NULL_HANDLE)
            continue;

        uint32_t width  = (res->sizeMode == RG_SIZE_SWAPCHAIN) ? rg->swapchainWidth : res->width;
        uint32_t height = (res->sizeMode == RG_SIZE_SWAPCHAIN) ? rg->swapchainHeight : res->height;

        // Accumulate usage flags from all passes
        VkImageUsageFlags usageFlags = 0;
        for (uint32_t p = 0; p < rg->passCount; p++)
        {
            for (uint32_t o = 0; o < rg->passes[p].outputCount; o++)
            {
                if (rg->passes[p].outputs[o].resourceIndex == r)
                    usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            for (uint32_t i = 0; i < rg->passes[p].inputCount; i++)
            {
                if (rg->passes[p].inputs[i].resourceIndex == r)
                    usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }
        }

        VkImageCreateInfo imgInfo = {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType     = VK_IMAGE_TYPE_2D,
            .extent        = {width, height, 1},
            .mipLevels     = 1,
            .arrayLayers   = 1,
            .format        = res->format,
            .tiling        = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage         = usageFlags,
            .samples       = res->samples,
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        };

        vkCreateImage(rg->device, &imgInfo, NULL, &res->image);

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(rg->device, res->image, &memReqs);

        // Find device-local memory type
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(*rg->physicalDevice->gpu, &memProps);
        uint32_t memTypeIndex = UINT32_MAX;
        for (uint32_t m = 0; m < memProps.memoryTypeCount; m++)
        {
            if ((memReqs.memoryTypeBits & (1 << m)) &&
                (memProps.memoryTypes[m].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            {
                memTypeIndex = m;
                break;
            }
        }
        assert(memTypeIndex != UINT32_MAX);

        VkMemoryAllocateInfo allocInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memReqs.size,
            .memoryTypeIndex = memTypeIndex,
        };

        vkAllocateMemory(rg->device, &allocInfo, NULL, &res->memory);
        vkBindImageMemory(rg->device, res->image, res->memory, 0);

        VkImageViewCreateInfo viewInfo = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = res->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = res->format,
            .subresourceRange = {
                .aspectMask     = res->aspectMask,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };

        vkCreateImageView(rg->device, &viewInfo, NULL, &res->imageView);
    }

    // Step 2: Topological sort + barriers
    rgCompileSortAndBarriers(rg);

    // Step 3: Create render passes and framebuffers for graphics passes
    for (uint32_t p = 0; p < rg->passCount; p++)
    {
        if (rg->passes[p].type == RG_PASS_GRAPHICS)
        {
            rgCreateRenderPassForPass(rg, &rg->passes[p]);
            rgCreateFramebuffersForPass(rg, &rg->passes[p]);
        }
    }

    rg->isCompiled = true;
}

// ---------------------------------------------------------------------------
// Execute (per-frame)
// ---------------------------------------------------------------------------

void rgExecute(RenderGraph* rg, VkCommandBuffer cmd, uint32_t frameIndex)
{
    assert(rg->isCompiled);

    for (uint32_t orderIdx = 0; orderIdx < rg->executionOrderCount; orderIdx++)
    {
        uint32_t passIdx = rg->executionOrder[orderIdx];
        RGPass*  pass    = &rg->passes[passIdx];

        // Update barrier image handles for external resources whose VkImage
        // changes per frame (e.g. swapchain color attachment)
        for (uint32_t b = 0; b < pass->imageBarrierCount; b++)
        {
            uint32_t resIdx = UINT32_MAX;

            // Find which resource this barrier belongs to by matching the stored image
            // against input resource images. For external resources whose image changes
            // per frame, patch the barrier with the current handle.
            for (uint32_t i = 0; i < pass->inputCount; i++)
            {
                RGResource* res = &rg->resources[pass->inputs[i].resourceIndex];
                if (res->type == RG_IMAGE && res->isExternal)
                {
                    // Match by resource index: barrier was created for this input
                    // in the same order during rgComputeBarriers
                    pass->imageBarriers[b].image = res->image;
                }
            }
        }

        // Insert pre-pass barriers
        if (pass->imageBarrierCount > 0 || pass->bufferBarrierCount > 0)
        {
            vkCmdPipelineBarrier(
                cmd,
                pass->barrierSrcStageMask ? pass->barrierSrcStageMask : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                pass->barrierDstStageMask ? pass->barrierDstStageMask : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0, NULL,
                pass->bufferBarrierCount, pass->bufferBarriers,
                pass->imageBarrierCount, pass->imageBarriers);
        }

        if (pass->type == RG_PASS_GRAPHICS)
        {
            // Build clear values per attachment
            VkClearValue clearValues[RG_MAX_RESOURCE_USAGES * 2];
            uint32_t     clearCount = 0;

            for (uint32_t o = 0; o < pass->outputCount; o++)
            {
                RGResource* res = &rg->resources[pass->outputs[o].resourceIndex];
                if (res->type != RG_IMAGE)
                    continue;

                if (res->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
                    clearValues[clearCount++] = (VkClearValue){.depthStencil = {1.0f, 0}};
                else
                    clearValues[clearCount++] = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
            }

            // Input-only attachments
            for (uint32_t i = 0; i < pass->inputCount; i++)
            {
                bool isAlsoOutput = false;
                for (uint32_t o = 0; o < pass->outputCount; o++)
                {
                    if (pass->outputs[o].resourceIndex == pass->inputs[i].resourceIndex)
                    {
                        isAlsoOutput = true;
                        break;
                    }
                }
                if (!isAlsoOutput)
                {
                    RGResource* res = &rg->resources[pass->inputs[i].resourceIndex];
                    if (res->type == RG_IMAGE)
                        clearValues[clearCount++] = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
                }
            }

            VkRenderPassBeginInfo rpBegin = {
                .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass        = pass->renderPass,
                .framebuffer       = pass->framebuffers[frameIndex],
                .renderArea.offset = {0, 0},
                .renderArea.extent = {rg->swapchainWidth, rg->swapchainHeight},
                .clearValueCount   = clearCount,
                .pClearValues      = clearValues,
            };

            vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

            if (pass->execute)
                pass->execute(cmd, pass, pass->userData);

            vkCmdEndRenderPass(cmd);
        }
        else if (pass->type == RG_PASS_COMPUTE)
        {
            if (pass->execute)
                pass->execute(cmd, pass, pass->userData);
        }
    }
}

// ---------------------------------------------------------------------------
// Swapchain resize
// ---------------------------------------------------------------------------

void rgOnSwapchainResize(RenderGraph* rg, uint32_t newWidth, uint32_t newHeight)
{
    // Destroy transient resources
    for (uint32_t r = 0; r < rg->resourceCount; r++)
    {
        RGResource* res = &rg->resources[r];
        if (!res->isActive || res->isExternal)
            continue;
        if (res->type == RG_IMAGE && res->sizeMode == RG_SIZE_SWAPCHAIN)
        {
            if (res->imageView != VK_NULL_HANDLE)
                vkDestroyImageView(rg->device, res->imageView, NULL);
            if (res->image != VK_NULL_HANDLE)
                vkDestroyImage(rg->device, res->image, NULL);
            if (res->memory != VK_NULL_HANDLE)
                vkFreeMemory(rg->device, res->memory, NULL);
            res->imageView = VK_NULL_HANDLE;
            res->image     = VK_NULL_HANDLE;
            res->memory    = VK_NULL_HANDLE;
        }
    }

    // Destroy framebuffers and render passes
    for (uint32_t p = 0; p < rg->passCount; p++)
    {
        RGPass* pass = &rg->passes[p];
        if (pass->type == RG_PASS_GRAPHICS)
        {
            if (pass->framebuffers)
            {
                for (uint32_t i = 0; i < rg->swapchainImageCount; i++)
                    vkDestroyFramebuffer(rg->device, pass->framebuffers[i], NULL);
                free(pass->framebuffers);
                pass->framebuffers = NULL;
            }
            if (pass->renderPass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(rg->device, pass->renderPass, NULL);
                pass->renderPass = VK_NULL_HANDLE;
            }
        }
    }

    rg->swapchainWidth  = newWidth;
    rg->swapchainHeight = newHeight;
    rg->isCompiled      = false;

    rgCompile(rg);
}
