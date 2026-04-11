#include "sceneSpec.h"
#include "scene.h"
#include "transformNode.h"
#include "model.h"
#include "light.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Builder helpers
// ---------------------------------------------------------------------------

SceneEntitySpec* sceneSpecNewModel(uint32_t typeIdx, vec3 position, GEOM_VERTEX_COLOR_RGBA color)
{
    SceneEntitySpec* spec = (SceneEntitySpec*) calloc(1, sizeof(SceneEntitySpec));
    spec->kind            = SCENE_ENTITY_MODEL;
    spec->typeIdx         = typeIdx;
    glm_vec3_copy(position, spec->position);
    glm_quat_identity(spec->rotation);
    glm_vec3_one(spec->scale);
    spec->color = color;
    return spec;
}

SceneEntitySpec* sceneSpecNewLight(uint32_t typeIdx, vec3 position, float uniformScale,
                                   GEOM_VERTEX_COLOR_RGBA color)
{
    SceneEntitySpec* spec = (SceneEntitySpec*) calloc(1, sizeof(SceneEntitySpec));
    spec->kind            = SCENE_ENTITY_LIGHT;
    spec->typeIdx         = typeIdx;
    glm_vec3_copy(position, spec->position);
    glm_quat_identity(spec->rotation);
    spec->scale[0] = uniformScale;
    spec->scale[1] = uniformScale;
    spec->scale[2] = uniformScale;
    spec->color    = color;
    return spec;
}

void sceneSpecAddChild(SceneEntitySpec* parent, SceneEntitySpec* child)
{
    assert(parent != NULL);
    assert(child != NULL);

    if (parent->childCount == parent->childCapacity)
    {
        uint32_t newCap       = parent->childCapacity == 0 ? 4 : parent->childCapacity * 2;
        parent->children      = (SceneEntitySpec**) realloc(parent->children, newCap * sizeof(SceneEntitySpec*));
        parent->childCapacity = newCap;
    }
    parent->children[parent->childCount++] = child;
}

void sceneSpecFree(SceneEntitySpec* root)
{
    if (!root)
        return;
    for (uint32_t i = 0; i < root->childCount; i++)
        sceneSpecFree(root->children[i]);
    free(root->children);
    free(root);
}

// ---------------------------------------------------------------------------
// Scene fill — two-pass DFS (count, then emit)
// ---------------------------------------------------------------------------

// Pass 1: walk the tree, accumulate total entity count + per-type instance counts.
static void specCount(SceneEntitySpec* spec, uint32_t* total,
                      uint32_t* modelCounts, uint32_t* lightCounts)
{
    (*total)++;
    if (spec->kind == SCENE_ENTITY_MODEL)
        modelCounts[spec->typeIdx]++;
    else
        lightCounts[spec->typeIdx]++;

    for (uint32_t i = 0; i < spec->childCount; i++)
        specCount(spec->children[i], total, modelCounts, lightCounts);
}

// State carried through the recursive emit.
typedef struct EmitContext
{
    SCENE*    scene;
    uint32_t  nextPreorderIdx;
    uint32_t* modelFill; // next free slot per model type in pDenseModelInstances
    uint32_t* lightFill; // next free slot per light type in pDenseLightInstances
} EmitContext;

// Pass 2: emit one spec + its descendants. Returns (1 + subtreeSize), i.e. the
// total node count including this node. skipCount is stored as subtreeSize
// (not the +1), matching the semantics consumers will use for bulk skip.
static uint32_t specEmit(EmitContext* ctx, SceneEntitySpec* spec, uint32_t parentPreorderIdx)
{
    uint32_t          myIdx = ctx->nextPreorderIdx++;
    TransformManager* tman  = &ctx->scene->transformManager;

    // Hot slot
    TransformHot* hot = &tman->pTransformsHot[myIdx];
    glm_mat4_identity(hot->worldMatrix);
    hot->parentIdx = parentPreorderIdx;
    hot->lifecycle = TRANSFORM_ALIVE;
    hot->passFlags = 0;
    hot->skipCount = 0; // back-patched after recursion

    // Cold slot
    TransformCold* cold = &tman->pTransformsCold[myIdx];
    glm_vec3_copy(spec->position, cold->position);
    glm_quat_copy(spec->rotation, cold->rotation);
    glm_vec3_copy(spec->scale, cold->scale);
    glm_mat4_identity(cold->localMatrix);
    cold->_pad1 = 0.0f;
    cold->_pad2 = 0.0f;

    // Backing per-type dense instance slot
    uint32_t denseIdx;
    if (spec->kind == SCENE_ENTITY_MODEL)
    {
        Model3D*         type = &ctx->scene->modelsManager.pModel[spec->typeIdx];
        denseIdx              = type->denseBaseOffset + ctx->modelFill[spec->typeIdx]++;
        Model3DInstance* inst = &ctx->scene->modelsManager.pDenseModelInstances[denseIdx];
        inst->color           = spec->color;
        inst->transformIdx    = myIdx;
        inst->lifecycle       = INSTANCE_ALIVE;
        inst->passFlags       = 0;
    }
    else
    {
        LightVolume*         type = &ctx->scene->lightManager.pLight[spec->typeIdx];
        denseIdx                  = type->denseBaseOffset + ctx->lightFill[spec->typeIdx]++;
        LightVolumeInstance* inst = &ctx->scene->lightManager.pDenseLightInstances[denseIdx];
        inst->color               = spec->color;
        inst->transformIdx        = myIdx;
        inst->lifecycle           = LIGHT_INSTANCE_ALIVE;
        inst->passFlags           = 0;
    }

    // Preorder entity reverse index (cold-path lookup)
    tman->pPreorderEntities[myIdx].kind     = (uint16_t) spec->kind;
    tman->pPreorderEntities[myIdx].typeIdx  = (uint16_t) spec->typeIdx;
    tman->pPreorderEntities[myIdx].denseIdx = denseIdx;

    // Per-entity local AABB (copy from backing type)
    const AABB* srcAABB = (spec->kind == SCENE_ENTITY_MODEL)
                              ? &ctx->scene->modelsManager.pModel[spec->typeIdx].localAABB
                              : &ctx->scene->lightManager.pLight[spec->typeIdx].localAABB;
    tman->pLocalAABBs[myIdx] = *srcAABB;

    // Recurse into children, accumulating subtree size
    uint32_t subtreeSize = 0;
    for (uint32_t i = 0; i < spec->childCount; i++)
    {
        subtreeSize += specEmit(ctx, spec->children[i], myIdx);
    }

    // Back-patch skipCount with total descendants
    tman->pTransformsHot[myIdx].skipCount = subtreeSize;

    return 1 + subtreeSize;
}

void fillSceneFromSpecTree(SCENE* scene, SceneEntitySpec** roots, uint32_t rootCount)
{
    assert(scene != NULL);
    assert(rootCount == 0 || roots != NULL);

    uint32_t modelLen = scene->modelsManager.modelLen;
    uint32_t lightLen = scene->lightManager.lightLen;

    // --- Pass 1: count ---
    uint32_t  total       = 0;
    uint32_t* modelCounts = (uint32_t*) calloc(modelLen > 0 ? modelLen : 1, sizeof(uint32_t));
    uint32_t* lightCounts = (uint32_t*) calloc(lightLen > 0 ? lightLen : 1, sizeof(uint32_t));

    for (uint32_t r = 0; r < rootCount; r++)
        specCount(roots[r], &total, modelCounts, lightCounts);

    // --- Pass 2: assign per-type denseBaseOffset + instanceCount ---
    uint32_t modelInstanceCapacity = 0;
    for (uint32_t m = 0; m < modelLen; m++)
    {
        Model3D* type             = &scene->modelsManager.pModel[m];
        type->denseBaseOffset     = modelInstanceCapacity;
        type->instanceCount       = modelCounts[m];
        type->modelInstanceCount  = modelCounts[m];
        type->liveInstanceCount   = modelCounts[m];
        type->alive               = MODEL_ALIVE;
        modelInstanceCapacity += modelCounts[m];
    }
    scene->modelsManager.modelInstanceCapacity = modelInstanceCapacity;
    scene->modelsManager.modelInstanceLen      = modelInstanceCapacity;

    uint32_t lightInstanceCapacity = 0;
    for (uint32_t l = 0; l < lightLen; l++)
    {
        LightVolume* type              = &scene->lightManager.pLight[l];
        type->denseBaseOffset          = lightInstanceCapacity;
        type->lightVolumeInstanceCount = lightCounts[l];
        type->liveInstanceCount        = lightCounts[l];
        type->alive                    = LIGHT_ALIVE;
        lightInstanceCapacity += lightCounts[l];
    }
    scene->lightManager.lightInstanceCapacity = lightInstanceCapacity;
    scene->lightManager.lightInstanceCount    = lightInstanceCapacity;

    // --- Pass 3: allocate dense arrays ---
    scene->modelsManager.pDenseModelInstances =
        (Model3DInstance*) calloc(modelInstanceCapacity > 0 ? modelInstanceCapacity : 1, sizeof(Model3DInstance));
    scene->lightManager.pDenseLightInstances =
        (LightVolumeInstance*) calloc(lightInstanceCapacity > 0 ? lightInstanceCapacity : 1, sizeof(LightVolumeInstance));

    // Per-type instanceOffset arrays (consumed by UBO fill compaction / draw loop)
    scene->modelsManager.instanceOffset.items    = (uint32_t*) calloc(modelLen > 0 ? modelLen : 1, sizeof(uint32_t));
    scene->modelsManager.instanceOffset.itemsLen = modelLen;
    scene->lightManager.instanceOffset.items     = (uint32_t*) calloc(lightLen > 0 ? lightLen : 1, sizeof(uint32_t));
    scene->lightManager.instanceOffset.itemsLen  = lightLen;
    for (uint32_t m = 0; m < modelLen; m++)
        scene->modelsManager.instanceOffset.items[m] = scene->modelsManager.pModel[m].denseBaseOffset;
    for (uint32_t l = 0; l < lightLen; l++)
        scene->lightManager.instanceOffset.items[l] = scene->lightManager.pLight[l].denseBaseOffset;

    // Transform arrays sized to total entities
    TransformManager* tman = &scene->transformManager;
    tman->pTransformsHot    = (TransformHot*)   calloc(total > 0 ? total : 1, sizeof(TransformHot));
    tman->pTransformsCold   = (TransformCold*)  calloc(total > 0 ? total : 1, sizeof(TransformCold));
    tman->pPreorderEntities = (PreorderEntity*) calloc(total > 0 ? total : 1, sizeof(PreorderEntity));
    tman->pLocalAABBs       = (AABB*)           calloc(total > 0 ? total : 1, sizeof(AABB));
    tman->count             = total;
    tman->liveCount         = total;

    // --- Pass 4: DFS emit ---
    EmitContext ctx = {
        .scene           = scene,
        .nextPreorderIdx = 0,
        .modelFill       = (uint32_t*) calloc(modelLen > 0 ? modelLen : 1, sizeof(uint32_t)),
        .lightFill       = (uint32_t*) calloc(lightLen > 0 ? lightLen : 1, sizeof(uint32_t)),
    };

    for (uint32_t r = 0; r < rootCount; r++)
        specEmit(&ctx, roots[r], TRANSFORM_NO_PARENT);

    assert(ctx.nextPreorderIdx == total);

    free(ctx.modelFill);
    free(ctx.lightFill);
    free(modelCounts);
    free(lightCounts);
}
