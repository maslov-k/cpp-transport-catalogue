#include "transport_catalogue.h"
#include "json_reader.h"
#include "svg.h"

#include <iostream>

using namespace std;
using namespace std::literals;
using namespace transport;

int main()
{
	TransportCatalogue tc;
	json_reader::Reader reader(tc);
	reader.ReadJSON(cin);
	reader.ParseRequests();
	reader.GetResponses(cout);
}
