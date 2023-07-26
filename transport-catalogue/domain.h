#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <unordered_map>

using namespace std::literals;
using namespace std::string_view_literals;

using BusName = std::string_view;
using StopName = std::string_view;

struct Request {
    Request();
    virtual ~Request();
    Request(int id_, std::string type_);
    int id;
    std::string type;
};

struct Transport : public Request {
    Transport(int id_, std::string type_, std::string name_);
    Transport(Transport&& other);

    std::string name;
};

struct Map : public Request {
    Map(int id_, std::string type_);
    Map(Map&& other);
};

struct Route : public Request {
    Route(int id, std::string type, std::string from, std::string to);
    Route(Route&& other);
    
    std::string from; // остановка может и не принадлежать ни одному маршруту
    std::string to;   // остановка может и не принадлежать ни одному маршруту
};

struct Neighbours {
    std::string from = ""s;
    std::string to = ""s;
    int curved = 0;
};

struct Distance {
    double straight = 0.0;
    int curved = 0;
};

struct Stop {
    std::string stopname = ""s;
    geo::Coordinates location;
    size_t id = 0;
};

struct RawBus {
    std::string busname = ""s;
    std::vector<std::string> stops;
    bool is_cycle = false;
};

struct Bus {
    std::string busname = ""s;
    bool is_cycle = false;
    std::vector<Stop*> stops;
    size_t id = 0;
};

struct StopHasher {
    size_t operator() (std::pair<const Stop*, const Stop*> from_to) const {
        return pt_hasher_(from_to.first) + pt_hasher_(from_to.second);
    }

    size_t operator() (const Stop* stop) const {
        return pt_hasher_(stop);
    }
private:
    std::hash<const Stop*> pt_hasher_{};
};

struct PairSizetHasher {
    size_t operator() (std::pair<size_t, size_t> from_to) const {
        return from_to.first * 37 + from_to.second * 37 * 37 + ((from_to.first + from_to.second) * 37 * 37 * 37);
    }
};

struct BusInfo {
    BusName busname = ""sv;
    int number_of_stops = 0;
    int number_of_unique_stops = 0;
    double bus_straight_length = 0.0;
    int bus_curved_length = 0;
    double curvature = 0.0;
};

struct StopInfo {
    StopName stopname;
    std::set<BusName>* buses_across_stop;
    bool is_exist = false;
};

struct RoutingSettings {
    int bus_wait_time = 1000;
    int bus_velocity = 1000;
};

struct RenderSettings {
    double width;
    double height;

    double padding;

    double line_width;
    double stop_radius;

    int bus_label_font_size;
    std::pair<double, double> bus_label_offset;

    int stop_label_font_size;
    std::pair<double, double> stop_label_offset;

    std::string underlayer_color;
    double underlayer_width;

    std::vector<std::string> color_palette;
};

struct ProjectorSettings {
    double padding;
    double min_lon;
    double max_lat;
    double zoom_coeff;
};