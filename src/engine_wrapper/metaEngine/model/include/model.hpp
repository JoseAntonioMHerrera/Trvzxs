#include "metaObject.hpp"
#include <memory>
#include <type_traits>

extern "C" {
    #include "primitives.h"
    #include "model.h"
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
    explicit Model(const std::string& modelPath) noexcept
    : MetaObject<T, void(*)(T*)>() {
        T* model;
        model = load_obj_to_geom_object(modelPath.c_str());
        this->MetaObject<T, void(*)(T*)>::operator=(MetaObject<T, void(*)(T*)>(model, modelDeletor<T>));
        // TODO: Implement model loading logic
    }

    // Constructor for creating primitives
    explicit Model(PRIMITIVE_TYPE primitive, GEOM_VERTEX_POSITION* const globalCoord, const uint32_t size)
    : MetaObject<T, void(*)(T*)>() {
        T* object = nullptr;
        switch(primitive) {
            case PRIMITIVE_TYPE::CUBE:

                object = create_cube(globalCoord, size);
            // Add other cases for different primitives
            break;
             case PRIMITIVE_TYPE::QUAD:

                object = create_quad(globalCoord, size);
            // Add other cases for different primitives
            break;
        default:
            throw std::runtime_error("Primitive not supported!");
        }
        if (object) {
            this->MetaObject<T, void(*)(T*)>::operator=(MetaObject<T, void(*)(T*)>(object, modelDeletor<T>));
        }

    };

    // Copy constructor
    Model(const Model<T>& other) = delete;

    const Model& operator=(const Model& other) = delete;

    // Move constructor
    Model(Model<T>&& other) noexcept : MetaObject<T, void(*)(T*)>(std::move(other)){
    };

    // Move assignment operator
    Model& operator=(Model&& other) noexcept {
                MetaObject<T,void(*)(T*)>::operator=(std::move(other));
    };


    friend std::ostream& operator<<(std::ostream& output, const Model& model){
        std::cout << "EN SOBRECARGA OPERADOR <<" << std::endl;
        for(int i = 0; i < model.object->dataLen; i++){
            output << "X: " << model.object->data[i].point.x << " ";
            output << "Y: " << model.object->data[i].point.y << " ";
            output << "Z: " << model.object->data[i].point.z << std::endl;
        }
        return output;
    }
    ~Model(){
        if(this->object){
            std::cout << "DESTRUCTOR MODELO" << std::endl;
        }

    };
};
