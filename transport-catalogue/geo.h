#pragma once

#include <cmath>

namespace geo {

struct Coordinates {
    double lat; // Широта
    double lng; // Долгота

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