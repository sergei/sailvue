#include "NavStats.h"

SlidingWindow::SlidingWindow(uint32_t size)
: m_maxLen(size)
{

}

void SlidingWindow::clear() {
    m_values.clear();
}

void SlidingWindow::add(double value) {
    double oldVal = 0;

    if (m_values.size() == m_maxLen) {
        oldVal = m_values[0];
        m_values.erase(m_values.begin());
    }
    m_values.push_back(value);
    m_sum += value - oldVal;
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

    // Suspected tack or gybe
    double sum = abs(m_turnsStbdPort.getSum());
    if ( m_turnsStbdPort.isFull() && sum < TURN_THR1) {
        std::pair<double, double> halves = m_turnsStbdPort.sumHalves();
        if ( abs(halves.first) > TURN_THR2 && abs(halves.second) > TURN_THR2 ) {
            bool isTack = abs(twa) < 90;
            
            // Determine the clip start and end inices so it lasts 15 seconds before event and 45 seconds after
            int eventIdx = int(epochIdx - HALF_WIN);
            auto eventUtcMs = m_InstrDataVector[eventIdx].utc.getUnixTimeMs();
            int startIdx;
            for(startIdx = eventIdx; startIdx > 0; startIdx--) {
                if (m_InstrDataVector[startIdx].utc.getUnixTimeMs() < eventUtcMs - CHAPTER_HEAD_LEN_MS) {
                    break;
                }
            }
            
            int endIdx;
            for( endIdx = eventIdx; endIdx < m_InstrDataVector.size(); endIdx++) {
                if (m_InstrDataVector[endIdx].utc.getUnixTimeMs() > eventUtcMs +  CHAPTER_TAIL_LEN_MS) {
                    break;
                }
            }
            
            m_listener.onTack(startIdx, endIdx, isTack, 0);
            reset();
        }
    }
}

