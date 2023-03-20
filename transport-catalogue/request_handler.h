#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_reader.h"

#include <optional>

class RequestHandler {
public:
    explicit RequestHandler(const JsonReader parsed_json);

    void BuildTransportDatabase();
    void BuildMapRenderer();

    void SolveStatRequests();
    void RenderMap(std::ostream& out);

    void PrintSolution(std::ostream& out) const;

private:
    // RequestHandler использует композицию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const JsonReader parsed_json_;
    transport::TransportCatalogue db_;
    renderer::MapRenderer renderer_;
    std::optional<json::Document> solved_;
};