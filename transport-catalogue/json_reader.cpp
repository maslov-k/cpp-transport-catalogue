#include "json_reader.h"
#include "json_builder.h"
#include "graph.h"
#include "transport_router.h"
#include "serialization.h"
#include "fstream"

#include <sstream>

using namespace std;
using namespace transport::json_reader;
using namespace transport::domain;
using namespace transport::request_handler;
using namespace transport::serializer;
using namespace renderer;
using namespace json;
using namespace graph;
using namespace router;

void Reader::MakeBase(istream& input)
{
    Dict base_requests = ReadJSON(input);
    TransportData td;
    TransportCatalogue tc;
    Queries queries = ParseBaseRequests(base_requests.at("base_requests"s));
    unordered_map<string_view, const Dict*> distances_info;
    for (const Node* stop_query : queries.stop_queries)
    {
        const Dict& query_dict = stop_query->AsMap();
        const string& stop_name = query_dict.at("name"s).AsString();
        double lat = query_dict.at("latitude"s).AsDouble();
        double lng = query_dict.at("longitude"s).AsDouble();
        tc.AddStop(stop_name, { lat, lng });
        td.AddStop(stop_name, { lat, lng });
        distances_info[stop_name] = &query_dict.at("road_distances"s).AsMap();
    }
    for (const auto& [stop_name, distances] : distances_info)
    {
        for (const auto& [to_stop, distance] : *distances)
        {
            tc.SetDistanceBetweenStops(tc.SearchStop(stop_name), tc.SearchStop(to_stop), distance.AsInt());
            td.SetDistanceBetweenStops(stop_name, to_stop, distance.AsInt());
        }
    }
    for (const Node* bus_query : queries.bus_queries)
    {
        const Dict& query_dict = bus_query->AsMap();
        const string& bus_name = query_dict.at("name"s).AsString();
        bool is_round = query_dict.at("is_roundtrip"s).AsBool();
        auto [stops, unique_stops] = GetStops(tc, query_dict.at("stops"s).AsArray(), is_round);
        tc.AddBus(bus_name, stops, unique_stops, is_round);
        td.AddBus(bus_name, stops, is_round);
    }
    td.SetRenderSettings(ParseRenderSettings(base_requests));

    router::TransportRouter transport_router(tc, ParseRoutingSettings(base_requests));
    transport_router.BuildGraph();
    td.SetRouter(&transport_router);

    string file_name = base_requests.at("serialization_settings"s).AsMap().at("file"s).AsString();
    ofstream out_file(file_name, ios::binary);
    td.Serialize(out_file);
}

void Reader::ProcessRequests(istream& input, ostream& output)
{
    Dict process_requests = ReadJSON(input);

    string file_name = process_requests.at("serialization_settings"s).AsMap().at("file"s).AsString();
    ifstream in_file(file_name, ios::binary);
    TransportCatalogue tc;
    RenderSettings render_settings;
    router::TransportRouter router(tc);
    Deserialize(in_file, tc, render_settings, router);
    MapRenderer renderer(render_settings);

    RequestHandler handler(tc, renderer, router);
    ExecuteStatRequests(process_requests.at("stat_requests"s), tc, handler, output);
}

Dict Reader::ReadJSON(istream& input)
{
    Document document = Load(input);
    return document.GetRoot().AsMap();
}

Queries Reader::ParseBaseRequests(const Node& base_requests)
{
    Queries queries;
    for (const Node& query : base_requests.AsArray())
    {
        if (query.AsMap().at("type"s) == "Bus"s)
        {
            queries.bus_queries.push_back(&query);
        }
        else if (query.AsMap().at("type"s) == "Stop"s)
        {
            queries.stop_queries.push_back(&query);
        }
    }
    return queries;
}

void Reader::ExecuteStatRequests(const Node& stat_requests, const TransportCatalogue& tc,
                                 const RequestHandler& handler, ostream& output)
{
    Builder builder;
    builder.StartArray();
    for (const Node& query : stat_requests.AsArray())
    {
        const Dict& query_dict = query.AsMap();
        if (query_dict.at("type"s).AsString() == "Stop"s)
        {
            ExecuteStopRequest(query_dict, tc, handler, builder);
        }
        else if (query_dict.at("type"s).AsString() == "Bus"s)
        {
            ExecuteBusRequest(query_dict, handler, builder);
        }
        else if (query_dict.at("type"s).AsString() == "Map"s)
        {
            ExecuteMapRequest(query_dict, tc, handler, builder);
        }
        else if (query_dict.at("type"s).AsString() == "Route"s)
        {
            ExecuteRouteRequest(query_dict, handler, builder);
        }
    }
    builder.EndArray();
    Print(Document{ builder.Build() }, output);
}

void Reader::ExecuteStopRequest(const Dict& query_dict, const TransportCatalogue& tc,
                                const RequestHandler& handler, Builder& builder)
{
    Array buses_array;
    int id = query_dict.at("id"s).AsInt();
    const string& stop_name = query_dict.at("name"s).AsString();
    const sv_set* buses = handler.GetBusesByStop(stop_name);
    if (!tc.SearchStop(stop_name))
    {
        builder.StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict();
    }
    else
    {
        if (buses)
        {
            for (string_view bus_name : *buses)
            {
                buses_array.emplace_back(string{ bus_name });
            }
        }
        builder.StartDict()
                    .Key("buses"s).Value(buses_array)
                    .Key("request_id"s).Value(id)
                .EndDict();
    }
}

void Reader::ExecuteBusRequest(const Dict& query_dict, const RequestHandler& handler, Builder& builder)
{
    int id = query_dict.at("id"s).AsInt();
    const string& bus_name = query_dict.at("name"s).AsString();
    optional<BusInfo> bus_info = handler.GetBusInfo(bus_name);
    if (!bus_info)
    {
        builder.StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict();
    }
    else
    {
        builder.StartDict()
                    .Key("curvature"s).Value(bus_info->curvature)
                    .Key("request_id"s).Value(id)
                    .Key("route_length"s).Value(bus_info->real_length)
                    .Key("stop_count"s).Value(bus_info->n_stops)
                    .Key("unique_stop_count"s).Value(bus_info->n_unique_stops)
                .EndDict();
    }
}

void Reader::ExecuteMapRequest(const Dict& query_dict, const transport::TransportCatalogue& tc,
                               const RequestHandler& handler, Builder& builder)
{
    int id = query_dict.at("id"s).AsInt();
    svg::Document svg_document = handler.RenderMap(tc.GetValidBuses(), tc.GetValidStops());
    ostringstream map_out;
    svg_document.Render(map_out);
    builder.StartDict()
                .Key("map"s).Value(map_out.str())
                .Key("request_id"s).Value(id)
            .EndDict();
}

void Reader::ExecuteRouteRequest(const Dict& query_dict, const RequestHandler& handler, Builder& builder)
{
    int id = query_dict.at("id"s).AsInt();
    const string& stop_name_from = query_dict.at("from"s).AsString();
    const string& stop_name_to = query_dict.at("to"s).AsString();
    optional<RouteData> route_data = handler.GetRouteData(stop_name_from, stop_name_to);
    if (!route_data)
    {
        builder.StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict();
    }
    else
    {
        builder.StartDict()
            .Key("request_id"s).Value(id)
            .Key("total_time"s).Value(route_data->time)
            .Key("items"s).StartArray();
        for (EdgeInfo edge_info : route_data->edges_info)
        {
            builder.StartDict();
            if (edge_info.type == EdgeType::WAIT)
            {
                builder.Key("type"s).Value("Wait"s)
                       .Key("stop_name"s).Value(string{ edge_info.stop_name });
            }
            else if (edge_info.type == EdgeType::BUS)
            {
                builder.Key("type"s).Value("Bus"s)
                       .Key("bus"s).Value(string{ edge_info.bus_name })
                       .Key("span_count"s).Value(edge_info.span_count);
            }
            builder.Key("time"s).Value(edge_info.time).EndDict();
        }
        builder.EndArray().EndDict();
    }
}


pair<vector<const Stop*>, unordered_set<string_view>> Reader::GetStops(const TransportCatalogue& tc,
                                                                       const Array& stops_array,
                                                                       bool is_round) const
{
    vector<const Stop*> stops;
    unordered_set<string_view> unique_stops;
    for (const Node& stop_node : stops_array)
    {
        const Stop* stop = tc.SearchStop(stop_node.AsString());
        stops.push_back(stop);
        unique_stops.insert(stop->name);
    }
    if (!is_round)
    {
        for (int i = stops.size() - 2; i >= 0; --i)
        {
            stops.push_back(stops[i]);
        }
    }
    return { stops, unique_stops };
}

RenderSettings Reader::ParseRenderSettings(const Dict& requests) const
{
    if (!requests.count("render_settings"s))
    {
        return RenderSettings();
    }
    const Dict& render_settings_dict = requests.at("render_settings"s).AsMap();
    RenderSettings render_settings;
    render_settings.width = render_settings_dict.at("width"s).AsDouble();
    render_settings.height = render_settings_dict.at("height"s).AsDouble();
    render_settings.padding = render_settings_dict.at("padding"s).AsDouble();
    render_settings.line_width = render_settings_dict.at("line_width"s).AsDouble();
    render_settings.stop_radius = render_settings_dict.at("stop_radius"s).AsDouble();
    render_settings.bus_label_font_size = render_settings_dict.at("bus_label_font_size"s).AsInt();
    Array bus_label_offset = render_settings_dict.at("bus_label_offset"s).AsArray();
    render_settings.bus_label_offset = { bus_label_offset.at(0).AsDouble(),
                                         bus_label_offset.at(1).AsDouble() };
    render_settings.stop_label_font_size = render_settings_dict.at("stop_label_font_size"s).AsInt();
    Array stop_label_offset = render_settings_dict.at("stop_label_offset"s).AsArray();
    render_settings.stop_label_offset = { stop_label_offset.at(0).AsDouble(),
                                          stop_label_offset.at(1).AsDouble() };
    svg::Color underlayer_color = GetColor(render_settings_dict.at("underlayer_color"s));
    render_settings.underlayer_color = underlayer_color;
    render_settings.underlayer_width = render_settings_dict.at("underlayer_width"s).AsDouble();
    for (const Node& color_node : render_settings_dict.at("color_palette"s).AsArray())
    {
        render_settings.color_palette.emplace_back(GetColor(color_node));
    }
    return render_settings;
}

RoutingSettings Reader::ParseRoutingSettings(const json::Dict& requests) const
{
    if (!requests.count("routing_settings"s))
    {
        return RoutingSettings();
    }
    const Dict& routing_settings_dict = requests.at("routing_settings"s).AsMap();
    return { routing_settings_dict.at("bus_wait_time"s).AsInt(),
             routing_settings_dict.at("bus_velocity"s).AsDouble() };
}

svg::Color Reader::GetColor(json::Node color_node) const
{
    if (color_node.IsArray())
    {
        const Array& color_node_array = color_node.AsArray();
        uint8_t red = color_node_array.at(0).AsInt();
        uint8_t green = color_node_array.at(1).AsInt();
        uint8_t blue = color_node_array.at(2).AsInt();
        if (color_node_array.size() > 3)
        {
            double opacity = color_node_array.at(3).AsDouble();
            svg::Rgba rgba{ red, green, blue, opacity };
            return svg::Color(rgba);
        }
        else
        {
            svg::Rgb rgb{ red, green, blue };
            return svg::Color(rgb);
        }
    }
    else if (color_node.IsString())
    {
        return svg::Color(color_node.AsString());
    }
    return svg::Color();
}
