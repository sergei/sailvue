#include <algorithm>
#include <cmath>
#include "SurfDetector.h"

SurfDetector::SurfDetector(Polars &polars, const std::vector<InstrumentInput> &instrDataVector, SurfParams params)
        : m_rPolars(polars), m_rInstrDataVector(instrDataVector), m_params(params) {
}

double SurfDetector::getRatio(uint64_t idx, double &speedKts) {
    const InstrumentInput &ii = m_rInstrDataVector[idx];
    uint64_t ms = ii.utc.getUnixTimeMs();

    if (!ii.twa.isValid(ms) || !ii.tws.isValid(ms))
        return -1;

    double twa = ii.twa.getDegrees();
    if (std::abs(twa) < m_params.minTwaDeg)
        return -1;

    // Prefer speed through water, fall back to GPS speed
    if (ii.sow.isValid(ms)) {
        speedKts = ii.sow.getKnots();
    } else if (ii.sog.isValid(ms)) {
        speedKts = ii.sog.getKnots();
    } else {
        return -1;
    }

    double polarSpeed = m_rPolars.getSpeed(twa, ii.tws.getKnots());
    if (polarSpeed < SURF_MIN_POLAR_KTS)
        return -1;

    return speedKts / polarSpeed;
}

uint64_t SurfDetector::moveIdxByMs(uint64_t idx, int64_t ms, uint64_t startIdx, uint64_t endIdx) const {
    int64_t targetUtc = int64_t(utcMs(idx)) + ms;
    if (ms > 0) {
        while (idx + 1 < endIdx && int64_t(utcMs(idx)) < targetUtc)
            idx++;
    } else {
        while (idx > startIdx && int64_t(utcMs(idx)) > targetUtc)
            idx--;
    }
    return idx;
}

std::vector<SurfEpisode> SurfDetector::detect(uint64_t startIdx, uint64_t endIdx) {
    std::vector<SurfEpisode> episodes;
    endIdx = std::min(endIdx, uint64_t(m_rInstrDataVector.size()));
    if (startIdx >= endIdx)
        return episodes;

    // Compute raw ratios
    size_t n = endIdx - startIdx;
    std::vector<double> rawRatio(n);
    std::vector<double> speed(n, 0);
    for (size_t i = 0; i < n; i++) {
        rawRatio[i] = getRatio(startIdx + i, speed[i]);
    }

    // Time based median filter, invalid points stay invalid
    std::vector<double> ratio(n, -1);
    size_t lo = 0, hi = 0;
    std::vector<double> window;
    for (size_t i = 0; i < n; i++) {
        if (rawRatio[i] < 0)
            continue;
        uint64_t t = utcMs(startIdx + i);
        while (utcMs(startIdx + lo) + SURF_SMOOTH_MS / 2 < t)
            lo++;
        if (hi < i)
            hi = i;
        while (hi + 1 < n && utcMs(startIdx + hi + 1) <= t + SURF_SMOOTH_MS / 2)
            hi++;
        window.clear();
        for (size_t j = lo; j <= hi; j++) {
            if (rawRatio[j] >= 0)
                window.push_back(rawRatio[j]);
        }
        auto mid = window.begin() + long(window.size() / 2);
        std::nth_element(window.begin(), mid, window.end());
        ratio[i] = *mid;
    }

    // Hysteresis state machine
    bool inSurf = false;
    SurfEpisode current;
    auto closeEpisode = [&](size_t lastIdx) {
        current.endIdx = startIdx + lastIdx;
        if (utcMs(current.endIdx) - utcMs(current.startIdx) >= m_params.minDurationMs)
            episodes.push_back(current);
        inSurf = false;
    };

    for (size_t i = 0; i < n; i++) {
        double r = ratio[i];
        if (!inSurf) {
            if (r >= m_params.enterRatio) {
                inSurf = true;
                current = SurfEpisode();
                current.startIdx = startIdx + i;
                current.peakIdx = startIdx + i;
            }
        }
        if (inSurf) {
            if (r < m_params.exitRatio) {
                closeEpisode(i - 1);
                continue;
            }
            if (i > 0) {
                double dt = double(utcMs(startIdx + i) - utcMs(startIdx + i - 1)) / 1000.;
                current.score += (r - 1) * dt;
            }
            if (r > current.maxRatio)
                current.maxRatio = r;
            if (speed[i] > current.maxSpeedKts) {
                current.maxSpeedKts = speed[i];
                current.peakIdx = startIdx + i;
            }
        }
    }
    if (inSurf)
        closeEpisode(n - 1);

    // Merge episodes that are close to each other
    std::vector<SurfEpisode> merged;
    for (const auto &e: episodes) {
        if (!merged.empty() && utcMs(e.startIdx) - utcMs(merged.back().endIdx) < m_params.mergeGapMs) {
            SurfEpisode &m = merged.back();
            m.endIdx = e.endIdx;
            m.score += e.score;
            m.maxRatio = std::max(m.maxRatio, e.maxRatio);
            if (e.maxSpeedKts > m.maxSpeedKts) {
                m.maxSpeedKts = e.maxSpeedKts;
                m.peakIdx = e.peakIdx;
            }
        } else {
            merged.push_back(e);
        }
    }

    // Keep only the best ones, spread in time
    std::sort(merged.begin(), merged.end(),
              [](const SurfEpisode &a, const SurfEpisode &b) { return a.score > b.score; });
    std::vector<SurfEpisode> selected;
    for (const auto &e: merged) {
        if (selected.size() >= m_params.maxPerRace)
            break;
        bool tooClose = std::any_of(selected.begin(), selected.end(), [&](const SurfEpisode &s) {
            uint64_t a = utcMs(e.peakIdx), b = utcMs(s.peakIdx);
            return (a > b ? a - b : b - a) < m_params.minSpacingMs;
        });
        if (!tooClose)
            selected.push_back(e);
    }
    std::sort(selected.begin(), selected.end(),
              [](const SurfEpisode &a, const SurfEpisode &b) { return a.startIdx < b.startIdx; });

    // Add some context before and after
    for (auto &e: selected) {
        e.startIdx = moveIdxByMs(e.startIdx, -int64_t(m_params.preRollMs), startIdx, endIdx);
        e.endIdx = moveIdxByMs(e.endIdx, int64_t(m_params.postRollMs), startIdx, endIdx);
    }

    return selected;
}
