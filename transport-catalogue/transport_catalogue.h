#pragma once

#include "domain.h"

#include <string>
#include <string_view>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <set>
#include <optional>

namespace transport
{

using sv_set = std::set<std::string_view, std::less<>>;
using Distances_btw_stops = std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, domain::StopsHasher>;

class TransportCatalogue
{
public:
	void AddBus(const std::string& name, std::vector<const domain::Stop*> stops,
		const std::unordered_set<std::string_view>& unique_stops, bool is_round);
	void AddStop(const std::string& name, geo::Coordinates coordinates);
	const domain::Bus* SearchBus(std::string_view bus_name) const;
	const domain::Stop* SearchStop(std::string_view stop_name) const;
	domain::BusInfo GetBusInfo(const domain::Bus* bus) const;
	const sv_set* GetStopToBuses(const domain::Stop* stop) const;
	void SetDistanceBetweenStops(const domain::Stop* stop_a, const domain::Stop* stop_b, int distance);
	int GetDistanceBetweenStops(const domain::Stop* stop_a, const domain::Stop* stop_b) const;

private:
	std::deque<domain::Stop>									stops_;
	std::deque<domain::Bus>										buses_;
	std::unordered_map<std::string_view, const domain::Bus*>	name_to_bus_;
	std::unordered_map<std::string_view, const domain::Stop*>	name_to_stop_;
	std::unordered_map<const domain::Stop*, sv_set>				stop_to_buses_;
	Distances_btw_stops											distances_btw_stops_;
	std::unordered_map<const domain::Bus*, domain::BusInfo>		buses_info_;
};

} // namespace transport
