#ifndef SAILVUE_SPEED_H
#define SAILVUE_SPEED_H


#include "Quantity.h"

class Speed : public Quantity {
public:
    static Speed INVALID;
    explicit Speed(): Quantity(false) {};

    static Speed fromMetersPerSecond(double metersPerSecond) {
        return Speed(metersPerSecond  / 1852. * 3600.);
    }
    static Speed fromKnots(double knots) {
        return Speed(knots);
    }
    [[nodiscard]] double getKnots() const { return m_dKnots; }
    explicit operator std::string() const {
        std::stringstream ss;
        if( isValid())
            ss << std::setprecision(3) << m_dKnots;
        else
            ss << "";
        return ss.str();
    }
private:
    explicit Speed(double knots):Quantity(true){
        m_dKnots = knots;
    };
    double m_dKnots=0;
};


#endif //SAILVUE_SPEED_H
