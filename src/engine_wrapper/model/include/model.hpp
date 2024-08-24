#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C" {
    #include "primitives.h"
}

// Enumeration for primitive types
enum class PRIMITIVE_TYPE {
    CUBE,
    PYRAMID,
    CYLINDER,
    QUAD,
    TRIANGLE,
    SPHERE,
    MODEL
};

template <typename T>
class Model : public MetaObject<T> {
public:
    // default constructor
    Model() : MetaObject<T>(){};

    explicit Model(const std::string& modelPath, const GEOM_VERTEX* const globalCoords, uint32_t size) noexcept{
        std::cout << "Loading PRIMITIVE_TYPE::MODEL" << std::endl;
    };

    explicit Model(PRIMITIVE_TYPE primitive, GEOM_VERTEX* const globalCoord, const uint32_t size) {
        T* cube;
        switch(primitive){
            case PRIMITIVE_TYPE::CUBE:
                std::cout << "CREAMOS MODELO CUBO FLAG" << std::endl;
                cube = create_cube(globalCoord, size);
                this->object = std::unique_ptr<T>(cube);
            break;
            default:
                throw std::runtime_error("Primitive not supported!");
        }
        std::cout << "FINALIZAMOS MODELO CUBO FLAG" << std::endl;
    };

    // Copy constructor
    Model(const Model& other) = delete;

    const Model& operator=(const Model& other) = delete;

    // Move constructor
    Model(Model&& other) noexcept : MetaObject<T>(std::move(other)){
        std::cout << "LLAMAMOS AL MODEL MOVE THE METAOBJECT" << std::endl;
    };

    // Move assignment operator
    Model& operator=(Model&& other) noexcept {
            if(this != other){
                MetaObject<T>::operator=(std::move(other));
            }
            std::cout << "LLAMAMOS AL MODEL operator= THE METAOBJECT" << std::endl;
    };


    friend std::ostream& operator<<(std::ostream& output, const Model& model){
        std::cout << "EN SOBRECARGA OPERADOR <<" << std::endl;
        for(int i = 0; i < model.object->dataLen; i++){
            output << "X: " << model.object->data[i].point.x << " ";
            output << "Y: " << model.object->data[i].point.y << " ";
            output << "Z: " << model.object->data[i].point.z << std::endl;
        }
        exit(0);
        return output;
    }
    ~Model(){
        if(this->object){
            std::cout << "LLAMAMOS AL DESTRUCTOR DE MODELO" << std::endl;
            free(this->object->data);
            free(this->object->indices);
        }

    };
};
