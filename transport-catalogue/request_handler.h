#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_reader.h"
#include "transport_router.h"

#include <optional>

class RequestHandler {
public:
    explicit RequestHandler(const JsonReader& parsed_json);

    void BuildTransportDatabase();
    void BuildMapRenderer();
    void BuildTransportRouter();

    void SolveStatRequests();
    void RenderMap(std::ostream& out);

    void PrintSolution(std::ostream& out) const;

private:
    // RequestHandler использует композицию объектов "Транспортный Справочник", "Визуализатор Карты" и "Маршрутизатор"
    const JsonReader& parsed_json_;
    transport::TransportCatalogue db_;
    renderer::MapRenderer renderer_;
    std::unique_ptr<transport_router::TransportRouter<double>> transport_router_;

    std::optional<json::Document> solved_;
};