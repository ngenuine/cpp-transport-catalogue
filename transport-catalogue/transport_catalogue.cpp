#include "transport_catalogue.h"
#include <iomanip>
#include <iostream>
#include <unordered_set>

std::ostream& operator<<(std::ostream& out, const BusInfo bus_info) {

    if (bus_info.number_of_stops == 0) {
        out << "Bus " << bus_info.busname << ": not found";
    } else {
        out << "Bus " << bus_info.busname << ": "
            << bus_info.number_of_stops << " stops on route, "
            << bus_info.number_of_unique_stops << " unique stops, "
            << std::setprecision(6) << bus_info.route_length << " route length";
    }

    return out;
}

// RawBus::RawBus() = default;

// RawBus::~RawBus() = default;

// RawBus::RawBus(std::string& raw_name, std::vector<std::string>& raw_stops, bool raw_is_cycle) {
//     busname = std::move(raw_name);
//     stops = std::move(raw_stops);
//     is_cycle = raw_is_cycle;
// }

// RawBus::RawBus(const RawBus& other) {
//     busname = other.busname;
//     stops = other.stops;
//     is_cycle = other.is_cycle;
// }

// RawBus& RawBus::operator=(const RawBus& rhs) {
    
//     if (this != &rhs) {
//         auto rhs_copy(rhs);
//         swap(rhs_copy);
//     }

//     return *this;
// }

// RawBus::RawBus(RawBus&& other) {
//     swap(other);
// }

// RawBus& RawBus::operator=(RawBus&& rhs) {
//     swap(rhs);
//     return *this;
// }

// void RawBus::swap(RawBus& other) noexcept {
//     std::swap(busname, other.busname);
//     std::swap(stops, other.stops);
//     std::swap(is_cycle, other.is_cycle);
// }

void TransportCatalogue::AddStop(Stop& stop) {
    
    stops_.push_back({std::move(stop.stopname), stop.location});
    stopname_to_stop_[stops_.back().stopname] = &stops_.back();
}

const Stop* TransportCatalogue::FindStop(std::string_view stopname) const {
    return stopname_to_stop_.at(stopname);
}

void TransportCatalogue::AddBus(RawBus& raw_bus) {
    
    Bus bus;
    bus.busname = std::move(raw_bus.busname);
    bus.is_cycle = raw_bus.is_cycle;
    bus.stops.reserve(raw_bus.stops.size());
    
    for (const auto& stopname : raw_bus.stops) {
        bus.stops.push_back(stopname_to_stop_.at(stopname));
    }

    buses_.push_back(bus);
    busname_to_bus_[buses_.back().busname] = &buses_.back();
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