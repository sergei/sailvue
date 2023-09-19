#ifndef SAILVUE_QUANTITY_H
#define SAILVUE_QUANTITY_H

#include <iomanip>
#include <sstream>


class Quantity {
public:
    explicit Quantity(bool isValid, uint64_t utcMs): m_bValid(isValid),m_utcMs(utcMs) {};
    virtual bool isValid(uint64_t utcMs) const { return m_bValid && (utcMs - m_utcMs) <= MAX_AGE_MS; }
protected:
    bool m_bValid;
private:
    int MAX_AGE_MS = 5000;
    uint64_t m_utcMs;
};


#endif //SAILVUE_QUANTITY_H
