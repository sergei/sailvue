#ifndef SAILVUE_GEOLOC_H
#define SAILVUE_GEOLOC_H


#include "Quantity.h"

class GeoLoc : public Quantity {
public:
    static GeoLoc INVALID;
    explicit GeoLoc(): Quantity(false, 0) {};

    static GeoLoc fromDegrees(double lat, double lon, uint64_t utcMs) {
        return GeoLoc(lat, lon, utcMs);
    }
    [[nodiscard]] double getLat() const { return m_dLat; }
    [[nodiscard]] double getLon() const { return m_dLon; }
    [[nodiscard]] std::string toString(uint64_t utcMs) const {
        std::stringstream ss;
        if( isValid(utcMs))
            ss << std::setprecision(7) << m_dLat << ";" << std::setprecision(7) << m_dLon << "";
        else
            ss << "";
        return ss.str();
    }
private:
    explicit GeoLoc(double lat, double lon, uint64_t utcMs):Quantity(true, utcMs){
        m_dLat = lat;
        m_dLon = lon;
    };
    double m_dLat=0;
    double m_dLon=0;

};


#endif //SAILVUE_GEOLOC_H
