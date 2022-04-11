#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "request_handler.h"
#include "map_renderer.h"

namespace transport::json_reader
{

struct Queries
{
	std::vector<const json::Node*> stop_queries;
	std::vector<const json::Node*> bus_queries;
};

class Reader
{
public:
	explicit Reader(TransportCatalogue& tc)
		: tc_(tc)
	{
	}
	void ReadJSON(std::istream& input);
	void ParseRequests();
	void GetResponses(std::ostream& output);

private:
	Queries ParseBaseRequests(const json::Node& base_requests);
	std::pair<std::vector<const domain::Stop*>, std::unordered_set<std::string_view>>
		GetStops(const json::Array& stops_array, bool is_round) const;
	void ExecuteStatRequests(const json::Node& stat_requests,
		const transport::request_handler::RequestHandler& handler, std::ostream& output);
	void ExecuteStopRequest(const json::Dict& query_dict,
		const transport::request_handler::RequestHandler& handler, std::ostream& output);
	void ExecuteBusRequest(const json::Dict& query_dict,
		const transport::request_handler::RequestHandler& handler, std::ostream& output);
	void ExecuteMapRequest(const json::Dict& query_dict,
		const transport::request_handler::RequestHandler& handler, std::ostream& output);
	renderer::RenderSettings ParseRenderSettings();
	svg::Color GetColor(json::Node color_node) const;

	TransportCatalogue& tc_;
	json::Dict requests_;
	transport::sv_set valid_buses_;
};

} // namespace transport::json_reader