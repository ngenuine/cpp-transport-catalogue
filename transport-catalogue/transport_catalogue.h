#pragma once

#include "geo.h"
#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

using namespace std::literals;
using namespace std::string_view_literals;

namespace transport {

struct Stop {
    // stopname строка, потому что много откуда сюда будут смотреть вьюхи
    std::string stopname = ""s;
    Coordinates location;
};

struct RawBus {
    // RawBus владеет строками, потому что ему еще некуда вьюхами смотреть, т.к по условию задачи
    // маршруты (Bus, RawBus) могут идти до остановок, которые в них содержатся
    std::string busname = ""s;
    std::vector<std::string> stops;
    bool is_cycle = false;
};

struct Bus {
    // busname строка, т.к. в принципе больше нет нигде места, куда вьюхой можно было бы смотреть busname-ом
    std::string busname = ""s;
    bool is_cycle = false;
    std::vector<Stop*> stops;
};

struct PairStopPointersHasher {
    size_t operator() (const std::pair<Stop*, Stop*> from_to) const {
        return pt_hasher_(from_to.first) + pt_hasher_(from_to.second);
    }

private:
    std::hash<Stop*> pt_hasher_{};
};

struct BusPointerHasher {
    size_t operator() (Bus* bus) const {
        return pt_hasher_(bus);
    }

private:
    std::hash<Bus*> pt_hasher_{};
};

struct BusInfo {
    // в момент получения BusInfo справочник полностью готов;
    // виьюха busname тут модет смотреть на строку busname из дэка buses_;
    std::string_view busname = ""sv;
    int number_of_stops = 0;
    int number_of_unique_stops = 0;
    double route_length = 0.0;
};

struct StopInfo {
    std::string stopname = ""s;
    std::unordered_set<Bus*, BusPointerHasher>* buses_across_stop;
    bool is_exist = false;
};

std::ostream& operator<<(std::ostream& out, const BusInfo& bus_info);
std::ostream& operator<<(std::ostream& out, const std::unordered_set<Bus*, BusPointerHasher>& bus_set);
std::ostream& operator<<(std::ostream& out, const StopInfo& stop_info);

class TransportCatalogue {
public:
    void AddStop(Stop& stop);
    void AddBus(RawBus& raw_bus);
    const Stop* FindStop(std::string_view stopname) const;
    const Bus* FindBus(std::string_view busname) const;
    const BusInfo GetBusInfo(std::string_view busname);
    const StopInfo GetStopInfo(std::string& stopname);

private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> busname_to_bus_;
    std::unordered_map<std::pair<Stop*, Stop*>, double, PairStopPointersHasher> distances_between_stops_;
    std::unordered_map<std::string_view, BusInfo> bus_info_;
    std::unordered_map<std::string_view, std::unordered_set<Bus*, BusPointerHasher>> stop_info_;

    double GetBusLength(const Bus* bus);
    void MapStopToBus(Bus* bus);
};

} // namespace transport

// -------- Начало модульных тестов транспортного справочника ----------
namespace tests {

    void AddingStop();
    void AddingBus();
    void FindingStop();
    void FindingBus();
    void GettingBusLength();
    void GettingBusInfo();

namespace input {
    void ReadInput();
}

namespace output {
    void WriteOutput();
}

void RunTransportCatalogueTests();
void RunInputTests();
void RunOutputTests();

template <typename ReturnedType>
void RunTestImpl(ReturnedType func, std::string_view func_name) {
    func();
    std::cerr << func_name << " OK"sv << std::endl;
}


} // namespace transport

#define RUN_TEST(func) tests::RunTestImpl((func), #func)
// -------- Окончание модульных тестов транспортного справочника --------