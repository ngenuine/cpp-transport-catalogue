#include "request_handler.h"

RequestHandler::RequestHandler(const JsonReader parsed_json)
    : parsed_json_(parsed_json)
    {
    }

void RequestHandler::BuildTransportDatabase() {
    // сначала надо добавить остановки, т.к. указатели на них в справочнике много где используются
    for (const Stop& stop : parsed_json_.GetStopRequests()) {
        db_.AddStop(stop);
    }

    // когда все возможные остановки находятся в каталоге, можно строить из них маршруты
    for (const RawBus& raw_bus : parsed_json_.GetBusRequests()) {
        db_.AddBus(raw_bus);
    }

    // также установим расстояния между остановками по дорогам
    for (const Neighbours& adjasent_stops : parsed_json_.GetDistancesRequests()) {
        db_.SetCurvedDistance(adjasent_stops);
    }
}

void RequestHandler::BuildMapRenderer() {
    renderer_.ConfigureMapRenderer(parsed_json_.GetRenderSettings(),
                                   db_.GetUsefulStopCoordinates());
}

void RequestHandler::SolveStatRequests() {

    auto not_found = [](int id) {
        return json::Dict{
            {"request_id"s, id},
            {"error_message"s, "not found"s}};
    };

    json::Array answer;

    if (parsed_json_.GetStatRequests().size() == 0) {
        throw std::logic_error("Nothing to solve"s);
    }
    
    for (const Request& request : parsed_json_.GetStatRequests()) {
        
        if (request.type == "Bus"sv) {
            
            const BusInfo businfo = db_.GetBusInfo(request.name);

            if (businfo.number_of_stops > 0) {
                answer.push_back(
                    json::Dict{
                        {"curvature"s, businfo.curvature},
                        {"request_id"s, request.id},
                        {"route_length"s, businfo.bus_curved_length},
                        {"stop_count"s, businfo.number_of_stops},
                        {"unique_stop_count"s, businfo.number_of_unique_stops}
                    });
            } else {
                answer.push_back(not_found(request.id));
            }
        } else if (request.type == "Stop"sv) {

            const StopInfo stopinfo = db_.GetStopInfo(request.name);
            json::Array buses;

            if (stopinfo.is_exist) {
                for (BusName busname : *stopinfo.buses_across_stop) {
                    buses.push_back(std::string(busname));
                }

                // move(buses) и id превратятся в Node, потому что Dict = std::map<string, Node> и потому что у Node разрешена неявное преобразование типов
                answer.push_back(
                    json::Dict{
                        {"buses"s, move(buses)},
                        {"request_id"s, request.id}
                    });
            } else {
                answer.push_back(not_found(request.id));
            }
        } else if (request.type == "Map"sv) {
            std::stringstream svg_map;
            RenderMap(svg_map);
            answer.push_back(
                    json::Dict{
                        {"map"s, svg_map.str()},
                        {"request_id"s, request.id},
                    });
        } else {

            throw std::logic_error("Unknown request: " + "\""s + request.type + "\""s);
        }
    }

    solved_ = json::Document{answer};
}

void RequestHandler::RenderMap(std::ostream& out) {
    renderer_.CreateMap(db_.GetAllBuses(), db_.GetUsefulStopCoordinates());
    renderer_.GetMap().Render(out);
}

void RequestHandler::PrintSolution(std::ostream& out) const {
    if (solved_) {
        json::Print(solved_.value(), out);
    } else {
        throw std::logic_error("Nothing to print"s);
    }
}