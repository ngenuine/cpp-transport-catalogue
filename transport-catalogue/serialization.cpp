#include <fstream>
#include "transport_catalogue.h"
#include "transport_router.h"
#include "graph.h"
#include "map_renderer.h"
#include "domain.h"
#include "serialization.h"

#include <map_renderer.pb.h>
#include <transport_catalogue.pb.h>
#include <transport_router.pb.h>
#include <graph.pb.h>

namespace serialization {

Serializator::Serializator(TransportData& transport_data)
    : t_cat_(transport_data.t_cat)
    , m_rend_(transport_data.m_rend)
    , t_route_(transport_data.t_route)
{
}

void Serializator::SeializeTransportCatalogue() {
    // здесь у остановки есть имя и уникальный номер, который
    // можно использовать вместо имени в мапе transport_catalogue.id_to_stopname()
    const std::deque<Stop>& all_stops = t_cat_.GetAllStopsInOrder();

    // добавим остановки в массив остановок и мапу, шифрующую названия остановок числами
    for (const auto& stop : all_stops) {
        // поставим в соответствие уникальному числу название остановки
        transport_catalogue_.mutable_id_to_stopname()->insert({stop.id, stop.stopname});

        // добавим остановку в массив остановок
        pb_msg::Stop* pb_stop = transport_catalogue_.add_stop();
        
        // уникальный id остановки, по которому при десериализации можно понять имя
        pb_stop->set_unique_stop_id(stop.id);

        pb_stop->mutable_location()->set_lat(stop.location.lat);
        pb_stop->mutable_location()->set_lng(stop.location.lng);
    }

    // добавим маршруты
    const std::deque<Bus>& all_buses = t_cat_.GetAllBusesInOrder();
    for (const auto& bus : all_buses) {
        // поставим в соответствие уникальному числу название маршрута
        transport_catalogue_.mutable_id_to_busname()->insert({bus.id, bus.busname});

        // добавил маршрут в массив маршрутов
        pb_msg::Bus* pb_bus = transport_catalogue_.add_bus();

        // уникальный id маршрута, по которому при десериализации можно понять имя
        pb_bus->set_unique_bus_id(bus.id);
        pb_bus->set_is_cycle(bus.is_cycle);

        for (const Stop* stop : bus.stops) {
            // добавляю вместо строк числовое представление остановки
            // после десериализации имя восстановлю из мапы transport_catalogue.id_to_stopname()
            pb_bus->add_stop_ids(stop->id);
        }
    }

    // добавим расстояния
    const std::unordered_map<std::pair<const Stop*, const Stop*>, Distance, StopHasher>& curved_distances = t_cat_.GetDistancesList();
    for (const auto& [stop_ptrs, dist] : curved_distances) {
        pb_msg::Distance* pb_dist = transport_catalogue_.add_distance();
        pb_dist->set_from(stop_ptrs.first->id);
        pb_dist->set_to(stop_ptrs.second->id);
        pb_dist->set_curved_distance(dist.curved);
    }
}

void Serializator::SeializeMapRenderer() {
    // добавим настроки рендеринга
    const RenderSettings& settings = m_rend_.GetRenderSettings();
    pb_msg::RenderSettings* pb_render_settings = transport_catalogue_.mutable_render_settings();
    pb_render_settings->set_width(settings.width);
    pb_render_settings->set_height(settings.height);
    pb_render_settings->set_padding(settings.padding);
    pb_render_settings->set_line_width(settings.line_width);
    pb_render_settings->set_stop_radius(settings.stop_radius);
    pb_render_settings->set_bus_label_font_size(settings.bus_label_font_size);
    pb_render_settings->set_bus_label_offset_x(settings.bus_label_offset.first);
    pb_render_settings->set_bus_label_offset_y(settings.bus_label_offset.second);
    pb_render_settings->set_stop_label_font_size(settings.stop_label_font_size);
    pb_render_settings->set_stop_label_offset_x(settings.stop_label_offset.first);
    pb_render_settings->set_stop_label_offset_y(settings.stop_label_offset.second);
    pb_render_settings->set_underlayer_color(settings.underlayer_color);
    pb_render_settings->set_underlayer_width(settings.underlayer_width);
    for (const std::string& s : settings.color_palette) {
        pb_render_settings->add_color_palette(s);
    }

    // добавим настройки проектора
    ProjectorSettings projector_settings = m_rend_.GetProjectorSettings();
    pb_msg::ProjectorSettings* pb_projector_settings = transport_catalogue_.mutable_projector();
    pb_projector_settings->set_min_lon(projector_settings.min_lon);
    pb_projector_settings->set_max_lat(projector_settings.max_lat);
    pb_projector_settings->set_padding(projector_settings.padding);
    pb_projector_settings->set_zoom_coeff(projector_settings.zoom_coeff);
}

void Serializator::SeializeTransportRouter() {
    pb_msg::TransportRouter* transport_router = transport_catalogue_.mutable_transport_router();

    // добавим настройки маршрутизатора
    pb_msg::RoutingSettings* routing_settings = transport_router->mutable_settings();
    routing_settings->set_velocity_mph(t_route_->GetVelocity());
    routing_settings->set_wait_time_min(t_route_->GetWaitTime());

    // добавим граф
    pb_msg::Graph* pb_graph = transport_router->mutable_graph();

    const graph::DirectedWeightedGraph<double>& graph = t_route_->GetGraph();

    for (size_t edge_id = 0; edge_id < graph.GetEdgeCount(); ++edge_id) {
        const graph::Edge<double>& edge = graph.GetEdge(edge_id);
        pb_msg::Edge* pb_edge = pb_graph->add_edge();

        pb_edge->set_from(edge.from);
        pb_edge->set_to(edge.to);
        pb_edge->set_weight(edge.weight);

        // в графе есть ребра как с именем автобуса, там и без этого имени (ребра ожидания),
        // и если искать имя автобуса, состоящее из пустой строки, то будет падение;
        // установим ребрам ожидания id = 0, а при десериализации по этому значению их различим
        // и не будем стараться искать имя по id = 0, т.к его ведь не сущетсвует у ребер ожидания
        if (edge.busname.size() > 0) {
            pb_edge->set_busname(t_cat_.GetAllBuses().at(edge.busname)->id);
        } else {
            pb_edge->set_busname(0);
        }
        pb_edge->set_span(edge.span);
        pb_edge->set_stop_name_from(t_cat_.GetAllStops().at(edge.stop_name_from)->id);
    }

    for (size_t vertex_id = 0; vertex_id < graph.GetVertexCount(); ++vertex_id) {
        const auto& incident_edges = graph.GetIncidentEdges(vertex_id);
        pb_msg::IncidenceEdgeIds* edge_ids_list = pb_graph->add_incidence_edges();

        for (auto it = incident_edges.begin(); it != incident_edges.end(); ++it) {
            graph::EdgeId edge_id = *it;
            edge_ids_list->add_edge_id(edge_id);
        }
    }
}

void Serializator::DeseializeTransportCatalogue() {
    for (const pb_msg::Stop& pb_stop : transport_catalogue_.stop()) {
        Stop tc_stop;
        size_t pb_id = pb_stop.unique_stop_id();

        tc_stop.stopname = transport_catalogue_.id_to_stopname().at(pb_id);
        tc_stop.location.lat = pb_stop.location().lat();
        tc_stop.location.lng = pb_stop.location().lng();

        t_cat_.AddStop(std::move(tc_stop));
    }

    for (const pb_msg::Bus& pb_bus : transport_catalogue_.bus()) {
        RawBus raw_bus;
        raw_bus.busname = transport_catalogue_.id_to_busname().at(pb_bus.unique_bus_id());
        raw_bus.is_cycle = pb_bus.is_cycle();
        for (size_t id : pb_bus.stop_ids()) {
            std::string pb_stopname = transport_catalogue_.id_to_stopname().at(id);
            raw_bus.stops.push_back(std::move(pb_stopname));
        }

        t_cat_.AddBus(std::move(raw_bus));
    }

    for (const pb_msg::Distance& dist : transport_catalogue_.distance()) {
        const std::string& from = transport_catalogue_.id_to_stopname().at(dist.from());
        const std::string& to = transport_catalogue_.id_to_stopname().at(dist.to());
        int curved_distance = static_cast<int>(dist.curved_distance());
        t_cat_.SetCurvedDistance({from, to, curved_distance});
    }
}

void Serializator::DeseializeMapRenderer() {
    const pb_msg::RenderSettings& pb_render_settings = transport_catalogue_.render_settings();
    RenderSettings render_settings;
    render_settings.width = pb_render_settings.width();
    render_settings.height = pb_render_settings.height();
    render_settings.padding = pb_render_settings.padding();
    render_settings.bus_label_font_size = pb_render_settings.bus_label_font_size();
    render_settings.bus_label_offset.first = pb_render_settings.bus_label_offset_x();
    render_settings.bus_label_offset.second = pb_render_settings.bus_label_offset_y();
    render_settings.stop_label_font_size = pb_render_settings.stop_label_font_size();
    render_settings.stop_label_offset.first = pb_render_settings.stop_label_offset_x();
    render_settings.stop_label_offset.second = pb_render_settings.stop_label_offset_y();
    render_settings.stop_radius = pb_render_settings.stop_radius();
    render_settings.line_width = pb_render_settings.line_width();
    render_settings.underlayer_color = pb_render_settings.underlayer_color();
    render_settings.underlayer_width = pb_render_settings.underlayer_width();
    for (const std::string color : pb_render_settings.color_palette()) {
        render_settings.color_palette.push_back(color);
    }

    const pb_msg::ProjectorSettings& pb_projector_settings = transport_catalogue_.projector();
    ProjectorSettings projector_settings;
    projector_settings.min_lon = pb_projector_settings.min_lon();
    projector_settings.max_lat = pb_projector_settings.max_lat();
    projector_settings.padding = pb_projector_settings.padding();
    projector_settings.zoom_coeff = pb_projector_settings.zoom_coeff();

    m_rend_.ConfigureMapRenderer(render_settings, projector_settings);
}

void Serializator::DeseializeTransportRouter() {
    const pb_msg::TransportRouter& pb_transport_router = transport_catalogue_.transport_router();
    RoutingSettings routing_settings;
    routing_settings.bus_velocity = pb_transport_router.settings().velocity_mph();
    routing_settings.bus_wait_time = pb_transport_router.settings().wait_time_min();

    const pb_msg::Graph& pb_graph = pb_transport_router.graph();
    std::vector<graph::Edge<double>> edges;
    edges.resize(pb_graph.edge_size());

    for (size_t i = 0; i < pb_graph.edge_size(); ++i) {
        const auto& pb_edge = pb_graph.edge()[i];
        edges[i].from = pb_edge.from();
        edges[i].to = pb_edge.to();
        edges[i].weight = pb_edge.weight();
        if (pb_edge.busname() > 0) {
            std::string pb_busname = transport_catalogue_.id_to_busname().at(pb_edge.busname());

            // так как graph::Edge хранит вьюху на куда-то, то просто написать pb_busname справа от =
            // нельзя, так как устанавливаемая вьюха будет смотреть на строку, которая вот-вот уничтожится
            // а вот так мы вьюхой начинаем смотреть на строку в дэке -- централизованном месте для строк
            edges[i].busname = t_cat_.GetAllBuses().at(pb_busname)->busname;
        }
        edges[i].span = pb_edge.span();

        std::string pb_stopname = transport_catalogue_.id_to_stopname().at(pb_edge.stop_name_from());
        edges[i].stop_name_from = t_cat_.GetAllStops().at(pb_stopname)->stopname;
    }
    
    std::vector<std::vector<size_t>> incidence_list;
    incidence_list.resize(pb_graph.incidence_edges_size());

    for (size_t i = 0; i < pb_graph.incidence_edges_size(); ++i) {
        const auto& pb_vertex = pb_graph.incidence_edges()[i];

        for (size_t edge_id : pb_vertex.edge_id()) {
            incidence_list[i].push_back(edge_id);
        }
    }
        
    graph::DirectedWeightedGraph<double> graph(std::move(edges), std::move(incidence_list));

    t_route_ = std::make_unique<transport_router::TransportRouter<double>>(t_cat_, std::move(graph), routing_settings);
}

void Serializator::Serialize(std::ostream& output) {
    SeializeTransportCatalogue();
    SeializeMapRenderer();
    SeializeTransportRouter();

    transport_catalogue_.SerializeToOstream(&output);
}

void Serializator::Deserialize(std::istream& input) {
    transport_catalogue_.ParseFromIstream(&input);

    DeseializeTransportCatalogue();
    DeseializeMapRenderer();
    DeseializeTransportRouter();
}

} // namespace serialization