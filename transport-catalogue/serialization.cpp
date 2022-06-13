#include "serialization.h"

#include <variant>

using namespace std;
using namespace transport;
using namespace serializer;

void TransportData::AddStop(const string& name, geo::Coordinates coordinates)
{
    stops_.push_back({ name, coordinates });
    stop_to_index_[name] = stops_.size() - 1;
}

void TransportData::AddBus(const std::string &name, const vector<const domain::Stop*>& stops, bool is_round)
{
    vector<int> stops_indexes;
    unordered_set<int> unique_stops_indexes;
    for (const domain::Stop* stop : stops)
    {
        stops_indexes.push_back(stop_to_index_[stop->name]);
        unique_stops_indexes.insert(stop_to_index_[stop->name]);
    }
    buses_.push_back({ name, move(stops_indexes), move(unique_stops_indexes), is_round });
    bus_to_index_[name] = buses_.size() - 1;
}

void TransportData::SetDistanceBetweenStops(string_view stop_a, string_view stop_b, int distance)
{
    distances_.push_back({stop_to_index_[stop_a], stop_to_index_[stop_b], distance});
}

void TransportData::SetRenderSettings(const renderer::RenderSettings& render_settings)
{
    render_settings_ = render_settings;
}

void TransportData::SetRouter(const router::TransportRouter* router)
{
    router_ = router;
}

void TransportData::Serialize(ostream& output)
{
    transport_proto::TransportData transport_data_pb;

    WrapStops(transport_data_pb);
    WrapDistances(transport_data_pb);
    WrapBuses(transport_data_pb);
    WrapRenderSettings(transport_data_pb);
    WrapRouter(transport_data_pb);

    transport_data_pb.SerializeToOstream(&output);
}

void TransportData::WrapStops(transport_proto::TransportData& transport_data_pb) const
{
    for (const auto& [stop_name, coordinates] : stops_)
        {
            transport_proto::Stop stop_pb;
            transport_proto::Coordinates coordinates_pb;
            coordinates_pb.set_lat(coordinates.lat);
            coordinates_pb.set_lng(coordinates.lng);
            *stop_pb.mutable_coordinates() = coordinates_pb;
            stop_pb.set_name(stop_name);
            *transport_data_pb.add_stop() = move(stop_pb);
        }
}

void TransportData::WrapDistances(transport_proto::TransportData& transport_data_pb) const
{
    for (const Distance& distance : distances_)
    {
        transport_proto::DistanceBetweenStops distance_pb;
        distance_pb.set_stop_a_index(distance.stop_a_index);
        distance_pb.set_stop_b_index(distance.stop_b_index);
        distance_pb.set_distance(distance.distance);
        *transport_data_pb.add_distance() = move(distance_pb);
    }
}

void TransportData::WrapBuses(transport_proto::TransportData& transport_data_pb) const
{
    for (const SimpleBus& bus : buses_)
    {
        transport_proto::Bus bus_pb;
        bus_pb.set_name(bus.name);
        for (int stop_index : bus.stops_indexes)
        {
            bus_pb.add_stop_index(stop_index);
        }
        bus_pb.set_is_round(bus.is_round);
        *transport_data_pb.add_bus() = move(bus_pb);
    }
}

void TransportData::WrapRenderSettings(transport_proto::TransportData& transport_data_pb) const
{
    render_proto::RenderSettings render_settings_pb;
    render_settings_pb.set_width(render_settings_.width);
    render_settings_pb.set_height(render_settings_.height);
    render_settings_pb.set_padding(render_settings_.padding);
    render_settings_pb.set_line_width(render_settings_.line_width);
    render_settings_pb.set_stop_radius(render_settings_.stop_radius);
    render_settings_pb.set_bus_label_font_size(render_settings_.bus_label_font_size);
    render_settings_pb.set_stop_label_font_size(render_settings_.stop_label_font_size);
    render_settings_pb.set_underlayer_width(render_settings_.underlayer_width);

    render_proto::Point point_pb;
    point_pb.set_x(render_settings_.bus_label_offset.x);
    point_pb.set_y(render_settings_.bus_label_offset.y);
    *render_settings_pb.mutable_bus_label_offset() = point_pb;

    point_pb.set_x(render_settings_.stop_label_offset.x);
    point_pb.set_y(render_settings_.stop_label_offset.y);
    *render_settings_pb.mutable_stop_label_offset() = point_pb;

    svg_proto::Color color_pb = GetProtoColor(render_settings_.underlayer_color);
    *render_settings_pb.mutable_underlayer_color() = move(color_pb);

    for (const svg::Color& color : render_settings_.color_palette)
    {
        *render_settings_pb.add_color_palette() = move(GetProtoColor(color));
    }
    *transport_data_pb.mutable_render_settings() = move(render_settings_pb);
}

void TransportData::WrapGraph(router_proto::Router& router_pb) const
{
    graph_proto::Graph graph_pb;
    const graph::DirectedWeightedGraph<double>& graph = router_->GetGraph();
    for (size_t i = 0; i < graph.GetEdgeCount(); ++i)
    {
        graph_proto::Edge edge_pb;
        const graph::Edge<double>& edge = graph.GetEdge(i);
        edge_pb.set_from(edge.from);
        edge_pb.set_to(edge.to);
        edge_pb.set_weight(edge.weight);
        *graph_pb.add_edge() = edge_pb;
    }
    for (size_t i = 0; i < graph.GetVertexCount(); ++i)
    {
        graph_proto::IncidenceList incidence_list_pb;
        vector<size_t> incidence_list = graph.GetIncidentList(i);
        for (size_t edge_id : incidence_list)
        {
            incidence_list_pb.add_edge_id(edge_id);
        }
        *graph_pb.add_incidence_list() = incidence_list_pb;
    }
    *router_pb.mutable_graph() = move(graph_pb);
}

void TransportData::WrapRoutingSettings(router_proto::Router& router_pb) const
{
    router_proto::RoutingSettings routing_settings_pb;
    const router::RoutingSettings& routing_settings = router_->GetRoutingSettings();
    routing_settings_pb.set_bus_wait_time(routing_settings.bus_wait_time);
    routing_settings_pb.set_bus_velocity(routing_settings.bus_velocity);
    *router_pb.mutable_routing_settings() = routing_settings_pb;
}

void TransportData::WrapRouter(transport_proto::TransportData& transport_data_pb) const
{
    if (!router_ && !transport_data_pb.router().has_routing_settings())
    {
        return;
    }
    router_proto::Router router_pb;
    WrapGraph(router_pb);
    WrapRoutingSettings(router_pb);
    for (size_t i = 0; i < router_->GetGraph().GetEdgeCount(); ++i)
    {
        router_proto::EdgeInfo edge_info_pb;
        const router::EdgeInfo& edge_info = router_->GetEdgeInfo(i);
        edge_info_pb.set_type(edge_info.type == router::EdgeType::BUS);
        edge_info_pb.set_stop_index(stop_to_index_.at(edge_info.stop_name));
        if (!edge_info.bus_name.empty())
        {
            edge_info_pb.set_bus_index(bus_to_index_.at(edge_info.bus_name));
        }
        else
        {
            edge_info_pb.set_bus_index(-1);
        }
        edge_info_pb.set_time(edge_info.time);
        edge_info_pb.set_span_count(edge_info.span_count);
        *router_pb.add_edge_info() = edge_info_pb;
    }
    *transport_data_pb.mutable_router() = router_pb;
}

svg_proto::Color TransportData::GetProtoColor(const svg::Color& color_svg)
{
    svg_proto::Color color_pb;
    if (holds_alternative<svg::Rgb>(color_svg))
    {
        svg::Rgb color_rgb = get<svg::Rgb>(color_svg);
        svg_proto::ColorRgb color_rgb_pb;
        color_rgb_pb.set_red(color_rgb.red);
        color_rgb_pb.set_green(color_rgb.green);
        color_rgb_pb.set_blue(color_rgb.blue);
        *color_pb.mutable_rgb() = move(color_rgb_pb);
    }
    else if (holds_alternative<svg::Rgba>(color_svg))
    {
        svg::Rgba color_rgba = get<svg::Rgba>(color_svg);
        svg_proto::ColorRgba color_rgba_pb;
        color_rgba_pb.set_red(color_rgba.red);
        color_rgba_pb.set_green(color_rgba.green);
        color_rgba_pb.set_blue(color_rgba.blue);
        color_rgba_pb.set_opacity(color_rgba.opacity);
        *color_pb.mutable_rgba() = move(color_rgba_pb);
    }
    else if (holds_alternative<string>(color_svg))
    {
        string color_str = get<string>(color_svg);
        svg_proto::ColorStr color_str_pb;
        color_str_pb.set_color(move(color_str));
        *color_pb.mutable_str() = move(color_str_pb);
    }
    return color_pb;
}

void serializer::AddStops(TransportCatalogue& tc, const transport_proto::TransportData& transport_data_pb)
{
    for (int i = 0; i < transport_data_pb.stop_size(); ++i)
    {
        transport_proto::Stop stop_pb = transport_data_pb.stop(i);
        transport_proto::Coordinates coordinates_pb = stop_pb.coordinates();
        tc.AddStop(stop_pb.name(), { coordinates_pb.lat(), coordinates_pb.lng() });
    }
}

void serializer::SetDistancesBetweenStops(TransportCatalogue& tc, const transport_proto::TransportData& transport_data_pb)
{
    for (int i = 0; i < transport_data_pb.distance_size(); ++i)
    {
        transport_proto::DistanceBetweenStops distance_btw_stops_pb = transport_data_pb.distance(i);
        int stop_a_index = distance_btw_stops_pb.stop_a_index();
        int stop_b_index = distance_btw_stops_pb.stop_b_index();
        string_view stop_a_name = transport_data_pb.stop(stop_a_index).name();
        string_view stop_b_name = transport_data_pb.stop(stop_b_index).name();
        tc.SetDistanceBetweenStops(tc.SearchStop(stop_a_name), tc.SearchStop(stop_b_name),
                                   distance_btw_stops_pb.distance());
    }
}

void serializer::AddBuses(TransportCatalogue& tc, const transport_proto::TransportData& transport_data_pb)
{
    for (int i = 0; i < transport_data_pb.bus_size(); ++i)
    {
        transport_proto::Bus bus_pb = transport_data_pb.bus(i);
        vector<const domain::Stop*> stops;
        unordered_set<string_view> unique_stops;
        for (int j = 0; j < bus_pb.stop_index_size(); ++j)
        {
            int stop_index = bus_pb.stop_index(j);
            const domain::Stop* stop = tc.SearchStop(transport_data_pb.stop(stop_index).name());
            stops.push_back(stop);
            unique_stops.insert(stop->name);
        }

        tc.AddBus(bus_pb.name(), stops, unique_stops, bus_pb.is_round());
    }
}

renderer::RenderSettings serializer::UnwrapRenderSettings(const transport_proto::TransportData& transport_data_pb)
{
    renderer::RenderSettings render_settings;
    render_proto::RenderSettings render_settings_pb = transport_data_pb.render_settings();
    render_settings.width = render_settings_pb.width();
    render_settings.height = render_settings_pb.height();
    render_settings.padding = render_settings_pb.padding();
    render_settings.line_width = render_settings_pb.line_width();
    render_settings.stop_radius = render_settings_pb.stop_radius();
    render_settings.bus_label_font_size = render_settings_pb.bus_label_font_size();
    render_settings.stop_label_font_size = render_settings_pb.stop_label_font_size();
    render_settings.underlayer_width = render_settings_pb.underlayer_width();

    render_settings.bus_label_offset = {
        render_settings_pb.bus_label_offset().x(),
        render_settings_pb.bus_label_offset().y(),
    };
    render_settings.stop_label_offset = {
        render_settings_pb.stop_label_offset().x(),
        render_settings_pb.stop_label_offset().y()
    };

    render_settings.underlayer_color = GetSvgColor(render_settings_pb.underlayer_color());

    for(int i = 0; i < render_settings_pb.color_palette_size(); ++i)
    {
        svg::Color color = GetSvgColor(render_settings_pb.color_palette(i));
        render_settings.color_palette.push_back(move(color));
    }

    return render_settings;
}

svg::Color serializer::GetSvgColor(const svg_proto::Color& color_pb)
{
    svg::Color color;
    if (color_pb.has_rgb())
    {
        color = svg::Rgb(color_pb.rgb().red(),
                         color_pb.rgb().green(),
                         color_pb.rgb().blue());
    }
    else if(color_pb.has_rgba())
    {
        color = svg::Rgba(color_pb.rgba().red(),
                         color_pb.rgba().green(),
                         color_pb.rgba().blue(),
                         color_pb.rgba().opacity());
    }
    else if(color_pb.has_str())
    {
        color = color_pb.str().color();
    }
    return color;
}

router::RoutingSettings serializer::UnwrapRoutingSettings(const router_proto::Router& router_pb)
{
    router::RoutingSettings routing_settings;
    router_proto::RoutingSettings routing_settings_pb = router_pb.routing_settings();
    routing_settings.bus_wait_time = routing_settings_pb.bus_wait_time();
    routing_settings.bus_velocity = routing_settings_pb.bus_velocity();

    return routing_settings;
}

graph::DirectedWeightedGraph<double> serializer::UnwrapGraph(const graph_proto::Graph& graph_pb)
{
    vector<graph::Edge<double>> edges;
    edges.reserve(graph_pb.edge_size());
    for (int i = 0; i < graph_pb.edge_size(); ++i)
    {
        graph_proto::Edge edge_pb = graph_pb.edge(i);
        graph::Edge<double> edge;
        edge.from = edge_pb.from();
        edge.to = edge_pb.to();
        edge.weight = edge_pb.weight();
        edges.push_back(edge);
    }
    vector<vector<size_t>> incidence_lists;
    incidence_lists.reserve(graph_pb.incidence_list_size());
    for (int i = 0; i < graph_pb.incidence_list_size(); ++i)
    {
        graph_proto::IncidenceList incidence_list_pb = graph_pb.incidence_list(i);
        vector<size_t> incidence_list;
        incidence_list.reserve(incidence_list_pb.edge_id_size());
        for (int j = 0; j < incidence_list_pb.edge_id_size(); ++j)
        {
            incidence_list.push_back(incidence_list_pb.edge_id(j));
        }
        incidence_lists.push_back(move(incidence_list));
    }
    return graph::DirectedWeightedGraph<double>(move(edges), move(incidence_lists));
}

void serializer::MakeRouter(router::TransportRouter& router, const TransportCatalogue& tc,
                            const transport_proto::TransportData& transport_data_pb)
{
    if (!transport_data_pb.has_router())
    {
        return;
    }
    const router_proto::Router& router_pb = transport_data_pb.router();
    graph::DirectedWeightedGraph<double> graph = UnwrapGraph(router_pb.graph());
    router.SetGraph(move(graph));
    router::RoutingSettings routing_settings = UnwrapRoutingSettings(router_pb);
    router.SetRoutingSettings(move(routing_settings));

    int stop_index = 0;
    for (string_view stop_name : tc.GetValidStops())
    {
        router.AddStopIndex(stop_name, stop_index++);
    }

    for (int i = 0; i < router_pb.edge_info_size(); ++i)
    {
        router::EdgeInfo edge_info;
        const router_proto::EdgeInfo& edge_info_pb = router_pb.edge_info(i);

        edge_info.type = edge_info_pb.type() ? router::EdgeType::BUS : router::EdgeType::WAIT;

        string_view stop_name_pb = transport_data_pb.stop(edge_info_pb.stop_index()).name();
        string_view stop_name = tc.SearchStop(stop_name_pb)->name;
        edge_info.stop_name = stop_name;
        if (edge_info_pb.bus_index() != -1)
        {
            string_view bus_name_pb = transport_data_pb.bus(edge_info_pb.bus_index()).name();
            string_view bus_name = tc.SearchBus(bus_name_pb)->name;
            edge_info.bus_name = bus_name;
        }

        edge_info.time = edge_info_pb.time();
        edge_info.span_count = edge_info_pb.span_count();
        router.AddEdgeInfo(i, edge_info);
    }
}

void serializer::Deserialize(std::istream &input,
                             TransportCatalogue& tc,
                             renderer::RenderSettings& render_settings,
                             router::TransportRouter& router)
{
    transport_proto::TransportData transport_data_pb;
    transport_data_pb.ParseFromIstream(&input);

    AddStops(tc, transport_data_pb);
    SetDistancesBetweenStops(tc, transport_data_pb);
    AddBuses(tc, transport_data_pb);
    render_settings = UnwrapRenderSettings(transport_data_pb);

    MakeRouter(router, tc, transport_data_pb);
}
