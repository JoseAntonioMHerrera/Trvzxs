#pragma once

#include "initContext.hpp"

// A11 Phase 3a — instantiate BindlessSystem + PerPassLayouts on LogicalDevice.
// Runs after initDisplay (logical device must exist) and before any code
// that might want to register resources or build pipeline layouts against
// the bindless set. No registrations performed here.
void metaInitBindless(MetaInitContext &ctx);
