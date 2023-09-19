#ifndef SAILVUE_UTCTIME_H
#define SAILVUE_UTCTIME_H


#include <cstdint>
#include "Quantity.h"

class UtcTime : public Quantity {
public:
    static UtcTime INVALID;
    explicit UtcTime(): Quantity(false, 0) {};
    // We ignore the time stamp when check validity of UTC time itself
    [[nodiscard]] bool isValid(uint64_t ) const override { return m_bValid; }

    static UtcTime fromUnixTimeMs(uint64_t ms) {
        return UtcTime(ms);
    }
    explicit operator std::string() const {
        std::stringstream ss;
        if( isValid(m_uiUnixMilliSecs))
            ss << m_uiUnixMilliSecs;
        else
            ss << "";
        return ss.str();
    }
    [[nodiscard]] uint64_t getUnixTimeMs() const {
        return m_uiUnixMilliSecs;
    }

private:
    explicit UtcTime(uint64_t ms):Quantity(true, ms) {
        m_uiUnixMilliSecs = ms;
    }

    uint64_t m_uiUnixMilliSecs=0;

};


#endif //SAILVUE_UTCTIME_H

