
#ifndef SAILVUE_POLAROVERLAYMAKER_H
#define SAILVUE_POLAROVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "OverlayElement.h"

static const int HIST_DISPLAY_LEN_MS = 1000 * 30;

class PolarOverlayMaker : public  OverlayElement{
public:
    PolarOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrDataVector, int width, int height, int x, int y);
    virtual ~PolarOverlayMaker();
    void addEpoch(QPainter &painter, const InstrumentInput &epoch) override;
    void setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) override;

private:
    [[nodiscard]] QPoint toScreen(const std::pair<float, float> &xy) const;
    static std::pair<float, float> polToCart(float kts, float rad);
    void setHistory(const std::list<InstrumentInput> &chapterEpochs);
    QPoint drawGrid();
    void drawPolarCurve(float tws);
private:
    std::vector<InstrumentInput> &m_rInstrDataVector;
    const int m_dotRadius = 10;
    const int m_xPad = m_dotRadius;
    const int m_yPad = m_dotRadius;
    const QColor m_polarGridColor = QColor(255, 255, 255, 127);

    int m_maxSpeedKts = 0;
    int m_minSpeedKts = 100;
    const int m_speedStep = 2;

    double m_minTwa = 20;
    double m_maxTwa = -1;
    bool m_showTopHalf = false;
    bool m_showBottomHalf = false;

    int m_y0=0;

    float m_xScale=1;
    float m_yScale=1;

    std::map<uint64_t, std::pair<float, float>> m_history;
    std::vector<uint64_t> m_TimeStamps;

    Polars &m_polars;

    QImage *m_pBackgroundImage = nullptr;
    QImage *m_PolarCurveImage = nullptr;
    QPoint m_origin;

};


#endif //SAILVUE_POLAROVERLAYMAKER_H
