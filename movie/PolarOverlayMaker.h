
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
    void addEpoch(QPainter &painter, int epochIdx) override;
    void setChapter(Chapter &chapter) override;

private:
    [[nodiscard]] QPoint toScreen(const std::pair<float, float> &xy) const;
    void setHistory(int startIdx, int endIdx);
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

    int m_startIdx=0;

    std::vector<std::pair<float, float>> m_history;

    static std::pair<float, float> polToCart(float kts, float rad);
    Polars &m_polars;

    QImage *m_pBackgroundImage = nullptr;
    QImage *m_PolarCurveImage = nullptr;
    QPoint m_origin;

    QPoint drawGrid();

    void drawPolarCurve(float tws);
};


#endif //SAILVUE_POLAROVERLAYMAKER_H
