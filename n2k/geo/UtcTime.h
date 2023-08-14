#ifndef SAILVUE_UTCTIME_H
#define SAILVUE_UTCTIME_H


#include <cstdint>
#include "Quantity.h"

class UtcTime : public Quantity {
public:
    static UtcTime INVALID;
    explicit UtcTime(): Quantity(false) {};

    static UtcTime fromUnixTimeMs(uint64_t ms) {
        return UtcTime(ms);
    }
    explicit operator std::string() const {
        std::stringstream ss;
        if( isValid())
            ss << m_uiUnixMilliSecs;
        else
            ss << "";
        return ss.str();
    }
    [[nodiscard]] uint64_t getUnixTimeMs() const {
        return m_uiUnixMilliSecs;
    }

private:
    explicit UtcTime(uint64_t ms):Quantity(true) {
        m_uiUnixMilliSecs = ms;
    }

    uint64_t m_uiUnixMilliSecs=0;

};


#endif //SAILVUE_UTCTIME_H

