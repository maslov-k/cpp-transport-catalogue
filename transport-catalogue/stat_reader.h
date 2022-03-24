#pragma once

#include "transport_catalogue.h"
#include "input_reader.h"

#include <sstream>
#include <string>
#include <vector>

namespace transport::query::stat
{
    void Process(std::istream& is, std::ostream& os, transport::TransportCatalogue& tc);
    void Execute(std::string_view raw_query, transport::TransportCatalogue& tc, std::ostream& os);
} //transport::query::stat