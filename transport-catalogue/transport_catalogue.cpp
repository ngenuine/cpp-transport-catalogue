#include "transport_catalogue.h"
// #include "test_framework.h"

#include <iostream>
#include <unordered_set>
#include <set>
#include <cassert>

namespace transport {

const int GAP = 10000; // множетсво остановок, участвующих в построении маршрутов, не должно быть больше чем это число  

void TransportCatalogue::AddStop(const Stop& stop) {
    // всем остановкам выдается уникальный номер, тут я его при помощи GAP отодвигаю подальше от 0,
    // потому что у тех остановок, которые участвуют в построении маршрутов, этот id
    // будет перезаписан в методе AddBus на число от 0 до N = <количество полезных остановок> * 2 - 1;
    // id кодирует вершины в роутере: каждой остановке -- по две смежные вершины, но это не просто рандомные
    // уникальные числа, а от нуля подряд идущие уникальные числа
    size_t id = stops_.size() + GAP;
    stops_.push_back({stop.stopname, stop.location, id});
    
    stopname_to_stop_[stops_.back().stopname] = &stops_.back();
    
    stopname_to_stopinfo_[stops_.back().stopname];
}

std::optional<size_t> TransportCatalogue::GetUsefulStopId(std::string_view stopname) const {
    if (stopname_to_useful_stop_coordinates_.count(stopname) == 1) {
        return stopname_to_stop_.at(stopname)->id;
    }

    return std::nullopt;
}

void TransportCatalogue::AddBus(const RawBus& raw_bus) {

    Bus bus;
    bus.busname = raw_bus.busname;
    bus.is_cycle = raw_bus.is_cycle;
    bus.stops.reserve(raw_bus.stops.size());

    // отодвинем id автобусов, так как 0 это спецзначение для id автобусов, используещееся при сериализации
    bus.id = buses_.size() + GAP;

    for (const auto& stopname : raw_bus.stops) {
        Stop* stop = stopname_to_stop_.at(stopname);
        bus.stops.push_back(stop);

        // каждая участвующая в построении маршрутов, получит свой id;
        // эти id обязательно от 0 и больше по порядку без пропусков должны быть -- для роутера это важно;
        // например, если в программе всего 10 остановок, то в роутере будет сгенерировано 0, 1, ..., 19 вершин,
        // и если остановка А имеет id_1 = 15, остановка В имеет id_2 = 150, то когда роутер будет искать путь
        // из А в В, то он на самом деле будет искать путь из вершины 15 * 2 = 30 в вершину 150 * 2 + 1 = 301,
        // когда вершин всего лишь 0, 1, ..., 19, поэтому это ключевой момент id полезной остановки должен быть перезаписан
        if (stopname_to_useful_stop_coordinates_.count(stopname) == 0) {
            geo::Coordinates useful_coordinates = stop->location;
            stop->id = stopname_to_useful_stop_coordinates_.size(); // перезаписываем ранее выданный уникальный номер
            stopname_to_useful_stop_coordinates_.insert({stop->stopname, useful_coordinates});
        }
    }

    buses_.push_back(bus);
    busname_to_bus_[buses_.back().busname] = &buses_.back();

    MapStopToBus(&buses_.back());
}

const Stop* TransportCatalogue::FindStop(std::string_view stopname) const {
    if (stopname_to_stop_.count(stopname) == 1) {
        return stopname_to_stop_.at(stopname);
    }

    return nullptr;    
}

const Bus* TransportCatalogue::FindBus(std::string_view busname) const {
    if (busname_to_bus_.count(busname) == 1) {
        return busname_to_bus_.at(busname);
    }

    return nullptr;
}

const BusInfo TransportCatalogue::GetBusInfo(std::string_view busname) {
    /* выдаст информацию по маршруту */

    const Bus* existing_bus = FindBus(busname);

    if (existing_bus == nullptr) {
        BusInfo not_found_bus;
        not_found_bus.busname = busname;
        return not_found_bus;
    }

    if (busname_to_businfo_.count(busname) == 0) {
        BusInfo bus_info;

        bus_info.busname = existing_bus->busname;
        bus_info.number_of_stops = existing_bus->is_cycle ?
                                   existing_bus->stops.size() :
                                   existing_bus->stops.size() * 2 - 1;

        std::unordered_set<std::string_view> unique_stops;
        for (const Stop* stop : existing_bus->stops) {
            unique_stops.insert(stop->stopname);
        }
        bus_info.number_of_unique_stops = unique_stops.size();

        const Distance distance = GetBusLength(existing_bus);

        bus_info.bus_straight_length = distance.straight;
        bus_info.bus_curved_length = distance.curved;
        bus_info.curvature = bus_info.bus_curved_length * 1.0 / bus_info.bus_straight_length;

        busname_to_businfo_[existing_bus->busname] = bus_info;
    }

    return busname_to_businfo_.at(existing_bus->busname);
}

const StopInfo TransportCatalogue::GetStopInfo(std::string_view stopname) {
    /* выдаст информацию по остановке */

    const Stop* existing_stop = FindStop(stopname);
    
    if (existing_stop == nullptr) {
        StopInfo not_found_stop;
        not_found_stop.stopname = stopname;
        not_found_stop.is_exist = false;
        return not_found_stop;
    }

    return {existing_stop->stopname, &stopname_to_stopinfo_.at(existing_stop->stopname), true};
}

void TransportCatalogue::SetCurvedDistance(const Neighbours& neighbours) {
    /* установить реальное (по дорогам, а не по прямой) расстояние между остановками */

    std::pair<const Stop*, const Stop*> from_to = {FindStop(neighbours.from), FindStop(neighbours.to)};

    distances_between_stops_[from_to].curved = neighbours.curved;
}

const std::map<StopName, geo::Coordinates>& TransportCatalogue::GetUsefulStopCoordinates() const {
    return stopname_to_useful_stop_coordinates_;
}

const std::map<BusName, Bus*>& TransportCatalogue::GetAllBuses() const {
    return busname_to_bus_;
}

const std::map<StopName, Stop*>& TransportCatalogue::GetAllStops() const {
    return stopname_to_stop_;
}

const std::deque<Bus>& TransportCatalogue::GetAllBusesInOrder() const {
    return buses_;
}
const std::deque<Stop>& TransportCatalogue::GetAllStopsInOrder() const {
    return stops_;
}

const std::unordered_map<std::pair<const Stop*, const Stop*>, Distance, StopHasher>& TransportCatalogue::GetDistancesList() const {
    return distances_between_stops_;
}

const Distance& TransportCatalogue::GetFromToDistance(const Stop* from, const Stop* to) {
    /* в виде структуры Distance выдаст расстояние, которое имеется в базе */

    std::pair<const Stop*, const Stop*> from_to = {from, to};

    if (distances_between_stops_.count(from_to) == 0) {
        distances_between_stops_[from_to];
    }

    Distance& distance = distances_between_stops_.at(from_to);

    if (distance.straight == 0.0) {
        distance.straight = geo::ComputeDistance(from->location, to->location);
    }

    return distance;
}

const Distance TransportCatalogue::GetBusLength(const Bus* bus) {
    /* рассчитает географическую длину маршрута и длину маршрута по дорогам */

    Distance bus_length;

    for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
        
        const Stop* from = bus->stops[i];
        const Stop* to = bus->stops[i + 1];

        const Distance& fwd_dist = GetFromToDistance(from, to);
        const Distance& bwd_dist = GetFromToDistance(to, from);

        const int fwd_dist_curved = fwd_dist.curved;
        const int bwd_dist_curved = bwd_dist.curved;

        bus_length.curved += fwd_dist_curved;

        if (bus->is_cycle) {
            bus_length.straight += fwd_dist.straight;

            if (fwd_dist_curved == 0.0) {
                bus_length.curved += bwd_dist_curved;
            }
        } else {
            bus_length.straight += (fwd_dist.straight * 2);

            if (fwd_dist_curved == 0.0) {
                bus_length.curved += bwd_dist_curved * 2;
            } else {
                bus_length.curved += bwd_dist_curved;

                if (bwd_dist_curved == 0.0) {
                    bus_length.curved += fwd_dist_curved;
                }
            }
        }
    }

    return bus_length;
}

void TransportCatalogue::MapStopToBus(Bus* bus) {
    for (Stop* stop : bus->stops) {
        stopname_to_stopinfo_[stop->stopname].insert(bus->busname);
    }
}

} // namespace transport

namespace tests {

void input::ReadInput() {
    
}

void AddingStop() {
    
}

void AddingBus() {
    
}

void FindingStop() {
    
}

void FindingBus() {
    
}

void GettingBusLength() {
    
}

void GettingBusInfo() {
    
}

void output::WriteOutput() {

}

// Точки входа в тестирование
void RunInputTests() {
    using input::ReadInput;
    RUN_TEST(ReadInput);
}

void RunTransportCatalogueTests() {
    RUN_TEST(AddingStop);
    RUN_TEST(AddingBus);
    RUN_TEST(FindingStop);
    RUN_TEST(FindingBus);
    RUN_TEST(GettingBusLength);
    RUN_TEST(GettingBusInfo);

}

void RunOutputTests() {
    using output::WriteOutput;
    RUN_TEST(WriteOutput);
}

} // namespace tests