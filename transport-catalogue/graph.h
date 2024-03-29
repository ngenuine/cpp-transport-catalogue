#pragma once

#include "ranges.h"
#include "domain.h"

#include <cstdlib>
#include <vector>

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
    explicit DirectedWeightedGraph(std::vector<Edge<Weight>>&& edges, std::vector<IncidenceList>&& incidence_lists);
    EdgeId AddEdge(const Edge<Weight>& edge);

    size_t GetVertexCount() const;
    size_t GetEdgeCount() const;
    const Edge<Weight>& GetEdge(EdgeId edge_id) const;
    IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;
    const std::vector<IncidenceList>& GetVectorIncidentEdges() const;

private:
    std::vector<Edge<Weight>> edges_;
    std::vector<IncidenceList> incidence_lists_;
};

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
    : incidence_lists_(vertex_count) {
}

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(std::vector<Edge<Weight>>&& edges, std::vector<IncidenceList>&& incidence_lists)
    : edges_(std::move(edges))
    , incidence_lists_(std::move(incidence_lists))
{
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
const std::vector<std::vector<size_t>>& DirectedWeightedGraph<Weight>::GetVectorIncidentEdges() const {
    return incidence_lists_;
}

}  // namespace graph