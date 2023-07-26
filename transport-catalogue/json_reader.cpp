#include "json_reader.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <sstream>
#include <memory>
#include <stdexcept>

using namespace std::literals;
using namespace std::string_view_literals;

const auto& node_by_key = [](const json::Node& node, const std::string& key) {
    return node.AsDict().at(key);
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

const std::vector<std::unique_ptr<Request>>& JsonReader::GetStatRequests() const {
    return stat_requests_;
}

const RenderSettings& JsonReader::GetRenderSettings() const {
    return render_settings_;
}

RoutingSettings JsonReader::GetRoutingSettings() const {
    return routing_settings_;
}

void JsonReader::MakeBase(std::istream& input) {
    json::Document json_document = json::Load(input);

    // путь до файла, в который надо сериализовать базу данных справочника
    const auto& serialization_settings = node_by_key(json_document.GetRoot(), "serialization_settings"s);
    output_file_ = std::filesystem::path(serialization_settings.AsDict().at("file"s).AsString());

    // заполнение базы транспортного каталога маршрутами и остановками
    const auto& base_requests = node_by_key(json_document.GetRoot(), "base_requests"s);

    for (const auto& request : base_requests.AsArray()) {
        const auto& query_type = node_by_key(request, "type"s);

        // перенос каждого вида запроса на создание базы данных справочника в свою категорию
        if (query_type.AsString() == "Bus"sv) {
            bus_requests_.push_back(ExtractBus(request));
        } else /* Stop */ {

            stop_requests_.push_back(ExtractStop(request));

            // перенос всех соседей в глобальный вектор соседей
            const auto& from = node_by_key(request, "name"s);
            const auto& distances = node_by_key(request, "road_distances"s);

            for (const auto& [to, distance] : distances.AsDict()) {
                distances_.push_back({from.AsString(), to, distance.AsInt()});
            }
        }
    }

    // формирование мапы с настройками рендеринга
    // цвет может быть задан как в rgb или в rgba формате,
    // а также в виде конкретного названия цвета, а эта лямбда
    // вернет строку, которую понимает svg
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

    // формирование структуры с настройками рендеринга
    const json::Node& render_settings = node_by_key(json_document.GetRoot(), "render_settings"s);

    render_settings_.width = node_by_key(render_settings, "width"s).AsDouble();
    render_settings_.height = node_by_key(render_settings, "height"s).AsDouble();
    render_settings_.padding = node_by_key(render_settings, "padding"s).AsDouble();

    auto stop_label_offset = node_by_key(render_settings, "stop_label_offset"s).AsArray();
    render_settings_.stop_label_offset.first = stop_label_offset[0].AsDouble();
    render_settings_.stop_label_offset.second = stop_label_offset[1].AsDouble();
    render_settings_.stop_label_font_size = node_by_key(render_settings, "stop_label_font_size"s).AsDouble();
    render_settings_.stop_radius = node_by_key(render_settings, "stop_radius"s).AsDouble();
    
    render_settings_.line_width = node_by_key(render_settings, "line_width"s).AsDouble();
    
    auto bus_label_offset = node_by_key(render_settings, "bus_label_offset"s).AsArray();
    render_settings_.bus_label_offset.first = bus_label_offset[0].AsDouble();
    render_settings_.bus_label_offset.second = bus_label_offset[1].AsDouble();
    render_settings_.bus_label_font_size = node_by_key(render_settings, "bus_label_font_size"s).AsDouble();

    render_settings_.underlayer_color = make_color(node_by_key(render_settings, "underlayer_color"s));
    render_settings_.underlayer_width = node_by_key(render_settings, "underlayer_width"s).AsDouble();

    const auto& color_palette = node_by_key(render_settings, "color_palette"s); // если сюда поставить .AsArray()
    
    for (const auto& color_node : color_palette.AsArray()) {                    // а убрать отсюда, то будет ub -- почему? 
        render_settings_.color_palette.push_back(make_color(color_node));
    }

    // формирование настроек маршрутизатора: скорость автобуса и время ожидания на остановке
    const auto& routing_settings = node_by_key(json_document.GetRoot(), "routing_settings"s);
    
    routing_settings_.bus_wait_time = routing_settings.AsDict().at("bus_wait_time"s).AsInt();
    routing_settings_.bus_velocity = routing_settings.AsDict().at("bus_velocity"s).AsInt();
}

void JsonReader::MakeRequests(std::istream& input) {
    json::Document json_document = json::Load(input);

    // путь до файла, в котором сериализованная база данных справочника
    const auto& serialization_settings = node_by_key(json_document.GetRoot(), "serialization_settings"s);
    input_file_ = std::filesystem::path(serialization_settings.AsDict().at("file"s).AsString());

    // формирование вектора запросов к готовой базе справочника
    const auto& stat_requests = node_by_key(json_document.GetRoot(), "stat_requests"s);
    for (const auto& request : stat_requests.AsArray()) {
        const auto& id = node_by_key(request, "id"s);
        const auto& type = node_by_key(request, "type"s);
        if (type.AsString() == "Map"s) {
            stat_requests_.push_back(std::make_unique<Map>(Map(id.AsInt(), type.AsString())));
        } else if (type.AsString() == "Route"s) {
            const auto& from = node_by_key(request, "from"s);
            const auto& to = node_by_key(request, "to"s);
            
            stat_requests_.push_back(std::make_unique<Route>(Route(id.AsInt(),
                                                                   type.AsString(),
                                                                   from.AsString(),
                                                                   to.AsString())));
        } else /* Bus или Stop */ {
            const auto& name = node_by_key(request, "name"s);
            stat_requests_.push_back(std::make_unique<Transport>(Transport(id.AsInt(),
                                                                           type.AsString(),
                                                                           name.AsString())));
        }
    }
}

void JsonReader::LoadJSON(std::istream& input, ReadMode read_mode) {
    /* загрузка json в одном из режимов: парсинг запросов для формирования
    базы данных справочника или парсинг запросов к сформированной базе данных справочника */
    switch (read_mode)
    {
    case ReadMode::MAKE_BASE:
        MakeBase(input);
        break;
    case ReadMode::PROCESS_REQUEST:
        MakeRequests(input);
        break;
    default:
        throw std::runtime_error("Error mode: MAKE_BASE and PROCESS_REQUEST are allowed"s);
    }
}

std::filesystem::path JsonReader::GetOutputFilepath() {
    return output_file_;
}
std::filesystem::path JsonReader::GetInputFilepath() {
    return input_file_;
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