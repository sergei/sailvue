#ifndef SAILVUE_TARGETSOVERLAYMAKER_H
#define SAILVUE_TARGETSOVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "OverlayElement.h"

class Strip {
public:
    Strip(const std::string &label, int x, int y, int width, int height, int startIdx, int endIdx,
          double threshold, double floor, double ceiling);
    void addSample(double value, bool isValid);
    void drawBackground(QPainter &painter);
    void drawCurrent(QPainter &painter, int idx);

    void setChapter(int startIdx, int endIdx);

private:
    [[nodiscard]] QPoint toScreen(int idx, double y) const;
private:
    const std::string &m_label;
    const int m_x0;
    const int m_y0;
    const int m_width;
    const int m_height;
    const int m_startIdx;
    const int m_endIdx;
    const double m_threshold;
    const double m_floor;
    const double m_ceiling;
    int m_labelWidth;
    double m_xScale=0;
    double m_yScale=0;
    double m_maxValue = -100000;
    double m_minValue = 100000;
    int m_chapterStartIdx = 0;
    int m_chapterEndIdx = 0;

    std::vector<double> m_values;
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
    void addEpoch(QPainter &painter, int epochIdx) override;
    void setChapter(Chapter &chapter) override;

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
