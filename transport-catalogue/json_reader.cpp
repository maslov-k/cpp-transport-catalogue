#include "json_reader.h"

#include <sstream>

using namespace std;
using namespace transport::json_reader;
using namespace transport::domain;
using namespace transport::request_handler;
using namespace renderer;
using namespace json;

void Reader::ReadJSON(istream& input)
{
	Document document = Load(input);
	requests_ = document.GetRoot().AsMap();
}

void Reader::ParseRequests()
{
	Queries queries = ParseBaseRequests(requests_.at("base_requests"s));
	unordered_map<string_view, const Dict*> distances_info;
	for (const Node* stop_query : queries.stop_queries)
	{
		const Dict& query_dict = stop_query->AsMap();
		const string& stop_name = query_dict.at("name"s).AsString();
		double lat = query_dict.at("latitude"s).AsDouble();
		double lng = query_dict.at("longitude"s).AsDouble();
		tc_.AddStop(stop_name, { lat, lng });
		distances_info[stop_name] = &query_dict.at("road_distances"s).AsMap();
	}
	for (const auto& [stop_name, distances] : distances_info)
	{
		for (const auto& [to_stop, distance] : *distances)
		{
			tc_.SetDistanceBetweenStops(tc_.SearchStop(stop_name), tc_.SearchStop(to_stop), distance.AsInt());
		}
	}
	for (const Node* bus_query : queries.bus_queries)
	{
		const Dict& query_dict = bus_query->AsMap();
		const string& bus_name = query_dict.at("name"s).AsString();
		bool is_round = query_dict.at("is_roundtrip"s).AsBool();
		auto [stops, unique_stops] = GetStops(query_dict.at("stops"s).AsArray(), is_round);
		tc_.AddBus(bus_name, stops, unique_stops, is_round);
	}
}

void Reader::GetResponses(ostream& output)
{
	RenderSettings render_settings = ParseRenderSettings();
	MapRenderer renderer(render_settings);
	RequestHandler handler(tc_, renderer);
	ExecuteStatRequests(requests_.at("stat_requests"s), handler, output);
}

Queries Reader::ParseBaseRequests(const Node& base_requests)
{
	Queries queries;
	for (const Node& query : base_requests.AsArray())
	{
		if (query.AsMap().at("type"s) == "Bus"s)
		{
			queries.bus_queries.push_back(&query);
			if (!query.AsMap().at("stops"s).AsArray().empty())
			{
				valid_buses_.insert(query.AsMap().at("name"s).AsString());
			}
		}
		else if (query.AsMap().at("type"s) == "Stop"s)
		{
			queries.stop_queries.push_back(&query);
		}
	}
	return queries;
}

pair<vector<const Stop*>, unordered_set<string_view>> Reader::GetStops(const Array& stops_array, bool is_round) const
{
	vector<const Stop*> stops;
	unordered_set<string_view> unique_stops;
	for (const Node& stop_node : stops_array)
	{
		const Stop* stop = tc_.SearchStop(stop_node.AsString());
		stops.push_back(stop);
		unique_stops.insert(stop->name);
	}
	if (!is_round)
	{
		for (int i = stops.size() - 2; i >= 0; --i)
		{
			stops.push_back(stops[i]);
		}
	}
	return { stops, unique_stops };
}

void Reader::ExecuteStatRequests(const Node& stat_requests, const RequestHandler& handler, ostream& output)
{
	output << "[\n"s;
	bool is_first = true;
	for (const Node& query : stat_requests.AsArray())
	{
		if (is_first)
		{
			is_first = false;
		}
		else
		{
			output << ",\n";
		}
		const Dict& query_dict = query.AsMap();
		if (query_dict.at("type"s).AsString() == "Stop"s)
		{
			ExecuteStopRequest(query_dict, handler, output);
		}
		else if (query_dict.at("type"s).AsString() == "Bus"s)
		{
			ExecuteBusRequest(query_dict, handler, output);
		}
		else if (query_dict.at("type"s).AsString() == "Map"s)
		{
			ExecuteMapRequest(query_dict, handler, output);
		}
	}
	output << "\n]"s;
}

void Reader::ExecuteStopRequest(const Dict& query_dict, const RequestHandler& handler, ostream& output)
{
	Array buses_array;
	int id = query_dict.at("id"s).AsInt();
	const string& stop_name = query_dict.at("name"s).AsString();
	const sv_set* buses = handler.GetBusesByStop(stop_name);
	int current_indent = 4;
	if (!tc_.SearchStop(stop_name))
	{
		Print(Document{ Dict{ {"request_id"s, id}, {"error_message"s , "not found"s} } }, output, current_indent);
	}
	else
	{
		if (buses)
		{
			for (string_view bus_name : *buses)
			{
				buses_array.emplace_back(string{ bus_name });
			}
		}
		Print(Document{ Dict{ {"buses"s, buses_array}, {"request_id"s, id} } }, output, current_indent);
	}
}

void Reader::ExecuteBusRequest(const Dict& query_dict, const RequestHandler& handler, ostream& output)
{
	int id = query_dict.at("id"s).AsInt();
	const string& bus_name = query_dict.at("name"s).AsString();
	optional<RouteInfo> route_info = handler.GetRouteInfo(bus_name);
	int current_indent = 4;
	if (!route_info)
	{
		Print(Document{ Dict{ {"request_id"s, id}, {"error_message"s , "not found"s} } }, output, current_indent);
	}
	else
	{
		Print(Document{ Dict{
			{"curvature"s, route_info->curvature},
			{"request_id"s, id},
			{"route_length"s, route_info->real_length},
			{"stop_count"s, route_info->n_stops},
			{"unique_stop_count"s, route_info->n_unique_stops} } }, output, current_indent);
	}
}

void Reader::ExecuteMapRequest(const Dict& query_dict, const RequestHandler& handler, ostream& output)
{
	int id = query_dict.at("id"s).AsInt();
	svg::Document svg_document = handler.RenderMap(valid_buses_);
	ostringstream map_out;
	svg_document.Render(map_out);
	int current_indent = 4;
	Print(Document{ Dict{ {"map"s, map_out.str()}, {"request_id"s , id} } }, output, current_indent);
}

RenderSettings Reader::ParseRenderSettings()
{
	if (!requests_.count("render_settings"s))
	{
		return RenderSettings();
	}
	const Dict& render_settings_dict = requests_.at("render_settings"s).AsMap();
	RenderSettings render_settings;
	render_settings.width = render_settings_dict.at("width"s).AsDouble();
	render_settings.height = render_settings_dict.at("height"s).AsDouble();
	render_settings.padding = render_settings_dict.at("padding"s).AsDouble();
	render_settings.line_width = render_settings_dict.at("line_width"s).AsDouble();
	render_settings.stop_radius = render_settings_dict.at("stop_radius"s).AsDouble();
	render_settings.bus_label_font_size = render_settings_dict.at("bus_label_font_size"s).AsInt();
	Array bus_label_offset = render_settings_dict.at("bus_label_offset"s).AsArray();
	render_settings.bus_label_offset = { bus_label_offset.at(0).AsDouble(),
										 bus_label_offset.at(1).AsDouble() };
	render_settings.stop_label_font_size = render_settings_dict.at("stop_label_font_size"s).AsInt();
	Array stop_label_offset = render_settings_dict.at("stop_label_offset"s).AsArray();
	render_settings.stop_label_offset = { stop_label_offset.at(0).AsDouble(),
										 stop_label_offset.at(1).AsDouble() };
	svg::Color underlayer_color = GetColor(render_settings_dict.at("underlayer_color"s));
	render_settings.underlayer_color = underlayer_color;
	render_settings.underlayer_width = render_settings_dict.at("underlayer_width"s).AsDouble();
	for (const Node& color_node : render_settings_dict.at("color_palette"s).AsArray())
	{
		render_settings.color_palette.emplace_back(GetColor(color_node));
	}
	return render_settings;
}

svg::Color Reader::GetColor(json::Node color_node) const
{
	if (color_node.IsArray())
	{
		const Array& color_node_array = color_node.AsArray();
		uint8_t red = color_node_array.at(0).AsInt();
		uint8_t green = color_node_array.at(1).AsInt();
		uint8_t blue = color_node_array.at(2).AsInt();
		if (color_node_array.size() > 3)
		{
			double opacity = color_node_array.at(3).AsDouble();
			svg::Rgba rgba{ red, green, blue, opacity };
			return svg::Color(rgba);
		}
		else
		{
			svg::Rgb rgb{ red, green, blue };
			return svg::Color(rgb);
		}
	}
	else if (color_node.IsString())
	{
		return svg::Color(color_node.AsString());
	}
	return svg::Color();
}
