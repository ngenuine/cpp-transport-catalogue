#include "router.h"
#include "graph.h"
#include "transport_catalogue.h"
#include <memory>

static const double METERS_PER_KILOMETER = 1000;
static const double MINUTES_PER_HOUR = 60;

namespace transport_router {

using graph::VertexId;
using graph::EdgeId;

template<typename Weight>
class TransportRouter {
    using VertexIdsToEdgeId = std::unordered_map<std::pair<VertexId, VertexId>, EdgeId, PairSizetHasher>;
public:
    TransportRouter(const transport::TransportCatalogue& transport_catalogue, RoutingSettings settings);
    std::optional<typename graph::Router<Weight>::RouteInfo> BuildRouteBetweenStops(std::string_view from, std::string_view to) const;
    const graph::Edge<Weight>& GetEdge(EdgeId edge_id) const;

private:
    const transport::TransportCatalogue& transport_catalogue_;
    graph::DirectedWeightedGraph<Weight> graph_;
    const double velocity_mph_;
    const double wait_time_min_;
    std::unique_ptr<graph::Router<Weight>> router_;

    void GenerateWaitingEdge(std::string_view stopname, std::pair<VertexId, VertexId> stop_ids);
    void GenerateEdgesOnBus(std::string_view busname, const Bus* bus);
    void GenerateChordEdgesOnBus(const VertexIdsToEdgeId& adjacent_edge_ids, std::string_view busname, const Bus* bus);
};

template<typename Weight>
TransportRouter<Weight>::TransportRouter(const transport::TransportCatalogue& transport_catalogue, RoutingSettings settings)
    : transport_catalogue_(transport_catalogue)
    , graph_(transport_catalogue.GetUsefulStopCoordinates().size() * 2)
    , velocity_mph_(settings.bus_velocity * METERS_PER_KILOMETER / MINUTES_PER_HOUR)
    , wait_time_min_(settings.bus_wait_time)
{
    // в решении остановке будет поставлено в соответствие две вершины
    // например, Universam : [0, 1]; Prashskaya : [2, 3] и тд
    for (const auto& [stopname, stop_ptr] : transport_catalogue_.GetUsefulStopCoordinates()) {
        std::pair<VertexId, VertexId> stop_ids = {stop_ptr.id * 2, stop_ptr.id * 2 + 1};
        GenerateWaitingEdge(stopname, stop_ids);
    }

    // для каждого маршрута генерируются все ребра на нем
    for (const auto& [busname, bus_ptr] : transport_catalogue.GetAllBuses()) {
        GenerateEdgesOnBus(busname, bus_ptr);
    }

    router_= std::make_unique<graph::Router<Weight>>(graph_);
}

template<typename Weight>
std::optional<typename graph::Router<Weight>::RouteInfo> TransportRouter<Weight>::BuildRouteBetweenStops(std::string_view from, std::string_view to) const{
    std::optional<VertexId> from_vertex = transport_catalogue_.GetUsefulStopId(from);
    std::optional<VertexId> to_vertex = transport_catalogue_.GetUsefulStopId(to);

    if (!from_vertex || !to_vertex) {
        return std::nullopt;
    }

    return router_->BuildRoute(from_vertex.value() * 2, to_vertex.value() * 2);
}

template<typename Weight>
const graph::Edge<Weight>& TransportRouter<Weight>::GetEdge(EdgeId edge_id) const {
    return graph_.GetEdge(edge_id);
}

template<typename Weight>
void TransportRouter<Weight>::GenerateWaitingEdge(std::string_view stopname, std::pair<VertexId, VertexId> stop_ids) {
    graph::Edge<Weight> waiting_edge;

    waiting_edge.from = stop_ids.first;
    waiting_edge.to = stop_ids.second;
    waiting_edge.span = 0;
    waiting_edge.weight = wait_time_min_;
    waiting_edge.stop_name_from = stopname;

    graph_.AddEdge(waiting_edge);
}

template<typename Weight>
void TransportRouter<Weight>::GenerateEdgesOnBus(std::string_view busname, const Bus* bus) {
    VertexIdsToEdgeId adjacent_edge_ids; // в эту хэш-мапу будут положены id смежных ребер на маршруте

    const std::vector<Stop*>& stops = bus->stops;
    const auto& distances = transport_catalogue_.GetDistancesList();

    // в цикле генерируются смежные ребра из входных данных
    for (size_t i = 0; i < stops.size() - 1; ++i) {

        double forward_distance = 0;
        if (distances.count({stops[i], stops[i + 1]}) == 1) {
            forward_distance = distances.at({stops[i], stops[i + 1]}).curved;
        } else {
            forward_distance = distances.at({stops[i + 1], stops[i]}).curved;
        }

        graph::Edge<Weight> forward_edge;
        forward_edge.from = stops[i]->id * 2 + 1;
        forward_edge.to = stops[i + 1]->id * 2;
        forward_edge.weight = forward_distance / velocity_mph_; // ребра, на основе которых идут дальнейшие расчеты, получают вес в минутах перемещения
        forward_edge.busname = busname;
        forward_edge.span = 1;
        forward_edge.stop_name_from = stops[i]->stopname;
        
        adjacent_edge_ids[{forward_edge.from, forward_edge.to}] = graph_.AddEdge(forward_edge);

        if (!bus->is_cycle) {

            double backward_distance = 0;
            if (distances.count({stops[i + 1], stops[i]}) == 1) {
                backward_distance = distances.at({stops[i + 1], stops[i]}).curved;
            } else {
                backward_distance = forward_distance;
            }

            graph::Edge<Weight> backward_edge;
            backward_edge.from = stops[i + 1]->id * 2 + 1;
            backward_edge.to = stops[i]->id * 2;
            backward_edge.weight = backward_distance / velocity_mph_; // ребра на основе которых идут дальнейшие расчеты получают вес в минутах перемещения
            backward_edge.busname = busname;
            backward_edge.span = 1;
            backward_edge.stop_name_from = stops[i + 1]->stopname;
            
            adjacent_edge_ids[{backward_edge.from, backward_edge.to}] = graph_.AddEdge(backward_edge);
        }
    }

    // на основе смежных ребер маршрута генерируются ребра-хорды этого маршрута
    GenerateChordEdgesOnBus(adjacent_edge_ids, busname, bus);
}

template<typename Weight>
void TransportRouter<Weight>::GenerateChordEdgesOnBus(const VertexIdsToEdgeId& adjacent_edge_ids, std::string_view busname, const Bus* bus) {
    const std::vector<Stop*>& stops = bus->stops;

    // ребра-хорды генерируются из конечной остановки во все остальные, а из всех остальных
    // только в конечную; так будет сделана пересадка, если при передвижении по маршруту
    // мы двигаемся сквозь конечную остановку (по условию задачи надо выйти и подождать)

    for (size_t i = 0; i < stops.size() - 1; ++i) {

        // на каждом цикле генерируются свои ребра-хорды; эта хэш-мапа, нужна, чтобы
        // запоминать, какие хорды у меня есть, чтобы использовать их в расчетах других хорд
        VertexIdsToEdgeId chord_edge_ids;
        bool is_first = true;

        for (size_t j = i + 1; j < stops.size(); ++j) {
            if (j - i <= 1) { // это смежные ребра, они уже добавлены
                continue;
            }

            std::pair<VertexId, VertexId> stop_from = {stops[i]->id * 2, stops[i]->id * 2 + 1};
            std::pair<VertexId, VertexId> stop_to = {stops[j]->id * 2, stops[j]->id * 2 + 1};
            std::pair<VertexId, VertexId> stop_between = {stops[j - 1]->id * 2, stops[j - 1]->id * 2 + 1};

            // первую половину хорды всегда беру из хордовых (кроме расчета первой хорды в текущем цикле)
            const graph::Edge<Weight>& from_between = is_first ? graph_.GetEdge(adjacent_edge_ids.at({stop_from.second, stop_between.first}))
                                                                : graph_.GetEdge(chord_edge_ids.at({stop_from.second, stop_between.first}));
            // вторую половину хорды всегда беру из смежных ребер
            const graph::Edge<Weight>& between_to = graph_.GetEdge(adjacent_edge_ids.at({stop_between.second, stop_to.first}));
            
            graph::Edge<Weight> forward_edge;
            forward_edge.from = stops[i]->id * 2 + 1;
            forward_edge.to = stops[j]->id * 2;
            forward_edge.weight = from_between.weight + between_to.weight; // ребра, на основе которых идут дальнейшие расчеты, получают вес в минутах перемещения
            forward_edge.busname = busname;
            forward_edge.span = from_between.span + between_to.span;
            forward_edge.stop_name_from = stops[i]->stopname;

            chord_edge_ids[{forward_edge.from, forward_edge.to}] = graph_.AddEdge(forward_edge);

            if (!bus->is_cycle) {
                // первую половину хорды на обратном пути всегда беру из смежных
                const graph::Edge<Weight>& to_between = graph_.GetEdge(adjacent_edge_ids.at({stop_to.second, stop_between.first}));
                // вторую половину хорды на обратном пути всегда беру из хордовых (кроме расчета первой хорды в текущем цикле)
                const graph::Edge<Weight>& between_from = is_first ? graph_.GetEdge(adjacent_edge_ids.at({stop_between.second, stop_from.first}))
                                                                    : graph_.GetEdge(chord_edge_ids.at({stop_between.second, stop_from.first}));

                graph::Edge<Weight> backward_edge;
                backward_edge.from = stops[j]->id * 2 + 1;
                backward_edge.to = stops[i]->id * 2;
                backward_edge.weight = to_between.weight + between_from.weight; // ребра на основе которых идут дальнейшие расчеты получают вес в минутах перемещения
                backward_edge.busname = busname;
                backward_edge.span = to_between.span + between_from.span;
                backward_edge.stop_name_from = stops[j]->stopname;
                
                chord_edge_ids[{backward_edge.from, backward_edge.to}] = graph_.AddEdge(backward_edge);
            }

            is_first = false;
        }
    }
}

} // namespace transport_router
