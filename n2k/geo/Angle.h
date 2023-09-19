#ifndef SAILVUE_ANGLE_H
#define SAILVUE_ANGLE_H


#include <cmath>
#include "Quantity.h"

class Angle : public Quantity {
public:
    static Angle INVALID;
    explicit Angle(): Quantity(false,0) {};

    static Angle fromRadians(double radians, uint64_t utcMs) {
        return Angle(radians * 180.0 / M_PI, utcMs);
    }
    static Angle fromDegrees(double degrees, uint64_t utcMs) {
        return Angle(degrees, utcMs);
    }
    [[nodiscard]] double getDegrees() const { return m_dDegrees; }
    [[nodiscard]] std::string toString(uint64_t utcMs) const {
        std::stringstream ss;
        if( isValid(utcMs) )
            ss << std::setprecision(3) << m_dDegrees;
        else
            ss << "";
        return ss.str();
    }
private:
    explicit Angle(double degrees, uint64_t utcMs):Quantity(true, utcMs){
        if( degrees > 180 ) {
            m_dDegrees = degrees - 360;
        }else{
            m_dDegrees = degrees;
        }
    };
    double m_dDegrees=0;
};


#endif //SAILVUE_ANGLE_H
