#ifndef SAILVUE_ANGLE_H
#define SAILVUE_ANGLE_H


#include <cmath>
#include "Quantity.h"

class Angle : public Quantity {
public:
    static Angle INVALID;
    explicit Angle(): Quantity(false) {};

    static Angle fromRadians(double radians) {
        return Angle(radians * 180.0 / M_PI);
    }
    static Angle fromDegrees(double degrees) {
        return Angle(degrees);
    }
    [[nodiscard]] double getDegrees() const { return m_dDegrees; }
    explicit operator std::string() const {
        std::stringstream ss;
        if( isValid())
            ss << std::setprecision(3) << m_dDegrees << "°";
        else
            ss << "--.--°";
        return ss.str();
    }
private:
    explicit Angle(double degrees):Quantity(true){
        if( degrees > 180 ) {
            m_dDegrees = degrees - 360;
        }else{
            m_dDegrees = degrees;
        }
    };
    double m_dDegrees=0;
};


#endif //SAILVUE_ANGLE_H
