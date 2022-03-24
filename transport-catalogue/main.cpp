#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

#include <iostream>

using namespace std;

int main()
{
	transport::TransportCatalogue tc;
	transport::query::input::Process(cin, tc);
	transport::query::stat::Process(cin, cout, tc);
}
