#pragma once

#include "geo.h"
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <algorithm>

using namespace std::literals;
using namespace std::string_view_literals;

struct Stop {
    // stopname строка, потому что много откуда сюда будут смотреть вьюхи
    std::string stopname = "";
    Coordinates location;
};

struct RawBus {
    // RawBus владеет строками, потому что ему еще некуда вьюхами смотреть, т.к по условию задачи
    // маршруты (Bus, RawBus) могут идти до остановок, которые в них содержатся
    std::string busname = "";
    std::vector<std::string> stops;
    bool is_cycle = false;
};

struct Bus {
    // busname строка, т.к. в принципе больше нет нигде места, куда вьюхой можно было бы смотреть busname-ом
    std::string busname = "";
    bool is_cycle = false;
    std::vector<Stop*> stops;
};

struct BusInfo {
    // в момент получения BusInfo справочник полностью готов;
    // виьюха busname тут модет смотреть на строку busname из дэка buses_;
    std::string_view busname = "";
    int number_of_stops = 0;
    int number_of_unique_stops = 0;
    double route_length = 0.0;
};

std::ostream& operator<<(std::ostream& out, const BusInfo bus_info);

struct PairPointersHasher {
    size_t operator() (const std::pair<Stop*, Stop*> from_to) const {
        return pt_hasher_(from_to.first) + pt_hasher_(from_to.second);
    }

private:
    std::hash<Stop*> pt_hasher_{};
};

class TransportCatalogue {
public:
    void AddStop(Stop& stop);
    void AddBus(RawBus& raw_bus);
    const Stop* FindStop(std::string_view stopname) const;
    const Bus* FindBus(std::string_view busname) const;
    const BusInfo GetBusInfo(std::string_view busname);

private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> busname_to_bus_;
    std::unordered_map<std::pair<Stop*, Stop*>, double, PairPointersHasher> distances_between_stops_;
    std::unordered_map<std::string_view, BusInfo> bus_info_;

    double GetBusLength(const Bus* bus);
};