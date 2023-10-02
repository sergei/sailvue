#include <iostream>
#include "NavStats.h"

SlidingWindow::SlidingWindow(uint32_t size)
: m_maxLen(size)
{

}

void SlidingWindow::clear() {
    m_values.clear();
    m_sum = 0;
}

void SlidingWindow::add(double value) {
    double oldVal = 0;

    if (m_values.size() == m_maxLen) {
        oldVal = m_values[0];
        m_values.erase(m_values.begin());
    }
    m_values.push_back(value);
    m_sum -= oldVal;
    m_sum += value;
    if ( m_sum > m_maxLen){
        std::cout << "SlidingWindow::add: m_sum > m_maxLen" << std::endl;
    }
}

std::pair<double, double> SlidingWindow::sumHalves(int splitIdx) const {
    if ( splitIdx == -1 ) {
        splitIdx = int(m_values.size()) / 2;
    }
    double sumBefore = 0;
    double sumAfter = 0;
    for( int i = 0; i < m_values.size(); i++){
        if ( i < splitIdx ) {
            sumBefore += m_values[i];
        } else {
            sumAfter += m_values[i];
        }
    }
    return {sumBefore, sumAfter};
}

NavStats::NavStats(std::vector<InstrumentInput> &rInstrDataVector, NavStatsEventsListener &listener)
        : m_InstrDataVector(rInstrDataVector), m_listener(listener)
{

}

void NavStats::reset() {
    m_turnsUpDown.clear();
    m_turnsStbdPort.clear();
}

void NavStats::update(uint64_t epochIdx, InstrumentInput &ii) {

    if ( !ii.twa.isValid(ii.utc.getUnixTimeMs()) ) {
        return;
    }

    // Update sliding windows
    double twa = ii.twa.getDegrees();
    double upDown = 0;
    if (abs(twa) < 70)
        upDown = 1;
    else if (abs(twa) > 110)
        upDown = -1;

    m_turnsUpDown.add(upDown);

    double stbdPort = 0;
    if (twa > 0)
        stbdPort = 1;
    else if (twa < 0)
        stbdPort = -1;

    // Not so sure if port or starboard
    if( abs(twa) < 5 || abs(twa) > 175 )
        stbdPort = 0;
    m_turnsStbdPort.add(stbdPort);

    // Analyze sliding windows
    // Suspected mark rounding
    if( m_turnsUpDown.isFull() && abs(m_turnsUpDown.getSum()) < TURN_THR1){
        std::pair<double, double> halves = m_turnsUpDown.sumHalves();
        if ( abs(halves.first) > TURN_THR1 && abs(halves.second) > TURN_THR2 ) {
            bool isWindward = halves.second < 0;

            int eventIdx = int(epochIdx - HALF_WIN);
            int startIdx = findStartIdx(eventIdx, MARK_HEAD_LEN_MS);
            int endIdx = findEndIdx(eventIdx, MARK_TAIL_LEN_MS);

            m_listener.onMarkRounding(eventIdx, startIdx, endIdx, isWindward);
            reset();
        }
    }

    // Suspected tack or gybe
    double sum = abs(m_turnsStbdPort.getSum());
    if ( m_turnsStbdPort.isFull() && sum < TURN_THR1) {
        std::pair<double, double> halves = m_turnsStbdPort.sumHalves();
        if ( abs(halves.first) > TURN_THR1 && abs(halves.second) > TURN_THR2 ) {

            // Check if we are still sailing up or downwind
            std::pair<double, double> upDownHalves = m_turnsUpDown.sumHalves();
            if ( upDownHalves.first * upDownHalves.second > 0 ){
                bool isTack = abs(twa) < 90;

                int eventIdx = int(epochIdx - HALF_WIN);
                int startIdx = findStartIdx(eventIdx, TACK_HEAD_LEN_MS);
                int endIdx = findEndIdx(eventIdx, TACK_TAIL_LEN_MS);

                m_listener.onTack(startIdx, endIdx, isTack, 0);
                reset();
            }
        }
    }
}

int NavStats::findStartIdx(int eventIdx, int durationMs) {
    uint64_t eventUtcMs= m_InstrDataVector[eventIdx].utc.getUnixTimeMs();
    int startIdx;
    for(startIdx = eventIdx; startIdx > 0; startIdx--) {
        if (m_InstrDataVector[startIdx].utc.getUnixTimeMs() < eventUtcMs - durationMs) {
            break;
        }
    }
    return startIdx;
}

int NavStats::findEndIdx(int eventIdx, int durationMs) {
    uint64_t eventUtcMs= m_InstrDataVector[eventIdx].utc.getUnixTimeMs();
    int endIdx;
    for( endIdx = eventIdx; endIdx < m_InstrDataVector.size(); endIdx++) {
        if (m_InstrDataVector[endIdx].utc.getUnixTimeMs() > eventUtcMs + durationMs) {
            break;
        }
    }
    return endIdx;
}

