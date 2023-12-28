#ifndef SAILVUE_RUDDEROVERLAYMAKER_H
#define SAILVUE_RUDDEROVERLAYMAKER_H


#include "OverlayElement.h"


class RudderOverlayMaker : public  OverlayElement{
public:
    RudderOverlayMaker(int width, int height, int x, int y);
    virtual ~RudderOverlayMaker();
    void setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) override;
    void addEpoch(QPainter &painter, const InstrumentInput &epoch) override;

private:
    [[nodiscard]] std::pair<QPoint, QPoint> toScreen(const float angle) const;
    void setHistory(const std::list<InstrumentInput> &chapterEpochs);
    void drawGrid();
    int getFontSize(const char *fontFamily, const char *text, int width, int height) const;

private:
    std::map<uint64_t, float> m_history;
    std::vector<uint64_t> m_TimeStamps;
    QImage *m_pBackgroundImage = nullptr;
    const QColor m_gridColor = QColor(255, 255, 255, 127);

    QFont m_RudderFont;
    QFont m_PilotFont;
    QPen m_RudderPen = QPen(QColor(255, 255, 0, 128));
    QPen m_RudderFontPen = QPen(QColor(255, 255, 255, 255));
    QPen m_AutoPen = QPen(QColor(255, 0, 0, 128));
    QPen m_AutoFontPen = QPen(QColor(255, 0, 0, 200));

    int m_rudderY0=0;
    int m_rudderX0=0;
    int m_textPad = 10;
    int m_maxAngleDeg = 30;
    int m_rudderBoxWidth;
    int m_rudderBoxHeight;

    QRect m_rudderTextRect;
    QRect m_pilotTextRect;

    const int m_yPad = 0;
    const int m_xPad = 0;

};


#endif //SAILVUE_RUDDEROVERLAYMAKER_H
