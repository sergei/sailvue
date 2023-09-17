#include <iostream>
#include "StartTimerOverlayMaker.h"

static const char *const FONT_FAMILY_TIMESTAMP = "Courier";

StartTimerOverlayMaker::StartTimerOverlayMaker(std::filesystem::path &workDir, int height, bool ignoreCache)
: m_workDir(workDir), m_height(height), m_width(height), m_ignoreCache(ignoreCache) {
    std::filesystem::create_directories(m_workDir);

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

void StartTimerOverlayMaker::addEpoch(const std::string &fileName, uint64_t gunUtcTimeMs, InstrumentInput &instrData) {
    std::filesystem::path pngName = std::filesystem::path(m_workDir) / fileName;

    if ( std::filesystem::is_regular_file(pngName) && !m_ignoreCache){
        return;
    }
    QImage image(m_width, m_height, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));
    QPainter painter(&image);


    int64_t timeToStartSec = (int64_t(gunUtcTimeMs) - int64_t(instrData.utc.getUnixTimeMs())) / 1000;

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

    image.save(QString::fromStdString(pngName.string()), "PNG");
    std::cout << "Created " << pngName << std::endl;
}



