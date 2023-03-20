#pragma once

#include "geo.h" // Coordinates
#include "domain.h" // RenderSettings
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <map>

namespace renderer {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    SphereProjector();
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    void Configure(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
    {
        padding_ = padding;

        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // возьму только точки из итераторов на мапу
        std::vector<geo::Coordinates> points;
        for (auto it = points_begin; it != points_end; ++it) {
            points.push_back(it->second);
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points.begin(), points.end(),
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points.begin(), points.end(),
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_ = 0;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer();
    void ConfigureMapRenderer(const RenderSettings& render_settings,
                              const std::map<StopName, geo::Coordinates>& points_limits);
    
    void CreateMap(const std::map<BusName, Bus*>& all_buses,
                   const std::map<StopName, geo::Coordinates>& useful_stops);
    
    const svg::Document& GetMap();
private:

    void DrawBusesLay(const std::map<BusName, Bus*>& busname_to_buses);
    void DrawBusNamesLay(const std::map<BusName, Bus*>& busname_to_buses);
    void DrawStopsLay(const std::map<StopName, geo::Coordinates>& useful_stops);
    void DrawStopNamesLay(const std::map<StopName, geo::Coordinates>& useful_stops);

    RenderSettings render_settings_;
    SphereProjector projector_;
    svg::Document svg_document_;
};

} // namespace renderer
