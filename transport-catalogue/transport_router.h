#pragma once

#include "router.h"
#include "transport_catalogue.h"
#include "domain.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <map>
#include <vector>

namespace router
{

struct RoutingSettings
{
	int bus_wait_time;
	double bus_velocity;
};

enum class EdgeType
{
	WAIT,
	BUS
};

struct EdgeInfo
{
	EdgeType type{};
	std::string_view stop_name;
	double time = 0;
	std::string_view bus_name;
	int span_count = 0;
};

struct RouteData
{
	double time = 0;
	std::vector<EdgeInfo> edges_info;
};

class TransportRouter
{
public:
    explicit TransportRouter(const transport::TransportCatalogue& tc);
    explicit TransportRouter(const transport::TransportCatalogue& tc, const RoutingSettings& settings);

    void BuildGraph();
    const graph::DirectedWeightedGraph<double>& GetGraph() const;
    const RoutingSettings& GetRoutingSettings() const;
    const EdgeInfo& GetEdgeInfo(graph::EdgeId edge_id) const;

    void SetGraph(graph::DirectedWeightedGraph<double> graph);
    void SetRoutingSettings(RoutingSettings routing_settings);
    void AddStopIndex(std::string_view name, int index);
    void AddEdgeInfo(graph::EdgeId edge_id, EdgeInfo edge_info);

	std::optional<RouteData> GetRouteData(std::string_view stop_from_name, std::string_view stop_to_name) const;

private:
	double GetDistanceBetweenStops(const std::vector<const transport::domain::Stop*>& stops, int stop_from_index, int stop_to_index);

	std::optional<graph::VertexId> GetVertexId(std::string_view stop_name) const;

    const transport::TransportCatalogue*		tc_;
	graph::DirectedWeightedGraph<double>		tc_graph_;
    RoutingSettings								settings_;
	std::unordered_map<std::string_view, int>	stops_indexes_;
	std::unordered_map<graph::EdgeId, EdgeInfo>	edges_info_;
};

} // namespace router
