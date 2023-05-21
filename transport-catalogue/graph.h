#pragma once

#include "ranges.h"
#include "domain.h"

#include <cstdlib>
#include <vector>
#include <cassert>

static const double METERS_PER_KILOMETER = 1000;
static const double MINUTES_PER_HOUR = 60;

namespace graph {
using VertexId = size_t;
using EdgeId = size_t;

template <typename Weight>
struct Edge {
    VertexId from = 0;
    VertexId to = 0;
    Weight weight = 0;
    BusName busname = ""sv;
    size_t span = 0;
    StopName stop_name_from = ""sv;
};

template <typename Weight>
class DirectedWeightedGraph {
private:
    using IncidenceList = std::vector<EdgeId>;
    using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;

public:
    DirectedWeightedGraph() = default;
    explicit DirectedWeightedGraph(size_t vertex_count);
    EdgeId AddEdge(const Edge<Weight>& edge);

    size_t GetVertexCount() const;
    size_t GetEdgeCount() const;
    const Edge<Weight>& GetEdge(EdgeId edge_id) const;
    IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

private:
    std::vector<Edge<Weight>> edges_;
    std::vector<IncidenceList> incidence_lists_;
};

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
    : incidence_lists_(vertex_count) {
}

template <typename Weight>
EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight>& edge) {
    edges_.push_back(edge);
    const EdgeId id = edges_.size() - 1;
    incidence_lists_.at(edge.from).push_back(id);
    return id;
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
    return incidence_lists_.size();
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
    return edges_.size();
}

template <typename Weight>
const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
    return edges_.at(edge_id);
}

template <typename Weight>
typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
    return ranges::AsRange(incidence_lists_.at(vertex));
}

template <typename Weight>
class GraphBuilder {
    using StopNameToCoordinates = std::map<StopName, geo::Coordinates>;
    using BusNameToBusPtr = std::map<BusName, Bus*>;
    using PairStopPtrToDist = std::unordered_map<std::pair<const Stop*, const Stop*>, Distance, StopHasher>;

    using VertexIdsToEdgeId = std::unordered_map<std::pair<VertexId, VertexId>, EdgeId, PairSizetHasher>;
public:
    GraphBuilder(const StopNameToCoordinates& stops, const BusNameToBusPtr& buses, const PairStopPtrToDist& distances_list, double velocity, double wait_time_minutes)
    : stops_(stops)
    , buses_(buses)
    , distances_list_(distances_list)
    , velocity_(velocity * METERS_PER_KILOMETER / MINUTES_PER_HOUR)
    , wait_time_minutes_(wait_time_minutes)
    , graph_(std::make_unique<DirectedWeightedGraph<Weight>>(stops.size() * 2)) {
        // в решении остановке будет поставлено в соответствие две вершины
        // например, Universam : [0, 1]; Prashskaya : [2, 3] и тд
        for (const auto& [stopname, stop_ptr] : stops_) {
            std::pair<VertexId, VertexId> stop_ids = {stop_ptr.id * 2, stop_ptr.id * 2 + 1};
            GenerateWaitingEdge(stopname, stop_ids);
        }

        // для каждого маршрута генерируются все ребра на нем
        for (const auto& [busname, bus_ptr] : buses_) {
            GenerateEdgesOnBus(busname, bus_ptr);
        }
    }

    std::unique_ptr<graph::DirectedWeightedGraph<Weight>> Build() {
        return std::move(graph_);
    }

private:
    const StopNameToCoordinates& stops_;
    const BusNameToBusPtr& buses_;
    const PairStopPtrToDist& distances_list_;
    double velocity_;
    double wait_time_minutes_;

    std::unique_ptr<graph::DirectedWeightedGraph<Weight>> graph_;

    void GenerateWaitingEdge(std::string_view stopname, std::pair<VertexId, VertexId> stop_ids) {
        Edge<Weight> waiting_edge;

        waiting_edge.from = stop_ids.first;
        waiting_edge.to = stop_ids.second;
        waiting_edge.span = 0;
        waiting_edge.weight = wait_time_minutes_;
        waiting_edge.stop_name_from = stopname;

        graph_->AddEdge(waiting_edge);
    }

    void GenerateEdgesOnBus(std::string_view busname, const Bus* bus) {
        VertexIdsToEdgeId adjacent_edge_ids; // в эту хэш-мапу будут положены id смежных ребер на маршруте

        const std::vector<Stop*>& stops = bus->stops;

        // в цикле генерируются смежные ребра из входных данных
        for (size_t i = 0; i < stops.size() - 1; ++i) {

            double forward_distance = 0;
            if (distances_list_.count({stops[i], stops[i + 1]}) == 1) {
                forward_distance = distances_list_.at({stops[i], stops[i + 1]}).curved;
            } else {
                forward_distance = distances_list_.at({stops[i + 1], stops[i]}).curved;
            }

            Edge<Weight> forward_edge;
            forward_edge.from = stops[i]->id * 2 + 1;
            forward_edge.to = stops[i + 1]->id * 2;
            forward_edge.weight = forward_distance / velocity_; // ребра, на основе которых идут дальнейшие расчеты, получают вес в минутах перемещения
            forward_edge.busname = busname;
            forward_edge.span = 1;
            forward_edge.stop_name_from = stops[i]->stopname;
            
            adjacent_edge_ids[{forward_edge.from, forward_edge.to}] = graph_->AddEdge(forward_edge);

            if (!bus->is_cycle) {

                double backward_distance = 0;
                if (distances_list_.count({stops[i + 1], stops[i]}) == 1) {
                    backward_distance = distances_list_.at({stops[i + 1], stops[i]}).curved;
                } else {
                    backward_distance = forward_distance;
                }

                Edge<Weight> backward_edge;
                backward_edge.from = stops[i + 1]->id * 2 + 1;
                backward_edge.to = stops[i]->id * 2;
                backward_edge.weight = backward_distance / velocity_; // ребра на основе которых идут дальнейшие расчеты получают вес в минутах перемещения
                backward_edge.busname = busname;
                backward_edge.span = 1;
                backward_edge.stop_name_from = stops[i + 1]->stopname;
                
                adjacent_edge_ids[{backward_edge.from, backward_edge.to}] = graph_->AddEdge(backward_edge);
            }
        }

        // на основе смежных ребер маршрута генерируются ребра-хорды этого маршрута
        GenerateChordEdgesOnBus(adjacent_edge_ids, busname, bus);
    }
    
    void GenerateChordEdgesOnBus(const VertexIdsToEdgeId& adjacent_edge_ids, std::string_view busname, const Bus* bus) {
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
                const Edge<Weight>& from_between = is_first ? graph_->GetEdge(adjacent_edge_ids.at({stop_from.second, stop_between.first}))
                                                            : graph_->GetEdge(chord_edge_ids.at({stop_from.second, stop_between.first}));
                // вторую половину хорды всегда беру из смежных ребер
                const Edge<Weight>& between_to = graph_->GetEdge(adjacent_edge_ids.at({stop_between.second, stop_to.first}));
                
                Edge<Weight> forward_edge;
                forward_edge.from = stops[i]->id * 2 + 1;
                forward_edge.to = stops[j]->id * 2;
                forward_edge.weight = from_between.weight + between_to.weight; // ребра, на основе которых идут дальнейшие расчеты, получают вес в минутах перемещения
                forward_edge.busname = busname;
                forward_edge.span = from_between.span + between_to.span;
                forward_edge.stop_name_from = stops[i]->stopname;

                chord_edge_ids[{forward_edge.from, forward_edge.to}] = graph_->AddEdge(forward_edge);

                if (!bus->is_cycle) {
                    // первую половину хорды на обратном пути всегда беру из смежных
                    const Edge<Weight>& to_between = graph_->GetEdge(adjacent_edge_ids.at({stop_to.second, stop_between.first}));
                    // вторую половину хорды на обратном пути всегда беру из хордовых (кроме расчета первой хорды в текущем цикле)
                    const Edge<Weight>& between_from = is_first ? graph_->GetEdge(adjacent_edge_ids.at({stop_between.second, stop_from.first}))
                                                                : graph_->GetEdge(chord_edge_ids.at({stop_between.second, stop_from.first}));

                    Edge<Weight> backward_edge;
                    backward_edge.from = stops[j]->id * 2 + 1;
                    backward_edge.to = stops[i]->id * 2;
                    backward_edge.weight = to_between.weight + between_from.weight; // ребра на основе которых идут дальнейшие расчеты получают вес в минутах перемещения
                    backward_edge.busname = busname;
                    backward_edge.span = to_between.span + between_from.span;
                    backward_edge.stop_name_from = stops[j]->stopname;
                    
                    chord_edge_ids[{backward_edge.from, backward_edge.to}] = graph_->AddEdge(backward_edge);
                }

                is_first = false;
            }
        }
    }
};

}  // namespace graph