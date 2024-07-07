#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include "../../include/langDefinitions/numberTypes.h"
#include "../../include/geometry/vertex.h"
#include "../device/include/deviceManagment.h"
const int MAX_FRAMES_IN_FLIGHT = 2;

// Queues for operations
/*
extern VkQueue graphicsQueue;
extern VkQueue presentQueue;
extern VkQueue transferQueue;
extern VkRenderPass renderPass;
extern VkPipelineLayout pipelineLayout;
extern VkPipeline graphicsPipeline;
extern VkSurfaceKHR surface;
extern VkPhysicalDevice *physicalDevices;
TRVZXS_SIZE physicalDevicesLen;
extern VkDevice device;

extern uint32_t graphicsQueueIndex;

extern VkSwapchainKHR swapChain;
extern VkImage* swapChainImages;
extern uint32_t swapChainImageCount;

extern VkPipelineLayout pipelineLayout;
extern VkDescriptorSetLayout descriptorSetLayout;

extern uint32_t swapChainImageCount;
extern VkFormat swapChainImageFormat;
extern VkExtent2D swapChainExtent;
extern VkImageView* swapChainImageViews;
extern uint32_t swapChainImageViewsCount;
extern VkDeviceQueueCreateInfo* queuesCreateInfo;
extern uint32_t queuesCreateInfoCount;
extern uint32_t queuesFamilyIndexes[];
*/

/*
extern VkPipelineShaderStageCreateInfo shaderStages[2];
extern VkPipelineDynamicStateCreateInfo dynamicState;
extern VkPipelineVertexInputStateCreateInfo vertexInputInfo;
extern VkPipelineInputAssemblyStateCreateInfo inputAssembly;
extern VkPipelineViewportStateCreateInfo viewportState;
extern VkPipelineRasterizationStateCreateInfo rasterizer;
extern VkPipelineMultisampleStateCreateInfo multisampling;
extern VkPipelineColorBlendAttachmentState colorBlendAttachment;
extern VkPipelineColorBlendStateCreateInfo colorBlending;
extern VkFramebuffer* swapChainFramebuffers;
extern uint32_t swapChainFramebuffersCount;
extern VkCommandPool graphicsCommandPool;
extern VkCommandPool transferCommandPool;
extern VkCommandBuffer* commandBuffers;
extern uint32_t commandBuffersCount;
extern VkSemaphore* imageAvailableSemaphore;
extern VkSemaphore* renderFinishedSemaphore;
extern
extern VkSubpassDependency dependency;
extern VkSurfaceFormatKHR selectedSurfaceFormat;
extern VkDebugUtilsMessengerEXT* logCallback;
extern VkInstance instance;
extern uint32_t currentFrame;
extern VkShaderModule vertShaderModule;
extern VkShaderModule fragShaderModule;
extern SDL_Window* sdlWindow;
extern VkExtent2D actualExtent;
extern VkVertexInputAttributeDescription objectsIAD[2];
extern uint32_t objectsIADCount;
extern VkVertexInputBindingDescription objectIBD;

extern VkBuffer geometryBuffer;
extern VkDeviceMemory geometryBufferMemory;
extern VkBuffer indexBuffer;
extern VkDeviceMemory indexBufferMemory;
extern VkBuffer stagingBuffer;
extern VkDeviceMemory stagingBufferMemory;



extern VkDevice device;
extern VkQueue graphicsQueue;
extern VkSurfaceKHR surface;*/

typedef struct EngineHut {
    DeviceManager * dm; // DeviceManager (physical and logical) with related fields for memory types, queues available, etc...
    uint32_t deviceBufferLen;

    VkInstance* instance;
    VkDebugUtilsMessengerEXT* logCallback;
    SDL_Window *window; //TODO: move to an agonstic library window manager for the engine.
} EngineHut;

struct EngineHut *engineHut;


void initVulkan(SDL_Window* window);
void destroyVulkanSurface(const VkInstance* instance, VkSurfaceKHR* surface);
void destroyVulkanInstance(const VkInstance* instance);
void destroyDebugCallbackForVulkan(VkInstance* instance, VkDebugUtilsMessengerEXT* callback_messenger);
void createGraphicsPipeline();
void createRenderPass();
void drawFrame();
void cleanup();
