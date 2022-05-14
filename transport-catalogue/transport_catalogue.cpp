#include "transport_catalogue.h"

#include <algorithm>

using namespace std;
using namespace transport;
using namespace domain;
using namespace geo;

void TransportCatalogue::AddBus(const string& name, vector<const Stop*> stops, const unordered_set<string_view>& unique_stops, bool is_round)
{
	buses_.push_back({ name, move(stops), unique_stops, is_round});
	Bus* bus = &buses_.back();
	for (string_view stop_name : bus->unique_stops)
	{
		stop_to_buses_[SearchStop(stop_name)].insert(bus->name);
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
		geo_length += ComputeDistance(stop_a->coordinates, stop_b->coordinates);
		real_length += GetDistanceBetweenStops(stop_a, stop_b);
	}
	double curvature = real_length / geo_length;
	buses_info_[bus] = { n_stops, n_unique_stops, real_length, curvature };
}

void TransportCatalogue::AddStop(const string& name, Coordinates coordinates)
{
	stops_.push_back({ name, coordinates });
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

BusInfo TransportCatalogue::GetBusInfo(const Bus* bus) const
{
	return buses_info_.at(bus);
}

const sv_set* TransportCatalogue::GetStopToBuses(const Stop* stop) const
{
	if (!stop_to_buses_.count(stop))
	{
		return nullptr;
	}
	return &stop_to_buses_.at(stop);
}

void TransportCatalogue::SetDistanceBetweenStops(const Stop* stop_a, const Stop* stop_b, int distance)
{
	distances_btw_stops_[{stop_a, stop_b}] = distance;
}

int TransportCatalogue::GetDistanceBetweenStops(const Stop* stop_a, const Stop* stop_b) const
{
	if (distances_btw_stops_.count({ stop_a, stop_b }))
	{
		return distances_btw_stops_.at({ stop_a, stop_b });
	}
	else if (distances_btw_stops_.count({ stop_b, stop_a }))
	{
		return distances_btw_stops_.at({ stop_b, stop_a });
	}
	return 0;
}
