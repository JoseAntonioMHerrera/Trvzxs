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
constexpr auto modelDeletor = [](T* obj){ free(obj->data); free(obj->indices);};

template <typename T>
class Model : public MetaObject<T, void(*)(T*)> {
public:
    // Default constructor
    Model() : MetaObject<T, void(*)(T*)>() {}

    // Constructor for loading from file
    explicit Model(const std::string& modelPath, const GEOM_VERTEX* const globalCoords, uint32_t size) noexcept
    : MetaObject<T, void(*)(T*)>() {
        std::cout << "Loading PRIMITIVE_TYPE::MODEL" << std::endl;
        // TODO: Implement model loading logic
    }

    // Constructor for creating primitives
    explicit Model(PRIMITIVE_TYPE primitive, GEOM_VERTEX* const globalCoord, const uint32_t size)
    : MetaObject<T, void(*)(T*)>() {
        T* object = nullptr;
        switch(primitive) {
            case PRIMITIVE_TYPE::CUBE:
                std::cout << "CREAMOS MODELO CUBO FLAG" << std::endl;
                object = create_cube(globalCoord, size);
            // Add other cases for different primitives
            break;
        default:
            throw std::runtime_error("Primitive not supported!");
        }
        if (object) {
            this->MetaObject<T, void(*)(T*)>::operator=(MetaObject<T, void(*)(T*)>(object, modelDeletor<T>));
        }
        std::cout << "FINALIZAMOS MODELO CUBO FLAG" << std::endl;
    };

    // Copy constructor
    Model(const Model<T>& other) = delete;

    const Model& operator=(const Model& other) = delete;

    // Move constructor
    Model(Model<T>&& other) noexcept : MetaObject<T, void(*)(T*)>(std::move(other)){
        std::cout << "LLAMAMOS AL MODEL MOVE" << std::endl;
    };

    // Move assignment operator
    Model& operator=(Model&& other) noexcept {
                MetaObject<T,void(*)(T*)>::operator=(std::move(other));

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
            std::cout << "DESTRUCTOR MODELO" << std::endl;
        }

    };
};
