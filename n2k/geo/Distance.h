#ifndef DISTANCE_SPEED_H
#define DISTANCE_SPEED_H

#include "Quantity.h"

class Distance : public Quantity {
public:
    static Distance INVALID;
    explicit Distance(): Quantity(false, 0) {};

    static Distance fromMiles(double nauticalMiles, uint64_t utcMs) {
        return Distance(uint32_t(nauticalMiles * 1852.) , utcMs);
    }
    static Distance fromMeters(uint32_t meters, uint64_t utcMs) {
        return Distance(meters, utcMs);
    }
    static Distance fromMillimeters(uint32_t mm, uint64_t utcMs) {
        return Distance(mm / 1000., utcMs);
    }

    [[nodiscard]] uint32_t getMeters() const { return lround(m_dMeters); }
    [[nodiscard]] uint32_t getMillimeters() const { return lround(m_dMeters * 1000 ); }
    [[nodiscard]] std::string toString(uint64_t utcMs) const {
        std::stringstream ss;
        if( isValid(utcMs))
            ss << std::fixed << std::setprecision(3) << m_dMeters;
        else
            ss << "";
        return ss.str();
    }
private:
    explicit Distance(double meters, uint64_t utcMs): Quantity(true, utcMs){
        m_dMeters = meters;
    };
    double m_dMeters=0;
};


#endif //DISTANCE_SPEED_H
