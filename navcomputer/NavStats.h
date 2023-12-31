#ifndef SAILVUE_NAVSTATS_H
#define SAILVUE_NAVSTATS_H

#include <vector>
#include "NavStatsEventsListener.h"
#include "InstrumentInput.h"

static const int WIN_LEN = 600;
static const int HALF_WIN = WIN_LEN / 2;
static const int TURN_THR1 = WIN_LEN / 10;  // Threshold to detect roundings and tacks
static const int TURN_THR2 = WIN_LEN / 4;   // Threshold to detect roundings and tacks

static const int TACK_HEAD_LEN_MS = 15000;
static const int TACK_TAIL_LEN_MS = 45000;

static const int MARK_HEAD_LEN_MS = 30000;
static const int MARK_TAIL_LEN_MS = 30000;

class SlidingWindow {
public:
    explicit SlidingWindow(uint32_t size);

    void clear();
    void add(double value);
    [[nodiscard]] uint32_t getCount() const{ return m_values.size(); };
    [[nodiscard]] bool isFull() const { return m_values.size() == m_maxLen; };

    [[nodiscard]] double getAvg() const { return m_sum / double(m_values.size()); }
    [[nodiscard]] double getSum() const { return m_sum; }
    [[nodiscard]] std::pair<double, double> sumHalves(int splitIdx = -1) const;
private:
    const uint32_t m_maxLen;
    double m_sum = 0;
    std::vector<double> m_values;
};

class NavStats {
public:
    explicit NavStats(std::vector<InstrumentInput> &rInstrDataVector, NavStatsEventsListener &listener);
    void update(uint64_t epochIdx, InstrumentInput &ii);
    void reset();
private:
    NavStatsEventsListener &m_listener;
    SlidingWindow m_turnsUpDown{WIN_LEN};
    SlidingWindow m_turnsStbdPort{WIN_LEN};
    std::vector<InstrumentInput> &m_InstrDataVector;

    int findStartIdx(int eventIdx, int durationMs);
    int findEndIdx(int eventIdx,int durationMs);
};

#endif //SAILVUE_NAVSTATS_H
