#include "map_renderer.h"

#include <map>
#include <vector>

namespace renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

SphereProjector::SphereProjector() = default;
svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

MapRenderer::MapRenderer() = default;

void MapRenderer::ConfigureMapRenderer(const RenderSettings& render_settings,
                                       const std::map<StopName, geo::Coordinates>& points_limits) {
    render_settings_ = render_settings;

    const double width = std::get<double>(render_settings.at("width"s));
    const double height = std::get<double>(render_settings.at("height"s));
    const double padding = std::get<double>(render_settings.at("padding"s));
    projector_.Configure(points_limits.begin(), points_limits.end(), width, height, padding);
}

void MapRenderer::CreateMap(const std::map<BusName, Bus*>& all_buses,
                            const std::map<StopName, geo::Coordinates>& useful_stops) {
    DrawBusesLay(all_buses);
    DrawBusNamesLay(all_buses);
    DrawStopsLay(useful_stops);
    DrawStopNamesLay(useful_stops);
}

const svg::Document& MapRenderer::GetMap() {
    return svg_document_;
}

void MapRenderer::DrawBusesLay(const std::map<BusName, Bus*>& all_buses) {
    auto& color_palette = std::get<std::vector<std::string>>(render_settings_.at("color_palette"s));

    // настройка свойств полилиний маршрутов
    svg::Polyline base_bus_path;
    base_bus_path
        .SetStrokeWidth(std::get<double>(render_settings_.at("line_width"s)))
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
        .SetFillColor(svg::NoneColor);

    // добавляем в документ маршруты в виде полилиний
    size_t color_order = 0;
    for (const auto& [busname, ptr_bus] : all_buses) {
        Bus& bus = *ptr_bus;
        if (bus.stops.size() == 0) {
            continue;
        }

        std::string bus_color = color_palette[(color_order++) % color_palette.size()];

        // создаем ломаную линию маршрута
        svg::Polyline bus_path{base_bus_path};
        bus_path.SetStrokeColor(bus_color);
        for (const Stop* stop : bus.stops) {
            bus_path.AddPoint(projector_(stop->location));
        }

        // если маршрут не круговой -- строим обратный маршрут от предпоследней до первой остановки
        if (!bus.is_cycle) {
            for (auto it = (++bus.stops.rbegin()); it != bus.stops.rend(); ++it) {
                bus_path.AddPoint(projector_((*it)->location));
            }
        }

        // добавляем в документ ломаную линию маршрута
        svg_document_.Add(bus_path);
    }
}

void MapRenderer::DrawBusNamesLay(const std::map<BusName, Bus*>& all_buses) {
    auto& color_palette = std::get<std::vector<std::string>>(render_settings_.at("color_palette"s));

    // настройка свойств текста для подписи маршрутов
    auto [b_dx, b_dy] = std::get<std::pair<double, double>>(render_settings_.at("bus_label_offset"s));
    svg::Text base_bus_text;
    base_bus_text
        .SetOffset({b_dx, b_dy})
        .SetFontSize(std::get<int>(render_settings_.at("bus_label_font_size"s)))
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s);

    // настройка свойств подложки для подписи маршрутов
    std::string underlayer_color = std::get<std::string>(render_settings_.at("underlayer_color"s));
    double underlayer_width = std::get<double>(render_settings_.at("underlayer_width"s));
    
    svg::Text base_underlayer_bus_text{base_bus_text};
    base_underlayer_bus_text
        .SetFillColor(underlayer_color)
        .SetStrokeColor(underlayer_color)
        .SetStrokeWidth(underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    // добавляем в документ подписи маршрутов
    size_t color_order = 0;
    for (const auto& [busname, ptr_bus] : all_buses) {
        Bus& bus = *ptr_bus;
        if (bus.stops.size() == 0) {
            continue;
        }

        std::string bus_color = color_palette[(color_order++) % color_palette.size()];

        // донастраиваем подложку названия маршрута около первой остановки
        svg::Text underlayer_bus_text{base_underlayer_bus_text};
        underlayer_bus_text
            .SetData(std::string(bus.busname))
            .SetPosition(projector_(bus.stops.front()->location));

        // донастраиваем текст названия маршрута около первой остановки
        svg::Text bus_text{base_bus_text};
        bus_text
            .SetData(std::string(bus.busname))
            .SetPosition(projector_(bus.stops.front()->location))
            .SetFillColor(bus_color);
        
        // добавляем в документ подложку и текст названия маршрута около первой остановки
        svg_document_.Add(underlayer_bus_text);
        svg_document_.Add(bus_text);

        // добавляем в документ подложку и текст названия маршрута около последней остановки (если первая и последняя остановки не совпадают)
        bool is_first_stop_is_same_last_stop =
            (bus.stops.front()->stopname == bus.stops.back()->stopname);
        if (!is_first_stop_is_same_last_stop) {
            underlayer_bus_text.SetPosition(projector_(bus.stops.back()->location));
            bus_text.SetPosition(projector_(bus.stops.back()->location));
            
            svg_document_.Add(underlayer_bus_text);
            svg_document_.Add(bus_text);

        }
    }
}

void MapRenderer::DrawStopsLay(const std::map<StopName, geo::Coordinates>& useful_stops) {
    // настройка свойств окружностей остановок
    svg::Circle base_stop_symbol;
    base_stop_symbol
        .SetRadius(std::get<double>(render_settings_.at("stop_radius"s)))
        .SetFillColor("white");
    
    // добавляем в документ точки остановок
    for (const auto& [stopname, stop_coordinates] : useful_stops) {
        svg::Circle stop_symbol{base_stop_symbol};
        stop_symbol.SetCenter(projector_(stop_coordinates));

        svg_document_.Add(stop_symbol);
    }
}

void MapRenderer::DrawStopNamesLay(const std::map<StopName, geo::Coordinates>& useful_stops) {
    // настройка свойств текста для подписи остановок
    std::string underlayer_color = std::get<std::string>(render_settings_.at("underlayer_color"s));
    double underlayer_width = std::get<double>(render_settings_.at("underlayer_width"s));
    
    auto [st_dx, st_dy] = std::get<std::pair<double, double>>(render_settings_.at("stop_label_offset"s));
    svg::Text base_stop_text;
    base_stop_text
        .SetOffset({st_dx, st_dy})
        .SetFontSize(std::get<int>(render_settings_.at("stop_label_font_size"s)))
        .SetFontFamily("Verdana"s);

    // настройка свойств подложки для подписи остановок
    svg::Text base_underlayer_stop_text{base_stop_text};
    base_underlayer_stop_text
        .SetFillColor(underlayer_color)
        .SetStrokeColor(underlayer_color)
        .SetStrokeWidth(underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    // наполняем документ названиями остановок
    for (const auto& [stopname, stop_coordinates] : useful_stops) {
        svg::Text stop_text{base_stop_text};
        svg::Text underlayer_stop_text{base_underlayer_stop_text};

        // донастраиваем подложку названия остановки
        underlayer_stop_text
            .SetPosition(projector_(stop_coordinates))
            .SetData(std::string(stopname));

        // донастраиваем текст названия остановки
        stop_text
            .SetFillColor("black")
            .SetPosition(projector_(stop_coordinates))
            .SetData(std::string(stopname));
        
        // добавляем в документ подложку и текст названия остановки
        svg_document_.Add(underlayer_stop_text);
        svg_document_.Add(stop_text);
    }
}

} // namespace renderer