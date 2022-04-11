#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <optional>

namespace transport::request_handler
{

class RequestHandler
{
public:
	// MapRenderer ����������� � ��������� ����� ��������� �������
	RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);

	// ���������� ���������� � �������� (������ Bus)
	std::optional<domain::RouteInfo> GetRouteInfo(std::string_view bus_name) const;

	// ���������� ��������, ���������� ����� ���������
	const sv_set* GetBusesByStop(std::string_view stop_name) const;

	svg::Document RenderMap(sv_set buses) const;

private:
	// RequestHandler ���������� ��������� �������� "������������ ����������" � "������������ �����"
	const TransportCatalogue& db_;
	const renderer::MapRenderer& renderer_;
};

} // namespace transport::request_handler
