#ifndef SAILVUE_SPEED_H
#define SAILVUE_SPEED_H


#include "Quantity.h"

class Speed : public Quantity {
public:
    static Speed INVALID;
    explicit Speed(): Quantity(false, 0) {};

    static Speed fromMetersPerSecond(double metersPerSecond,  uint64_t utcMs) {
        return Speed(metersPerSecond  / 1852. * 3600., utcMs);
    }
    static Speed fromKnots(double knots, uint64_t utcMs) {
        return Speed(knots, utcMs);
    }
    static Speed median(const std::list<Speed> &values, uint64_t utcMs);

    [[nodiscard]] double getKnots() const { return m_dKnots; }
    [[nodiscard]] std::string toString(uint64_t utcMs) const {
        std::stringstream ss;
        if( isValid(utcMs))
            ss << std::setprecision(3) << m_dKnots;
        else
            ss << "";
        return ss.str();
    }
private:
    explicit Speed(double knots,  uint64_t utcMs):Quantity(true, utcMs){
        m_dKnots = knots;
    };
    double m_dKnots=0;
};


#endif //SAILVUE_SPEED_H
