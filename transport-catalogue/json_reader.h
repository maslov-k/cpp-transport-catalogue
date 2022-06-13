#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "json_builder.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "router.h"
#include "transport_router.h"

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
    void MakeBase(std::istream& input);
    void ProcessRequests(std::istream& input, std::ostream& output);

private:
    json::Dict ReadJSON(std::istream& input);
    Queries ParseBaseRequests(const json::Node& base_requests);
    void ExecuteStatRequests(const json::Node& stat_requests, const transport::TransportCatalogue& tc,
		const transport::request_handler::RequestHandler& handler, std::ostream& output);
    void ExecuteStopRequest(const json::Dict& query_dict, const transport::TransportCatalogue& tc,
		const transport::request_handler::RequestHandler& handler, json::Builder& builder);
	void ExecuteBusRequest(const json::Dict& query_dict,
		const transport::request_handler::RequestHandler& handler, json::Builder& builder);
    void ExecuteMapRequest(const json::Dict& query_dict, const transport::TransportCatalogue& tc,
        const transport::request_handler::RequestHandler& handler, json::Builder& builder);
    void ExecuteRouteRequest(const json::Dict& query_dict,
        const transport::request_handler::RequestHandler& handler, json::Builder& builder);
    std::pair<std::vector<const transport::domain::Stop*>,
              std::unordered_set<std::string_view>> GetStops( const transport::TransportCatalogue& tc,
                                                              const json::Array& stops_array,
                                                              bool is_round) const;
    renderer::RenderSettings ParseRenderSettings(const json::Dict& requests) const;
    router::RoutingSettings ParseRoutingSettings(const json::Dict& requests) const;
	svg::Color GetColor(json::Node color_node) const;
};

} // namespace transport::json_reader
