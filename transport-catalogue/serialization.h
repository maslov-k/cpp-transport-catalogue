#pragma once

#include <transport_catalogue.pb.h>

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace transport::serializer
{

struct SimpleBus
{
    std::string name;
    std::vector<int> stops_indexes;
    std::unordered_set<int> unique_stops_indexes;
    bool is_round;
};

struct Distance
{
    int stop_a_index;
    int stop_b_index;
    int distance;
};

class TransportData
{
public:
    void AddStop(const std::string& name, geo::Coordinates coordinates);
    void AddBus(const std::string& name, const std::vector<const domain::Stop*>& stops, bool is_round);
    void SetDistanceBetweenStops(std::string_view stop_a, std::string_view stop_b, int distance);
    void SetRenderSettings(const renderer::RenderSettings& render_settings);
    void SetRouter(const router::TransportRouter* router);

    void Serialize(std::ostream& output);

private:
    void WrapStops(transport_proto::TransportData& transport_data_pb) const;
    void WrapDistances(transport_proto::TransportData& transport_data_pb) const ;
    void WrapBuses(transport_proto::TransportData& transport_data_pb) const;
    void WrapRenderSettings(transport_proto::TransportData& transport_data_pb) const;
    void WrapGraph(router_proto::Router& router_pb) const;
    void WrapRoutingSettings(router_proto::Router& router_pb) const;
    void WrapRouter(transport_proto::TransportData& transport_data_pb) const;
    static svg_proto::Color GetProtoColor(const svg::Color& color);

    std::vector<std::pair<std::string, geo::Coordinates>>   stops_;
    std::unordered_map<std::string_view, int>               stop_to_index_;
    std::vector<Distance>                                   distances_;
    std::vector<SimpleBus>                                  buses_;
    std::unordered_map<std::string_view, int>               bus_to_index_;
    renderer::RenderSettings                                render_settings_;
    const router::TransportRouter*                          router_;
};

void AddStops(TransportCatalogue& tc, const transport_proto::TransportData& transport_data_pb);
void SetDistancesBetweenStops(TransportCatalogue& tc, const transport_proto::TransportData& transport_data_pb);
void AddBuses(TransportCatalogue& tc, const transport_proto::TransportData& transport_data_pb);
renderer::RenderSettings UnwrapRenderSettings(const transport_proto::TransportData& transport_data_pb);
svg::Color GetSvgColor(const svg_proto::Color& color_pb);
router::RoutingSettings UnwrapRoutingSettings(const router_proto::Router& router_pb);
graph::DirectedWeightedGraph<double> UnwrapGraph(const graph_proto::Graph& graph_pb);
void MakeRouter(router::TransportRouter& router, const TransportCatalogue& tc,
                const transport_proto::TransportData& transport_data_pb);
void Deserialize(std::istream& input, TransportCatalogue& tc,
                                      renderer::RenderSettings& render_settings,
                                      router::TransportRouter& router);

} // namespace transport::serializer
