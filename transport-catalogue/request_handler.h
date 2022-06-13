#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <optional>

namespace transport::request_handler
{

class RequestHandler
{
public:
    RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer,
                    const router::TransportRouter& router);

	std::optional<domain::BusInfo> GetBusInfo(std::string_view bus_name) const;

	const transport::sv_set* GetBusesByStop(std::string_view stop_name) const;

    svg::Document RenderMap(const transport::sv_set& valid_buses, const transport::sv_set& valid_stops) const;

    std::optional<router::RouteData> GetRouteData(std::string_view stop_from_name,
                                                  std::string_view stop_to_name) const;

private:
    void RenderRouteLines(std::vector<std::unique_ptr<svg::Drawable>>& picture,
        const transport::sv_set& valid_buses,
        const std::unordered_map<std::string_view,
        std::vector<svg::Point>>& bus_to_points,
        const std::unordered_map<std::string_view, svg::Color>& bus_to_color,
        const renderer::RenderSettings& render_settings) const;
    void RenderRouteNames(std::vector<std::unique_ptr<svg::Drawable>>& picture,
        const transport::sv_set& valid_buses,
        const std::unordered_map<std::string_view,
        svg::Color>& bus_to_color,
        const renderer::RenderSettings& render_settings,
        const renderer::SphereProjector& sphere_projector) const;
    void RenderStops(std::vector<std::unique_ptr<svg::Drawable>>& picture,
        const transport::sv_set& valid_stops,
        const renderer::RenderSettings& render_settings,
        const renderer::SphereProjector& sphere_projector) const;
    void RenderStopsNames(std::vector<std::unique_ptr<svg::Drawable>>& picture,
        const transport::sv_set& valid_stops,
        const renderer::RenderSettings& render_settings,
        const renderer::SphereProjector& sphere_projector) const;

	const TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    const router::TransportRouter& router_;
};

} // namespace transport::request_handler
