#include <iostream>
#include <cmath>
#include "PolarOverlayMaker.h"

PolarOverlayMaker::PolarOverlayMaker(std::vector<InstrumentInput> &instrDataVector, std::filesystem::path &workDir,
                                     int width, int startIdx, int endIdx, bool ignoreCache)
 :m_rInstrDataVector(instrDataVector),
 m_workDir(workDir), m_width(width), m_ignoreCache(ignoreCache), m_startIdx(startIdx)
 {
    std::filesystem::create_directories(m_workDir);
    m_height = width;
     setHistory(startIdx, endIdx);
}

void PolarOverlayMaker::addEpoch(const std::string &fileName, int epochIdx) {
    std::filesystem::path pngName = std::filesystem::path(m_workDir) / fileName;

    if ( std::filesystem::is_regular_file(pngName) && !m_ignoreCache){
        return ;
    }

    QImage image(m_width, m_height, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));
    QPainter painter(&image);


    // Draw grid

    int startAngle;
    int endAngle;
    int arcStart;
    int arcEnd;
    if ( m_isTack ) {
        startAngle = -90;
        endAngle = 100;
        arcStart = 180;
        arcEnd = 0;
    }else{
        startAngle = 90;
        endAngle = 280;
        arcStart = 0;
        arcEnd = 180;
    }

    auto axisPen = QPen(m_polarGridColor);
    axisPen.setWidth(2);
    painter.setPen(axisPen);
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

    // Draw history
    auto armPen = QPen(Qt::red);
    armPen.setWidth(4);

    int lastHistIdx = epochIdx - m_startIdx;
    for (int i=0; i <= lastHistIdx; i++){
        int a = int(255 * (1 - 1. * (lastHistIdx - i) / (lastHistIdx + 1)));

        if ( isnan(m_history[i].first) || isnan(m_history[i].second))
            continue;

        QPoint p = toScreen(m_history[i]);
        auto historyColor = QColor(255, 0, 0, a);
        painter.setBrush(historyColor);

        if ( i == lastHistIdx ){
            painter.setPen(armPen);
            painter.drawLine(origin, p) ;
            painter.drawEllipse(p, m_dotRadius, m_dotRadius);
        }else{
            auto historyPen = QPen(historyColor);
            painter.setPen(historyPen);
            painter.drawEllipse(p, m_dotRadius, m_dotRadius);
        }
    }

    image.save(QString::fromStdString(pngName.string()), "PNG");

    std::cout << "Created " << pngName << std::endl;
}

void PolarOverlayMaker::setHistory(int startIdx, int endIdx) {
    m_history.clear();

    float twsSum = 0;

    m_maxSpeedKts = 0;
    m_minSpeedKts = 100;

    m_isTack = true;
    for(int i=startIdx; i<endIdx; i++) {
        InstrumentInput &instrData = m_rInstrDataVector[i];

        if (instrData.twa.isValid(instrData.utc.getUnixTimeMs()) && instrData.sow.isValid(instrData.utc.getUnixTimeMs())) {
            auto twaRad = float(instrData.twa.getDegrees() * M_PI / 180);
            auto sowKts = float(instrData.sow.getKnots());
            m_maxSpeedKts = (int)lround(std::max(float(m_maxSpeedKts), sowKts));
            m_minSpeedKts = (int)lround(std::min(float(m_minSpeedKts), sowKts));
            twsSum += sowKts;
            std::pair<float, float> xy = polToCart(sowKts, - twaRad);
            m_history.push_back(xy);

            if( twaRad > M_PI * 0.75  )
                m_isTack = false;

        } else {
            std::pair<float, float> xy = {nanf(""), nanf("")};
            m_history.push_back(xy);
        }
    }

    float meanTws = twsSum / float(endIdx - startIdx);
    m_minSpeedKts = 0;
    m_maxSpeedKts += m_speedStep;

    m_xScale = float( m_width - m_xPad * 2) / float( 2 * (m_maxSpeedKts - m_minSpeedKts) );
    m_yScale = m_xScale;

    m_height = int( m_yScale * float(m_maxSpeedKts - m_minSpeedKts) + float(m_yPad) * 2);
    if ( m_isTack ){
        m_y0 = 0;
    }else{
        m_y0 = m_height - 2 * m_yPad;
    }
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
