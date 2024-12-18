#ifndef SAILVUE_STARTTIMEROVERLAYMAKER_H
#define SAILVUE_STARTTIMEROVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "OverlayElement.h"
#include "ColorPalette.h"

class StartTimerOverlayMaker : public OverlayElement {
public:
    StartTimerOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrData, int width, int height, int x, int y);
    void addEpoch(QPainter &painter, const InstrumentInput &epoch) override;
    void setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) override;

    [[nodiscard]] int getWidth() const { return m_width; }
private:
    Polars &m_polars;
    Speed m_startVmg;
    std::vector<InstrumentInput> &m_rInstrDataVector;
    bool m_isStart = false;
    uint64_t m_gunUtcTimeMs = 0;
    QFont m_timeStampFont;
    QFont m_distFont;
    QPen m_distToLinePen = NOT_OCS_PEN;
    int m_timerHeight;
    int m_distHeight;


    void selectFont(QFont &font, int h, const QString &s) ;
    static QString formatSeconds(int64_t timeSec);
};


#endif //SAILVUE_STARTTIMEROVERLAYMAKER_H
