#pragma once

#include "geo.h"

#include <string>
#include <vector>
#include <set>
#include <variant>
#include <unordered_map>

using namespace std::literals;
using namespace std::string_view_literals;

using BusName = std::string_view;
using StopName = std::string_view;

using Setting = std::variant<std::nullptr_t, double, int, std::pair<double, double>, std::vector<std::string>, std::string>;
using RenderSettings = std::unordered_map<std::string, Setting>;

struct Request {
    int id = 0;
    std::string type = ""s;
    std::string name = ""s;
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
    // stopname строка, потому что много откуда сюда будут смотреть вьюхи
    std::string stopname = ""s;
    geo::Coordinates location;
};

struct RawBus {
    std::string busname = ""s;
    std::vector<std::string> stops;
    bool is_cycle = false;
};

struct Bus {
    // busname строка, т.к. нет нигде места, куда вьюхой можно было бы смотреть busname-ом
    std::string busname = ""s;
    bool is_cycle = false;
    std::vector<Stop*> stops;
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

struct BusInfo {
    // в момент получения BusInfo справочник полностью готов;
    // вьюха busname тут может смотреть на строку busname из дэка buses_;
    BusName busname = ""sv;
    int number_of_stops = 0;
    int number_of_unique_stops = 0;
    double bus_straight_length = 0.0;
    int bus_curved_length = 0;
    double curvature = 0.0;
};

struct StopInfo {
    // в момент получения StopInfo справочник полностью готов;
    // вьюха stopname тут может смотреть на строку stopname из дэка stops_;
    StopName stopname = ""s;
    std::set<BusName>* buses_across_stop;
    bool is_exist = false;
};