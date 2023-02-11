#include "input_reader.h"
#include <vector>
#include <string>

using namespace std::literals;
using namespace std::string_view_literals;

transport::RawBus readinput::detail::ExtractStops(std::istream& input) {
    /* извлечение остановок марштура */
    
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

    transport::RawBus raw_bus{std::move(busname), std::move(stops), is_cycle};

    return raw_bus;
}

transport::Neighbours readinput::detail::ParseNeighbour(std::string&& neighbour, const std::string& mother_stopname) {
    /* парсинг curved-расстояния до соседа-остановки и имя соседа-останови */
    
    transport::Neighbours nbr;
    // вот тут "плохое" делаю. в каждом соседе 'from' остановки храню копию 'from' остановки; почему?

    // это позволяет наполнять САМОДОСТАТОЧНЫЙ вектор 'соседей' std::vector<transport::Neighbours> neighbours;
    // благодаря этому в CreateTransportCatalogue простые инструкции:
    // - "распарсить, добавить, извлечь" с простыми типами "сосед, остановка"; вместо
    // - "распарсить, запомнить 'from' остановку, вернуть std::pair из ссылки (или указателя) на 'from' остановку и вектор ее соседей, создать вектор таких пар"

    nbr.from = mother_stopname;
    std::string::size_type m_sym = neighbour.find('m');

    nbr.curved = std::stoi(neighbour.substr(0, m_sym));
    neighbour.erase(0, m_sym + 5);
    nbr.to = std::move(neighbour);
    
    return nbr; 
}

std::pair<transport::Stop, std::vector<transport::Neighbours>> readinput::detail::ExtractStopAndNeigbours(std::istream& input) {
    /* извлечение остановки и ее многочисленных соседей */
    
    // парсим и создаем остановку
    transport::Stop stop;

    std::string raw;
    getline(input, raw);

    std::string::size_type colon = raw.find_first_of(':');
    std::string stopname = {raw.begin(), raw.begin() + colon};
    raw.erase(0, colon + 2);
    
    stop.stopname = stopname;

    std::string::size_type sz;
    double lat = std::stod(&raw[0], &sz);
    raw.erase(0, sz + 2);
    double lng = std::stod(&raw[0], &sz);
    
    stop.location = {lat, lng};

    raw.erase(0, sz + 2);

    // парсим соседей остановки с расстояниями до них
    std::vector<transport::Neighbours> meters_to_neighbours;
    std::string raw_neighbour;
    while (!raw.empty()) {
        sz = raw.find_first_of(',');
        if (sz != std::string::npos) {
            raw_neighbour = {raw.begin(), raw.begin() + sz};
            meters_to_neighbours.push_back(ParseNeighbour(std::move(raw_neighbour), stopname));
            raw.erase(0, sz + 2);
        } else {
            meters_to_neighbours.push_back(ParseNeighbour(std::move(raw), stopname));
        }
    }

    return {stop, meters_to_neighbours};
}

transport::TransportCatalogue readinput::CreateTransportCatalogue(std::istream& input) {
    /* создание транспортного каталога из потока ввода */

    transport::TransportCatalogue transport_catalogue;

    size_t n;
    input >> n;

    std::vector<transport::RawBus> buffer;
    std::vector<transport::Neighbours> neighbours;

    for (size_t i = 0; i < n; ++i) {
        std::string query;
        input >> query;
        input.get();

        if (query == "Bus"sv) {
            // перенос всех машратов в глобальный вектор "сырых" маршрутов, сырые, потому что готовый
            // маршрут состоит не из строк (копии) имен остановок, а из указателей на готовые остановки
            buffer.push_back(detail::ExtractStops(input));
        } else {
            // извлечение остановки и ее соседей с расстояниями до них
            auto stop_and_its_neighbours = detail::ExtractStopAndNeigbours(input);

            // добавление остановки
            transport_catalogue.AddStop(stop_and_its_neighbours.first);

            // перенос всех соседей в глобальный вектор соседей
            for (auto& neighbour : stop_and_its_neighbours.second) {
                neighbours.push_back(neighbour);
            }
        }
    }

    // создание индекса "готовых" маршрутов из "сырых"; в это время все возможные остановки,
    // из которых состоят маршруты, уже находятся в каталоге
    for (auto& raw_bus : buffer) {
        transport_catalogue.AddBus(raw_bus);
    }

    // создание индекса расстояний между остановками из информации о соседних остановках
    for (auto& adjasent_stops : neighbours) {
        transport_catalogue.SetCurvedDistance(adjasent_stops);
    }
    
    return transport_catalogue;
}