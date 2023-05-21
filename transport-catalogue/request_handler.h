#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_reader.h"

#include "router.h"

#include <optional>

class RequestHandler {
public:
    explicit RequestHandler(const JsonReader& parsed_json);

    void BuildTransportDatabase();
    void BuildMapRenderer();
    void BuildRouter();

    void SolveStatRequests();
    void RenderMap(std::ostream& out);

    void PrintSolution(std::ostream& out) const;

private:

    // RequestHandler использует композицию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const JsonReader& parsed_json_;
    transport::TransportCatalogue db_;
    renderer::MapRenderer renderer_;

    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;

    std::optional<json::Document> solved_;
};