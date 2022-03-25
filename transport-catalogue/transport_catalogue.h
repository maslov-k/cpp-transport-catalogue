#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <set>

using sv_set = std::set<std::string_view, std::less<>>;

namespace transport
{
struct Stop
{
	std::string name;
	geo::Coordinates coordinates;
};

struct Bus
{
	std::string name;
	std::vector<const Stop*> stops;
	std::unordered_set<std::string_view> unique_stops;
};

struct RouteInfo
{
	int n_stops;
	int n_unique_stops;
	double real_length;
	double curvature;
};

class TransportCatalogue
{
public:
	void AddBus(const std::string& name, std::vector<const Stop*> stops,
		const std::unordered_set<std::string_view>& unique_stops);
	void AddStop(const std::string& name, geo::Coordinates coordinates);
	const Bus* SearchBus(std::string_view bus_name) const;
	const Stop* SearchStop(std::string_view stop_name) const;
	RouteInfo GetRouteInfo(const Bus* bus) const;
	const sv_set& GetStopToBuses(std::string_view stop_name);
	void SetDistanceBetweenStops(const Stop* stop_a, const Stop* stop_b, int distance);
	int GetDistanceBetweenStops(const Stop* stop_a, const Stop* stop_b) const;

private:
	struct StopsHasher
	{
		size_t operator() (const std::pair<const Stop*, const Stop*>& stops) const;
		std::hash<const void*> pointer_hasher;
	};
	struct BusHasher
	{
		size_t operator() (const Bus* bus) const;
		std::hash<const void*> pointer_hasher;
	};

	std::deque<Stop>									stops_;
	std::deque<Bus>										buses_;
	std::unordered_map<std::string_view, const Bus*>	name_to_bus_;
	std::unordered_map<std::string_view, const Stop*>	name_to_stop_;
	std::unordered_map<std::string_view, sv_set>		stop_to_buses_;
	std::unordered_map<std::pair<const Stop*,
		const Stop*>, int, StopsHasher>					distances_between_stops_;
	std::unordered_map<const Bus*, RouteInfo>			routes_info_;

};
} //transport
