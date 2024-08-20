#include <iostream>
#include <string>
#include <stdexcept>
#include <fstream>
#include "vertex.h"

namespace trzs {

enum class OBJECT_TYPE { PRIMITIVE };

struct META_GEOM_OBJECT {
    GEOM_OBJECT* object;
    OBJECT_TYPE tag;
    // Other common members...
};

template<typename T>
class Model {
public:
    void load(const std::string& filename, T& obj) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open file: " + filename);
        }

        // Read the tag from the file
        int tagValue;
        file >> tagValue;
        obj.tag = static_cast<GeometryType>(tagValue);

        switch (obj.tag) {
            case OBJECT_TYPE::PRIMITIVE:
                loadPoint(file, obj);
                break;
            default:
                throw std::runtime_error("Unknown geometry type");
        }

        file.close();
    }

private:
    void loadPoint(std::ifstream& file, T& obj) {
        std::cout << "Loading a point" << std::endl;
        // Point-specific loading...
        // For example:
        // file >> obj.x >> obj.y;
    }

    void loadLine(std::ifstream& file, T& obj) {
        std::cout << "Loading a line" << std::endl;
        // Line-specific loading...
        // For example:
        // file >> obj.x1 >> obj.y1 >> obj.x2 >> obj.y2;
    }

    void loadTriangle(std::ifstream& file, T& obj) {
        std::cout << "Loading a triangle" << std::endl;
        // Triangle-specific loading...
        // For example:
        // file >> obj.x1 >> obj.y1 >> obj.x2 >> obj.y2 >> obj.x3 >> obj.y3;
    }

    void loadPolygon(std::ifstream& file, T& obj) {
        std::cout << "Loading a polygon" << std::endl;
        // Polygon-specific loading...
        // For example:
        // int numVertices;
        // file >> numVertices;
        // obj.vertices.resize(numVertices);
        // for (auto& vertex : obj.vertices) {
        //     file >> vertex.x >> vertex.y;
        // }
    }
};
}
