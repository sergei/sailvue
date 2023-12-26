#include <cmath>

#include "RudderOverlayMaker.h"

static const char *const FONT_FAMILY = "Courier";

RudderOverlayMaker::RudderOverlayMaker(int width, int height, int x, int y)
:OverlayElement(width, height, x, y)
{
    m_textBoxWidth = width;
    m_textBoxHeight = 50;
    m_rudderBoxWidth = width;
    m_rudderBoxHeight = height - m_textBoxHeight;
    m_textY0 = m_rudderBoxHeight + 5;
    m_textX0 = 0;

    m_RudderOnlyFont = QFont(FONT_FAMILY, getFontSize(FONT_FAMILY, "RUDDER 33º", m_textBoxWidth));
    m_RudderFont = QFont(FONT_FAMILY, getFontSize(FONT_FAMILY, "RUDDER 22º", m_textBoxWidth / 2 - m_textPad));
    m_AutoFont = m_RudderFont;

    m_RudderPen.setWidth(15);
    m_RudderPen.setCapStyle(Qt::RoundCap);

    m_AutoPen.setWidth(10);
    m_AutoPen.setCapStyle(Qt::RoundCap);
}

int RudderOverlayMaker::getFontSize(const char *fontFamily, const char *text, int width) const {
    int fontPointSize;
    QFont font = QFont(fontFamily);
    for(int fs=1; fs < 100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.boundingRect(text);
        if(rect.height() > m_textBoxHeight || rect.width() > width){
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
            float fAngleRad = instrData.rdr.getRadians();
            fMaxAngleRad = std::max(abs(fAngleRad), fMaxAngleRad);
            m_history[utcMs] = float(fAngleRad);
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

    // Flat bottom
//    x = float(m_rudderBoxWidth) / 2 + float(m_rudderBoxHeight - m_rudderY0) * tg;
//    QPoint bottom(int(x), m_rudderBoxHeight);

    // Curved bottom
    x = float(m_rudderBoxWidth) / 2 + float(m_rudderBoxHeight - m_rudderY0) * sin(angleRad);
    float y = float(m_rudderBoxHeight - m_rudderY0) * cos(angleRad) + float(m_rudderY0);
    QPoint bottom((int)x, (int)y);

    return {top, bottom};
}

void RudderOverlayMaker::drawGrid() {
    QPainter painter(m_pBackgroundImage);

    int startAngle = -60;
    int endAngle = 30;

    auto axisPen = QPen(m_gridColor);
    axisPen.setWidth(2);
    painter.setPen(axisPen);

    // Angle lines
    for( int angle = - m_maxAngleDeg; angle <= m_maxAngleDeg; angle += 2 ){
        auto tick = toScreen(angle * M_PI / 180);
        painter.drawLine(tick.first, tick.second);
    }
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

    if( epoch.rdr.isValid(epoch.utc.getUnixTimeMs()) && epoch.cmdRdr.isValid(epoch.utc.getUnixTimeMs())){
        std::ostringstream ossRdr;
        ossRdr << "RUDDER " << std::setw(2) << std::setfill(' ') << std::fixed << std::setprecision(0) << abs(epoch.rdr.getDegrees()) << "°";
        painter.setFont(m_RudderFont);
        painter.setPen(m_RudderFontPen);
        painter.drawText(QRect(m_textX0, m_textY0, m_textBoxWidth/ 2 - m_textPad , m_textBoxWidth), QString::fromStdString(ossRdr.str()));

        auto rudderTick = toScreen(float(epoch.rdr.getRadians()));
        painter.setPen(m_RudderPen);
        painter.drawLine(rudderTick.first, rudderTick.second);

        std::ostringstream ossAuto;
        ossAuto  << "AUTO " <<  std::setw(2) << std::setfill(' ') << std::fixed << std::setprecision(0) << abs(epoch.cmdRdr.getDegrees()) << "°";
        painter.setFont(m_AutoFont);
        painter.setPen(m_AutoFontPen);
        painter.drawText(QRect(m_textX0 +  m_textBoxWidth/ 2 + m_textPad, m_textY0, m_textBoxWidth/ 2 - m_textPad , m_textBoxWidth), QString::fromStdString(ossAuto.str()));

        auto autoTick = toScreen(float(epoch.cmdRdr.getRadians()));
        painter.setPen(m_AutoPen);
        painter.drawLine(autoTick.first, autoTick.second);
    } else {  // Rudder only
        std::ostringstream oss;
        oss  << "RUDDER " <<  std::setw(2) << std::setfill(' ') << std::fixed << std::setprecision(0) << abs(epoch.rdr.getDegrees()) << "°";
        painter.setFont(m_RudderOnlyFont);
        painter.setPen(m_RudderFontPen);
        painter.drawText(QRect(m_textX0, m_textY0, m_textBoxWidth, m_textBoxWidth), QString::fromStdString(oss.str()));

        auto rudderTick = toScreen(float(epoch.rdr.getRadians()));
        painter.setPen(m_RudderPen);
        painter.drawLine(rudderTick.first, rudderTick.second);
    }

}
