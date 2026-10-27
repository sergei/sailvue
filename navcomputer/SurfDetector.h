#ifndef SAILVUE_SURFDETECTOR_H
#define SAILVUE_SURFDETECTOR_H

#include <vector>
#include "InstrumentInput.h"
#include "Polars.h"

static const double SURF_MIN_POLAR_KTS = 0.5;    // Ignore points where polar speed is meaningless
static const uint64_t SURF_SMOOTH_MS = 1000;     // Median filter window

struct SurfParams {
    double minTwaDeg = 110;          // Only look at downwind sailing
    double enterRatio = 1.4;         // Speed/polar ratio to start the surf episode
    double exitRatio = 1.1;          // Speed/polar ratio to end the surf episode
    uint64_t minDurationMs = 4000;
    uint64_t mergeGapMs = 10000;     // Merge surfs closer than this
    size_t maxPerRace = 10;
    uint64_t minSpacingMs = 120000;  // Minimum time between peaks of selected surfs
    uint64_t preRollMs = 5000;       // Show the wave building up
    uint64_t postRollMs = 3000;
};

struct SurfEpisode {
    uint64_t startIdx = 0;
    uint64_t endIdx = 0;
    uint64_t peakIdx = 0;
    double maxSpeedKts = 0;
    double maxRatio = 0;
    double score = 0;  // Seconds of excess speed over polar, i.e. sum of (ratio - 1) * dt
};

class SurfDetector {
public:
    SurfDetector(Polars &polars, const std::vector<InstrumentInput> &instrDataVector, SurfParams params = SurfParams());
    // Returns the best surf episodes within [startIdx, endIdx) sorted by time
    std::vector<SurfEpisode> detect(uint64_t startIdx, uint64_t endIdx);

private:
    // Returns speed to polar ratio or negative value if not applicable
    double getRatio(uint64_t idx, double &speedKts);
    uint64_t utcMs(uint64_t idx) const { return m_rInstrDataVector[idx].utc.getUnixTimeMs(); }
    uint64_t moveIdxByMs(uint64_t idx, int64_t ms, uint64_t startIdx, uint64_t endIdx) const;

private:
    Polars &m_rPolars;
    const std::vector<InstrumentInput> &m_rInstrDataVector;
    SurfParams m_params;
};


#endif //SAILVUE_SURFDETECTOR_H
