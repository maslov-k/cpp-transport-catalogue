#include "domain.h"

size_t transport::domain::StopsHasher::operator()(const std::pair<const Stop*, const Stop*>& stops) const
{
	return pointer_hasher(stops.first) + pointer_hasher(stops.second) * 37;
}
