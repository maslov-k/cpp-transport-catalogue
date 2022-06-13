#include "request_handler.h"

using namespace std;
using namespace transport::request_handler;
using namespace renderer;

RequestHandler::RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer,
                                    const router::TransportRouter& router)
    : db_(db), renderer_(renderer), router_(router)
{
}

std::optional<transport::domain::BusInfo> RequestHandler::GetBusInfo(std::string_view bus_name) const
{
	const domain::Bus* bus = db_.SearchBus(bus_name);
	if (!bus)
	{
		return std::nullopt;
	}
	domain::BusInfo bus_info = db_.GetBusInfo(bus);
	return bus_info;
}

const transport::sv_set* RequestHandler::GetBusesByStop(std::string_view stop_name) const
{
	const domain::Stop* stop = db_.SearchStop(stop_name);
	if (!stop)
	{
		return nullptr;
	}
	const sv_set* buses = db_.GetStopToBuses(stop);
	return buses;
}

svg::Document RequestHandler::RenderMap(const transport::sv_set& valid_buses, const transport::sv_set& valid_stops) const
{
    vector<geo::Coordinates> stops_coordinates;
    unordered_map<string_view, svg::Color> bus_to_color;
    unordered_map<string_view, vector<svg::Point>> bus_to_points;
    for (string_view stop_name : valid_stops)
    {
        stops_coordinates.push_back(db_.SearchStop(stop_name)->coordinates);
    }
    RenderSettings render_settings = renderer_.GetRenderSettings();
    SphereProjector sphere_projector(stops_coordinates.begin(), stops_coordinates.end(),
        render_settings.width, render_settings.height, render_settings.padding);
    size_t color_index = 0;
    size_t colors_count = render_settings.color_palette.size();
    for (string_view bus_name : valid_buses)
    {
        bus_to_color[bus_name] = render_settings.color_palette[color_index % colors_count];
        ++color_index;
        for (const domain::Stop* stop : db_.SearchBus(bus_name)->stops)
        {
            bus_to_points[bus_name].push_back(sphere_projector(stop->coordinates));
        }
    }

    vector<unique_ptr<svg::Drawable>> picture;
	
    RenderRouteLines(picture, valid_buses, bus_to_points, bus_to_color, render_settings);
    RenderRouteNames(picture, valid_buses, bus_to_color, render_settings, sphere_projector);
    RenderStops(picture, valid_stops, render_settings, sphere_projector);
    RenderStopsNames(picture, valid_stops, render_settings, sphere_projector);

    svg::Document doc;
    renderer::DrawMap(picture, doc);
    return doc;
}

optional<router::RouteData> RequestHandler::GetRouteData(std::string_view stop_from_name, string_view stop_to_name) const
{
    return router_.GetRouteData(stop_from_name, stop_to_name);
}

void RequestHandler::RenderRouteLines(vector<unique_ptr<svg::Drawable>>& picture,
    const transport::sv_set& valid_buses,
    const unordered_map<string_view, vector<svg::Point>>& bus_to_points,
    const unordered_map<string_view, svg::Color>& bus_to_color,
    const RenderSettings& render_settings) const
{
    for (string_view bus_name : valid_buses)
    {
        picture.emplace_back(make_unique<RoutePolyline>(bus_to_points.at(bus_name),
            bus_to_color.at(bus_name), render_settings.line_width));
    }
}

void RequestHandler::RenderRouteNames(vector<unique_ptr<svg::Drawable>>& picture,
    const transport::sv_set& valid_buses,
    const unordered_map<string_view, svg::Color>& bus_to_color,
    const RenderSettings& render_settings,
    const SphereProjector& sphere_projector) const
{
    for (string_view bus_name : valid_buses)
    {
        const domain::Bus* bus = db_.SearchBus(bus_name);
        const domain::Stop* first_stop = bus->stops.front();
        svg::Point first_stop_point = sphere_projector(first_stop->coordinates);
        picture.emplace_back(make_unique<RouteName>(first_stop_point, render_settings.bus_label_offset,
            render_settings.bus_label_font_size, string{ bus_name }, render_settings.underlayer_color,
            render_settings.underlayer_width, bus_to_color.at(bus_name)));
        if (!bus->is_round)
        {
            int last_stop_index = bus->stops.size() / 2;
            const domain::Stop* last_stop = bus->stops[last_stop_index];
            if (first_stop != last_stop)
            {
                svg::Point last_stop_point = sphere_projector(last_stop->coordinates);
                picture.emplace_back(make_unique<RouteName>(last_stop_point, render_settings.bus_label_offset,
                    render_settings.bus_label_font_size, string{ bus_name }, render_settings.underlayer_color,
                    render_settings.underlayer_width, bus_to_color.at(bus_name)));
            }
        }
    }
}

void RequestHandler::RenderStops(vector<unique_ptr<svg::Drawable>>& picture,
    const transport::sv_set& valid_stops,
    const RenderSettings& render_settings,
    const SphereProjector& sphere_projector) const
{
    for (string_view stop_name : valid_stops)
    {
        picture.emplace_back(make_unique<StopCircle>(sphere_projector(db_.SearchStop(stop_name)->coordinates),
            render_settings.stop_radius));
    }
}

void RequestHandler::RenderStopsNames(vector<unique_ptr<svg::Drawable>>& picture,
    const transport::sv_set& valid_stops,
    const RenderSettings& render_settings,
    const SphereProjector& sphere_projector) const
{
    for (string_view stop_name : valid_stops)
    {
        picture.emplace_back(make_unique<StopName>(sphere_projector(db_.SearchStop(stop_name)->coordinates),
            render_settings.stop_label_offset, render_settings.stop_label_font_size, string{ stop_name },
            render_settings.underlayer_color, render_settings.underlayer_width));
    }
}
