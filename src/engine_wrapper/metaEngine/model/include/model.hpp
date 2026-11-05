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
    PLANE,
    MODEL
};

constexpr auto modelDeletor = [](Model3D* obj){};

class Model  {
public:
};
