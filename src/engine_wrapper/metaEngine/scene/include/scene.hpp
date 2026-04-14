#pragma once

#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C"
{
#include "scene.h"
#include "transform.h"
#include "transformNode.h"
#include "sceneSpec.h"
}

inline void sceneDeletor(SCENE *scene) {
    /* std::cout << "Starting SCENE deletion..." << std::endl;

     if (scene)
     {

         // Free Models3DManager resources
         if (scene->modelsManager.modelLen > 0)
         {

             // Free each model's mesh resources
             for (uint32_t modelIndex = 0; modelIndex < scene->modelsManager.modelLen; modelIndex++)
             {
                 Model3D *currentModel = &scene->modelsManager.pModel[modelIndex];

                 // Free meshes in each model
                 if (currentModel->meshManager.meshesLen > 0)
                 {
                     free(currentModel->meshManager.pMesh);
                 }
             }

             // Free model array
             free(scene->modelsManager.pModel);



             // Free packed vertices if they exist
             if (scene->modelsManager.packedModelVertexLen > 0)
             {
                 free(scene->modelsManager.pPackedModelVertex);
             }
             if (scene->modelsManager.pPackedModelVertexLean)
             {
                 free(scene->modelsManager.pPackedModelVertexLean);
             }

             // Free instanceOffset array
             if (scene->modelsManager.instanceOffset.items != NULL)
             {
                        (void *)scene->modelsManager.instanceOffset.items, scene->modelsManager.instanceOffset.itemsLen);
                 free(scene->modelsManager.instanceOffset.items);
             }
             else
             {
             }
         }
         else
         {
         }

         // Free VolumetricLightManager resources
         if (scene->lightManager.lightLen > 0)
         {

             // Free each volumetric light's resources
             for (uint32_t lightIndex = 0; lightIndex < scene->lightManager.lightLen; lightIndex++)
             {
                 LightVolume *currentLight = &scene->lightManager.pLight[lightIndex];

                 // Free meshes in the light's model
                 if (currentLight->model.meshManager.meshesLen > 0)
                 {
                     free(currentLight->model.meshManager.pMesh);
                 }
             }

             // Free volumetric lights array
             free(scene->lightManager.pLight);

             // Free packed vertices if they exist
             if (scene->lightManager.packedLightLen > 0)
             {
                        scene->lightManager.packedLightLen, (void *)scene->lightManager.pPackedLight);
                 free(scene->lightManager.pPackedLight);
             }
             else
             {
             }

             // Free light instanceOffset array
             if (scene->lightManager.instanceOffset.items != NULL)
             {
                        (void *)scene->lightManager.instanceOffset.items, scene->lightManager.instanceOffset.itemsLen);
                 free(scene->lightManager.instanceOffset.items);
             }
             else
             {
             }
         }
         else
         {
         }

         // Finally free the scene itself
         free(scene);
     }
     else
     {
     }

     std::cout << "SCENE deletion completed" << std::endl;*/
};
// TODO: maybe ditch classes and use namespaces as a general approach, in the end working with the render library
// structs is more natural
class Scene
{

private:
    SCENE *scene_;
    Camera3D *camera;

    std::vector<Model3D> models;
    std::vector<Model3DInstance *> modelsInstances;

    std::vector<LightVolume> lights;
    std::vector<LightVolumeInstance *> lightsInstances;

    std::vector<TrzVertex> particle;
    std::vector<uint32_t> particleIndices;
    std::vector<ParticleInstance *> particleInstances;
    std::vector<ParticleGroup> particleGroups;

public:
    // Default constructor
    Scene()
    {
        std::cout << "DEFAULT CONSTRUCTOR DE SCENE" << std::endl;
    };

    // Copy constructor
    Scene(const Scene &other) = delete;

    const Scene &operator=(const Scene &other) = delete;

    Scene(Scene &&other) noexcept

    {
        this->models = std::move(other.models);
        this->lights = std::move(other.lights);
        this->scene_ = other.scene_;
        std::cout << "Move constructor of Scene called" << std::endl;

        other.models.clear();
        other.lights.clear();
    }

    // Move assignment operator
    Scene &operator=(Scene &&other) noexcept
    {
        this->models = std::move(other.models);
        this->lights = std::move(other.lights);
        std::cout << "Move constructor of Scene called" << std::endl;

        other.models.clear();
        other.lights.clear();
        std::cout << "LLAMAMOS AL MODEL operator= THE METAOBJECT" << std::endl;
        return *this;
    };

    friend std::ostream &operator<<(std::ostream &output, const Scene &scene)
    {
        std::cout << "EN SOBRECARGA OPERADOR <<" << std::endl;
        printSceneModels(&scene.scene_->modelsManager);
        printSceneLights(&scene.scene_->lightManager);

        return output;
    };

    ~Scene() {

    };

    std::vector<Model3D> &getModels()
    {
        return this->models;
    };

    std::vector<LightVolume> &getLights()
    {
        return this->lights;
    };

    Model3D &getModel(uint32_t index)
    {
        return this->models.at(index);
    };

    LightVolume &getVolumetricLight(uint32_t index)
    {
        return this->lights.at(index);
    };

    void addModel(Model3D *model)
    {
        this->models.push_back(*model);
    }

    void addLight(LightVolume *light)
    {
        this->lights.push_back(*light);
    }
    void initParticle()
    {
        TrzVertex v1, v2, v3, v4;

        // Triangle 1: bottom-left, bottom-right, top-left

        vec4 temp1 = {0.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp1, v1.point); // bottom-right
        vec4 temp2 = {0.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp2, v1.color); // bottom-right
        vec4 temp3 = {1.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp3, v1.bary); // bottom-right

        vec4 temp4 = {1.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp4, v2.point); // bottom-right
        vec4 temp5 = {0.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp5, v2.color); // bottom-right
        vec4 temp6 = {0.0f, 1.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp6, v2.bary); // bottom-right

        vec4 temp7 = {0.0f, 1.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp7, v3.point); // bottom-right
        vec4 temp8 = {0.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp8, v3.color); // bottom-right
        vec4 temp9 = {0.0f, 0.0f, 1.0f, 0.0f};
        glm_vec4_copy(temp9, v3.bary); // bottom-right

        vec4 temp10 = {1.0f, 1.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp10, v4.point); // bottom-right
        vec4 temp11 = {0.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp11, v4.color); // bottom-right
        vec4 temp12 = {0.0f, 0.0f, 0.0f, 0.0f};
        glm_vec4_copy(temp12, v4.bary); // bottom-right

        this->particle.push_back(v1);
        this->particle.push_back(v2);
        this->particle.push_back(v3);
        this->particle.push_back(v4);

        this->particleIndices.push_back(0); // bottom-left
        this->particleIndices.push_back(1); // bottom-right
        this->particleIndices.push_back(2); // top-left

        this->particleIndices.push_back(1); // bottom-right
        this->particleIndices.push_back(3); // top-right
        this->particleIndices.push_back(2); // top-left
    }

    void addParticleGroup(ParticleGroup *particleGroup)
    {
        this->particleGroups.push_back(*particleGroup);
    }
    void addCamera3D(Camera3D *camera)
    {
        this->camera = camera;
    }
    Camera3D *getCamera3D()
    {
        return this->camera;
    }
    SCENE *getScene()
    {
        return this->scene_;
    }
    void setScene(SCENE *scene)
    {
        this->scene_ = scene;
    }
    // Light staging data: position and scale passed as arrays alongside instance color data.
    // The old LightVolumeInstance no longer holds position/scale — those go into staging transforms.
    void initInstanceDataLight(LightVolume *lightVolume, uint32_t instanceCount,
                               GEOM_VERTEX_COLOR_RGBA *colors, vec3 *positions, float *scales)
    {
        lightVolume->pStagingInstances  = (LightVolumeInstance *)calloc(instanceCount, sizeof(LightVolumeInstance));
        lightVolume->pStagingTransforms = (TransformCold *)calloc(instanceCount, sizeof(TransformCold));

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {
            LightVolumeInstance *inst = &lightVolume->pStagingInstances[instanceIndex];
            inst->color     = colors[instanceIndex];
            inst->lifecycle = LIGHT_INSTANCE_ALIVE;

            TransformCold *cold = &lightVolume->pStagingTransforms[instanceIndex];
            transformInitCold(cold);
            transformSetPosition(cold, positions[instanceIndex]);
            transformSetScaleUniform(cold, scales[instanceIndex]);
        }

        lightVolume->lightVolumeInstanceCount = instanceCount;
    }

    // Grid layout: one light per grid cell, matching initInstanceDataModel grid
    void initLightsGrid(LightVolume *lightVolume, uint32_t instanceCount,
                        int gridCols, float gridSpacing)
    {
        GEOM_VERTEX_COLOR_RGBA *colors = (GEOM_VERTEX_COLOR_RGBA *)malloc(sizeof(GEOM_VERTEX_COLOR_RGBA) * instanceCount);
        vec3  *positions = (vec3 *)malloc(sizeof(vec3) * instanceCount);
        float *scales    = (float *)malloc(sizeof(float) * instanceCount);

        for (uint32_t i = 0; i < instanceCount; i++)
        {
            int col = i % gridCols;
            int row = i / gridCols;
            positions[i][0] = (col - (gridCols - 1) / 2.0f) * gridSpacing;
            positions[i][1] = (row - 2.0f) * gridSpacing;
            positions[i][2] = 0.0f;
            colors[i] = (i % 2 == 0) ? GEOM_VERTEX_COLOR_RGBA{0.0f, 1.0f, 0.0f, 1.0f}
                                      : GEOM_VERTEX_COLOR_RGBA{1.0f, 0.0f, 0.0f, 1.0f};
            scales[i] = 1.0f;
        }
        initInstanceDataLight(lightVolume, instanceCount, colors, positions, scales);
        free(colors); free(positions); free(scales);
    }

    // Two groups of overlapping cubes covering both model planes from initInstanceDataModelDualPlane.
    // perSide lights at each plane position (x = ±planeGap).
    void initLightsDualPlane(LightVolume *lightVolume, float planeGap, float cubeScale, uint32_t perSide)
    {
        uint32_t count = perSide * 2;
        GEOM_VERTEX_COLOR_RGBA *colors = (GEOM_VERTEX_COLOR_RGBA *)malloc(sizeof(GEOM_VERTEX_COLOR_RGBA) * count);
        vec3  *positions = (vec3 *)malloc(sizeof(vec3) * count);
        float *scales    = (float *)malloc(sizeof(float) * count);

        for (uint32_t i = 0; i < count; i++)
        {
            bool rightSide = (i < perSide);
            positions[i][0] = rightSide ? planeGap : -planeGap;
            positions[i][1] = 0.0f;
            positions[i][2] = 0.0f;
            colors[i] = rightSide ? GEOM_VERTEX_COLOR_RGBA{0.0f, 1.0f, 0.0f, 1.0f}
                                  : GEOM_VERTEX_COLOR_RGBA{1.0f, 0.0f, 0.0f, 1.0f};
            scales[i] = cubeScale;
        }
        initInstanceDataLight(lightVolume, count, colors, positions, scales);
        free(colors); free(positions); free(scales);
    }

    void initInstanceDataParticle(ParticleGroup *particleGroup, uint32_t instanceCount, ParticleInstance *instanceData)
    {

        particleGroup->pParticleInstances = (ParticleInstance *)malloc(sizeof(ParticleInstance) * instanceCount);

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {

            assert(particleGroup->pParticleInstances != 0);
            glm_vec4_copy(instanceData[instanceIndex].position, particleGroup->pParticleInstances[instanceIndex].position);
            glm_vec4_copy(instanceData[instanceIndex].color, particleGroup->pParticleInstances[instanceIndex].color);
            glm_vec4_copy(instanceData[instanceIndex].velocity, particleGroup->pParticleInstances[instanceIndex].velocity);
            this->particleInstances.push_back(&instanceData[instanceIndex]);
        }

        particleGroup->particleInstancesLen = instanceCount;

        // free(instanceData);
    }
    // Two parallel planes of models at x = ±planeGap, each with instanceCount/2
    // models spread in a YZ grid. Designed for two overlapping light cubes.
    void initInstanceDataModelDualPlane(Model3D *pModel, uint32_t instanceCount)
    {
        pModel->pStagingInstances  = (Model3DInstance *)calloc(instanceCount, sizeof(Model3DInstance));
        pModel->pStagingTransforms = (TransformCold *)calloc(instanceCount, sizeof(TransformCold));

        uint32_t half     = instanceCount / 2;
        uint32_t gridSide = (uint32_t)sqrtf((float)half);
        if (gridSide * gridSide < half) gridSide++;

        const float gridSpacing = 1.0f;
        const float planeGap    = 5.0f; // half-distance between the two planes

        for (uint32_t i = 0; i < instanceCount; i++)
        {
            Model3DInstance *inst = &pModel->pStagingInstances[i];
            inst->lifecycle = INSTANCE_ALIVE;
            inst->color = (GEOM_VERTEX_COLOR_RGBA){1.0f, 1.0f, 1.0f, 1.0f};

            TransformCold *cold = &pModel->pStagingTransforms[i];
            transformInitCold(cold);

            bool     rightPlane = (i < half);
            uint32_t localIdx   = rightPlane ? i : (i - half);
            uint32_t col = localIdx % gridSide;
            uint32_t row = localIdx / gridSide;

            vec3 pos = {
                rightPlane ? planeGap : -planeGap,
                (col - gridSide / 2.0f) * gridSpacing,
                (row - gridSide / 2.0f) * gridSpacing
            };
            transformSetPosition(cold, pos);
        }
        pModel->instanceCount = instanceCount;
    }

    void initInstanceDataModel(Model3D *pModel, uint32_t instanceCount)
    {
        pModel->pStagingInstances  = (Model3DInstance *)calloc(instanceCount, sizeof(Model3DInstance));
        pModel->pStagingTransforms = (TransformCold *)calloc(instanceCount, sizeof(TransformCold));

        // Spread instances in a 10-column grid matching the light layout
        const float gridSpacing = 8.0f;
        const int   gridCols    = 5;

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {
            Model3DInstance *inst = &pModel->pStagingInstances[instanceIndex];
            inst->lifecycle = INSTANCE_ALIVE;
            inst->color = (GEOM_VERTEX_COLOR_RGBA){1.0f, 1.0f, 1.0f, 1.0f};

            TransformCold *cold = &pModel->pStagingTransforms[instanceIndex];
            transformInitCold(cold);

            int col = instanceIndex % gridCols;
            int row = instanceIndex / gridCols;
            vec3 pos = {
                (col - (gridCols - 1) / 2.0f) * gridSpacing,
                (row - 2.0f) * gridSpacing,
                0.0f
            };
            transformSetPosition(cold, pos);
        }
        pModel->instanceCount = instanceCount;
    }

    void init()
    {

        SCENE *scene;
        initScene(&scene, this->models.size(), this->lights.size(), this->particle.size(), this->particleGroups.size(),
                  this->particleInstances.size());

        fillSceneWithModelData(scene, this->models.data(), this->models.size());

        fillSceneWithLightData(scene, this->lights.data(), this->lights.size());

        fillSceneWithParticleData(scene, this->particleGroups.data(), this->particleGroups.size(),
                                  this->particle.data(), this->particle.size(), this->particleIndices.data(),
                                  this->particleIndices.size());

        /*Model3DInstance **modelInstances = this->modelsInstances.data();
        fillSceneWithInstancedModelData(&scene->modelsManager, modelInstances, this->modelsInstances.size());

        LightVolumeInstance **lightInstance = this->lightsInstances.data();
        fillSceneWithInstancedLightVolumeData(&scene->lightManager, lightInstance, this->lightsInstances.size());

        printSceneModels(&scene->modelsManager);
        */
        scene->sceneInstancesLen = scene->modelsManager.modelInstanceLen + scene->lightManager.lightInstanceCount;

        this->scene_ = &(*scene);
    }

    // Rebuild the scene's transform + dense instance arrays using a spec tree
    // that parents every model instance under one of 2 light roots (one per
    // plane). Must be called AFTER init() so per-type metadata (packed vertex
    // buffers, localAABBs) is already populated. Replaces the flat layout
    // created by init() with a preorder hierarchy layout.
    //
    // World-space positions match the original dual-plane layout. Because the
    // light roots have scale = cubeScale > 1, each model child's local TRS is
    // compensated so its world scale stays at 1.
    void buildAndApplyDualPlaneSpecTree(uint32_t modelInstanceCount, float planeGap, float cubeScale)
    {
        uint32_t perSide = modelInstanceCount / 2;

        uint32_t gridSide = (uint32_t) sqrtf((float) perSide);
        if (gridSide * gridSide < perSide) gridSide++;
        const float gridSpacing = 1.0f;

        SceneEntitySpec *roots[2] = {nullptr, nullptr};

        for (int side = 0; side < 2; side++)
        {
            float                  planeX = side == 0 ? planeGap : -planeGap;
            GEOM_VERTEX_COLOR_RGBA lightColor;
            if (side == 0)
            {
                lightColor.r = 0.0f; lightColor.g = 1.0f; lightColor.b = 0.0f; lightColor.a = 1.0f;
            }
            else
            {
                lightColor.r = 1.0f; lightColor.g = 0.0f; lightColor.b = 0.0f; lightColor.a = 1.0f;
            }

            // Light root at (planeX, 0, 0) with scale = cubeScale.
            vec3             rootPos   = {planeX, 0.0f, 0.0f};
            SceneEntitySpec *lightRoot = sceneSpecNewLight(0 /*type idx*/, rootPos, cubeScale, lightColor);

            // Model children. Each child's LOCAL TRS must compensate for the
            // parent's scale so its world transform matches the original
            // flat-layout intent: world position = (planeX, gridY, gridZ),
            // world scale = 1.
            //
            //   childWorldScale = parentScale * childLocalScale
            //   1              = cubeScale    * childLocalScale
            //   => childLocalScale = 1 / cubeScale
            //
            //   childWorldPos = parentPos + parentScale * childLocalPos
            //   (planeX, gY, gZ) = (planeX, 0, 0) + cubeScale * childLocalPos
            //   => childLocalPos = (0, gY / cubeScale, gZ / cubeScale)
            float invCube = 1.0f / cubeScale;

            GEOM_VERTEX_COLOR_RGBA white;
            white.r = 1.0f; white.g = 1.0f; white.b = 1.0f; white.a = 1.0f;

            for (uint32_t i = 0; i < perSide; i++)
            {
                uint32_t col = i % gridSide;
                uint32_t row = i / gridSide;
                float    gY  = (col - gridSide / 2.0f) * gridSpacing;
                float    gZ  = (row - gridSide / 2.0f) * gridSpacing;

                vec3             localPos = {0.0f, gY * invCube, gZ * invCube};
                SceneEntitySpec *child    = sceneSpecNewModel(0 /*type idx*/, localPos, white);
                child->scale[0]           = invCube;
                child->scale[1]           = invCube;
                child->scale[2]           = invCube;

                sceneSpecAddChild(lightRoot, child);
            }

            roots[side] = lightRoot;
        }

        fillSceneFromSpecTree(this->scene_, roots, 2);

        this->scene_->sceneInstancesLen =
            this->scene_->modelsManager.modelInstanceLen + this->scene_->lightManager.lightInstanceCount;

        sceneSpecFree(roots[0]);
        sceneSpecFree(roots[1]);
    }

    // Build a scene organized as a grid of groups, where each group is a
    // light root parenting its models. Designed to give hierarchical frustum
    // culling something real to skip: each group's light volume is sized to
    // tightly bound its children, so when the camera flies around, most
    // groups fall entirely outside the frustum and their subtrees are
    // bulk-skipped via the skipCount fast path.
    //
    // Layout:
    //   - groupsPerSide × groupsPerSide groups arranged on the XY plane
    //     at Z=0, centered at the origin with `groupSpacing` between
    //     group centers.
    //   - Each group has one light root at the group center with uniform
    //     scale `lightScale`. The light mesh is the standard 10-unit cube,
    //     so at lightScale=1 the world AABB is 10×10×10 per group.
    //   - Each group contains modelsPerGroupX × modelsPerGroupY model
    //     children arranged on the group's local XY plane at `modelSpacing`
    //     apart. Child local TRS is compensated so child world scale = 1
    //     regardless of lightScale.
    //
    // Must be called AFTER init() so per-type metadata is populated.
    // Replaces any existing dense-array layout with a preorder hierarchy.
    void buildAndApplyGroupedSpecTree(uint32_t groupsPerSide,
                                      uint32_t modelsPerGroupX,
                                      uint32_t modelsPerGroupY,
                                      float    groupSpacing,
                                      float    modelSpacing,
                                      float    lightScale,
                                      float    modelScale)
    {
        uint32_t totalGroups    = groupsPerSide * groupsPerSide;
        uint32_t modelsPerGroup = modelsPerGroupX * modelsPerGroupY;

        SceneEntitySpec **roots = (SceneEntitySpec **)malloc(totalGroups * sizeof(SceneEntitySpec *));

        // Inverse of parent scale — used to compensate child TRS so that
        // childWorldScale = parentScale * childLocalScale = modelScale.
        // Position compensation is independent of modelScale (parent scale
        // cancels out regardless) so modelSpacing units are world-space.
        float invLight       = 1.0f / lightScale;
        float childLocalScale = modelScale * invLight;

        GEOM_VERTEX_COLOR_RGBA white;
        white.r = 1.0f; white.g = 1.0f; white.b = 1.0f; white.a = 1.0f;

        for (uint32_t gi = 0; gi < totalGroups; gi++)
        {
            uint32_t col = gi % groupsPerSide;
            uint32_t row = gi / groupsPerSide;

            float groupX = (col - (groupsPerSide - 1) / 2.0f) * groupSpacing;
            float groupY = (row - (groupsPerSide - 1) / 2.0f) * groupSpacing;
            float groupZ = 0.0f;

            // Checkerboard color so visually distinct groups are easy to spot
            GEOM_VERTEX_COLOR_RGBA lightColor;
            if ((col + row) % 2 == 0)
            {
                lightColor.r = 0.2f; lightColor.g = 1.0f; lightColor.b = 0.3f; lightColor.a = 1.0f;
            }
            else
            {
                lightColor.r = 1.0f; lightColor.g = 0.3f; lightColor.b = 0.2f; lightColor.a = 1.0f;
            }

            vec3             rootPos   = {groupX, groupY, groupZ};
            SceneEntitySpec *lightRoot = sceneSpecNewLight(0 /*light type*/, rootPos, lightScale, lightColor);

            // Models arranged on the group's local XY plane.
            for (uint32_t mx = 0; mx < modelsPerGroupX; mx++)
            {
                for (uint32_t my = 0; my < modelsPerGroupY; my++)
                {
                    float worldOffsetX = (mx - (modelsPerGroupX - 1) / 2.0f) * modelSpacing;
                    float worldOffsetY = (my - (modelsPerGroupY - 1) / 2.0f) * modelSpacing;

                    // Compensate local position for the parent's scale cascade.
                    vec3             localPos = {worldOffsetX * invLight, worldOffsetY * invLight, 0.0f};
                    SceneEntitySpec *child    = sceneSpecNewModel(0 /*model type*/, localPos, white);
                    child->scale[0]           = childLocalScale;
                    child->scale[1]           = childLocalScale;
                    child->scale[2]           = childLocalScale;

                    sceneSpecAddChild(lightRoot, child);
                }
            }

            roots[gi] = lightRoot;
        }

        fillSceneFromSpecTree(this->scene_, roots, totalGroups);

        this->scene_->sceneInstancesLen =
            this->scene_->modelsManager.modelInstanceLen + this->scene_->lightManager.lightInstanceCount;

        for (uint32_t i = 0; i < totalGroups; i++)
            sceneSpecFree(roots[i]);
        free(roots);

        (void) modelsPerGroup; // silence unused-variable warning; kept for future asserts
    }
};
