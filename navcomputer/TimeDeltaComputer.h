#ifndef SAILVUE_TIMEDELTACOMPUTER_H
#define SAILVUE_TIMEDELTACOMPUTER_H

#include "InstrumentInput.h"
#include "Polars.h"
#include "Performance.h"

enum LegType{
    LEG_TYPE_UNKNOWN,
    LEG_TYPE_UPWIND,
    LEG_TYPE_DOWNWIND,
};

class TimeDeltaComputer {
public:
    TimeDeltaComputer(Polars &polars, std::vector<InstrumentInput> &instrData);
    void startLeg();
    void updatePerformance(uint64_t idx, Performance &performance, bool isFetch);

private:
    Polars &m_polars;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    LegType m_legType = LEG_TYPE_UNKNOWN;
    double m_accumulateDistToTargetMeters = 0;  // Positive if we are ahead
    double m_accumulateTimeToTarget = 0;  // Positive if we are ahead
    u_int64_t m_prevTimeStamp = 0;
};


#endif //SAILVUE_TIMEDELTACOMPUTER_H
