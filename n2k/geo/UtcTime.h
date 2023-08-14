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
        struct tm *tm;
        auto t  = time_t(m_uiUnixMilliSecs / 1000);
        tm = gmtime(&t);
        char dateStr[64];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", tm);
        uint32_t ms = m_uiUnixMilliSecs % 1000;
        sprintf(dateStr, "%s.%03d", dateStr, ms);
        return dateStr;
    }

private:
    explicit UtcTime(uint64_t ms):Quantity(true) {
        m_uiUnixMilliSecs = ms;
    }

    uint64_t m_uiUnixMilliSecs=0;

};


#endif //SAILVUE_UTCTIME_H

