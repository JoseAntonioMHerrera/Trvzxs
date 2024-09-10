#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <cstring>
#include "vertex.h"
#include <memory>
#include <functional>
template <typename T>
using DeleterFunctionType = std::function<void(T*)>;
template<typename T, typename Deleter = std::default_delete<T>>
class MetaObject {
protected:
        std::unique_ptr<T, Deleter> object;

public:
    // Default constructor
    MetaObject() : object(nullptr, Deleter()){};
    MetaObject(T* data, Deleter deleter = Deleter()) : object(std::unique_ptr<T, Deleter>(data, deleter)) {};
    template<typename MetaObjectType = T>
    MetaObject(DeleterFunctionType<T> deleter): object((T*)malloc(sizeof(T)), deleter){};
    // Copy constructor
    MetaObject(const MetaObject&) = delete;

    // Move constructor
    MetaObject(MetaObject&& other) noexcept : object(std::move(other.object)){

    };

    // Move assignment operator
    MetaObject& operator=(MetaObject&& other) {


            this->object = std::move(other.object);

        return *this;
    };

    // Shallow copy assignment operator
    MetaObject& operator=(const MetaObject&) = delete;

    ~MetaObject(){};

    T* getStruct(){
        return this->object.get();
    };
};
