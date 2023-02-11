#pragma once

#include "transport_catalogue.h"
#include <istream>


namespace readinput {
transport::TransportCatalogue CreateTransportCatalogue(std::istream& input);

namespace detail {
transport::RawBus ExtractStops(std::istream& input);
transport::Neighbours ParseNeighbour(std::string&& neighbour, const std::string& stopname);
std::pair<transport::Stop, std::vector<transport::Neighbours>> ExtractStopAndNeigbours(std::istream& input);
} // namespace detail

} // namespace readinput