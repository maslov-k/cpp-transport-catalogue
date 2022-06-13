#include "svg.h"

namespace svg {

using namespace std::literals;

void ColorPrinter::operator()(std::monostate) const
{
	out << "none"sv;
}

void ColorPrinter::operator()(std::string str) const
{
	out << std::move(str);
}

void ColorPrinter::operator()(svg::Rgb rgb) const
{
	out << "rgb("sv << std::to_string(rgb.red) << ","sv << std::to_string(rgb.green) << ","sv
		<< std::to_string(rgb.blue) << ")"sv;
}

void ColorPrinter::operator()(svg::Rgba rgba) const
{
	out << "rgba("sv << std::to_string(rgba.red) << ","sv << std::to_string(rgba.green) << ","sv
		<< std::to_string(rgba.blue) << ","sv <<  rgba.opacity << ")"sv;
}

std::ostream& operator<<(std::ostream& output, StrokeLineCap line_cap)
{
	switch (line_cap)
	{
	case StrokeLineCap::BUTT:
		output << "butt"sv;
		break;
	case StrokeLineCap::ROUND:
		output << "round"sv;
		break;
	case StrokeLineCap::SQUARE:
		output << "square"sv;
		break;
	default:
		break;
	}
	return output;
}

std::ostream& operator<<(std::ostream& output, StrokeLineJoin line_join)
{
	switch (line_join)
	{
	case svg::StrokeLineJoin::ARCS:
		output << "arcs"sv;
		break;
	case svg::StrokeLineJoin::BEVEL:
		output << "bevel"sv;
		break;
	case svg::StrokeLineJoin::MITER:
		output << "miter"sv;
		break;
	case svg::StrokeLineJoin::MITER_CLIP:
		output << "miter-clip"sv;
		break;
	case svg::StrokeLineJoin::ROUND:
		output << "round"sv;
		break;
	default:
		break;
	}
	return output;
}


void Object::Render(const RenderContext& context) const
{
	context.RenderIndent();

	RenderObject(context);

	context.out << std::endl;
}

Circle& Circle::SetCenter(Point center)
{
	center_ = center;
	return *this;
}

Circle& Circle::SetRadius(double radius)
{
	radius_ = radius;
	return *this;
}

void Circle::RenderObject(const RenderContext& context) const
{
	auto& out = context.out;
	out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
	out << "r=\""sv << radius_ << "\""sv;
	RenderAttrs(context.out);
	out << "/>"sv;
}

Polyline& Polyline::AddPoint(Point point)
{
	points_.push_back(point);
	return *this;
}

void Polyline::RenderObject(const RenderContext& context) const
{
	auto& out = context.out;
	out << "<polyline points=\""sv;
	bool is_first = true;
	for (const Point& point : points_)
	{
		if (!is_first)
		{
			out << ' ' << point.x << ","sv << point.y;
		}
		else
		{
			out << point.x << ","sv << point.y;
			is_first = false;
		}
	}
	out << "\""sv;
	RenderAttrs(context.out);
	out << "/>"sv;
}

Text& Text::SetPosition(Point pos)
{
	pos_ = pos;
	return *this;
}

Text& Text::SetOffset(Point offset)
{
	offset_ = offset;
	return *this;
}

Text& Text::SetFontSize(uint32_t size)
{
	size_ = size;
	return *this;
}

Text& Text::SetFontFamily(std::string font_family)
{
	font_family_ = move(font_family);
	return *this;
}

Text& Text::SetFontWeight(std::string font_weight)
{
	font_weight_ = move(font_weight);
	return *this;
}

Text& Text::SetData(std::string_view data)
{
	data_.clear();
	for (char c : data)
	{
		switch (c)
		{
		case '"':
			data_.append("&quot;"s);
			break;
		case '\'':
			data_.append("&apos;"s);
			break;
		case '<':
			data_.append("&lt;"s);
			break;
		case '>':
			data_.append("&gt;"s);
			break;
		case '&':
			data_.append("&amp;"s);
			break;
		default:
			data_.push_back(c);
			break;
		}
	}
	return *this;
}

void Text::RenderObject(const RenderContext& context) const
{
	auto& out = context.out;
	out << "<text x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\""sv;
	out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
	out << " font-size=\""sv << size_ << "\"";
	if (!font_family_.empty())
	{
		out << " font-family=\""sv << font_family_ << "\""sv;
	}
	if (!font_weight_.empty())
	{
		out << " font-weight=\""sv << font_weight_ << "\""sv;
	}
	RenderAttrs(context.out);
	out << ">"sv << data_ << "</text>"sv;
}

void Document::AddPtr(std::unique_ptr<Object>&& obj)
{
	objects_.emplace_back(move(obj));
}

void Document::Render(std::ostream& out) const
{
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
	out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

	for (const auto& obj : objects_)
	{
		RenderContext ctx(out, 2, 2);
		obj->Render(ctx);
	}

	out << "</svg>"sv;
}

}  // namespace svg
