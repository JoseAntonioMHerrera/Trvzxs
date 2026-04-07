#pragma once
#include "model.h"

// Set decomposed TRS components
void instanceSetPosition(Model3DInstance* inst, vec3 pos);
void instanceSetRotation(Model3DInstance* inst, versor q);
void instanceSetRotationEuler(Model3DInstance* inst, float pitchDeg, float yawDeg, float rollDeg);
void instanceSetScale(Model3DInstance* inst, vec3 scale);
void instanceSetScaleUniform(Model3DInstance* inst, float scale);

// Incremental modification
void instanceTranslate(Model3DInstance* inst, vec3 delta);
void instanceRotateAxis(Model3DInstance* inst, float angleDeg, vec3 axis);

// Rebuild mat4 m from position, rotation, scale (called per-frame by UBO fill)
void instanceBuildMatrix(Model3DInstance* inst);

// Initialize TRS to identity (position=0, rotation=identity, scale=1)
void instanceInitTransform(Model3DInstance* inst);

// Global index: model's denseBaseOffset + local instance index
uint32_t computeGlobalInstanceIndex(Model3D* model, uint32_t localIdx);

// Resolve global index to instance pointer in the dense array
Model3DInstance* resolveGlobalInstance(Models3DManager* man, uint32_t globalIdx);

// Set parent-child relationship. Validates parent comes before child in the dense array.
void instanceSetParent(Models3DManager* man,
                       uint32_t childModelIdx, uint32_t childLocalIdx,
                       uint32_t parentModelIdx, uint32_t parentLocalIdx);

// Clear parent (make instance a root again).
void instanceClearParent(Models3DManager* man, uint32_t modelIdx, uint32_t localIdx);
