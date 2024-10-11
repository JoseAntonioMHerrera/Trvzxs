#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C" {
    #include "scene.h"
}

constexpr auto sceneDeletor = [](SCENE* scene){

    printScene(&scene->pScene, scene->sceneLen);
    std::cout <<"LLAMAMOS AL DELETER DE SCENE*" << std::endl;
    if(scene->sceneLen > 0)
        free(scene->pScene);
    };

template <typename T, template <typename> class U, typename V>
class Scene : public MetaObject<T, DeleterFunctionType<SCENE>> {

private:
    std::vector<U<V>> models;

public:
    // Default constructor
    template <typename SceneType = T>
    Scene(){
        std::cout << "DEFAULT CONSTRUCTOR DE SCENE" << std::endl;
    };

    Scene(DeleterFunctionType<T> deleteScene): MetaObject<T, DeleterFunctionType<T>>(deleteScene){};

    // Copy constructor
    Scene(const Scene& other) = delete;


    const Scene& operator=(const Scene& other) = delete;

    Scene(Scene&& other) noexcept
        : MetaObject<T, DeleterFunctionType<SCENE>>(std::move(other)),
          models(std::move(other.models))
    {
        std::cout << "Move constructor of Scene called" << std::endl;

        // If there's any additional member that needs to be moved, do it here
        // For example, if you had a member like:
        // this->someOtherMember = std::move(other.someOtherMember);

        // Optionally, reset the other object's state
        // other.someResetFunction();
    }

    // Move assignment operator
    Scene& operator=(Scene&& other) noexcept {
                MetaObject<T,void(*)(T*)>::operator=(std::move(other));

            std::cout << "LLAMAMOS AL MODEL operator= THE METAOBJECT" << std::endl;
    };

    friend std::ostream& operator<<(std::ostream& output, const Scene& model){
        std::cout << "EN SOBRECARGA OPERADOR <<" << std::endl;
        for(int i = 0; i < model.object->dataLen; i++){
            output << "X: " << model.object->data[i].point.x << " ";
            output << "Y: " << model.object->data[i].point.y << " ";
            output << "Z: " << model.object->data[i].point.z << std::endl;
        }

        return output;
    };

    ~Scene(){

    };

    std::vector<U<V>>& getModels(){
        return this->models;
    };

    U<V>& getModel(uint32_t index){
        return this->models.at(index);
    };

    void addModel(U<V>&& model){
        this->models.emplace_back(std::move(model));
    }

    void init()
    {
        std::cout << "EN INIT() SCENE MODELS SIZE " << this->models.size() << '\n';
        T* scene;
        std::vector<V*> objectsData(this->models.size());
        for (size_t i = 0; i < this->models.size(); ++i) {
            objectsData[i] = this->models[i].getStruct();
        }

        initSceneWithData(&scene, objectsData.data(), this->models.size());
        std::cout << "VOLVEMOS DE INITSCENEWITHDATA" << std::endl;
        this->object.reset(scene);
        std::cout << "QUE ESTA PASANDO AQUI" << std::endl;
    };


};
