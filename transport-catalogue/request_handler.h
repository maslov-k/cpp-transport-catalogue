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
	const sv_set* GetBusesByStop(std::string_view stop_name) const;

	svg::Document RenderMap(sv_set buses) const;

private:
	// RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
	const TransportCatalogue& db_;
	const renderer::MapRenderer& renderer_;
};

} // namespace transport::request_handler
