#pragma once

#include "domain.h"

#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <optional>

using namespace std::literals;
using namespace std::string_view_literals;

namespace transport {

class TransportCatalogue {
public:
    void AddStop(const Stop& stop);
    std::optional<size_t> GetUsefulStopId(std::string_view stopname) const;
    void AddBus(const RawBus& raw_bus);
    const Stop* FindStop(StopName stopname) const;
    const Bus* FindBus(BusName busname) const;
    const BusInfo GetBusInfo(BusName busname);
    const StopInfo GetStopInfo(StopName stopname);
    void SetCurvedDistance(const Neighbours& neighbours);
    const std::map<StopName, geo::Coordinates>& GetUsefulStopCoordinates() const;
    const std::map<BusName, Bus*>& GetAllBuses() const;
    const std::map<StopName, Stop*>& GetAllStops() const;
    const std::deque<Bus>& GetAllBusesInOrder() const;
    const std::deque<Stop>& GetAllStopsInOrder() const;
    const std::unordered_map<std::pair<const Stop*, const Stop*>, Distance, StopHasher>& GetDistancesList() const;

private:
    // для сериализации
    std::deque<Stop> stops_; // сериализцется через stopname_to_stop_
    std::deque<Bus> buses_;  // сериализцется через busname_to_bus_
    std::unordered_map<std::pair<const Stop*, const Stop*>, Distance, StopHasher> distances_between_stops_;
    std::map<StopName, geo::Coordinates> stopname_to_useful_stop_coordinates_;
    std::unordered_map<StopName, std::set<BusName>> stopname_to_stopinfo_;

    // заполнить после десериализации
    std::map<StopName, Stop*> stopname_to_stop_;
    std::map<BusName, Bus*> busname_to_bus_; // маршруты надо отcортировать лексикографически, т.к. от этого зависит их цвет 
    std::unordered_map<BusName, BusInfo> busname_to_businfo_; // это поле не затрагивается при наполнении базы данных

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