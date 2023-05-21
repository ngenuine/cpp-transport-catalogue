#pragma once

#include "domain.h"
#include "json.h"

#include <vector>
#include <string>
#include <istream>
#include <unordered_map>
#include <variant>
#include <memory>

class JsonReader {
public:

    const std::vector<RawBus>& GetBusRequests() const;
    const std::vector<Stop>& GetStopRequests() const;
    const std::vector<Neighbours>& GetDistancesRequests() const;
    const std::vector<std::unique_ptr<Request>>& GetStatRequests() const;
    const RenderSettings& GetRenderSettings() const;
    RoutingSettings GetRoutingSettings() const;

    void LoadJSON(std::istream& input);
    
private:
    std::vector<RawBus> bus_requests_;
    std::vector<Stop> stop_requests_;
    std::vector<Neighbours> distances_;
    std::vector<std::unique_ptr<Request>> stat_requests_;
    RenderSettings render_settings_;
    RoutingSettings routing_settings_;

    RawBus ExtractBus(const json::Node& map_node);
    Stop ExtractStop(const json::Node& map_node);
};