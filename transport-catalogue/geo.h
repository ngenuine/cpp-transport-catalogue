#pragma once

#include <cmath>

namespace geo {

struct Coordinates {
    double lat; // Широта
    double lng; // Долгота
    // когда очередные координаты добавят куда-то, этот счетчик
    // отметит очередь, при которой координаты были добавлены
    size_t id = 0;

    bool operator==(const Coordinates& other) const {
        return std::abs(lat - other.lat) < 1e-6
            && std::abs(lng - other.lng) < 1e-6;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }

};

double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo