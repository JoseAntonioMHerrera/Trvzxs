#pragma once

#include <stdint.h>

// A11 — push constant layout shared by every bindless-aware pipeline.
// Matches `layout(push_constant) uniform PC { uint textureIdx; uint transformIdx; }`
// in GLSL. Stages consume only the fields they need; unused fields are
// harmless (push constant storage is shared per-stage subset).
typedef struct BindlessPushConstants
{
    uint32_t textureIdx;
    uint32_t transformIdx;
} BindlessPushConstants;
