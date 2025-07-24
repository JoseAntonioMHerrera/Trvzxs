#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C" {
    #include "scene.h"
}

constexpr auto sceneDeletor = [](SCENE* scene) {
    std::cout << "Starting SCENE deletion..." << std::endl;

    if (scene) {
        // Free Models3DManager resources
        if (scene->modelsManager.modelLen > 0) {
            // Free each model's mesh resources
            for (uint32_t modelIndex = 0; modelIndex < scene->modelsManager.modelLen; modelIndex++) {
                Model3D* currentModel = &scene->modelsManager.pModel[modelIndex];

                // Free meshes in each model
                if (currentModel->meshManager.meshesLen > 0) {
                    free(currentModel->meshManager.pMesh);
                }
            }

            // Free model array
            free(scene->modelsManager.pModel);

            // Free model instances if they exist
            /*if (scene->modelsManager.modelInstanceLen > 0) {
                free(scene->modelsManager.pModelInstance);
            }*/

            // Free packed vertices if they exist
            if (scene->modelsManager.packedModelVertexLen > 0) {
                free(scene->modelsManager.pPackedModelVertex);
            }
        }

        // Free VolumetricLightManager resources
        if (scene->lightManager.lightLen > 0) {
            // Free each volumetric light's resources
            for (uint32_t lightIndex = 0; lightIndex < scene->lightManager.lightLen; lightIndex++) {
                LightVolume* currentLight = &scene->lightManager.pLight[lightIndex];

                // Free meshes in the light's model
                if (currentLight->model.meshManager.meshesLen > 0) {
                    free(currentLight->model.meshManager.pMesh);
                }
            }

            // Free volumetric lights array
            free(scene->lightManager.pLight);

            // Free packed vertices if they exist
            if (scene->lightManager.packedLightLen> 0) {
                free(scene->lightManager.pPackedLight);
            }
        }

        // Finally free the scene itself
        free(scene);
    }

    std::cout << "SCENE deletion completed" << std::endl;
};
// TODO: maybe ditch classes and use namespaces as a general approach, in the end working with the render library
// structs is more natural
class Scene {

private:
    std::unique_ptr<SCENE, decltype(sceneDeletor)> scene_;

    std::vector<Model3D> models;
    std::vector<Model3DInstance*> modelsInstances;
    std::vector<LightVolume> lights;
    std::vector<LightVolumeInstance*> lightsInstances;
public:
    // Default constructor
    Scene(){
        std::cout << "DEFAULT CONSTRUCTOR DE SCENE" << std::endl;
    };

    // Copy constructor
    Scene(const Scene& other) = delete;


    const Scene& operator=(const Scene& other) = delete;

    Scene(Scene&& other) noexcept

    {
        this->models = std::move(other.models);
        this->lights = std::move(other.lights);
        std::cout << "Move constructor of Scene called" << std::endl;

        other.models.clear();
        other.lights.clear();
    }

    // Move assignment operator
    Scene& operator=(Scene&& other) noexcept {
            this->models = std::move(other.models);
            this->lights = std::move(other.lights);
            std::cout << "Move constructor of Scene called" << std::endl;

            other.models.clear();
            other.lights.clear();
            std::cout << "LLAMAMOS AL MODEL operator= THE METAOBJECT" << std::endl;
            return *this;
    };

    friend std::ostream& operator<<(std::ostream& output, const Scene& scene){
        std::cout << "EN SOBRECARGA OPERADOR <<" << std::endl;
        printSceneModels(&scene.scene_->modelsManager);
        printSceneLights(&scene.scene_->lightManager);

        return output;
    };

    ~Scene(){

    };

    std::vector<Model3D>& getModels(){
        return this->models;
    };

    std::vector<LightVolume>& getLights(){
        return this->lights;
    };

    Model3D& getModel(uint32_t index){
        return this->models.at(index);
    };

    LightVolume& getVolumetricLight(uint32_t index){
        return this->lights.at(index);
    };

    void addModel(Model3D* model){
        this->models.push_back(*model);
        printf("AÑADIDO MODELO CON VERTICES LEN %d Y INDICES LEN %d\n", model->meshManager.totalMeshVerticesLen, model->indices.itemsLen);

    }

    void addLight(LightVolume* light){
        this->lights.push_back(*light);
    }

    SCENE* getScene(){
        return this->scene_.get();
    }

    void initInstanceDataStub(){
        LightVolumeInstance* light = (LightVolumeInstance*) malloc(sizeof(LightVolumeInstance));
        vec3 position = {0.0f, 3.0f, 0.0f};
        glm_translate_make(light->m, position);
        vec3 scale = {1.0f,1.0f,1.0f};
        glm_scale_make(light->m, scale);
        this->lightsInstances.push_back(light);

        Model3DInstance* vikingRoomInstance = (Model3DInstance*) malloc(sizeof(Model3DInstance));
        vec3 vRPosition = {0.0f, 0.0f, 0.0f};
        glm_translate_make(vikingRoomInstance->m, vRPosition);
        vec3 vRScale = {1.0f,1.0f,1.0f};
        glm_scale_make(vikingRoomInstance->m, vRScale);
        this->modelsInstances.push_back(vikingRoomInstance);
        printf("HOPLA\n");
        Model3DInstance* cubeInstance = (Model3DInstance*) malloc(sizeof(Model3DInstance));
        vec3 cubePosition = {0.0f, 0.0f, 0.0f};
        glm_translate_make(cubeInstance->m, cubePosition);
        vec3 cubeScale = {1.0f, 1.0f, 1.0f};
        glm_scale_make(cubeInstance->m, cubeScale);
        this->modelsInstances.push_back(cubeInstance);

    }
    void init()
    {
        std::cout << "EN INIT() SCENE MODELS SIZE " << this->models.size() << '\n';
        SCENE* scene;
        initScene(&scene, this->models.size(), this->lights.size());
        fillSceneWithModelData(scene, this->models.data(), this->models.size());
                fillSceneWithLightData(scene, this->lights.data(), this->lights.size());
        Model3DInstance** modelInstances = this->modelsInstances.data();
        fillSceneWithInstancedModelData(&scene->modelsManager, modelInstances, this->modelsInstances.size());
        LightVolumeInstance** lightInstance = this->lightsInstances.data();
        fillSceneWithInstancedLightVolumeData(&scene->lightManager, lightInstance, this->lightsInstances.size());
        std::cout << "HOLA EN INIT THE HPP SCNEE\n";
        std::cout << scene->modelsManager.packedModelVertexLen << " ESTO VA DONDE VA VA VA \n\n\n\n\n";

        scene->sceneInstancesLen = 3;
        std::cout << "QUE ESTA PASANDO AQUI" << std::endl;
        this->scene_.reset(scene);
        std::cout << "YYYYY AQUI?" << std::endl;
    };

};
