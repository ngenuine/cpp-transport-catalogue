#include "transport_catalogue.h"
// #include "test_framework.h"
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <set>

namespace transport {

std::ostream& operator<<(std::ostream& out, const std::unordered_set<Bus*, BusPointerHasher>& bus_set) {
    std::set<std::string_view> eh_zrya;
    
    for (auto bus = bus_set.begin(); bus != bus_set.end(); ++bus) {
        eh_zrya.insert((*bus)->busname);
    }

    auto busname = eh_zrya.begin();
    for (size_t cnt = 0; cnt < eh_zrya.size() - 1; ++cnt) {
        out << *(busname++) << ' ';
    }

    out << *busname;

    return out;
}

std::ostream& operator<<(std::ostream& out, const BusInfo& bus_info) {

    if (bus_info.number_of_stops == 0) {
        out << "Bus "sv << bus_info.busname << ": not found"sv;
    } else {
        out << "Bus "sv << bus_info.busname << ": "sv
            << bus_info.number_of_stops << " stops on route, "sv
            << bus_info.number_of_unique_stops << " unique stops, "sv
            << std::setprecision(6) << bus_info.route_length << " route length"sv;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const StopInfo& stop_info) {

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
    
    stop_info_[stops_.back().stopname];
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

        bus_info.route_length = GetBusLength(existing_bus);

        bus_info_[existing_bus->busname] = bus_info;
    }

    return bus_info_.at(existing_bus->busname);
}

const StopInfo TransportCatalogue::GetStopInfo(std::string& stopname) {
    
    if (stop_info_.count(stopname) == 0) {
        StopInfo not_found_stop;
        not_found_stop.stopname = std::move(stopname);
        not_found_stop.is_exist = false;
        return not_found_stop;
    }

    return {stopname, &stop_info_.at(stopname), true}; // move(stopname) тут не надо, а то stop_info_.at сломается
}

double TransportCatalogue::GetBusLength(const Bus* bus) {

    double one_way_route_length = 0.0;
    double from_to_distance = 0.0;

    for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
        
        Stop* from = bus->stops[i];
        Stop* to = bus->stops[i + 1];
        
        std::pair<Stop*, Stop*> from_to = {from, to};  

        if (distances_between_stops_.count(from_to) == 0) {
            from_to_distance = ComputeDistance(from->location, to->location);
            distances_between_stops_[from_to] = from_to_distance;
            one_way_route_length += from_to_distance;
        } else {
            one_way_route_length += distances_between_stops_.at(from_to);
        }
    }

    if (bus->is_cycle) {
        return one_way_route_length;
    }

    return one_way_route_length * 2;
}

void TransportCatalogue::MapStopToBus(Bus* bus) {
    for (const Stop* stop : bus->stops) {
        stop_info_[stop->stopname].insert(bus);
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
