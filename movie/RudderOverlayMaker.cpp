#include <cmath>
#include <iostream>

#include "RudderOverlayMaker.h"
#include "ColorPalette.h"


RudderOverlayMaker::RudderOverlayMaker(int width, int height, int x, int y)
:OverlayElement(width, height, x, y)
{
    int rudderTextBoxWidth = width / 4;
    int rudderTextBoxHeight = 50;
    int rudderTextY0 = 50;
    int rudderTextX0 = (width - rudderTextBoxWidth) / 2;

    int pilotTextBoxWidth = width;
    int pilotTextBoxHeight = 50;

    m_rudderBoxWidth = width;
    m_rudderBoxHeight = height - pilotTextBoxHeight;

    int m_pilotTextY0 = m_rudderBoxHeight + 5;
    int m_pilotTextX0 = 0;

    m_rudderTextRect = QRect(rudderTextX0, rudderTextY0, rudderTextBoxWidth, rudderTextBoxHeight);
    m_pilotTextRect = QRect(m_pilotTextX0, m_pilotTextY0, pilotTextBoxWidth, pilotTextBoxHeight);

    m_RudderFont = QFont(FONT_FAMILY, getFontSize(FONT_FAMILY, "◄33º", rudderTextBoxWidth, rudderTextBoxHeight));
    m_PilotFont = QFont(FONT_FAMILY, getFontSize(FONT_FAMILY, "PILOT 33º►", pilotTextBoxWidth, pilotTextBoxHeight));
}

int RudderOverlayMaker::getFontSize(const char *fontFamily, const char *text, int width, int height) const {
    int fontPointSize;
    QFont font = QFont(fontFamily);
    for(int fs=1; fs < 100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.boundingRect(text);
        if(rect.height() > height || rect.width() > width){
            fontPointSize = fs - 4;
            break;
        }
    }
    return fontPointSize;
}

RudderOverlayMaker::~RudderOverlayMaker() {
    delete m_pBackgroundImage;
}

void RudderOverlayMaker::setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) {
    delete m_pBackgroundImage;
    m_pBackgroundImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_pBackgroundImage->fill(QColor(0, 0, 0, 0));
    setHistory(chapterEpochs);
    drawGrid();
}

void RudderOverlayMaker::setHistory(const std::list<InstrumentInput> &chapterEpochs) {
    m_history.clear();
    m_TimeStamps.clear();

    float fMaxAngleRad = -7;

    for (auto &instrData: chapterEpochs) {
        auto utcMs = instrData.utc.getUnixTimeMs();
        if (instrData.rdr.isValid(utcMs)) {
            auto fAngleRad = float(instrData.rdr.getRadians());
            fMaxAngleRad = std::max(abs(fAngleRad), fMaxAngleRad);
            m_history[utcMs] = instrData.rdr;
            m_TimeStamps.push_back(utcMs);
        }
    }

    m_maxAngleDeg = lround(fMaxAngleRad * 180 / M_PI * 2) / 2;
    if (m_maxAngleDeg > 44) {
        m_maxAngleDeg = 44;
    }

    if (m_maxAngleDeg < 20) {
        m_maxAngleDeg = 20;
    }

    fMaxAngleRad = float(m_maxAngleDeg) * M_PI / 180;


    // Compute top of the triangle, so it's base is the bottom part of the screen
    m_rudderX0 = m_rudderBoxWidth / 2;
    m_rudderY0 = m_rudderBoxHeight - int(float(m_rudderBoxWidth) / (2.f * tan(fMaxAngleRad)));

}

std::pair<QPoint, QPoint> RudderOverlayMaker::toScreen(const float angleRad) const {
    float tg = tan(angleRad);

    float x = float(m_rudderBoxWidth) / 2 - float(m_rudderY0) * tg;
    QPoint top(int(x), 0);

    x = float(m_rudderBoxWidth) / 2 + float(m_rudderBoxHeight - m_rudderY0) * sin(angleRad);
    float y = float(m_rudderBoxHeight - m_rudderY0) * cos(angleRad) + float(m_rudderY0);
    QPoint bottom((int)x, (int)y);

    return {top, bottom};
}

void RudderOverlayMaker::drawGrid() {
    QPainter painter(m_pBackgroundImage);

    painter.setPen(RUDDER_GRID_PEN);

    // Angle lines
    for( int angle = - m_maxAngleDeg; angle <= m_maxAngleDeg; angle += 2 ){
        auto tick = toScreen(angle * M_PI / 180);
        painter.drawLine(tick.first, tick.second);
    }

    // Erase  angle rectangle on to of them
    //    painter.setPen(m_RudderPen);
    painter.save();
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.eraseRect(m_rudderTextRect);
    painter.restore();

    painter.drawRect(m_rudderTextRect);
}


void RudderOverlayMaker::addEpoch(QPainter &painter, const InstrumentInput &epoch) {
    if ( m_pBackgroundImage == nullptr ){
        return;
    }

    if( !epoch.rdr.isValid(epoch.utc.getUnixTimeMs()) && !epoch.cmdRdr.isValid(epoch.utc.getUnixTimeMs())){
        return;
    }

    QImage image = m_pBackgroundImage->copy();
    painter.drawImage(0, 0, image);

    plotHistory(painter, epoch);

    auto rudderTick = toScreen(float(epoch.rdr.getRadians()));
    painter.setPen(RUDDER_PEN);
    painter.drawLine(rudderTick.first, rudderTick.second);

    if( epoch.cmdRdr.isValid(epoch.utc.getUnixTimeMs())){
        std::ostringstream ossAuto;

        ossAuto  << "PILOT " ;
        if ( epoch.cmdRdr.getDegrees() < -0.5 ){
            ossAuto  << "◄";
        }
        ossAuto <<  std::setw(2) << std::setfill(' ') << std::fixed << std::setprecision(0) << abs(epoch.cmdRdr.getDegrees()) << "°";
        if ( epoch.cmdRdr.getDegrees() > 0.5 ){
            ossAuto  << "►";
        }

        painter.setFont(m_PilotFont);
        painter.setPen(AUTO_FONT_PEN);
        painter.drawText(m_pilotTextRect, Qt::AlignCenter, QString::fromStdString(ossAuto.str()));

        auto autoTick = toScreen(float(epoch.cmdRdr.getRadians()));
        painter.setPen(AUTO_PEN);
        painter.drawLine(autoTick.first, autoTick.second);
    }

    std::ostringstream oss;
    if ( epoch.rdr.getDegrees() < -0.5 ){
        oss  << "◄";
    }
    oss  << std::setw(2) << std::setfill(' ') << std::fixed << std::setprecision(0) << abs(epoch.rdr.getDegrees()) << "°";
    if ( epoch.rdr.getDegrees() > 0.5 ){
        oss  << "►";
    }

    painter.setFont(m_RudderFont);
    painter.setPen(RUDDER_FONT_PEN);
    painter.drawText(m_rudderTextRect, Qt::AlignCenter, QString::fromStdString(oss.str()));

}

void RudderOverlayMaker::plotHistory(QPainter &painter, const InstrumentInput &epoch) {

    // Show no more than HIST_DISPLAY_LEN_MS of history
    uint64_t epochUtcMs = epoch.utc.getUnixTimeMs();

    int lastHistIdx;
    for( lastHistIdx =0; lastHistIdx < m_TimeStamps.size(); lastHistIdx++)
        if ( m_TimeStamps[lastHistIdx] >= epochUtcMs )
            break;

    int firstHistoryIdx;
    for( firstHistoryIdx = lastHistIdx; firstHistoryIdx > 0; firstHistoryIdx--){
        uint64_t histUtcMs = m_TimeStamps[firstHistoryIdx];
        if ( epochUtcMs - histUtcMs > HIST_DISPLAY_LEN_MS ){
            break;
        }
    }
    int shownHistoryLen = lastHistIdx - firstHistoryIdx;
    if (shownHistoryLen == 0) shownHistoryLen = 1;

    int minAlpha = 10;
    int maxAlpha = 127;
    int alphaStep = 10;
    for (int i=firstHistoryIdx; i <= lastHistIdx; i++){
        double r = double(i - firstHistoryIdx) / shownHistoryLen; // 0 to 1
        int alpha = minAlpha + int((maxAlpha - minAlpha) * r);
        // Quantize alpha to alphaStep
        alpha = alpha - alpha % alphaStep;
//        std::cout << "alpha: " << alpha << std::endl;

        auto utcMs = m_TimeStamps[i];
        auto pair = toScreen(float(m_history[utcMs].getRadians()));
        QPoint p = pair.second;
        auto historyColor = RUDDER_RUDDER_COLOR;
        historyColor.setAlpha(alpha);
        painter.setBrush(historyColor);

        auto historyPen = QPen(historyColor);
        painter.setPen(historyPen);
        painter.drawEllipse(p, RUDDER_DOT_RADIUS, RUDDER_DOT_RADIUS);
    }


}
