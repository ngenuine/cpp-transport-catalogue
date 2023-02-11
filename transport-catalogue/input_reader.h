#pragma once

#include "transport_catalogue.h"
#include <istream>


namespace readinput {
transport::TransportCatalogue CreateTransportCatalogue(std::istream& input);

namespace detail {
transport::RawBus ExtractStops(std::istream& input);
transport::Stop BuildStop(std::istream& input);

}

}