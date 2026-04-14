#pragma once

#include "initContext.hpp"

// Phase 4c — initStorageBuffers
//
// Allocates SSBOBufferManager slots and creates the SSBOs used by compute
// and graphics pipelines:
//   - Particle SSBO (double-buffered, compute reads prev / writes curr;
//     particle graphics reads curr as vertex input)
//   - Light readback SSBO (pass-1 readback path, owned by light pipeline)
//
// SSBOs only — particle vertex/index, model geometry, and light geometry
// buffers live in initGeometryBuffers (Phase 4d).
void metaInitStorageBuffers(MetaInitContext &ctx);
