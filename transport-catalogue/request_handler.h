#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_reader.h"
#include "transport_router.h"

#include <optional>

struct TransportData {
    TransportData() = default;

    transport::TransportCatalogue t_cat;
    renderer::MapRenderer m_rend;
    std::unique_ptr<transport_router::TransportRouter<double>> t_route;
};

class RequestHandler {
public:
    explicit RequestHandler(const JsonReader& parsed_json, TransportData& transport_data, ReadMode mode);

    void SolveStatRequests();
    void RenderMap(std::ostream& out);
    void PrintSolution(std::ostream& out) const;

private:
    const JsonReader& parsed_json_;
    transport::TransportCatalogue& db_;
    renderer::MapRenderer& renderer_;
    std::unique_ptr<transport_router::TransportRouter<double>>& transport_router_;

    std::optional<json::Document> solved_;

    void BuildTransportDatabase();
    void BuildMapRenderer();
    void BuildTransportRouter();
};