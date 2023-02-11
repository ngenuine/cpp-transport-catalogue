#include "transport_catalogue.h"
// #include "test_framework.h"
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <set>
#include <cassert>

namespace transport {
std::ostream& operator<<(std::ostream& out, const std::set<std::string_view>& bus_set) {
    
    auto busname = bus_set.begin();
    for (size_t cnt = 0; cnt < bus_set.size() - 1; ++cnt) {
        out << *(busname++) << ' ';
    }

    out << *busname;

    return out;
}

std::ostream& operator<<(std::ostream& out, const transport::BusInfo& bus_info) {

    if (bus_info.number_of_stops == 0) {
        out << "Bus "sv << bus_info.busname << ": not found"sv;
    } else {
        out << "Bus "sv << bus_info.busname << ": "sv
            << bus_info.number_of_stops << " stops on route, "sv
            << bus_info.number_of_unique_stops << " unique stops, "sv
            << std::setprecision(6) << bus_info.bus_curved_length << " route length, "sv
            << bus_info.curvature << " curvature"sv;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const transport::StopInfo& stop_info) {

    if (!stop_info.is_exist) {
        out << "Stop "sv << stop_info.stopname << ": not found"sv;
    } else {
        if (stop_info.buses_across_stop->size() == 0) {
            out << "Stop "sv << stop_info.stopname << ": no buses"sv;
        } else {
            out << "Stop "sv
                << stop_info.stopname
                << ": buses "sv
                << *(stop_info.buses_across_stop);
        }
    }

    return out;
}

void TransportCatalogue::AddStop(Stop& stop) {    
    stops_.push_back({std::move(stop.stopname), stop.location});
    stopname_to_stop_[stops_.back().stopname] = &stops_.back();
    
    stop_info_[&stops_.back()];
}

void TransportCatalogue::AddBus(RawBus& raw_bus) {
    
    Bus bus;
    bus.busname = raw_bus.busname;
    bus.is_cycle = raw_bus.is_cycle;
    bus.stops.reserve(raw_bus.stops.size());
    
    for (const auto& stopname : raw_bus.stops) {
        bus.stops.push_back(stopname_to_stop_.at(stopname));
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

    if (bus_info_.count(busname) == 0) {
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

        bus_info_[existing_bus->busname] = bus_info;
    }

    return bus_info_.at(existing_bus->busname);
}

const StopInfo TransportCatalogue::GetStopInfo(std::string& stopname) {

    const Stop* existing_stop = FindStop(stopname);
    // прим.: если делаешь так std::unordered_map<const Stop*, std::set<std::string_view>, StopHasher> stop_info_; то можно не конст_кастить
    // auto nonconst_stop = const_cast<Stop*>(existing_stop);
    
    if (existing_stop == nullptr) {
        StopInfo not_found_stop;
        not_found_stop.stopname = std::move(stopname);
        not_found_stop.is_exist = false;
        return not_found_stop;
    }

    return {stopname, &stop_info_.at(existing_stop), true};
}

void TransportCatalogue::SetCurvedDistance(Neighbours& neighbours) {
    /* установить реальное (по дорогам, а не по прямой) расстояние между остановками */

    // FindStop возвращает const Stop*; на обявление пары без const компилятор ругается :( 
    std::pair<const Stop*, const Stop*> from_to = {FindStop(neighbours.from), FindStop(neighbours.to)};

    // if (distances_between_stops_.count(from_to) == 0) {
    //     distances_between_stops_[from_to];
    // }

    distances_between_stops_[from_to].curved = neighbours.curved;
}

const Distance& TransportCatalogue::GetFromToDistance(const Stop* from, const Stop* to) {
    /* в виде структуры Distance выдаст расстояние, которое имеется в базе */

    std::pair<const Stop*, const Stop*> from_to = {from, to};

    if (distances_between_stops_.count(from_to) == 0) {
        distances_between_stops_[from_to];
    }

    Distance& distance = distances_between_stops_.at(from_to);

    if (distance.straight == 0.0) {
        distance.straight = ComputeDistance(from->location, to->location);
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
        stop_info_[stop].insert(bus->busname);
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