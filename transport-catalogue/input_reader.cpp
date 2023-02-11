#include "input_reader.h"
#include <vector>
#include <string>

using namespace std::literals;
using namespace std::string_view_literals;

RawBus ExtractStops(std::istream& input) {
    std::string busname;
    getline(input, busname, ':');
    input.get();

    char separator = '-';
    std::string stop;
    std::vector<std::string> stops;
    for (char c; input.get(c) && c != '\n'; ) {
        if (c == '>' || c == '-') {
            separator = c;
            stop.pop_back();
            input.get();
            stops.push_back(stop);
            stop.clear();
        } else {
            stop.push_back(c);
        }
    }

    stops.push_back(stop);

    bool is_cycle = true ? separator == '>' : false;

    RawBus raw_bus{std::move(busname), std::move(stops), is_cycle};

    return raw_bus;
}

TransportCatalogue CreateTransportCatalogue(std::istream& input) {
    TransportCatalogue transport_catalogue;

    size_t n;
    input >> n;

    std::vector<RawBus> buffer;

    for (size_t i = 0; i < n; ++i) {
        std::string query;
        input >> query;
        input.get();

        if (query == "Bus"sv) {
            buffer.push_back(ExtractStops(input));
        } else {
            std::string raw;
            getline(input, raw);

            size_t colon = raw.find_first_of(':');
            std::string stopname = {raw.begin(), raw.begin() + colon};
            
            Stop stop;
            stop.stopname = stopname;

            std::string::size_type sz;
            double lat = std::stod(&raw[colon + 2], &sz);
            double lng = std::stod(&raw[colon + sz + 3]);
            stop.location = {lat, lng};

            transport_catalogue.AddStop(stop);
        }
    }

    for (auto& raw_bus : buffer) {
        transport_catalogue.AddBus(raw_bus);
    }
    
    return transport_catalogue;
}