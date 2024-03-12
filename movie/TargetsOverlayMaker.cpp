#include "TargetsOverlayMaker.h"

TargetsOverlayMaker::TargetsOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrDataVector,
                                         int width, int height, int x, int y, int startIdx, int endIdx)
 :OverlayElement(width, height, x, y)
 ,m_rInstrDataVector(instrDataVector),m_polars(polars)
 ,m_speedStrip("SPD", 0, 0, m_width, m_height/2-10,
               instrDataVector[startIdx].utc.getUnixTimeMs(), instrDataVector[endIdx].utc.getUnixTimeMs(),
               90 , 50, 150)
 ,m_vmgStrip("VMG", 0, m_height/2 + 5, m_width, m_height/2-10,
             instrDataVector[startIdx].utc.getUnixTimeMs(), instrDataVector[endIdx].utc.getUnixTimeMs(),
             90 , 50, 150)
 {
    m_pBackgroundImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_pBackgroundImage->fill(QColor(0, 0, 0, 0));
    m_pHighLightImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_pHighLightImage->fill(QColor(0, 0, 0, 0));
    makeBaseImage(startIdx, endIdx);
}

TargetsOverlayMaker::~TargetsOverlayMaker() {
    delete m_pBackgroundImage;
    delete m_pHighLightImage;
}

void TargetsOverlayMaker::setChapter(Chapter &chapter, const std::list<InstrumentInput> &chapterEpochs) {
    uint64_t startTime = chapterEpochs.front().utc.getUnixTimeMs();
    uint64_t endTime = chapterEpochs.back().utc.getUnixTimeMs();
    m_speedStrip.setChapter(startTime, endTime);
    m_vmgStrip.setChapter(startTime, endTime);
}

void TargetsOverlayMaker::addEpoch(QPainter &painter, const InstrumentInput &epoch) {

    if ( m_pBackgroundImage == nullptr ){
        std::cout << "PolarOverlayMaker::addEpoch() called after destructor" << std::endl;
        return;
    }

    QImage image = m_pBackgroundImage->copy();
    painter.drawImage(0, 0, image);

    m_speedStrip.drawCurrent(painter, epoch);
    m_vmgStrip.drawCurrent(painter, epoch);
}


void TargetsOverlayMaker::makeBaseImage(int startIdx, int endIdx){

    QPainter painter(m_pBackgroundImage);
    for( int i = startIdx; i < endIdx; i++){

        InstrumentInput &instr = m_rInstrDataVector[i];
        auto utcMs = instr.utc.getUnixTimeMs();
        bool spdIsValid = instr.sow.isValid(utcMs)
                          && instr.twa.isValid(utcMs)
                          && instr.tws.isValid(utcMs);
        if ( spdIsValid) {
            double spd = instr.sow.getKnots();
            double vmg = abs(spd * cos(instr.twa.getRadians()));
            double targetSpeed = m_polars.getSpeed(instr.twa.getDegrees(), instr.tws.getKnots());
            std::pair<double, double> targets =  m_polars.getTargets(instr.tws.getKnots(), instr.twa.getDegrees() < 90);
            double targetVmg = abs(targets.second);
            m_speedStrip.addSample(utcMs, spd / targetSpeed * 100, true);
            m_vmgStrip.addSample(utcMs, vmg / targetVmg * 100, true);
        }else{
            m_speedStrip.addSample(utcMs, 0, false);
            m_vmgStrip.addSample(utcMs, 0, false);
        }
    }

    m_speedStrip.drawBackground(painter);
    m_vmgStrip.drawBackground(painter);
}

Strip::Strip(const std::string &label, int x, int y, int width, int height, uint64_t startTime, uint64_t endTime,
             double threshold, double floor, double ceiling)
        :m_label(label), m_x0(x), m_y0(y), m_width(width), m_height(height), m_startTime(startTime), m_endTime(endTime),
        m_threshold(threshold), m_floor(floor), m_ceiling(ceiling)
{
    int fontPointSize;
    for(int fs=10; fs <100; fs ++){
        fontPointSize = fs;
        m_labelFont.setPointSize(fs);
        QFontMetrics fm(m_labelFont);
        QRect rect = fm.boundingRect("SPDX");
        m_labelWidth = rect.width();
        if(rect.height() > m_height){
            fontPointSize = fs - 1;
            break;
        }
    }

    m_labelFont.setPointSize(fontPointSize);

    uint64_t ptsNum = endTime - startTime;
    m_xScale = double(m_width - m_labelWidth * 2) / double(ptsNum);

    m_maxValue = threshold;
    m_minValue = threshold;

    m_axisPen.setWidth(2);
    m_goodPen.setWidth(3);
    m_badPen.setWidth(3);
    m_tickPen.setWidth(6);
}

void Strip::addSample(uint64_t t, double value, bool isValid) {
    if( isValid ) {
        value = std::min(value, m_ceiling);
        value = std::max(value, m_floor);
        m_maxValue = std::max(m_maxValue, value);
        m_minValue = std::min(m_minValue, value);
        m_values[t] = value;
    } else {
        m_values[t] = nan("");
    }
}

void Strip::drawBackground(QPainter &painter) {
    m_yScale = double(m_height) / double(m_maxValue - m_minValue);

    // Draw label
    painter.setFont(m_labelFont);
    painter.setPen(m_labelPen);
    int labelX = m_x0;
    int labelY = m_y0 + m_height;
    painter.drawText(labelX, labelY, QString::fromStdString(m_label));

//    std::cout << "Painting label with font " << m_labelFont.toString().toStdString() << std::endl;
//    std::cout << "Painting label [" << m_label << "] at x= " << labelX << ", y=" << labelY << std::endl;

    painter.setPen(m_axisPen);
    // Draw 100% lne
    QPoint from = toScreen(m_startTime, 100);
    QPoint to = toScreen(m_endTime, 100);
    painter.drawLine(from, to);

    // Draw threshold horizontal line
    from = toScreen(m_startTime, m_threshold);
    to = toScreen(m_endTime, m_threshold);
    painter.drawLine(from, to);

    for(auto & mapEntry : m_values) {
        double val = mapEntry.second;
        uint64_t t = mapEntry.first;
        QPen *pen = val < m_threshold ? &m_badPen : &m_okPen;
        if (val >=100 ) pen = &m_goodPen;
        painter.setPen(*pen );
        QPoint p = toScreen(t, val);
        painter.drawPoint(p);
    }

}

void Strip::drawCurrent(QPainter &painter, const InstrumentInput &epoch) {

    // Draw value
    painter.setFont(m_labelFont);
    painter.setPen(m_labelPen);
    auto utcMs = epoch.utc.getUnixTimeMs();
    double val = m_values[utcMs];
    if ( std::isnan(val) ){
        painter.drawText(m_x0 + m_width - m_labelWidth, m_y0 + m_height, "--");
    } else {
        QString valStr = QString::fromStdString(std::to_string(int(val))) + "%";
        painter.drawText(m_x0 + m_width - m_labelWidth, m_y0 + m_height, valStr);
    }

    // Draw tick mark
    QPoint from = toScreen(utcMs, m_maxValue);
    QPoint to = toScreen(utcMs, m_minValue);
    painter.setPen(m_tickPen);
    painter.drawLine(from, to);

    // Shade out of chapter area
    painter.setPen(m_outOfChapterPen);
    painter.setBrush(m_outOfChapterBrush);
    QPoint ul = toScreen(m_startTime, m_maxValue);
    QPoint lr = toScreen(m_chapterStartTime, m_minValue);
    QRect rect(ul, lr);
    painter.drawRect(rect);
    ul = toScreen(m_chapterEndTime, m_maxValue);
    lr = toScreen(m_endTime, m_minValue);
    rect = QRect(ul, lr);
    painter.drawRect(rect);

}

QPoint Strip::toScreen(uint64_t idx, double y) const {
    int sx = m_x0 + m_labelWidth + int((idx - m_startTime) * m_xScale);
    int sy = m_y0 + int( (m_maxValue - y) * m_yScale);
    return {sx, sy};
}

void Strip::setChapter(uint64_t startTime, uint64_t endTime) {
    m_chapterStartTime = startTime;
    m_chapterEndTime = endTime;
}
