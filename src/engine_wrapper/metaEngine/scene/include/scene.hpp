#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C" {
    #include "scene.h"
}

template <typename T>
constexpr auto sceneDeletor = [](T* scene){ free(scene->pScene); free(scene->sceneIndices); free(scene);};

template <typename T, template <typename> class U, typename V>
class Scene : MetaObject<T, void(*)(T*)> {

private:
    std::vector<U<V>> models;
    std::unique_ptr<MetaObject<T, void(*)(T*)>> scene;

public:
    // Default constructor
    Scene(){
        std::cout << "DEFAULT CONSTRUCTOR DE SCENE" << std::endl;
    };

    // Copy constructor
    Scene(const Scene& other) = delete;

    const Scene& operator=(const Scene& other) = delete;

    // Move constructor
    Scene(Scene&& other) noexcept {
        std::cout << "LLAMAMOS AL MODEL MOVE THE METAOBJECT" << std::endl;
    };

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
        exit(0);
        return output;
    };

    ~Scene(){
            std::cout << "DESTRUCTOR MODELO" << std::endl;

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
        T* scene = static_cast<T*>(malloc(sizeof(T)));
        V** objectsData = static_cast<V**>(malloc(sizeof(V*) * this->models.size()));
        std::cout << this->models.size() << std::endl;
        for(uint32_t i = 0; i < this->models.size(); i++) {
            objectsData[i] = this->models.at(i).getStruct();
        }

        initSceneWithData(scene, objectsData[0], this->models.size());
        std::cout << "VOLVEMOS DE INITSCENEWITHDATA" << std::endl;
        this->scene = std::make_unique<MetaObject<T, void(*)(T*)>>(scene, sceneDeletor<T>);
    };


};
