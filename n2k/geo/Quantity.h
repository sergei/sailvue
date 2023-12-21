#ifndef SAILVUE_QUANTITY_H
#define SAILVUE_QUANTITY_H

#include <iomanip>
#include <sstream>


class Quantity {
public:
    explicit Quantity(bool isValid, uint64_t utcMs): m_bValid(isValid),m_utcMs(utcMs) {};
    [[nodiscard]] virtual bool isValid(uint64_t utcMs) const { return m_bValid && (int64_t(utcMs) - int64_t(m_utcMs)) <= MAX_AGE_MS; }
protected:
    bool m_bValid;
    static double medianAngle(std::vector<double> &sines, std::vector<double> &cosines);
    static double medianValue(std::vector<double> &vals);

private:
    int MAX_AGE_MS = 5000;
    uint64_t m_utcMs;
};


#endif //SAILVUE_QUANTITY_H
