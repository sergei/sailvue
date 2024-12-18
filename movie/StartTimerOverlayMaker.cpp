#include <iostream>
#include "StartTimerOverlayMaker.h"
#include "ColorPalette.h"


StartTimerOverlayMaker::StartTimerOverlayMaker(Polars &polars, std::vector<InstrumentInput>  &instrData, int width, int height, int x, int y)
:OverlayElement(width, height, x, y), m_polars(polars), m_rInstrDataVector(instrData){

    m_timerHeight = height / 2;
    m_distHeight = height / 4;

    selectFont(m_timeStampFont, m_timerHeight * 7 / 8, "0:00");
    selectFont(m_distFont, m_distHeight * 7 / 8, "TTK 0:00");
}

void StartTimerOverlayMaker::selectFont(QFont &font, int h, const QString &s)  {
    int fontPointSize;
    for(int fs=10; fs <100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.tightBoundingRect(s);
        if ( m_width < rect.width() ){
            m_width = rect.width();
        }
        if(rect.height() > h){
            fontPointSize = fs - 1;
            break;
        }
    }
    font.setPointSize(fontPointSize);
}

void StartTimerOverlayMaker::setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) {
    m_isStart = chapter.getChapterType() == ChapterTypes::START;
    m_gunUtcTimeMs = m_rInstrDataVector[chapter.getGunIdx()].utc.getUnixTimeMs();

    // Get median TWS and corresponding VMG for entire start chapter
    InstrumentInput median = InstrumentInput::median(m_rInstrDataVector.begin() + long(chapter.getStartIdx()),
                                                     m_rInstrDataVector.begin() + long(chapter.getEndIdx()));
    if ( median.tws.isValid(median.utc.getUnixTimeMs())){
        double tws = median.tws.getKnots();
        auto targets = m_polars.getTargets(tws, true);
        m_startVmg = Speed::fromKnots(targets.second, median.utc.getUnixTimeMs());
    }else{
        m_startVmg = Speed::INVALID;
    }
}

void StartTimerOverlayMaker::addEpoch(QPainter &painter, const InstrumentInput &epoch) {
    if ( ! m_isStart ){
        return;
    }

    int64_t timeToStartSec = (int64_t(m_gunUtcTimeMs) - int64_t(epoch.utc.getUnixTimeMs())) / 1000;
    // Distance to line
    double distanceToLineMeters = epoch.distToStart.getMeters();

    if ( timeToStartSec < 0 ){
        painter.setPen(AFTER_START_PEN);

        if (distanceToLineMeters > 0 ){  // Change the meaning of color to opposite
            m_distToLinePen = OCS_PEN;   // Over the line pen
        } else {
            m_distToLinePen = NOT_OCS_PEN;
        }

    } else {  // Not started yet
        painter.setPen(BEFORE_START_PEN);

        if (distanceToLineMeters < 0 ){
            m_distToLinePen = OCS_PEN;   // Over the line pen
        } else {
            m_distToLinePen = NOT_OCS_PEN;
        }
    }

    painter.setFont(m_timeStampFont);
    painter.drawText(0, m_timerHeight, formatSeconds(abs(timeToStartSec)));

    painter.setFont(m_distFont);
    painter.setPen(m_distToLinePen);
    double distBl = abs(distanceToLineMeters) / 10;
    painter.drawText(0, m_timerHeight + m_distHeight, "BL " + QString::number(distBl, 'f', 1));

    // Compute time to start line using VMG
    if (  timeToStartSec > 0  ){
        int64_t timeToStartLineSec = lround(distanceToLineMeters / m_startVmg.getMetersPerSec());
        int64_t timeToKill = timeToStartSec - timeToStartLineSec ;
        if( timeToKill > 0) {
            painter.drawText(0, m_timerHeight + m_distHeight * 2, "TTK: " + formatSeconds(timeToKill));
        }else{
            painter.drawText(0, m_timerHeight + m_distHeight * 2, "late !");
        }
    }

}

QString StartTimerOverlayMaker::formatSeconds(int64_t timeSec){
    int64_t timeToStartMin = timeSec / 60;
    int64_t timeToStartSecRem = timeSec % 60;

    std::ostringstream oss;
    oss <<  std::setw(1) << std::setfill('0') << timeToStartMin << ":" << std::setw(2) << std::setfill('0') << timeToStartSecRem;

    return QString::fromStdString(oss.str());
}
