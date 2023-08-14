#ifndef SAILVUE_GEOLOC_H
#define SAILVUE_GEOLOC_H


#include "Quantity.h"

class GeoLoc : public Quantity {
public:
    static GeoLoc INVALID;
    explicit GeoLoc(): Quantity(false) {};

    static GeoLoc fromDegrees(double lat, double lon) {
        return GeoLoc(lat, lon);
    }
    [[nodiscard]] double getLat() const { return m_dLat; }
    [[nodiscard]] double getLon() const { return m_dLon; }
    explicit operator std::string() const {
        std::stringstream ss;
        if( isValid())
            ss << std::setprecision(7) << m_dLat << ";" << std::setprecision(7) << m_dLon << "";
        else
            ss << "";
        return ss.str();
    }
private:
    explicit GeoLoc(double lat, double lon):Quantity(true){
        m_dLat = lat;
        m_dLon = lon;
    };
    double m_dLat=0;
    double m_dLon=0;

};


#endif //SAILVUE_GEOLOC_H
