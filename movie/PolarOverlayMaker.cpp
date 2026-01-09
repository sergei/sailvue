#include <iostream>
#include <cmath>
#include "PolarOverlayMaker.h"
#include "ColorPalette.h"

PolarOverlayMaker::PolarOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrDataVector,
                                     int width, int height, int x, int y)
 :OverlayElement(width, height, x, y),
 m_polars(polars), m_rInstrDataVector(instrDataVector)
 {
    m_height = width;
 }

PolarOverlayMaker::~PolarOverlayMaker() {
    delete m_pBackgroundImage;
    delete m_PolarCurveImage;
}

void PolarOverlayMaker::setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) {
    delete m_pBackgroundImage;
    m_pBackgroundImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_pBackgroundImage->fill(QColor(0, 0, 0, 0));
    delete m_PolarCurveImage;
    m_PolarCurveImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_PolarCurveImage->fill(QColor(0, 0, 0, 0));
    setHistory(chapterEpochs);
}


void PolarOverlayMaker::addEpoch(QPainter &painter, const InstrumentInput &epoch) {

    if ( m_pBackgroundImage == nullptr ){
        std::cout << "PolarOverlayMaker::addEpoch() called before setChapter" << std::endl;
        return;
    }

    QImage image = m_pBackgroundImage->copy();
    painter.drawImage(0, 0, image);

    // Draw the autopilot target TWA
    if (epoch.pilotTwa.isValid(epoch.utc.getUnixTimeMs())) {
        auto twaRad = float(epoch.pilotTwa.getDegrees() * M_PI / 180);
        auto xy = polToCart(float(m_maxSpeedKts), - twaRad);
        QPoint p = toScreen(xy);
        painter.setPen(POLAR_PILOT_TWA_PEN);
        painter.drawLine(m_origin, p) ;
    }

    // Draw history
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

    int minAlpha = 64;
    for (int i=firstHistoryIdx; i <= lastHistIdx; i++){
        double r = double(i - firstHistoryIdx) / shownHistoryLen; // 0 to 1
        int alpha = minAlpha + int((255 - minAlpha)  * r);

        auto utcMs = m_TimeStamps[i];

        if ( isnan(m_history[utcMs].first) || isnan(m_history[utcMs].second))
            continue;

        QPoint p = toScreen(m_history[utcMs]);
        auto historyColor = POLAR_HISTORY_COLOR;
        historyColor.setAlpha(alpha);
        painter.setBrush(historyColor);

        if ( i == lastHistIdx ){
            painter.setPen(POLAR_ARM_PEN);
            painter.drawLine(m_origin, p) ;
            painter.drawEllipse(p, m_dotRadius, m_dotRadius);
        }else{
            auto historyPen = QPen(historyColor);
            painter.setPen(historyPen);
            painter.drawEllipse(p, m_dotRadius, m_dotRadius);
        }
    }

    // Copy polar curve on top of history
    painter.drawImage(0, 0, *m_PolarCurveImage);
}

void PolarOverlayMaker::draw_grid_and_polar_curve() {

    // Compute min/max values and other statistics using the history containers
    m_maxSpeedKts = 0;
    m_minSpeedKts = 100;
    m_minTwa = 20;
    m_maxTwa = -1;
    float twsSum = 0;
    int iTwsCount = 0;
    for (size_t i = 0; i < m_twaHistory.size(); ++i) {
        float twa = m_twaHistory[i];
        float sow = m_sowHistory[i];
        float tws = m_twsHistory[i];
        if (!std::isnan(twa) && !std::isnan(sow)) {
            m_minTwa = std::min(m_minTwa, twa);
            m_maxTwa = std::max(m_maxTwa, twa);
            m_maxSpeedKts = (int)lround(std::max(float(m_maxSpeedKts), sow));
            m_minSpeedKts = (int)lround(std::min(float(m_minSpeedKts), sow));
            twsSum += tws;
            iTwsCount++;
        }
    }

    float meanTws = iTwsCount > 0 ? twsSum / float(iTwsCount) : 0;

    // Check case when the polar curve speed exceeds the max speed
    for( int twaDeg =0; twaDeg <= 180; twaDeg += 1 ) {
        auto targetSpd = (float)m_polars.getSpeed(twaDeg, meanTws);
        m_maxSpeedKts = (int)lround(std::max(float(m_maxSpeedKts), targetSpd));
    }

    m_minSpeedKts = 0;
    m_maxSpeedKts = ( m_maxSpeedKts / m_speedStep ) * m_speedStep;
    m_maxSpeedKts += m_speedStep;

    // Determine scale and origin
    m_xScale = float( m_width - m_xPad * 2) / float( 2 * (m_maxSpeedKts - m_minSpeedKts) );
    m_yScale = m_xScale;
    WhatToShow whatToShow = UNKOWN;
    if ( m_maxTwa <= 90 ){        // Top half only
        m_y0 = 0;
        m_height = int( m_yScale * float(m_maxSpeedKts - m_minSpeedKts) + float(m_yPad) * 2);
        m_showTopHalf = true;
        whatToShow = SHOW_TOP_HALF;
    }else if ( m_minTwa >= 90 ){  // Bottom half only
        m_y0 = m_height - 2 * m_yPad;
        m_height = int( m_yScale * float(m_maxSpeedKts - m_minSpeedKts) + float(m_yPad) * 2);
        m_showBottomHalf = true;
        whatToShow = SHOW_BOTTOM_HALF;
    }else {  // Full circle
        m_height = m_width;
        m_y0 = m_height / 2 - m_yPad;
        m_showTopHalf = true;
        m_showBottomHalf = true;
        whatToShow = SHOW_FULL_POLAR;
    }

    if (whatToShow != m_whatToShow) {
        m_whatToShow = whatToShow;
        delete m_pBackgroundImage;
        m_pBackgroundImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
        m_pBackgroundImage->fill(QColor(0, 0, 0, 0));
        m_origin = drawGrid();
    }

    if ( abs(m_lastMeanTws - meanTws) > 2 ) {
        m_lastMeanTws = meanTws;
        delete m_PolarCurveImage;
        m_PolarCurveImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
        m_PolarCurveImage->fill(QColor(0, 0, 0, 0));
        drawPolarCurve(meanTws);
    }
}

void PolarOverlayMaker::setHistory(const std::list<InstrumentInput> &chapterEpochs) {
    m_history.clear();
    m_TimeStamps.clear();
    m_twaHistory.clear();
    m_sowHistory.clear();

    // First, initialize m_history, m_TimeStamps, m_twaHistory, m_sowHistory
    for (const auto &instrData : chapterEpochs) {
        auto utcMs = instrData.utc.getUnixTimeMs();
        if (instrData.twa.isValid(utcMs) && instrData.sow.isValid(utcMs)) {
            std::pair<float, float> xy = polToCart(float(instrData.sow.getKnots()), -float(instrData.twa.getDegrees() * M_PI / 180));
            m_history[utcMs] = xy;
            m_twaHistory.push_back(abs(instrData.twa.getDegrees()));
            m_sowHistory.push_back(float(instrData.sow.getKnots()));
            m_twsHistory.push_back(float(instrData.tws.getKnots()));
        } else {
            m_history[utcMs] = {nanf(""), nanf("")};
            m_twaHistory.push_back(std::numeric_limits<float>::quiet_NaN());
            m_sowHistory.push_back(std::numeric_limits<float>::quiet_NaN());
            m_twsHistory.push_back(std::numeric_limits<float>::quiet_NaN());
        }
        m_TimeStamps.push_back(utcMs);
    }

    draw_grid_and_polar_curve();

}

void PolarOverlayMaker::initHistory(std::vector<InstrumentInput> &rInstrDataVector) {
    delete m_pBackgroundImage;
    m_pBackgroundImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_pBackgroundImage->fill(QColor(0, 0, 0, 0));
    delete m_PolarCurveImage;
    m_PolarCurveImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_PolarCurveImage->fill(QColor(0, 0, 0, 0));

    // Init chapter epochs from rInstrDataVector using either MAX_HISTORY_SIZE or size of rInstrDataVector
    size_t historySize = std::min(MAX_HISTORY_SIZE, rInstrDataVector.size());
    std::list chapterEpochs(rInstrDataVector.begin(), rInstrDataVector.begin() + historySize);
    setHistory(chapterEpochs);
}

void PolarOverlayMaker::updateHistory(const InstrumentInput &epoch) {
    auto utcMs = epoch.utc.getUnixTimeMs();
    // Remove oldest if at max size
    if (m_TimeStamps.size() >= MAX_HISTORY_SIZE) {
        uint64_t oldestUtc = m_TimeStamps.front();
        m_TimeStamps.erase(m_TimeStamps.begin());
        m_history.erase(oldestUtc);
        m_twaHistory.erase(m_twaHistory.begin());
        m_sowHistory.erase(m_sowHistory.begin());
    }
    // Add new epoch
    if (epoch.twa.isValid(utcMs) && epoch.sow.isValid(utcMs)) {
        std::pair<float, float> xy = polToCart(float(epoch.sow.getKnots()), -float(epoch.twa.getDegrees() * M_PI / 180));
        m_history[utcMs] = xy;
        m_twaHistory.push_back(abs(epoch.twa.getDegrees()));
        m_sowHistory.push_back(float(epoch.sow.getKnots()));
    } else {
        m_history[utcMs] = {nanf(""), nanf("")};
        m_twaHistory.push_back(std::numeric_limits<float>::quiet_NaN());
        m_sowHistory.push_back(std::numeric_limits<float>::quiet_NaN());
    }
    m_TimeStamps.push_back(utcMs);

    draw_grid_and_polar_curve();

}


std::pair<float, float> PolarOverlayMaker::polToCart(float rho, float thetaRad) {
    float y =  rho * cos(thetaRad);
    float x =  rho * sin(thetaRad);
    return {x, y};
}

QPoint PolarOverlayMaker::toScreen(const std::pair<float, float> &xy) const {
    float x = xy.first;
    float y = xy.second;

    int screenX = int(x * m_xScale) + m_width / 2 ;
    int screenY = - int(y * m_yScale) + m_height - m_yPad -  m_y0 ;

    return {screenX, screenY};
}

QPoint PolarOverlayMaker::drawGrid() {
    QPainter painter(m_pBackgroundImage);

    int startAngle;
    int endAngle;
    int arcStart;
    int arcEnd;
    if ( m_maxTwa <= 90 ) { // Top half only
        startAngle = -90;
        endAngle = 100;
        arcStart = 180;
        arcEnd = 0;
    }else if ( m_minTwa >= 90 ){  // Bottom half only
        startAngle = 90;
        endAngle = 280;
        arcStart = 0;
        arcEnd = 180;
    }else {  // Full circle
        startAngle = 0;
        endAngle = 360;
        arcStart = 0;
        arcEnd = 359;
    }

    painter.setPen(POLAR_GRID_PEN);
    // Speed circles
    for( int speed = m_minSpeedKts; speed <= m_maxSpeedKts; speed += m_speedStep ){
        QPoint ul = toScreen({-speed, speed});  // Top left
        QPoint  lr = toScreen({speed, -speed});  // Bottom right
        painter.drawArc(QRectF(ul, lr), arcStart * 16, (arcEnd - arcStart) * 16);
    }

    QPoint origin  = toScreen(polToCart(float(m_minSpeedKts), 0));

    // Angle lines
    for( int angle = startAngle; angle < endAngle; angle += 30 ){
        auto rad = float(angle * M_PI / 180);
        QPoint p = toScreen(polToCart(float(m_maxSpeedKts), rad));
        painter.drawLine(origin, p);
    }

    return origin;
}

void PolarOverlayMaker::drawPolarCurve(float tws) {
    QPainter painter(m_PolarCurveImage);

    painter.setPen(POLAR_CURVE_PEN);

    QPoint prevPt;
    bool isFirst = true;

    for(int twa = -90; twa <= 270; twa += 1 ){
        int twaDeg = twa > 180 ? twa - 360 : twa;

        if (abs(twaDeg) < m_polars.getMinTwa() || abs(twaDeg) > m_polars.getMaxTwa()){  // Skip the no sail zone
            isFirst = true;
            continue;
        }

        if (abs(twaDeg) <= 90 && !m_showTopHalf ){  // Stay within zoom level
            isFirst = true;
            continue;
        }

        if (abs(twaDeg) >= 90 && !m_showBottomHalf ){  // Stay within zoom level
            isFirst = true;
            continue;
        }

        auto twaRad = float(twaDeg * M_PI / 180);
        auto spd = m_polars.getSpeed(twaDeg, tws);
        QPoint p = toScreen(polToCart(float(spd), - twaRad));
        if( !isFirst ){
            painter.drawLine(prevPt, p);
        }
        prevPt = p;
        isFirst = false;
    }

}
