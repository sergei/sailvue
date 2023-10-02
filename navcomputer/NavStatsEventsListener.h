#ifndef SAILVUE_NAVSTATSEVENTSLISTENER_H
#define SAILVUE_NAVSTATSEVENTSLISTENER_H


#include <cstdint>

class NavStatsEventsListener {
public:
    virtual void onTack(uint32_t fromIdx, uint32_t toIdx, bool isTack, double distLossMeters) = 0;
    virtual void onMarkRounding(uint32_t eventIdx, uint32_t fromIdx, uint32_t toIdx, bool isWindward) = 0;
};


#endif //SAILVUE_NAVSTATSEVENTSLISTENER_H
