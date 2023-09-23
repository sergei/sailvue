
#ifndef SAILVUE_POLAROVERLAYMAKER_H
#define SAILVUE_POLAROVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"

class PolarOverlayMaker {
public:
    virtual ~PolarOverlayMaker();

    PolarOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrDataVector, std::filesystem::path &workDir, int width,
                      int startIdx, int endIdx,
                      bool ignoreCache);
    void addEpoch(const std::string &fileName, int epochIdx);
private:
    [[nodiscard]] QPoint toScreen(const std::pair<float, float> &xy) const;
    void setHistory(int startIdx, int endIdx);
private:
    std::vector<InstrumentInput> &m_rInstrDataVector;
    std::filesystem::path m_workDir;
    const int m_width;
    const bool m_ignoreCache;
    const int m_dotRadius = 10;
    const int m_xPad = m_dotRadius;
    const int m_yPad = m_dotRadius;
    const QColor m_polarGridColor = QColor(255, 255, 255, 127);

    int m_height;
    int m_maxSpeedKts = 0;
    int m_minSpeedKts = 100;
    const int m_speedStep = 2;

    double m_minTwa = 20;
    double m_maxTwa = -1;
    bool m_showTopHalf = false;
    bool m_showBottomHalf = false;

    int m_y0;

    float m_xScale;
    float m_yScale;

    const int m_startIdx;

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
