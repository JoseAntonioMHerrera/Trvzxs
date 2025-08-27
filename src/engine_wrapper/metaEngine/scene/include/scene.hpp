#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C"
{
#include "scene.h"
}

void sceneDeletor(SCENE *scene)
{
    std::cout << "Starting SCENE deletion..." << std::endl;
    printf("DEBUG: sceneDeletor called with scene pointer: %p\n", (void *)scene);

    if (scene)
    {
        printf("DEBUG: Scene is valid, proceeding with deletion\n");
        printf("DEBUG: Models count: %u, Lights count: %u\n", scene->modelsManager.modelLen, scene->lightManager.lightLen);

        // Free Models3DManager resources
        if (scene->modelsManager.modelLen > 0)
        {
            printf("DEBUG: Freeing %u models\n", scene->modelsManager.modelLen);

            // Free each model's mesh resources
            for (uint32_t modelIndex = 0; modelIndex < scene->modelsManager.modelLen; modelIndex++)
            {
                printf("DEBUG: Processing model %u/%u\n", modelIndex, scene->modelsManager.modelLen - 1);
                Model3D *currentModel = &scene->modelsManager.pModel[modelIndex];
                printf("DEBUG: Current model address: %p\n", (void *)currentModel);

                // Free meshes in each model
                if (currentModel->meshManager.meshesLen > 0)
                {
                    printf("DEBUG: Freeing %u meshes for model %u\n", currentModel->meshManager.meshesLen, modelIndex);
                    free(currentModel->meshManager.pMesh);
                    printf("DEBUG: Meshes freed for model %u\n", modelIndex);
                }
            }

            // Free model array
            printf("DEBUG: Freeing pModel array at %p\n", (void *)scene->modelsManager.pModel);
            free(scene->modelsManager.pModel);
            printf("DEBUG: pModel array freed\n");

            // Free model instances if they exist
            /*if (scene->modelsManager.modelInstanceLen > 0) {
                free(scene->modelsManager.pModelInstance);
            }*/

            // Free packed vertices if they exist
            if (scene->modelsManager.packedModelVertexLen > 0)
            {
                printf("DEBUG: Freeing packed vertices (%u vertices) at %p\n",
                       scene->modelsManager.packedModelVertexLen, (void *)scene->modelsManager.pPackedModelVertex);
                free(scene->modelsManager.pPackedModelVertex);
                printf("DEBUG: Packed vertices freed\n");
            }
            else
            {
                printf("DEBUG: No packed vertices to free (packedModelVertexLen = %u)\n", scene->modelsManager.packedModelVertexLen);
            }

            // Free instanceOffset array
            if (scene->modelsManager.instanceOffset.items != NULL)
            {
                printf("DEBUG: Freeing instanceOffset array at %p (itemsLen = %u)\n",
                       (void *)scene->modelsManager.instanceOffset.items, scene->modelsManager.instanceOffset.itemsLen);
                free(scene->modelsManager.instanceOffset.items);
                printf("DEBUG: instanceOffset array freed\n");
            }
            else
            {
                printf("DEBUG: instanceOffset.items is NULL, nothing to free\n");
            }
        }
        else
        {
            printf("DEBUG: No models to free (modelLen = %u)\n", scene->modelsManager.modelLen);
        }

        // Free VolumetricLightManager resources
        if (scene->lightManager.lightLen > 0)
        {
            printf("DEBUG: Freeing %u lights\n", scene->lightManager.lightLen);

            // Free each volumetric light's resources
            for (uint32_t lightIndex = 0; lightIndex < scene->lightManager.lightLen; lightIndex++)
            {
                printf("DEBUG: Processing light %u/%u\n", lightIndex, scene->lightManager.lightLen - 1);
                LightVolume *currentLight = &scene->lightManager.pLight[lightIndex];
                printf("DEBUG: Current light address: %p\n", (void *)currentLight);

                // Free meshes in the light's model
                if (currentLight->model.meshManager.meshesLen > 0)
                {
                    printf("DEBUG: Freeing %u meshes for light %u\n", currentLight->model.meshManager.meshesLen, lightIndex);
                    free(currentLight->model.meshManager.pMesh);
                    printf("DEBUG: Light meshes freed for light %u\n", lightIndex);
                }
            }

            // Free volumetric lights array
            printf("DEBUG: Freeing pLight array at %p\n", (void *)scene->lightManager.pLight);
            free(scene->lightManager.pLight);
            printf("DEBUG: pLight array freed\n");

            // Free packed vertices if they exist
            if (scene->lightManager.packedLightLen > 0)
            {
                printf("DEBUG: Freeing packed lights (%u lights) at %p\n",
                       scene->lightManager.packedLightLen, (void *)scene->lightManager.pPackedLight);
                free(scene->lightManager.pPackedLight);
                printf("DEBUG: Packed lights freed\n");
            }
            else
            {
                printf("DEBUG: No packed lights to free (packedLightLen = %u)\n", scene->lightManager.packedLightLen);
            }

            // Free light instanceOffset array
            if (scene->lightManager.instanceOffset.items != NULL)
            {
                printf("DEBUG: Freeing light instanceOffset array at %p (itemsLen = %u)\n",
                       (void *)scene->lightManager.instanceOffset.items, scene->lightManager.instanceOffset.itemsLen);
                free(scene->lightManager.instanceOffset.items);
                printf("DEBUG: Light instanceOffset array freed\n");
            }
            else
            {
                printf("DEBUG: Light instanceOffset.items is NULL, nothing to free\n");
            }
        }
        else
        {
            printf("DEBUG: No lights to free (lightLen = %u)\n", scene->lightManager.lightLen);
        }

        // Finally free the scene itself
        printf("DEBUG: Freeing scene structure at %p\n", (void *)scene);
        free(scene);
        printf("DEBUG: Scene structure freed\n");
    }
    else
    {
        printf("DEBUG: Scene pointer is NULL, nothing to delete\n");
    }

    std::cout << "SCENE deletion completed" << std::endl;
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

    std::vector<GEOM_VERTEX> particle;
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
        GEOM_VERTEX v1, v2, v3, v4;

        // Triangle 1: bottom-left, bottom-right, top-left
        v1.point = {0.0, 0.0, 0.0}; // bottom-right
        v1.color = {0.0, 0.0, 0.0, 1.0};
        v1.bary = {1.0, 0.0, 0.0};
        v2.point = {1.0, 0.0, 0.0}; // bottom-left
        v2.color = {0.0, 0.0, 0.0, 1.0};
        v2.bary = {0.0, 1.0, 0.0};
        v3.point = {0.0, 1.0, 0.0}; // top-left
        v3.color = {0.0, 0.0, 0.0, 1.0};
        v3.bary = {0.0, 0.0, 1.0};
        v4.point = {1.0, 1.0, 0.0}; // top-right
        v4.color = {0.0, 0.0, 0.0, 1.0};
        v4.bary = {0.0, 0.0, 1.0};

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
    void initInstanceDataLight(LightVolume *lightVolume, uint32_t instanceCount, LightVolumeInstance *instanceData)
    {
        printf("DEBUG: initInstanceDataLight START - lightVolume=%p, instanceCount=%u\n", lightVolume, instanceCount);

        lightVolume->pLightVolumeInstance = (LightVolumeInstance *)malloc(sizeof(LightVolumeInstance) * instanceCount);
        printf("DEBUG: Allocated pLightVolumeInstance array at %p, size=%zu bytes\n",
               lightVolume->pLightVolumeInstance, sizeof(LightVolumeInstance) * instanceCount);

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {
            printf("DEBUG: Creating light instance %u/%u\n", instanceIndex, instanceCount);

            LightVolumeInstance *lightInstance = (LightVolumeInstance *)malloc(sizeof(LightVolumeInstance));
            printf("DEBUG: Allocated individual lightInstance at %p\n", lightInstance);

            assert(lightVolume->pLightVolumeInstance != 0);
            glm_vec4_copy(instanceData[instanceIndex].position, lightInstance->position);

            lightInstance->scale = instanceData[instanceIndex].scale;
            lightInstance->color = instanceData[instanceIndex].color;

            lightVolume->pLightVolumeInstance[instanceIndex] = *lightInstance;

            printf("DEBUG: Copied light instance %u to array\n", instanceIndex);
        }

        lightVolume->lightVolumeInstanceCount = instanceCount;

        printf("DEBUG: initInstanceDataLight END\n");
    }

    void initInstanceDataParticle(ParticleGroup *particleGroup, uint32_t instanceCount, ParticleInstance *instanceData)
    {
        printf("DEBUG: initInstanceDataParticle START - particleGroup=%p, instanceCount=%u\n", particleGroup, instanceCount);

        particleGroup->pParticleInstances = (ParticleInstance *)malloc(sizeof(ParticleInstance) * instanceCount);
        printf("DEBUG: Allocated pParticle array at %p, size=%zu bytes\n",
               particleGroup->pParticleInstances, sizeof(ParticleInstance) * instanceCount);

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {

            ParticleInstance *particleInstance = (ParticleInstance *)malloc(sizeof(ParticleInstance));

            assert(particleGroup->pParticleInstances != 0);
            particleInstance->position = instanceData[instanceIndex].position;

            particleInstance->color = instanceData[instanceIndex].color;
            particleInstance->velocity = instanceData[instanceIndex].velocity;

            particleGroup->pParticleInstances[instanceIndex] = *particleInstance;
        }

        particleGroup->particleInstancesLen = instanceCount;

        free(instanceData);

        printf("DEBUG: initInstanceDataParticle %d END\n", particleGroup->particleInstancesLen);
    }
    // TODO adaptar al estilo de initInstanceLightVolumes
    void initInstanceDataModel(Model3D *pModel, uint32_t instanceCount)
    {
        printf("DEBUG: initInstanceDataModel START - pModel=%p, instanceCount=%u\n", pModel, instanceCount);
        pModel->pModelInstance = (Model3DInstance *)malloc(sizeof(Model3DInstance) * instanceCount);
        printf("DEBUG: pModel->pModelInstance=%p\n", pModel->pModelInstance);
        printf("DEBUG: Allocated pModelInstance array at %p, size=%zu bytes\n",
               pModel->pModelInstance, sizeof(Model3D) * instanceCount);

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
        {
            printf("DEBUG: Creating model instance %u/%u\n", instanceIndex, instanceCount);

            Model3DInstance *modelInstance = (Model3DInstance *)malloc(sizeof(Model3DInstance));
            printf("DEBUG: Allocated individual vikingRoomInstance at %p\n", modelInstance);
            vec3 vRPosition = {(instanceIndex == 0 ? 5 : 0), (instanceIndex == 0 ? 0 : 0), 0.0f};
            glm_translate_make(modelInstance->m, vRPosition);
            // Print model matrix values
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    printf("Model[%d][%d] = %.6f ", i, j, modelInstance->m[i][j]);
                }
                printf("\n");
            }
            vec3 vRScale = {1.0f, 1.0f, 1.0f};
            glm_scale(modelInstance->m, vRScale);
            pModel->pModelInstance[instanceIndex] = *modelInstance;
            printf("DEBUG: Copied model instance %u to array\n", instanceIndex);
        }
        pModel->instanceCount = instanceCount;
        printf("DEBUG: modelInstanceCount pModel=%p, instanceCount=%u\n", pModel, pModel->instanceCount);
        printf("DEBUG: initInstanceDataModel END\n");
    }

    void init()
    {
        printf("=== SCENE INITIALIZATION START ===\n");
        printf("Models count: %zu\n", this->models.size());
        printf("Lights count: %zu\n", this->lights.size());
        printf("Model instances count: %zu\n", this->modelsInstances.size());
        printf("Light instances count: %zu\n", this->lightsInstances.size());

        SCENE *scene;
        printf("Initializing scene with %zu models and %zu lights\n", this->models.size(), this->lights.size());
        initScene(&scene, this->models.size(), this->lights.size(), this->particle.size(), this->particleGroups.size());
        printf("Scene initialized at address: %p\n", scene);

        printf("Filling scene with model data (%zu models)\n", this->models.size());
        fillSceneWithModelData(scene, this->models.data(), this->models.size());
        printf("Model data filled successfully\n");

        printf("Filling scene with light data (%zu lights)\n", this->lights.size());
        fillSceneWithLightData(scene, this->lights.data(), this->lights.size());
        printf("Light data filled successfully\n");

        printf("Filling scene with particle data (%zu particles)\n", this->lights.size());
        fillSceneWithParticleData(scene, this->particleGroups.data(), this->particleGroups.size(),
                                  this->particle.data(), this->particle.size(), this->particleIndices.data(),
                                  this->particleIndices.size());
        printf("particle data filled successfully\n");

        /*Model3DInstance **modelInstances = this->modelsInstances.data();
        printf("Model instances array address: %p\n", modelInstances);
        printf("Filling scene with instanced model data (%zu instances)\n", this->modelsInstances.size());
        fillSceneWithInstancedModelData(&scene->modelsManager, modelInstances, this->modelsInstances.size());
        printf("Instanced model data filled successfully\n");*/

        /*LightVolumeInstance **lightInstance = this->lightsInstances.data();
        printf("Light instances array address: %p\n", lightInstance);
        printf("Filling scene with instanced light volume data (%zu instances)\n", this->lightsInstances.size());
        fillSceneWithInstancedLightVolumeData(&scene->lightManager, lightInstance, this->lightsInstances.size());
        printf("Instanced light volume data filled successfully\n");*/

        printf("Printing scene models (before scene assignment)\n");
        printSceneModels(&scene->modelsManager);

        scene->sceneInstancesLen = this->modelsInstances.size() + this->lightsInstances.size();
        printf("Total scene instances: %zu (models: %zu + lights: %zu)\n",
               scene->sceneInstancesLen, this->modelsInstances.size(), this->lightsInstances.size());

        printf("Assigning scene to smart pointer\n");
        this->scene_ = &(*scene);
        printf("Scene assigned to smart pointer successfully %d\n", this->scene_->modelsManager.modelInstanceLen);
        printf("Printing scene models (after scene assignment)\n");
        printSceneModels(&this->scene_->modelsManager);

        printf("=== SCENE INITIALIZATION COMPLETE ===\n");
    }
};
