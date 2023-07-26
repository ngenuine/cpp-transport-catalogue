#pragma once

#include "domain.h"
#include "json.h"

#include <vector>
#include <string>
#include <istream>
#include <unordered_map>
#include <memory>
#include <filesystem>

enum ReadMode {
    MAKE_BASE,
    PROCESS_REQUEST
};

class JsonReader {
public:

    const std::vector<RawBus>& GetBusRequests() const;
    const std::vector<Stop>& GetStopRequests() const;
    const std::vector<Neighbours>& GetDistancesRequests() const;
    const std::vector<std::unique_ptr<Request>>& GetStatRequests() const;
    const RenderSettings& GetRenderSettings() const;
    RoutingSettings GetRoutingSettings() const;
    std::filesystem::path GetOutputFilepath();
    std::filesystem::path GetInputFilepath();

    void LoadJSON(std::istream& input, ReadMode read_mode);
    
private:
    std::vector<RawBus> bus_requests_;
    std::vector<Stop> stop_requests_;
    std::vector<Neighbours> distances_;
    std::vector<std::unique_ptr<Request>> stat_requests_;
    RenderSettings render_settings_;
    RoutingSettings routing_settings_;

    std::filesystem::path output_file_;
    std::filesystem::path input_file_;

    RawBus ExtractBus(const json::Node& map_node);
    Stop ExtractStop(const json::Node& map_node);

    void MakeBase(std::istream& input);
    void MakeRequests(std::istream& input);
};