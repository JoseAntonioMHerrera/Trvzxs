#include "scene.h"
#include "frustum.h"
#include "light.h"
#include "transformNode.h"
#include "vertex.h"
#include "graphicConfiguration.h"
#include "meshoptimizer/meshoptimizer.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void apply_offset_to_indices(TRVZXS_SIZE* indices, size_t num_indices, TRVZXS_SIZE offset)
{
    for (size_t i = 0; i < num_indices; i++)
    {
        indices[i] += offset;
    }
}

void initIndicesScene(uint32_t* indices, uint32_t indicesLen)
{
    for (int i = 0; i < indicesLen; i++)
    {
        indices[i] = 0;
    }
}

uint32_t getModelIndicesLenFromScene(const SCENE* const scene)
{
    uint32_t indicesLen = 0;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        indicesLen += scene->modelsManager.pModel[i].indices.itemsLen;
    }

    return indicesLen;
}

uint32_t* getModelIndicesFromScene(const SCENE* const scene)
{
    assert(scene->modelsManager.packedModelIndex.itemsLen > 0);

    uint32_t* indices = malloc(sizeof(uint32_t) * scene->modelsManager.packedModelIndex.itemsLen);
    initIndicesScene(indices, scene->modelsManager.packedModelIndex.itemsLen);

    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    uint32_t iteratorStorageIndices = 0;

    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        uint32_t currentObjectIndicesLen = scene->modelsManager.pModel[i].indices.itemsLen;
        uint32_t startPositionOffset     = iteratorStorageIndices;

        uint32_t iteratorReadIndices = 0;
        for (iteratorStorageIndices; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen);
             iteratorStorageIndices++)
        {
            indices[indicesStorageIterator] = scene->modelsManager.pModel[i].indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }
    return indices;
}

uint32_t getModelVerticesLenFromScene(const SCENE* const scene)
{
    uint32_t dataLen = 0;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        for (int j = 0; j < scene->modelsManager.pModel[i].meshManager.meshesLen; j++)
        {
            dataLen += scene->modelsManager.pModel[i].meshManager.pMesh[j].verticesLen;
        }
    }

    return dataLen;
}

GEOM_VERTEX* getModelVerticesFromScene(const SCENE* const scene)
{
    assert(scene->modelsManager.modelLen > 0);
    GEOM_VERTEX* pVertex                = malloc(sizeof(GEOM_VERTEX) * scene->modelsManager.packedModelVertexLen);
    uint32_t     indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        for (int j = 0; j < scene->modelsManager.pModel[i].meshManager.meshesLen; j++)
        {
            for (int indexObjVertex = 0;
                 indexObjVertex < scene->modelsManager.pModel[i].meshManager.pMesh[j].verticesLen; indexObjVertex++)
            {
                pVertex[indicesStorageIterator] =
                    scene->modelsManager.pModel[i].meshManager.pMesh[j].pVertice[indexObjVertex];
                // pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
                indicesStorageIterator++;
            }
        }
    }
    return pVertex;
}

MODEL_VERTEX* getModelVerticesLeanFromScene(const SCENE* const scene)
{
    assert(scene->modelsManager.modelLen > 0);
    MODEL_VERTEX* pVertex               = malloc(sizeof(MODEL_VERTEX) * scene->modelsManager.packedModelVertexLen);
    uint32_t      indicesStorageIterator = 0;

    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        for (int j = 0; j < scene->modelsManager.pModel[i].meshManager.meshesLen; j++)
        {
            for (int v = 0; v < scene->modelsManager.pModel[i].meshManager.pMesh[j].verticesLen; v++)
            {
                const GEOM_VERTEX* src    = &scene->modelsManager.pModel[i].meshManager.pMesh[j].pVertice[v];
                MODEL_VERTEX*      dst    = &pVertex[indicesStorageIterator];
                dst->point                = src->point;
                dst->texCoord.u           = src->textCoords.u;
                dst->texCoord.v           = src->textCoords.v;
                dst->normal               = src->normal;
                indicesStorageIterator++;
            }
        }
    }
    return pVertex;
}

uint32_t getVolumetricLightsIndicesLenFromScene(const SCENE* const scene)
{
    uint32_t indicesLen = 0;
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        indicesLen += scene->lightManager.pLight[i].model.indices.itemsLen;
    }

    return indicesLen;
}

uint32_t* getVolumetricLightsIndicesFromScene(const SCENE* const scene)
{
    if (scene->lightManager.packedLightIndex.itemsLen > 0)
    {
        return 0;
    }

    uint32_t* indices = malloc(sizeof(uint32_t) * scene->lightManager.packedLightIndex.itemsLen);

    initIndicesScene(indices, scene->lightManager.packedLightIndex.itemsLen);

    uint32_t indicesStorageIterator = 0;
    uint32_t iteratorStorageIndices = 0;
    uint32_t totalSceneObjects      = scene->lightManager.lightLen;

    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        uint32_t currentObjectIndicesLen = scene->lightManager.pLight[i].model.indices.itemsLen;

        uint32_t startPositionOffset = iteratorStorageIndices; // flag current offset

        uint32_t iteratorReadIndices = 0;
        for (iteratorStorageIndices; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen);
             iteratorStorageIndices++)
        {

            indices[indicesStorageIterator] = scene->lightManager.pLight[i].model.indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }

    return indices;
}

uint32_t getVolumetricLightVerticesLenFromScene(const SCENE* const scene)
{
    uint32_t dataLen = 0;
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        for (int j = 0; j < scene->lightManager.pLight[i].model.meshManager.meshesLen; j++)
        {
            dataLen += scene->lightManager.pLight[i].model.meshManager.pMesh[j].verticesLen;
        }
    }
    return dataLen;
}

GEOM_VERTEX* getLightVolumesVerticesFromScene(const SCENE* const scene)
{
    assert(scene->lightManager.packedLightLen > 0);
    GEOM_VERTEX* pVertex                = malloc(sizeof(GEOM_VERTEX) * scene->lightManager.packedLightLen);
    uint32_t     indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        for (int j = 0; j < scene->lightManager.pLight[i].model.meshManager.meshesLen; j++)
        {
            for (int indexObjVertex = 0;
                 indexObjVertex < scene->lightManager.pLight[i].model.meshManager.pMesh[j].verticesLen;
                 indexObjVertex++)
            {
                pVertex[indicesStorageIterator] =
                    scene->lightManager.pLight[i].model.meshManager.pMesh[j].pVertice[indexObjVertex];
                indicesStorageIterator++;
            }
        }
    }

    return pVertex;
}

/*
    SCENE** scene: pointer reference to init the memory for the scene;
    uint32_t modelsLen: number of GEOM_OBJE

*/
void initScene(SCENE** scene, uint32_t modelsLen, uint32_t lightsLen, uint32_t particleVerticesLen,
               uint32_t particleGroupsLen, uint32_t particleInstancesLen)
{
    (*scene) = (SCENE*) malloc(sizeof(SCENE));

    // TransformManager — allocated later once total instance counts are known
    (*scene)->transformManager.pTransformsHot    = NULL;
    (*scene)->transformManager.pTransformsWorld  = NULL;
    (*scene)->transformManager.pTransformsCold   = NULL;
    (*scene)->transformManager.pPreorderEntities = NULL;
    (*scene)->transformManager.pLocalAABBs       = NULL;
    (*scene)->transformManager.count             = 0;
    (*scene)->transformManager.liveCount         = 0;

    if (modelsLen <= 0)
    {
        (*scene)->modelsManager.modelLen = 0;
    }
    else
    {
        (*scene)->modelsManager.pModel   = (Model3D*) malloc(sizeof(Model3D) * modelsLen);
        (*scene)->modelsManager.modelLen = modelsLen;
    }

    if (lightsLen <= 0)
    {
        (*scene)->lightManager.lightLen = 0;
    }
    else
    {
        (*scene)->lightManager.pLight   = (LightVolume*) malloc(sizeof(LightVolume) * lightsLen);
        (*scene)->lightManager.lightLen = lightsLen;
    }

    if (particleGroupsLen <= 0)
    {
        (*scene)->particleManager.particleGroupLen = 0;
    }
    else
    {

        (*scene)->particleManager.packedParticleVertexLen = particleVerticesLen;
        (*scene)->particleManager.particleGroupLen        = particleGroupsLen;
        (*scene)->particleManager.pParticleGroup = (ParticleGroup*) malloc(sizeof(ParticleGroup) * particleGroupsLen);
        (*scene)->particleManager.pParticleInstance =
            (ParticleInstance*) malloc(sizeof(ParticleInstance) * particleInstancesLen);
        (*scene)->particleManager.packedParticleInstanceLen = particleInstancesLen;
    }
}

void printSceneModels(const Models3DManager* const modelManager)
{

    // Iterate through all models
    for (uint32_t modelIndex = 0; modelIndex < modelManager->modelLen; modelIndex++)
    {
        Model3D* currentModel = &modelManager->pModel[modelIndex];

        Mesh3DManager* meshManager = &currentModel->meshManager;

        // Iterate through all meshes in current model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
        {
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

            if (currentMesh->verticesLen == 0)
            {
                continue;
            }

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex += 1000)
            {
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
            }
        }
    }
}

void printSceneLights(const LightManager* const volManager)
{
    // Iterate through all volumetric lights
    for (uint32_t lightIndex = 0; lightIndex < volManager->lightLen; lightIndex++)
    {
        LightVolume*   currentLight = &volManager->pLight[lightIndex];
        Model3D*       currentModel = &currentLight->model;
        Mesh3DManager* meshManager  = &currentModel->meshManager;

        // Iterate through all meshes in current light's model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
        {
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++)
            {
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
            }
        }
    }
}

void printLightVolume(LightVolume* const light)
{
    if (!light)
    {
        return;
    }

    // Print light properties
    // Get the model and mesh manager
    Model3D*       currentModel = &light->model;
    Mesh3DManager* meshManager  = &currentModel->meshManager;

    // Iterate through all meshes in the light's model
    for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
    {
        Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

        // Iterate through all vertices in current mesh
        for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++)
        {
            GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
        }
    }
}

void printScene_(GEOM_OBJECT* objects, uint32_t objectsLen) {};

void fillModelInstancesIndices(Models3DManager* man, TransformManager* tman)
{

    man->instanceOffset.items    = malloc(sizeof(*man->instanceOffset.items) * man->modelLen);
    man->instanceOffset.itemsLen = man->modelLen;

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < man->modelLen; itemIdx++)
    {
        man->instanceOffset.items[itemIdx]   = instanceOffset;
        man->pModel[itemIdx].denseBaseOffset = instanceOffset;
        instanceOffset += man->pModel[itemIdx].instanceCount;
    }

    // Allocate single dense array and consolidate per-model temporary allocations
    man->pDenseModelInstances = (Model3DInstance*) calloc(man->modelInstanceCapacity, sizeof(Model3DInstance));

    // Allocate transform nodes for model instances [0, modelInstanceCapacity).
    // Four parallel arrays indexed in lock-step: hot, cold, preorder-entity
    // reverse index, and per-entity local AABB. If transforms already
    // allocated (e.g. lights ran first), realloc to grow.
    uint32_t transformBase = tman->count;
    uint32_t newCount      = tman->count + man->modelInstanceCapacity;
    if (tman->pTransformsHot == NULL)
    {
        tman->pTransformsHot    = (TransformHot*)   calloc(newCount, sizeof(TransformHot));
        tman->pTransformsWorld  = (TransformWorld*) calloc(newCount, sizeof(TransformWorld));
        tman->pTransformsCold   = (TransformCold*)  calloc(newCount, sizeof(TransformCold));
        tman->pPreorderEntities = (PreorderEntity*) calloc(newCount, sizeof(PreorderEntity));
        tman->pLocalAABBs       = (AABB*)           calloc(newCount, sizeof(AABB));
    }
    else
    {
        tman->pTransformsHot    = (TransformHot*)   realloc(tman->pTransformsHot,    newCount * sizeof(TransformHot));
        tman->pTransformsWorld  = (TransformWorld*) realloc(tman->pTransformsWorld,  newCount * sizeof(TransformWorld));
        tman->pTransformsCold   = (TransformCold*)  realloc(tman->pTransformsCold,   newCount * sizeof(TransformCold));
        tman->pPreorderEntities = (PreorderEntity*) realloc(tman->pPreorderEntities, newCount * sizeof(PreorderEntity));
        tman->pLocalAABBs       = (AABB*)           realloc(tman->pLocalAABBs,       newCount * sizeof(AABB));
        memset(&tman->pTransformsHot[tman->count],    0, man->modelInstanceCapacity * sizeof(TransformHot));
        memset(&tman->pTransformsWorld[tman->count],  0, man->modelInstanceCapacity * sizeof(TransformWorld));
        memset(&tman->pTransformsCold[tman->count],   0, man->modelInstanceCapacity * sizeof(TransformCold));
        memset(&tman->pPreorderEntities[tman->count], 0, man->modelInstanceCapacity * sizeof(PreorderEntity));
        memset(&tman->pLocalAABBs[tman->count],       0, man->modelInstanceCapacity * sizeof(AABB));
    }
    tman->count = newCount;

    for (uint32_t m = 0; m < man->modelLen; m++)
    {
        Model3D* model = &man->pModel[m];
        uint32_t base  = model->denseBaseOffset;
        if (model->pStagingInstances)
        {
            memcpy(&man->pDenseModelInstances[base], model->pStagingInstances,
                   model->instanceCount * sizeof(Model3DInstance));
            free(model->pStagingInstances);
            model->pStagingInstances = NULL;
        }

        // Set transformIdx and populate transform nodes from staging transforms
        for (uint32_t i = 0; i < model->instanceCount; i++)
        {
            uint32_t         tIdx = transformBase + base + i;
            Model3DInstance* inst = &man->pDenseModelInstances[base + i];
            inst->transformIdx    = tIdx;

            // Hot slot: default-initialized (parentIdx=NO_PARENT, alive, no flags, skipCount=0)
            transformInitHot(&tman->pTransformsHot[tIdx]);

            if (model->pStagingTransforms)
            {
                // Cold slot: copy TRS + localMatrix from staging
                tman->pTransformsCold[tIdx] = model->pStagingTransforms[i];
            }
            else
            {
                transformInitCold(&tman->pTransformsCold[tIdx]);
            }

            // Preorder entity reverse index: this transform position backs a
            // model instance of type `m` at dense position `base + i`.
            tman->pPreorderEntities[tIdx].kind     = (uint16_t) SCENE_ENTITY_MODEL;
            tman->pPreorderEntities[tIdx].typeIdx  = (uint16_t) m;
            tman->pPreorderEntities[tIdx].denseIdx = base + i;

            // Per-entity local AABB: copy from the type. Load order guarantees
            // localAABB is already computed by this point (see fillSceneWithModelData).
            tman->pLocalAABBs[tIdx] = model->localAABB;
        }

        if (model->pStagingTransforms)
        {
            free(model->pStagingTransforms);
            model->pStagingTransforms = NULL;
        }
    }
}

// Load-time postcondition check: an AABB consumed by frustum culling must have
// finite components and min <= max on every axis. Asserts compile out in
// release; in debug a malformed asset trips here instead of silently breaking
// per-frame cull.
static inline void assertAABBValid(const AABB* aabb)
{
    assert(aabb != NULL);
    assert(isfinite(aabb->min[0]) && isfinite(aabb->min[1]) && isfinite(aabb->min[2]));
    assert(isfinite(aabb->max[0]) && isfinite(aabb->max[1]) && isfinite(aabb->max[2]));
    assert(aabb->min[0] <= aabb->max[0]);
    assert(aabb->min[1] <= aabb->max[1]);
    assert(aabb->min[2] <= aabb->max[2]);
}

void fillSceneWithModelData(SCENE* scene, Model3D* pModel, uint32_t modelLen)
{
    uint32_t indexOffsetForModelIndices   = 0;
    uint32_t indexOffset                  = 0;
    uint32_t vertexOffset                 = 0;
    scene->modelsManager.modelInstanceLen = 0;

    for (int objIndex = 0; objIndex < modelLen; objIndex++)
    {
        scene->modelsManager.pModel[objIndex] = pModel[objIndex];

        Model3D* model = &(scene)->modelsManager.pModel[objIndex];

        // Initialize lifecycle fields for runtime disable/enable/destroy
        model->alive             = 1;
        model->liveInstanceCount = 0;

        model->indiceOffset = indexOffset;
        indexOffset += model->indices.itemsLen;

        model->vertexOffset = vertexOffset;
        vertexOffset += model->meshManager.totalMeshVerticesLen;

        scene->modelsManager.modelInstanceLen += model->instanceCount;
    }

    // Immutable capacity for buffer creation and descriptor range sizing
    scene->modelsManager.modelInstanceCapacity = scene->modelsManager.modelInstanceLen;

    (scene)->modelsManager.packedModelVertexLen = getModelVerticesLenFromScene(scene);
    (scene)->modelsManager.pPackedModelVertex   = getModelVerticesFromScene(scene);

    (scene)->modelsManager.packedModelVertexLeanLen = scene->modelsManager.packedModelVertexLen;
    (scene)->modelsManager.pPackedModelVertexLean   = getModelVerticesLeanFromScene(scene);

    (scene)->modelsManager.packedModelIndex.itemsLen = getModelIndicesLenFromScene(scene);
    (scene)->modelsManager.packedModelIndex.items    = getModelIndicesFromScene((scene));

    // Compute model-space AABBs from packed vertex data
    for (int i = 0; i < modelLen; i++)
    {
        Model3D* model       = &scene->modelsManager.pModel[i];
        uint32_t vertCount   = model->meshManager.totalMeshVerticesLen;
        GEOM_VERTEX* pVerts  = &scene->modelsManager.pPackedModelVertex[model->vertexOffset];
        computeAABBFromVertices(pVerts, vertCount, &model->localAABB);
        recenterVerticesAroundOrigin(pVerts, vertCount, &model->localAABB);
        assertAABBValid(&model->localAABB);

        // Bounding-sphere radius for LOD screen-space selection: half the
        // AABB diagonal. Conservative — never smaller than the true radius.
        float dx = model->localAABB.max[0] - model->localAABB.min[0];
        float dy = model->localAABB.max[1] - model->localAABB.min[1];
        float dz = model->localAABB.max[2] - model->localAABB.min[2];
        model->localAABBRadius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);
    }

    // ---- LOD generation (runs once at scene load) ----
    //
    // For each model:
    //   1. meshopt_optimizeVertexCache on LOD 0 (the full mesh, in place) to
    //      improve vertex reuse in the existing draw path.
    //   2. meshopt_simplify twice to produce LOD 1 (~30% of full indices) and
    //      LOD 2 (~10%). Results appended to packedModelIndex after all LOD 0
    //      ranges. Each LOD shares lod[0]'s vertex buffer in Phase 1 — the
    //      index buffer is authoritative for which vertices each LOD uses.
    //
    // Position stream for simplify: MODEL_VERTEX's point is at offset 0, so
    // &pPackedModelVertexLean[vertexOffset].point is a stride-32 float[3] stream.
    {
        Models3DManager* mm = &scene->modelsManager;

        // meshopt_simplify can return up to index_count when the mesh can't be
        // simplified to the target (e.g. no shared vertices). Reserve worst-case:
        // one full copy of LOD 0 indices per extra LOD level (MODEL_LOD_COUNT - 1).
        uint32_t extraCapacity = 0;
        for (int i = 0; i < modelLen; i++)
        {
            extraCapacity += mm->pModel[i].indices.itemsLen * (MODEL_LOD_COUNT - 1);
        }

        uint32_t baseLen    = mm->packedModelIndex.itemsLen;
        uint32_t reservedLen = baseLen + extraCapacity;
        uint32_t* grown = (uint32_t*) realloc(mm->packedModelIndex.items,
                                              sizeof(uint32_t) * reservedLen);
        assert(grown != NULL);
        mm->packedModelIndex.items = grown;

        // Per-model scratch buffers (sized to largest model).
        uint32_t maxLod0 = 0;
        uint32_t maxVert = 0;
        for (int i = 0; i < modelLen; i++)
        {
            uint32_t ni = mm->pModel[i].indices.itemsLen;
            uint32_t nv = mm->pModel[i].meshManager.totalMeshVerticesLen;
            if (ni > maxLod0) maxLod0 = ni;
            if (nv > maxVert) maxVert = nv;
        }
        uint32_t* remap           = (uint32_t*) malloc(sizeof(uint32_t) * maxVert);
        uint32_t* reverseRemap    = (uint32_t*) malloc(sizeof(uint32_t) * maxVert);
        uint32_t* remappedIdx     = (uint32_t*) malloc(sizeof(uint32_t) * maxLod0);
        uint32_t* simplifyScratch = (uint32_t*) malloc(sizeof(uint32_t) * maxLod0);
        MODEL_VERTEX* compactVerts = (MODEL_VERTEX*) malloc(sizeof(MODEL_VERTEX) * maxVert);
        assert(remap && reverseRemap && remappedIdx && simplifyScratch && compactVerts);

        uint32_t appendCursor = baseLen;
        for (int i = 0; i < modelLen; i++)
        {
            Model3D* model = &mm->pModel[i];

            uint32_t lod0IndexOffset = model->indiceOffset;
            uint32_t lod0IndexCount  = model->indices.itemsLen;
            uint32_t vertCount       = model->meshManager.totalMeshVerticesLen;

            uint32_t* lod0Indices = &mm->packedModelIndex.items[lod0IndexOffset];
            MODEL_VERTEX* leanVerts = &mm->pPackedModelVertexLean[model->vertexOffset];

            // --- Vertex welding ---
            // The OBJ loader unrolls every face corner into a unique vertex
            // (identity indices) to handle split texcoords. meshopt_simplify
            // needs shared-edge topology, so we generate a remap that merges
            // vertices with identical (pos+normal+texcoord) attributes.
            size_t uniqueCount = meshopt_generateVertexRemap(
                remap, lod0Indices, lod0IndexCount,
                leanVerts, vertCount, sizeof(MODEL_VERTEX));

            // Build reverse map: for each compact ID, store one original ID.
            for (uint32_t v = 0; v < vertCount; v++)
                reverseRemap[remap[v]] = v;

            // Remap LOD 0 indices to compact vertex IDs.
            meshopt_remapIndexBuffer(remappedIdx, lod0Indices, lod0IndexCount, remap);

            // Build compact vertex buffer so positions[compactID] is correct.
            meshopt_remapVertexBuffer(compactVerts, leanVerts, vertCount,
                                      sizeof(MODEL_VERTEX), remap);
            const float* positions = (const float*) &compactVerts[0].point;

            // 1. Optimize LOD 0 (remapped) for vertex cache, then write back
            //    to the packed buffer as original vertex IDs.
            meshopt_optimizeVertexCache(remappedIdx, remappedIdx, lod0IndexCount, uniqueCount);
            for (uint32_t k = 0; k < lod0IndexCount; k++)
                lod0Indices[k] = reverseRemap[remappedIdx[k]];

            // LOD 0 bookkeeping
            model->lod[0].vertexOffset = model->vertexOffset;
            model->lod[0].vertexCount  = vertCount;
            model->lod[0].indexOffset  = lod0IndexOffset;
            model->lod[0].indexCount   = lod0IndexCount;
            model->lod[0].minPixelSize = LOD_MIN_PX_FULL;

            // 2. Generate LOD 1 (~30% of full) in compact space, then unremap.
            uint32_t target1 = (uint32_t)((size_t) lod0IndexCount * 3 / 10);
            if (target1 % 3) target1 += 3 - (target1 % 3);
            size_t lod1Count = meshopt_simplify(simplifyScratch, remappedIdx, lod0IndexCount,
                                                positions, uniqueCount, sizeof(MODEL_VERTEX),
                                                target1, /*target_error*/ 0.01f,
                                                /*options*/ 0, /*error_out*/ NULL);
            meshopt_optimizeVertexCache(simplifyScratch, simplifyScratch, lod1Count, uniqueCount);
            for (size_t k = 0; k < lod1Count; k++)
                mm->packedModelIndex.items[appendCursor + k] = reverseRemap[simplifyScratch[k]];

            model->lod[1].vertexOffset = model->vertexOffset;
            model->lod[1].vertexCount  = vertCount;
            model->lod[1].indexOffset  = appendCursor;
            model->lod[1].indexCount   = (uint32_t) lod1Count;
            model->lod[1].minPixelSize = LOD_MIN_PX_MEDIUM;
            appendCursor += (uint32_t) lod1Count;

            // 3. Generate LOD 2 (~10% of full) — same pattern.
            uint32_t target2 = (uint32_t)((size_t) lod0IndexCount * 1 / 10);
            if (target2 % 3) target2 += 3 - (target2 % 3);
            size_t lod2Count = meshopt_simplify(simplifyScratch, remappedIdx, lod0IndexCount,
                                                positions, uniqueCount, sizeof(MODEL_VERTEX),
                                                target2, /*target_error*/ 0.05f,
                                                /*options*/ 0, /*error_out*/ NULL);
            meshopt_optimizeVertexCache(simplifyScratch, simplifyScratch, lod2Count, uniqueCount);
            for (size_t k = 0; k < lod2Count; k++)
                mm->packedModelIndex.items[appendCursor + k] = reverseRemap[simplifyScratch[k]];

            model->lod[2].vertexOffset = model->vertexOffset;
            model->lod[2].vertexCount  = vertCount;
            model->lod[2].indexOffset  = appendCursor;
            model->lod[2].indexCount   = (uint32_t) lod2Count;
            model->lod[2].minPixelSize = LOD_MIN_PX_COARSEST;  // coarsest — always matches
            appendCursor += (uint32_t) lod2Count;

            // Zero the per-frame LOD live counts
            for (int k = 0; k < MODEL_LOD_COUNT; k++) model->lodLiveInstanceCount[k] = 0;

            printf("[lod] type %d: lod0=%u idx, lod1=%zu idx, lod2=%zu idx "
                   "(unique=%zu/%u, radius=%.2f)\n",
                   i, lod0IndexCount, lod1Count, lod2Count,
                   uniqueCount, vertCount, model->localAABBRadius);
        }

        free(compactVerts);
        free(simplifyScratch);
        free(remappedIdx);
        free(reverseRemap);
        free(remap);
        mm->packedModelIndex.itemsLen = appendCursor;
    }

    fillModelInstancesIndices(&scene->modelsManager, &scene->transformManager);
}

void fillLightVolumeInstancesIndices(LightManager* lan, TransformManager* tman)
{

    lan->instanceOffset.items    = malloc(sizeof(*lan->instanceOffset.items) * lan->lightLen);
    lan->instanceOffset.itemsLen = lan->lightLen;

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < lan->lightLen; itemIdx++)
    {
        lan->instanceOffset.items[itemIdx]   = instanceOffset;
        lan->pLight[itemIdx].denseBaseOffset = instanceOffset;
        instanceOffset += lan->pLight[itemIdx].lightVolumeInstanceCount;
    }

    // Allocate single dense array and consolidate per-light staging allocations
    lan->pDenseLightInstances = (LightVolumeInstance*) calloc(lan->lightInstanceCapacity, sizeof(LightVolumeInstance));

    // Allocate transform nodes for light instances (appended after model transforms).
    // Four parallel arrays, grown together.
    uint32_t transformBase = tman->count;
    uint32_t newCount      = tman->count + lan->lightInstanceCapacity;
    if (tman->pTransformsHot == NULL)
    {
        tman->pTransformsHot    = (TransformHot*)   calloc(newCount, sizeof(TransformHot));
        tman->pTransformsWorld  = (TransformWorld*) calloc(newCount, sizeof(TransformWorld));
        tman->pTransformsCold   = (TransformCold*)  calloc(newCount, sizeof(TransformCold));
        tman->pPreorderEntities = (PreorderEntity*) calloc(newCount, sizeof(PreorderEntity));
        tman->pLocalAABBs       = (AABB*)           calloc(newCount, sizeof(AABB));
    }
    else
    {
        tman->pTransformsHot    = (TransformHot*)   realloc(tman->pTransformsHot,    newCount * sizeof(TransformHot));
        tman->pTransformsWorld  = (TransformWorld*) realloc(tman->pTransformsWorld,  newCount * sizeof(TransformWorld));
        tman->pTransformsCold   = (TransformCold*)  realloc(tman->pTransformsCold,   newCount * sizeof(TransformCold));
        tman->pPreorderEntities = (PreorderEntity*) realloc(tman->pPreorderEntities, newCount * sizeof(PreorderEntity));
        tman->pLocalAABBs       = (AABB*)           realloc(tman->pLocalAABBs,       newCount * sizeof(AABB));
        memset(&tman->pTransformsHot[tman->count],    0, lan->lightInstanceCapacity * sizeof(TransformHot));
        memset(&tman->pTransformsWorld[tman->count],  0, lan->lightInstanceCapacity * sizeof(TransformWorld));
        memset(&tman->pTransformsCold[tman->count],   0, lan->lightInstanceCapacity * sizeof(TransformCold));
        memset(&tman->pPreorderEntities[tman->count], 0, lan->lightInstanceCapacity * sizeof(PreorderEntity));
        memset(&tman->pLocalAABBs[tman->count],       0, lan->lightInstanceCapacity * sizeof(AABB));
    }
    tman->count = newCount;

    for (uint32_t l = 0; l < lan->lightLen; l++)
    {
        LightVolume* light = &lan->pLight[l];
        uint32_t     base  = light->denseBaseOffset;
        if (light->pStagingInstances)
        {
            memcpy(&lan->pDenseLightInstances[base], light->pStagingInstances,
                   light->lightVolumeInstanceCount * sizeof(LightVolumeInstance));
            free(light->pStagingInstances);
            light->pStagingInstances = NULL;
        }

        // Set transformIdx and populate transform nodes from staging transforms
        for (uint32_t i = 0; i < light->lightVolumeInstanceCount; i++)
        {
            uint32_t             tIdx = transformBase + base + i;
            LightVolumeInstance* inst = &lan->pDenseLightInstances[base + i];
            inst->transformIdx        = tIdx;

            transformInitHot(&tman->pTransformsHot[tIdx]);

            if (light->pStagingTransforms)
            {
                tman->pTransformsCold[tIdx] = light->pStagingTransforms[i];
            }
            else
            {
                transformInitCold(&tman->pTransformsCold[tIdx]);
            }

            tman->pPreorderEntities[tIdx].kind     = (uint16_t) SCENE_ENTITY_LIGHT;
            tman->pPreorderEntities[tIdx].typeIdx  = (uint16_t) l;
            tman->pPreorderEntities[tIdx].denseIdx = base + i;

            tman->pLocalAABBs[tIdx] = light->localAABB;
        }

        if (light->pStagingTransforms)
        {
            free(light->pStagingTransforms);
            light->pStagingTransforms = NULL;
        }
    }
}
void fillParticleGroupsInstancesIndices(ParticleGroupManager* pgm)
{

    pgm->particleInstanceOffsets.items    = malloc(sizeof(*pgm->particleInstanceOffsets.items) * pgm->particleGroupLen);
    pgm->particleInstanceOffsets.itemsLen = pgm->particleGroupLen;

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < pgm->particleGroupLen; itemIdx++)
    {
        pgm->particleInstanceOffsets.items[itemIdx] = instanceOffset;
        instanceOffset += pgm->pParticleGroup[itemIdx].particleInstancesLen;
    }
}

void fillSceneWithLightData(SCENE* scene, LightVolume* pLight, uint32_t volumetricLightsLen)
{
    uint32_t vertexIndexOffsetForModelIndices = 0;
    uint32_t indexOffset                      = 0;
    uint32_t vertexOffset                     = 0;
    scene->lightManager.lightInstanceCount    = 0;
    // printScene(objects, sceneLen);

    for (int objIndex = 0; objIndex < volumetricLightsLen; objIndex++)
    {
        scene->lightManager.pLight[objIndex] = pLight[objIndex];
        printLightVolume(pLight);
        LightVolume* light = &(scene)->lightManager.pLight[objIndex];

        // Initialize lifecycle fields
        light->alive              = LIGHT_ALIVE;
        light->liveInstanceCount  = 0;
        for (uint32_t i = 0; i < light->lightVolumeInstanceCount; i++)
        {
            light->pStagingInstances[i].lifecycle = LIGHT_INSTANCE_ALIVE;
        }

        // apply_offset_to_indices(light->model.indices.items, light->model.indices.itemsLen,
        // vertexIndexOffsetForModelIndices); vertexIndexOffsetForModelIndices +=
        // light->model.meshManager.totalMeshVerticesLen;
        light->model.indiceOffset = indexOffset;
        light->model.vertexOffset = vertexOffset;
        indexOffset += light->model.indices.itemsLen;
        vertexOffset += light->model.meshManager.totalMeshVerticesLen;
        scene->lightManager.lightInstanceCount += light->lightVolumeInstanceCount;
    }

    // Immutable capacity for buffer creation and descriptor range sizing
    scene->lightManager.lightInstanceCapacity = scene->lightManager.lightInstanceCount;

    (scene)->lightManager.packedLightLen = getVolumetricLightVerticesLenFromScene(scene);
    (scene)->lightManager.pPackedLight   = getLightVolumesVerticesFromScene(scene);

    (scene)->lightManager.packedLightIndex.itemsLen = getVolumetricLightsIndicesLenFromScene(scene);
    (scene)->lightManager.packedLightIndex.items    = getVolumetricLightsIndicesFromScene((scene));
    // Compute model-space AABBs for light volume geometry
    for (int i = 0; i < volumetricLightsLen; i++)
    {
        LightVolume* light   = &scene->lightManager.pLight[i];
        uint32_t     vertCount = light->model.meshManager.totalMeshVerticesLen;
        GEOM_VERTEX* pVerts  = &scene->lightManager.pPackedLight[light->model.vertexOffset];
        computeAABBFromVertices(pVerts, vertCount, &light->localAABB);
        recenterVerticesAroundOrigin(pVerts, vertCount, &light->localAABB);
        assertAABBValid(&light->localAABB);
    }

    fillLightVolumeInstancesIndices(&scene->lightManager, &scene->transformManager);
}

void fillSceneWithParticleData(SCENE* scene, ParticleGroup* particleGroups, uint32_t particleGroupsLen,
                               TrzVertex* particleVertices, uint32_t particleVerticesCount,
                               uint32_t* particleVertexIndices, uint32_t particleVertexIndicesLen)
{

    uint32_t vertexIndexOffsetForModelIndices = 0;
    uint32_t indexOffset                      = 0;
    uint32_t vertexOffset                     = 0;
    uint32_t packedParticleInstancesOffset    = 0;
    scene->particleManager.particleGroupLen   = particleGroupsLen;

    for (int objIndex = 0; objIndex < particleGroupsLen; objIndex++)
    {
        scene->particleManager.pParticleGroup[objIndex] = particleGroups[objIndex];

        for (int particleInstanceIndex = 0;
             particleInstanceIndex < scene->particleManager.pParticleGroup[objIndex].particleInstancesLen;
             particleInstanceIndex++)
        {
            glm_vec4_copy(
                scene->particleManager.pParticleGroup[objIndex].pParticleInstances[particleInstanceIndex].position,
                scene->particleManager.pParticleInstance[packedParticleInstancesOffset].position);

            glm_vec4_copy(
                scene->particleManager.pParticleGroup[objIndex].pParticleInstances[particleInstanceIndex].velocity,
                scene->particleManager.pParticleInstance[packedParticleInstancesOffset].velocity);

            glm_vec4_copy(
                scene->particleManager.pParticleGroup[objIndex].pParticleInstances[particleInstanceIndex].color,
                scene->particleManager.pParticleInstance[packedParticleInstancesOffset].color);

            packedParticleInstancesOffset++;
        }
    }

    scene->particleManager.pPackedParticleVertex     = particleVertices;
    scene->particleManager.packedParticleVertexLen   = particleVerticesCount;
    scene->particleManager.pPackedParticleIndices    = particleVertexIndices;
    scene->particleManager.packedParticleIndicesLen  = particleVertexIndicesLen;
    scene->particleManager.packedParticleInstanceLen = packedParticleInstancesOffset;

    fillParticleGroupsInstancesIndices(&scene->particleManager);
}

// ---------------------------------------------------------------------------
// Runtime model lifecycle — instance-level toggle
// ---------------------------------------------------------------------------

void disableInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[disableInstance] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        fprintf(stderr, "[disableInstance] model %u is permanently destroyed\n", modelIndex);
        return;
    }

    if (instanceIndex >= model->instanceCount)
    {
        fprintf(stderr, "[disableInstance] instanceIndex %u out of range (instanceCount=%u)\n", instanceIndex,
                model->instanceCount);
        return;
    }

    Model3DInstance* inst = &man->pDenseModelInstances[model->denseBaseOffset + instanceIndex];

    if (inst->lifecycle != INSTANCE_ALIVE)
    {
        return;
    }

    inst->lifecycle = INSTANCE_DISABLED;
}

void enableInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[enableInstance] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        fprintf(stderr, "[enableInstance] model %u is permanently destroyed, cannot re-enable\n", modelIndex);
        return;
    }

    if (instanceIndex >= model->instanceCount)
    {
        fprintf(stderr, "[enableInstance] instanceIndex %u out of range (instanceCount=%u)\n", instanceIndex,
                model->instanceCount);
        return;
    }

    Model3DInstance* inst = &man->pDenseModelInstances[model->denseBaseOffset + instanceIndex];

    if (inst->lifecycle == INSTANCE_DESTROYED)
    {
        fprintf(stderr, "[enableInstance] instance %u of model %u is permanently destroyed, cannot re-enable\n",
                instanceIndex, modelIndex);
        return;
    }

    inst->lifecycle = INSTANCE_ALIVE;
}

// ---------------------------------------------------------------------------
// Runtime model lifecycle — model-level toggle
// ---------------------------------------------------------------------------

void disableModel(Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[disableModel] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (model->alive == 0)
    {
        fprintf(stderr, "[disableModel] model %u is permanently destroyed, cannot disable\n", modelIndex);
        return;
    }

    model->alive = 2;
}

void enableModel(Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[enableModel] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (model->alive == 0)
    {
        fprintf(stderr, "[enableModel] model %u is permanently destroyed, cannot re-enable\n", modelIndex);
        return;
    }

    model->alive = 1;
}

// ---------------------------------------------------------------------------
// Runtime model lifecycle — permanent destruction
// ---------------------------------------------------------------------------

void destroyInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[destroyInstance] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        fprintf(stderr, "[destroyInstance] model %u is permanently destroyed\n", modelIndex);
        return;
    }

    if (instanceIndex >= model->instanceCount)
    {
        fprintf(stderr, "[destroyInstance] instanceIndex %u out of range (instanceCount=%u)\n", instanceIndex,
                model->instanceCount);
        return;
    }

    Model3DInstance* inst = &man->pDenseModelInstances[model->denseBaseOffset + instanceIndex];

    if (inst->lifecycle == INSTANCE_DESTROYED)
    {
        fprintf(stderr, "[destroyInstance] instance %u of model %u is already destroyed\n", instanceIndex, modelIndex);
        return;
    }

    inst->lifecycle = INSTANCE_DESTROYED;
}

void destroyModel(Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[destroyModel] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        return;
    }

    // Mark as permanently dead — instances stay in dense array as dead slots
    model->alive = 0;
}

int queryInstanceAlive(const Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
        return -1;

    const Model3D* model = &man->pModel[modelIndex];
    if (!model->alive || instanceIndex >= model->instanceCount)
        return -1;

    return man->pDenseModelInstances[model->denseBaseOffset + instanceIndex].lifecycle;
}

int queryModelAlive(const Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
        return -1;

    return man->pModel[modelIndex].alive;
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — instance-level toggle
// ---------------------------------------------------------------------------

void disableLightInstance(LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[disableLightInstance] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        fprintf(stderr, "[disableLightInstance] light %u is permanently destroyed\n", lightIndex);
        return;
    }

    if (instanceIndex >= light->lightVolumeInstanceCount)
    {
        fprintf(stderr, "[disableLightInstance] instanceIndex %u out of range (count=%u)\n", instanceIndex,
                light->lightVolumeInstanceCount);
        return;
    }

    LightVolumeInstance* inst = &man->pDenseLightInstances[light->denseBaseOffset + instanceIndex];

    if (inst->lifecycle != LIGHT_INSTANCE_ALIVE)
    {
        return;
    }

    inst->lifecycle = LIGHT_INSTANCE_DISABLED;
}

void enableLightInstance(LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[enableLightInstance] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        fprintf(stderr, "[enableLightInstance] light %u is permanently destroyed, cannot re-enable\n", lightIndex);
        return;
    }

    if (instanceIndex >= light->lightVolumeInstanceCount)
    {
        fprintf(stderr, "[enableLightInstance] instanceIndex %u out of range (count=%u)\n", instanceIndex,
                light->lightVolumeInstanceCount);
        return;
    }

    LightVolumeInstance* inst = &man->pDenseLightInstances[light->denseBaseOffset + instanceIndex];

    if (inst->lifecycle == LIGHT_INSTANCE_DESTROYED)
    {
        fprintf(stderr, "[enableLightInstance] instance %u of light %u is permanently destroyed, cannot re-enable\n",
                instanceIndex, lightIndex);
        return;
    }

    inst->lifecycle = LIGHT_INSTANCE_ALIVE;
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — light-level toggle
// ---------------------------------------------------------------------------

void disableLight(LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[disableLight] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (light->alive == LIGHT_DESTROYED)
    {
        fprintf(stderr, "[disableLight] light %u is permanently destroyed, cannot disable\n", lightIndex);
        return;
    }

    light->alive = LIGHT_DISABLED;
}

void enableLight(LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[enableLight] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (light->alive == LIGHT_DESTROYED)
    {
        fprintf(stderr, "[enableLight] light %u is permanently destroyed, cannot re-enable\n", lightIndex);
        return;
    }

    light->alive = LIGHT_ALIVE;
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — permanent destruction
// ---------------------------------------------------------------------------

void destroyLightInstance(LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[destroyLightInstance] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        fprintf(stderr, "[destroyLightInstance] light %u is permanently destroyed\n", lightIndex);
        return;
    }

    if (instanceIndex >= light->lightVolumeInstanceCount)
    {
        fprintf(stderr, "[destroyLightInstance] instanceIndex %u out of range (count=%u)\n", instanceIndex,
                light->lightVolumeInstanceCount);
        return;
    }

    LightVolumeInstance* inst = &man->pDenseLightInstances[light->denseBaseOffset + instanceIndex];

    if (inst->lifecycle == LIGHT_INSTANCE_DESTROYED)
    {
        fprintf(stderr, "[destroyLightInstance] instance %u of light %u is already destroyed\n", instanceIndex,
                lightIndex);
        return;
    }

    inst->lifecycle = LIGHT_INSTANCE_DESTROYED;
}

void destroyLight(LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[destroyLight] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        return;
    }

    light->alive = LIGHT_DESTROYED;
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — queries
// ---------------------------------------------------------------------------

int queryLightInstanceAlive(const LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
        return -1;

    const LightVolume* light = &man->pLight[lightIndex];
    if (light->alive == LIGHT_DESTROYED || instanceIndex >= light->lightVolumeInstanceCount)
        return -1;

    return man->pDenseLightInstances[light->denseBaseOffset + instanceIndex].lifecycle;
}

int queryLightAlive(const LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
        return -1;

    return man->pLight[lightIndex].alive;
}
