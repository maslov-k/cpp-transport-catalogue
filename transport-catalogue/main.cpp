#include "transport_catalogue.h"
#include "json_reader.h"
#include "svg.h"

#include <fstream>
#include <iostream>
#include <string_view>

using namespace std::literals;
using namespace transport;

void PrintUsage(std::ostream& stream = std::cerr)
{
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    json_reader::Reader reader;
    if (mode == "make_base"sv)
    {
        reader.MakeBase(std::cin);
    }
    else if (mode == "process_requests"sv)
    {
        reader.ProcessRequests(std::cin, std::cout);
    }
    else
    {
        PrintUsage();
        return 1;
    }
}
