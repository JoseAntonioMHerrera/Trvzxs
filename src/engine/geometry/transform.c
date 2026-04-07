#include <stdint.h>
#include "transform.h"
#include "cglm.h"

void instanceInitTransform(Model3DInstance* inst)
{
    glm_vec3_zero(inst->position);
    glm_quat_identity(inst->rotation);
    glm_vec3_one(inst->scale);
    glm_mat4_identity(inst->m);
}

void instanceSetPosition(Model3DInstance* inst, vec3 pos)
{
    glm_vec3_copy(pos, inst->position);
}

void instanceSetRotation(Model3DInstance* inst, versor q)
{
    glm_quat_copy(q, inst->rotation);
}

void instanceSetRotationEuler(Model3DInstance* inst, float pitchDeg, float yawDeg, float rollDeg)
{
    // Build quaternion from euler angles (in radians internally)
    // Rotation order: Y (yaw) * X (pitch) * Z (roll)
    versor qPitch, qYaw, qRoll, temp;
    glm_quatv(qPitch, glm_rad(pitchDeg), (vec3){1.0f, 0.0f, 0.0f});
    glm_quatv(qYaw,   glm_rad(yawDeg),   (vec3){0.0f, 1.0f, 0.0f});
    glm_quatv(qRoll,  glm_rad(rollDeg),   (vec3){0.0f, 0.0f, 1.0f});

    glm_quat_mul(qYaw, qPitch, temp);
    glm_quat_mul(temp, qRoll, inst->rotation);
}

void instanceSetScale(Model3DInstance* inst, vec3 scale)
{
    glm_vec3_copy(scale, inst->scale);
}

void instanceSetScaleUniform(Model3DInstance* inst, float scale)
{
    inst->scale[0] = scale;
    inst->scale[1] = scale;
    inst->scale[2] = scale;
}

void instanceTranslate(Model3DInstance* inst, vec3 delta)
{
    glm_vec3_add(inst->position, delta, inst->position);
}

void instanceRotateAxis(Model3DInstance* inst, float angleDeg, vec3 axis)
{
    versor deltaQ;
    glm_quatv(deltaQ, glm_rad(angleDeg), axis);
    // Compose: new rotation = deltaQ * current rotation
    versor result;
    glm_quat_mul(deltaQ, inst->rotation, result);
    glm_quat_normalize_to(result, inst->rotation);
}

uint32_t computeGlobalInstanceIndex(Model3D* model, uint32_t localIdx)
{
    return model->denseBaseOffset + localIdx;
}

Model3DInstance* resolveGlobalInstance(Models3DManager* man, uint32_t globalIdx)
{
    return &man->pDenseModelInstances[globalIdx];
}

void instanceBuildMatrix(Model3DInstance* inst)
{
    // M = T * R * S
    mat4 T, R, S, temp;

    glm_translate_make(T, inst->position);
    glm_quat_mat4(inst->rotation, R);
    glm_scale_make(S, inst->scale);

    glm_mat4_mul(T, R, temp);
    glm_mat4_mul(temp, S, inst->m);
}
