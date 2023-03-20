#pragma once

#include "domain.h"

#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <optional>

using namespace std::literals;
using namespace std::string_view_literals;

namespace transport {

class TransportCatalogue {
public:
    void AddStop(const Stop& stop);
    void AddBus(const RawBus& raw_bus);
    const Stop* FindStop(StopName stopname) const;
    const Bus* FindBus(BusName busname) const;
    const BusInfo GetBusInfo(BusName busname);
    const StopInfo GetStopInfo(StopName stopname);
    void SetCurvedDistance(const Neighbours& neighbours);
    const std::map<StopName, geo::Coordinates>& GetUsefulStopCoordinates() const;
    const std::map<BusName, Bus*>& GetAllBuses() const;
    
private:
    std::deque<Stop> stops_;
    std::map<StopName, Stop*> stopname_to_stop_;
    std::map<StopName, geo::Coordinates> stopname_to_useful_stop_coordinates_;
    std::deque<Bus> buses_;
    std::map<BusName, Bus*> busname_to_bus_; // маршруты надо отcортировать лексикографически, т.к. от этого зависит их цвет
    std::unordered_map<std::pair<const Stop*, const Stop*>, Distance, StopHasher> distances_between_stops_;
    std::unordered_map<BusName, BusInfo> busname_to_info_;
    std::unordered_map<StopName, std::set<BusName>> stopname_to_stopinfo_;

    const Distance& GetFromToDistance(const Stop* from, const Stop* to);
    const Distance GetBusLength(const Bus* bus);
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


} // namespace tests

#define RUN_TEST(func) tests::RunTestImpl((func), #func)
// -------- Окончание модульных тестов транспортного справочника --------