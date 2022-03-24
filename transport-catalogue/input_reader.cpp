#include "input_reader.h"

using namespace std;
using namespace transport;

query::Type query::detail::StringToEnum(string_view q)
{
    if (q == "Stop")
    {
        return query::Type::Stop;
    }
    else if (q == "Bus")
    {
        return query::Type::Bus;
    }
    else
    {
        return query::Type();
    }
}

string_view query::GetWord(string_view text, char separate_ch, int& pos)
{
    int start_pos = text.find_first_not_of(' ', pos);
    int end_pos = text.find(separate_ch, start_pos);
    while (end_pos > 0 && text[end_pos - 1] == ' ')
    {
        --end_pos;
    }
    pos = text.find_first_not_of(" ,:->"s, end_pos);
    return text.substr(start_pos, end_pos - start_pos);
}

namespace transport::query::input
{
double GetDouble(string_view text, int& pos)
{
    return stod(GetWord(text, ',', pos).data());
}

pair<string_view, int> GetDistance(string_view text, int& pos)
{
    int distance = 0;
    string_view stop_name;

    distance = stoi(GetWord(text, 'm', pos).data());
    pos += 5;
    stop_name = GetWord(text, ',', pos);

    return { stop_name, distance };
}

void GetStops(vector<const Stop*>& stops, unordered_set<string_view>& unique_stops,
    TransportCatalogue& tc, string_view text, bool is_circle, int& pos)
{
    char separate_ch = '-';
    if (is_circle)
    {
        separate_ch = '>';
    }
    while (pos != text.npos)
    {
        const Stop* stop = tc.SearchStop(GetWord(text, separate_ch, pos));
        stops.push_back(stop);
        unique_stops.insert(stop->name);
    }
    if (!is_circle)
    {
        for (int i = stops.size() - 2; i >= 0; --i)
        {
            stops.push_back(stops[i]);
        }
    }
}

void Process(istream& is, TransportCatalogue& tc)
{
    Queries queries;
    int queries_num;
    is >> queries_num;
    string raw_query;
    getline(is, raw_query);
    for (int i = 0; i < queries_num; ++i)
    {
        getline(is, raw_query);
        Read(raw_query, queries);
    }
    Execute(queries, tc);
}

void Read(string_view raw_query, Queries& queries)
{
    int pos = 0;
    Type query_type = detail::StringToEnum(GetWord(raw_query, ' ', pos));
    raw_query.remove_prefix(pos);
    if (query_type == Type::Stop)
    {
        queries.stop_queries.push_back(string{ raw_query });
    }
    else if (query_type == Type::Bus)
    {
        queries.bus_queries.push_back(string{ raw_query });
    }
}

void Execute(Queries& queries, TransportCatalogue& tc)
{
    unordered_map<string_view, string_view> distances_info;
    for (string_view stop_query : queries.stop_queries)
    {
        int pos = 0;
        string_view stop_name = GetWord(stop_query, ':', pos);
        double lat = GetDouble(stop_query, pos);
        double lng = GetDouble(stop_query, pos);
        tc.AddStop(string{ stop_name }, { lat, lng });

        if (pos != stop_query.npos)
        {
            distances_info[stop_name] = stop_query.substr(pos);
        }
    }
    for (auto [stop_name, distances] : distances_info)
    {
        int pos = 0;
        while (pos != distances.npos)
        {
            auto [to_stop, distance] = GetDistance(distances, pos);
            tc.SetDistanceBetweenStops(tc.SearchStop(stop_name), tc.SearchStop(to_stop), distance);
        }
    }
    for (string_view bus_query : queries.bus_queries)
    {
        int pos = 0;
        string_view bus_name = GetWord(bus_query, ':', pos);
        bool is_circle = false;
        if (bus_query.find('>') != bus_query.npos)
        {
            is_circle = true;
        }
        vector<const Stop*> stops;
        unordered_set<string_view> unique_stops;
        GetStops(stops, unique_stops, tc, bus_query, is_circle, pos);
        tc.AddBus(string{ bus_name }, stops, unique_stops);
    }
}
} //transport::query::input
