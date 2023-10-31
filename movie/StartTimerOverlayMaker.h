#ifndef SAILVUE_STARTTIMEROVERLAYMAKER_H
#define SAILVUE_STARTTIMEROVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "OverlayElement.h"

class StartTimerOverlayMaker : public OverlayElement {
public:
    StartTimerOverlayMaker(std::vector<InstrumentInput> &instrData, int width, int height, int x, int y);
    void addEpoch(QPainter &painter, int epochIdx) override;
    void setChapter(Chapter &chapter) override;

    [[nodiscard]] int getWidth() const { return m_width; }
private:
    std::vector<InstrumentInput> &m_rInstrDataVector;
    bool m_isStart = false;
    uint64_t m_gunUtcTimeMs = 0;
    QFont m_timeStampFont;
    QPen m_beforeStartPen = QPen(QColor(255, 0, 0, 220));
    QPen m_afterStartPen = QPen(QColor(0, 255, 0, 220));
};


#endif //SAILVUE_STARTTIMEROVERLAYMAKER_H
