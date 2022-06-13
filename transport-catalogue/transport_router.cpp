#include "transport_router.h"

#include <iostream>

using namespace router;
using namespace graph;
using namespace transport;
using namespace std;

TransportRouter::TransportRouter(const transport::TransportCatalogue &tc)
    : tc_(&tc)
{
}

TransportRouter::TransportRouter(const TransportCatalogue& tc, const RoutingSettings& settings)
    : tc_(&tc), tc_graph_(tc_->GetValidStops().size() * 2), settings_(settings)
{
	int stop_index = 0;
    for (string_view stop_name : tc_->GetValidStops())
	{
		stops_indexes_[stop_name] = stop_index++;
    }
}

void TransportRouter::BuildGraph()
{

	for (auto [stop_name, stop_index] : stops_indexes_)
	{
		VertexId vertex_1 = stop_index * 2;
		VertexId vertex_2 = stop_index * 2 + 1;
		EdgeId edge_id = tc_graph_.AddEdge({ vertex_1, vertex_2, static_cast<double>(settings_.bus_wait_time) });
		edges_info_[edge_id] = { EdgeType::WAIT,
								 stop_name,
								 static_cast<double>(settings_.bus_wait_time),
								 string_view() };
	}
    double kmh_to_m_per_min = 3.6 / 60;

    for (string_view bus_name : tc_->GetValidBuses())
	{
        const domain::Bus* bus = tc_->SearchBus(bus_name);
		const vector<const domain::Stop*>& stops = bus->stops;
		for (auto stop_from_it = stops.begin(); stop_from_it != prev(stops.end()); ++stop_from_it)
		{
			for (auto stop_to_it = next(stop_from_it); stop_to_it != stops.end(); ++stop_to_it)
			{
				string_view stop_from_name = (*stop_from_it)->name;
				string_view stop_to_name = (*stop_to_it)->name;
				VertexId stop_from_vertex_2 = stops_indexes_[stop_from_name] * 2 + 1;
				VertexId stop_to_vertex_1 = stops_indexes_[stop_to_name] * 2;
				double time_between_stops = GetDistanceBetweenStops(stops, distance(stops.begin(), stop_from_it),
																		   distance(stops.begin(), stop_to_it))
                                                                    / settings_.bus_velocity * kmh_to_m_per_min;
				EdgeId edge_id = tc_graph_.AddEdge({ stop_from_vertex_2, stop_to_vertex_1, time_between_stops });
				edges_info_[edge_id] = { EdgeType::BUS,
										 stop_to_name,
										 time_between_stops,
										 bus->name,
										 static_cast<int>(distance(stop_from_it, stop_to_it)) };
			}
		}
    }
}

const graph::DirectedWeightedGraph<double>& TransportRouter::GetGraph() const
{
    return tc_graph_;
}

const RoutingSettings &TransportRouter::GetRoutingSettings() const
{
    return settings_;
}

const EdgeInfo& router::TransportRouter::GetEdgeInfo(EdgeId edge_id) const
{
    return edges_info_.at(edge_id);
}

void TransportRouter::SetGraph(graph::DirectedWeightedGraph<double> graph)
{
    tc_graph_ = move(graph);
}

void TransportRouter::SetRoutingSettings(RoutingSettings routing_settings)
{
    settings_ = move(settings_);
}

void TransportRouter::AddStopIndex(std::string_view name, int index)
{
    stops_indexes_[name] = index;
}

void TransportRouter::AddEdgeInfo(graph::EdgeId edge_id, EdgeInfo edge_info)
{
    edges_info_[edge_id] = move(edge_info);
}

optional<RouteData> TransportRouter::GetRouteData(string_view stop_from_name, string_view stop_to_name) const
{
	static const Router router(tc_graph_);
	optional<VertexId> vertex_1 = GetVertexId(stop_from_name);
	optional<VertexId> vertex_2 = GetVertexId(stop_to_name);
	if (!vertex_1 || !vertex_2)
	{
		return nullopt;
	}
	optional<Router<double>::RouteInfo> route_info = router.BuildRoute(*vertex_1, *vertex_2);
	if (!route_info)
	{
		return nullopt;
	}
	RouteData route_data;
	route_data.time = route_info->weight;
	for (auto it = route_info->edges.begin(); it != route_info->edges.end(); ++it)
	{
		EdgeInfo edge_info = GetEdgeInfo(*it);
		route_data.edges_info.push_back(edge_info);
	}
	return route_data;
}

double TransportRouter::GetDistanceBetweenStops(const vector<const domain::Stop*>& stops, int stop_from_index, int stop_to_index)
{
	double distance = 0;
	for (auto it = next(stops.begin(), stop_from_index); it != next(stops.begin(), stop_to_index); ++it)
	{
        distance += tc_->GetDistanceBetweenStops(*it, *next(it));
	}
	return distance;
}

optional<VertexId> TransportRouter::GetVertexId(string_view stop_name) const
{
	if (stops_indexes_.count(stop_name))
	{
		return stops_indexes_.at(stop_name) * 2;
	}
	return nullopt;
}
