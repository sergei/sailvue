#include "PerformanceOverlayMaker.h"
#include "ColorPalette.h"

PerformanceOverlayMaker::PerformanceOverlayMaker(std::map<uint64_t, Performance> &rPerformanceMap, int width, int height, int x, int y)
 : OverlayElement(width, height, x, y),  m_height(height),
   m_rPerformanceVector(rPerformanceMap)
{
    int fontPointSize;
    QFont font = QFont(FONT_FAMILY_TIME);

    for(int fs=10; fs <100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.tightBoundingRect("(+00:00:00)");
        if(rect.width() > width || rect.height() > height / 3){
            fontPointSize = fs - 1;
            break;
        }
    }

    m_totalTimeFont = QFont(FONT_FAMILY_TIME, fontPointSize);
    m_currentTimeFont = QFont(FONT_FAMILY_TIME, fontPointSize);
    m_labelTimeFont = QFont(FONT_FAMILY_TIME, fontPointSize / 2);
}

void PerformanceOverlayMaker::addEpoch(QPainter &painter, const InstrumentInput &epoch) {

    auto utcMs = epoch.utc.getUnixTimeMs();
    if( m_rPerformanceVector[utcMs].isValid ){
        auto thisLegDeltaMs = int64_t(m_rPerformanceVector[utcMs].legTimeLostToTargetSec * 1000);
        auto thisRaceDeltaMs = int64_t(m_rPerformanceVector[utcMs].raceTimeLostToTargetSec * 1000);

        painter.setFont(m_labelTimeFont);
        painter.setPen(PERF_LABEL_PEN);
        painter.drawText(0, m_height * 5 / 8, "  entire video");

        painter.setFont(m_currentTimeFont);
        painter.setPen(thisLegDeltaMs > 0 ? SLOW_TIME_PEN : FAST_TIME_PEN);
        painter.drawText(0, m_height * 1 / 4,  " " + formatTime(thisLegDeltaMs) );
        painter.setFont(m_totalTimeFont);
        painter.setPen(thisRaceDeltaMs > 0 ? SLOW_TIME_PEN : FAST_TIME_PEN);
        painter.drawText(0, m_height * 2 / 4,  "(" + formatTime(thisRaceDeltaMs) + ")");
    }

}

QString PerformanceOverlayMaker::formatTime(int64_t ms) {

    int64_t totalSec = abs(ms) / 1000;

    int64_t hour = totalSec / 3600;
    totalSec = totalSec % 3600;
    int64_t min = totalSec / 60;
    int64_t sec = totalSec % 60;

    std::ostringstream oss;
    if( ms < 0 ) {
        oss << "-";
    }else{
        oss << "+";
    }

    if ( hour == 0 ) {
        oss << std::setw(2) << std::setfill('0') << min
        << ":" << std::setw(2) << std::setfill('0') << sec;
    }else{
        oss << std::setw(2) << std::setfill('0') << hour
        << ":" << std::setw(2) << std::setfill('0') << min
        << ":" << std::setw(2) << std::setfill('0') << sec;
    }

    return QString::fromStdString(oss.str());
}

