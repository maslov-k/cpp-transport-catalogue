#include "map_renderer.h"

using namespace std;
using namespace svg;
using namespace renderer;

RoutePolyline::RoutePolyline(vector<Point> points, Color stroke_color, double stroke_width)
	: points_(move(points)), stroke_color_(stroke_color), stroke_width_(stroke_width)
{
}

void RoutePolyline::Draw(ObjectContainer& container) const
{
	Polyline polyline;
	for (Point point : points_)
	{
		polyline.AddPoint(point);
	}
	container.Add(polyline.SetFillColor("none"s)
						  .SetStrokeColor(stroke_color_)
						  .SetStrokeWidth(stroke_width_)
						  .SetStrokeLineCap(StrokeLineCap::ROUND)
						  .SetStrokeLineJoin(StrokeLineJoin::ROUND));
}

StopCircle::StopCircle(Point center, double radius)
	: center_(center), radius_(radius)
{
}

void StopCircle::Draw(ObjectContainer& container) const
{
	container.Add(Circle().SetCenter(center_)
						  .SetRadius(radius_)
						  .SetFillColor("white"s));
}


StopName::StopName(Point pos, Point offset, uint32_t font_size, string stop_name,
	Color underlayer_color, double stroke_width)
	: pos_(pos), offset_(offset), font_size_(font_size), stop_name_(move(stop_name)),
		underlayer_color_(underlayer_color), stroke_width_(stroke_width)
{
}

void StopName::Draw(ObjectContainer& container) const
{
	const Text base_text =
		Text()
		.SetPosition(pos_)
		.SetOffset(offset_)
		.SetFontSize(font_size_)
		.SetFontFamily("Verdana"s)
		.SetData(stop_name_);
	container.Add(Text{ base_text }
		.SetFillColor(underlayer_color_)
		.SetStrokeColor(underlayer_color_)
		.SetStrokeWidth(stroke_width_)
		.SetStrokeLineCap(StrokeLineCap::ROUND)
		.SetStrokeLineJoin(StrokeLineJoin::ROUND));
	container.Add(Text{ base_text }
		.SetFillColor("black"s));
}

RouteName::RouteName(Point pos, Point offset, uint32_t font_size, string route_name,
	Color underlayer_color, double stroke_width, Color route_color)
	: pos_(pos), offset_(offset), font_size_(font_size), route_name_(move(route_name)),
	underlayer_color_(underlayer_color), stroke_width_(stroke_width), route_color_(route_color)
{
}

void RouteName::Draw(svg::ObjectContainer& container) const
{
	const Text base_text =
		Text()
		.SetPosition(pos_)
		.SetOffset(offset_)
		.SetFontSize(font_size_)
		.SetFontFamily("Verdana"s)
		.SetFontWeight("bold"s)
		.SetData(route_name_);
	container.Add(Text{ base_text }
		.SetFillColor(underlayer_color_)
		.SetStrokeColor(underlayer_color_)
		.SetStrokeWidth(stroke_width_)
		.SetStrokeLineCap(StrokeLineCap::ROUND)
		.SetStrokeLineJoin(StrokeLineJoin::ROUND));
	container.Add(Text{ base_text }
		.SetFillColor(route_color_));
}

svg::Point renderer::SphereProjector::operator()(geo::Coordinates coords) const
{
	return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
		(max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
}

RenderSettings renderer::MapRenderer::GetRenderSettings() const
{
	return render_settings_;
}
