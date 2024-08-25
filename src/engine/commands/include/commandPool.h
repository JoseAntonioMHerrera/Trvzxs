#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "transformations.h"
#include "vertex.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include "deviceManagment.h"
#include "geometryPipelineTypes.h"
#include <string.h> //TODO revisar memcpy de aqui

 void createGraphicsCommandPool(PhysicalDevice* pd, VkDevice device, GeometryDisplayPipelineManager *gdp);

 void createTransferQueueCommandPool(PhysicalDevice* pd, VkDevice device, GeometryDisplayPipelineManager *gdp);
