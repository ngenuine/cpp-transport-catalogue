#include "json_reader.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <sstream>

using namespace std::literals;
using namespace std::string_view_literals;

auto node_by_key = [](const json::Node& node, const std::string& key) {
    return node.AsMap().at(key);
};

const std::vector<RawBus>& JsonReader::GetBusRequests() const {
    return bus_requests_;
}

const std::vector<Stop>& JsonReader::GetStopRequests() const {
    return stop_requests_;
}

const std::vector<Neighbours>& JsonReader::GetDistancesRequests() const {
    return distances_;
}

const std::vector<Request>& JsonReader::GetStatRequests() const {
    return stat_requests_;
}

const RenderSettings& JsonReader::GetRenderSettings() const {
    return render_settings_;
}

void JsonReader::LoadJSON(std::istream& input) {
    json::Document json_document = json::Load(input);

    const auto& base_requests = node_by_key(json_document.GetRoot(), "base_requests"s);

    for (const auto& request : base_requests.AsArray()) {
        const auto& query_type = node_by_key(request, "type"s);

        // перенос каждый вид запроса на создание базы данных справочника в свою категорию
        if (query_type.AsString() == "Bus"sv) {
            bus_requests_.push_back(ExtractBus(request));
        } else  {

            stop_requests_.push_back(ExtractStop(request));

            // перенос всех соседей в глобальный вектор соседей
            const auto& from = node_by_key(request, "name"s);
            const auto& distances = node_by_key(request, "road_distances"s);

            for (const auto& [to, distance] : distances.AsMap()) {
                distances_.push_back({from.AsString(), to, distance.AsInt()});
            }
        }
    }

    // формирование вектора запросов к готовой базе справочника
    const auto& stat_requests = node_by_key(json_document.GetRoot(), "stat_requests"s);
    for (const auto& request : stat_requests.AsArray()) {
        const auto& id = node_by_key(request, "id"s);
        const auto& type = node_by_key(request, "type"s);
        if (type.AsString() == "Map"s) {
            stat_requests_.push_back(
                Request{id.AsInt(), type.AsString()
            });

        } else {
            const auto& name = node_by_key(request, "name"s);

            stat_requests_.push_back(
                Request{id.AsInt(),
                type.AsString(),
                name.AsString()});

        }
    }

    // формирование мапы с настройками рендеринга
    const auto& render_settings = node_by_key(json_document.GetRoot(), "render_settings"s);

    std::unordered_set<std::string> as_double{"width"s, "height"s, "padding"s, "line_width"s, "stop_radius"s, "underlayer_width"s};
    std::unordered_set<std::string> as_int{"bus_label_font_size"s, "stop_label_font_size"s};
    std::unordered_set<std::string> as_pair_double{"bus_label_offset"s, "stop_label_offset"s};
    std::unordered_set<std::string> as_vector_string{"color_palette"s};
    std::unordered_set<std::string> as_string{"underlayer_color"s};

    auto make_color = [](const json::Node& node) {
        if (node.IsString()) {
            return node.AsString();
        }

        std::stringstream color_code;
        color_code << node.AsArray()[0].AsInt()
        << ',' << node.AsArray()[1].AsInt()
        << ',' << node.AsArray()[2].AsInt();

        if (node.AsArray().size() == 3) {
            return "rgb("s + color_code.str() + ")"s;
        }

        color_code << ',' << node.AsArray()[3].AsDouble();
        return "rgba("s + color_code.str() + ")"s;
    };

    for (const auto& [setting, value] : render_settings.AsMap()) {
        if (as_double.count(setting) == 1) {
            render_settings_.insert({setting, value.AsDouble()});
        } else if (as_int.count(setting) == 1) {
            render_settings_.insert({setting, value.AsInt()});
        } else if (as_pair_double.count(setting) == 1) {
            std::pair<double, double> offsets;
            offsets.first = value.AsArray()[0].AsDouble();
            offsets.second = value.AsArray()[1].AsDouble();

            render_settings_.insert({setting, offsets});
        } else if (as_vector_string.count(setting) == 1) {
            std::vector<std::string> color_palette;

            for (const auto& node : value.AsArray()) {
                color_palette.push_back(make_color(node));
            }        

            render_settings_.insert({setting, color_palette});

        } else if (as_string.count(setting) == 1) {
            std::string color = make_color(value);
            render_settings_.insert({setting, color});
        } else {
            throw std::logic_error("Unknown setting: "s + "\""s + setting + "\""s + " at render settings section");
        }
    }
}

RawBus JsonReader::ExtractBus(const json::Node& node) {
    /* извлечение остановок марштура */

    RawBus raw_bus;
    raw_bus.busname = node_by_key(node, "name"s).AsString();
    raw_bus.is_cycle = node_by_key(node, "is_roundtrip"s).AsBool();

    const json::Node& stops = node_by_key(node, "stops"s);
    
    for (const auto& stop : stops.AsArray()) {
        raw_bus.stops.push_back(stop.AsString());
    }

    return raw_bus;
}

Stop JsonReader::ExtractStop(const json::Node& node) {
    /* извлечение остановки */

    Stop stop;

    stop.stopname = node_by_key(node, "name"s).AsString();
    double latitude = node_by_key(node, "latitude"s).AsDouble();
    double longitude = node_by_key(node, "longitude"s).AsDouble();

    stop.location = {latitude, longitude};

    return stop;
}