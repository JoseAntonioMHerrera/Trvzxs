#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <cstring>
#include "vertex.h"
#include <memory>

template<typename T, typename Deleter>
class MetaObject {
protected:
        std::unique_ptr<T, Deleter> object;

public:
    // Default constructor
    MetaObject() : object(nullptr, Deleter()){};
    MetaObject(T* data, Deleter deleter = Deleter()) : object(std::unique_ptr<T, Deleter>(data, deleter)) {};

    // Copy constructor
    MetaObject(const MetaObject&) = delete;

    // Move constructor
    MetaObject(MetaObject&& other) noexcept : object(std::move(other.object)){
        std::cout << "LLAMAMOS AL CONSTRUCTOR MOVE THE METAOBJECT" << std::endl;
    };

    // Move assignment operator
    MetaObject& operator=(MetaObject&& other) {
        std::cout << "LLAMAMOS AL OPERADOR ASIGNACION MOVE THE METAOBJECT" << std::endl;

            this->object = std::move(other.object);

        return *this;
    };

    // Shallow copy assignment operator
    MetaObject& operator=(const MetaObject&) = delete;

    ~MetaObject(){};

    T* getStruct(){
        return object.get();
    };
};
