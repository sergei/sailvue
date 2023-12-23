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

    [[nodiscard]] double getKnots() const { return m_ulMeters; }
    [[nodiscard]] std::string toString(uint64_t utcMs) const {
        std::stringstream ss;
        if( isValid(utcMs))
            ss << m_ulMeters;
        else
            ss << "";
        return ss.str();
    }
private:
    explicit Distance(uint32_t meters, uint64_t utcMs): Quantity(true, utcMs){
        m_ulMeters = meters;
    };
    uint32_t m_ulMeters=0;
};


#endif //DISTANCE_SPEED_H
