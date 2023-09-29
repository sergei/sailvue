#ifndef SAILVUE_TIMEDELTACOMPUTER_H
#define SAILVUE_TIMEDELTACOMPUTER_H

#include "InstrumentInput.h"
#include "Polars.h"

enum LegType{
    LEG_TYPE_UNKNOWN,
    LEG_TYPE_UPWIND,
    LEG_TYPE_DOWNWIND,
    LEG_TYPE_REACH
};

class TimeDeltaComputer {
public:
    explicit TimeDeltaComputer(Polars &polars, std::vector<InstrumentInput> &rInstrDataVector, u_int64_t startIdx, u_int64_t endIdx);
    int64_t getAccDeltaMs(u_int64_t idx);  // Positive if we are ahead
private:
    Polars &m_polars;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    LegType m_legType = LEG_TYPE_UNKNOWN;
    double m_accumulateTimeToTarget = 0;  // Positive if we are ahead
    u_int64_t m_prevTimeStamp = 0;
};


#endif //SAILVUE_TIMEDELTACOMPUTER_H
