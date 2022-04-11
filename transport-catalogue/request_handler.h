#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <optional>

namespace transport::request_handler
{

class RequestHandler
{
public:
	// MapRenderer понадобится в следующей части итогового проекта
	RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);

	// Возвращает информацию о маршруте (запрос Bus)
	std::optional<domain::RouteInfo> GetRouteInfo(std::string_view bus_name) const;

	// Возвращает маршруты, проходящие через остановку
	const transport::sv_set* GetBusesByStop(std::string_view stop_name) const;

	svg::Document RenderMap(const transport::sv_set& valid_buses) const;

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
};

} // namespace transport::request_handler
