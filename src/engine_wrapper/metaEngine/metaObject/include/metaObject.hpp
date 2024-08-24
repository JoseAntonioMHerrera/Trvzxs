#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <cstring>
#include "vertex.h"
#include <memory>

template<typename T>
// Class representing a geometric object
class MetaObject {
protected:
        std::unique_ptr<T> object;

public:
    // Default constructor
    MetaObject() : object(nullptr){};

    // Copy constructor
    MetaObject(const MetaObject&) = delete;

    // Move constructor
    MetaObject(MetaObject&& other) noexcept : object(std::move(other.object)){
        std::cout << "LLAMAMOS AL CONSTRUCTOR MOVE THE METAOBJECT" << std::endl;
    };

    // Move assignment operator
    MetaObject& operator=(MetaObject&& other) {
        std::cout << "LLAMAMOS AL OPERADOR ASIGNACION MOVE THE METAOBJECT" << std::endl;
        if(this != other){
            this->object = std::move<T>(other.object);
        }
        return *this;
    };

    // Shallow copy assignment operator
    MetaObject& operator=(const MetaObject&) = delete;

    ~MetaObject()= default;
};
