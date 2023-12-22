#ifndef SAILVUE_PERFORMANCEOVERLAYMAKER_H
#define SAILVUE_PERFORMANCEOVERLAYMAKER_H

#include <filesystem>
#include <QPainter>

#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "navcomputer/TimeDeltaComputer.h"
#include "OverlayElement.h"

static const char *const FONT_FAMILY_TIME = "Courier";

class PerformanceOverlayMaker : public OverlayElement {
public:
    PerformanceOverlayMaker(std::map<uint64_t, Performance> &rPerformanceMap, int width, int height, int x, int y);
    void addEpoch(QPainter &painter, const InstrumentInput &epoch) override;

private:
    static QString formatTime(int64_t ms);
private:
    const int m_height;

    std::map<uint64_t, Performance> &m_rPerformanceVector;

private:
    QFont m_labelTimeFont;
    QFont m_totalTimeFont;
    QFont m_currentTimeFont;
    QPen m_timePen = QPen(QColor(255, 255, 255, 220));
    QPen m_labelPen = QPen(QColor(200, 200, 200, 220));
};


#endif //SAILVUE_PERFORMANCEOVERLAYMAKER_H
