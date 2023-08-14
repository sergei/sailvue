#ifndef SAILVUE_QUANTITY_H
#define SAILVUE_QUANTITY_H

#include <iomanip>
#include <sstream>


class Quantity {
public:
    explicit Quantity(bool isValid): m_bValid(isValid) {};
    [[nodiscard]] bool isValid() const { return m_bValid; }
private:
    bool m_bValid;
};


#endif //SAILVUE_QUANTITY_H
