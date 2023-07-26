#include "request_handler.h"
#include "json_builder.h"
#include "router.h"

#include <cassert>

RequestHandler::RequestHandler(const JsonReader& parsed_json, TransportData& transport_data, ReadMode mode)
    : parsed_json_(parsed_json)
    , db_(transport_data.t_cat)
    , renderer_(transport_data.m_rend)
    , transport_router_(transport_data.t_route)
    {
        if (mode == ReadMode::MAKE_BASE) {
            BuildTransportDatabase();
            BuildMapRenderer();
            BuildTransportRouter();
        }
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
        // вместо такого формирования ответа
        // return json::Dict{
        //     {"request_id"s, id},
        //     {"error_message"s, "not found"s}};

        // более удобный и не способствующий ошибкам способ
        return json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(id)
                        .Key("error_message"s).Value("not found"s)
                    .EndDict()
                    .Build();
    };

    json::Array answer;

    if (parsed_json_.GetStatRequests().size() == 0) {
        throw std::logic_error("Nothing to solve"s);
    }
    
    for (const auto& request_ptr : parsed_json_.GetStatRequests()) {
        
        if (request_ptr->type == "Bus"sv) {
            
            Transport* bus = static_cast<Transport*>(request_ptr.get());

            const BusInfo businfo = db_.GetBusInfo(bus->name);

            if (businfo.number_of_stops > 0) {
                answer.push_back(
                    json::Builder{}
                        .StartDict()
                            .Key("curvature"s).Value(businfo.curvature)
                            .Key("request_id"s).Value(bus->id)
                            .Key("route_length"s).Value(businfo.bus_curved_length)
                            .Key("stop_count"s).Value(businfo.number_of_stops)
                            .Key("unique_stop_count"s).Value(businfo.number_of_unique_stops)
                        .EndDict()
                        .Build());
            } else {
                answer.push_back(not_found(bus->id));
            }
        } else if (request_ptr->type == "Stop"sv) {

            Transport* stop = static_cast<Transport*>(request_ptr.get());

            const StopInfo stopinfo = db_.GetStopInfo(stop->name);
            json::Array buses;

            if (stopinfo.is_exist) {
                for (BusName busname : *stopinfo.buses_across_stop) {
                    buses.push_back(std::string(busname));
                }

                // move(buses) и id превратятся в Node, потому что Dict = std::map<string, Node>
                // и потому что у Node разрешена неявное преобразование типов
                answer.push_back(
                    json::Builder{}
                        .StartDict()
                            .Key("buses"s).Value(move(buses))
                            .Key("request_id"s).Value(stop->id)
                        .EndDict()
                        .Build());
            } else {
                answer.push_back(not_found(stop->id));
            }
        } else if (request_ptr->type == "Map"sv) {

            Map* map_request = static_cast<Map*>(request_ptr.get());
            
            std::stringstream svg_map;
            RenderMap(svg_map);
            answer.push_back(
                json::Builder{}
                    .StartDict()
                        .Key("map"s).Value(svg_map.str())
                        .Key("request_id"s).Value(map_request->id)
                    .EndDict()
                    .Build());
        } else if (request_ptr->type == "Route"sv) {

            Route* route = static_cast<Route*>(request_ptr.get());
            std::optional<graph::Router<double>::RouteInfo> builded_route = transport_router_->BuildRouteBetweenStops(route->from, route->to);
            
            if (builded_route) {
                if (builded_route.value().edges.size() == 0) {
                    answer.push_back(
                    json::Builder{}
                        .StartDict()
                            .Key("items"s).StartArray().EndArray()
                            .Key("request_id"s).Value(route->id)
                            .Key("total_time"s).Value(0)
                        .EndDict()
                        .Build());
                } else {
                    json::Array items;

                    for (const auto edge_id : builded_route.value().edges) {
                        const graph::Edge<double>& edge = transport_router_->GetGraph().GetEdge(edge_id);
                        if (edge.span == 0) {
                            items.push_back(
                                json::Builder{}
                                    .StartDict()
                                        .Key("stop_name"s).Value(std::string(edge.stop_name_from))
                                        .Key("time"s).Value(edge.weight)
                                        .Key("type"s).Value("Wait"s)
                                    .EndDict()
                                    .Build());
                        } else if (edge.span > 0) {
                            items.push_back(
                                json::Builder{}
                                    .StartDict()
                                        .Key("bus"s).Value(std::string(edge.busname))
                                        .Key("span_count"s).Value(static_cast<int>(edge.span))
                                        .Key("time"s).Value(edge.weight)
                                        .Key("type"s).Value("Bus"s)
                                    .EndDict()
                                    .Build());
                        }
                    }

                    answer.push_back(
                        json::Builder{}
                            .StartDict()
                                .Key("items"s).Value(std::move(items))
                                .Key("request_id"s).Value(route->id)
                                .Key("total_time"s).Value(builded_route.value().weight)
                            .EndDict()
                            .Build());
                }
            } else {
                answer.push_back(not_found(route->id));
            }

        } else {

            throw std::logic_error("Unknown request: " + "\""s + (request_ptr->type) + "\""s);
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

void RequestHandler::BuildTransportRouter() {
    transport_router_ = std::make_unique<transport_router::TransportRouter<double>>(db_, parsed_json_.GetRoutingSettings());
}