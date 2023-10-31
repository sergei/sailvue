#include "PerformanceOverlayMaker.h"

PerformanceOverlayMaker::PerformanceOverlayMaker(std::vector<Performance> &rPerformanceVector, int width, int height, int x, int y)
 : OverlayElement(width, height, x, y),  m_height(height),
   m_rPerformanceVector(rPerformanceVector)
{
    int fontPointSize;
    QFont font = QFont(FONT_FAMILY_TIME);

    for(int fs=10; fs <100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.tightBoundingRect("-00:00:00");
        if(rect.width() > width || rect.height() > height / 3){
            fontPointSize = fs - 1;
            break;
        }
    }

    m_totalTimeFont = QFont(FONT_FAMILY_TIME, fontPointSize);
    m_currentTimeFont = QFont(FONT_FAMILY_TIME, fontPointSize);
    m_labelTimeFont = QFont(FONT_FAMILY_TIME, fontPointSize / 2);
}

void PerformanceOverlayMaker::addEpoch(QPainter &painter, int epochIdx) {


    if( m_rPerformanceVector[epochIdx].isValid ){
        auto thisLegDeltaMs = int64_t(m_rPerformanceVector[epochIdx].legTimeLostToTargetSec * 1000);
        auto thisRaceDeltaMs = int64_t(m_rPerformanceVector[epochIdx].raceTimeLostToTargetSec * 1000);

        painter.setFont(m_labelTimeFont);
        painter.setPen(m_labelPen);
        painter.drawText(0, m_height / 4, "This chapter");
        painter.drawText(0, m_height * 3 / 4, "Entire race");

        painter.setFont(m_currentTimeFont);
        painter.setPen(m_timePen);
        painter.drawText(0, m_height / 2,  formatTime(thisLegDeltaMs) );
        painter.setFont(m_totalTimeFont);
        painter.drawText(0, m_height ,  formatTime(thisRaceDeltaMs) );
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

