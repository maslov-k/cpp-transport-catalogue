#include "transport_catalogue.h"
#include "geo.h"
#include <algorithm>

using namespace std;

namespace transport
{
void TransportCatalogue::AddBus(const string& name, vector<const Stop*> stops, const unordered_set<string_view>& unique_stops)
{
	buses_.push_back({ name, move(stops), unique_stops});
	Bus* bus = &buses_.back();
	for (string_view stop_name : bus->unique_stops)
	{
		stop_to_buses_[stop_name].insert(bus->name);
	}
	name_to_bus_[bus->name] = bus;

	int n_stops = bus->stops.size();
	int n_unique_stops = bus->unique_stops.size();
	double geo_length = 0;
	double real_length = 0;
	for (auto it = bus->stops.begin(); it != prev(bus->stops.end()); ++it)
	{
		const Stop* stop_a = *it;
		const Stop* stop_b = *next(it);
		geo_length += geo::ComputeDistance(stop_a->coordinates, stop_b->coordinates);
		real_length += GetDistanceBetweenStops(stop_a, stop_b);
	}
	double curvature = real_length / geo_length;
	routes_info_[bus] = { n_stops, n_unique_stops, real_length, curvature };
}

void TransportCatalogue::AddStop(const string& name, geo::Coordinates coordinates)
{
	stops_.push_back({ name, move(coordinates) });
	Stop* stop = &stops_.back();
	name_to_stop_[stop->name] = stop;
}

const Bus* TransportCatalogue::SearchBus(string_view bus_name) const
{
	auto search = name_to_bus_.find(bus_name);
	if (search == name_to_bus_.end())
	{
		return nullptr;
	}
	return search->second;
}

const Stop* TransportCatalogue::SearchStop(string_view stop_name) const
{
	auto search = name_to_stop_.find(stop_name);
	if (search == name_to_stop_.end())
	{
		return nullptr;
	}
	return search->second;
}

RouteInfo TransportCatalogue::GetRouteInfo(const Bus* bus) const
{
	return routes_info_.at(bus);
}

const sv_set& TransportCatalogue::GetStopToBuses(string_view stop_name)
{
	return stop_to_buses_[stop_name];
}

void TransportCatalogue::SetDistanceBetweenStops(const Stop* stop_a, const Stop* stop_b, int distance)
{
	distances_between_stops_[{stop_a, stop_b}] = distance;
}

int TransportCatalogue::GetDistanceBetweenStops(const Stop* stop_a, const Stop* stop_b) const
{
	if (distances_between_stops_.count({ stop_a, stop_b }))
	{
		return distances_between_stops_.at({ stop_a, stop_b });
	}
	else if (distances_between_stops_.count({ stop_b, stop_a }))
	{
		return distances_between_stops_.at({ stop_b, stop_a });
	}
	return 0;
}

size_t TransportCatalogue::StopsHasher::operator()(const pair<const Stop*, const Stop*>& stops) const
{
	return pointer_hasher(stops.first) + pointer_hasher(stops.second) * 37;
}
size_t TransportCatalogue::BusHasher::operator()(const Bus* bus) const
{
	return pointer_hasher(bus);
}
} //transport
