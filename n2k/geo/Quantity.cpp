#include <vector>
#include <algorithm>
#include "Angle.h"
#include "Direction.h"
#include "Speed.h"
#include "UtcTime.h"
#include "GeoLoc.h"
#include "Distance.h"

Angle Angle::INVALID = Angle();
Direction Direction::INVALID = Direction();
Speed Speed::INVALID = Speed();
UtcTime UtcTime::INVALID = UtcTime();
GeoLoc GeoLoc::INVALID = GeoLoc();
Distance Distance::INVALID = Distance();

double Quantity::medianValue(std::vector<double> &vals) {
    std::sort(vals.begin(), vals.end());
    size_t size = vals.size();
    double medianValue;
    if (size % 2 == 0) {
        // If the number of angles is even, average the middle two values
        medianValue = (vals[size / 2 - 1] + vals[size / 2]) / 2.0;
    } else {
        // If the number of angles is odd, return the middle value
        medianValue =  vals[size / 2];
    }
    return medianValue;
}

double Quantity::medianAngle(std::vector<double> &sines, std::vector<double> &cosines) {
    double medianSine = medianValue(sines);
    double medianCosine = medianValue(cosines);

    auto medianRadians = atan2(medianSine, medianCosine);
    return medianRadians;
}

Angle Angle::median(const std::list<Angle> &values, uint64_t utcMs) {
    std::vector<double> sines;
    std::vector<double> cosines;
    for(auto &v: values ) {
        if( v.isValid(utcMs) ){
            sines.push_back(sin(v.getRadians()));
            cosines.push_back(cos(v.getRadians()));
        }
    }
    if( sines.empty())
        return Angle::INVALID;
    double medianRadians = medianAngle(sines, cosines);
    return Angle::fromRadians(medianRadians, utcMs);
}

Direction Direction::median(const std::list<Direction> &values, uint64_t utcMs) {
    std::vector<double> sines;
    std::vector<double> cosines;
    for(auto &v: values ) {
        if( v.isValid(utcMs) ){
            sines.push_back(sin(v.getRadians()));
            cosines.push_back(cos(v.getRadians()));
        }
    }
    if( sines.empty())
        return Direction::INVALID;
    double medianRadians = medianAngle(sines, cosines);
    return Direction::fromRadians(medianRadians, utcMs);
}

GeoLoc GeoLoc::median(const std::list<GeoLoc> &values, uint64_t utcMs) {
    std::vector<double> lats;
    std::vector<double> lons;
    for(auto &v: values ) {
        if( v.isValid(utcMs) ){
            lats.push_back(v.getLat());
            lons.push_back(v.getLon());
        }
    }
    if( lats.empty())
        return GeoLoc::INVALID;
    double medianLat = medianValue(lats);
    double medianLon = medianValue(lons);
    return GeoLoc::fromDegrees(medianLat, medianLon, utcMs);
}

Speed Speed::median(const std::list<Speed> &values, uint64_t utcMs) {
    std::vector<double> vals;
    for(auto &v: values ) {
        if( v.isValid(utcMs) ){
            vals.push_back(v.getKnots());
        }
    }
    if( vals.empty())
        return Speed::INVALID;
    double medianSpeed = medianValue(vals);
    return Speed::fromKnots(medianSpeed, utcMs);
}
