#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg
{
struct Point
{
	Point() = default;
	Point(double x, double y)
		: x(x)
		, y(y)
    {
	}
	double x = 0;
	double y = 0;
};

struct Rgb
{
	Rgb() = default;
	Rgb(uint8_t red, uint8_t green, uint8_t blue)
		: red(red), green(green), blue(blue)
	{
	}
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
};

struct Rgba
{
	Rgba() = default;
	Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
		: red(red), green(green), blue(blue), opacity(opacity)
	{
	}
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
	double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;
inline const Color NoneColor{ "none" };

struct ColorPrinter
{
	std::ostream& out;

	void operator()(std::monostate) const;
	void operator()(std::string str) const;
	void operator()(svg::Rgb rgb) const;
	void operator()(svg::Rgba rgba) const;
};

enum class StrokeLineCap
{
	BUTT,
	ROUND,
	SQUARE,
};

enum class StrokeLineJoin
{
	ARCS,
	BEVEL,
	MITER,
	MITER_CLIP,
	ROUND,
};

std::ostream& operator<<(std::ostream& output, StrokeLineCap line_cap);
std::ostream& operator<<(std::ostream& output, StrokeLineJoin line_join);

struct RenderContext
{
	RenderContext(std::ostream& out)
		: out(out)
	{
	}

	RenderContext(std::ostream& out, int indent_step, int indent = 0)
		: out(out)
		, indent_step(indent_step)
		, indent(indent)
	{
	}

	RenderContext Indented() const
	{
		return { out, indent_step, indent + indent_step };
	}

	void RenderIndent() const
	{
		for (int i = 0; i < indent; ++i)
		{
			out.put(' ');
		}
	}

	std::ostream& out;
	int indent_step = 0;
	int indent = 0;
};

class Object
{
public:
	void Render(const RenderContext& context) const;

	virtual ~Object() = default;

private:
	virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer
{
public:
	template <typename Obj>
	void Add(Obj obj);

	virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
protected:
	~ObjectContainer() = default;
};

class Drawable
{
public:
	virtual ~Drawable() = default;
	virtual void Draw(ObjectContainer& container) const = 0;
};

template <typename Owner>
class PathProps
{
public:
	Owner& SetFillColor(Color color);
	Owner& SetStrokeColor(Color color);
	Owner& SetStrokeWidth(double width);
	Owner& SetStrokeLineCap(StrokeLineCap line_cap);
	Owner& SetStrokeLineJoin(StrokeLineJoin line_join);

protected:
	~PathProps() = default;

	void RenderAttrs(std::ostream& out) const;

private:
	Owner& AsOwner();

	std::optional<Color> fill_color_;
	std::optional<Color> stroke_color_;
	std::optional<double> stroke_width_;
	std::optional<StrokeLineCap> stroke_linecap_;
	std::optional<StrokeLineJoin> stroke_linejoin_;
};

class Circle : public Object, public PathProps<Circle>
{
public:
	Circle& SetCenter(Point center);
	Circle& SetRadius(double radius);

private:
	void RenderObject(const RenderContext& context) const override;

	Point center_;
	double radius_ = 1.0;
};

class Polyline : public Object, public PathProps<Polyline>
{
public:
	Polyline& AddPoint(Point point);

private:
	void RenderObject(const RenderContext& context) const override;

	std::vector<Point> points_;
};

class Text : public Object, public PathProps<Text>
{
public:
	Text& SetPosition(Point pos);
	Text& SetOffset(Point offset);
	Text& SetFontSize(uint32_t size);
	Text& SetFontFamily(std::string font_family);
	Text& SetFontWeight(std::string font_weight);
	Text& SetData(std::string_view data);

private:
	void RenderObject(const RenderContext& context) const override;

	Point pos_;
	Point offset_;
	uint32_t size_ = 1;
	std::string font_family_;
	std::string font_weight_;
	std::string data_;
};

class Document : public ObjectContainer
{
public:
	void AddPtr(std::unique_ptr<Object>&& obj) override;
    
	void Render(std::ostream& out) const;
private:
	std::vector<std::unique_ptr<Object>> objects_;
};

template<typename Obj>
void ObjectContainer::Add(Obj obj)
{
	AddPtr(std::make_unique<Obj>(std::move(obj)));
}

template<typename Owner>
Owner& PathProps<Owner>::SetFillColor(Color color)
{
	fill_color_ = std::move(color);
	return AsOwner();
}

template<typename Owner>
Owner& PathProps<Owner>::SetStrokeColor(Color color)
{
	stroke_color_ = std::move(color);
	return AsOwner();
}

template<typename Owner>
Owner& PathProps<Owner>::SetStrokeWidth(double width)
{
	stroke_width_ = width;
	return AsOwner();
}

template<typename Owner>
Owner& PathProps<Owner>::SetStrokeLineCap(StrokeLineCap line_cap)
{
	stroke_linecap_ = std::move(line_cap);
	return AsOwner();
}

template<typename Owner>
inline Owner& PathProps<Owner>::SetStrokeLineJoin(StrokeLineJoin line_join)
{
	stroke_linejoin_ = std::move(line_join);
	return AsOwner();
}

template<typename Owner>
void PathProps<Owner>::RenderAttrs(std::ostream& out) const
{
	using namespace std::literals;

	if (fill_color_)
	{
		out << " fill=\""sv;
		std::visit(ColorPrinter{ out }, *fill_color_);
		out << "\""sv;
	}
	if (stroke_color_)
	{
		out << " stroke=\""sv;
		std::visit(ColorPrinter{ out }, *stroke_color_);
		out << "\""sv;
	}
	if (stroke_width_)
	{
		out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
	}
	if (stroke_linecap_)
	{
		out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
	}
	if (stroke_linejoin_)
	{
		out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
	}
}

template<typename Owner>
Owner& PathProps<Owner>::AsOwner()
{
	return static_cast<Owner&>(*this);
}

}  // namespace svg
