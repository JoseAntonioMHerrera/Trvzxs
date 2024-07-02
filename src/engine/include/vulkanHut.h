#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include "../../include/langDefinitions/numberTypes.h"

#pragma once

#include "../device/include/deviceManagment.h"
const int MAX_FRAMES_IN_FLIGHT = 2;

// Queues for operations
/*extern VkQueue graphicsQueue;
VkQueue presentQueue;
VkQueue transferQueue;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;

uint32_t graphicsQueueIndex;

VkSwapchainKHR swapChain;
VkImage* swapChainImages;
uint32_t swapChainImageCount;


VkPipelineLayout pipelineLayout;
VkDescriptorSetLayout descriptorSetLayout;

uint32_t swapChainImageCount;
VkFormat swapChainImageFormat = VK_FORMAT_R8G8B8_SRGB;
VkExtent2D swapChainExtent;
VkImageView* swapChainImageViews;
uint32_t swapChainImageViewsCount;
VkDeviceQueueCreateInfo* queuesCreateInfo;
uint32_t queuesCreateInfoCount = 2;
uint32_t queuesFamilyIndexes[] = {0, 3}; // Harcoded indexes representing the specified index of each queue in the pool of queues
typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    VkPresentModeKHR* presentModes;
} SwapChainSupportDetails;
VkPipelineShaderStageCreateInfo shaderStages[2];
VkPipelineDynamicStateCreateInfo dynamicState;
VkPipelineVertexInputStateCreateInfo vertexInputInfo;
VkPipelineInputAssemblyStateCreateInfo inputAssembly;
VkPipelineViewportStateCreateInfo viewportState;
VkPipelineRasterizationStateCreateInfo rasterizer;
VkPipelineMultisampleStateCreateInfo multisampling;
VkPipelineColorBlendAttachmentState colorBlendAttachment;
VkPipelineColorBlendStateCreateInfo colorBlending;
VkFramebuffer* swapChainFramebuffers;
uint32_t swapChainFramebuffersCount = 0;
VkCommandPool graphicsCommandPool;
VkCommandPool transferCommandPool;
VkCommandBuffer* commandBuffers;
uint32_t commandBuffersCount;
VkSemaphore* imageAvailableSemaphore;
VkSemaphore* renderFinishedSemaphore;
VkFence* inFlightFence;
VkSubpassDependency dependency;
VkSurfaceFormatKHR selectedSurfaceFormat;
VkDebugUtilsMessengerEXT* logCallback;
VkInstance instance;
uint32_t currentFrame = 0;
VkShaderModule vertShaderModule;
VkShaderModule fragShaderModule;
SDL_Window* sdlWindow;
QueueFamilyIndices queuesIndices = {
    .graphicsFamily = UINT32_MAX,
    .presentFamily = UINT32_MAX,
    .transferFamily = UINT32_MAX,
};
VkExtent2D actualExtent;
VkVertexInputAttributeDescription objectsIAD[2];
uint32_t objectsIADCount = 2;
VkVertexInputBindingDescription objectIBD;

// VkBuffers
VkBuffer geometryBuffer;
VkDeviceMemory geometryBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
VkBuffer stagingBuffer;
VkDeviceMemory stagingBufferMemory;

// GPU queues for drawing (ideally with presentation support), transfer and compute tasks
struct QueueFamilyIndices {
    uint32_t graphicsFamily; // VK_QUEUE_GRAPHICS_BIT
    uint32_t presentFamily; // No specific flag vkGetPhysicalDeviceSurfaceSupportKHR
    uint32_t transferFamily; // VK_QUEUE_TRANSFER_BIT
};

typedef struct QueueFamilyIndices QueueFamilyIndices;

extern VkDevice device;
extern VkQueue graphicsQueue;
extern VkSurfaceKHR surface;
void initVulkan(SDL_Window* window);
void destroyVulkanSurface(const VkInstance* instance, VkSurfaceKHR* surface);
void destroyVulkanInstance(const VkInstance* instance);
void destroyDebugCallbackForVulkan(VkInstance* instance, VkDebugUtilsMessengerEXT* callback_messenger);
void createGraphicsPipeline();
void createRenderPass();
void drawFrame();
void cleanup();*/

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

typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    VkPresentModeKHR* presentModes;
} SwapChainSupportDetails;

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
extern VkFence* inFlightFence;
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
extern VkSurfaceKHR surface;


void initVulkan(SDL_Window* window);
void destroyVulkanSurface(const VkInstance* instance, VkSurfaceKHR* surface);
void destroyVulkanInstance(const VkInstance* instance);
void destroyDebugCallbackForVulkan(VkInstance* instance, VkDebugUtilsMessengerEXT* callback_messenger);
void createGraphicsPipeline();
void createRenderPass();
void drawFrame();
void cleanup();
