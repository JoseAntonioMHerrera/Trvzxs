#pragma once
#include "model.h"
#include "transformNode.h"

// --- Convenience wrappers: resolve transformIdx then delegate to TransformNode API ---

// Set decomposed TRS components via instance's transformIdx
void instanceSetPosition(TransformManager* tman, Model3DInstance* inst, vec3 pos);
void instanceSetRotation(TransformManager* tman, Model3DInstance* inst, versor q);
void instanceSetRotationEuler(TransformManager* tman, Model3DInstance* inst, float pitchDeg, float yawDeg, float rollDeg);
void instanceSetScale(TransformManager* tman, Model3DInstance* inst, vec3 scale);
void instanceSetScaleUniform(TransformManager* tman, Model3DInstance* inst, float scale);

// Incremental modification
void instanceTranslate(TransformManager* tman, Model3DInstance* inst, vec3 delta);
void instanceRotateAxis(TransformManager* tman, Model3DInstance* inst, float angleDeg, vec3 axis);

// Initialize the transform node referenced by this instance
void instanceInitTransform(TransformManager* tman, Model3DInstance* inst);

// Global index: model's denseBaseOffset + local instance index
uint32_t computeGlobalInstanceIndex(Model3D* model, uint32_t localIdx);

// Resolve global index to instance pointer in the dense array
Model3DInstance* resolveGlobalInstance(Models3DManager* man, uint32_t globalIdx);
