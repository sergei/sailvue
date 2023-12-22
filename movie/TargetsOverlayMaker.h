#ifndef SAILVUE_TARGETSOVERLAYMAKER_H
#define SAILVUE_TARGETSOVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "OverlayElement.h"

class Strip {
public:
    Strip(const std::string &label, int x, int y, int width, int height, uint64_t startTime, uint64_t endTime,
          double threshold, double floor, double ceiling);
    void addSample(uint64_t t, double value, bool isValid);
    void drawBackground(QPainter &painter);
    void drawCurrent(QPainter &painter, const InstrumentInput &epoch);

    void setChapter(uint64_t startTime, uint64_t endTime);

private:
    [[nodiscard]] QPoint toScreen(uint64_t idx, double y) const;
private:
    const std::string &m_label;
    const int m_x0;
    const int m_y0;
    const int m_width;
    const int m_height;
    const uint64_t m_startTime;
    const uint64_t m_endTime;
    const double m_threshold;
    const double m_floor;
    const double m_ceiling;
    int m_labelWidth;
    double m_xScale=0;
    double m_yScale=0;
    double m_maxValue = -100000;
    double m_minValue = 100000;
    uint64_t m_chapterStartTime = 0;
    uint64_t m_chapterEndTime = 0;

    std::map<uint64_t,double> m_values;
    QFont m_labelFont;
    QPen m_labelPen = QPen(QColor(255, 255, 255, 220));
    QPen m_axisPen = QPen(QColor(200, 200, 200, 220));
    QPen m_goodPen = QPen(QColor(0, 255, 0, 220));
    QPen m_okPen = QPen(QColor(255, 255, 0, 220));
    QPen m_badPen = QPen(QColor(255, 0, 0, 220));
    QPen m_tickPen = QPen(QColor(255, 255, 255, 255));

    QBrush m_outOfChapterBrush = QBrush(QColor(0, 0, 0, 128));
    QPen m_outOfChapterPen = QPen(QColor(0, 0, 0, 0));
};

class TargetsOverlayMaker : public  OverlayElement {
public:
    TargetsOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrDataVector, int width, int height, int x, int y,
                        int startIdx, int endIdx);
    void addEpoch(QPainter &painter, const InstrumentInput &epoch) override;
    void setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) override;

    virtual ~TargetsOverlayMaker();

private:
    void makeBaseImage(int startIdx, int endIdx);

    std::vector<InstrumentInput> &m_rInstrDataVector;
    Polars &m_polars;

    QImage *m_pBackgroundImage = nullptr;
    QImage *m_pHighLightImage = nullptr;

    Strip m_speedStrip;
    Strip m_vmgStrip;
};


#endif //SAILVUE_TARGETSOVERLAYMAKER_H
