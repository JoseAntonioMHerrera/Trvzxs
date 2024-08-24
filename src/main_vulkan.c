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
/*
void destroyDebugCallbackForVulkan(VkInstance* instance, VkDebugUtilsMessengerEXT* callback_messenger){
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = VK_NULL_HANDLE;
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT\0");
    assert(vkDestroyDebugUtilsMessengerEXT != NULL);

    vkDestroyDebugUtilsMessengerEXT(*instance, *callback_messenger, NULL);

    printf("Vulkan log callback destroyed.\n");
}



static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * pUserData){

    printf("@@ DEBUG CALLBACK @@\n");
    printf("Mascara de severidad de mensaje: %u\n",severity);
    printf("Mascara de tipo de mensaje: %u\n",type);
    printf("Numero de objetos reportados: %d\n", pCallbackData->objectCount);
    for(uint32_t i =0; i <= pCallbackData->objectCount; i++) {
        printf("%s\n",pCallbackData->pMessage);
        printf("%lu\n",pCallbackData->pObjects[i].objectHandle);

    }
    printf("@@ DEBUG CALLBACK @@\n");

    // More deep checks and analysis here

    // Based on the data received, choose to halt or not halt program
    return VK_FALSE;

};



void destroyVulkanSurface(const VkInstance* instance, VkSurfaceKHR* surface){
    PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
    vkDestroySurface = (PFN_vkDestroySurfaceKHR) vkGetInstanceProcAddr(*instance, "vkDestroySurfaceKHR\0");
    assert(vkDestroySurface != NULL);

    vkDestroySurface(*instance, *surface, NULL);

    printf("Vulkan surface destroyed.\n");
}

void destroyVulkanInstance(const VkInstance* instance){
    vkDestroyInstance(*instance, NULL);

    printf("Vulkan instance destroyed.\n");
}

VkDebugUtilsMessengerEXT* createDebugCallbackForVulkan(const VkInstance* instance){
    VkDebugUtilsMessengerCreateInfoEXT debug_callback_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        .messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &vulkanDebugCallback,
        .pUserData = NULL,
    };
    VkDebugUtilsMessengerEXT* callback_messenger = NULL;
    callback_messenger = malloc(sizeof(VkDebugUtilsMessengerEXT));
    assert(callback_messenger != NULL);

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = VK_NULL_HANDLE;
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT\0");
    assert(vkCreateDebugUtilsMessengerEXT != NULL);

    VkResult messenger_creation_result = vkCreateDebugUtilsMessengerEXT(*instance, &debug_callback_info, NULL,  callback_messenger);
    assert(messenger_creation_result == VK_SUCCESS);

    return callback_messenger;
}

void cleanup(DeviceManager *dm, LogicalDevice *ld, DisplayManager* dispM, GeometryDisplayPipelineManager* gdp, VkDebugUtilsMessengerEXT *logCallback) {
    vkDeviceWaitIdle(dm->lDevice[dm->deviceIndex]->device);
    cleanupSwapChain(dm->lDevice[dm->deviceIndex], ld->displayManager, ld->displayManager->swapChainM);
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
    cleanupSyncObjects(ld->device,gdp);

    //vkDestroyPipelineLayout(device, pipelineLayout, NULL);


    vkDestroyDevice(ld->device, NULL);

    if (logCallback) {
        destroyDebugCallbackForVulkan(&dm->instance, dm->logCallback);
    }

    vkDestroySurfaceKHR(dm->instance, dispM->surface, NULL);
    vkDestroyInstance(dm->instance, NULL);
}


/*
void fillGeometryData(GEOM_OBJECT **geometry){
    //printf("GEOM OBJECTS FLAG 0 \n");
    (*geometry) = (GEOM_OBJECT*) malloc(sizeof(GEOM_OBJECT));
    //printf("GEOM OBJECTS FLAG 1 \n");
    (*geometry)->dataLen = 4;
    (*geometry)->indicesLen = 6;
    (*geometry)->data = malloc(sizeof(*((*geometry)->data)) * (*geometry)->dataLen);
    //printf("GEOM OBJECTS FLAG 2 \n");
    GEOM_VERTEX_COLOR_RGBA colorData[4];
    for(uint32_t i=0; i < 4; i++){
            colorData[i].r = i == 0 ? 1.0f : i == 1 ? 0.0f : i == 2 ? 0.0f : 1.0f;
            colorData[i].b = i == 0 ? 0.0f : i == 1 ? 1.0f : i == 2 ? 0.0f : 1.0f;
            colorData[i].g = i == 0 ? 0.0f : i == 1 ? 0.0f : i == 2 ? 1.0f : 1.0f;
            colorData[i].a = 1.0f;
            (*geometry)->data[i].color = colorData[i];
    }

    GEOM_VERTEX_POSITION positionDat[4];
    for(uint32_t i=0; i < 4; i++){
            positionDat[i].x = i == 0 ? -0.5f : i == 1 ? 0.5f : i == 2 ? 0.5f : -0.5f;
            positionDat[i].y = i == 0 ? -0.5f : i == 1 ? -0.5f : i == 2 ? 0.5f : 0.5f;
            positionDat[i].z = 0.0f;
            printf("Punto %d (%f,%f,%f)\n", i, positionDat[i].x, positionDat[i].y, positionDat[i].z);
            (*geometry)->data[i].point = positionDat[i];
    }

    GEOM_VERTEX_TEXT_COORD textCoords[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    };

    (*geometry)->data[0].textCoords = textCoords[0];
    (*geometry)->data[1].textCoords = textCoords[1];
    (*geometry)->data[2].textCoords = textCoords[2];
    (*geometry)->data[3].textCoords = textCoords[3];

    (*geometry)->indices = malloc(sizeof(uint32_t) * (*geometry)->indicesLen);
    (*geometry)->indices[0] = 0;
    (*geometry)->indices[1] = 1;
    (*geometry)->indices[2] = 2;
    (*geometry)->indices[3] = 2;
    (*geometry)->indices[4] = 3;
    (*geometry)->indices[5] = 0;
}*/

/*
void apply_offset_to_indices(TRVZXS_SIZE* indices, size_t num_indices, TRVZXS_SIZE offset) {
    for (size_t i = 0; i < num_indices; i++) {
        indices[i] += offset;
    }
}

void getScene(SCENE **scene, size_t objectsLen, float size) {
    *scene = malloc(sizeof(SCENE));
    (*scene)->sceneLen = objectsLen;
    (*scene)->pScene = malloc(sizeof(GEOM_OBJECT)* objectsLen);

    SCENE* derrefScene = *scene;
    uint32_t spaceOffsetForModel = 0;
    uint32_t indexOffsetForModelIndices = 0;
    for(int objIndex = 0; objIndex < objectsLen; objIndex++){
        derrefScene->pScene[objIndex] = *(create_cube(size, spaceOffsetForModel));
        GEOM_OBJECT* cube = &derrefScene->pScene[objIndex];

        apply_offset_to_indices(cube->indices, cube->indicesLen, indexOffsetForModelIndices);
        indexOffsetForModelIndices += cube->vertexOffset;

        printf("INDICES DEL CUBO: %d\n", derrefScene->pScene[objIndex].indicesLen);
        spaceOffsetForModel+=5;
    }

    derrefScene->sceneVertexLen =  getVertexLenFromScene(derrefScene);
    derrefScene->sceneVertex =  getVertexFromScene(derrefScene);

    derrefScene->totalIndicesLen = getIndicesLenFromScene(derrefScene);
    derrefScene->sceneIndices = getIndicesFromScene(derrefScene);
    printf("FINALIZAMOS CREACION ESCENA!\n");
    printf("SCENELEN : %d", derrefScene->sceneLen);
        printf("INDICESLEN : %d", derrefScene->totalIndicesLen);
}*/
/*
void getGrid(GEOM_OBJECT **geometry, int points, int padding) {
    *geometry = create_grid(geometry, points, padding);
}*/
/*
DeviceManager* initTRVZXSEngine(SDL_Window* window){

    DeviceManager* dm = malloc(sizeof(DeviceManager)); // Init for the general state object of the render
    printf("inittrazosengine flag 1\n");
    /*dm->lDevice = malloc(sizeof(*dm->lDevice));
    (*dm->lDevice) = malloc(sizeof(**dm->lDevice));
    dm->pDevice = malloc(sizeof(dm->pDevice));
    dm->gpuIndex = 0;
    dm->deviceIndex = 0;



    createInstance(&dm->instance, window);
        printf("RETORNO CREATEINSTANCE\n\0");
    DisplayManager *dispM = (DisplayManager*) malloc(sizeof(DisplayManager));
    dispM->swapChainM = malloc(sizeof(*dispM->swapChainM));
    dispM->geomPipeline = malloc(sizeof(*dispM->geomPipeline));
    DisplaySwapchainManager *dsm = dispM->swapChainM;

    GeometryDisplayPipelineManager *gdp = dispM->geomPipeline;
    DeviceBufferManager *devicebm;
    gdp->commandPool = malloc(sizeof(*gdp->commandPool));
    gdp->texture =
    // setting the max number of images according to config frames in flight
    gdp->maxFrames = MAX_FRAMES_IN_FLIGHT;
    dsm->scImagesLen = gdp->maxFrames;
    createSDLSurface(dispM, dm->instance, window);
    printf("RETORNO CREATESURFACE\n\0");

    initDeviceManager(dm->instance, dm, dispM);
    printf("RETORNOINITDEVICEMANAGER\n\0");


    PhysicalDevice* pd = &dm->pDevice[dm->gpuIndex];
    LogicalDevice *ld = dm->lDevice[dm->deviceIndex];

    // GET THE SCENE MODELS
    getScene(&gdp->geometry, 2,  2.0f);
    // exit(0);
    printf("RETORNO FILLGEOMETRYDATA\n\0");
    physicalDeviceTest(pd->gpu);
    if(VK_FALSE == isDeviceValid(ld->device)){
        printf("DEVICE INVALID\n");
        exit(1);
    }
    createSwapChain(pd, ld, dispM, dsm, window);
    //WE UPDATE MAXFRAMES WITH THE SWAPCHAIN RETURNED MAX IMAGE VIEWS allowed
    gdp->maxFrames = dsm->scImagesLen;
    printf("NUMERO DE FRAMES %d\n", gdp->maxFrames);
    printf("RETORNO CREATESWAPCHAIN\n\0");
    createRenderPass(dispM->swapChainM,ld->device,gdp);
        printf("RETORNO RENDERPASS\n\0");
    dsm->scImageViewLen = dsm->scImagesLen;
    // dsm->p_scImageView = malloc(sizeof(VkImageView) * dsm->scImageViewLen);
    // createImageView(dsm, &ld->device, dsm->currentSurfaceFormat->format);
    for(uint32_t i=0; i < ld->displayManager->swapChainM->scImagesLen; i++){
        ld->displayManager->swapChainM->p_scImageView[i] =
                    createImageView(dsm, ld->device, dsm->currentSurfaceFormat->format, ld->displayManager->swapChainM->p_scImages[i]);
    }
    printf("RETORNO IMAGEVIEWS\n\0");
    createFrameBuffers(dsm,gdp->renderPass, &ld->device);
    printf("RETORNO FRAMEBUFFERS\n\0");
    gdp->deviceBuffer = malloc(sizeof(*gdp->deviceBuffer));
    devicebm = gdp->deviceBuffer;


    createGraphicsCommandPool(pd,ld->device,gdp);
    printf("RETORNO COMMANDPOOLD\n\0");
    createTransferQueueCommandPool(pd,ld->device,gdp);
        printf("RETORNO TRANSFERPOOL\n\0");
    createCommandBuffer(gdp, &ld->device, gdp->maxFrames);
    VkDeviceSize textureSize;
    stbi_uc *pixels;
    uint32_t width;
    uint32_t height;
    createTextureImage( pd,  ld, gdp, &gdp->texture);
        printf("RETORNO createTextureImage\n\0");
    createTextureImageView(dispM->swapChainM, ld->device, gdp->texture);
        printf("RETORNO createTextureImageView\n\0");
    devicebm->ubo = malloc(sizeof(*(gdp->deviceBuffer->ubo)));
    createUniformBuffers(pd,ld, gdp->maxFrames, gdp->deviceBuffer->ubo);
    printf("RETORNO UNIFORMBUFFERS\n\0");
    printf("!!!!!! %d", gdp->maxFrames);
        createDescriptorPool(gdp->maxFrames, ld->device, gdp);
    createDescriptorSetLayout(ld, gdp, gdp->maxFrames);
        printf("RETORNO SETLAYOUT\n\0");
        printf("RETORNO DESCRIPTOR POOL\n\0");
    createDescriptorSet(gdp->maxFrames, ld->device, gdp, gdp->texture);
    printf("RETORNO SETLAYOUT\n\0");
    createGraphicsPipeline(gdp, dispM, &ld->device);
    printf("RETORNO GRPAHICSPIPLINE\n\0");
//55555555555555//
     printf("RETORNO COMMANDBUFFER\n\0");
    createSyncObjects(gdp, &ld->device);
         printf("RETORNO SYNC\n\0");

    createGeometryBuffer(pd,ld,gdp->deviceBuffer,gdp->geometry, gdp->commandPool);
         printf("RETORNO GEOMETRYBUFFER\n\0");

    assert(&gdp->geometry->pScene[0]);
    createIndiceBuffer(pd, ld,gdp->deviceBuffer,gdp->geometry, gdp->commandPool);
         printf("RETORNOs INDICE\n\0");

    dm->logCallback = createDebugCallbackForVulkan(dm->instance);
    dispM->currentFrame = 0;
    printf("Init vulkan ended.\n");

    return dm;

}*/
void createInstance(VkInstance** instance, SDL_Window* window) {
    *instance = (VkInstance*) malloc(sizeof(VkInstance));
    printf("CREATE INSTANCE FLAG 1\n");
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "GrassBox\0",
        .applicationVersion = 0,
        .pEngineName = "GrassBox\0",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0,
    };

    char* extensionsToSelect[] = {"VK_EXT_debug_utils\0", "VK_KHR_xlib_surface\0", "VK_KHR_surface\0", "VK_KHR_dynamic_rendering"};

    uint32_t available_instance_layers_count = 0;
    VkLayerProperties* vulkan_instance_layer_properties;

    VkResult vkEnumInstLayerPropsResult = vkEnumerateInstanceLayerProperties(&available_instance_layers_count, VK_NULL_HANDLE);
    assert(vkEnumInstLayerPropsResult == VK_SUCCESS && available_instance_layers_count > 0);
    vulkan_instance_layer_properties = malloc(sizeof(VkLayerProperties) * available_instance_layers_count);

    VkResult vkEnumInstLayPropResult = vkEnumerateInstanceLayerProperties(&available_instance_layers_count, vulkan_instance_layer_properties);
    assert(vkEnumInstLayPropResult == VK_SUCCESS);
    // Devices TODO: Aqui deberiamos buscar por las extensiones del dipositivo despues de la creación, asegurarnos
    // que soporta, y a
    char* layers_to_select[] = {"VK_LAYER_KHRONOS_validation\0"};
    const char** actual_layers_selected = malloc(sizeof(char*));
    actual_layers_selected[0] = malloc(sizeof(char) * 256);
    assert(actual_layers_selected != NULL);
    printf("CREATE INSTANCE FLAG 2\n");
    uint32_t actual_number_layers_selected = 0;
    for(short i = 0; i < available_instance_layers_count; i++){
        // TODO: Cambiar esto cuando se necesite mas de una capa
        if(ARE_STRINGS_EQUAL(vulkan_instance_layer_properties[i].layerName, layers_to_select[0])){
            actual_layers_selected[0] = vulkan_instance_layer_properties[i].layerName;
            actual_number_layers_selected++;
        }
    }

    printf("CREATE INSTANCE FLAG 3\n");
    //Probamos con la configuracion de SDL

    uint32_t extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL);
    const char** extensionNames = malloc(extensionCount * sizeof(char*));
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames);

    //VkInstance instance;
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledLayerCount = actual_number_layers_selected,
        .ppEnabledLayerNames = (const char *const *)actual_layers_selected,
        .enabledExtensionCount = 3,
        .ppEnabledExtensionNames = (const char *const *) extensionsToSelect,
    };
    //TODO: Para asegurar portabilidad entre sistemas con sdl, leer la direccion de la propia funcion vkGetInstanceProdAddr
    //a traves de SDL en vez de llamarla directamente como hacemos en createDebugCallbackForVulkan
    VkResult debug = vkCreateInstance(&instance_info, NULL, *instance);
    printf("Debug: %x\n", debug);
    printf("CREATE INSTANCE FLAG 4\n");
    if ( debug != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance\n");
        exit(EXIT_FAILURE);
    }

    assert(instance != NULL);

}
