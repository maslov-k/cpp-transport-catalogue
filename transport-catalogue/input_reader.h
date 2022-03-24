#pragma once

#include "transport_catalogue.h"

#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_set>

namespace transport::query
{
enum class Type
{
    Stop,
    Bus
};

namespace detail
{
Type StringToEnum(std::string_view q);
} //query::detail

std::string_view GetWord(std::string_view text, char separate_ch, int& pos);

namespace input
{
struct Queries
{
    std::vector<std::string> stop_queries;
    std::vector<std::string> bus_queries;
};

double GetDouble(std::string_view text, int& pos);
std::pair<std::string_view, int> GetDistance(std::string_view text, int& pos);
void GetStops(std::vector<const transport::Stop*>& stops, std::unordered_set<std::string_view>& unique_stops,
    transport::TransportCatalogue& tc, std::string_view text, bool is_circle, int& pos);

void Process(std::istream& is, transport::TransportCatalogue& tc);

void Read(std::string_view text, Queries& queries);

void Execute(Queries& queries, transport::TransportCatalogue& tc);
} //transport::query::input
} //transport::query
