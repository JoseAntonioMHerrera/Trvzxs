#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <stdint.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include <assert.h>
#include "string_helper_macros.h"
#include "comparation_helper_macros.h"
#include <unistd.h>
#include <vulkan/vulkan_core.h>
#include "file_functions.h"
#include "vertex.h"
#include <vulkan/vulkan_core.h>
#include "strictDebug.h"
#include "main_vulkan.h"
#include "cglm.h"
#include "graphicsPipeline.h"
#include "commandPool.h"
#include "displayManager.h"
#include "swapchainManager.h"
#include "uniformBuffer.h"
#include "descriptor.h"
#include "textureManager.h"
#include "primitives.h"
#include "scene.h"

void destroyDebugCallbackForVulkan(VkInstance *instance, VkDebugUtilsMessengerEXT *callback_messenger)
{
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = VK_NULL_HANDLE;
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT\0");
    assert(vkDestroyDebugUtilsMessengerEXT != NULL);

    vkDestroyDebugUtilsMessengerEXT(*instance, *callback_messenger, NULL);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{

    // Get severity level as string
    const char *severity = "UNKNOWN";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        severity = "VERBOSE";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        severity = "INFO";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = "WARNING";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severity = "ERROR";

    // Feature detection
    if (strstr(pCallbackData->pMessage, "GPU-Assisted") != NULL)
    {
        printf("[FEATURE DETECT] GPU-Assisted Validation is active | %s: %s\n",
               severity, pCallbackData->pMessage);
    }
    if (strstr(pCallbackData->pMessage, "Synchronization Validation") != NULL)
    {
        printf("[FEATURE DETECT] Synchronization Validation is active | %s: %s\n",
               severity, pCallbackData->pMessage);
    }
    if (strstr(pCallbackData->pMessage, "Best Practices") != NULL)
    {
        printf("[FEATURE DETECT] Best Practices Validation is active | %s: %s\n",
               severity, pCallbackData->pMessage);
    }

    // General message output with formatting based on severity
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        fprintf(stderr, "[%s] %s | msgIDName: %s | msgIDNumber: %d | message: %s\n",
                severity,
                messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ? "Validation" : "Performance",
                pCallbackData->pMessageIdName,
                pCallbackData->messageIdNumber,
                pCallbackData->pMessage);
    }

    // More deep checks and analysis here

    // Based on the data received, choose to halt or not halt program
    return VK_FALSE;
};

VkDebugUtilsMessengerEXT *createDebugCallbackForVulkan(const VkInstance *instance)
{
    VkDebugUtilsMessengerCreateInfoEXT debug_callback_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &vulkanDebugCallback,
        .pUserData = NULL,
    };
    VkDebugUtilsMessengerEXT *callback_messenger = NULL;
    callback_messenger = malloc(sizeof(VkDebugUtilsMessengerEXT));
    assert(callback_messenger != NULL);

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = VK_NULL_HANDLE;
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT\0");
    assert(vkCreateDebugUtilsMessengerEXT != NULL);

    VkResult messenger_creation_result = vkCreateDebugUtilsMessengerEXT(*instance, &debug_callback_info, NULL, callback_messenger);
    assert(messenger_creation_result == VK_SUCCESS);

    return callback_messenger;
}

void cleanup(DeviceManager *dm, LogicalDevice *ld, DisplayManager *dispM, GraphicPipelineToSurface *gdp, VkDebugUtilsMessengerEXT *logCallback)
{
    vkDeviceWaitIdle(dm->lDevice[dm->deviceIndex]->device);
    cleanupSwapChain(dm->lDevice[dm->deviceIndex], ld->displayManager);
    vkDestroyBuffer(ld->device, gdp->deviceBuffer->buffer, NULL);
    vkDestroyBuffer(ld->device, gdp->deviceBuffer->indexBuffer, NULL);
    vkFreeMemory(ld->device, gdp->deviceBuffer->bufferMemory, NULL);
    vkFreeMemory(ld->device, gdp->deviceBuffer->indexBufferMemory, NULL);
    vkDestroyRenderPass(ld->device, gdp->renderPass, NULL);
    vkDestroyCommandPool(ld->device, gdp->commandPool->graphicsCommandPool, NULL);
    vkDestroyCommandPool(ld->device, gdp->commandPool->transferCommandPool, NULL);
    vkDestroyPipeline(ld->device, gdp->pipeline, NULL);
    vkDestroyDescriptorSetLayout(ld->device, gdp->dsl, NULL);
    vkDestroyPipelineLayout(ld->device, gdp->pipelineLayout, NULL);
    cleanupSyncObjects(ld->device, gdp);

    // vkDestroyPipelineLayout(device, pipelineLayout, NULL);

    vkDestroyDevice(ld->device, NULL);

    if (logCallback)
    {
        destroyDebugCallbackForVulkan(&dm->instance, dm->logCallback);
    }

    vkDestroySurfaceKHR(dm->instance, dispM->surface, NULL);
    vkDestroyInstance(dm->instance, NULL);
}

void createInstance(VkInstance **instance, SDL_Window *window)
{
    *instance = (VkInstance *)malloc(sizeof(VkInstance));

    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "GrassBox\0",
        .applicationVersion = 0,
        .pEngineName = "GrassBox\0",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    uint32_t available_instance_layers_count = 0;
    VkLayerProperties *vulkan_instance_layer_properties;

    VkResult vkEnumInstLayerPropsResult = vkEnumerateInstanceLayerProperties(&available_instance_layers_count, VK_NULL_HANDLE);
    assert(vkEnumInstLayerPropsResult == VK_SUCCESS && available_instance_layers_count > 0);
    vulkan_instance_layer_properties = malloc(sizeof(VkLayerProperties) * available_instance_layers_count);

    VkResult vkEnumInstLayPropResult = vkEnumerateInstanceLayerProperties(&available_instance_layers_count, vulkan_instance_layer_properties);
    assert(vkEnumInstLayPropResult == VK_SUCCESS);
    // Devices TODO: Aqui deberiamos buscar por las extensiones del dipositivo despues de la creación, asegurarnos
    // que soporta, y a
#ifdef RENDERDOC_ENABLED
    char *layers_to_select[2] = {"VK_LAYER_KHRONOS_validation\0", "VK_LAYER_RENDERDOC_Capture\0"};
    const char **actual_layers_selected = malloc(sizeof(char *) * 2);
    actual_layers_selected[0] = malloc(sizeof(char) * 256);
    actual_layers_selected[1] = malloc(sizeof(char) * 256);
    printf("RENDERDOC_LAYER_ENABELED\n");
#else
    char *layers_to_select[1] = {"VK_LAYER_KHRONOS_validation\0"};
    const char **actual_layers_selected = malloc(sizeof(char *) * 1);
    actual_layers_selected[0] = malloc(sizeof(char) * 256);
#endif

    assert(actual_layers_selected != NULL);

    // TODO esto no sirve pa na me parece a mi
    uint32_t actual_number_layers_selected = 0;
    for (short i = 0; i < available_instance_layers_count; i++)
    {
        // TODO: Cambiar esto cuando se necesite mas de una capa
        if (ARE_STRINGS_EQUAL(vulkan_instance_layer_properties[i].layerName, layers_to_select[0]))
        {
            actual_layers_selected[0] = vulkan_instance_layer_properties[i].layerName;
            actual_number_layers_selected++;
        }
    }
#ifdef RENDERDOC_ENABLED
    printf("RENDERDOC_LAYER_ENABELED\n");
    actual_layers_selected[actual_number_layers_selected++] = layers_to_select[1];
#endif
    printf("SELECCINOADAS %d capas de vvalidacion\n", actual_number_layers_selected);

    // Probamos con la configuracion de SDL

    uint32_t extensionCountDeb = 0;
    VkExtensionProperties *extensions = NULL;

    // First call to get the number of extensions
    VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCountDeb, NULL);
    if (result != VK_SUCCESS)
    {
        printf("Failed to get extension count\n");
        return;
    }

    // Allocate memory for extensions
    extensions = malloc(extensionCountDeb * sizeof(VkExtensionProperties));
    if (extensions == NULL)
    {
        printf("Memory allocation failed\n");
        return;
    }

    // Second call to get the actual extension properties
    result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCountDeb, extensions);
    if (result != VK_SUCCESS)
    {
        printf("Failed to enumerate extensions\n");
        free(extensions);
        return;
    }

    // Print extensions
    for (uint32_t i = 0; i < extensionCountDeb; i++)
    {
        printf("Extension name: %s, Version: %d\n",
               extensions[i].extensionName,
               extensions[i].specVersion);
    }

    uint32_t extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL);
    printf("SELECCINOADAS %d extensiones de SDL\n", extensionCount);
    const char **extensionNames = malloc((extensionCount + 2) * sizeof(char *)); // TODO refactor this method to handle memory properly
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames);

    // char* extensionsToSelect[] = {"VK_EXT_debug_utils\0", "VK_KHR_xlib_surface\0", "VK_KHR_surface\0", "VK_KHR_dynamic_rendering", VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    char *extensionsToSelect[] = {"VK_EXT_debug_utils\0", "VK_KHR_xlib_surface\0"};
    extensionNames[extensionCount++] = extensionsToSelect[0];
    extensionNames[extensionCount++] = extensionsToSelect[1];
    // extensionNames[extensionCount++] = extensionsToSelect[2];
    /*
    extensionNames[extensionCount++] = extensionsToSelect[3];
        extensionNames[extensionCount++] = extensionsToSelect[4];*/
    printf("SELECCINOADAS %d extensiones\n", extensionCount);
    VkValidationFeatureEnableEXT enables[] = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,

    };

    VkValidationFeaturesEXT validationFeatures = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = sizeof(enables) / sizeof(enables[0]),
        .pEnabledValidationFeatures = enables,
        .pNext = NULL,
    };

    // VkInstance instance;
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledLayerCount = actual_number_layers_selected,
        .ppEnabledLayerNames = (const char *const *)actual_layers_selected,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = (const char *const *)extensionNames,
        .pNext = &validationFeatures,
    };
    printf("JUSTO ANTES DEL VKCREATEINSTANCE\n");
    // TODO: Para asegurar portabilidad entre sistemas con sdl, leer la direccion de la propia funcion vkGetInstanceProdAddr
    // a traves de SDL en vez de llamarla directamente como hacemos en createDebugCallbackForVulkan
    VkResult debug = vkCreateInstance(&instance_info, NULL, *instance);

    if (debug != VK_SUCCESS)
    {
        printf("ERROR AL CREAR INSTANCE: %d \n", debug);
        exit(EXIT_FAILURE);
    }
    assert(instance != NULL);
}
