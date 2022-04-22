#pragma once

#include "geo.h"

#include <string>
#include <vector>
#include <unordered_set>

namespace transport::domain
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
	bool is_round;
};

struct RouteInfo
{
	int n_stops;
	int n_unique_stops;
	double real_length;
	double curvature;
};

struct StopsHasher
{
	size_t operator()(const std::pair<const Stop*, const Stop*>& stops) const;
	std::hash<const void*> pointer_hasher;
};

} // namspace transport::domain
