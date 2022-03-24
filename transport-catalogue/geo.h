#pragma once

#include <cmath>

namespace transport::geo
{
struct Coordinates
{
    double lat;
    double lng;
    bool operator==(const Coordinates& other) const;
    bool operator!=(const Coordinates& other) const;
};

double ComputeDistance(Coordinates from, Coordinates to);
} //transport::geo