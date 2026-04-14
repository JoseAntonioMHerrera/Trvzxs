#include "perPassLayouts.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static VkDescriptorSetLayout createLayout(VkDevice device,
                                          const VkDescriptorSetLayoutBinding* bindings,
                                          uint32_t count)
{
    const VkDescriptorSetLayoutCreateInfo info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = count,
        .pBindings    = bindings,
    };
    VkDescriptorSetLayout out = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device, &info, NULL, &out) != VK_SUCCESS)
    {
        fprintf(stderr, "perPassLayoutsCreate: vkCreateDescriptorSetLayout failed\n");
        exit(1);
    }
    return out;
}

void perPassLayoutsCreate(PerPassLayouts* pp, VkDevice device)
{
    assert(pp && device);
    pp->device = device;

    // Model + depth prepass: same shape, single camera/frame-globals UBO.
    // Stage mask conservative: vertex for transforms math path, fragment for
    // eye position consumers (specular, depth reconstruction).
    const VkDescriptorSetLayoutBinding frameGlobalsOnly[] = {
        {
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    pp->modelPass        = createLayout(device, frameGlobalsOnly, 1);
    pp->depthPrepassPass = createLayout(device, frameGlobalsOnly, 1);

    // Light pass: frame globals + two subpass input attachments (geometry,
    // normals). Input attachments must stay in a descriptor set — they
    // cannot go bindless.
    const VkDescriptorSetLayoutBinding lightBindings[] = {
        {
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding         = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding         = 2,
            .descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    pp->lightPass = createLayout(device, lightBindings, 3);

    // Particle compute: delta time UBO only. Particle SSBOs go bindless.
    const VkDescriptorSetLayoutBinding computeBindings[] = {
        {
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    pp->particleComputePass = createLayout(device, computeBindings, 1);
}

void perPassLayoutsDestroy(PerPassLayouts* pp)
{
    if (!pp || !pp->device) return;
    if (pp->modelPass)           vkDestroyDescriptorSetLayout(pp->device, pp->modelPass, NULL);
    if (pp->depthPrepassPass)    vkDestroyDescriptorSetLayout(pp->device, pp->depthPrepassPass, NULL);
    if (pp->lightPass)           vkDestroyDescriptorSetLayout(pp->device, pp->lightPass, NULL);
    if (pp->particleComputePass) vkDestroyDescriptorSetLayout(pp->device, pp->particleComputePass, NULL);
    memset(pp, 0, sizeof(*pp));
}
