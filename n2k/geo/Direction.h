#ifndef SAILVUE_DIRECTION_H
#define SAILVUE_DIRECTION_H


#include <cmath>
#include "Quantity.h"

class Direction : public Quantity {
public:
    static Direction INVALID;
    explicit Direction(): Quantity(false) {};

    static Direction fromRadians(double radians) {
        return Direction(radians * 180.0 / M_PI);
    }
    static Direction fromDegrees(double degrees) {
        return Direction(degrees);
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
    explicit Direction(double degrees):Quantity(true){
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
