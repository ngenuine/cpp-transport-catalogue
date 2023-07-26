#pragma once

#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "graph.h"
#include "transport_router.h"
#include "request_handler.h"

#include <fstream>

#include <map_renderer.pb.h>
#include <transport_catalogue.pb.h>
#include <transport_router.pb.h>
#include <graph.pb.h>

namespace serialization {

class Serializator {
public:
    explicit Serializator(TransportData& data);

    void Serialize(std::ostream& output);
    void Deserialize(std::istream& input);

private:
    transport::TransportCatalogue& t_cat_;
    renderer::MapRenderer& m_rend_;
    std::unique_ptr<transport_router::TransportRouter<double>>& t_route_;

    pb_msg::TransportCatalogue transport_catalogue_;

    void SeializeTransportCatalogue();
    void SeializeMapRenderer();
    void SeializeTransportRouter();

    void DeseializeTransportCatalogue();
    void DeseializeMapRenderer();
    void DeseializeTransportRouter();
};

} // namespace serialization