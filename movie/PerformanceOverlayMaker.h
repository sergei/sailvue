#ifndef SAILVUE_PERFORMANCEOVERLAYMAKER_H
#define SAILVUE_PERFORMANCEOVERLAYMAKER_H

#include <filesystem>
#include <QPainter>

#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "navcomputer/TimeDeltaComputer.h"
#include "OverlayElement.h"

class PerformanceOverlayMaker : public OverlayElement {
public:
    PerformanceOverlayMaker(std::map<uint64_t, Performance> &rPerformanceMap, int width, int height, int x, int y);
    void setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs)  override{
        m_chapterType = chapter.getChapterType();
    }
    void addEpoch(QPainter &painter, const InstrumentInput &epoch) override;

private:
    static QString formatTime(int64_t ms);
private:
    const int m_height;
    ChapterTypes::ChapterType m_chapterType = ChapterTypes::TACK_GYBE;
    std::map<uint64_t, Performance> &m_rPerformanceVector;
private:
    QFont m_labelTimeFont;
    QFont m_totalTimeFont;
    QFont m_currentTimeFont;
};


#endif //SAILVUE_PERFORMANCEOVERLAYMAKER_H
