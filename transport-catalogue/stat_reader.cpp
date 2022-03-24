#include "stat_reader.h"

#include <iomanip>

using namespace std;

namespace transport::query::stat
{
void Process(istream& is, ostream& os, TransportCatalogue& tc)
{
    int queries_num;
    is >> queries_num;
    string raw_query;
    getline(is, raw_query);
    for (int i = 0; i < queries_num; ++i)
    {
        getline(is, raw_query);
        Execute(raw_query, tc, os);
    }
}

void Execute(string_view raw_query, TransportCatalogue& tc, ostream& os)
{
    int pos = 0;
    Type query_type = detail::StringToEnum(GetWord(raw_query, ' ', pos));
    raw_query.remove_prefix(pos);
    if (query_type == Type::Stop)
    {
        string_view stop_name = raw_query;
        const Stop* stop = tc.SearchStop(stop_name);
        if (!stop)
        {
            os << "Stop "s << stop_name << ": not found\n"s;
            return;
        }
        const sv_set& stop_to_buses = tc.GetStopToBuses(stop_name);
        if (stop_to_buses.empty())
        {
            os << "Stop "s << stop_name << ": no buses\n"s;
            return;
        }
        os << "Stop "s << stop_name << ": buses "s;
        for (string_view bus_name : stop_to_buses)
        {
            os << bus_name << " "s;
        }
        os << "\n"s;
    }
    else if (query_type == Type::Bus)
    {
        string_view bus_name = raw_query;
        const Bus* bus = tc.SearchBus(bus_name);
        if (!bus)
        {
            os << "Bus "s << bus_name << ": not found\n"s;
            return;
        }
        RouteInfo route = tc.GetRouteInfo(bus);
        os << "Bus "s << bus_name << ": "s << route.n_stops << " stops on route, "s <<
            route.n_unique_stops << " unique stops, "s << setprecision(6) <<
            route.real_length << " route length, "s << route.curvature << " curvature\n";
    }
}
} //transport::query::stat_query
