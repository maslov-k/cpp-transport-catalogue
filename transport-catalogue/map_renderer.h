#pragma once

#include "svg.h"
#include "geo.h"
#include "domain.h"

#include <algorithm>


inline const double EPSILON = 1e-6;

namespace renderer
{

struct RenderSettings
{
	double width;
	double height;
	double padding;
	double line_width;
	double stop_radius;
	int bus_label_font_size;
	svg::Point bus_label_offset;
	int stop_label_font_size;
	svg::Point stop_label_offset;
	svg::Color underlayer_color;
	double underlayer_width;
	std::vector<svg::Color> color_palette;
};

inline bool IsZero(double value) {
	return std::abs(value) < EPSILON;
}

class SphereProjector
{
public:
	template <typename PointInputIt>
	SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
		double max_height, double padding)
		: padding_(padding)
	{
		if (points_begin == points_end) 
		{
			return;
		}

		const auto [left_it, right_it]
			= std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs)
				{
					return lhs.lng < rhs.lng;
				});
		min_lon_ = left_it->lng;
		const double max_lon = right_it->lng;

		const auto [bottom_it, top_it]
			= std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs)
				{
					return lhs.lat < rhs.lat;
				});
		const double min_lat = bottom_it->lat;
		max_lat_ = top_it->lat;

		std::optional<double> width_zoom;
		if (!IsZero(max_lon - min_lon_))
		{
			width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
		}

		std::optional<double> height_zoom;
		if (!IsZero(max_lat_ - min_lat))
		{
			height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
		}

		if (width_zoom && height_zoom)
		{
			zoom_coeff_ = std::min(*width_zoom, *height_zoom);
		}
		else if (width_zoom)
		{
			zoom_coeff_ = *width_zoom;
		}
		else if (height_zoom)
		{
			zoom_coeff_ = *height_zoom;
		}
	}

	svg::Point operator()(geo::Coordinates coords) const;

private:
	double padding_;
	double min_lon_ = 0;
	double max_lat_ = 0;
	double zoom_coeff_ = 0;
};

class RoutePolyline : public svg::Drawable
{
public:
	RoutePolyline(std::vector<svg::Point> points, svg::Color stroke_color, double stroke_width);

	void Draw(svg::ObjectContainer& container) const override;

private:
	std::vector<svg::Point> points_;
	svg::Color stroke_color_;
	double stroke_width_;
};

class StopCircle : public svg::Drawable
{
public:
	StopCircle(svg::Point center, double radius);

	void Draw(svg::ObjectContainer& container) const override;

private:
	svg::Point center_;
	double radius_;
};

class RouteName : public svg::Drawable
{
public:
	RouteName(svg::Point pos, svg::Point offset, uint32_t font_size, std::string route_name,
		svg::Color underlayer_color, double stroke_width, svg::Color route_color);

	void Draw(svg::ObjectContainer& container) const override;

private:
	svg::Point pos_;
	svg::Point offset_;
	uint32_t font_size_;
	std::string route_name_;
	svg::Color underlayer_color_;
	double stroke_width_;
	svg::Color route_color_;
};

class StopName : public svg::Drawable
{
public:
	StopName(svg::Point pos, svg::Point offset, uint32_t font_size, std::string stop_name,
		svg::Color underlayer_color, double stroke_width);

	void Draw(svg::ObjectContainer& container) const override;

private:
	svg::Point pos_;
	svg::Point offset_;
	uint32_t font_size_;
	std::string stop_name_;
	svg::Color underlayer_color_;
	double stroke_width_;
};

class MapRenderer
{
public:
	MapRenderer(RenderSettings render_settings)
		: render_settings_(std::move(render_settings))
	{
	}

	RenderSettings GetRenderSettings() const;

private:
	RenderSettings render_settings_;
};

template <typename Container>
void DrawMap(const Container& container, svg::ObjectContainer& target);

template<typename Container>
void DrawMap(const Container& container, svg::ObjectContainer& target)
{
	for (auto it = std::begin(container); it != std::end(container); ++it)
	{
		(*it)->Draw(target);
	}
}

} // namespace renderer