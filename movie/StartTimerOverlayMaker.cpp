#include <iostream>
#include "StartTimerOverlayMaker.h"

static const char *const FONT_FAMILY_TIMESTAMP = "Courier";

StartTimerOverlayMaker::StartTimerOverlayMaker(std::vector<InstrumentInput>  &instrData, int width, int height, int x, int y)
:  OverlayElement(width, height, x, y), m_rInstrDataVector(instrData){

    int fontPointSize;
    for(int fs=10; fs <100; fs ++){
        fontPointSize = fs;
        m_timeStampFont.setPointSize(fs);
        QFontMetrics fm(m_timeStampFont);
        QRect rect = fm.tightBoundingRect("00:000");
        m_width = rect.width();
        if(rect.height() > m_height){
            fontPointSize = fs - 1;
            break;
        }
    }
    m_timeStampFont.setPointSize(fontPointSize);
}

void StartTimerOverlayMaker::setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) {
    m_isStart = chapter.getChapterType() == ChapterTypes::START;
    m_gunUtcTimeMs = m_rInstrDataVector[chapter.getGunIdx()].utc.getUnixTimeMs();
}

void StartTimerOverlayMaker::addEpoch(QPainter &painter, const InstrumentInput &epoch) {
    if ( ! m_isStart ){
        return;
    }

    int64_t timeToStartSec = (int64_t(m_gunUtcTimeMs) - int64_t(epoch.utc.getUnixTimeMs())) / 1000;

    if ( timeToStartSec < 0 ){
        painter.setPen(m_afterStartPen);
        timeToStartSec = -timeToStartSec;
    } else {
        painter.setPen(m_beforeStartPen);
    }

    int64_t timeToStartMin = timeToStartSec / 60;
    int64_t timeToStartSecRem = timeToStartSec % 60;

    std::ostringstream oss;
    oss <<  std::setw(2) << std::setfill('0') << timeToStartMin << ":" << std::setw(2) << std::setfill('0') << timeToStartSecRem;

    painter.setFont(m_timeStampFont);
    painter.drawText(0, m_height, QString::fromStdString(oss.str()));

}


