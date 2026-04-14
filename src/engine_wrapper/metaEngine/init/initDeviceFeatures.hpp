#pragma once

#include "initContext.hpp"

// Phase 7 / A11 hook — initDeviceFeatures
//
// Declares required device extensions and VkPhysicalDevice feature struct
// chains before logical device creation. Today this is an empty no-op — the
// engine relies on initDeviceManager's hardcoded defaults.
//
// A11 (bindless descriptors) populates this module with:
//   - VK_EXT_descriptor_indexing (or core 1.2 equivalent)
//   - VkPhysicalDeviceDescriptorIndexingFeatures:
//       descriptorBindingPartiallyBound,
//       descriptorBindingVariableDescriptorCount,
//       runtimeDescriptorArray,
//       descriptorBindingSampledImageUpdateAfterBind,
//       ... (exact set depends on which bindings go bindless first)
//
// MUST run between metaInitInstance and metaInitDisplay's initDeviceManager
// call (extensions are selected at logical-device creation time).
void metaInitDeviceFeatures(MetaInitContext &ctx);
