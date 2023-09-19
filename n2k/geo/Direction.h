#ifndef SAILVUE_DIRECTION_H
#define SAILVUE_DIRECTION_H


#include <cmath>
#include "Quantity.h"

class Direction : public Quantity {
public:
    static Direction INVALID;
    explicit Direction(): Quantity(false, 0) {};

    static Direction fromRadians(double radians, uint64_t utcMs) {
        return Direction(radians * 180.0 / M_PI, utcMs);
    }
    static Direction fromDegrees(double degrees, uint64_t utcMs) {
        return Direction(degrees, utcMs);
    }
    [[nodiscard]] double getDegrees() const { return m_dDegrees; }
    [[nodiscard]] std::string toString( uint64_t utcMs) const {
        std::stringstream ss;
        if( isValid(utcMs))
            ss << std::setprecision(3) << m_dDegrees;
        else
            ss << "";
        return ss.str();
    }
private:
    explicit Direction(double degrees, uint64_t utcMs):Quantity(true, utcMs){
        if( degrees > 360 ) {
            m_dDegrees = degrees - 360;
        }else if( degrees < 0 ) {
            m_dDegrees = degrees + 360;
        }else{
            m_dDegrees = degrees;
        }
    };
    double m_dDegrees=0;
};


#endif //SAILVUE_DIRECTION_H
