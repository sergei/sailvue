#ifndef SAILVUE_STARTTIMEROVERLAYMAKER_H
#define SAILVUE_STARTTIMEROVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"

class StartTimerOverlayMaker {
public:
    StartTimerOverlayMaker(std::filesystem::path &workDir, int height, bool ignoreCache);
    void addEpoch(const std::string &fileName, uint64_t gunUtcTimeMs, InstrumentInput &instrData);
    [[nodiscard]] int getWidth() const { return m_width; }
private:
    const std::filesystem::path &m_workDir;
    const int m_height;
    const bool m_ignoreCache;
    int m_width;

private:
    QFont m_timeStampFont;
    QPen m_beforeStartPen = QPen(QColor(255, 0, 0, 220));
    QPen m_afterStartPen = QPen(QColor(0, 255, 0, 220));
};


#endif //SAILVUE_STARTTIMEROVERLAYMAKER_H
