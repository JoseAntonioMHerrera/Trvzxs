#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C"
{
#include "scene.h"
#include "transform.h"
#include "transformNode.h"
}

void sceneDeletor(SCENE *scene) {
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
                        scene->modelsManager.packedModelVertexLen, (void *)scene->modelsManager.pPackedModelVertex);
                 free(scene->modelsManager.pPackedModelVertex);
             }
             else
             {
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
        lightVolume->pStagingTransforms = (TransformNode *)calloc(instanceCount, sizeof(TransformNode));

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {
            LightVolumeInstance *inst = &lightVolume->pStagingInstances[instanceIndex];
            inst->color     = colors[instanceIndex];
            inst->lifecycle = LIGHT_INSTANCE_ALIVE;

            TransformNode *node = &lightVolume->pStagingTransforms[instanceIndex];
            transformInit(node);
            transformSetPosition(node, positions[instanceIndex]);
            transformSetScaleUniform(node, scales[instanceIndex]);
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
        pModel->pStagingTransforms = (TransformNode *)calloc(instanceCount, sizeof(TransformNode));

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

            TransformNode *node = &pModel->pStagingTransforms[i];
            transformInit(node);

            bool     rightPlane = (i < half);
            uint32_t localIdx   = rightPlane ? i : (i - half);
            uint32_t col = localIdx % gridSide;
            uint32_t row = localIdx / gridSide;

            vec3 pos = {
                rightPlane ? planeGap : -planeGap,
                (col - gridSide / 2.0f) * gridSpacing,
                (row - gridSide / 2.0f) * gridSpacing
            };
            transformSetPosition(node, pos);
        }
        pModel->instanceCount = instanceCount;
    }

    void initInstanceDataModel(Model3D *pModel, uint32_t instanceCount)
    {
        pModel->pStagingInstances  = (Model3DInstance *)calloc(instanceCount, sizeof(Model3DInstance));
        pModel->pStagingTransforms = (TransformNode *)calloc(instanceCount, sizeof(TransformNode));

        // Spread instances in a 10-column grid matching the light layout
        const float gridSpacing = 8.0f;
        const int   gridCols    = 5;

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {
            Model3DInstance *inst = &pModel->pStagingInstances[instanceIndex];
            inst->lifecycle = INSTANCE_ALIVE;
            inst->color = (GEOM_VERTEX_COLOR_RGBA){1.0f, 1.0f, 1.0f, 1.0f};

            TransformNode *node = &pModel->pStagingTransforms[instanceIndex];
            transformInit(node);

            int col = instanceIndex % gridCols;
            int row = instanceIndex / gridCols;
            vec3 pos = {
                (col - (gridCols - 1) / 2.0f) * gridSpacing,
                (row - 2.0f) * gridSpacing,
                0.0f
            };
            transformSetPosition(node, pos);
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
};
