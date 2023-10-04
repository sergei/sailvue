#include "TargetsOverlayMaker.h"

TargetsOverlayMaker::TargetsOverlayMaker(Polars &polars, std::vector<InstrumentInput> &instrDataVector,
                                         int width, int height, int startIdx,
                                         int endIdx, bool ignoreCache)
 :m_rInstrDataVector(instrDataVector),m_polars(polars)
 ,m_width(width), m_height(height), m_ignoreCache(ignoreCache)
 ,m_speedStrip("SPD", 0, 0, m_width, m_height/2-10, startIdx, endIdx, 90 , 50, 150)
 ,m_vmgStrip("VMG", 0, m_height/2 + 5, m_width, m_height/2-10, startIdx, endIdx, 90 , 50, 150)
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

void TargetsOverlayMaker::addChapter(std::filesystem::path workDir, int startIdx, int endIdx) {
    m_workDir = workDir;
    std::filesystem::create_directories(m_workDir);
    m_speedStrip.setChapter(startIdx, endIdx);
    m_vmgStrip.setChapter(startIdx, endIdx);
}

void TargetsOverlayMaker::addEpoch(const std::string &fileName, int epochIdx) {
    std::filesystem::path pngName = std::filesystem::path(m_workDir) / fileName;

    if ( std::filesystem::is_regular_file(pngName) && !m_ignoreCache){
        return ;
    }
    if ( m_pBackgroundImage == nullptr ){
        std::cout << "PolarOverlayMaker::addEpoch() called after destructor" << std::endl;
        return;
    }

    QImage image = m_pBackgroundImage->copy();
    QPainter painter(&image);

    m_speedStrip.drawCurrent(painter, epochIdx);
    m_vmgStrip.drawCurrent(painter, epochIdx);

    image.save(QString::fromStdString(pngName.string()), "PNG");
}


void TargetsOverlayMaker::makeBaseImage(int startIdx, int endIdx){

    QPainter painter(m_pBackgroundImage);
    for( int i = startIdx; i < endIdx; i++){

        InstrumentInput &instr = m_rInstrDataVector[i];
        bool spdIsValid = instr.sow.isValid(instr.utc.getUnixTimeMs())
                && instr.twa.isValid(instr.utc.getUnixTimeMs())
                && instr.tws.isValid(instr.utc.getUnixTimeMs());
        if ( spdIsValid) {
            double spd = instr.sow.getKnots();
            double vmg = abs(spd * cos(instr.twa.getRadians()));
            double targetSpeed = m_polars.getSpeed(instr.twa.getDegrees(), instr.tws.getKnots());
            std::pair<double, double> targets =  m_polars.getTargets(instr.tws.getKnots(), instr.twa.getDegrees() < 90);
            double targetVmg = abs(targets.second);
            m_speedStrip.addSample(spd / targetSpeed * 100, true);
            m_vmgStrip.addSample(vmg / targetVmg * 100, true);
        }else{
            m_speedStrip.addSample(0, false);
            m_vmgStrip.addSample(0, false);
        }
    }

    m_speedStrip.drawBackground(painter);
    m_vmgStrip.drawBackground(painter);
}

Strip::Strip(const std::string &label, int x, int y, int width, int height, int startIdx, int endIdx,
             double threshold, double floor, double ceiling)
        :m_label(label), m_x0(x), m_y0(y), m_width(width), m_height(height), m_startIdx(startIdx), m_endIdx(endIdx),
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

    int ptsNum = endIdx - startIdx;
    m_xScale = double(m_width - m_labelWidth * 2) / double(ptsNum);

    m_maxValue = threshold;
    m_minValue = threshold;

    m_axisPen.setWidth(2);
    m_goodPen.setWidth(3);
    m_badPen.setWidth(3);
    m_tickPen.setWidth(6);
}

void Strip::addSample(double value, bool isValid) {
    if( isValid ) {
        value = std::min(value, m_ceiling);
        value = std::max(value, m_floor);
        m_maxValue = std::max(m_maxValue, value);
        m_minValue = std::min(m_minValue, value);
        m_values.push_back(value);
    } else {
        m_values.push_back(nan(""));
    }
}

void Strip::drawBackground(QPainter &painter) {
    m_yScale = double(m_height) / double(m_maxValue - m_minValue);

    // Draw label
    painter.setFont(m_labelFont);
    painter.setPen(m_labelPen);
    painter.drawText(m_x0, m_y0 + m_height, QString::fromStdString(m_label));

    painter.setPen(m_axisPen);
    // Draw 100% lne
    QPoint from = toScreen(m_startIdx, 100);
    QPoint to = toScreen(m_endIdx, 100);
    painter.drawLine(from, to);

    // Draw threshold horizontal line
    from = toScreen(m_startIdx, m_threshold);
    to = toScreen(m_endIdx, m_threshold);
    painter.drawLine(from, to);

    for( int idx = m_startIdx; idx < m_endIdx; idx++) {
        double val = m_values[idx - m_startIdx];
        QPen *pen = val < m_threshold ? &m_badPen : &m_okPen;
        if (val >=100 ) pen = &m_goodPen;
        painter.setPen(*pen );
        QPoint p = toScreen(idx, val);
        painter.drawPoint(p);
    }

}

void Strip::drawCurrent(QPainter &painter, int idx) {

    // Draw value
    painter.setFont(m_labelFont);
    painter.setPen(m_labelPen);
    double val = m_values[idx - m_startIdx];
    if ( std::isnan(val) ){
        painter.drawText(m_x0 + m_width - m_labelWidth, m_y0 + m_height, "--");
    } else {
        QString valStr = QString::fromStdString(std::to_string(int(val))) + "%";
        painter.drawText(m_x0 + m_width - m_labelWidth, m_y0 + m_height, valStr);
    }

    // Draw tick mark
    QPoint from = toScreen(idx, m_maxValue);
    QPoint to = toScreen(idx, m_minValue);
    painter.setPen(m_tickPen);
    painter.drawLine(from, to);

    // Shade out of chapter area
    painter.setPen(m_outOfChapterPen);
    painter.setBrush(m_outOfChapterBrush);
    QPoint ul = toScreen(m_startIdx, m_maxValue);
    QPoint lr = toScreen(m_chapterStartIdx, m_minValue);
    QRect rect(ul, lr);
    painter.drawRect(rect);
    ul = toScreen(m_chapterEndIdx, m_maxValue);
    lr = toScreen(m_endIdx, m_minValue);
    rect = QRect(ul, lr);
    painter.drawRect(rect);

}

QPoint Strip::toScreen(int idx, double y) const {
    int sx = m_x0 + m_labelWidth + int((idx - m_startIdx) * m_xScale);
    int sy = m_y0 + int( (m_maxValue - y) * m_yScale);
    return {sx, sy};
}

void Strip::setChapter(int startIdx, int endIdx) {
    m_chapterStartIdx = startIdx;
    m_chapterEndIdx = endIdx;

}
