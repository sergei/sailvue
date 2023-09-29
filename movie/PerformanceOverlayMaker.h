#ifndef SAILVUE_PERFORMANCEOVERLAYMAKER_H
#define SAILVUE_PERFORMANCEOVERLAYMAKER_H

#include <filesystem>
#include <QPainter>

#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "navcomputer/TimeDeltaComputer.h"

static const char *const FONT_FAMILY_TIME = "Courier";

class PerformanceOverlayMaker {
public:
    PerformanceOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrDataVector, std::filesystem::path &workDir,
                            int64_t timeDeltaBefore, int width, int height, int startIdx, int endIdx, bool ignoreCache);
    void addEpoch(const std::string &fileName, int epochIdx);
    [[nodiscard]] int64_t getTimeDeltaThisLeg() const { return m_timeDeltaThisLegMs; }

private:
    static QString formatTime(int64_t ms);
private:
    std::filesystem::path m_workDir;
    const int m_width;
    const int m_height;
    const bool m_ignoreCache;
    int64_t m_timeDeltaBeforeMs;
    int64_t m_timeDeltaThisLegMs=0;

private:
    TimeDeltaComputer m_timeDeltaComputer;

    QFont m_labelTimeFont;
    QFont m_totalTimeFont;
    QFont m_currentTimeFont;
    QPen m_timePen = QPen(QColor(255, 255, 255, 220));
    QPen m_labelPen = QPen(QColor(200, 200, 200, 220));
};


#endif //SAILVUE_PERFORMANCEOVERLAYMAKER_H
